#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep aligned-success follow-up probes separate from the larger MemBlock
 * MAB files to reduce merge pressure.
 */

#define AI_MICRO_MAB_ALIGNED_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET 15U
#define AI_MICRO_MAB_ALIGNED_LOW_FAULT_BYTE_INDEX \
    (AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET - \
     AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET)

static inline pte_t *ai_micro_mab_aligned_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_aligned_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_aligned_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_aligned_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_aligned_set_leaf_paddr(page, phys_page_base(page), flags);
}

static inline void ai_micro_mab_aligned_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_aligned_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_ALIGNED_PMP_TOR_RWX);
}

static inline void ai_micro_mab_aligned_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
}

static inline void ai_micro_mab_aligned_clear_translation_env(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_aligned_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_ALIGNED_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_MAB_ALIGNED_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_aligned_enable_mprv_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
    CSRS(CSR_MSTATUS, MSTATUS_MPP);
}

static inline void ai_micro_mab_aligned_reset_env(void)
{
    ai_micro_mab_aligned_restore_pmp_full_access();
    ai_micro_mab_aligned_clear_machine_access_mode();
    ai_micro_mab_aligned_clear_translation_env();
}

static inline void ai_micro_mab_aligned_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_aligned_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_aligned_overlay_little_endian_bytes(
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

static inline uint64_t ai_micro_mab_aligned_u64_from_bytes(const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }
    return value;
}

static inline void ai_micro_mab_aligned_overlay_low_fault_tail_byte(
    uint8_t *bytes, uint64_t fault_store_value)
{
    bytes[AI_MICRO_MAB_ALIGNED_LOW_FAULT_BYTE_INDEX] =
        (uint8_t)(fault_store_value & 0xffU);
}

#define AI_MICRO_MAB_ALIGNED_TRANSLATED_STORE_INSTRUCTION(name, inst, type) \
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

AI_MICRO_MAB_ALIGNED_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_aligned_translated_sd, "sd", uint64_t)

#define AI_MICRO_MAB_ALIGNED_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
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

