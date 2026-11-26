#include <page_tables.h>
#include <rvh_test.h>
#include <stdint.h>

/* Keep the BYTE5 tail-edge follow-up store separate from the BYTE6/BYTE7
 * point7 cases so the 3B+5B stale high-half mask suspicion can evolve
 * independently.
 */

#define AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET 13U

static inline pte_t *ai_micro_mab_retry_byte5_tail_edge_store_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_byte5_tail_edge_store_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_byte5_tail_edge_store_hs_leaf_entry(page) =
        (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf_paddr(
        page, phys_page_base(page), flags);
}

static inline void
ai_micro_mab_retry_byte5_tail_edge_store_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void
ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_RWX);
}

static inline void
ai_micro_mab_retry_byte5_tail_edge_store_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_RWX |
        (deny_cfg << 8) |
        (AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_byte5_tail_edge_store_reset_env(void)
{
    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();
    ai_micro_mab_retry_byte5_tail_edge_store_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
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
ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
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

#define AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION( \
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

AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd, "sd", uint64_t)
AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sb, "sb", uint8_t)
AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sh, "sh", uint16_t)
AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sw, "sw", uint32_t)

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_first_out_of_range_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02,
        0xf1, 0xe0, 0xcf, 0xbe, 0xad, 0x9c, 0x8b, 0x7a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd3, 0xe5, 0xf7, 0x09, 0x1b, 0x2d, 0x3f, 0x41,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_first_out_of_range_byte_store_vaddr =
        aligned_upper_store_vaddr + 5U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x6a7b8c9daebfd0e1ULL;
    const uint64_t refault_store_value = 0xf203142536475869ULL;
    const uint64_t aligned_upper_store_value_first = 0x32435465768798a9ULL;
    const uint64_t aligned_upper_store_value_second = 0xbaebfc0d1e2f3041ULL;
    const uint64_t aligned_upper_store_value_third = 0x435465768798a9baULL;
    const uint64_t aligned_upper_store_value_fourth = 0xebfc0d1e2f304152ULL;
    const uint64_t retry_3b5b_store_value = 0x7c8d9eafc0d1e2f3ULL;
    const uint8_t followup_upper_tail_first_out_of_range_byte_store_value =
        0x5dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the first-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate first-out-of-range tail-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sb(
        followup_upper_tail_first_out_of_range_byte_store_vaddr,
        followup_upper_tail_first_out_of_range_byte_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate first-out-of-range tail-byte translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge beyond the semantic BYTE5 window",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 13U,
        (uint64_t)followup_upper_tail_first_out_of_range_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate first-out-of-range tail-byte store, the final boundary bytes should preserve bytes5-12 from the retried image while refreshing only byte13 and keeping bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate first-out-of-range tail-byte follow-up store sequence",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_first_out_of_range_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_second_out_of_range_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x8a, 0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13,
        0x02, 0xf1, 0xe0, 0xcf, 0xbe, 0xad, 0x9c, 0x8b,
    };
    static const uint8_t guard_before_seed[8] = {
        0x17, 0x29, 0x3b, 0x4d, 0x5f, 0x61, 0x73, 0x85,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe4, 0xf6, 0x08, 0x1a, 0x2c, 0x3e, 0x40, 0x52,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_second_out_of_range_byte_store_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x7b8c9daebfd0e1f2ULL;
    const uint64_t refault_store_value = 0x031425364758697aULL;
    const uint64_t aligned_upper_store_value_first = 0x435465768798a9baULL;
    const uint64_t aligned_upper_store_value_second = 0xcbecfd0e1f203142ULL;
    const uint64_t aligned_upper_store_value_third = 0x5465768798a9bacbULL;
    const uint64_t aligned_upper_store_value_fourth = 0xecfd0e1f20314253ULL;
    const uint64_t retry_3b5b_store_value = 0x8d9eafc0d1e2f304ULL;
    const uint8_t followup_upper_tail_second_out_of_range_byte_store_value =
        0x6eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the second-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate second-out-of-range tail-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sb(
        followup_upper_tail_second_out_of_range_byte_store_vaddr,
        followup_upper_tail_second_out_of_range_byte_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate second-out-of-range tail-byte translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge beyond the semantic BYTE5 window",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 14U,
        (uint64_t)followup_upper_tail_second_out_of_range_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate second-out-of-range tail-byte store, the final boundary bytes should preserve bytes5-12 from the retried image while refreshing only byte14 and keeping byte13 and byte15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate second-out-of-range tail-byte follow-up store sequence",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_second_out_of_range_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_third_out_of_range_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x9b, 0x8a, 0x79, 0x68, 0x57, 0x46, 0x35, 0x24,
        0x13, 0x02, 0xf1, 0xe0, 0xcf, 0xbe, 0xad, 0x9c,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74, 0x86,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf5, 0x07, 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_third_out_of_range_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x9eafc0d1e2f30415ULL;
    const uint64_t refault_store_value = 0x1425364758697a8bULL;
    const uint64_t aligned_upper_store_value_first = 0x5465768798a9bacbULL;
    const uint64_t aligned_upper_store_value_second = 0xdcfd0e1f20314253ULL;
    const uint64_t aligned_upper_store_value_third = 0x65768798a9bacbdcULL;
    const uint64_t aligned_upper_store_value_fourth = 0xfd0e1f2031425364ULL;
    const uint64_t retry_3b5b_store_value = 0xaebfd0e1f2031425ULL;
    const uint8_t followup_upper_tail_third_out_of_range_byte_store_value =
        0x7fU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then third-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then third-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then third-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then third-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then third-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then third-out-of-range tail-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the third-out-of-range tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate third-out-of-range tail-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sb(
        followup_upper_tail_third_out_of_range_byte_store_vaddr,
        followup_upper_tail_third_out_of_range_byte_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate third-out-of-range tail-byte translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge beyond the semantic BYTE5 window",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_upper_tail_third_out_of_range_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate third-out-of-range tail-byte store, the final boundary bytes should preserve bytes5-12 from the retried image while refreshing only byte15 and keeping bytes13-14 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate third-out-of-range tail-byte follow-up store sequence",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_third_out_of_range_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_out_of_range_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xac, 0x9b, 0x8a, 0x79, 0x68, 0x57, 0x46, 0x35,
        0x24, 0x13, 0x02, 0xf1, 0xe0, 0xcf, 0xbe, 0xad,
    };
    static const uint8_t guard_before_seed[8] = {
        0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
    };
    static const uint8_t guard_after_seed[8] = {
        0x06, 0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_out_of_range_halfword_store_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xbfd0e1f203142536ULL;
    const uint64_t refault_store_value = 0x25364758697a8b9cULL;
    const uint64_t aligned_upper_store_value_first = 0x65768798a9bacbdcULL;
    const uint64_t aligned_upper_store_value_second = 0xedfe0f1021324354ULL;
    const uint64_t aligned_upper_store_value_third = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_fourth = 0xfe0f102132435465ULL;
    const uint64_t retry_3b5b_store_value = 0xc0d1e2f304152637ULL;
    const uint16_t followup_upper_tail_out_of_range_halfword_store_value =
        0xa190U;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then out-of-range tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the out-of-range tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate out-of-range tail-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sh(
        followup_upper_tail_out_of_range_halfword_store_vaddr,
        followup_upper_tail_out_of_range_halfword_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate out-of-range tail-halfword translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge beyond the semantic BYTE5 window",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 14U,
        (uint64_t)followup_upper_tail_out_of_range_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate out-of-range tail-halfword store, the final boundary bytes should preserve bytes5-12 from the retried image while refreshing only bytes14-15 and keeping byte13 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate out-of-range tail-halfword follow-up store sequence",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_out_of_range_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_overlapping_out_of_range_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xbd, 0xac, 0x9b, 0x8a, 0x79, 0x68, 0x57, 0x46,
        0x35, 0x24, 0x13, 0x02, 0xf1, 0xe0, 0xcf, 0xbe,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1b, 0x2d, 0x3f, 0x41, 0x53, 0x65, 0x77, 0x89,
    };
    static const uint8_t guard_after_seed[8] = {
        0x17, 0x29, 0x3b, 0x4d, 0x5f, 0x61, 0x73, 0x85,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_overlapping_out_of_range_word_store_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xd1e2f30415263748ULL;
    const uint64_t refault_store_value = 0x4758697a8b9cadbeULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0xfe0f102132435465ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x0f10213243546576ULL;
    const uint64_t retry_3b5b_store_value = 0xe2f3041526374859ULL;
    const uint32_t followup_upper_tail_overlapping_out_of_range_word_store_value =
        0xb4a39281U;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then overlapping-out-of-range tail-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then overlapping-out-of-range tail-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then overlapping-out-of-range tail-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then overlapping-out-of-range tail-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then overlapping-out-of-range tail-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then overlapping-out-of-range tail-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the overlapping-out-of-range tail-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate overlapping-out-of-range tail-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sw(
        followup_upper_tail_overlapping_out_of_range_word_store_vaddr,
        followup_upper_tail_overlapping_out_of_range_word_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate overlapping-out-of-range tail-word translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge beyond the semantic BYTE5 window",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 12U,
        (uint64_t)followup_upper_tail_overlapping_out_of_range_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate overlapping-out-of-range tail-word store, the final boundary bytes should preserve bytes5-11 from the retried image while refreshing bytes12-15",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate overlapping-out-of-range tail-word follow-up store sequence",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_overlapping_out_of_range_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_exact_upper_tail_first_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xce, 0xbd, 0xac, 0x9b, 0x8a, 0x79, 0x68, 0x57,
        0x46, 0x35, 0x24, 0x13, 0x02, 0xf1, 0xe0, 0xcf,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1c, 0x2e, 0x30, 0x42, 0x54, 0x66, 0x78, 0x8a,
    };
    static const uint8_t guard_after_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74, 0x86,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_exact_upper_tail_first_byte_store_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xf30415263748596aULL;
    const uint64_t refault_store_value = 0x697a8b9cadbecfe0ULL;
    const uint64_t aligned_upper_store_value_first = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_second = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x31425364758697a8ULL;
    const uint64_t retry_3b5b_store_value = 0x031425364758697aULL;
    const uint8_t followup_exact_upper_tail_first_byte_store_value = 0x7cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then exact upper-tail first-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the exact upper-tail first-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate exact upper-tail first-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sb(
        followup_exact_upper_tail_first_byte_store_vaddr,
        followup_exact_upper_tail_first_byte_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate exact upper-tail first-byte translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half tail window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 12U,
        (uint64_t)followup_exact_upper_tail_first_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate exact upper-tail first-byte store, the final boundary bytes should preserve bytes5-11 from the retried image while refreshing only byte12, keeping bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate exact upper-tail first-byte follow-up store sequence",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_exact_upper_tail_first_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_exact_upper_tail_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xdf, 0xce, 0xbd, 0xac, 0x9b, 0x8a, 0x79, 0x68,
        0x57, 0x46, 0x35, 0x24, 0x13, 0x02, 0xf1, 0xe0,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1d, 0x2f, 0x41, 0x53, 0x65, 0x77, 0x89, 0x9b,
    };
    static const uint8_t guard_after_seed[8] = {
        0x19, 0x2b, 0x3d, 0x4f, 0x61, 0x73, 0x85, 0x97,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_exact_upper_tail_halfword_store_vaddr =
        aligned_upper_store_vaddr + 3U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x1425364758697a8bULL;
    const uint64_t refault_store_value = 0x8c9daebfd0e1f203ULL;
    const uint64_t aligned_upper_store_value_first = 0xbacbdcedfe0f1021ULL;
    const uint64_t aligned_upper_store_value_second = 0x32435465768798a9ULL;
    const uint64_t aligned_upper_store_value_third = 0xcbdcedfe0f102132ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x435465768798a9baULL;
    const uint64_t retry_3b5b_store_value = 0x25364758697a8b9cULL;
    const uint16_t followup_exact_upper_tail_halfword_store_value = 0x8d7cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then exact upper-tail halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then exact upper-tail halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then exact upper-tail halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then exact upper-tail halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then exact upper-tail halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then exact upper-tail halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the exact upper-tail halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate exact upper-tail halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sh(
        followup_exact_upper_tail_halfword_store_vaddr,
        followup_exact_upper_tail_halfword_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate exact upper-tail halfword translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half tail window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 11U,
        (uint64_t)followup_exact_upper_tail_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate exact upper-tail halfword store, the final boundary bytes should preserve bytes5-10 from the retried image while refreshing only bytes11-12 and keeping bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate exact upper-tail halfword follow-up store sequence",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_exact_upper_tail_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_exact_upper_tail_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xe0, 0xdf, 0xce, 0xbd, 0xac, 0x9b, 0x8a, 0x79,
        0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02, 0xf1,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1e, 0x30, 0x42, 0x54, 0x66, 0x78, 0x8a, 0x9c,
    };
    static const uint8_t guard_after_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x50, 0x62, 0x74, 0x86, 0x98,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_exact_upper_tail_word_store_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x364758697a8b9cadULL;
    const uint64_t refault_store_value = 0xaebfd0e1f2031425ULL;
    const uint64_t aligned_upper_store_value_first = 0xdcedfe0f10213243ULL;
    const uint64_t aligned_upper_store_value_second = 0x5465768798a9bacbULL;
    const uint64_t aligned_upper_store_value_third = 0xedfe0f1021324354ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x65768798a9bacbdcULL;
    const uint64_t retry_3b5b_store_value = 0x4758697a8b9cadbeULL;
    const uint32_t followup_exact_upper_tail_word_store_value = 0xa7968574U;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_tail_edge_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_tail_edge_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_tail_edge_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then exact upper-tail word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then exact upper-tail word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then exact upper-tail word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then exact upper-tail word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then exact upper-tail word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then exact upper-tail word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_TAIL_EDGE_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the exact upper-tail word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_tail_edge_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate exact upper-tail word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_tail_edge_store_translated_sw(
        followup_exact_upper_tail_word_store_vaddr,
        followup_exact_upper_tail_word_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate exact upper-tail word translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half tail window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_tail_edge_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 9U,
        (uint64_t)followup_exact_upper_tail_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate exact upper-tail word store, the final boundary bytes should preserve bytes5-8 from the retried image while refreshing only bytes9-12 and keeping bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate exact upper-tail word follow-up store sequence",
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_tail_edge_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_tail_edge_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_exact_upper_tail_word_store_corner");
}
