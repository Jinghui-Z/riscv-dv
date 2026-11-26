#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep refault-refresh probes in a dedicated file so the aligned recovery
 * extension file does not keep growing without bound.
 */

#define AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_REFAULT_EXT_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_refault_ext_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_refault_ext_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_refault_ext_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_refault_ext_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_refault_ext_set_leaf_paddr(page, phys_page_base(page), flags);
}

static inline void ai_micro_mab_refault_ext_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_refault_ext_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_RWX);
}

static inline void ai_micro_mab_refault_ext_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_refault_ext_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_refault_ext_reset_env(void)
{
    ai_micro_mab_refault_ext_restore_pmp_full_access();
    ai_micro_mab_refault_ext_clear_machine_access_mode();
}

static inline void ai_micro_mab_refault_ext_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_refault_ext_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_refault_ext_overlay_little_endian_bytes(
    uint8_t *dst, const uint8_t *seed, unsigned len,
    unsigned start_byte, uint64_t value, unsigned size)
{
    for (unsigned i = 0; i < len; ++i) {
        dst[i] = seed[i];
    }
    for (unsigned i = 0; i < size; ++i) {
        dst[start_byte + i] = (uint8_t)((value >> (8U * i)) & 0xffU);
    }
}

static inline uint64_t ai_micro_mab_refault_ext_u64_from_bytes(
    const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }
    return value;
}

#define AI_MICRO_MAB_REFAULT_EXT_TRANSLATED_STORE_INSTRUCTION(name, inst, type) \
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

AI_MICRO_MAB_REFAULT_EXT_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_refault_ext_translated_sd, "sd", uint64_t)

#define AI_MICRO_MAB_REFAULT_EXT_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
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

