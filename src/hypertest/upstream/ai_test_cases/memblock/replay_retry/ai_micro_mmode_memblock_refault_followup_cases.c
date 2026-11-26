#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep repaired-refault follow-up probes in a dedicated file so the refault
 * extension file does not keep growing without bound.
 */

#define AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_refault_followup_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_refault_followup_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_refault_followup_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_refault_followup_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_refault_followup_set_leaf_paddr(page, phys_page_base(page),
        flags);
}

static inline void ai_micro_mab_refault_followup_prepare_supervisor_translation(
    void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_refault_followup_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_RWX);
}

static inline void ai_micro_mab_refault_followup_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_refault_followup_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_RWX | (deny_cfg << 8) |
            (AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_refault_followup_reset_env(void)
{
    ai_micro_mab_refault_followup_restore_pmp_full_access();
    ai_micro_mab_refault_followup_clear_machine_access_mode();
}

static inline void ai_micro_mab_refault_followup_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_refault_followup_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_refault_followup_overlay_little_endian_bytes(
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

static inline uint64_t ai_micro_mab_refault_followup_u64_from_bytes(
    const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }
    return value;
}

#define AI_MICRO_MAB_REFAULT_FOLLOWUP_TRANSLATED_STORE_INSTRUCTION( \
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

AI_MICRO_MAB_REFAULT_FOLLOWUP_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_refault_followup_translated_sd, "sd", uint64_t)

#define AI_MICRO_MAB_REFAULT_FOLLOWUP_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
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

