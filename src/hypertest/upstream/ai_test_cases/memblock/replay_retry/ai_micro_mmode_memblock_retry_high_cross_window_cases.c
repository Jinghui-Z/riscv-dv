#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep cross-window retry-high consumer narrowing in a fresh file once the
 * previous follow-up file becomes too long to read comfortably.
 */

#define AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_HIGH_XWIN_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_retry_high_xwin_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_high_xwin_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_high_xwin_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_high_xwin_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_high_xwin_set_leaf_paddr(
        page, phys_page_base(page), flags);
}

static inline void ai_micro_mab_retry_high_xwin_prepare_supervisor_translation(
    void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_retry_high_xwin_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_RWX);
}

static inline void ai_micro_mab_retry_high_xwin_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_retry_high_xwin_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_RWX | (deny_cfg << 8) |
            (AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_high_xwin_reset_env(void)
{
    ai_micro_mab_retry_high_xwin_restore_pmp_full_access();
    ai_micro_mab_retry_high_xwin_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_high_xwin_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_high_xwin_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_retry_high_xwin_overlay_little_endian_bytes(
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

#define AI_MICRO_MAB_RETRY_HIGH_XWIN_TRANSLATED_STORE_INSTRUCTION(name, inst, type) \
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

AI_MICRO_MAB_RETRY_HIGH_XWIN_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_high_xwin_translated_sd, "sd", uint64_t)

#define AI_MICRO_MAB_RETRY_HIGH_XWIN_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
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

AI_MICRO_MAB_RETRY_HIGH_XWIN_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_xwin_translated_lbu, "lbu")
AI_MICRO_MAB_RETRY_HIGH_XWIN_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_xwin_translated_lhu, "lhu")

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_byte_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x61, 0x4f, 0x3d, 0x2b, 0x19, 0x07, 0xf5, 0xe3,
        0xd1, 0xbf, 0xad, 0x9b, 0x89, 0x77, 0x65, 0x53,
    };
    static const uint8_t guard_before_seed[8] = {
        0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84, 0x96,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_byte_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x93a4b5c6d7e8f90aULL;
    const uint64_t refault_store_value = 0x263748596a7b8c9dULL;
    const uint64_t aligned_upper_store_value_first = 0xd7e8fa0b1d2f4153ULL;
    const uint64_t aligned_upper_store_value_second = 0x687a8c9eb0c2d4e6ULL;
    const uint64_t aligned_upper_store_value_third = 0xf80a1c2e40526476ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x899b9dafc1d3e5f7ULL;
    const uint64_t retry_same_template_store_value = 0x1a2b3c4d5e6f8091ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_byte = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_first_upper_byte = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_xwin_reset_env();
    ai_micro_mab_retry_high_xwin_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_xwin_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_xwin_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_xwin_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_xwin_prepare_supervisor_translation();
    ai_micro_mab_retry_high_xwin_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_xwin_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the first-upper-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the first-upper-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_xwin_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-first-upper-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-first-upper-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_xwin_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the first-upper-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_xwin_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-byte probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_xwin_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_xwin_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_byte = (uint64_t)expected_boundary[8];
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_first_upper_byte =
        ai_micro_mab_retry_high_xwin_translated_lbu(first_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-upper-byte translated load at boundary+8 should clear stale exception metadata and read the retried high-half byte0 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_first_upper_byte == expected_first_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_xwin_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the first-upper-byte probe has already shown the retried high-half byte0 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_xwin_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the first-upper-byte probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_xwin_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_xwin_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_xwin_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_byte_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x72, 0x60, 0x4e, 0x3c, 0x2a, 0x18, 0x06, 0xf4,
        0xe2, 0xd0, 0xbe, 0xac, 0x9a, 0x88, 0x76, 0x64,
    };
    static const uint8_t guard_before_seed[8] = {
        0x39, 0x4b, 0x5d, 0x6f, 0x71, 0x83, 0x95, 0xa7,
    };
    static const uint8_t guard_after_seed[8] = {
        0xb8, 0xca, 0xdc, 0xee, 0x00, 0x12, 0x24, 0x36,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t second_upper_byte_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET +
        9U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xa4b5c6d7e8f90a1bULL;
    const uint64_t refault_store_value = 0x3748596a7b8c9daeULL;
    const uint64_t aligned_upper_store_value_first = 0xe8fa0b1d2f415365ULL;
    const uint64_t aligned_upper_store_value_second = 0x798b9dafc1d3e5f7ULL;
    const uint64_t aligned_upper_store_value_third = 0x091b2d3f51637587ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x9aacbed0e2f40618ULL;
    const uint64_t retry_same_template_store_value = 0x2b3c4d5e6f8091a2ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_second_upper_byte = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_second_upper_byte = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_xwin_reset_env();
    ai_micro_mab_retry_high_xwin_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_xwin_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_xwin_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_xwin_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_xwin_prepare_supervisor_translation();
    ai_micro_mab_retry_high_xwin_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_xwin_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the second-upper-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the second-upper-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_xwin_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-second-upper-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-second-upper-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-second-upper-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-second-upper-byte-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_xwin_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the second-upper-byte probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_xwin_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing second-upper-byte probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_xwin_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_xwin_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_second_upper_byte = (uint64_t)expected_boundary[9];
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_second_upper_byte =
        ai_micro_mab_retry_high_xwin_translated_lbu(second_upper_byte_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a second-upper-byte translated load at boundary+9 should clear stale exception metadata and read the retried high-half byte1 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_second_upper_byte == expected_second_upper_byte
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_xwin_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the second-upper-byte probe has already shown the retried high-half byte1 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_xwin_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the second-upper-byte probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_xwin_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_xwin_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_xwin_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_halfword_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x83, 0x71, 0x5f, 0x4d, 0x3b, 0x29, 0x17, 0x05,
        0xf3, 0xe1, 0xcf, 0xbd, 0xab, 0x99, 0x87, 0x75,
    };
    static const uint8_t guard_before_seed[8] = {
        0x4a, 0x5c, 0x6e, 0x80, 0x82, 0x94, 0xa6, 0xb8,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc9, 0xdb, 0xed, 0xff, 0x11, 0x23, 0x35, 0x47,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_XWIN_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xb5c6d7e8f90a1b2cULL;
    const uint64_t refault_store_value = 0x48596a7b8c9daebfULL;
    const uint64_t aligned_upper_store_value_first = 0xfa0b1d2f41536577ULL;
    const uint64_t aligned_upper_store_value_second = 0x8b9dafc1d3e5f709ULL;
    const uint64_t aligned_upper_store_value_third = 0x1c2e40526476889aULL;
    const uint64_t aligned_upper_store_value_fourth = 0xadbfc1d3e5f7091bULL;
    const uint64_t retry_same_template_store_value = 0x3c4d5e6f8091a2b3ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_halfword = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_first_upper_halfword = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_xwin_reset_env();
    ai_micro_mab_retry_high_xwin_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_xwin_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_xwin_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_xwin_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_xwin_prepare_supervisor_translation();
    ai_micro_mab_retry_high_xwin_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_xwin_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the first-upper-halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the first-upper-halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_xwin_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-first-upper-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-first-upper-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_xwin_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_XWIN_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the first-upper-halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_xwin_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_xwin_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-halfword probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_xwin_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_xwin_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_halfword =
        (uint64_t)expected_boundary[8] |
        ((uint64_t)expected_boundary[9] << 8U);
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_first_upper_halfword =
        ai_micro_mab_retry_high_xwin_translated_lhu(
            first_upper_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-upper-halfword translated load at boundary+8 should clear stale exception metadata and read the retried high-half bytes0-1 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_first_upper_halfword == expected_first_upper_halfword
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_xwin_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the first-upper-halfword probe has already shown the retried high-half bytes0-1 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_xwin_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the first-upper-halfword probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_xwin_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_xwin_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_xwin_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_halfword_then_upper_tail_last_halfword_corner");
}
