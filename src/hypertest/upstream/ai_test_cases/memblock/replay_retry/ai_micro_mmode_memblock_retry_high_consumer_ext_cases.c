#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep follow-up width-escalation probes separate once the main retry-high
 * consumer file becomes too long to read comfortably.
 */

#define AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_retry_high_ext_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_high_ext_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_high_ext_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_high_ext_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_high_ext_set_leaf_paddr(
        page, phys_page_base(page), flags);
}

static inline void ai_micro_mab_retry_high_ext_prepare_supervisor_translation(
    void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_retry_high_ext_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_RWX);
}

static inline void ai_micro_mab_retry_high_ext_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_retry_high_ext_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_RWX | (deny_cfg << 8) |
            (AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_high_ext_reset_env(void)
{
    ai_micro_mab_retry_high_ext_restore_pmp_full_access();
    ai_micro_mab_retry_high_ext_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_high_ext_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_high_ext_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
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

static inline uint64_t ai_micro_mab_retry_high_ext_u64_from_bytes(
    const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }
    return value;
}

#define AI_MICRO_MAB_RETRY_HIGH_EXT_TRANSLATED_STORE_INSTRUCTION( \
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

AI_MICRO_MAB_RETRY_HIGH_EXT_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_high_ext_translated_sd, "sd", uint64_t)

#define AI_MICRO_MAB_RETRY_HIGH_EXT_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
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