AI_MICRO_MAB_REFAULT_FOLLOWUP_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_refault_followup_translated_lbu, "lbu")
AI_MICRO_MAB_REFAULT_FOLLOWUP_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_refault_followup_translated_lhu, "lhu")
AI_MICRO_MAB_REFAULT_FOLLOWUP_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_refault_followup_translated_lwu, "lwu")
AI_MICRO_MAB_REFAULT_FOLLOWUP_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_refault_followup_translated_ld, "ld")

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_low_load_clears_stale_exception_and_observes_refault_updated_low_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x63, 0x51, 0x3f, 0x2d, 0x1b, 0x09, 0xf7, 0xe5,
        0xd4, 0xc2, 0xb0, 0x9e, 0x8c, 0x7a, 0x68, 0x56,
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
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_low_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x31425364758697a8ULL;
    const uint64_t refault_store_value = 0xc5d6e7f8091a2b3cULL;
    const uint64_t aligned_upper_store_value_first = 0x1021324354657687ULL;
    const uint64_t aligned_upper_store_value_second = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_third = 0x1122334455667788ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x99aabbccddeeff10ULL;
    const uint64_t repaired_upper_store_value = 0x2131415161718191ULL;
    uint64_t observed_low_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, repaired upper-store, and trailing low-load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, repaired upper-store, and trailing low-load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-store-then-low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-store-then-low-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-store and trailing low-load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, repaired_upper_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing low-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_low_value = ai_micro_mab_refault_followup_translated_ld(
        aligned_low_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and repaired upper-half aligned store, a same-boundary low-half aligned translated load should clear stale exception metadata while preserving the seeded low half because the refaulting store has no architectural low-byte side effect",
        excpt.triggered == false &&
        observed_low_value ==
            ai_micro_mab_refault_followup_u64_from_bytes(boundary_seed)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, repaired upper-store, and trailing low-load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_low_load_clears_stale_exception_and_observes_refault_updated_low_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_upper_load_clears_stale_exception_and_reads_latest_high_half_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x74, 0x62, 0x50, 0x3e, 0x2c, 0x1a, 0x08, 0xf6,
        0xe5, 0xd3, 0xc1, 0xaf, 0x9d, 0x8b, 0x79, 0x67,
    };
    static const uint8_t guard_before_seed[8] = {
        0x14, 0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82,
    };
    static const uint8_t guard_after_seed[8] = {
        0x93, 0xa5, 0xb7, 0xc9, 0xdb, 0xed, 0xef, 0x11,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t aligned_upper_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x425364758697a8b9ULL;
    const uint64_t refault_store_value = 0xd4e5f60718293a4bULL;
    const uint64_t aligned_upper_store_value_first = 0x1029384756a1b2c3ULL;
    const uint64_t aligned_upper_store_value_second = 0x64758697a8b9cadbULL;
    const uint64_t aligned_upper_store_value_third = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x89abcdeffedcba98ULL;
    const uint64_t repaired_upper_store_value = 0x2132435465768798ULL;
    uint64_t observed_high_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, repaired upper-store, and trailing upper-load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, repaired upper-store, and trailing upper-load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-store-then-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-store-then-upper-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-store and trailing upper-load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, repaired_upper_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_high_value = ai_micro_mab_refault_followup_translated_ld(
        aligned_upper_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and repaired upper-half aligned store, a same-boundary upper-half aligned translated load should clear stale exception metadata and read the latest committed high-half payload",
        excpt.triggered == false &&
        observed_high_value == repaired_upper_store_value
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, repaired upper-store, and trailing upper-load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_upper_load_clears_stale_exception_and_reads_latest_high_half_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_byte_load_clears_stale_exception_and_reads_refault_updated_byte7_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x58, 0x46, 0x34, 0x22, 0x10, 0xfe, 0xec, 0xda,
        0xc9, 0xb7, 0xa5, 0x93, 0x81, 0x6f, 0x5d, 0x4b,
    };
    static const uint8_t guard_before_seed[8] = {
        0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84,
    };
    static const uint8_t guard_after_seed[8] = {
        0x95, 0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0xf1, 0x13,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        7U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2435465768798a9bULL;
    const uint64_t refault_store_value = 0xb4c5d6e7f8091a2bULL;
    const uint64_t aligned_upper_store_value_first = 0x132537496b7d8f91ULL;
    const uint64_t aligned_upper_store_value_second = 0xa2b4c6d8ea0c1e20ULL;
    const uint64_t aligned_upper_store_value_third = 0x31435567798b9dafULL;
    const uint64_t aligned_upper_store_value_fourth = 0xc0d1e2f304152637ULL;
    const uint64_t repaired_upper_store_value = 0x415263748596a7b8ULL;
    uint64_t observed_tail_byte = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, repaired upper-store, and trailing byte7 load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, repaired upper-store, and trailing byte7 load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-store-then-byte7-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-byte7-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-byte7-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-store-then-byte7-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-store and trailing byte7 load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, repaired_upper_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing byte7 load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_tail_byte = ai_micro_mab_refault_followup_translated_lbu(
        tail_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and repaired upper-half aligned store, a tail-byte translated load at boundary+7 should clear stale exception metadata while preserving seeded byte7 because the refaulting store has no architectural low-byte side effect",
        excpt.triggered == false &&
        observed_tail_byte == (uint64_t)boundary_seed[7]
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, repaired upper-store, and trailing byte7 load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_byte_load_clears_stale_exception_and_reads_refault_updated_byte7_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_byte_load_clears_stale_exception_and_reads_latest_high_half_byte0_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x69, 0x57, 0x45, 0x33, 0x21, 0x0f, 0xfd, 0xeb,
        0xda, 0xc8, 0xb6, 0xa4, 0x92, 0x80, 0x6e, 0x5c,
    };
    static const uint8_t guard_before_seed[8] = {
        0x17, 0x29, 0x3b, 0x4d, 0x5f, 0x61, 0x73, 0x85,
    };
    static const uint8_t guard_after_seed[8] = {
        0x96, 0xa8, 0xba, 0xcc, 0xde, 0xf0, 0xf2, 0x14,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x35465768798a9bacULL;
    const uint64_t refault_store_value = 0xc6d7e8f90a1b2c3dULL;
    const uint64_t aligned_upper_store_value_first = 0x1426384a5c6e7082ULL;
    const uint64_t aligned_upper_store_value_second = 0x93a5b7c9dbeeff01ULL;
    const uint64_t aligned_upper_store_value_third = 0x223446586a7c8e90ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xd1e2f30415263748ULL;
    const uint64_t repaired_upper_store_value = 0x5162738495a6b7c8ULL;
    uint64_t observed_first_upper_byte = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, repaired upper-store, and trailing first-upper-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, repaired upper-store, and trailing first-upper-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-store-then-first-upper-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-first-upper-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-first-upper-byte-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-store-then-first-upper-byte-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-store and trailing first-upper-byte load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, repaired_upper_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-byte load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_byte = ai_micro_mab_refault_followup_translated_lbu(
        first_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and repaired upper-half aligned store, a first-byte translated load at boundary+8 should clear stale exception metadata and read the latest committed high-half byte0",
        excpt.triggered == false &&
        observed_first_upper_byte ==
            (uint64_t)(uint8_t)repaired_upper_store_value
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, repaired upper-store, and trailing first-upper-byte load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_byte_load_clears_stale_exception_and_reads_latest_high_half_byte0_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_halfword_load_clears_stale_exception_and_reads_seeded_byte6_plus_refault_updated_byte7_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x7a, 0x68, 0x56, 0x44, 0x32, 0x20, 0x1e, 0x0c,
        0xfb, 0xe9, 0xd7, 0xc5, 0xb3, 0xa1, 0x8f, 0x7d,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74, 0x86,
    };
    static const uint8_t guard_after_seed[8] = {
        0x97, 0xa9, 0xbb, 0xcd, 0xdf, 0xe1, 0xf3, 0x15,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        6U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x465768798a9bacbdULL;
    const uint64_t refault_store_value = 0xd7e8f90a1b2c3d4eULL;
    const uint64_t aligned_upper_store_value_first = 0x2537495b6d7f8193ULL;
    const uint64_t aligned_upper_store_value_second = 0xb4c6d8ea0c1e2032ULL;
    const uint64_t aligned_upper_store_value_third = 0x334557697b8d9fb1ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xe2f3041526374859ULL;
    const uint64_t repaired_upper_store_value = 0x61728394a5b6c7d8ULL;
    const uint64_t expected_tail_halfword =
        (uint64_t)(uint16_t)((uint16_t)boundary_seed[6] |
            ((uint16_t)boundary_seed[7] << 8U));
    uint64_t observed_tail_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, repaired upper-store, and trailing tail-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, repaired upper-store, and trailing tail-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-store-then-tail-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-tail-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-tail-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-store-then-tail-halfword-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-store and trailing tail-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, repaired_upper_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing tail-halfword load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_tail_halfword = ai_micro_mab_refault_followup_translated_lhu(
        tail_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and repaired upper-half aligned store, a tail-halfword translated load at boundary+6 should clear stale exception metadata while preserving seeded bytes6-7 because the refaulting store has no architectural low-byte side effect",
        excpt.triggered == false &&
        observed_tail_halfword == expected_tail_halfword
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, repaired upper-store, and trailing tail-halfword load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_halfword_load_clears_stale_exception_and_reads_seeded_byte6_plus_refault_updated_byte7_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_halfword_load_clears_stale_exception_and_reads_latest_high_half_bytes01_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x8b, 0x79, 0x67, 0x55, 0x43, 0x31, 0x1f, 0x0d,
        0xfc, 0xea, 0xd8, 0xc6, 0xb4, 0xa2, 0x90, 0x7e,
    };
    static const uint8_t guard_before_seed[8] = {
        0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
    };
    static const uint8_t guard_after_seed[8] = {
        0x98, 0xaa, 0xbc, 0xce, 0xd0, 0xe2, 0xf4, 0x16,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_halfword_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5768798a9bacbdceULL;
    const uint64_t refault_store_value = 0xe8f90a1b2c3d4e5fULL;
    const uint64_t aligned_upper_store_value_first = 0x36485a6c7e8092a4ULL;
    const uint64_t aligned_upper_store_value_second = 0xc5d7e9fb0d1f3143ULL;
    const uint64_t aligned_upper_store_value_third = 0x4456687a8c9eb0c2ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xf30415263748596aULL;
    const uint64_t repaired_upper_store_value = 0x718293a4b5c6d7e8ULL;
    uint64_t observed_first_upper_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, repaired upper-store, and trailing first-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, repaired upper-store, and trailing first-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-store-then-first-upper-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-first-upper-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-first-upper-halfword-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-store-then-first-upper-halfword-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-store and trailing first-upper-halfword load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, repaired_upper_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-halfword load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_halfword =
        ai_micro_mab_refault_followup_translated_lhu(
            first_upper_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and repaired upper-half aligned store, a first-halfword translated load at boundary+8 should clear stale exception metadata and read the latest committed high-half bytes0-1",
        excpt.triggered == false &&
        observed_first_upper_halfword ==
            (uint64_t)(uint16_t)repaired_upper_store_value
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, repaired upper-store, and trailing first-upper-halfword load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_halfword_load_clears_stale_exception_and_reads_latest_high_half_bytes01_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_word_load_clears_stale_exception_and_reads_seeded_bytes4to6_plus_refault_updated_byte7_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x9c, 0x8a, 0x78, 0x66, 0x54, 0x42, 0x30, 0x1e,
        0x0d, 0xfb, 0xe9, 0xd7, 0xc5, 0xb3, 0xa1, 0x8f,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x17,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        4U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x68798a9bacbdcedfULL;
    const uint64_t refault_store_value = 0xf90a1b2c3d4e5f60ULL;
    const uint64_t aligned_upper_store_value_first = 0x47596b7d8f91a3b5ULL;
    const uint64_t aligned_upper_store_value_second = 0xd6e8fa0c1e203244ULL;
    const uint64_t aligned_upper_store_value_third = 0x5567798b9dafb1c3ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x0415263748596a7bULL;
    const uint64_t repaired_upper_store_value = 0x8192a3b4c5d6e7f8ULL;
    const uint64_t expected_tail_word =
        (uint64_t)(uint32_t)((uint32_t)boundary_seed[4] |
            ((uint32_t)boundary_seed[5] << 8U) |
            ((uint32_t)boundary_seed[6] << 16U) |
            ((uint32_t)boundary_seed[7] << 24U));
    uint64_t observed_tail_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, repaired upper-store, and trailing tail-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, repaired upper-store, and trailing tail-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-store-then-tail-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-tail-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-tail-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-store-then-tail-word-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-store and trailing tail-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, repaired_upper_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing tail-word load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_tail_word = ai_micro_mab_refault_followup_translated_lwu(
        tail_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and repaired upper-half aligned store, a tail-word translated load at boundary+4 should clear stale exception metadata while preserving seeded bytes4-7 because the refaulting store has no architectural low-byte side effect",
        excpt.triggered == false &&
        observed_tail_word == expected_tail_word
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, repaired upper-store, and trailing tail-word load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_word_load_clears_stale_exception_and_reads_seeded_bytes4to6_plus_refault_updated_byte7_corner");
}

bool ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_word_load_clears_stale_exception_and_reads_latest_high_half_bytes0to3_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xad, 0x9b, 0x89, 0x77, 0x65, 0x53, 0x41, 0x2f,
        0x1d, 0x0b, 0xf9, 0xe7, 0xd5, 0xc3, 0xb1, 0x9f,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1b, 0x2d, 0x3f, 0x41, 0x53, 0x65, 0x77, 0x89,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9a, 0xac, 0xbe, 0xc0, 0xd2, 0xe4, 0xf6, 0x18,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_word_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x798a9bacbdcedff0ULL;
    const uint64_t refault_store_value = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t aligned_upper_store_value_first = 0x586a7c8e90a2b4c6ULL;
    const uint64_t aligned_upper_store_value_second = 0xe7f90b1d2f314355ULL;
    const uint64_t aligned_upper_store_value_third = 0x66788a9caec0d2e4ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x15263748596a7b8cULL;
    const uint64_t repaired_upper_store_value = 0x91a2b3c4d5e6f708ULL;
    uint64_t observed_first_upper_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, repaired upper-store, and trailing first-upper-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, repaired upper-store, and trailing first-upper-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-upper-store-then-first-upper-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-first-upper-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-upper-store-then-first-upper-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-upper-store-then-first-upper-word-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the repaired upper-store and trailing first-upper-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, repaired_upper_store_value);
    TEST_ASSERT("after the refault episode is repaired again, the first upper-half aligned translated store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-word load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_word = ai_micro_mab_refault_followup_translated_lwu(
        first_upper_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and repaired upper-half aligned store, a first-word translated load at boundary+8 should clear stale exception metadata and read the latest committed high-half bytes0-3",
        excpt.triggered == false &&
        observed_first_upper_word == (uint64_t)(uint32_t)repaired_upper_store_value
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, repaired upper-store, and trailing first-upper-word load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_word_load_clears_stale_exception_and_reads_latest_high_half_bytes0to3_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xb2, 0xa0, 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34,
        0x22, 0x10, 0xfe, 0xec, 0xda, 0xc8, 0xb6, 0xa4,
    };
    static const uint8_t guard_before_seed[8] = {
        0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
    };
    static const uint8_t guard_after_seed[8] = {
        0x98, 0xaa, 0xbc, 0xce, 0xd0, 0xe2, 0xf4, 0x16,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        4U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5a6b7c8d9eafc0d1ULL;
    const uint64_t refault_store_value = 0xe2f3041526374859ULL;
    const uint64_t aligned_upper_store_value_first = 0x31435567798b9dafULL;
    const uint64_t aligned_upper_store_value_second = 0xc0d2e4f6081a2c3eULL;
    const uint64_t aligned_upper_store_value_third = 0x41536577899badbfULL;
    const uint64_t aligned_upper_store_value_fourth = 0xd1e3f507192b3d4fULL;
    const uint64_t retry_same_template_store_value = 0x60718293a4b5c6d7ULL;
    uint8_t expected_boundary[16];
    const uint64_t expected_tail_word = (uint64_t)(uint32_t)(
        (uint32_t)boundary_seed[4] |
        ((uint32_t)boundary_seed[5] << 8U) |
        ((uint32_t)boundary_seed[6] << 16U) |
        ((uint32_t)(uint8_t)retry_same_template_store_value << 24U));
    uint64_t observed_tail_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, and trailing tail-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, and trailing tail-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-tail-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-tail-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-tail-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-tail-word-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store and trailing tail-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing tail-word load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_refault_followup_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));

    TEST_SETUP_EXCEPT();
    observed_tail_word = ai_micro_mab_refault_followup_translated_lwu(
        tail_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a tail-word translated load at boundary+4 should clear stale exception metadata and still observe seeded bytes4-6 plus the retried current low-half byte7",
        excpt.triggered == false &&
        observed_tail_word == expected_tail_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_refault_followup_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, and trailing tail-word load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xc3, 0xb1, 0x9f, 0x8d, 0x7b, 0x69, 0x57, 0x45,
        0x33, 0x21, 0x0f, 0xfd, 0xeb, 0xd9, 0xc7, 0xb5,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1c, 0x2e, 0x40, 0x52, 0x54, 0x66, 0x78, 0x8a,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9b, 0xad, 0xbf, 0xc1, 0xd3, 0xe5, 0xf7, 0x19,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_REFAULT_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_word_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x6b7c8d9eafc0d1e2ULL;
    const uint64_t refault_store_value = 0xf405162738495a6bULL;
    const uint64_t aligned_upper_store_value_first = 0x425466788a9caec0ULL;
    const uint64_t aligned_upper_store_value_second = 0xd1e3f507192b3d4fULL;
    const uint64_t aligned_upper_store_value_third = 0x526476889aacbed0ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xe2f406182a3c4e50ULL;
    const uint64_t retry_same_template_store_value = 0x718293a4b5c6d7e8ULL;
    uint8_t expected_boundary[16];
    const uint64_t expected_first_upper_word =
        (uint64_t)(uint32_t)((uint32_t)(uint8_t)(retry_same_template_store_value >> 8U) |
            ((uint32_t)(uint8_t)(retry_same_template_store_value >> 16U) << 8U) |
            ((uint32_t)(uint8_t)(retry_same_template_store_value >> 24U) << 16U) |
            ((uint32_t)(uint8_t)(retry_same_template_store_value >> 32U) << 24U));
    uint64_t observed_first_upper_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_refault_followup_reset_env();
    ai_micro_mab_refault_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_refault_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_refault_followup_prepare_supervisor_translation();
    ai_micro_mab_refault_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_refault_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, and trailing first-upper-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, and trailing first-upper-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-first-upper-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-word-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-first-upper-word-load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_REFAULT_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store and trailing first-upper-word load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_refault_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_refault_followup_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-word load probe",
        excpt.triggered == false
    );

    ai_micro_mab_refault_followup_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_refault_followup_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));

    TEST_SETUP_EXCEPT();
    observed_first_upper_word = ai_micro_mab_refault_followup_translated_lwu(
        first_upper_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-word translated load at boundary+8 should clear stale exception metadata and read the retried high-half bytes0-3",
        excpt.triggered == false &&
        observed_first_upper_word == expected_first_upper_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_refault_followup_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, and trailing first-upper-word load sequence",
        ai_micro_mab_refault_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_refault_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_refault_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_corner");
}
