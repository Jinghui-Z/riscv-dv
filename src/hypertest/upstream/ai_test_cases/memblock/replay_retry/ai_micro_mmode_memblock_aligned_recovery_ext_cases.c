#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep later same-page aligned-recovery probes in a separate file so the
 * original aligned_recovery_cases.c does not keep growing without bound.
 */

#define AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_aligned_ext_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_aligned_ext_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_aligned_ext_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_aligned_ext_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_aligned_ext_set_leaf_paddr(page, phys_page_base(page), flags);
}

static inline void ai_micro_mab_aligned_ext_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_aligned_ext_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_RWX);
}

static inline void ai_micro_mab_aligned_ext_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_aligned_ext_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_aligned_ext_reset_env(void)
{
    ai_micro_mab_aligned_ext_restore_pmp_full_access();
    ai_micro_mab_aligned_ext_clear_machine_access_mode();
}

static inline void ai_micro_mab_aligned_ext_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_aligned_ext_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
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

static inline uint64_t ai_micro_mab_aligned_ext_u64_from_bytes(
    const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }
    return value;
}

#define AI_MICRO_MAB_ALIGNED_EXT_TRANSLATED_STORE_INSTRUCTION(name, inst, type) \
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

AI_MICRO_MAB_ALIGNED_EXT_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_aligned_ext_translated_sd, "sd", uint64_t)

#define AI_MICRO_MAB_ALIGNED_EXT_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
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

