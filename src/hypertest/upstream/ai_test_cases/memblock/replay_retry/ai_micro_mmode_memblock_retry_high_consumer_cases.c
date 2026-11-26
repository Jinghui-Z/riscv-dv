#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep retry-same-template high-half consumer probes separate so the older
 * refault follow-up file stays readable.
 */

#define AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_HIGH_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_retry_high_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_high_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_high_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_high_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_high_set_leaf_paddr(page, phys_page_base(page), flags);
}

static inline void ai_micro_mab_retry_high_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_retry_high_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_RWX);
}

static inline void ai_micro_mab_retry_high_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_retry_high_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_high_reset_env(void)
{
    ai_micro_mab_retry_high_restore_pmp_full_access();
    ai_micro_mab_retry_high_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_high_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_high_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_retry_high_overlay_little_endian_bytes(
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

static inline uint64_t ai_micro_mab_retry_high_u64_from_bytes(
    const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }
    return value;
}

#define AI_MICRO_MAB_RETRY_HIGH_TRANSLATED_STORE_INSTRUCTION(name, inst, type) \
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

AI_MICRO_MAB_RETRY_HIGH_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_high_translated_sd, "sd", uint64_t)

#define AI_MICRO_MAB_RETRY_HIGH_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
static inline uint64_t name(uintptr_t addr) \
{ \
    uint64_t value = 0ULL; \
    const uint64_t clear_mpp = 3ULL << 11; \
    const uint64_t set_mprv = MSTATUS_MPRV; \
    const uint64_t set_mpp_s = MSTATUS_MPP; \
    asm volatile( \
        "csrc mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        "csrs mstatus, %3\n\t" \
        inst " %0, 0(%4)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %2\n\t" \
        "csrc mstatus, %1\n\t" \
        : "=r"(value) \
        : "r"(clear_mpp), "r"(set_mprv), "r"(set_mpp_s), "r"(addr) \
        : "memory"); \
    return value; \
}

AI_MICRO_MAB_RETRY_HIGH_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_translated_lbu, "lbu")
AI_MICRO_MAB_RETRY_HIGH_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_translated_lhu, "lhu")
AI_MICRO_MAB_RETRY_HIGH_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_translated_ld, "ld")

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa4, 0x92, 0x80, 0x6e, 0x5c, 0x4a, 0x38, 0x26,
        0x14, 0x02, 0xf0, 0xde, 0xcc, 0xba, 0xa8, 0x96,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1d, 0x2f, 0x41, 0x53, 0x55, 0x67, 0x79, 0x8b,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9c, 0xae, 0xc0, 0xd2, 0xe4, 0xf6, 0x08, 0x1a,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_store_vaddr =
        first_upper_halfword_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x62738495a6b7c8d9ULL;
    const uint64_t refault_store_value = 0xf30415263748596aULL;
    const uint64_t aligned_upper_store_value_first = 0x435567798b9dafc1ULL;
    const uint64_t aligned_upper_store_value_second = 0xd2e4f6081a2c3e40ULL;
    const uint64_t aligned_upper_store_value_third = 0x536577899badbfd1ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xe3f507192b3d4f51ULL;
    const uint64_t retry_same_template_store_value = 0x8293a4b5c6d7e8f9ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_halfword = 0ULL;
    uint64_t observed_first_upper_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_reset_env();
    ai_micro_mab_retry_high_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_prepare_supervisor_translation();
    ai_micro_mab_retry_high_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, and trailing first-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, and trailing first-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-first-upper-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-first-upper-halfword-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store and trailing first-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-halfword load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_halfword =
        (uint64_t)(uint16_t)ai_micro_mab_retry_high_u64_from_bytes(
            expected_boundary + 8U);

    TEST_SETUP_EXCEPT();
    observed_first_upper_halfword =
        ai_micro_mab_retry_high_translated_lhu(first_upper_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-halfword translated load at boundary+8 should clear stale exception metadata and read the retried high-half bytes0-1",
        excpt.triggered == false &&
        observed_first_upper_halfword == expected_first_upper_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, and trailing first-upper-halfword load sequence",
        ai_micro_mab_retry_high_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xb5, 0xa3, 0x91, 0x7f, 0x6d, 0x5b, 0x49, 0x37,
        0x25, 0x13, 0x01, 0xef, 0xdd, 0xcb, 0xb9, 0xa7,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1e, 0x30, 0x42, 0x54, 0x56, 0x68, 0x7a, 0x8c,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9d, 0xaf, 0xc1, 0xd3, 0xe5, 0xf7, 0x09, 0x1b,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_store_vaddr = aligned_upper_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x738495a6b7c8d9eaULL;
    const uint64_t refault_store_value = 0x0415263748596a7bULL;
    const uint64_t aligned_upper_store_value_first = 0x5466788a9caec0d2ULL;
    const uint64_t aligned_upper_store_value_second = 0xe5f7091b2d3f4153ULL;
    const uint64_t aligned_upper_store_value_third = 0x6476889aacbed0e2ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xf6081a2c3e405264ULL;
    const uint64_t retry_same_template_store_value = 0x93a4b5c6d7e8f90aULL;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_value = 0ULL;
    uint64_t observed_upper_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_reset_env();
    ai_micro_mab_retry_high_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_prepare_supervisor_translation();
    ai_micro_mab_retry_high_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, and trailing upper-load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, and trailing upper-load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store and trailing upper-load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_upper_value =
        ai_micro_mab_retry_high_u64_from_bytes(expected_boundary + 8U);

    TEST_SETUP_EXCEPT();
    observed_upper_value =
        ai_micro_mab_retry_high_translated_ld(aligned_upper_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-half aligned translated load at boundary+8 should clear stale exception metadata and read the retried high-half image",
        excpt.triggered == false &&
        observed_upper_value == expected_upper_value
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, and trailing upper-load sequence",
        ai_micro_mab_retry_high_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_byte_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xc6, 0xb4, 0xa2, 0x90, 0x7e, 0x6c, 0x5a, 0x48,
        0x36, 0x24, 0x12, 0x00, 0xee, 0xdc, 0xca, 0xb8,
    };
    static const uint8_t guard_before_seed[8] = {
        0x21, 0x33, 0x45, 0x57, 0x59, 0x6b, 0x7d, 0x8f,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa0, 0xb2, 0xc4, 0xd6, 0xe8, 0xfa, 0x0c, 0x1e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8495a6b7c8d9eafbULL;
    const uint64_t refault_store_value = 0x15263748596a7b8cULL;
    const uint64_t aligned_upper_store_value_first = 0x6577899badbfd1e3ULL;
    const uint64_t aligned_upper_store_value_second = 0xf6081a2c3e405264ULL;
    const uint64_t aligned_upper_store_value_third = 0x758799abbdcfe1f3ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x07192b3d4f516375ULL;
    const uint64_t retry_same_template_store_value = 0xa4b5c6d7e8f90a1bULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_byte = 0ULL;
    uint64_t observed_first_upper_byte = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_reset_env();
    ai_micro_mab_retry_high_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_prepare_supervisor_translation();
    ai_micro_mab_retry_high_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, and trailing first-upper-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, and trailing first-upper-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-first-upper-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-first-upper-byte-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store and trailing first-upper-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-byte load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_byte = (uint64_t)expected_boundary[8];

    TEST_SETUP_EXCEPT();
    observed_first_upper_byte =
        ai_micro_mab_retry_high_translated_lbu(first_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-byte translated load at boundary+8 should clear stale exception metadata and read the retried high-half byte0",
        excpt.triggered == false &&
        observed_first_upper_byte == expected_first_upper_byte
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, and trailing first-upper-byte load sequence",
        ai_micro_mab_retry_high_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_byte_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xd7, 0xc5, 0xb3, 0xa1, 0x8f, 0x7d, 0x6b, 0x59,
        0x47, 0x35, 0x23, 0x11, 0xff, 0xed, 0xdb, 0xc9,
    };
    static const uint8_t guard_before_seed[8] = {
        0x22, 0x34, 0x46, 0x58, 0x5a, 0x6c, 0x7e, 0x90,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa1, 0xb3, 0xc5, 0xd7, 0xe9, 0xfb, 0x0d, 0x1f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t mid_upper_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 10U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x95a6b7c8d9eafb0cULL;
    const uint64_t refault_store_value = 0x263748596a7b8c9dULL;
    const uint64_t aligned_upper_store_value_first = 0x76889aacbed0e2f4ULL;
    const uint64_t aligned_upper_store_value_second = 0x17293b4d5f617385ULL;
    const uint64_t aligned_upper_store_value_third = 0x8698aabcced0f204ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x27394b5d6f718395ULL;
    const uint64_t retry_same_template_store_value = 0xb5c6d7e8f90a1b2cULL;
    uint8_t expected_boundary[16];
    uint64_t expected_mid_upper_halfword = 0ULL;
    uint64_t observed_mid_upper_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_reset_env();
    ai_micro_mab_retry_high_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_prepare_supervisor_translation();
    ai_micro_mab_retry_high_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, and trailing mid-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, and trailing mid-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-mid-upper-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-mid-upper-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-mid-upper-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-mid-upper-halfword-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store and trailing mid-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing mid-upper-halfword load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_mid_upper_halfword =
        (uint64_t)(uint16_t)((uint16_t)expected_boundary[10] |
            ((uint16_t)expected_boundary[11] << 8U));

    TEST_SETUP_EXCEPT();
    observed_mid_upper_halfword =
        ai_micro_mab_retry_high_translated_lhu(mid_upper_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a mid-upper-halfword translated load at boundary+10 should clear stale exception metadata and read the retried high-half bytes2-3",
        excpt.triggered == false &&
        observed_mid_upper_halfword == expected_mid_upper_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, and trailing mid-upper-halfword load sequence",
        ai_micro_mab_retry_high_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xe8, 0xd6, 0xc4, 0xb2, 0xa0, 0x8e, 0x7c, 0x6a,
        0x58, 0x46, 0x34, 0x22, 0x10, 0xfe, 0xec, 0xda,
    };
    static const uint8_t guard_before_seed[8] = {
        0x23, 0x35, 0x47, 0x59, 0x5b, 0x6d, 0x7f, 0x91,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa2, 0xb4, 0xc6, 0xd8, 0xea, 0xfc, 0x0e, 0x20,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t second_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 9U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xa6b7c8d9eafb0c1dULL;
    const uint64_t refault_store_value = 0x3748596a7b8c9daeULL;
    const uint64_t aligned_upper_store_value_first = 0x8799abbddff10315ULL;
    const uint64_t aligned_upper_store_value_second = 0x283a4c5e708294a6ULL;
    const uint64_t aligned_upper_store_value_third = 0x97a9bbcdef011325ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x384a5c6e8092a4b6ULL;
    const uint64_t retry_same_template_store_value = 0xc6d7e8f90a1b2c3dULL;
    uint8_t expected_boundary[16];
    uint64_t expected_second_upper_byte = 0ULL;
    uint64_t observed_second_upper_byte = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_reset_env();
    ai_micro_mab_retry_high_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_prepare_supervisor_translation();
    ai_micro_mab_retry_high_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, and trailing second-upper-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, and trailing second-upper-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-second-upper-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-second-upper-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-second-upper-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-second-upper-byte-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store and trailing second-upper-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing second-upper-byte load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_second_upper_byte = (uint64_t)expected_boundary[9];

    TEST_SETUP_EXCEPT();
    observed_second_upper_byte =
        ai_micro_mab_retry_high_translated_lbu(second_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a second-byte translated load at boundary+9 should clear stale exception metadata and read the retried high-half byte1",
        excpt.triggered == false &&
        observed_second_upper_byte == expected_second_upper_byte
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, and trailing second-upper-byte load sequence",
        ai_micro_mab_retry_high_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xf9, 0xe7, 0xd5, 0xc3, 0xb1, 0x9f, 0x8d, 0x7b,
        0x69, 0x57, 0x45, 0x33, 0x21, 0x0f, 0xfd, 0xeb,
    };
    static const uint8_t guard_before_seed[8] = {
        0x24, 0x36, 0x48, 0x5a, 0x5c, 0x6e, 0x80, 0x92,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa3, 0xb5, 0xc7, 0xd9, 0xeb, 0xfd, 0x0f, 0x21,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t second_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 9U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xb7c8d9eafb0c1d2eULL;
    const uint64_t refault_store_value = 0x48596a7b8c9daebfULL;
    const uint64_t aligned_upper_store_value_first = 0x98aabcced0f20416ULL;
    const uint64_t aligned_upper_store_value_second = 0x394b5d6f8193a5b7ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bbcddff1031527ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x4a5c6e8092a4b6c8ULL;
    const uint64_t retry_same_template_store_value = 0xd7e8f90a1b2c3d4eULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_byte = 0ULL;
    uint64_t expected_second_upper_byte = 0ULL;
    uint64_t observed_first_upper_byte = 0ULL;
    uint64_t observed_second_upper_byte = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_reset_env();
    ai_micro_mab_retry_high_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_prepare_supervisor_translation();
    ai_micro_mab_retry_high_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, and trailing dual-head-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, and trailing dual-head-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-dual-head-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-dual-head-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-dual-head-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-dual-head-byte-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store and trailing dual-head-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing dual-head-byte load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_byte = (uint64_t)expected_boundary[8];
    expected_second_upper_byte = (uint64_t)expected_boundary[9];

    TEST_SETUP_EXCEPT();
    observed_first_upper_byte =
        ai_micro_mab_retry_high_translated_lbu(first_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-byte translated load at boundary+8 should clear stale exception metadata and read the retried high-half byte0 before the following second-byte probe",
        excpt.triggered == false &&
        observed_first_upper_byte == expected_first_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_second_upper_byte =
        ai_micro_mab_retry_high_translated_lbu(second_upper_byte_load_vaddr);
    TEST_ASSERT("after the first boundary+8 byte probe has already completed, a second-byte translated load at boundary+9 should still clear stale exception metadata and read the retried high-half byte1",
        excpt.triggered == false &&
        observed_second_upper_byte == expected_second_upper_byte
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, and trailing dual-head-byte load sequence",
        ai_micro_mab_retry_high_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x0a, 0xf8, 0xe6, 0xd4, 0xc2, 0xb0, 0x9e, 0x8c,
        0x7a, 0x68, 0x56, 0x44, 0x32, 0x20, 0x1e, 0x0c,
    };
    static const uint8_t guard_before_seed[8] = {
        0x25, 0x37, 0x49, 0x5b, 0x5d, 0x6f, 0x81, 0x93,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa4, 0xb6, 0xc8, 0xda, 0xec, 0xfe, 0x10, 0x22,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t second_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 9U;
    const uintptr_t first_upper_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xc8d9eafb0c1d2e3fULL;
    const uint64_t refault_store_value = 0x596a7b8c9daebfd0ULL;
    const uint64_t aligned_upper_store_value_first = 0xa9bbcddff1031527ULL;
    const uint64_t aligned_upper_store_value_second = 0x5a6c7e90a2b4c6d8ULL;
    const uint64_t aligned_upper_store_value_third = 0xbaccedf002142638ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x6b7d8fa1b3c5d7e9ULL;
    const uint64_t retry_same_template_store_value = 0xe8f90a1b2c3d4e5fULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_byte = 0ULL;
    uint64_t expected_second_upper_byte = 0ULL;
    uint64_t expected_first_upper_halfword = 0ULL;
    uint64_t observed_first_upper_byte = 0ULL;
    uint64_t observed_second_upper_byte = 0ULL;
    uint64_t observed_first_upper_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_reset_env();
    ai_micro_mab_retry_high_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_prepare_supervisor_translation();
    ai_micro_mab_retry_high_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, trailing dual-head-byte probes, and the final first-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, trailing dual-head-byte probes, and the final first-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-dual-head-byte-and-first-upper-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-dual-head-byte-and-first-upper-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-dual-head-byte-and-first-upper-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-dual-head-byte-and-first-upper-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, trailing dual-head-byte probes, and the final first-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing dual-head-byte probes and final first-upper-halfword load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_byte = (uint64_t)expected_boundary[8];
    expected_second_upper_byte = (uint64_t)expected_boundary[9];
    expected_first_upper_halfword =
        (uint64_t)(uint16_t)ai_micro_mab_retry_high_u64_from_bytes(
            expected_boundary + 8U);

    TEST_SETUP_EXCEPT();
    observed_first_upper_byte =
        ai_micro_mab_retry_high_translated_lbu(first_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-byte translated load at boundary+8 should clear stale exception metadata and read the retried high-half byte0 before the following head probes",
        excpt.triggered == false &&
        observed_first_upper_byte == expected_first_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_second_upper_byte =
        ai_micro_mab_retry_high_translated_lbu(second_upper_byte_load_vaddr);
    TEST_ASSERT("after the boundary+8 byte probe has already completed, a second-byte translated load at boundary+9 should still clear stale exception metadata and read the retried high-half byte1 before the final first-upper-halfword probe",
        excpt.triggered == false &&
        observed_second_upper_byte == expected_second_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_halfword =
        ai_micro_mab_retry_high_translated_lhu(first_upper_halfword_load_vaddr);
    TEST_ASSERT("after the dual head-byte probes have already shown retried high-half bytes0-1 individually visible, a first-upper-halfword translated load at boundary+8 should still clear stale exception metadata and read the combined retried high-half bytes0-1",
        excpt.triggered == false &&
        observed_first_upper_halfword == expected_first_upper_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, trailing dual-head-byte probes, and the final first-upper-halfword load sequence",
        ai_micro_mab_retry_high_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_halfword_corner");
}
