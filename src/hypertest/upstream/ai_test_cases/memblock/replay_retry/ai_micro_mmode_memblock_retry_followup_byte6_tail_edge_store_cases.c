#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep the BYTE6 tail-edge follow-up stores separate from the 1B+7B point7
 * cases so the 2B+6B stale high-half mask suspicion can evolve independently.
 */

#define AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET 14U

static inline pte_t *ai_micro_mab_retry_byte6_tail_edge_store_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_byte6_tail_edge_store_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_byte6_tail_edge_store_hs_leaf_entry(page) =
        (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf_paddr(
        page, phys_page_base(page), flags);
}

static inline void
ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void
ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_RWX);
}

static inline void
ai_micro_mab_retry_byte6_tail_edge_store_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void
ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_RWX |
        (deny_cfg << 8) |
        (AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_byte6_tail_edge_store_reset_env(void)
{
    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();
    ai_micro_mab_retry_byte6_tail_edge_store_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
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
ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
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

#define AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION( \
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

AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd, "sd", uint64_t)
AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sb, "sb", uint8_t)
AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sh, "sh", uint16_t)
AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sw, "sw", uint32_t)

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_first_out_of_range_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x86, 0x75, 0x64, 0x53, 0x42, 0x31, 0x20, 0x1f,
        0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77,
    };
    static const uint8_t guard_before_seed[8] = {
        0x14, 0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc3, 0xd5, 0xe7, 0xf9, 0x0b, 0x1d, 0x2f, 0x31,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_first_out_of_range_byte_store_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5162738495a6b7c8ULL;
    const uint64_t refault_store_value = 0xd1e2f30415263748ULL;
    const uint64_t aligned_upper_store_value_first = 0x2a3b4c5d6e7f8091ULL;
    const uint64_t aligned_upper_store_value_second = 0xa2b3c4d5e6f70819ULL;
    const uint64_t aligned_upper_store_value_third = 0x3b4c5d6e7f8091a2ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xb3c4d5e6f708192aULL;
    const uint64_t retry_2b6b_store_value = 0x748596a7b8c9daebULL;
    const uint8_t followup_upper_tail_first_out_of_range_byte_store_value =
        0x6dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate first-out-of-range tail-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sb(
        followup_upper_tail_first_out_of_range_byte_store_vaddr,
        followup_upper_tail_first_out_of_range_byte_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate first-out-of-range tail-byte translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge beyond the semantic BYTE6 window",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 14U,
        (uint64_t)followup_upper_tail_first_out_of_range_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate first-out-of-range tail-byte store, the final boundary bytes should preserve bytes6-13 from the retried image while refreshing only byte14 and keeping byte15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate first-out-of-range tail-byte follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_first_out_of_range_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_second_out_of_range_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x97, 0x86, 0x75, 0x64, 0x53, 0x42, 0x31, 0x20,
        0x1f, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88,
    };
    static const uint8_t guard_before_seed[8] = {
        0x15, 0x27, 0x39, 0x4b, 0x5d, 0x6f, 0x71, 0x83,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e, 0x30, 0x42,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_second_out_of_range_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x62738495a6b7c8d9ULL;
    const uint64_t refault_store_value = 0xe2f3041526374859ULL;
    const uint64_t aligned_upper_store_value_first = 0x3b4c5d6e7f8091a2ULL;
    const uint64_t aligned_upper_store_value_second = 0xb3c4d5e6f708192aULL;
    const uint64_t aligned_upper_store_value_third = 0x4c5d6e7f8091a2b3ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xc4d5e6f708192a3bULL;
    const uint64_t retry_2b6b_store_value = 0x8596a7b8c9daebfcULL;
    const uint8_t followup_upper_tail_second_out_of_range_byte_store_value =
        0x7eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate second-out-of-range tail-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sb(
        followup_upper_tail_second_out_of_range_byte_store_vaddr,
        followup_upper_tail_second_out_of_range_byte_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate second-out-of-range tail-byte translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge beyond the semantic BYTE6 window",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_upper_tail_second_out_of_range_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate second-out-of-range tail-byte store, the final boundary bytes should preserve bytes6-13 from the retried image while refreshing only byte15 and keeping byte14 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate second-out-of-range tail-byte follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_second_out_of_range_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_out_of_range_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa8, 0x97, 0x86, 0x75, 0x64, 0x53, 0x42, 0x31,
        0x20, 0x1f, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99,
    };
    static const uint8_t guard_before_seed[8] = {
        0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe5, 0xf7, 0x09, 0x1b, 0x2d, 0x3f, 0x41, 0x53,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_out_of_range_halfword_store_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x738495a6b7c8d9eaULL;
    const uint64_t refault_store_value = 0xf30415263748596aULL;
    const uint64_t aligned_upper_store_value_first = 0x4e5f708192a3b4c5ULL;
    const uint64_t aligned_upper_store_value_second = 0xd6e7f8091a2b3c4dULL;
    const uint64_t aligned_upper_store_value_third = 0x5f708192a3b4c5d6ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xe7f8091a2b3c4d5eULL;
    const uint64_t retry_2b6b_store_value = 0x96a7b8c9daebfc0dULL;
    const uint16_t followup_upper_tail_out_of_range_halfword_store_value =
        0x8f7eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the out-of-range tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate out-of-range tail-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sh(
        followup_upper_tail_out_of_range_halfword_store_vaddr,
        followup_upper_tail_out_of_range_halfword_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate out-of-range tail-halfword translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge beyond the semantic BYTE6 window",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 14U,
        (uint64_t)followup_upper_tail_out_of_range_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate out-of-range tail-halfword store, the final boundary bytes should preserve bytes6-13 from the retried image while refreshing only bytes14-15",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate out-of-range tail-halfword follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_out_of_range_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_overlapping_out_of_range_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xca, 0xb9, 0xa8, 0x97, 0x86, 0x75, 0x64, 0x53,
        0x42, 0x31, 0x20, 0x1f, 0xee, 0xdd, 0xcc, 0xbb,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74, 0x86,
    };
    static const uint8_t guard_after_seed[8] = {
        0x07, 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t
        followup_upper_tail_overlapping_out_of_range_word_store_vaddr =
            aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x95a6b7c8d9eafb0cULL;
    const uint64_t refault_store_value = 0x263748596a7b8c9dULL;
    const uint64_t aligned_upper_store_value_first = 0x708192a3b4c5d6e7ULL;
    const uint64_t aligned_upper_store_value_second = 0xf8091a2b3c4d5e6fULL;
    const uint64_t aligned_upper_store_value_third = 0x8192a3b4c5d6e7f8ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x091a2b3c4d5e6f70ULL;
    const uint64_t retry_2b6b_store_value = 0xb8c9daebfc0d1e2fULL;
    const uint32_t
        followup_upper_tail_overlapping_out_of_range_word_store_value =
            0x6a7b8c9dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then overlapping out-of-range tail-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then overlapping out-of-range tail-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then overlapping out-of-range tail-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then overlapping out-of-range tail-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then overlapping out-of-range tail-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then overlapping out-of-range tail-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the overlapping out-of-range tail-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate overlapping out-of-range tail-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sw(
        followup_upper_tail_overlapping_out_of_range_word_store_vaddr,
        followup_upper_tail_overlapping_out_of_range_word_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate overlapping out-of-range tail-word translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half upper-tail window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 12U,
        (uint64_t)
            followup_upper_tail_overlapping_out_of_range_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate overlapping out-of-range tail-word store, the final boundary bytes should preserve bytes6-11 from the retried image while refreshing only bytes12-15",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate overlapping out-of-range tail-word follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_overlapping_out_of_range_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_bridge_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xdc, 0xcb, 0xba, 0xa9, 0x98, 0x87, 0x76, 0x65,
        0x54, 0x43, 0x32, 0x21, 0x10, 0xef, 0xde, 0xcd,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0x09, 0x1b, 0x2d, 0x3f, 0x51, 0x63, 0x75, 0x87,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_bridge_byte_store_vaddr =
        aligned_upper_store_vaddr + 5U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xa6b7c8d9eafb0c1dULL;
    const uint64_t refault_store_value = 0x3748596a7b8c9daeULL;
    const uint64_t aligned_upper_store_value_first = 0x8192a3b4c5d6e7f8ULL;
    const uint64_t aligned_upper_store_value_second = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t aligned_upper_store_value_third = 0x92a3b4c5d6e7f809ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1b2c3d4e5f607182ULL;
    const uint64_t retry_2b6b_store_value = 0xc9daebfc0d1e2f30ULL;
    const uint8_t followup_upper_tail_bridge_byte_store_value = 0x7cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then bridge-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then bridge-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then bridge-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then bridge-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then bridge-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then bridge-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the bridge-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate bridge-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sb(
        followup_upper_tail_bridge_byte_store_vaddr,
        followup_upper_tail_bridge_byte_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate bridge-byte translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window around the BYTE6 semantic edge",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 13U,
        (uint64_t)followup_upper_tail_bridge_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate bridge-byte store, the final boundary bytes should preserve bytes6-12 from the retried image while refreshing only byte13 and keeping bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate bridge-byte follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_bridge_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_exact_upper_tail_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xed, 0xdc, 0xcb, 0xba, 0xa9, 0x98, 0x87, 0x76,
        0x65, 0x54, 0x43, 0x32, 0x21, 0x10, 0xff, 0xee,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1c, 0x2e, 0x30, 0x42, 0x54, 0x66, 0x78, 0x8a,
    };
    static const uint8_t guard_after_seed[8] = {
        0x0b, 0x1d, 0x2f, 0x41, 0x53, 0x65, 0x77, 0x89,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_exact_upper_tail_halfword_store_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xb7c8d9eafb0c1d2eULL;
    const uint64_t refault_store_value = 0x48596a7b8c9daebfULL;
    const uint64_t aligned_upper_store_value_first = 0xa3b4c5d6e7f8091aULL;
    const uint64_t aligned_upper_store_value_second = 0x2c3d4e5f60718293ULL;
    const uint64_t aligned_upper_store_value_third = 0xb4c5d6e7f8091a2bULL;
    const uint64_t aligned_upper_store_value_fourth = 0x3d4e5f60718293a4ULL;
    const uint64_t retry_2b6b_store_value = 0xdaebfc0d1e2f3041ULL;
    const uint16_t followup_exact_upper_tail_halfword_store_value = 0x8d7cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then exact upper-tail halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then exact upper-tail halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then exact upper-tail halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then exact upper-tail halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then exact upper-tail halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then exact upper-tail halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the exact upper-tail halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate exact upper-tail halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sh(
        followup_exact_upper_tail_halfword_store_vaddr,
        followup_exact_upper_tail_halfword_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate exact upper-tail halfword translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half tail window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 12U,
        (uint64_t)followup_exact_upper_tail_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate exact upper-tail halfword store, the final boundary bytes should preserve bytes6-11 from the retried image while refreshing only bytes12-13 and keeping bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate exact upper-tail halfword follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_exact_upper_tail_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_exact_upper_tail_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xfe, 0xed, 0xdc, 0xcb, 0xba, 0xa9, 0x98, 0x87,
        0x76, 0x65, 0x54, 0x43, 0x32, 0x21, 0x10, 0xff,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1e, 0x20, 0x32, 0x44, 0x56, 0x68, 0x7a, 0x8c,
    };
    static const uint8_t guard_after_seed[8] = {
        0x0d, 0x1f, 0x31, 0x43, 0x55, 0x67, 0x79, 0x8b,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_exact_upper_tail_word_store_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xc8d9eafb0c1d2e3fULL;
    const uint64_t refault_store_value = 0x596a7b8c9daebfd0ULL;
    const uint64_t aligned_upper_store_value_first = 0xb4c5d6e7f8091a2bULL;
    const uint64_t aligned_upper_store_value_second = 0x4e5f60718293a4b5ULL;
    const uint64_t aligned_upper_store_value_third = 0xc5d6e7f8091a2b3cULL;
    const uint64_t aligned_upper_store_value_fourth = 0x5f60718293a4b5c6ULL;
    const uint64_t retry_2b6b_store_value = 0xebfc0d1e2f304152ULL;
    const uint32_t followup_exact_upper_tail_word_store_value = 0x9e8d7c6bU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then exact upper-tail word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then exact upper-tail word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then exact upper-tail word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then exact upper-tail word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then exact upper-tail word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then exact upper-tail word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the exact upper-tail word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate exact upper-tail word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sw(
        followup_exact_upper_tail_word_store_vaddr,
        followup_exact_upper_tail_word_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate exact upper-tail word translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half tail window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 10U,
        (uint64_t)followup_exact_upper_tail_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate exact upper-tail word store, the final boundary bytes should preserve bytes6-9 from the retried image while refreshing only bytes10-13 and keeping bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate exact upper-tail word follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_exact_upper_tail_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_exact_upper_tail_first_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x1c, 0x2d, 0x3e, 0x4f, 0x50, 0x61, 0x72, 0x83,
        0x94, 0xa5, 0xb6, 0xc7, 0xd8, 0xe9, 0xfa, 0x0b,
    };
    static const uint8_t guard_before_seed[8] = {
        0x11, 0x23, 0x35, 0x47, 0x59, 0x6b, 0x7d, 0x8f,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9a, 0x8c, 0x7e, 0x60, 0x52, 0x44, 0x36, 0x28,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_exact_upper_tail_first_byte_store_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8697a8b9cadbecfdULL;
    const uint64_t refault_store_value = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_first = 0x7a8b9cadbecfe0f1ULL;
    const uint64_t aligned_upper_store_value_second = 0x1425364758697a8bULL;
    const uint64_t aligned_upper_store_value_third = 0x8d9eafc0d1e2f304ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x2738495a6b7c8d9eULL;
    const uint64_t retry_2b6b_store_value = 0xb8c9daebfc0d1e2fULL;
    const uint8_t followup_exact_upper_tail_first_byte_store_value = 0x6dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the exact upper-tail first-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate exact upper-tail first-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sb(
        followup_exact_upper_tail_first_byte_store_vaddr,
        followup_exact_upper_tail_first_byte_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate exact upper-tail first-byte translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half tail window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 12U,
        (uint64_t)followup_exact_upper_tail_first_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate exact upper-tail first-byte store, the final boundary bytes should preserve bytes6-11 from the retried image while refreshing only byte12, keeping byte13 from the retried image, and keeping bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate exact upper-tail first-byte follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_exact_upper_tail_first_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_first_upper_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x92, 0x81, 0x70, 0x6f, 0x5e, 0x4d, 0x3c, 0x2b,
        0x1a, 0x09, 0xf8, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3,
    };
    static const uint8_t guard_before_seed[8] = {
        0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc5, 0xd7, 0xe9, 0xfb, 0x0d, 0x1f, 0x21, 0x33,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x425364758697a8b9ULL;
    const uint64_t refault_store_value = 0xc2d3e4f506172839ULL;
    const uint64_t aligned_upper_store_value_first = 0x2b3c4d5e6f708192ULL;
    const uint64_t aligned_upper_store_value_second = 0xa3b4c5d6e7f8091aULL;
    const uint64_t aligned_upper_store_value_third = 0x3c4d5e6f708192a3ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xb4c5d6e7f8091a2bULL;
    const uint64_t retry_2b6b_store_value = 0x64758697a8b9cadbULL;
    const uint8_t followup_first_upper_byte_store_value = 0x5aU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then first-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then first-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the first-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate first-upper-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sb(
        aligned_upper_store_vaddr, followup_first_upper_byte_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated first-upper-byte store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half head window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        (uint64_t)followup_first_upper_byte_store_value, sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate first-upper-byte store, the final boundary bytes should preserve bytes6-7 from the retried image, refresh only byte8, preserve bytes9-13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate first-upper-byte follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_first_upper_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_first_upper_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x84, 0x73, 0x62, 0x51, 0x40, 0x3f, 0x2e, 0x1d,
        0x0c, 0xfb, 0xea, 0xd9, 0xc8, 0xb7, 0xa6, 0x95,
    };
    static const uint8_t guard_before_seed[8] = {
        0x17, 0x29, 0x3b, 0x4d, 0x5f, 0x61, 0x73, 0x85,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc6, 0xd8, 0xea, 0xfc, 0x0e, 0x10, 0x22, 0x34,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_first_upper_halfword_store_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x31425364758697a8ULL;
    const uint64_t refault_store_value = 0xb1c2d3e4f5061728ULL;
    const uint64_t aligned_upper_store_value_first = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t aligned_upper_store_value_second = 0x92a3b4c5d6e7f809ULL;
    const uint64_t aligned_upper_store_value_third = 0x2b3c4d5e6f708192ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xa3b4c5d6e7f8091aULL;
    const uint64_t retry_2b6b_store_value = 0x5364758697a8b9caULL;
    const uint16_t followup_first_upper_halfword_store_value = 0x6b5aU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then first-upper-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then first-upper-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the first-upper-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate first-upper-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sh(
        followup_first_upper_halfword_store_vaddr,
        followup_first_upper_halfword_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated first-upper-halfword store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half head window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        (uint64_t)followup_first_upper_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate first-upper-halfword store, the final boundary bytes should preserve bytes6-7 from the retried image, refresh only bytes8-9, preserve bytes10-13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate first-upper-halfword follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_first_upper_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_first_upper_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x76, 0x65, 0x54, 0x43, 0x32, 0x21, 0x10, 0xff,
        0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77,
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
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_first_upper_word_store_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2031425364758697ULL;
    const uint64_t refault_store_value = 0xa0b1c2d3e4f50617ULL;
    const uint64_t aligned_upper_store_value_first = 0x091a2b3c4d5e6f70ULL;
    const uint64_t aligned_upper_store_value_second = 0x8192a3b4c5d6e7f8ULL;
    const uint64_t aligned_upper_store_value_third = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x92a3b4c5d6e7f809ULL;
    const uint64_t retry_2b6b_store_value = 0x425364758697a8b9ULL;
    const uint32_t followup_first_upper_word_store_value = 0x7c6b5a49U;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then first-upper-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then first-upper-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then first-upper-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then first-upper-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then first-upper-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then first-upper-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the first-upper-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate first-upper-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sw(
        followup_first_upper_word_store_vaddr,
        followup_first_upper_word_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated first-upper-word store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half head window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        (uint64_t)followup_first_upper_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate first-upper-word store, the final boundary bytes should preserve bytes6-7 from the retried image, refresh only bytes8-11, preserve bytes12-13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate first-upper-word follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_first_upper_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_middle_semantic_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02, 0xf1,
        0xe0, 0xcf, 0xbe, 0xad, 0x9c, 0x8b, 0x7a, 0x69,
    };
    static const uint8_t guard_before_seed[8] = {
        0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc8, 0xda, 0xec, 0xfe, 0x10, 0x12, 0x24, 0x36,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_middle_semantic_word_store_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x1021324354657687ULL;
    const uint64_t refault_store_value = 0x90a1b2c3d4e5f607ULL;
    const uint64_t aligned_upper_store_value_first = 0xf8091a2b3c4d5e6fULL;
    const uint64_t aligned_upper_store_value_second = 0x708192a3b4c5d6e7ULL;
    const uint64_t aligned_upper_store_value_third = 0x091a2b3c4d5e6f70ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x8192a3b4c5d6e7f8ULL;
    const uint64_t retry_2b6b_store_value = 0x31425364758697a8ULL;
    const uint32_t followup_middle_semantic_word_store_value = 0x8f7e6d5cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then middle-semantic-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then middle-semantic-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then middle-semantic-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then middle-semantic-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then middle-semantic-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then middle-semantic-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the middle-semantic-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate middle-semantic-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sw(
        followup_middle_semantic_word_store_vaddr,
        followup_middle_semantic_word_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated middle-semantic-word store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 9U,
        (uint64_t)followup_middle_semantic_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate middle-semantic-word store, the final boundary bytes should preserve bytes6-8 from the retried image, refresh only bytes9-12, preserve byte13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate middle-semantic-word follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_middle_semantic_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_middle_semantic_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x57, 0x46, 0x35, 0x24, 0x13, 0x02, 0xf1, 0xe0,
        0xdf, 0xce, 0xbd, 0xac, 0x9b, 0x8a, 0x79, 0x68,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc9, 0xdb, 0xed, 0xff, 0x21, 0x23, 0x25, 0x37,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_middle_semantic_halfword_store_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2132435465768798ULL;
    const uint64_t refault_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t aligned_upper_store_value_first = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t aligned_upper_store_value_second = 0x8293a4b5c6d7e8f9ULL;
    const uint64_t aligned_upper_store_value_third = 0x1b2c3d4e5f607182ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x93a4b5c6d7e8f90aULL;
    const uint64_t retry_2b6b_store_value = 0x5263748596a7b8c9ULL;
    const uint16_t followup_middle_semantic_halfword_store_value = 0x6d5cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then middle-semantic-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then middle-semantic-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then middle-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then middle-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then middle-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then middle-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the middle-semantic-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate middle-semantic-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sh(
        followup_middle_semantic_halfword_store_vaddr,
        followup_middle_semantic_halfword_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated middle-semantic-halfword store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 9U,
        (uint64_t)followup_middle_semantic_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate middle-semantic-halfword store, the final boundary bytes should preserve bytes6-8 from the retried image, refresh only bytes9-10, preserve bytes11-13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate middle-semantic-halfword follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_middle_semantic_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_middle_tail_bridge_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x49, 0x38, 0x27, 0x16, 0x05, 0xf4, 0xe3, 0xd2,
        0xc1, 0xb0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b, 0x5a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1b, 0x2d, 0x3f, 0x41, 0x53, 0x65, 0x77, 0x89,
    };
    static const uint8_t guard_after_seed[8] = {
        0xca, 0xdc, 0xee, 0xf0, 0x22, 0x34, 0x46, 0x58,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_middle_tail_bridge_halfword_store_vaddr =
        aligned_upper_store_vaddr + 3U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x32435465768798a9ULL;
    const uint64_t refault_store_value = 0xb2c3d4e5f6071829ULL;
    const uint64_t aligned_upper_store_value_first = 0x1b2c3d4e5f607182ULL;
    const uint64_t aligned_upper_store_value_second = 0x93a4b5c6d7e8f90aULL;
    const uint64_t aligned_upper_store_value_third = 0x2c3d4e5f60718293ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xa4b5c6d7e8f90a1bULL;
    const uint64_t retry_2b6b_store_value = 0x63748596a7b8c9daULL;
    const uint16_t followup_middle_tail_bridge_halfword_store_value = 0x5e4dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then middle-tail-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then middle-tail-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then middle-tail-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then middle-tail-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then middle-tail-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then middle-tail-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the middle-tail-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate middle-tail-bridge-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sh(
        followup_middle_tail_bridge_halfword_store_vaddr,
        followup_middle_tail_bridge_halfword_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated middle-tail-bridge-halfword store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 11U,
        (uint64_t)followup_middle_tail_bridge_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate middle-tail-bridge-halfword store, the final boundary bytes should preserve bytes6-10 from the retried image, refresh only bytes11-12, preserve byte13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate middle-tail-bridge-halfword follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_middle_tail_bridge_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_center_semantic_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x5a, 0x49, 0x38, 0x27, 0x16, 0x05, 0xf4, 0xe3,
        0xd2, 0xc1, 0xb0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1c, 0x2e, 0x30, 0x42, 0x54, 0x66, 0x78, 0x8a,
    };
    static const uint8_t guard_after_seed[8] = {
        0xcb, 0xdd, 0xef, 0xf1, 0x23, 0x35, 0x47, 0x59,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_center_semantic_halfword_store_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x435465768798a9baULL;
    const uint64_t refault_store_value = 0xc3d4e5f60718293aULL;
    const uint64_t aligned_upper_store_value_first = 0x2c3d4e5f60718293ULL;
    const uint64_t aligned_upper_store_value_second = 0xa4b5c6d7e8f90a1bULL;
    const uint64_t aligned_upper_store_value_third = 0x3d4e5f60718293a4ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xb5c6d7e8f90a1b2cULL;
    const uint64_t retry_2b6b_store_value = 0x748596a7b8c9daebULL;
    const uint16_t followup_center_semantic_halfword_store_value = 0x4f3eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then center-semantic-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then center-semantic-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then center-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then center-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then center-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then center-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the center-semantic-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate center-semantic-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sh(
        followup_center_semantic_halfword_store_vaddr,
        followup_center_semantic_halfword_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated center-semantic-halfword store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 10U,
        (uint64_t)followup_center_semantic_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate center-semantic-halfword store, the final boundary bytes should preserve bytes6-9 from the retried image, refresh only bytes10-11, preserve bytes12-13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate center-semantic-halfword follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_center_semantic_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_center_semantic_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x6b, 0x5a, 0x49, 0x38, 0x27, 0x16, 0x05, 0xf4,
        0xe3, 0xd2, 0xc1, 0xb0, 0xaf, 0x9e, 0x8d, 0x7c,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1d, 0x2f, 0x31, 0x43, 0x55, 0x67, 0x79, 0x8b,
    };
    static const uint8_t guard_after_seed[8] = {
        0xcc, 0xde, 0xe0, 0xf2, 0x24, 0x36, 0x48, 0x5a,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_center_semantic_byte_store_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5465768798a9bacbULL;
    const uint64_t refault_store_value = 0xd4e5f60718293a4bULL;
    const uint64_t aligned_upper_store_value_first = 0x3d4e5f60718293a4ULL;
    const uint64_t aligned_upper_store_value_second = 0xb5c6d7e8f90a1b2cULL;
    const uint64_t aligned_upper_store_value_third = 0x4e5f60718293a4b5ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xc6d7e8f90a1b2c3dULL;
    const uint64_t retry_2b6b_store_value = 0x8596a7b8c9daebfcULL;
    const uint8_t followup_center_semantic_byte_store_value = 0x2dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then center-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then center-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the center-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate center-semantic-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sb(
        followup_center_semantic_byte_store_vaddr,
        followup_center_semantic_byte_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated center-semantic-byte store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 10U,
        (uint64_t)followup_center_semantic_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate center-semantic-byte store, the final boundary bytes should preserve bytes6-9 from the retried image, refresh only byte10, preserve bytes11-13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate center-semantic-byte follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_center_semantic_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_near_tail_semantic_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x7c, 0x6b, 0x5a, 0x49, 0x38, 0x27, 0x16, 0x05,
        0xf4, 0xe3, 0xd2, 0xc1, 0xb0, 0xaf, 0x9e, 0x8d,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1e, 0x20, 0x32, 0x44, 0x56, 0x68, 0x7a, 0x8c,
    };
    static const uint8_t guard_after_seed[8] = {
        0xcd, 0xdf, 0xe1, 0xf3, 0x25, 0x37, 0x49, 0x5b,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_near_tail_semantic_byte_store_vaddr =
        aligned_upper_store_vaddr + 3U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xe5f60718293a4b5cULL;
    const uint64_t aligned_upper_store_value_first = 0x4e5f60718293a4b5ULL;
    const uint64_t aligned_upper_store_value_second = 0xc6d7e8f90a1b2c3dULL;
    const uint64_t aligned_upper_store_value_third = 0x5f60718293a4b5c6ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xd7e8f90a1b2c3d4eULL;
    const uint64_t retry_2b6b_store_value = 0x96a7b8c9daebfc0dULL;
    const uint8_t followup_near_tail_semantic_byte_store_value = 0x1cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the near-tail-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate near-tail-semantic-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sb(
        followup_near_tail_semantic_byte_store_vaddr,
        followup_near_tail_semantic_byte_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated near-tail-semantic-byte store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 11U,
        (uint64_t)followup_near_tail_semantic_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate near-tail-semantic-byte store, the final boundary bytes should preserve bytes6-10 from the retried image, refresh only byte11, preserve bytes12-13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate near-tail-semantic-byte follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_near_tail_semantic_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_second_upper_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x8d, 0x7c, 0x6b, 0x5a, 0x49, 0x38, 0x27, 0x16,
        0x05, 0xf4, 0xe3, 0xd2, 0xc1, 0xb0, 0xaf, 0x9e,
    };
    static const uint8_t guard_before_seed[8] = {
        0x21, 0x33, 0x45, 0x57, 0x69, 0x7b, 0x8d, 0x9f,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd0, 0xe2, 0xf4, 0x06, 0x18, 0x2a, 0x3c, 0x4e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_second_upper_byte_store_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x768798a9bacbdcedULL;
    const uint64_t refault_store_value = 0xf60718293a4b5c6dULL;
    const uint64_t aligned_upper_store_value_first = 0x60718293a4b5c6d7ULL;
    const uint64_t aligned_upper_store_value_second = 0xe8f90a1b2c3d4e5fULL;
    const uint64_t aligned_upper_store_value_third = 0x718293a4b5c6d7e8ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xf90a1b2c3d4e5f60ULL;
    const uint64_t retry_2b6b_store_value = 0xa7b8c9daebfc0d1eULL;
    const uint8_t followup_second_upper_byte_store_value = 0x3eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte6_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte6_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte6_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the true second-half start address before the retried-success then second-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 2B+6B write fault keeps the same precise metadata before the retried-success then second-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-2B+6B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-2B+6B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-2B+6B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE6_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 2B+6B misaligned store should refresh the same precise second-split store access fault metadata before the second-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte6_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sd(
        fault_vaddr, retry_2b6b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 2B+6B translated store as a success store should clear stale exception metadata before the immediate second-upper-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte6_tail_edge_store_translated_sb(
        followup_second_upper_byte_store_vaddr,
        followup_second_upper_byte_store_value);
    TEST_ASSERT("after the retried original-template 2B+6B store has already succeeded on the same boundary, the immediate translated second-upper-byte store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half head-side internal window beyond the semantic BYTE6 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 6U,
        retry_2b6b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte6_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 9U,
        (uint64_t)followup_second_upper_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 2B+6B success store, and immediate second-upper-byte store, the final boundary bytes should preserve bytes6-8 from the retried image, refresh only byte9, preserve bytes10-13 from the retried image, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 2B+6B fault, four upper-half aligned stores, refault, retried success store, and immediate second-upper-byte follow-up store sequence",
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte6_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte6_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_second_upper_byte_store_corner");
}