AI_MICRO_MAB_ALIGNED_EXT_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_aligned_ext_translated_ld, "ld")

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x5f, 0x4d, 0x3b, 0x29, 0x17, 0x05, 0xf4, 0xe2,
        0xd0, 0xbe, 0xac, 0x9a, 0x88, 0x76, 0x64, 0x52,
    };
    static const uint8_t guard_before_seed[8] = {
        0x91, 0xa3, 0xb5, 0xc7, 0xd9, 0xeb, 0xfd, 0x0f,
    };
    static const uint8_t guard_after_seed[8] = {
        0x2c, 0x3e, 0x50, 0x62, 0x74, 0x86, 0x98, 0xaa,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xa7b8c9daebfc0d1eULL;
    const uint64_t aligned_low_store_value_first = 0x18293a4b5c6d7e8fULL;
    const uint64_t aligned_low_store_value_second = 0x90a1b2c3d4e5f607ULL;
    const uint64_t aligned_low_store_value_third = 0x6b7c8d9eafc0d1e2ULL;
    const uint64_t aligned_upper_store_value = 0x31425364758697a8ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct, repeated, and third low-half aligned store plus final upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct, repeated, and third low-half aligned store plus final upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the repeated low-half and final upper-half store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the repeated low-half aligned translated store should still stay trap-free and keep the later probe focused on the latest low-half payload",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the same half, the third low-half aligned translated store should still stay trap-free and preserve the seeded high half before the final upper-half producer switch",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the same half, the following upper-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the latest committed low half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the latest committed low half from the third low-half aligned store while the final upper-half aligned store updates only the high half on the opposite half",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct low-half aligned store, repeated low-half aligned store, third low-half aligned store, and final upper-half aligned store sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa4, 0x92, 0x80, 0x6e, 0x5c, 0x4a, 0x38, 0x26,
        0x14, 0x02, 0xf1, 0xdf, 0xcd, 0xbb, 0xa9, 0x97,
    };
    static const uint8_t guard_before_seed[8] = {
        0x0d, 0x1f, 0x31, 0x43, 0x55, 0x67, 0x79, 0x8b,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe4, 0xd2, 0xc0, 0xae, 0x9c, 0x8a, 0x78, 0x66,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xc6d7e8f90a1b2c3dULL;
    const uint64_t aligned_upper_store_value_first = 0x2738495a6b7c8d9eULL;
    const uint64_t aligned_upper_store_value_second = 0xafc0d1e2f3041526ULL;
    const uint64_t aligned_upper_store_value_third = 0x718293a4b5c6d7e8ULL;
    const uint64_t aligned_low_store_value = 0x3a4b5c6d7e8fa0b1ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct, repeated, and third upper-half aligned store plus final low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct, repeated, and third upper-half aligned store plus final low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the repeated upper-half and final low-half store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the repeated upper-half aligned translated store should still stay trap-free and keep the later probe focused on the latest high-half payload",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the same half, the third upper-half aligned translated store should still stay trap-free and preserve the seeded low half before the final low-half producer switch",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the same half, the following low-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the latest committed high half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the latest committed high half from the third upper-half aligned store while the final low-half aligned store updates only the low half on the opposite half",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, repeated upper-half aligned store, third upper-half aligned store, and final low-half aligned store sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_preserves_seeded_low_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x1a, 0x2c, 0x3e, 0x50, 0x62, 0x74, 0x86, 0x98,
        0xab, 0xbd, 0xcf, 0xe1, 0xf3, 0x05, 0x17, 0x29,
    };
    static const uint8_t guard_before_seed[8] = {
        0xc8, 0xda, 0xec, 0xfe, 0x10, 0x22, 0x34, 0x46,
    };
    static const uint8_t guard_after_seed[8] = {
        0x39, 0x27, 0x15, 0x03, 0xf2, 0xe0, 0xce, 0xbc,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xd7e8f90a1b2c3d4eULL;
    const uint64_t aligned_upper_store_value_first = 0x28394a5b6c7d8e9fULL;
    const uint64_t aligned_upper_store_value_second = 0xa0b1c2d3e4f50617ULL;
    const uint64_t aligned_upper_store_value_third = 0x728394a5b6c7d8e9ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x4b5c6d7e8fa0b1c2ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct and prolonged repeated upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct and prolonged repeated upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the extended same-half high-half store continuation",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the second upper-half aligned translated store should still stay trap-free and preserve the seeded low half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the same half, the third upper-half aligned translated store should still stay trap-free and preserve the seeded low half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the same half, the fourth upper-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the seeded low half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded low half untouched while the fourth upper-half aligned store updates only the high half to its latest payload",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, second upper-half aligned store, third upper-half aligned store, and fourth upper-half aligned store sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_preserves_seeded_low_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xb9, 0xa7, 0x95, 0x83, 0x71, 0x5f, 0x4d, 0x3b,
        0x2a, 0x18, 0x06, 0xf4, 0xe2, 0xd0, 0xbe, 0xac,
    };
    static const uint8_t guard_before_seed[8] = {
        0x47, 0x59, 0x6b, 0x7d, 0x8f, 0xa1, 0xb3, 0xc5,
    };
    static const uint8_t guard_after_seed[8] = {
        0xde, 0xcc, 0xba, 0xa8, 0x96, 0x84, 0x72, 0x60,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xe8f90a1b2c3d4e5fULL;
    const uint64_t aligned_low_store_value_first = 0x394a5b6c7d8e9fb0ULL;
    const uint64_t aligned_low_store_value_second = 0xb2c3d4e5f6071829ULL;
    const uint64_t aligned_low_store_value_third = 0x8394a5b6c7d8e9faULL;
    const uint64_t aligned_low_store_value_fourth = 0x5c6d7e8fa0b1c2d3ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct and prolonged repeated low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct and prolonged repeated low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the extended same-half low-half store continuation",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the second low-half aligned translated store should still stay trap-free and preserve the seeded high half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the same half, the third low-half aligned translated store should still stay trap-free and preserve the seeded high half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the same half, the fourth low-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the seeded high half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded high half untouched while the fourth low-half aligned store updates only the low half to its latest payload",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct low-half aligned store, second low-half aligned store, third low-half aligned store, and fourth low-half aligned store sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x63, 0x51, 0x3f, 0x2d, 0x1b, 0x09, 0xf7, 0xe5,
        0xd4, 0xc2, 0xb0, 0x9e, 0x8c, 0x7a, 0x68, 0x56,
    };
    static const uint8_t guard_before_seed[8] = {
        0x19, 0x2b, 0x3d, 0x4f, 0x61, 0x73, 0x85, 0x97,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa8, 0xba, 0xcc, 0xde, 0xf0, 0x12, 0x24, 0x36,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x7a8b9cadbecfe0f1ULL;
    const uint64_t aligned_low_store_value_first = 0x2132435465768798ULL;
    const uint64_t aligned_low_store_value_second = 0xa9bacbdcedfe1021ULL;
    const uint64_t aligned_low_store_value_third = 0x32435465768798a9ULL;
    const uint64_t aligned_low_store_value_fourth = 0xcbdcedfe10213243ULL;
    const uint64_t aligned_upper_store_value = 0x5a6b7c8d9eafc0d1ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated low-half producer continuation and final upper-half producer switch",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated low-half producer continuation and final upper-half producer switch",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the extended same-half continuation and final upper-half producer switch",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the second low-half aligned translated store should still stay trap-free and preserve the later final upper-half producer switch as a clean probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the same half, the third low-half aligned translated store should still stay trap-free and preserve the eventual opposite-half producer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the same half, the fourth low-half aligned translated store should still stay trap-free and preserve the latest committed low half before the final upper-half producer switch",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after four low-half aligned translated stores have already completed on the same half, the final upper-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the latest committed low half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the fourth low-half aligned store as the latest committed low-half payload while the final upper-half aligned store updates only the high half to its own payload",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four low-half aligned stores, and final upper-half aligned store sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x2f, 0x41, 0x53, 0x65, 0x77, 0x89, 0x9b, 0xad,
        0xbe, 0xd0, 0xe2, 0xf4, 0x06, 0x18, 0x2a, 0x3c,
    };
    static const uint8_t guard_before_seed[8] = {
        0x8a, 0x9c, 0xae, 0xc0, 0xd2, 0xe4, 0xf6, 0x08,
    };
    static const uint8_t guard_after_seed[8] = {
        0x17, 0x29, 0x3b, 0x4d, 0x5f, 0x71, 0x83, 0x95,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x6d7e8fa0b1c2d3e4ULL;
    const uint64_t aligned_upper_store_value_first = 0x1021324354657687ULL;
    const uint64_t aligned_upper_store_value_second = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_third = 0x2132435465768798ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xbacbdcedfe0f1021ULL;
    const uint64_t aligned_low_store_value = 0x4d5e6f8091a2b3c4ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated upper-half producer continuation and final low-half producer switch",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated upper-half producer continuation and final low-half producer switch",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the extended same-half continuation and final low-half producer switch",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the second upper-half aligned translated store should still stay trap-free and preserve the later final low-half producer switch as a clean probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the same half, the third upper-half aligned translated store should still stay trap-free and preserve the eventual opposite-half producer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the same half, the fourth upper-half aligned translated store should still stay trap-free and preserve the latest committed high half before the final low-half producer switch",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the same half, the final low-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the latest committed high half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the fourth upper-half aligned store as the latest committed high-half payload while the final low-half aligned store updates only the low half to its own payload",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, and final low-half aligned store sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_load_preserves_seeded_low_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x6e, 0x5c, 0x4a, 0x38, 0x26, 0x14, 0x02, 0xf0,
        0xdf, 0xcd, 0xbb, 0xa9, 0x97, 0x85, 0x73, 0x61,
    };
    static const uint8_t guard_before_seed[8] = {
        0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x90,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa1, 0xb3, 0xc5, 0xd7, 0xe9, 0xfb, 0x0d, 0x1f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x748596a7b8c9daebULL;
    const uint64_t aligned_upper_store_value_first = 0x132435465768798aULL;
    const uint64_t aligned_upper_store_value_second = 0x9bacbdcedff00112ULL;
    const uint64_t aligned_upper_store_value_third = 0x2435465768798a9bULL;
    const uint64_t aligned_upper_store_value_fourth = 0xacbdcedff0011223ULL;
    const uint64_t expected_low_value =
        ai_micro_mab_aligned_ext_u64_from_bytes(boundary_seed);
    uint64_t loaded_low_value = 0ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated upper-half producer continuation and final low-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated upper-half producer continuation and final low-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the extended same-half continuation and final low-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the second upper-half aligned translated store should still stay trap-free and preserve the later final low-half load probe as a clean consumer check",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the same half, the third upper-half aligned translated store should still stay trap-free and preserve the eventual untouched low-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the same half, the fourth upper-half aligned translated store should still stay trap-free and preserve the seeded low half before the final low-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    loaded_low_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_low_load_vaddr);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the same half, the final low-half aligned load should still stay trap-free instead of reviving stale split-state or corrupting the seeded low half",
        excpt.triggered == false
    );

    TEST_ASSERT("the final low-half aligned load should read back the original seeded low half after four upper-half aligned stores have already committed on the opposite half",
        loaded_low_value == expected_low_value
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded low half untouched while the fourth upper-half aligned store remains the latest committed high-half payload",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, and final low-half aligned load sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_load_preserves_seeded_low_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_load_preserves_seeded_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa4, 0x92, 0x80, 0x6e, 0x5c, 0x4a, 0x38, 0x26,
        0x15, 0x03, 0xf1, 0xdf, 0xcd, 0xbb, 0xa9, 0x97,
    };
    static const uint8_t guard_before_seed[8] = {
        0x21, 0x33, 0x45, 0x57, 0x69, 0x7b, 0x8d, 0x9f,
    };
    static const uint8_t guard_after_seed[8] = {
        0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x8596a7b8c9daebfcULL;
    const uint64_t aligned_low_store_value_first = 0x31425364758697a8ULL;
    const uint64_t aligned_low_store_value_second = 0xb9cadbecfd0e1f20ULL;
    const uint64_t aligned_low_store_value_third = 0x425364758697a8b9ULL;
    const uint64_t aligned_low_store_value_fourth = 0xcadbecfd0e1f2031ULL;
    const uint64_t expected_upper_value =
        ai_micro_mab_aligned_ext_u64_from_bytes(boundary_seed + 8U);
    uint64_t loaded_upper_value = 0ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated low-half producer continuation and final upper-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated low-half producer continuation and final upper-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the extended same-half continuation and final upper-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the second low-half aligned translated store should still stay trap-free and preserve the later final upper-half load probe as a clean consumer check",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the same half, the third low-half aligned translated store should still stay trap-free and preserve the eventual untouched upper-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the same half, the fourth low-half aligned translated store should still stay trap-free and preserve the seeded high half before the final upper-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    loaded_upper_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_upper_load_vaddr);
    TEST_ASSERT("after four low-half aligned translated stores have already completed on the same half, the final upper-half aligned load should still stay trap-free instead of reviving stale split-state or corrupting the seeded high half",
        excpt.triggered == false
    );

    TEST_ASSERT("the final upper-half aligned load should read back the original seeded high half after four low-half aligned stores have already committed on the opposite half",
        loaded_upper_value == expected_upper_value
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded high half untouched while the fourth low-half aligned store remains the latest committed low-half payload",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four low-half aligned stores, and final upper-half aligned load sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_load_preserves_seeded_high_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_low_load_reads_latest_low_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x69, 0x57, 0x45, 0x33, 0x21, 0x0f, 0xfd, 0xeb,
        0xda, 0xc8, 0xb6, 0xa4, 0x92, 0x80, 0x6e, 0x5c,
    };
    static const uint8_t guard_before_seed[8] = {
        0x42, 0x54, 0x66, 0x78, 0x8a, 0x9c, 0xae, 0xc0,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd1, 0xe3, 0xf5, 0x07, 0x19, 0x2b, 0x3d, 0x4f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xa7b8c9daebfc0d1eULL;
    const uint64_t aligned_low_store_value_first = 0x5263748596a7b8c9ULL;
    const uint64_t aligned_low_store_value_second = 0xdaebfc0d1e2f4051ULL;
    const uint64_t aligned_low_store_value_third = 0x63748596a7b8c9daULL;
    const uint64_t aligned_low_store_value_fourth = 0xebfc0d1e2f405162ULL;
    uint64_t loaded_low_value = 0ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated low-half producer continuation and final same-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated low-half producer continuation and final same-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the extended same-half continuation and final same-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the second low-half aligned translated store should still stay trap-free and preserve the later final same-half load probe as a clean consumer check",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the same half, the third low-half aligned translated store should still stay trap-free and preserve the eventual same-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the same half, the fourth low-half aligned translated store should still stay trap-free and preserve the latest committed low half before the final same-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    loaded_low_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_low_load_vaddr);
    TEST_ASSERT("after four low-half aligned translated stores have already completed on the same half, the final low-half aligned load should still stay trap-free instead of reviving stale split-state or reading an older low-half image",
        excpt.triggered == false
    );

    TEST_ASSERT("the final low-half aligned load should read back the fourth low-half aligned store as the latest committed low-half payload",
        loaded_low_value == aligned_low_store_value_fourth
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded high half untouched while the fourth low-half aligned store remains the latest committed low-half payload before the final same-half load probe",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four low-half aligned stores, and final low-half aligned load sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_low_load_reads_latest_low_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_low_load_then_upper_load_observes_latest_low_and_seeded_high_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x7a, 0x68, 0x56, 0x44, 0x32, 0x20, 0x1e, 0x0c,
        0xfb, 0xe9, 0xd7, 0xc5, 0xb3, 0xa1, 0x8f, 0x7d,
    };
    static const uint8_t guard_before_seed[8] = {
        0x53, 0x65, 0x77, 0x89, 0x9b, 0xad, 0xbf, 0xd1,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe2, 0xf4, 0x06, 0x18, 0x2a, 0x3c, 0x4e, 0x60,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xb8c9daebfc0d1e2fULL;
    const uint64_t aligned_low_store_value_first = 0x61728394a5b6c7d8ULL;
    const uint64_t aligned_low_store_value_second = 0xe9fa0b1c2d3e4f50ULL;
    const uint64_t aligned_low_store_value_third = 0x728394a5b6c7d8e9ULL;
    const uint64_t aligned_low_store_value_fourth = 0xfa0b1c2d3e4f5061ULL;
    const uint64_t expected_upper_value =
        ai_micro_mab_aligned_ext_u64_from_bytes(boundary_seed + 8U);
    uint64_t loaded_low_value = 0ULL;
    uint64_t loaded_upper_value = 0ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated low-half producer continuation and dual load consumer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated low-half producer continuation and dual load consumer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the prolonged dual load consumer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after one low-half aligned translated store has already completed on the repaired boundary, the second low-half aligned translated store should stay trap-free before the low-load then upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the repaired boundary, the third low-half aligned translated store should stay trap-free before the low-load then upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the repaired boundary, the fourth low-half aligned translated store should stay trap-free and become the latest committed low-half payload before the dual load consumer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    loaded_low_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_low_load_vaddr);
    TEST_ASSERT("after four low-half aligned translated stores have already completed on the same half, the first low-half aligned load should still stay trap-free instead of reviving stale split-state or reading an older low-half image",
        excpt.triggered == false
    );

    TEST_ASSERT("the first low-half aligned load in the dual probe should read back the fourth low-half aligned store as the latest committed low-half payload",
        loaded_low_value == aligned_low_store_value_fourth
    );

    TEST_SETUP_EXCEPT();
    loaded_upper_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_upper_load_vaddr);
    TEST_ASSERT("after the same-half low-half aligned load has already consumed the latest repaired boundary image, the following upper-half aligned load should still stay trap-free instead of reviving stale split-state or corrupting the seeded high half",
        excpt.triggered == false
    );

    TEST_ASSERT("the second load in the dual probe should still read back the original seeded high half after the first low-half load has already observed the latest low-half payload",
        loaded_upper_value == expected_upper_value
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded high half untouched while the fourth low-half aligned store remains the latest committed low-half payload across the dual load probe",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four low-half aligned stores, and low-load then upper-load dual consumer sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_low_load_then_upper_load_observes_latest_low_and_seeded_high_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_upper_load_reads_latest_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x58, 0x46, 0x34, 0x22, 0x10, 0xfe, 0xec, 0xda,
        0xc9, 0xb7, 0xa5, 0x93, 0x81, 0x6f, 0x5d, 0x4b,
    };
    static const uint8_t guard_before_seed[8] = {
        0x31, 0x43, 0x55, 0x67, 0x79, 0x8b, 0x9d, 0xaf,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc0, 0xd2, 0xe4, 0xf6, 0x08, 0x1a, 0x2c, 0x3e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x96a7b8c9daebfc0dULL;
    const uint64_t aligned_upper_store_value_first = 0x2435465768798a9bULL;
    const uint64_t aligned_upper_store_value_second = 0xacbdcedff0011223ULL;
    const uint64_t aligned_upper_store_value_third = 0x35465768798a9bacULL;
    const uint64_t aligned_upper_store_value_fourth = 0xbdcedff001122334ULL;
    uint64_t loaded_upper_value = 0ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated upper-half producer continuation and final same-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated upper-half producer continuation and final same-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the extended same-half continuation and final same-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the second upper-half aligned translated store should still stay trap-free and preserve the later final same-half load probe as a clean consumer check",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the same half, the third upper-half aligned translated store should still stay trap-free and preserve the eventual same-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the same half, the fourth upper-half aligned translated store should still stay trap-free and preserve the latest committed high half before the final same-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    loaded_upper_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_upper_load_vaddr);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the same half, the final upper-half aligned load should still stay trap-free instead of reviving stale split-state or reading an older high-half image",
        excpt.triggered == false
    );

    TEST_ASSERT("the final upper-half aligned load should read back the fourth upper-half aligned store as the latest committed high-half payload",
        loaded_upper_value == aligned_upper_store_value_fourth
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded low half untouched while the fourth upper-half aligned store remains the latest committed high-half payload before the final same-half load probe",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, and final upper-half aligned load sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_upper_load_reads_latest_high_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_upper_load_then_low_load_observes_latest_high_and_seeded_low_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x4b, 0x39, 0x27, 0x15, 0x03, 0xf1, 0xdf, 0xcd,
        0xbc, 0xaa, 0x98, 0x86, 0x74, 0x62, 0x50, 0x3e,
    };
    static const uint8_t guard_before_seed[8] = {
        0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x90, 0xa2,
    };
    static const uint8_t guard_after_seed[8] = {
        0xb3, 0xc5, 0xd7, 0xe9, 0xfb, 0x0d, 0x1f, 0x31,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xc9daebfc0d1e2f40ULL;
    const uint64_t aligned_upper_store_value_first = 0x35465768798a9bacULL;
    const uint64_t aligned_upper_store_value_second = 0xbdcedff001122334ULL;
    const uint64_t aligned_upper_store_value_third = 0x465768798a9bacbdULL;
    const uint64_t aligned_upper_store_value_fourth = 0xcedff00112233445ULL;
    const uint64_t expected_low_value =
        ai_micro_mab_aligned_ext_u64_from_bytes(boundary_seed);
    uint64_t loaded_upper_value = 0ULL;
    uint64_t loaded_low_value = 0ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated upper-half producer continuation and dual load consumer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated upper-half producer continuation and dual load consumer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the prolonged dual load consumer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the upper-load then low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the upper-load then low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the dual load consumer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    loaded_upper_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_upper_load_vaddr);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the same half, the first upper-half aligned load should still stay trap-free instead of reviving stale split-state or reading an older high-half image",
        excpt.triggered == false
    );

    TEST_ASSERT("the first upper-half aligned load in the dual probe should read back the fourth upper-half aligned store as the latest committed high-half payload",
        loaded_upper_value == aligned_upper_store_value_fourth
    );

    TEST_SETUP_EXCEPT();
    loaded_low_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_low_load_vaddr);
    TEST_ASSERT("after the same-half upper-half aligned load has already consumed the latest repaired boundary image, the following low-half aligned load should still stay trap-free instead of reviving stale split-state or corrupting the seeded low half",
        excpt.triggered == false
    );

    TEST_ASSERT("the second load in the dual probe should still read back the original seeded low half after the first upper-half load has already observed the latest high-half payload",
        loaded_low_value == expected_low_value
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded low half untouched while the fourth upper-half aligned store remains the latest committed high-half payload across the dual load probe",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, and upper-load then low-load dual consumer sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_upper_load_then_low_load_observes_latest_high_and_seeded_low_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_load_then_upper_load_observes_seeded_low_and_latest_high_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x5c, 0x4a, 0x38, 0x26, 0x14, 0x02, 0xf0, 0xde,
        0xcd, 0xbb, 0xa9, 0x97, 0x85, 0x73, 0x61, 0x4f,
    };
    static const uint8_t guard_before_seed[8] = {
        0x35, 0x47, 0x59, 0x6b, 0x7d, 0x8f, 0xa1, 0xb3,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc4, 0xd6, 0xe8, 0xfa, 0x0c, 0x1e, 0x30, 0x42,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xdaebfc0d1e2f4051ULL;
    const uint64_t aligned_upper_store_value_first = 0x5465768798a9bacbULL;
    const uint64_t aligned_upper_store_value_second = 0xdcedfe0f20314253ULL;
    const uint64_t aligned_upper_store_value_third = 0x65768798a9bacbdcULL;
    const uint64_t aligned_upper_store_value_fourth = 0xedfe0f2031425364ULL;
    const uint64_t expected_low_value =
        ai_micro_mab_aligned_ext_u64_from_bytes(boundary_seed);
    uint64_t loaded_low_value = 0ULL;
    uint64_t loaded_upper_value = 0ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated upper-half producer continuation and reversed dual load consumer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated upper-half producer continuation and reversed dual load consumer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the reversed dual load consumer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the low-load then upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the low-load then upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the reversed dual load consumer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    loaded_low_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_low_load_vaddr);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the same half, the first low-half aligned load should still stay trap-free instead of reviving stale split-state or corrupting the seeded low half",
        excpt.triggered == false
    );

    TEST_ASSERT("the first low-half aligned load in the reversed dual probe should still read back the original seeded low half before the later upper-half load observes the latest high-half payload",
        loaded_low_value == expected_low_value
    );

    TEST_SETUP_EXCEPT();
    loaded_upper_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_upper_load_vaddr);
    TEST_ASSERT("after the opposite-half low-half aligned load has already consumed the repaired boundary image, the following upper-half aligned load should still stay trap-free instead of reviving stale split-state or reading an older high-half image",
        excpt.triggered == false
    );

    TEST_ASSERT("the second load in the reversed dual probe should read back the fourth upper-half aligned store as the latest committed high-half payload",
        loaded_upper_value == aligned_upper_store_value_fourth
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded low half untouched while the fourth upper-half aligned store remains the latest committed high-half payload across the reversed dual load probe",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, and low-load then upper-load dual consumer sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_load_then_upper_load_observes_seeded_low_and_latest_high_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_load_then_low_load_observes_seeded_high_and_latest_low_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x6d, 0x5b, 0x49, 0x37, 0x25, 0x13, 0x01, 0xef,
        0xde, 0xcc, 0xba, 0xa8, 0x96, 0x84, 0x72, 0x60,
    };
    static const uint8_t guard_before_seed[8] = {
        0x46, 0x58, 0x6a, 0x7c, 0x8e, 0xa0, 0xb2, 0xc4,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd5, 0xe7, 0xf9, 0x0b, 0x1d, 0x2f, 0x41, 0x53,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xebfc0d1e2f405162ULL;
    const uint64_t aligned_low_store_value_first = 0x728394a5b6c7d8e9ULL;
    const uint64_t aligned_low_store_value_second = 0xfa0b1c2d3e4f5061ULL;
    const uint64_t aligned_low_store_value_third = 0x8394a5b6c7d8e9faULL;
    const uint64_t aligned_low_store_value_fourth = 0x0b1c2d3e4f506172ULL;
    const uint64_t expected_upper_value =
        ai_micro_mab_aligned_ext_u64_from_bytes(boundary_seed + 8U);
    uint64_t loaded_upper_value = 0ULL;
    uint64_t loaded_low_value = 0ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged repeated low-half producer continuation and reversed dual load consumer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged repeated low-half producer continuation and reversed dual load consumer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the reversed dual load consumer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after one low-half aligned translated store has already completed on the repaired boundary, the second low-half aligned translated store should stay trap-free before the upper-load then low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the repaired boundary, the third low-half aligned translated store should stay trap-free before the upper-load then low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_fourth);
    TEST_ASSERT("after three low-half aligned translated stores have already completed on the repaired boundary, the fourth low-half aligned translated store should stay trap-free and become the latest committed low-half payload before the reversed dual load consumer probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    loaded_upper_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_upper_load_vaddr);
    TEST_ASSERT("after four low-half aligned translated stores have already completed on the same half, the first upper-half aligned load should still stay trap-free instead of reviving stale split-state or corrupting the seeded high half",
        excpt.triggered == false
    );

    TEST_ASSERT("the first upper-half aligned load in the reversed dual probe should still read back the original seeded high half before the later low-half load observes the latest low-half payload",
        loaded_upper_value == expected_upper_value
    );

    TEST_SETUP_EXCEPT();
    loaded_low_value = ai_micro_mab_aligned_ext_translated_ld(
        aligned_low_load_vaddr);
    TEST_ASSERT("after the opposite-half upper-half aligned load has already consumed the repaired boundary image, the following low-half aligned load should still stay trap-free instead of reviving stale split-state or reading an older low-half image",
        excpt.triggered == false
    );

    TEST_ASSERT("the second load in the reversed dual probe should read back the fourth low-half aligned store as the latest committed low-half payload",
        loaded_low_value == aligned_low_store_value_fourth
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_fourth, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded high half untouched while the fourth low-half aligned store remains the latest committed low-half payload across the reversed dual load probe",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys, expected_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four low-half aligned stores, and upper-load then low-load dual consumer sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_load_then_low_load_observes_seeded_high_and_latest_low_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_reports_precise_fault_and_preserves_latest_high_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x71, 0x5f, 0x4d, 0x3b, 0x29, 0x17, 0x05, 0xf3,
        0xe2, 0xd0, 0xbe, 0xac, 0x9a, 0x88, 0x76, 0x64,
    };
    static const uint8_t guard_before_seed[8] = {
        0x14, 0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x80, 0x92,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa3, 0xb5, 0xc7, 0xd9, 0xeb, 0xfd, 0x0f, 0x21,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_EXT_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x415263748596a7b8ULL;
    const uint64_t refault_store_value = 0xc9daebfc0d1e2f30ULL;
    const uint64_t aligned_upper_store_value_first = 0x1021324354657687ULL;
    const uint64_t aligned_upper_store_value_second = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_third = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xa8b9cadbecfd0e1fULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_ext_reset_env();
    ai_micro_mab_aligned_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);
    ai_micro_mab_aligned_ext_prepare_supervisor_translation();
    ai_micro_mab_aligned_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer continuation and refault refresh probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer continuation and refault refresh probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault refresh probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault refresh probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault refresh probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault refresh probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_third, sizeof(uint64_t));
    ai_micro_mab_aligned_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));

    ai_micro_mab_aligned_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata instead of surfacing stale success-path state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_ext_restore_pmp_full_access();

    TEST_ASSERT("the refault refresh probe should not clobber the committed upper-half boundary bytes written by the fourth upper-half aligned store",
        ai_micro_mab_aligned_ext_bytes_match(boundary_phys + 8U,
            expected_boundary + 8U, 8U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, and refault refresh sequence",
        ai_micro_mab_aligned_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_aligned_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_aligned_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_reports_precise_fault_and_preserves_latest_high_half_corner");
}
