#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_MAB_FLUSH_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET 12U
#define AI_MICRO_MAB_FLUSH_CROSS_16B_SD_3B5B_OFFSET 13U
#define AI_MICRO_MAB_FLUSH_CROSS_16B_SD_2B6B_OFFSET 14U
#define AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_flush_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_flush_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_flush_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_flush_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_flush_set_leaf_paddr(page, phys_page_base(page), flags);
}

static inline void ai_micro_mab_flush_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_flush_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_FLUSH_PMP_TOR_RWX);
}

static inline void ai_micro_mab_flush_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_flush_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_FLUSH_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_MAB_FLUSH_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_flush_enable_mprv_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
    CSRS(CSR_MSTATUS, MSTATUS_MPP);
}

static inline void ai_micro_mab_flush_reset_env(void)
{
    ai_micro_mab_flush_restore_pmp_full_access();
    ai_micro_mab_flush_clear_machine_access_mode();
}

static inline void ai_micro_mab_flush_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_flush_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_flush_overlay_little_endian_bytes(
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

static inline uint64_t ai_micro_mab_flush_u64_from_bytes(const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }
    return value;
}

#define AI_MICRO_MAB_FLUSH_TRANSLATED_STORE_INSTRUCTION(name, inst, type) \
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

AI_MICRO_MAB_FLUSH_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_flush_translated_sd, "sd", uint64_t)

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_load_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a, 0x39, 0x28,
        0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2, 0xb1, 0xa0,
    };
    static const uint8_t guard_before_seed[8] = {
        0x10, 0x22, 0x34, 0x46, 0x58, 0x6a, 0x7c, 0x8e,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf1, 0xe3, 0xd5, 0xc7, 0xb9, 0xab, 0x9d, 0x8f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t success_store_value = 0x0102030405060708ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise fault metadata before repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, success_store_value);
    TEST_ASSERT("after repair, the same translated cross-16B scalar store should complete without keeping stale exception state",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 7U, success_store_value,
        sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(expected_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store, the next translated aligned load on the same boundary should not observe stale exception metadata",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the latest successful store template",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through fault and repair",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_load_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_different_template_store_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x6e, 0x5d, 0x4c, 0x3b, 0x2a, 0x19, 0x08, 0xf7,
        0xe6, 0xd5, 0xc4, 0xb3, 0xa2, 0x91, 0x80, 0x7f,
    };
    static const uint8_t guard_before_seed[8] = {
        0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf2, 0xe4, 0xd6, 0xc8, 0xba, 0xac, 0x9e, 0x90,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x7f6e5d4c3b2a1908ULL;
    const uint64_t success_store_value = 0x8899aabbccddeeffULL;
    uint8_t expected_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated second-split write fault must still report store access fault on the original 1B+7B template address before the same-boundary different-template success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar store fault keeps the same precise metadata before the same-boundary different-template success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, success_store_value);
    TEST_ASSERT("after repair, switching the same boundary to a different 4B+4B translated store template should complete without keeping stale exception metadata from the earlier MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 4U, success_store_value,
        sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(expected_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store switches to the 4B+4B success template, the following aligned load on the same boundary should still see no stale exception metadata and the current byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the latest same-boundary different-template store overlay after the MAB path",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated fault, repair, and same-boundary different-template success store",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_different_template_store_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_different_template_store_then_original_template_store_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x73, 0x62, 0x51, 0x40, 0x3f, 0x2e, 0x1d, 0x0c,
        0xfb, 0xea, 0xd9, 0xc8, 0xb7, 0xa6, 0x95, 0x84,
    };
    static const uint8_t guard_before_seed[8] = {
        0x11, 0x25, 0x39, 0x4d, 0x61, 0x75, 0x89, 0x9d,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfe, 0xec, 0xda, 0xc8, 0xb6, 0xa4, 0x92, 0x80,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t mid_success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t final_success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x84736251403f2e1dULL;
    const uint64_t mid_success_store_value = 0x1032547698badcfeULL;
    const uint64_t final_success_store_value = 0x89abcdef01234567ULL;
    uint8_t expected_mid_boundary[16];
    uint8_t expected_final_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated second-split write fault must still report store access fault on the original 1B+7B template address before the two-step success-template switch probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar store fault keeps the same precise metadata before the two-step success-template switch probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(mid_success_vaddr, mid_success_store_value);
    TEST_ASSERT("after repair, the first same-boundary 4B+4B translated success store should complete without carrying stale exception metadata from the earlier MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_mid_boundary, boundary_seed, 16U, 4U, mid_success_store_value,
        sizeof(uint64_t));
    TEST_ASSERT("after repair, the intermediate same-boundary 4B+4B success store should only update its current template bytes and keep both guard regions unchanged",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_mid_boundary,
            16U) &&
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(final_success_vaddr,
        final_success_store_value);
    TEST_ASSERT("after the intermediate 4B+4B success store, switching back to the original 1B+7B translated store template should still clear stale exception metadata",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_final_boundary, expected_mid_boundary, 16U, 7U,
        final_success_store_value, sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(
        expected_final_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store switches templates twice, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the final original-template success overlay after the intermediate different-template store",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_final_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated fault, repair, and two-step same-boundary success-template switch",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_different_template_store_then_original_template_store_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_then_reuse_fault_template_store_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x85, 0x74, 0x63, 0x52, 0x41, 0x30, 0x2f, 0x1e,
        0x0d, 0xfc, 0xeb, 0xda, 0xc9, 0xb8, 0xa7, 0x96,
    };
    static const uint8_t guard_before_seed[8] = {
        0x14, 0x28, 0x3c, 0x50, 0x64, 0x78, 0x8c, 0xa0,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf4, 0xe2, 0xd0, 0xbe, 0xac, 0x9a, 0x88, 0x76,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t mid_success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t final_success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x968574635241302fULL;
    const uint64_t mid_success_store_value = 0x76543210fedcba98ULL;
    const uint64_t final_success_store_value = 0x0123456789abcdefULL;
    uint8_t expected_mid_boundary[16];
    uint8_t expected_final_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the original fault-template address before the reuse-fault-template success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 4B+4B store fault keeps the same precise metadata before the reuse-fault-template success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(mid_success_vaddr, mid_success_store_value);
    TEST_ASSERT("after repair, the first same-boundary 1B+7B translated success store should complete without carrying stale exception metadata from the earlier 4B+4B MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_mid_boundary, boundary_seed, 16U, 7U, mid_success_store_value,
        sizeof(uint64_t));
    TEST_ASSERT("after repair, the intermediate same-boundary 1B+7B success store should only update its current template bytes and keep both guard regions unchanged",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_mid_boundary,
            16U) &&
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(final_success_vaddr,
        final_success_store_value);
    TEST_ASSERT("after the intermediate 1B+7B success store, reusing the original 4B+4B fault template as a success store should still clear stale exception metadata",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_final_boundary, expected_mid_boundary, 16U, 4U,
        final_success_store_value, sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(
        expected_final_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store switches templates and then reuses the original fault template as a success store, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the final reused-fault-template success overlay after the intermediate different-template store",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_final_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 4B+4B fault, repair, and template reuse success sequence",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_then_reuse_fault_template_store_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d,
        0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2c, 0x40, 0x54, 0x68, 0x7c, 0x90, 0xa4,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf8, 0xe6, 0xd4, 0xc2, 0xb0, 0x9e, 0x8c, 0x7a,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xa594837261504f3eULL;
    const uint64_t success_store_value = 0xfedcba9876543210ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must report store access fault on the same fault-template address before the same-template success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 4B+4B store fault keeps the same precise metadata before the same-template success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, success_store_value);
    TEST_ASSERT("after repair, retrying the same 4B+4B fault template as a success store should complete without carrying stale exception metadata from the earlier MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 4U, success_store_value,
        sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(expected_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store retries the same 4B+4B template successfully, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the retried same-template success overlay after the earlier 4B+4B fault path",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 4B+4B fault, repair, and same-template success retry",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_template_store_twice_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa6, 0x95, 0x84, 0x73, 0x62, 0x51, 0x40, 0x2f,
        0x1e, 0x0d, 0xfc, 0xeb, 0xda, 0xc9, 0xb8, 0xa7,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2e, 0x42, 0x56, 0x6a, 0x7e, 0x92, 0xa6,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfa, 0xe8, 0xd6, 0xc4, 0xb2, 0xa0, 0x8e, 0x7c,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xb8a7968574635241ULL;
    const uint64_t first_success_store_value = 0x1122334455667788ULL;
    const uint64_t second_success_store_value = 0x8877665544332211ULL;
    uint8_t expected_first_boundary[16];
    uint8_t expected_second_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 1B+7B second-split write fault must report store access fault on the same template address before the double-success retry probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 1B+7B store fault keeps the same precise metadata before the double-success retry probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, first_success_store_value);
    TEST_ASSERT("after repair, the first same-template 1B+7B success store should complete without carrying stale exception metadata from the earlier MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_first_boundary, boundary_seed, 16U, 7U,
        first_success_store_value, sizeof(uint64_t));
    TEST_ASSERT("after repair, the first same-template 1B+7B success store should only update its current template bytes and keep both guard regions unchanged",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_first_boundary,
            16U) &&
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, second_success_store_value);
    TEST_ASSERT("after the first same-template success store, retrying the same 1B+7B template with a new payload should still clear stale exception metadata",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_second_boundary, expected_first_boundary, 16U, 7U,
        second_success_store_value, sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(
        expected_second_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store retries the same 1B+7B template twice successfully, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the second same-template success overlay after the earlier 1B+7B fault path",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_second_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 1B+7B fault, repair, and double same-template success retry",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_template_store_twice_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_twice_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xb7, 0xa6, 0x95, 0x84, 0x73, 0x62, 0x51, 0x40,
        0x2f, 0x1e, 0x0d, 0xfc, 0xeb, 0xda, 0xc9, 0xb8,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1c, 0x30, 0x44, 0x58, 0x6c, 0x80, 0x94, 0xa8,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfc, 0xea, 0xd8, 0xc6, 0xb4, 0xa2, 0x90, 0x7e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xc9b8a79685746352ULL;
    const uint64_t first_success_store_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t second_success_store_value = 0x78695a4b3c2d1e0fULL;
    uint8_t expected_first_boundary[16];
    uint8_t expected_second_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must report store access fault on the same fault-template address before the double-success retry probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 4B+4B store fault keeps the same precise metadata before the double-success retry probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, first_success_store_value);
    TEST_ASSERT("after repair, the first same-fault-template 4B+4B success store should complete without carrying stale exception metadata from the earlier MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_first_boundary, boundary_seed, 16U, 4U,
        first_success_store_value, sizeof(uint64_t));
    TEST_ASSERT("after repair, the first same-fault-template 4B+4B success store should only update its current template bytes and keep both guard regions unchanged",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_first_boundary,
            16U) &&
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, second_success_store_value);
    TEST_ASSERT("after the first same-fault-template success store, retrying the same 4B+4B template with a new payload should still clear stale exception metadata",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_second_boundary, expected_first_boundary, 16U, 4U,
        second_success_store_value, sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(
        expected_second_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store retries the same 4B+4B template twice successfully, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the second same-fault-template success overlay after the earlier 4B+4B fault path",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_second_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 4B+4B fault, repair, and double same-fault-template success retry",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_twice_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_twice_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xc8, 0xb7, 0xa6, 0x95, 0x84, 0x73, 0x62, 0x51,
        0x40, 0x2f, 0x1e, 0x0d, 0xfc, 0xeb, 0xda, 0xc9,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1e, 0x32, 0x46, 0x5a, 0x6e, 0x82, 0x96, 0xaa,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfe, 0xec, 0xda, 0xc8, 0xb6, 0xa4, 0x92, 0x80,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xdac9b8a796857463ULL;
    const uint64_t first_success_store_value = 0x2233445566778899ULL;
    const uint64_t second_success_store_value = 0x9988776655443322ULL;
    uint8_t expected_first_boundary[16];
    uint8_t expected_second_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must report store access fault on the original fault-template address before the different-template double-success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 4B+4B store fault keeps the same precise metadata before the different-template double-success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, first_success_store_value);
    TEST_ASSERT("after repair, the first same-boundary different-template 1B+7B success store should complete without carrying stale exception metadata from the earlier 4B+4B MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_first_boundary, boundary_seed, 16U, 7U,
        first_success_store_value, sizeof(uint64_t));
    TEST_ASSERT("after repair, the first same-boundary different-template 1B+7B success store should only update its current template bytes and keep both guard regions unchanged",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_first_boundary,
            16U) &&
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, second_success_store_value);
    TEST_ASSERT("after the first different-template success store, retrying the same 1B+7B template with a new payload should still clear stale exception metadata",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_second_boundary, expected_first_boundary, 16U, 7U,
        second_success_store_value, sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(
        expected_second_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store retries the same different-template 1B+7B success path twice, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the second different-template success overlay after the earlier 4B+4B fault path",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_second_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 4B+4B fault, repair, and double different-template success retry",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_twice_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_4b4b_store_twice_after_1b7b_fault_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xd9, 0xc8, 0xb7, 0xa6, 0x95, 0x84, 0x73, 0x62,
        0x51, 0x40, 0x2f, 0x1e, 0x0d, 0xfc, 0xeb, 0xda,
    };
    static const uint8_t guard_before_seed[8] = {
        0x20, 0x34, 0x48, 0x5c, 0x70, 0x84, 0x98, 0xac,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf0, 0xde, 0xcc, 0xba, 0xa8, 0x96, 0x84, 0x72,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xebdac9b8a7968574ULL;
    const uint64_t first_success_store_value = 0x31425364758697a8ULL;
    const uint64_t second_success_store_value = 0xa897867564534231ULL;
    uint8_t expected_first_boundary[16];
    uint8_t expected_second_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 1B+7B second-split write fault must report store access fault on the original fault-template address before the 4B+4B double-success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 1B+7B store fault keeps the same precise metadata before the 4B+4B double-success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, first_success_store_value);
    TEST_ASSERT("after repair, the first same-boundary 4B+4B success store should complete without carrying stale exception metadata from the earlier 1B+7B MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_first_boundary, boundary_seed, 16U, 4U,
        first_success_store_value, sizeof(uint64_t));
    TEST_ASSERT("after repair, the first same-boundary 4B+4B success store should only update its current template bytes and keep both guard regions unchanged",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_first_boundary,
            16U) &&
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, second_success_store_value);
    TEST_ASSERT("after the first same-boundary 4B+4B success store, retrying the same 4B+4B template with a new payload should still clear stale exception metadata",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_second_boundary, expected_first_boundary, 16U, 4U,
        second_success_store_value, sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(
        expected_second_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store retries the same 4B+4B success path twice after a 1B+7B fault episode, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the second 4B+4B success overlay after the earlier 1B+7B fault path",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_second_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 1B+7B fault, repair, and double 4B+4B success retry",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_4b4b_store_twice_after_1b7b_fault_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_template_store_then_different_template_store_no_stale_exception_after_1b7b_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xea, 0xd9, 0xc8, 0xb7, 0xa6, 0x95, 0x84, 0x73,
        0x62, 0x51, 0x40, 0x2f, 0x1e, 0x0d, 0xfc, 0xeb,
    };
    static const uint8_t guard_before_seed[8] = {
        0x22, 0x36, 0x4a, 0x5e, 0x72, 0x86, 0x9a, 0xae,
    };
    static const uint8_t guard_after_seed[8] = {
        0xee, 0xdc, 0xca, 0xb8, 0xa6, 0x94, 0x82, 0x70,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t second_success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xfcdbcab9a8978675ULL;
    const uint64_t first_success_store_value = 0x445566778899aabbULL;
    const uint64_t second_success_store_value = 0xbbaa998877665544ULL;
    uint8_t expected_first_boundary[16];
    uint8_t expected_second_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 1B+7B second-split write fault must report store access fault on the same template address before the same-then-different success-template probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 1B+7B store fault keeps the same precise metadata before the same-then-different success-template probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(first_success_vaddr,
        first_success_store_value);
    TEST_ASSERT("after repair, the first same-template 1B+7B success store should complete without carrying stale exception metadata from the earlier MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_first_boundary, boundary_seed, 16U, 7U,
        first_success_store_value, sizeof(uint64_t));
    TEST_ASSERT("after repair, the first same-template 1B+7B success store should only update its current template bytes and keep both guard regions unchanged",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_first_boundary,
            16U) &&
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(second_success_vaddr,
        second_success_store_value);
    TEST_ASSERT("after the first same-template success store, switching to the 4B+4B different template should still clear stale exception metadata",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_second_boundary, expected_first_boundary, 16U, 4U,
        second_success_store_value, sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(
        expected_second_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store first retries the same 1B+7B template and then switches to the 4B+4B template, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the final 4B+4B different-template success overlay after the first same-template success store",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_second_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 1B+7B fault, repair, same-template success, and later different-template success",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_template_store_then_different_template_store_no_stale_exception_after_1b7b_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_then_different_template_store_no_stale_exception_after_4b4b_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xfb, 0xea, 0xd9, 0xc8, 0xb7, 0xa6, 0x95, 0x84,
        0x73, 0x62, 0x51, 0x40, 0x2f, 0x1e, 0x0d, 0xfc,
    };
    static const uint8_t guard_before_seed[8] = {
        0x24, 0x38, 0x4c, 0x60, 0x74, 0x88, 0x9c, 0xb0,
    };
    static const uint8_t guard_after_seed[8] = {
        0xec, 0xda, 0xc8, 0xb6, 0xa4, 0x92, 0x80, 0x6e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t second_success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x0dfcdbcab9a89786ULL;
    const uint64_t first_success_store_value = 0x5566778899aabbccULL;
    const uint64_t second_success_store_value = 0xccbbaa9988776655ULL;
    uint8_t expected_first_boundary[16];
    uint8_t expected_second_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must report store access fault on the same template address before the same-then-different success-template probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 4B+4B store fault keeps the same precise metadata before the same-then-different success-template probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(first_success_vaddr,
        first_success_store_value);
    TEST_ASSERT("after repair, the first same-fault-template 4B+4B success store should complete without carrying stale exception metadata from the earlier MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_first_boundary, boundary_seed, 16U, 4U,
        first_success_store_value, sizeof(uint64_t));
    TEST_ASSERT("after repair, the first same-fault-template 4B+4B success store should only update its current template bytes and keep both guard regions unchanged",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_first_boundary,
            16U) &&
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(second_success_vaddr,
        second_success_store_value);
    TEST_ASSERT("after the first same-fault-template success store, switching to the 1B+7B different template should still clear stale exception metadata",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_second_boundary, expected_first_boundary, 16U, 7U,
        second_success_store_value, sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(
        expected_second_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store first retries the same 4B+4B template and then switches to the 1B+7B template, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the final 1B+7B different-template success overlay after the first same-fault-template success store",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_second_boundary,
            16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 4B+4B fault, repair, same-fault-template success, and later different-template success",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_then_different_template_store_no_stale_exception_after_4b4b_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_no_stale_exception_after_4b4b_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x1b, 0x0a, 0xf9, 0xe8, 0xd7, 0xc6, 0xb5, 0xa4,
        0x93, 0x82, 0x71, 0x60, 0x5f, 0x4e, 0x3d, 0x2c,
    };
    static const uint8_t guard_before_seed[8] = {
        0x26, 0x3a, 0x4e, 0x62, 0x76, 0x8a, 0x9e, 0xb2,
    };
    static const uint8_t guard_after_seed[8] = {
        0xed, 0xdb, 0xc9, 0xb7, 0xa5, 0x93, 0x81, 0x6f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_4B4B_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x2c1b0af9e8d7c6b5ULL;
    const uint64_t success_store_value = 0x66778899aabbccddULL;
    uint8_t expected_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the original 4B+4B template address before the direct different-template success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 4B+4B store fault keeps the same precise metadata before the direct different-template success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, success_store_value);
    TEST_ASSERT("after repair, switching directly from the 4B+4B fault template to the 1B+7B different template should complete without carrying stale exception metadata from the earlier MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 7U, success_store_value,
        sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(expected_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store switches directly from the 4B+4B fault template to the 1B+7B different template, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the direct 1B+7B different-template success overlay after the earlier 4B+4B fault path",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 4B+4B fault, repair, and direct different-template success retry",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_no_stale_exception_after_4b4b_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_3b5b_store_no_stale_exception_after_2b6b_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x2d, 0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6,
        0xa5, 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e,
    };
    static const uint8_t guard_before_seed[8] = {
        0x28, 0x3c, 0x50, 0x64, 0x78, 0x8c, 0xa0, 0xb4,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe8, 0xd6, 0xc4, 0xb2, 0xa0, 0x8e, 0x7c, 0x6a,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_2B6B_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t success_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_CROSS_16B_SD_3B5B_OFFSET;
    const uintptr_t probe_vaddr =
        page_vaddr + AI_MICRO_MAB_FLUSH_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x3e2d1c0bfae9d8c7ULL;
    const uint64_t success_store_value = 0x778899aabbccddeeULL;
    uint8_t expected_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_flush_reset_env();
    ai_micro_mab_flush_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_flush_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_flush_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_flush_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_FLUSH_PMP_TOR_NO_W);
    ai_micro_mab_flush_prepare_supervisor_translation();
    ai_micro_mab_flush_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_flush_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 2B+6B second-split write fault must still report store access fault on the original 2B+6B template address before the 3B+5B success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B translated scalar 2B+6B store fault keeps the same precise metadata before the 3B+5B success probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_flush_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_flush_translated_sd(success_vaddr, success_store_value);
    TEST_ASSERT("after repair, switching directly from the 2B+6B fault template to the 3B+5B different template should complete without carrying stale exception metadata from the earlier MAB fault path",
        excpt.triggered == false
    );

    ai_micro_mab_flush_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 5U, success_store_value,
        sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_flush_u64_from_bytes(expected_boundary);

    ai_micro_mab_flush_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(probe_vaddr);
    ai_micro_mab_flush_clear_machine_access_mode();
    TEST_ASSERT("after the repaired cross-16B store switches directly from the 2B+6B fault template to the 3B+5B different template, the following aligned load on the same boundary should still see no stale exception metadata and the latest byte image",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page cross-16B boundary bytes should match the direct 3B+5B different-template success overlay after the earlier 2B+6B fault path",
        ai_micro_mab_flush_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page cross-16B boundary should remain unchanged through the repeated 2B+6B fault, repair, and direct 3B+5B different-template success retry",
        ai_micro_mab_flush_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_flush_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_flush_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_3b5b_store_no_stale_exception_after_2b6b_mab_path_corner");
}