AI_MICRO_MAB_REFAULT_EXT_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_refault_ext_translated_ld, "ld")

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_reports_precise_fault_and_preserves_seeded_high_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x67, 0x55, 0x43, 0x31, 0x1f, 0x0d, 0xfb, 0xe9,
        0xd8, 0xc6, 0xb4, 0xa2, 0x90, 0x7e, 0x6c, 0x5a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x23, 0x35, 0x47, 0x59, 0x6b, 0x7d, 0x8f, 0xa1,
    };
    static const uint8_t guard_after_seed[8] = {
        0xb2, 0xc4, 0xd6, 0xe8, 0xfa, 0x0c, 0x1e, 0x30,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5263748596a7b8c9ULL;
    const uint64_t refault_store_value = 0xdaebfc0d1e2f4051ULL;
    const uint64_t aligned_low_store_value_first = 0x132435465768798aULL;
    const uint64_t aligned_low_store_value_second = 0x9bacbdcedff00112ULL;
    const uint64_t aligned_low_store_value_third = 0x233445566778899aULL;
    const uint64_t aligned_low_store_value_fourth = 0xabbccddeef102132ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_refault_ext_reset_env();
    ai_micro_mab_refault_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);
    ai_micro_mab_refault_ext_prepare_supervisor_translation();
    ai_micro_mab_refault_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged low-half producer continuation and refault refresh probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged low-half producer continuation and refault refresh probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault refresh probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after one low-half aligned translated store has already completed on the repaired boundary, the second low-half aligned translated store should stay trap-free before the refault refresh probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the repaired boundary, the third low-half aligned translated store should stay trap-free before the refault refresh probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the repaired boundary, the fourth low-half aligned translated store should stay trap-free and become the latest committed low-half payload before the refault refresh probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));
    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_fourth, sizeof(uint64_t));

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four low-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata instead of surfacing stale success-path state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_ASSERT("the refault refresh probe should not clobber the untouched upper-half boundary bytes that remained seeded throughout the prolonged low-half producer continuation",
        ai_micro_mab_refault_ext_bytes_match(boundary_phys + 8U,
            expected_boundary + 8U, 8U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four low-half aligned stores, and refault refresh sequence",
        ai_micro_mab_refault_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_reports_precise_fault_and_preserves_seeded_high_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_upper_load_clears_stale_exception_and_reads_seeded_high_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10,
        0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01,
    };
    static const uint8_t guard_before_seed[8] = {
        0x14, 0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82,
    };
    static const uint8_t guard_after_seed[8] = {
        0x93, 0xa5, 0xb7, 0xc9, 0xdb, 0xed, 0xff, 0x11,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x31425364758697a8ULL;
    const uint64_t refault_store_value = 0xc1d2e3f405162738ULL;
    const uint64_t aligned_low_store_value_first = 0x1021324354657687ULL;
    const uint64_t aligned_low_store_value_second = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_low_store_value_third = 0x2132435465768798ULL;
    const uint64_t aligned_low_store_value_fourth = 0xa0b1c2d3e4f50617ULL;
    const uint64_t expected_seeded_high_value =
        ai_micro_mab_refault_ext_u64_from_bytes(boundary_seed + 8U);
    uint64_t loaded_upper_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_ext_reset_env();
    ai_micro_mab_refault_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);
    ai_micro_mab_refault_ext_prepare_supervisor_translation();
    ai_micro_mab_refault_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged low-half producer, refault, and repaired upper-load stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged low-half producer, refault, and repaired upper-load stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after one low-half aligned translated store has already completed on the repaired boundary, the second low-half aligned translated store should stay trap-free before the refault-followed-by-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the repaired boundary, the third low-half aligned translated store should stay trap-free before the refault-followed-by-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the repaired boundary, the fourth low-half aligned translated store should stay trap-free and become the latest committed low-half payload before the refault-followed-by-upper-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four low-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-load stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    loaded_upper_value = ai_micro_mab_refault_ext_translated_ld(
        aligned_upper_load_vaddr);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated load should clear stale exception metadata from that refault instead of inheriting the prior store access fault",
        excpt.triggered == false
    );

    TEST_ASSERT("after the prolonged low-half producer continuation and repaired refault, the first upper-half aligned translated load should still read the untouched seeded high-half image",
        loaded_upper_value == expected_seeded_high_value
    );

    TEST_ASSERT("the repaired refault-followed-by-upper-load probe should not clobber the untouched upper-half boundary bytes that remained seeded throughout the prolonged low-half producer continuation",
        ai_micro_mab_refault_ext_bytes_match(boundary_phys + 8U,
            boundary_seed + 8U, 8U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four low-half aligned stores, refault, and repaired upper-load sequence",
        ai_micro_mab_refault_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_upper_load_clears_stale_exception_and_reads_seeded_high_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_load_clears_stale_exception_and_reads_latest_high_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x71, 0x5f, 0x4d, 0x3b, 0x29, 0x17, 0x05, 0xf3,
        0xe1, 0xcf, 0xbd, 0xab, 0x99, 0x87, 0x75, 0x63,
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
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x425364758697a8b9ULL;
    const uint64_t refault_store_value = 0xd4e5f60718293a4bULL;
    const uint64_t aligned_upper_store_value_first = 0x1029384756a1b2c3ULL;
    const uint64_t aligned_upper_store_value_second = 0x64758697a8b9cadbULL;
    const uint64_t aligned_upper_store_value_third = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x89abcdeffedcba98ULL;
    uint8_t expected_high_half[8];
    uint64_t loaded_upper_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_ext_reset_env();
    ai_micro_mab_refault_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);
    ai_micro_mab_refault_ext_prepare_supervisor_translation();
    ai_micro_mab_refault_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, and repaired upper-load stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, and repaired upper-load stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-load stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    for (unsigned i = 0; i < 8U; ++i) {
        expected_high_half[i] =
            (uint8_t)((aligned_upper_store_value_fourth >> (8U * i)) & 0xffU);
    }

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    loaded_upper_value = ai_micro_mab_refault_ext_translated_ld(
        aligned_upper_load_vaddr);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated load should clear stale exception metadata from that refault instead of inheriting the prior store access fault",
        excpt.triggered == false
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation and repaired refault, the first upper-half aligned translated load should still read the latest committed high-half payload",
        loaded_upper_value == aligned_upper_store_value_fourth
    );

    TEST_ASSERT("the repaired refault-followed-by-upper-load probe should not clobber the latest committed high-half boundary bytes throughout the prolonged upper-half producer continuation",
        ai_micro_mab_refault_ext_bytes_match(boundary_phys + 8U,
            expected_high_half, 8U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, and repaired upper-load sequence",
        ai_micro_mab_refault_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_load_clears_stale_exception_and_reads_latest_high_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_low_load_clears_stale_exception_and_reads_refault_updated_low_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x5d, 0x4b, 0x39, 0x27, 0x15, 0x03, 0xf1, 0xdf,
        0xce, 0xbc, 0xaa, 0x98, 0x86, 0x74, 0x62, 0x50,
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
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x64758697a8b9cadbULL;
    const uint64_t refault_store_value = 0xf203142536475869ULL;
    const uint64_t aligned_low_store_value_first = 0x1021324354657687ULL;
    const uint64_t aligned_low_store_value_second = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_low_store_value_third = 0x2132435465768798ULL;
    const uint64_t aligned_low_store_value_fourth = 0xa0b1c2d3e4f50617ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_refault_updated_low_value = 0ULL;
    uint64_t loaded_low_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_ext_reset_env();
    ai_micro_mab_refault_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);
    ai_micro_mab_refault_ext_prepare_supervisor_translation();
    ai_micro_mab_refault_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged low-half producer, refault, and repaired low-load stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged low-half producer, refault, and repaired low-load stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after one low-half aligned translated store has already completed on the repaired boundary, the second low-half aligned translated store should stay trap-free before the refault-followed-by-low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the repaired boundary, the third low-half aligned translated store should stay trap-free before the refault-followed-by-low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the repaired boundary, the fourth low-half aligned translated store should stay trap-free and become the latest committed low-half payload before the refault-followed-by-low-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));
    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_fourth, sizeof(uint64_t));

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four low-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired low-load stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    expected_refault_updated_low_value =
        ai_micro_mab_refault_ext_u64_from_bytes(expected_boundary);

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    loaded_low_value = ai_micro_mab_refault_ext_translated_ld(
        aligned_low_load_vaddr);
    TEST_ASSERT("after the refault episode is repaired again, the first low-half aligned translated load should clear stale exception metadata from that refault instead of inheriting the prior store access fault",
        excpt.triggered == false
    );

    TEST_ASSERT("after the prolonged low-half producer continuation and repaired refault, the first low-half aligned translated load should read the current committed low-half image without a visible byte from the refaulting split store",
        loaded_low_value == expected_refault_updated_low_value
    );

    TEST_ASSERT("the repaired refault-followed-by-low-load probe should keep the full same-page boundary image equal to the four committed low-half stores while leaving the seeded high half intact",
        ai_micro_mab_refault_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four low-half aligned stores, refault, and repaired low-load sequence",
        ai_micro_mab_refault_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_low_load_clears_stale_exception_and_reads_refault_updated_low_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_low_store_clears_stale_exception_and_commits_latest_low_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x4e, 0x3c, 0x2a, 0x18, 0x06, 0xf4, 0xe2, 0xd0,
        0xbf, 0xad, 0x9b, 0x89, 0x77, 0x65, 0x53, 0x41,
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
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5263748596a7b8c9ULL;
    const uint64_t refault_store_value = 0xdaebfc0d1e2f4051ULL;
    const uint64_t aligned_low_store_value_first = 0x132435465768798aULL;
    const uint64_t aligned_low_store_value_second = 0x9bacbdcedff00112ULL;
    const uint64_t aligned_low_store_value_third = 0x233445566778899aULL;
    const uint64_t aligned_low_store_value_fourth = 0xabbccddeef102132ULL;
    const uint64_t repaired_low_store_value = 0x31425364758697a8ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_refault_ext_reset_env();
    ai_micro_mab_refault_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);
    ai_micro_mab_refault_ext_prepare_supervisor_translation();
    ai_micro_mab_refault_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged low-half producer, refault, and repaired low-store stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged low-half producer, refault, and repaired low-store stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-low-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after one low-half aligned translated store has already completed on the repaired boundary, the second low-half aligned translated store should stay trap-free before the refault-followed-by-low-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the repaired boundary, the third low-half aligned translated store should stay trap-free before the refault-followed-by-low-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the repaired boundary, the fourth low-half aligned translated store should stay trap-free and become the latest committed low-half payload before the refault-followed-by-low-store probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four low-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired low-store stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_low_store_vaddr,
        repaired_low_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first low-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault",
        excpt.triggered == false
    );

    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        repaired_low_store_value, sizeof(uint64_t));

    TEST_ASSERT("after the prolonged low-half producer continuation and repaired refault, the first low-half aligned translated store should become the latest committed low-half payload while keeping the seeded high half intact",
        ai_micro_mab_refault_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four low-half aligned stores, refault, and repaired low-store sequence",
        ai_micro_mab_refault_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_low_store_clears_stale_exception_and_commits_latest_low_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_clears_stale_exception_and_commits_latest_high_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x73, 0x61, 0x4f, 0x3d, 0x2b, 0x19, 0x07, 0xf5,
        0xe4, 0xd2, 0xc0, 0xae, 0x9c, 0x8a, 0x78, 0x66,
    };
    static const uint8_t guard_before_seed[8] = {
        0x15, 0x27, 0x39, 0x4b, 0x5d, 0x6f, 0x71, 0x83,
    };
    static const uint8_t guard_after_seed[8] = {
        0x94, 0xa6, 0xb8, 0xca, 0xdc, 0xee, 0xf0, 0x12,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x425364758697a8b9ULL;
    const uint64_t refault_store_value = 0xd4e5f60718293a4bULL;
    const uint64_t aligned_upper_store_value_first = 0x1029384756a1b2c3ULL;
    const uint64_t aligned_upper_store_value_second = 0x64758697a8b9cadbULL;
    const uint64_t aligned_upper_store_value_third = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x89abcdeffedcba98ULL;
    const uint64_t repaired_upper_store_value = 0x2132435465768798ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_refault_ext_reset_env();
    ai_micro_mab_refault_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);
    ai_micro_mab_refault_ext_prepare_supervisor_translation();
    ai_micro_mab_refault_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, and repaired upper-store stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, and repaired upper-store stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-store probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-store stale-exception-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_ext_translated_sd(aligned_upper_store_vaddr,
        repaired_upper_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault",
        excpt.triggered == false
    );

    ai_micro_mab_refault_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        repaired_upper_store_value, sizeof(uint64_t));

    TEST_ASSERT("after the prolonged upper-half producer continuation and repaired refault, the first upper-half aligned translated store should become the latest committed high-half payload while preserving the seeded low half because the refaulting store has no architectural low-byte side effect",
        ai_micro_mab_refault_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, and repaired upper-store sequence",
        ai_micro_mab_refault_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_clears_stale_exception_and_commits_latest_high_half_corner");
}