AI_MICRO_MAB_ALIGNED_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_aligned_translated_ld, "ld")

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x71, 0x63, 0x55, 0x47, 0x39, 0x2b, 0x1d, 0x0f,
        0xf0, 0xe2, 0xd4, 0xc6, 0xb8, 0xaa, 0x9c, 0x8e,
    };
    static const uint8_t guard_before_seed[8] = {
        0x14, 0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf4, 0xe6, 0xd8, 0xca, 0xbc, 0xae, 0x90, 0x72,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t aligned_store_value_first = 0x1021324354657687ULL;
    const uint64_t aligned_store_value_second = 0x8899aabbccddeeffULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the aligned recovery store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the aligned recovery store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_store_vaddr,
        aligned_store_value_first);
    TEST_ASSERT("after repair, the next translated aligned store on the same boundary should complete without keeping stale exception metadata from the earlier MAB fault episode",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_store_vaddr,
        aligned_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one aligned translated store, the next same-boundary aligned translated store should also stay trap-free instead of inheriting stale MAB metadata",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U, aligned_store_value_second,
        sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should match the latest aligned store image without leaving stale split-template bytes in the upper half",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault and aligned recovery store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_same_boundary_aligned_load_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x52, 0x64, 0x76, 0x88, 0x9a, 0xac, 0xbe, 0xd0,
        0xe1, 0xf3, 0x05, 0x17, 0x29, 0x3b, 0x4d, 0x5f,
    };
    static const uint8_t guard_before_seed[8] = {
        0x13, 0x25, 0x37, 0x49, 0x5b, 0x6d, 0x7f, 0x91,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfe, 0xec, 0xda, 0xc8, 0xb6, 0xa4, 0x92, 0x80,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_boundary_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xd8c9baab9c8d7e6fULL;
    const uint64_t aligned_store_value = 0x1122334455667788ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_probe_value;
    uint64_t probe_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the aligned store-then-load recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the aligned store-then-load recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_boundary_vaddr,
        aligned_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the aligned load probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U, aligned_store_value,
        sizeof(uint64_t));
    expected_probe_value = ai_micro_mab_aligned_u64_from_bytes(expected_boundary);

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_value = ld(aligned_boundary_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store has already been followed by one aligned translated store, the next same-boundary aligned translated load should stay trap-free and return the latest aligned store payload instead of inheriting stale MAB metadata",
        excpt.triggered == false &&
        probe_value == expected_probe_value
    );

    TEST_ASSERT("the repaired same-page boundary bytes should match the aligned store image before the following aligned load retires",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, aligned recovery store, and aligned load sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_same_boundary_aligned_load_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_upper_half_aligned_load_preserves_seeded_high_half_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x41, 0x53, 0x65, 0x77, 0x89, 0x9b, 0xad, 0xbf,
        0xce, 0xdc, 0xea, 0xf8, 0x16, 0x24, 0x32, 0x40,
    };
    static const uint8_t guard_before_seed[8] = {
        0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x90,
    };
    static const uint8_t guard_after_seed[8] = {
        0xff, 0xed, 0xdb, 0xc9, 0xb7, 0xa5, 0x93, 0x81,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x90a1b2c3d4e5f607ULL;
    const uint64_t aligned_store_value = 0x5566778899aabbccULL;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_value;
    uint64_t probe_upper_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the upper-half aligned load preservation probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the upper-half aligned load preservation probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_store_vaddr, aligned_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the untouched-upper-half load probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U, aligned_store_value,
        sizeof(uint64_t));
    expected_upper_value = ai_micro_mab_aligned_u64_from_bytes(
        expected_boundary + sizeof(uint64_t));

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_upper_value = ld(aligned_upper_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the next upper-half aligned translated load should stay trap-free and still return the seeded high-half image instead of inheriting stale split-state",
        excpt.triggered == false &&
        probe_upper_value == expected_upper_value
    );

    TEST_ASSERT("the repaired same-page boundary bytes should keep the updated low half while preserving the original seeded high half before the upper-half aligned load retires",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, aligned low-half store, and upper-half aligned load sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_upper_half_aligned_load_preserves_seeded_high_half_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_upper_half_aligned_store_preserves_low_half_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x62, 0x74, 0x86, 0x98, 0xaa, 0xbc, 0xce, 0xe0,
        0xf1, 0x03, 0x15, 0x27, 0x39, 0x4b, 0x5d, 0x6f,
    };
    static const uint8_t guard_before_seed[8] = {
        0x11, 0x23, 0x35, 0x47, 0x59, 0x6b, 0x7d, 0x8f,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfa, 0xe8, 0xd6, 0xc4, 0xb2, 0xa0, 0x8e, 0x7c,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xa9b8c7d6e5f40312ULL;
    const uint64_t aligned_low_store_value = 0x1029384756abcedfULL;
    const uint64_t aligned_upper_store_value = 0x8899aabbccddeeffULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the upper-half aligned store preservation probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the upper-half aligned store preservation probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated aligned low-half store should complete without keeping stale exception metadata from the earlier MAB fault episode before the upper-half store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the next upper-half aligned translated store should also stay trap-free and preserve the previously committed low half instead of inheriting stale split-state",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U, aligned_low_store_value,
        sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U, aligned_upper_store_value,
        sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the low-half aligned store image while the following upper-half aligned store updates only the seeded high half",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, aligned low-half store, and upper-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_upper_half_aligned_store_preserves_low_half_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_low_half_aligned_load_preserves_seeded_low_half_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x73, 0x85, 0x97, 0xa9, 0xbb, 0xcd, 0xdf, 0xf1,
        0x10, 0x22, 0x34, 0x46, 0x58, 0x6a, 0x7c, 0x8e,
    };
    static const uint8_t guard_before_seed[8] = {
        0x21, 0x33, 0x45, 0x57, 0x69, 0x7b, 0x8d, 0x9f,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf9, 0xe7, 0xd5, 0xc3, 0xb1, 0x9f, 0x8d, 0x7b,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xbac9d8e7f6051423ULL;
    const uint64_t aligned_upper_store_value = 0x31425364758697a8ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_low_value;
    uint64_t probe_low_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the low-half aligned load probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U, aligned_upper_store_value,
        sizeof(uint64_t));
    expected_low_value = ai_micro_mab_aligned_u64_from_bytes(expected_boundary);

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_low_value = ld(aligned_low_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next low-half aligned translated load should stay trap-free and still return the seeded low-half image instead of inheriting stale split-state",
        excpt.triggered == false &&
        probe_low_value == expected_low_value
    );

    TEST_ASSERT("the repaired same-page boundary bytes should keep the original seeded low half while the direct upper-half aligned store updates only the high half",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, and low-half aligned load sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_low_half_aligned_load_preserves_seeded_low_half_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_low_half_aligned_store_preserves_high_half_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x84, 0x96, 0xa8, 0xba, 0xcc, 0xde, 0xf0, 0x12,
        0x25, 0x37, 0x49, 0x5b, 0x6d, 0x7f, 0x91, 0xa3,
    };
    static const uint8_t guard_before_seed[8] = {
        0x31, 0x43, 0x55, 0x67, 0x79, 0x8b, 0x9d, 0xaf,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfc, 0xea, 0xd8, 0xc6, 0xb4, 0xa2, 0x90, 0x7e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xc8d9eafb0c1d2e3fULL;
    const uint64_t aligned_upper_store_value = 0x5162738495a6b7c8ULL;
    const uint64_t aligned_low_store_value = 0xe1d2c3b4a5968778ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half aligned then low-half aligned store preservation probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half aligned then low-half aligned store preservation probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the low-half aligned store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next low-half aligned translated store should also stay trap-free and preserve the previously committed high half instead of inheriting stale split-state",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U, aligned_upper_store_value,
        sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_low_fault_tail_byte(expected_boundary,
        fault_store_value);
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U, aligned_low_store_value,
        sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the direct upper-half aligned store image while the following low-half aligned store updates only the seeded low half",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, and low-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_low_half_aligned_store_preserves_high_half_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_upper_half_aligned_load_reads_committed_high_half_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x95, 0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13,
        0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x80, 0x92, 0xa4,
    };
    static const uint8_t guard_before_seed[8] = {
        0x41, 0x53, 0x65, 0x77, 0x89, 0x9b, 0xad, 0xbf,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfd, 0xeb, 0xd9, 0xc7, 0xb5, 0xa3, 0x91, 0x7f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xd7e8f90a1b2c3d4eULL;
    const uint64_t aligned_upper_store_value = 0x62738495a6b7c8d9ULL;
    uint8_t expected_boundary[16];
    uint64_t probe_upper_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half aligned store then upper-half aligned load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half aligned store then upper-half aligned load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the upper-half aligned load probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U, aligned_upper_store_value,
        sizeof(uint64_t));

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_upper_value = ld(aligned_upper_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next upper-half aligned translated load should stay trap-free and return the committed high-half image instead of inheriting stale split-state",
        excpt.triggered == false &&
        probe_upper_value == aligned_upper_store_value
    );

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded low half while the direct upper-half aligned store updates the high half that the following upper-half load reads back",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, and upper-half aligned load sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_upper_half_aligned_load_reads_committed_high_half_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_repeat_upper_half_aligned_store_preserves_seeded_low_half_no_stale_exception_after_mab_path_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa6, 0xb8, 0xca, 0xdc, 0xee, 0x00, 0x12, 0x24,
        0x37, 0x49, 0x5b, 0x6d, 0x7f, 0x91, 0xa3, 0xb5,
    };
    static const uint8_t guard_before_seed[8] = {
        0x51, 0x63, 0x75, 0x87, 0x99, 0xab, 0xbd, 0xcf,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfe, 0xec, 0xda, 0xc8, 0xb6, 0xa4, 0x92, 0x80,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xe6f708192a3b4c5dULL;
    const uint64_t aligned_upper_store_value_first = 0x738495a6b7c8d9eaULL;
    const uint64_t aligned_upper_store_value_second = 0x1021324354657687ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half aligned then repeated upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half aligned then repeated upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the repeated upper-half store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next repeated upper-half aligned translated store should also stay trap-free and overwrite only the committed high half instead of inheriting stale split-state",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U, aligned_upper_store_value_second,
        sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded low half while the repeated upper-half aligned store updates the high half to the second payload",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, and repeated upper-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_repeat_upper_half_aligned_store_preserves_seeded_low_half_no_stale_exception_after_mab_path_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_load_upper_load_consistent_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xb7, 0xc9, 0xdb, 0xed, 0xff, 0x11, 0x23, 0x35,
        0x48, 0x5a, 0x6c, 0x7e, 0x90, 0xa2, 0xb4, 0xc6,
    };
    static const uint8_t guard_before_seed[8] = {
        0x61, 0x73, 0x85, 0x97, 0xa9, 0xbb, 0xcd, 0xdf,
    };
    static const uint8_t guard_after_seed[8] = {
        0xef, 0xdd, 0xcb, 0xb9, 0xa7, 0x95, 0x83, 0x71,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xf708192a3b4c5d6eULL;
    const uint64_t aligned_upper_store_value = 0x8495a6b7c8d9eafbULL;
    uint8_t expected_boundary[16];
    uint64_t expected_low_value;
    uint64_t probe_low_value;
    uint64_t probe_upper_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H page-tail cross-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half aligned store then low-half and upper-half aligned load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half aligned store then low-half and upper-half aligned load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the dual aligned load probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U, aligned_upper_store_value,
        sizeof(uint64_t));
    expected_low_value = ai_micro_mab_aligned_u64_from_bytes(expected_boundary);

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_low_value = ld(aligned_low_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next low-half aligned translated load should stay trap-free and return the seeded low-half image because the earlier faulting split store has no visible low-byte side effect",
        excpt.triggered == false &&
        probe_low_value == expected_low_value
    );

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_upper_value = ld(aligned_upper_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the same low-half aligned load has already confirmed the seeded low-half image, the following upper-half aligned translated load should still stay trap-free instead of resurfacing stale split-state",
        excpt.triggered == false
    );
    TEST_ASSERT("after the prior low-half aligned load has already observed the seeded low-half image, the following upper-half aligned translated load should still return the committed high-half image",
        probe_upper_value == aligned_upper_store_value
    );

    TEST_ASSERT("the repaired page-tail boundary bytes should keep the seeded low half with no visible byte from the earlier faulting split store while the direct upper-half aligned store updates the committed high half",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, low-half aligned load, and upper-half aligned load sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_load_upper_load_consistent_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_upper_load_low_load_consistent_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xc8, 0xda, 0xec, 0xfe, 0x10, 0x22, 0x34, 0x46,
        0x59, 0x6b, 0x7d, 0x8f, 0xa1, 0xb3, 0xc5, 0xd7,
    };
    static const uint8_t guard_before_seed[8] = {
        0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb, 0xdd, 0xef,
    };
    static const uint8_t guard_after_seed[8] = {
        0xdf, 0xcd, 0xbb, 0xa9, 0x97, 0x85, 0x73, 0x61,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x08192a3b4c5d6e7fULL;
    const uint64_t aligned_upper_store_value = 0x95a6b7c8d9eafb0cULL;
    uint8_t expected_boundary[16];
    uint64_t expected_low_value;
    uint64_t probe_upper_value;
    uint64_t probe_low_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H page-tail cross-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half aligned store then upper-half and low-half aligned load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half aligned store then upper-half and low-half aligned load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the reversed dual aligned load probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U, aligned_upper_store_value,
        sizeof(uint64_t));
    expected_low_value = ai_micro_mab_aligned_u64_from_bytes(expected_boundary);

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_upper_value = ld(aligned_upper_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next upper-half aligned translated load should stay trap-free and return the committed high-half image instead of resurfacing stale split-state",
        excpt.triggered == false &&
        probe_upper_value == aligned_upper_store_value
    );

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_low_value = ld(aligned_low_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the same upper-half aligned load has already confirmed the committed high half, the following low-half aligned translated load should still stay trap-free instead of reading an inconsistent boundary image",
        excpt.triggered == false
    );
    TEST_ASSERT("after the prior upper-half aligned load has already observed the committed high half, the following low-half aligned translated load should return the seeded low-half image because the earlier faulting split store has no visible low-byte side effect",
        probe_low_value == expected_low_value
    );

    TEST_ASSERT("the repaired page-tail boundary bytes should keep the seeded low half with no visible byte from the earlier faulting split store while the direct upper-half aligned store updates the committed high half",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, upper-half aligned load, and low-half aligned load sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_upper_load_low_load_consistent_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_upper_load_low_store_preserves_high_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xd1, 0xc3, 0xb5, 0xa7, 0x99, 0x8b, 0x7d, 0x6f,
        0x52, 0x64, 0x76, 0x88, 0x9a, 0xac, 0xbe, 0xd0,
    };
    static const uint8_t guard_before_seed[8] = {
        0x13, 0x25, 0x37, 0x49, 0x5b, 0x6d, 0x7f, 0x81,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf1, 0xe3, 0xd5, 0xc7, 0xb9, 0xab, 0x9d, 0x8f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x192a3b4c5d6e7f80ULL;
    const uint64_t aligned_upper_store_value = 0x4a5b6c7d8e9fb0c1ULL;
    const uint64_t aligned_low_store_value = 0xd2e3f40516273849ULL;
    uint8_t expected_boundary[16];
    uint64_t probe_upper_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half aligned store then upper-half load and low-half store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half aligned store then upper-half load and low-half store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the upper-half load then low-half store probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_upper_value = ld(aligned_upper_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next upper-half aligned translated load should stay trap-free and return the committed high-half image before the low-half recovery store",
        excpt.triggered == false &&
        probe_upper_value == aligned_upper_store_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after the same upper-half aligned load has already confirmed the committed high half, the following low-half aligned translated store should still stay trap-free instead of reviving stale split-state or fake-crosspage ownership",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U, aligned_upper_store_value,
        sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_low_fault_tail_byte(expected_boundary,
        fault_store_value);
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U, aligned_low_store_value,
        sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the committed high half from the direct upper-half aligned store while the later low-half aligned store updates only the low half",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, upper-half aligned load, and low-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_upper_load_low_store_preserves_high_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_upper_load_preserves_high_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa6, 0x98, 0x8a, 0x7c, 0x6e, 0x50, 0x42, 0x34,
        0x27, 0x39, 0x4b, 0x5d, 0x6f, 0x81, 0x93, 0xa5,
    };
    static const uint8_t guard_before_seed[8] = {
        0x11, 0x23, 0x35, 0x47, 0x59, 0x6b, 0x7d, 0x8f,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfe, 0xec, 0xda, 0xc8, 0xb6, 0xa4, 0x92, 0x80,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x2b3c4d5e6f708192ULL;
    const uint64_t aligned_upper_store_value = 0x60718293a4b5c6d7ULL;
    const uint64_t aligned_low_store_value = 0xf0e1d2c3b4a59687ULL;
    uint8_t expected_boundary[16];
    uint64_t probe_upper_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half aligned store then low-half store and upper-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half aligned store then low-half store and upper-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the low-half store then upper-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next low-half aligned translated store should also stay trap-free before the final upper-half load recheck",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_upper_value = ld(aligned_upper_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the low-half aligned translated store has already completed on the opposite half, the following upper-half aligned translated load should still stay trap-free and return the originally committed high-half image",
        excpt.triggered == false &&
        probe_upper_value == aligned_upper_store_value
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U, aligned_upper_store_value,
        sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_low_fault_tail_byte(expected_boundary,
        fault_store_value);
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U, aligned_low_store_value,
        sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the committed high half from the first upper-half aligned store while the later low-half aligned store updates only the low half that precedes the final upper-half load",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, low-half aligned store, and final upper-half aligned load sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_upper_load_preserves_high_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_load_low_store_preserves_high_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x93, 0x85, 0x77, 0x69, 0x5b, 0x4d, 0x3f, 0x21,
        0x14, 0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82,
    };
    static const uint8_t guard_before_seed[8] = {
        0x20, 0x32, 0x44, 0x56, 0x68, 0x7a, 0x8c, 0x9e,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfd, 0xeb, 0xd9, 0xc7, 0xb5, 0xa3, 0x91, 0x7f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x3c4d5e6f708192a3ULL;
    const uint64_t aligned_upper_store_value = 0x718293a4b5c6d7e8ULL;
    const uint64_t aligned_low_store_value = 0xe0d1c2b3a4958677ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_low_value;
    uint64_t probe_low_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H page-tail cross-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half aligned store then low-half load and low-half store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half aligned store then low-half load and low-half store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the low-half load then low-half store probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U, aligned_upper_store_value,
        sizeof(uint64_t));
    expected_low_value = ai_micro_mab_aligned_u64_from_bytes(expected_boundary);

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_low_value = ld(aligned_low_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next low-half aligned translated load should stay trap-free and return the seeded low-half image before the low-half store update because the earlier faulting split store has no visible low-byte side effect",
        excpt.triggered == false &&
        probe_low_value == expected_low_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after the same low-half aligned load has already confirmed the seeded low-half image, the following low-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the committed high half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U, aligned_low_store_value,
        sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the committed high half from the direct upper-half aligned store while the later low-half aligned store updates only the low half after the confirming low-half load",
        ai_micro_mab_aligned_bytes_match(boundary_phys,
            expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, low-half aligned load, and low-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_load_low_store_preserves_high_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_upper_store_preserves_low_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x84, 0x76, 0x68, 0x5a, 0x4c, 0x3e, 0x20, 0x12,
        0x05, 0x17, 0x29, 0x3b, 0x4d, 0x5f, 0x71, 0x83,
    };
    static const uint8_t guard_before_seed[8] = {
        0x31, 0x43, 0x55, 0x67, 0x79, 0x8b, 0x9d, 0xaf,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfa, 0xe8, 0xd6, 0xc4, 0xb2, 0xa0, 0x8e, 0x7c,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x4d5e6f708192a3b4ULL;
    const uint64_t aligned_upper_store_value_first = 0x8394a5b6c7d8e9faULL;
    const uint64_t aligned_low_store_value = 0x162738495a6b7c8dULL;
    const uint64_t aligned_upper_store_value_second = 0xa5b6c7d8e9fa0b1cULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half then low-half then upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half then low-half then upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the opposite-half store return probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next low-half aligned translated store should still stay trap-free and preserve the committed high half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the opposite-half low-half aligned translated store has already completed, the final upper-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the committed low half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U, aligned_low_store_value,
        sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the committed low half from the middle low-half aligned store while the final upper-half aligned store updates only the high half on the return path",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, low-half aligned store, and final upper-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_upper_store_preserves_low_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_repeat_low_store_preserves_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x95, 0x87, 0x79, 0x6b, 0x5d, 0x4f, 0x31, 0x23,
        0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84,
    };
    static const uint8_t guard_before_seed[8] = {
        0x42, 0x54, 0x66, 0x78, 0x8a, 0x9c, 0xae, 0xc0,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf3, 0xe1, 0xcf, 0xbd, 0xab, 0x99, 0x87, 0x75,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x5e6f708192a3b4c5ULL;
    const uint64_t aligned_upper_store_value = 0x94a5b6c7d8e9fa0bULL;
    const uint64_t aligned_low_store_value_first = 0x2738495a6b7c8d9eULL;
    const uint64_t aligned_low_store_value_second = 0xb2c3d4e5f6071829ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half then low-half then repeated low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half then low-half then repeated low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the low-half repeated-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the next low-half aligned translated store should still stay trap-free and preserve the committed high half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the first low-half aligned translated store has already completed on the opposite half, the repeated low-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the committed high half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the committed high half from the initial upper-half aligned store while the repeated low-half aligned store updates only the low half to its latest payload",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, low-half aligned store, and repeated low-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_repeat_low_store_preserves_high_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa6, 0x98, 0x8a, 0x7c, 0x6e, 0x50, 0x42, 0x34,
        0x27, 0x39, 0x4b, 0x5d, 0x6f, 0x81, 0x93, 0xa5,
    };
    static const uint8_t guard_before_seed[8] = {
        0x51, 0x63, 0x75, 0x87, 0x99, 0xab, 0xbd, 0xcf,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfc, 0xea, 0xd8, 0xc6, 0xb4, 0xa2, 0x90, 0x7e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x6f708192a3b4c5d6ULL;
    const uint64_t aligned_upper_store_value_first = 0xa5b6c7d8e9fa0b1cULL;
    const uint64_t aligned_upper_store_value_second = 0xd4e5f60718293a4bULL;
    const uint64_t aligned_low_store_value = 0x38495a6b7c8d9eafULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct upper-half, repeated upper-half, and final low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct upper-half, repeated upper-half, and final low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the repeated upper-half then low-half store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the repeated upper-half aligned translated store should still stay trap-free and update the committed high half to its latest payload",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after the repeated upper-half aligned translated store has already completed on the same half, the following low-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the latest committed high half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the latest committed high half from the repeated upper-half aligned store while the final low-half aligned store updates only the low half on the opposite half",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, repeated upper-half aligned store, and final low-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xb7, 0xc9, 0xdb, 0xed, 0xff, 0x11, 0x23, 0x35,
        0x48, 0x5a, 0x6c, 0x7e, 0x90, 0xa2, 0xb4, 0xc6,
    };
    static const uint8_t guard_before_seed[8] = {
        0x62, 0x74, 0x86, 0x98, 0xaa, 0xbc, 0xce, 0xe0,
    };
    static const uint8_t guard_after_seed[8] = {
        0xef, 0xdd, 0xcb, 0xb9, 0xa7, 0x95, 0x83, 0x71,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x708192a3b4c5d6e7ULL;
    const uint64_t aligned_low_store_value_first = 0x495a6b7c8d9eafc0ULL;
    const uint64_t aligned_low_store_value_second = 0xc8d9eafb0c1d2e3fULL;
    const uint64_t aligned_upper_store_value = 0x5b6c7d8e9fb0c1d2ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct low-half, repeated low-half, and final upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct low-half, repeated low-half, and final upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the repeated low-half then upper-half store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the repeated low-half aligned translated store should still stay trap-free and update the committed low half to its latest payload",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after the repeated low-half aligned translated store has already completed on the same half, the following upper-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the latest committed low half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the latest committed low half from the repeated low-half aligned store while the final upper-half aligned store updates only the high half on the opposite half",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct low-half aligned store, repeated low-half aligned store, and final upper-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_upper_store_repeat_upper_store_preserves_latest_low_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xc8, 0xda, 0xec, 0xfe, 0x10, 0x22, 0x34, 0x46,
        0x59, 0x6b, 0x7d, 0x8f, 0xa1, 0xb3, 0xc5, 0xd7,
    };
    static const uint8_t guard_before_seed[8] = {
        0x73, 0x85, 0x97, 0xa9, 0xbb, 0xcd, 0xdf, 0xf1,
    };
    static const uint8_t guard_after_seed[8] = {
        0xde, 0xcc, 0xba, 0xa8, 0x96, 0x84, 0x72, 0x60,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x8192a3b4c5d6e7f8ULL;
    const uint64_t aligned_low_store_value = 0x5a6b7c8d9eafc0d1ULL;
    const uint64_t aligned_upper_store_value_first = 0x6c7d8e9fb0c1d2e3ULL;
    const uint64_t aligned_upper_store_value_second = 0xd5e6f708192a3b4cULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct low-half, upper-half, and repeated upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct low-half, upper-half, and repeated upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the upper-half repeated-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the next upper-half aligned translated store should still stay trap-free and preserve the committed low half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the first upper-half aligned translated store has already completed on the opposite half, the repeated upper-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the latest committed low half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the latest committed low half from the initial low-half aligned store while the repeated upper-half aligned store updates only the high half to its latest payload",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct low-half aligned store, upper-half aligned store, and repeated upper-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_upper_store_repeat_upper_store_preserves_latest_low_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_upper_store_low_store_preserves_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xd9, 0xeb, 0xfd, 0x0f, 0x21, 0x33, 0x45, 0x57,
        0x6a, 0x7c, 0x8e, 0xa0, 0xb2, 0xc4, 0xd6, 0xe8,
    };
    static const uint8_t guard_before_seed[8] = {
        0x84, 0x96, 0xa8, 0xba, 0xcc, 0xde, 0xf0, 0x12,
    };
    static const uint8_t guard_after_seed[8] = {
        0xcd, 0xbb, 0xa9, 0x97, 0x85, 0x73, 0x61, 0x4f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0x92a3b4c5d6e7f809ULL;
    const uint64_t aligned_low_store_value_first = 0x6b7c8d9eafc0d1e2ULL;
    const uint64_t aligned_upper_store_value = 0x7d8e9fb0c1d2e3f4ULL;
    const uint64_t aligned_low_store_value_second = 0xe6f708192a3b4c5dULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct low-half, upper-half, and returned low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct low-half, upper-half, and returned low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the opposite-half then returned low-half store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the next upper-half aligned translated store should still stay trap-free and preserve the committed low half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the opposite-half upper-half aligned translated store has already completed, the returned low-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the committed high half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the committed high half from the middle upper-half aligned store while the returned low-half aligned store updates only the low half to its latest payload",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct low-half aligned store, upper-half aligned store, and returned low-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_upper_store_low_store_preserves_high_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_low_load_upper_load_consistent_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xea, 0xfc, 0x0e, 0x20, 0x32, 0x44, 0x56, 0x68,
        0x7b, 0x8d, 0x9f, 0xb1, 0xc3, 0xd5, 0xe7, 0xf9,
    };
    static const uint8_t guard_before_seed[8] = {
        0x95, 0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x11, 0x23,
    };
    static const uint8_t guard_after_seed[8] = {
        0xbc, 0xaa, 0x98, 0x86, 0x74, 0x62, 0x50, 0x3e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_load_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xa3b4c5d6e7f8091aULL;
    const uint64_t aligned_low_store_value = 0x7c8d9eafc0d1e2f3ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_low_value;
    uint64_t probe_low_value;
    uint64_t expected_upper_value;
    uint64_t probe_upper_value;

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct low-half aligned store then low-half and upper-half aligned load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct low-half aligned store then low-half and upper-half aligned load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the same-half then opposite-half aligned load probe",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U, aligned_low_store_value,
        sizeof(uint64_t));
    expected_low_value = ai_micro_mab_aligned_u64_from_bytes(expected_boundary);
    expected_upper_value = ai_micro_mab_aligned_u64_from_bytes(
        expected_boundary + sizeof(uint64_t));

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_low_value = ld(aligned_low_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the next low-half aligned translated load should stay trap-free and return the committed low-half image instead of resurfacing stale split-state",
        excpt.triggered == false &&
        probe_low_value == expected_low_value
    );

    ai_micro_mab_aligned_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    probe_upper_value = ld(aligned_upper_load_vaddr);
    ai_micro_mab_aligned_clear_machine_access_mode();
    TEST_ASSERT("after the same low-half aligned load has already confirmed the committed low half, the following upper-half aligned translated load should still stay trap-free instead of reading an inconsistent boundary image",
        excpt.triggered == false
    );
    TEST_ASSERT("after the prior low-half aligned load has already observed the committed low half, the following upper-half aligned translated load should still return the seeded high-half image",
        probe_upper_value == expected_upper_value
    );

    TEST_ASSERT("the repaired same-page boundary bytes should keep the committed low half from the direct low-half aligned store while the high half remains seeded for the same-half then opposite-half load pair",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct low-half aligned store, low-half aligned load, and upper-half aligned load sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_low_load_upper_load_consistent_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xfb, 0x0d, 0x1f, 0x31, 0x43, 0x55, 0x67, 0x79,
        0x8c, 0x9e, 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a,
    };
    static const uint8_t guard_before_seed[8] = {
        0xa6, 0xb8, 0xca, 0xdc, 0xee, 0x10, 0x22, 0x34,
    };
    static const uint8_t guard_after_seed[8] = {
        0xab, 0x99, 0x87, 0x75, 0x63, 0x51, 0x3f, 0x2d,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xb4c5d6e7f8091a2bULL;
    const uint64_t aligned_low_store_value_first = 0x8d9eafc0d1e2f304ULL;
    const uint64_t aligned_low_store_value_second = 0xf60718293a4b5c6dULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct and repeated low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct and repeated low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the repeated same-half store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the repeated low-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the seeded high half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded high half untouched while the repeated low-half aligned store updates only the low half to its latest payload",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct low-half aligned store, and repeated low-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_preserves_seeded_low_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x0c, 0x1e, 0x30, 0x42, 0x54, 0x66, 0x78, 0x8a,
        0x9d, 0xaf, 0xc1, 0xd3, 0xe5, 0xf7, 0x09, 0x1b,
    };
    static const uint8_t guard_before_seed[8] = {
        0xb7, 0xc9, 0xdb, 0xed, 0xff, 0x21, 0x33, 0x45,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9a, 0x88, 0x76, 0x64, 0x52, 0x40, 0x2e, 0x1c,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xc5d6e7f8091a2b3cULL;
    const uint64_t aligned_upper_store_value_first = 0x9eafc0d1e2f30415ULL;
    const uint64_t aligned_upper_store_value_second = 0x0718293a4b5c6d7eULL;
    const uint64_t aligned_upper_store_value_third = 0x5061728394a5b6c7ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct and repeated upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct and repeated upper-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the repeated same-half high-half store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one upper-half aligned translated store, the repeated upper-half aligned translated store should still stay trap-free and preserve the seeded low half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_upper_store_vaddr,
        aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the same half, the third upper-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the seeded low half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        aligned_upper_store_value_third, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded low half untouched while the third upper-half aligned store updates only the high half to its latest payload",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct upper-half aligned store, repeated upper-half aligned store, and third upper-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_preserves_seeded_low_half_no_stale_exception_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x6e, 0x5c, 0x4a, 0x38, 0x26, 0x14, 0x02, 0xf1,
        0xe3, 0xd5, 0xc7, 0xb9, 0xab, 0x9d, 0x8f, 0x71,
    };
    static const uint8_t guard_before_seed[8] = {
        0xc4, 0xb2, 0xa0, 0x8e, 0x7c, 0x6a, 0x58, 0x46,
    };
    static const uint8_t guard_after_seed[8] = {
        0x3d, 0x4f, 0x61, 0x73, 0x85, 0x97, 0xa9, 0xbb,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_store_vaddr =
        page_vaddr + AI_MICRO_MAB_ALIGNED_BOUNDARY_BASE_OFFSET;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t fault_store_value = 0xd4e5f60718293a4bULL;
    const uint64_t aligned_low_store_value_first = 0x31425364758697a8ULL;
    const uint64_t aligned_low_store_value_second = 0xb9cadbecfd0e1f20ULL;
    const uint64_t aligned_low_store_value_third = 0x6a7b8c9daebfd0e1ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_aligned_reset_env();
    ai_micro_mab_aligned_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_aligned_write_bytes(guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_aligned_write_bytes(guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_aligned_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_ALIGNED_PMP_TOR_NO_W);
    ai_micro_mab_aligned_prepare_supervisor_translation();
    ai_micro_mab_aligned_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_aligned_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the direct and repeated low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the direct and repeated low-half aligned store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_aligned_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated low-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the repeated same-half low-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_second);
    TEST_ASSERT("after the repaired misaligned store has already been followed by one low-half aligned translated store, the repeated low-half aligned translated store should still stay trap-free and preserve the seeded high half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_aligned_translated_sd(aligned_low_store_vaddr,
        aligned_low_store_value_third);
    TEST_ASSERT("after two low-half aligned translated stores have already completed on the same half, the third low-half aligned translated store should still stay trap-free instead of reviving stale split-state or corrupting the seeded high half",
        excpt.triggered == false
    );

    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 0U,
        aligned_low_store_value_first, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_second, sizeof(uint64_t));
    ai_micro_mab_aligned_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 0U,
        aligned_low_store_value_third, sizeof(uint64_t));

    TEST_ASSERT("the repaired same-page boundary bytes should keep the seeded high half untouched while the third low-half aligned store updates only the low half to its latest payload",
        ai_micro_mab_aligned_bytes_match(boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, direct low-half aligned store, repeated low-half aligned store, and third low-half aligned store sequence",
        ai_micro_mab_aligned_bytes_match(guard_before_phys, guard_before_seed,
            8U) &&
        ai_micro_mab_aligned_bytes_match(guard_after_phys, guard_after_seed,
            8U)
    );

    ai_micro_mab_aligned_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner");
}