AI_MICRO_MAB_RETRY_HIGH_EXT_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_ext_translated_lbu, "lbu")
AI_MICRO_MAB_RETRY_HIGH_EXT_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_ext_translated_lhu, "lhu")
AI_MICRO_MAB_RETRY_HIGH_EXT_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_ext_translated_lwu, "lwu")

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_halfword_then_first_upper_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x1b, 0x09, 0xf7, 0xe5, 0xd3, 0xc1, 0xaf, 0x9d,
        0x8b, 0x79, 0x67, 0x55, 0x43, 0x31, 0x1f, 0x0d,
    };
    static const uint8_t guard_before_seed[8] = {
        0x26, 0x38, 0x4a, 0x5c, 0x5e, 0x70, 0x82, 0x94,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa5, 0xb7, 0xc9, 0xdb, 0xed, 0xff, 0x11, 0x23,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t second_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        9U;
    const uintptr_t first_upper_halfword_load_vaddr =
        first_upper_byte_load_vaddr;
    const uintptr_t first_upper_word_load_vaddr =
        first_upper_byte_load_vaddr;
    const uintptr_t aligned_upper_store_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xd9eafb0c1d2e3f40ULL;
    const uint64_t refault_store_value = 0x6a7b8c9daebfd0e1ULL;
    const uint64_t aligned_upper_store_value_first = 0x1a2c3e5062748698ULL;
    const uint64_t aligned_upper_store_value_second = 0xabbdcff103152739ULL;
    const uint64_t aligned_upper_store_value_third = 0x3b4d5f718395a7b9ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xccdef0021426384aULL;
    const uint64_t retry_same_template_store_value = 0x5d6e7f90a1b2c3d4ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_byte = 0ULL;
    uint64_t expected_second_upper_byte = 0ULL;
    uint64_t expected_first_upper_halfword = 0ULL;
    uint64_t expected_first_upper_word = 0ULL;
    uint64_t observed_first_upper_byte = 0ULL;
    uint64_t observed_second_upper_byte = 0ULL;
    uint64_t observed_first_upper_halfword = 0ULL;
    uint64_t observed_first_upper_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, trailing dual-head-byte probes, the first-upper-halfword probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, trailing dual-head-byte probes, the first-upper-halfword probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-width-escalation probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-width-escalation probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-width-escalation probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-width-escalation probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store and trailing width-escalation probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing width-escalation probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_byte = (uint64_t)expected_boundary[8];
    expected_second_upper_byte = (uint64_t)expected_boundary[9];
    expected_first_upper_halfword =
        (uint64_t)(uint16_t)ai_micro_mab_retry_high_ext_u64_from_bytes(
            expected_boundary + 8U);
    expected_first_upper_word =
        (uint64_t)(uint32_t)ai_micro_mab_retry_high_ext_u64_from_bytes(
            expected_boundary + 8U);

    TEST_SETUP_EXCEPT();
    observed_first_upper_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(first_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-byte translated load at boundary+8 should clear stale exception metadata and read the retried high-half byte0 before the following width-escalation probes",
        excpt.triggered == false &&
        observed_first_upper_byte == expected_first_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_second_upper_byte = ai_micro_mab_retry_high_ext_translated_lbu(
        second_upper_byte_load_vaddr);
    TEST_ASSERT("after the boundary+8 byte probe has already completed, a second-byte translated load at boundary+9 should still clear stale exception metadata and read the retried high-half byte1 before the following wider probes",
        excpt.triggered == false &&
        observed_second_upper_byte == expected_second_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_halfword =
        ai_micro_mab_retry_high_ext_translated_lhu(
            first_upper_halfword_load_vaddr);
    TEST_ASSERT("after the dual head-byte probes have already shown retried high-half bytes0-1 individually visible, a first-upper-halfword translated load at boundary+8 should still clear stale exception metadata and read the combined retried high-half bytes0-1 before the final first-upper-word probe",
        excpt.triggered == false &&
        observed_first_upper_halfword == expected_first_upper_halfword
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_word =
        ai_micro_mab_retry_high_ext_translated_lwu(
            first_upper_word_load_vaddr);
    TEST_ASSERT("after the dual head-byte probes and the first-upper-halfword probe have already shown retried high-half bytes0-1 visible, a first-upper-word translated load at boundary+8 should still clear stale exception metadata and read the combined retried high-half bytes0-3",
        excpt.triggered == false &&
        observed_first_upper_word == expected_first_upper_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, trailing dual-head-byte probes, the first-upper-halfword probe, and the final first-upper-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_halfword_then_first_upper_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x2c, 0x1a, 0x08, 0xf6, 0xe4, 0xd2, 0xc0, 0xae,
        0x9c, 0x8a, 0x78, 0x66, 0x54, 0x42, 0x30, 0x1e,
    };
    static const uint8_t guard_before_seed[8] = {
        0x27, 0x39, 0x4b, 0x5d, 0x5f, 0x71, 0x83, 0x95,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa6, 0xb8, 0xca, 0xdc, 0xee, 0x00, 0x12, 0x24,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t second_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        9U;
    const uintptr_t first_upper_word_load_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t aligned_upper_store_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xeafb0c1d2e3f4051ULL;
    const uint64_t refault_store_value = 0x7b8c9daebfd0e1f2ULL;
    const uint64_t aligned_upper_store_value_first = 0x2b3d4f61738597a9ULL;
    const uint64_t aligned_upper_store_value_second = 0xbcced0f20416283aULL;
    const uint64_t aligned_upper_store_value_third = 0x4c5e708294a6b8caULL;
    const uint64_t aligned_upper_store_value_fourth = 0xddf1031527394b5dULL;
    const uint64_t retry_same_template_store_value = 0x6e7f90a1b2c3d4e5ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_byte = 0ULL;
    uint64_t expected_second_upper_byte = 0ULL;
    uint64_t expected_first_upper_word = 0ULL;
    uint64_t observed_first_upper_byte = 0ULL;
    uint64_t observed_second_upper_byte = 0ULL;
    uint64_t observed_first_upper_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, trailing dual-head-byte probes, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, trailing dual-head-byte probes, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-dual-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-dual-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-dual-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-dual-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, trailing dual-head-byte probes, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing dual-head-byte probes and final first-upper-word probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_byte = (uint64_t)expected_boundary[8];
    expected_second_upper_byte = (uint64_t)expected_boundary[9];
    expected_first_upper_word =
        (uint64_t)(uint32_t)ai_micro_mab_retry_high_ext_u64_from_bytes(
            expected_boundary + 8U);

    TEST_SETUP_EXCEPT();
    observed_first_upper_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(first_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-byte translated load at boundary+8 should clear stale exception metadata and read the retried high-half byte0 before the following first-upper-word probe",
        excpt.triggered == false &&
        observed_first_upper_byte == expected_first_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_second_upper_byte = ai_micro_mab_retry_high_ext_translated_lbu(
        second_upper_byte_load_vaddr);
    TEST_ASSERT("after the boundary+8 byte probe has already completed, a second-byte translated load at boundary+9 should still clear stale exception metadata and read the retried high-half byte1 before the final first-upper-word probe",
        excpt.triggered == false &&
        observed_second_upper_byte == expected_second_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_word =
        ai_micro_mab_retry_high_ext_translated_lwu(
            first_upper_word_load_vaddr);
    TEST_ASSERT("after the dual head-byte probes have already shown retried high-half bytes0-1 individually visible, a first-upper-word translated load at boundary+8 should still clear stale exception metadata and read the combined retried high-half bytes0-3",
        excpt.triggered == false &&
        observed_first_upper_word == expected_first_upper_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, trailing dual-head-byte probes, and the final first-upper-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_single_byte_then_first_upper_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x3d, 0x2b, 0x19, 0x07, 0xf5, 0xe3, 0xd1, 0xbf,
        0xad, 0x9b, 0x89, 0x77, 0x65, 0x53, 0x41, 0x2f,
    };
    static const uint8_t guard_before_seed[8] = {
        0x2a, 0x3c, 0x4e, 0x60, 0x62, 0x74, 0x86, 0x98,
    };
    static const uint8_t guard_after_seed[8] = {
        0xb7, 0xc9, 0xdb, 0xed, 0xff, 0x11, 0x23, 0x35,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t first_upper_word_load_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t aligned_upper_store_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xdcebfa0918273645ULL;
    const uint64_t refault_store_value = 0x6f7e8d9cabbac9d8ULL;
    const uint64_t aligned_upper_store_value_first = 0x10223446586a7c8eULL;
    const uint64_t aligned_upper_store_value_second = 0xa1b3c5d7e9fb0d1fULL;
    const uint64_t aligned_upper_store_value_third = 0x31435567798b9dafULL;
    const uint64_t aligned_upper_store_value_fourth = 0xc2d4e6f80a1c2e40ULL;
    const uint64_t retry_same_template_store_value = 0x5364758697a8b9caULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_byte = 0ULL;
    uint64_t expected_first_upper_word = 0ULL;
    uint64_t observed_first_upper_byte = 0ULL;
    uint64_t observed_first_upper_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the first head-byte probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the first head-byte probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-first-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-first-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the first head-byte probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-head-byte probe and final first-upper-word probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_byte = (uint64_t)expected_boundary[8];
    expected_first_upper_word =
        (uint64_t)(uint32_t)ai_micro_mab_retry_high_ext_u64_from_bytes(
            expected_boundary + 8U);

    TEST_SETUP_EXCEPT();
    observed_first_upper_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(first_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-byte translated load at boundary+8 should clear stale exception metadata and read the retried high-half byte0 before the following first-upper-word probe",
        excpt.triggered == false &&
        observed_first_upper_byte == expected_first_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_word =
        ai_micro_mab_retry_high_ext_translated_lwu(
            first_upper_word_load_vaddr);
    TEST_ASSERT("after the first head-byte probe has already shown retried high-half byte0 individually visible, a first-upper-word translated load at boundary+8 should still clear stale exception metadata and read the combined retried high-half bytes0-3",
        excpt.triggered == false &&
        observed_first_upper_word == expected_first_upper_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the first head-byte probe, and the final first-upper-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_single_byte_then_first_upper_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_then_first_upper_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x4e, 0x3c, 0x2a, 0x18, 0x06, 0xf4, 0xe2, 0xd0,
        0xbe, 0xac, 0x9a, 0x88, 0x76, 0x64, 0x52, 0x40,
    };
    static const uint8_t guard_before_seed[8] = {
        0x33, 0x45, 0x57, 0x69, 0x6b, 0x7d, 0x8f, 0xa1,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc0, 0xd2, 0xe4, 0xf6, 0x08, 0x1a, 0x2c, 0x3e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t second_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        9U;
    const uintptr_t first_upper_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_word_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xedfc0b1a29384756ULL;
    const uint64_t refault_store_value = 0x708f9eadbccbdae9ULL;
    const uint64_t aligned_upper_store_value_first = 0x21334557697b8d9fULL;
    const uint64_t aligned_upper_store_value_second = 0xb2c4d6e8fa0c1e20ULL;
    const uint64_t aligned_upper_store_value_third = 0x425466788a9caec0ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xd3e5f7091b2d3f41ULL;
    const uint64_t retry_same_template_store_value = 0x64758697a8b9cadbULL;
    uint8_t expected_boundary[16];
    uint64_t expected_second_upper_byte = 0ULL;
    uint64_t expected_first_upper_word = 0ULL;
    uint64_t observed_second_upper_byte = 0ULL;
    uint64_t observed_first_upper_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the second head-byte probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the second head-byte probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-second-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-second-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-second-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-second-head-byte-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the second head-byte probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing second-head-byte probe and final first-upper-word probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_second_upper_byte = (uint64_t)expected_boundary[9];
    expected_first_upper_word =
        (uint64_t)(uint32_t)ai_micro_mab_retry_high_ext_u64_from_bytes(
            expected_boundary + 8U);

    TEST_SETUP_EXCEPT();
    observed_second_upper_byte = ai_micro_mab_retry_high_ext_translated_lbu(
        second_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a second-byte translated load at boundary+9 should clear stale exception metadata and read the retried high-half byte1 before the following first-upper-word probe",
        excpt.triggered == false &&
        observed_second_upper_byte == expected_second_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_word =
        ai_micro_mab_retry_high_ext_translated_lwu(
            first_upper_word_load_vaddr);
    TEST_ASSERT("after the second head-byte probe has already shown retried high-half byte1 individually visible, a first-upper-word translated load at boundary+8 should still clear stale exception metadata and read the combined retried high-half bytes0-3",
        excpt.triggered == false &&
        observed_first_upper_word == expected_first_upper_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the second head-byte probe, and the final first-upper-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_then_first_upper_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_word_then_first_upper_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x5f, 0x4d, 0x3b, 0x29, 0x17, 0x05, 0xf3, 0xe1,
        0xcf, 0xbd, 0xab, 0x99, 0x87, 0x75, 0x63, 0x51,
    };
    static const uint8_t guard_before_seed[8] = {
        0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82, 0x94, 0xa6,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc5, 0xd7, 0xe9, 0xfb, 0x0d, 0x1f, 0x31, 0x43,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t first_upper_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_word_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xfe0d1c2b3a495867ULL;
    const uint64_t refault_store_value = 0x8190a0bfcddcebf1ULL;
    const uint64_t aligned_upper_store_value_first = 0x324456687a8c9eb0ULL;
    const uint64_t aligned_upper_store_value_second = 0xc3d5e7f90b1d2f31ULL;
    const uint64_t aligned_upper_store_value_third = 0x536577899badbfd1ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xe4f6081a2c3e4052ULL;
    const uint64_t retry_same_template_store_value = 0x758697a8b9cadbecULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_word = 0ULL;
    uint64_t expected_first_upper_word = 0ULL;
    uint64_t observed_tail_word = 0ULL;
    uint64_t observed_first_upper_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-word probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-word probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-word-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-word-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-word-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-word-and-first-upper-word probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-word probe, and the final first-upper-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-word probe and final first-upper-word probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_word =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U) |
        ((uint64_t)expected_boundary[14] << 16U) |
        ((uint64_t)expected_boundary[15] << 24U);
    expected_first_upper_word =
        (uint64_t)(uint32_t)ai_micro_mab_retry_high_ext_u64_from_bytes(
            expected_boundary + 8U);

    TEST_SETUP_EXCEPT();
    observed_tail_word =
        ai_micro_mab_retry_high_ext_translated_lwu(tail_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-word translated load at boundary+12 should clear stale exception metadata and read the retried high-half bytes4-7 before the following first-upper-word probe",
        excpt.triggered == false &&
        observed_tail_word == expected_tail_word
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_word =
        ai_micro_mab_retry_high_ext_translated_lwu(
            first_upper_word_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already shown the retried high-half tail bytes4-7 visible, a first-upper-word translated load at boundary+8 should still clear stale exception metadata and read the combined retried high-half bytes0-3",
        excpt.triggered == false &&
        observed_first_upper_word == expected_first_upper_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-word probe, and the final first-upper-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_word_then_first_upper_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_byte_then_upper_tail_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x60, 0x4e, 0x3c, 0x2a, 0x18, 0x06, 0xf4, 0xe2,
        0xd0, 0xbe, 0xac, 0x9a, 0x88, 0x76, 0x64, 0x52,
    };
    static const uint8_t guard_before_seed[8] = {
        0x3d, 0x4f, 0x61, 0x73, 0x75, 0x87, 0x99, 0xab,
    };
    static const uint8_t guard_after_seed[8] = {
        0xca, 0xdc, 0xee, 0x00, 0x12, 0x24, 0x36, 0x48,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t tail_word_load_vaddr = tail_byte_load_vaddr;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t refault_store_value = 0x92a1b0cfdeedfc0bULL;
    const uint64_t aligned_upper_store_value_first = 0x435567798b9dafc1ULL;
    const uint64_t aligned_upper_store_value_second = 0xd4e6f80a1c2e3032ULL;
    const uint64_t aligned_upper_store_value_third = 0x6476889aacbed0e2ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xf507192b3d4f5163ULL;
    const uint64_t retry_same_template_store_value = 0x8697a8b9cadbecfdULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_byte = 0ULL;
    uint64_t expected_tail_word = 0ULL;
    uint64_t observed_tail_byte = 0ULL;
    uint64_t observed_tail_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-byte probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-byte probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-byte probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-byte probe and final upper-tail-word probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_byte = (uint64_t)expected_boundary[12];
    expected_tail_word =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U) |
        ((uint64_t)expected_boundary[14] << 16U) |
        ((uint64_t)expected_boundary[15] << 24U);

    TEST_SETUP_EXCEPT();
    observed_tail_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(tail_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-byte translated load at boundary+12 should clear stale exception metadata and read the retried high-half byte4 before the following upper-tail-word probe",
        excpt.triggered == false &&
        observed_tail_byte == expected_tail_byte
    );

    TEST_SETUP_EXCEPT();
    observed_tail_word =
        ai_micro_mab_retry_high_ext_translated_lwu(tail_word_load_vaddr);
    TEST_ASSERT("after the upper-tail-byte probe has already shown the retried high-half byte4 individually visible, an upper-tail-word translated load at boundary+12 should still clear stale exception metadata and read the combined retried high-half bytes4-7",
        excpt.triggered == false &&
        observed_tail_word == expected_tail_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-byte probe, and the final upper-tail-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_byte_then_upper_tail_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_upper_tail_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x71, 0x5f, 0x4d, 0x3b, 0x29, 0x17, 0x05, 0xf3,
        0xe1, 0xcf, 0xbd, 0xab, 0x99, 0x87, 0x75, 0x63,
    };
    static const uint8_t guard_before_seed[8] = {
        0x4a, 0x5c, 0x6e, 0x80, 0x82, 0x94, 0xa6, 0xb8,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd7, 0xe9, 0xfb, 0x0d, 0x1f, 0x31, 0x43, 0x55,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t tail_word_load_vaddr = tail_halfword_load_vaddr;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x1021324354657687ULL;
    const uint64_t refault_store_value = 0xa3b4c5d6e7f8091aULL;
    const uint64_t aligned_upper_store_value_first = 0x5466788a9caec0d2ULL;
    const uint64_t aligned_upper_store_value_second = 0xe5f7091b2d3f4153ULL;
    const uint64_t aligned_upper_store_value_third = 0x758799abbddff103ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x06182a3c4e606274ULL;
    const uint64_t retry_same_template_store_value = 0x97a8b9cadbecfd0eULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_halfword = 0ULL;
    uint64_t expected_tail_word = 0ULL;
    uint64_t observed_tail_halfword = 0ULL;
    uint64_t observed_tail_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-halfword probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-halfword probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-halfword-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-halfword-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-halfword-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-halfword-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-halfword probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-halfword probe and final upper-tail-word probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_halfword =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U);
    expected_tail_word =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U) |
        ((uint64_t)expected_boundary[14] << 16U) |
        ((uint64_t)expected_boundary[15] << 24U);

    TEST_SETUP_EXCEPT();
    observed_tail_halfword =
        ai_micro_mab_retry_high_ext_translated_lhu(tail_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-halfword translated load at boundary+12 should clear stale exception metadata and read the retried high-half bytes4-5 before the following upper-tail-word probe",
        excpt.triggered == false &&
        observed_tail_halfword == expected_tail_halfword
    );

    TEST_SETUP_EXCEPT();
    observed_tail_word =
        ai_micro_mab_retry_high_ext_translated_lwu(tail_word_load_vaddr);
    TEST_ASSERT("after the upper-tail-halfword probe has already shown the retried high-half bytes4-5 visible, an upper-tail-word translated load at boundary+12 should still clear stale exception metadata and read the combined retried high-half bytes4-7",
        excpt.triggered == false &&
        observed_tail_word == expected_tail_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-halfword probe, and the final upper-tail-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_upper_tail_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_then_upper_tail_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x82, 0x70, 0x5e, 0x4c, 0x3a, 0x28, 0x16, 0x04,
        0xf2, 0xe0, 0xce, 0xbc, 0xaa, 0x98, 0x86, 0x74,
    };
    static const uint8_t guard_before_seed[8] = {
        0x57, 0x69, 0x7b, 0x8d, 0x8f, 0xa1, 0xb3, 0xc5,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe4, 0xf6, 0x08, 0x1a, 0x2c, 0x3e, 0x50, 0x62,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t tail_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2132435465768798ULL;
    const uint64_t refault_store_value = 0xb4c5d6e7f8091a2bULL;
    const uint64_t aligned_upper_store_value_first = 0x6577899badbfd1e3ULL;
    const uint64_t aligned_upper_store_value_second = 0xf6081a2c3e405264ULL;
    const uint64_t aligned_upper_store_value_third = 0x8698aabcced0f204ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x17293b4d5f617385ULL;
    const uint64_t retry_same_template_store_value = 0xa8b9cadbecfd0e1fULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t expected_tail_word = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;
    uint64_t observed_tail_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-last-halfword probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-last-halfword probe and final upper-tail-word probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);
    expected_tail_word =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U) |
        ((uint64_t)expected_boundary[14] << 16U) |
        ((uint64_t)expected_boundary[15] << 24U);

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_ext_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-last-halfword translated load at boundary+14 should clear stale exception metadata and read the retried high-half bytes6-7 before the following upper-tail-word probe",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_SETUP_EXCEPT();
    observed_tail_word =
        ai_micro_mab_retry_high_ext_translated_lwu(tail_word_load_vaddr);
    TEST_ASSERT("after the upper-tail-last-halfword probe has already shown the retried high-half bytes6-7 visible, an upper-tail-word translated load at boundary+12 should still clear stale exception metadata and read the combined retried high-half bytes4-7",
        excpt.triggered == false &&
        observed_tail_word == expected_tail_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-last-halfword probe, and the final upper-tail-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_then_upper_tail_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_low_byte_then_upper_tail_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x93, 0x81, 0x6f, 0x5d, 0x4b, 0x39, 0x27, 0x15,
        0x03, 0xf1, 0xdf, 0xcd, 0xbb, 0xa9, 0x97, 0x85,
    };
    static const uint8_t guard_before_seed[8] = {
        0x64, 0x76, 0x88, 0x9a, 0x9c, 0xae, 0xc0, 0xd2,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf1, 0x03, 0x15, 0x27, 0x39, 0x4b, 0x5d, 0x6f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_last_halfword_low_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t tail_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x32435465768798a9ULL;
    const uint64_t refault_store_value = 0xc5d6e7f8091a2b3cULL;
    const uint64_t aligned_upper_store_value_first = 0x76889aacbed0e2f4ULL;
    const uint64_t aligned_upper_store_value_second = 0x07192b3d4f516375ULL;
    const uint64_t aligned_upper_store_value_third = 0x97a9bbcddff10315ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x283a4c5e60728496ULL;
    const uint64_t retry_same_template_store_value = 0xb9cadbecfd0e1f30ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_last_halfword_low_byte = 0ULL;
    uint64_t expected_tail_word = 0ULL;
    uint64_t observed_tail_last_halfword_low_byte = 0ULL;
    uint64_t observed_tail_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword-low-byte probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword-low-byte probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-low-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-low-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-low-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-low-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-last-halfword-low-byte probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-last-halfword-low-byte probe and final upper-tail-word probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_last_halfword_low_byte = (uint64_t)expected_boundary[14];
    expected_tail_word =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U) |
        ((uint64_t)expected_boundary[14] << 16U) |
        ((uint64_t)expected_boundary[15] << 24U);

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_low_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(
            tail_last_halfword_low_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-last-halfword-low-byte translated load at boundary+14 should clear stale exception metadata and read the retried high-half byte6 before the following upper-tail-word probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_low_byte ==
            expected_tail_last_halfword_low_byte
    );

    TEST_SETUP_EXCEPT();
    observed_tail_word =
        ai_micro_mab_retry_high_ext_translated_lwu(tail_word_load_vaddr);
    TEST_ASSERT("after the upper-tail-last-halfword-low-byte probe has already shown the retried high-half byte6 visible, an upper-tail-word translated load at boundary+12 should still clear stale exception metadata and read the combined retried high-half bytes4-7",
        excpt.triggered == false &&
        observed_tail_word == expected_tail_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-last-halfword-low-byte probe, and the final upper-tail-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_low_byte_then_upper_tail_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_high_byte_then_upper_tail_word_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa4, 0x92, 0x80, 0x6e, 0x5c, 0x4a, 0x38, 0x26,
        0x14, 0x02, 0xf0, 0xde, 0xcc, 0xba, 0xa8, 0x96,
    };
    static const uint8_t guard_before_seed[8] = {
        0x71, 0x83, 0x95, 0xa7, 0xa9, 0xbb, 0xcd, 0xdf,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfe, 0x10, 0x22, 0x34, 0x46, 0x58, 0x6a, 0x7c,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_last_halfword_high_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        15U;
    const uintptr_t tail_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x435465768798a9baULL;
    const uint64_t refault_store_value = 0xd6e7f8091a2b3c4dULL;
    const uint64_t aligned_upper_store_value_first = 0x8799abbdcfe1f305ULL;
    const uint64_t aligned_upper_store_value_second = 0x182a3c4e50627486ULL;
    const uint64_t aligned_upper_store_value_third = 0xa8bacceef0021426ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x394b5d6f718395a7ULL;
    const uint64_t retry_same_template_store_value = 0xcadbecfd0e1f3041ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_last_halfword_high_byte = 0ULL;
    uint64_t expected_tail_word = 0ULL;
    uint64_t observed_tail_last_halfword_high_byte = 0ULL;
    uint64_t observed_tail_word = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword-high-byte probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword-high-byte probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-high-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-high-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-high-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-high-byte-and-upper-tail-word probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-last-halfword-high-byte probe, and the final upper-tail-word probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-last-halfword-high-byte probe and final upper-tail-word probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_last_halfword_high_byte = (uint64_t)expected_boundary[15];
    expected_tail_word =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U) |
        ((uint64_t)expected_boundary[14] << 16U) |
        ((uint64_t)expected_boundary[15] << 24U);

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_high_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(
            tail_last_halfword_high_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-last-halfword-high-byte translated load at boundary+15 should clear stale exception metadata and read the retried high-half byte7 before the following upper-tail-word probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_high_byte ==
            expected_tail_last_halfword_high_byte
    );

    TEST_SETUP_EXCEPT();
    observed_tail_word =
        ai_micro_mab_retry_high_ext_translated_lwu(tail_word_load_vaddr);
    TEST_ASSERT("after the upper-tail-last-halfword-high-byte probe has already shown the retried high-half byte7 visible, an upper-tail-word translated load at boundary+12 should still clear stale exception metadata and read the combined retried high-half bytes4-7",
        excpt.triggered == false &&
        observed_tail_word == expected_tail_word
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-last-halfword-high-byte probe, and the final upper-tail-word load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_high_byte_then_upper_tail_word_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xb5, 0xa3, 0x91, 0x7f, 0x6d, 0x5b, 0x49, 0x37,
        0x25, 0x13, 0x01, 0xef, 0xdd, 0xcb, 0xb9, 0xa7,
    };
    static const uint8_t guard_before_seed[8] = {
        0x82, 0x94, 0xa6, 0xb8, 0xba, 0xcc, 0xde, 0xf0,
    };
    static const uint8_t guard_after_seed[8] = {
        0x0f, 0x21, 0x33, 0x45, 0x57, 0x69, 0x7b, 0x8d,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_last_halfword_low_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5465768798a9bacbULL;
    const uint64_t refault_store_value = 0xe7f8091a2b3c4d5eULL;
    const uint64_t aligned_upper_store_value_first = 0x98aabcced0e2f406ULL;
    const uint64_t aligned_upper_store_value_second = 0x293b4d5f61738597ULL;
    const uint64_t aligned_upper_store_value_third = 0xb9cbddef01132537ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x4a5c6e708294a6b8ULL;
    const uint64_t retry_same_template_store_value = 0xdbecfd0e1f304152ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_last_halfword_low_byte = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_tail_last_halfword_low_byte = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword-low-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword-low-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-low-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-low-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-low-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-low-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-last-halfword-low-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-last-halfword-low-byte probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_last_halfword_low_byte = (uint64_t)expected_boundary[14];
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_low_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(
            tail_last_halfword_low_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-last-halfword-low-byte translated load at boundary+14 should clear stale exception metadata and read the retried high-half byte6 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_low_byte ==
            expected_tail_last_halfword_low_byte
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_ext_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the upper-tail-last-halfword-low-byte probe has already shown the retried high-half byte6 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-last-halfword-low-byte probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xc6, 0xb4, 0xa2, 0x90, 0x7e, 0x6c, 0x5a, 0x48,
        0x36, 0x24, 0x12, 0x00, 0xee, 0xdc, 0xca, 0xb8,
    };
    static const uint8_t guard_before_seed[8] = {
        0x93, 0xa5, 0xb7, 0xc9, 0xcb, 0xdd, 0xef, 0x01,
    };
    static const uint8_t guard_after_seed[8] = {
        0x20, 0x32, 0x44, 0x56, 0x68, 0x7a, 0x8c, 0x9e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_last_halfword_high_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        15U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xf8091a2b3c4d5e6fULL;
    const uint64_t aligned_upper_store_value_first = 0xa9bbcddff1031527ULL;
    const uint64_t aligned_upper_store_value_second = 0x3a4c5e60728496a8ULL;
    const uint64_t aligned_upper_store_value_third = 0xcadcf0021426384aULL;
    const uint64_t aligned_upper_store_value_fourth = 0x5b6d7f8193a5b7c9ULL;
    const uint64_t retry_same_template_store_value = 0xecfd0e1f30415263ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_last_halfword_high_byte = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_tail_last_halfword_high_byte = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword-high-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-last-halfword-high-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-high-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-high-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-high-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-last-halfword-high-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-last-halfword-high-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-last-halfword-high-byte probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_last_halfword_high_byte = (uint64_t)expected_boundary[15];
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_high_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(
            tail_last_halfword_high_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-last-halfword-high-byte translated load at boundary+15 should clear stale exception metadata and read the retried high-half byte7 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_high_byte ==
            expected_tail_last_halfword_high_byte
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_ext_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the upper-tail-last-halfword-high-byte probe has already shown the retried high-half byte7 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-last-halfword-high-byte probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_byte_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xd7, 0xc5, 0xb3, 0xa1, 0x8f, 0x7d, 0x6b, 0x59,
        0x47, 0x35, 0x23, 0x11, 0xff, 0xed, 0xdb, 0xc9,
    };
    static const uint8_t guard_before_seed[8] = {
        0xa4, 0xb6, 0xc8, 0xda, 0xdc, 0xee, 0x00, 0x12,
    };
    static const uint8_t guard_after_seed[8] = {
        0x31, 0x43, 0x55, 0x67, 0x79, 0x8b, 0x9d, 0xaf,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x768798a9bacbdcedULL;
    const uint64_t refault_store_value = 0x091a2b3c4d5e6f70ULL;
    const uint64_t aligned_upper_store_value_first = 0xbacbddef01132537ULL;
    const uint64_t aligned_upper_store_value_second = 0x4b5d6f718395a7b9ULL;
    const uint64_t aligned_upper_store_value_third = 0xdbedff1022344658ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x6c7e8092a4b6c8daULL;
    const uint64_t retry_same_template_store_value = 0xfd0e1f3041526374ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_byte = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_tail_byte = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-byte probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_byte = (uint64_t)expected_boundary[12];
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_tail_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(tail_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-byte translated load at boundary+12 should clear stale exception metadata and read the retried high-half byte4 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_tail_byte == expected_tail_byte
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_ext_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the upper-tail-byte probe has already shown the retried high-half byte4 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-byte probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_byte_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_second_byte_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xe8, 0xd6, 0xc4, 0xb2, 0xa0, 0x8e, 0x7c, 0x6a,
        0x58, 0x46, 0x34, 0x22, 0x10, 0xfe, 0xec, 0xda,
    };
    static const uint8_t guard_before_seed[8] = {
        0xb5, 0xc7, 0xd9, 0xeb, 0xed, 0xff, 0x11, 0x23,
    };
    static const uint8_t guard_after_seed[8] = {
        0x42, 0x54, 0x66, 0x78, 0x8a, 0x9c, 0xae, 0xc0,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_second_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        13U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t aligned_upper_store_value_first = 0xcbddef0113253749ULL;
    const uint64_t aligned_upper_store_value_second = 0x5c6e708294a6b8caULL;
    const uint64_t aligned_upper_store_value_third = 0xecfe10223446586aULL;
    const uint64_t aligned_upper_store_value_fourth = 0x7d8f91a3b5c7d9ebULL;
    const uint64_t retry_same_template_store_value = 0x0e1f304152637485ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_second_byte = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_tail_second_byte = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-second-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-second-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-second-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-second-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-second-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-second-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-second-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-second-byte probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_second_byte = (uint64_t)expected_boundary[13];
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_tail_second_byte =
        ai_micro_mab_retry_high_ext_translated_lbu(
            tail_second_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-second-byte translated load at boundary+13 should clear stale exception metadata and read the retried high-half byte5 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_tail_second_byte == expected_tail_second_byte
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_ext_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the upper-tail-second-byte probe has already shown the retried high-half byte5 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-second-byte probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_second_byte_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xf9, 0xe7, 0xd5, 0xc3, 0xb1, 0x9f, 0x8d, 0x7b,
        0x69, 0x57, 0x45, 0x33, 0x21, 0x0f, 0xfd, 0xeb,
    };
    static const uint8_t guard_before_seed[8] = {
        0xc6, 0xd8, 0xea, 0xfc, 0xfe, 0x10, 0x22, 0x34,
    };
    static const uint8_t guard_after_seed[8] = {
        0x53, 0x65, 0x77, 0x89, 0x9b, 0xad, 0xbf, 0xd1,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_EXT_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x98a9bacbdcedfe0fULL;
    const uint64_t refault_store_value = 0x2b3c4d5e6f708192ULL;
    const uint64_t aligned_upper_store_value_first = 0xdcedff1022344658ULL;
    const uint64_t aligned_upper_store_value_second = 0x6d7f8193a5b7c9dbULL;
    const uint64_t aligned_upper_store_value_third = 0xfd0f21334557697bULL;
    const uint64_t aligned_upper_store_value_fourth = 0x8ea0a2b4c6d8eafcULL;
    const uint64_t retry_same_template_store_value = 0x1f30415263748596ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_halfword = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_tail_halfword = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_ext_reset_env();
    ai_micro_mab_retry_high_ext_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_ext_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_ext_prepare_supervisor_translation();
    ai_micro_mab_retry_high_ext_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_ext_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_EXT_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_ext_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_ext_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-halfword probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_ext_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_halfword =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U);
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_tail_halfword =
        ai_micro_mab_retry_high_ext_translated_lhu(
            tail_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-halfword translated load at boundary+12 should clear stale exception metadata and read the retried high-half bytes4-5 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_tail_halfword == expected_tail_halfword
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_ext_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the upper-tail-halfword probe has already shown the retried high-half bytes4-5 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_ext_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-halfword probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_ext_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_ext_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_upper_tail_last_halfword_corner");
}
