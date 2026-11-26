#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep retry-high bridge-probe cross-window narrowing in a fresh file so the
 * middle-window and head/tail follow-up files stay readable.
 */

#define AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_retry_high_bridge_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_high_bridge_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_high_bridge_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_high_bridge_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_high_bridge_set_leaf_paddr(
        page, phys_page_base(page), flags);
}

static inline void ai_micro_mab_retry_high_bridge_prepare_supervisor_translation(
    void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_retry_high_bridge_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_RWX);
}

static inline void ai_micro_mab_retry_high_bridge_clear_machine_access_mode(
    void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_retry_high_bridge_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_RWX | (deny_cfg << 8) |
            (AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_high_bridge_reset_env(void)
{
    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();
    ai_micro_mab_retry_high_bridge_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_high_bridge_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_high_bridge_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
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

#define AI_MICRO_MAB_RETRY_HIGH_BRIDGE_TRANSLATED_STORE_INSTRUCTION(name, inst, type) \
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

AI_MICRO_MAB_RETRY_HIGH_BRIDGE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_high_bridge_translated_sd, "sd", uint64_t)

#define AI_MICRO_MAB_RETRY_HIGH_BRIDGE_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
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

AI_MICRO_MAB_RETRY_HIGH_BRIDGE_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_bridge_translated_lhu, "lhu")
AI_MICRO_MAB_RETRY_HIGH_BRIDGE_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_bridge_translated_lwu, "lwu")

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_middle_bridge_halfword_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xc8, 0xb6, 0xa4, 0x92, 0x80, 0x6e, 0x5c, 0x4a,
        0x38, 0x26, 0x14, 0x02, 0xf0, 0xde, 0xcc, 0xba,
    };
    static const uint8_t guard_before_seed[8] = {
        0x69, 0x7b, 0x8d, 0x9f, 0xa1, 0xb3, 0xc5, 0xd7,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe8, 0xfa, 0x0c, 0x1e, 0x30, 0x42, 0x54, 0x66,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t head_middle_bridge_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        9U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xf8091a2b3c4d5e6fULL;
    const uint64_t refault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_first = 0x1a2c3e5062748698ULL;
    const uint64_t aligned_upper_store_value_second = 0xa9bbcddff1031527ULL;
    const uint64_t aligned_upper_store_value_third = 0x3b4d5f718395a7b9ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xcbddef0113253749ULL;
    const uint64_t retry_same_template_store_value = 0x8091a2b3c4d5e6f7ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_head_middle_bridge_halfword = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_head_middle_bridge_halfword = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_bridge_reset_env();
    ai_micro_mab_retry_high_bridge_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_bridge_prepare_supervisor_translation();
    ai_micro_mab_retry_high_bridge_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_bridge_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the head-middle bridge halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the head-middle bridge halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-head-middle-bridge-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-head-middle-bridge-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-head-middle-bridge-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-head-middle-bridge-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the head-middle bridge halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing head-middle bridge halfword probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_head_middle_bridge_halfword =
        (uint64_t)expected_boundary[9] |
        ((uint64_t)expected_boundary[10] << 8U);
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_head_middle_bridge_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            head_middle_bridge_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a head-middle bridge halfword translated load at boundary+9 should clear stale exception metadata and read the retried high-half bytes1-2 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_head_middle_bridge_halfword ==
            expected_head_middle_bridge_halfword
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the head-middle bridge halfword probe has already shown the retried high-half bytes1-2 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bridge_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the head-middle bridge halfword probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_bridge_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_middle_bridge_halfword_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_halfword_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xd9, 0xc7, 0xb5, 0xa3, 0x91, 0x7f, 0x6d, 0x5b,
        0x49, 0x37, 0x25, 0x13, 0x01, 0xef, 0xdd, 0xcb,
    };
    static const uint8_t guard_before_seed[8] = {
        0x7a, 0x8c, 0x9e, 0xb0, 0xb2, 0xc4, 0xd6, 0xe8,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf9, 0x0b, 0x1d, 0x2f, 0x41, 0x53, 0x65, 0x77,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t middle_tail_bridge_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        11U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x091a2b3c4d5e6f70ULL;
    const uint64_t refault_store_value = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_first = 0x2b3d4f61738597a9ULL;
    const uint64_t aligned_upper_store_value_second = 0xbbcddff103152739ULL;
    const uint64_t aligned_upper_store_value_third = 0x4c5e708294a6b8caULL;
    const uint64_t aligned_upper_store_value_fourth = 0xdcfe10224456687aULL;
    const uint64_t retry_same_template_store_value = 0x91a2b3c4d5e6f708ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_middle_tail_bridge_halfword = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_middle_tail_bridge_halfword = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_bridge_reset_env();
    ai_micro_mab_retry_high_bridge_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_bridge_prepare_supervisor_translation();
    ai_micro_mab_retry_high_bridge_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_bridge_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the middle-tail bridge halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the middle-tail bridge halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-halfword-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the middle-tail bridge halfword probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing middle-tail bridge halfword probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_middle_tail_bridge_halfword =
        (uint64_t)expected_boundary[11] |
        ((uint64_t)expected_boundary[12] << 8U);
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_middle_tail_bridge_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            middle_tail_bridge_halfword_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a middle-tail bridge halfword translated load at boundary+11 should clear stale exception metadata and read the retried high-half bytes3-4 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_middle_tail_bridge_halfword ==
            expected_middle_tail_bridge_halfword
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the middle-tail bridge halfword probe has already shown the retried high-half bytes3-4 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bridge_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the middle-tail bridge halfword probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_bridge_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_halfword_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xea, 0xd8, 0xc6, 0xb4, 0xa2, 0x90, 0x7e, 0x6c,
        0x5a, 0x48, 0x36, 0x24, 0x12, 0x00, 0xee, 0xdc,
    };
    static const uint8_t guard_before_seed[8] = {
        0x8b, 0x9d, 0xaf, 0xc1, 0xc3, 0xd5, 0xe7, 0xf9,
    };
    static const uint8_t guard_after_seed[8] = {
        0x0a, 0x1c, 0x2e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t middle_tail_bridge_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        10U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_store_value = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_first = 0x3c4e60728496a8baULL;
    const uint64_t aligned_upper_store_value_second = 0xcddff1031527394bULL;
    const uint64_t aligned_upper_store_value_third = 0x5d6f8193a5b7c9dbULL;
    const uint64_t aligned_upper_store_value_fourth = 0xee00122436586a7cULL;
    const uint64_t retry_same_template_store_value = 0xa2b3c4d5e6f70819ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_middle_tail_bridge_word = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_middle_tail_bridge_word = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_bridge_reset_env();
    ai_micro_mab_retry_high_bridge_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_bridge_prepare_supervisor_translation();
    ai_micro_mab_retry_high_bridge_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_bridge_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the middle-tail bridge word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the middle-tail bridge word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the middle-tail bridge word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing middle-tail bridge word probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_middle_tail_bridge_word =
        (uint64_t)expected_boundary[10] |
        ((uint64_t)expected_boundary[11] << 8U) |
        ((uint64_t)expected_boundary[12] << 16U) |
        ((uint64_t)expected_boundary[13] << 24U);
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_middle_tail_bridge_word =
        ai_micro_mab_retry_high_bridge_translated_lwu(
            middle_tail_bridge_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a middle-tail bridge word translated load at boundary+10 should clear stale exception metadata and read the retried high-half bytes2-5 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_middle_tail_bridge_word == expected_middle_tail_bridge_word
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the middle-tail bridge word probe has already shown the retried high-half bytes2-5 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bridge_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the middle-tail bridge word probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_bridge_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xfb, 0xe9, 0xd7, 0xc5, 0xb3, 0xa1, 0x8f, 0x7d,
        0x6b, 0x59, 0x47, 0x35, 0x23, 0x11, 0xff, 0xed,
    };
    static const uint8_t guard_before_seed[8] = {
        0x9c, 0xae, 0xc0, 0xd2, 0xd4, 0xe6, 0xf8, 0x0a,
    };
    static const uint8_t guard_after_seed[8] = {
        0x1b, 0x2d, 0x3f, 0x51, 0x63, 0x75, 0x87, 0x99,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_word_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2b3c4d5e6f708192ULL;
    const uint64_t refault_store_value = 0xbacbdcedfe0f1021ULL;
    const uint64_t aligned_upper_store_value_first = 0x4d5f718395a7b9cbULL;
    const uint64_t aligned_upper_store_value_second = 0xde10224456687a8cULL;
    const uint64_t aligned_upper_store_value_third = 0x6e8092a4b6c8daecULL;
    const uint64_t aligned_upper_store_value_fourth = 0xff1133557799bbcdULL;
    const uint64_t retry_same_template_store_value = 0xb3c4d5e6f708192aULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_word = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_first_upper_word = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_bridge_reset_env();
    ai_micro_mab_retry_high_bridge_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_bridge_prepare_supervisor_translation();
    ai_micro_mab_retry_high_bridge_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_bridge_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the first-upper-word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the first-upper-word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-first-upper-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-first-upper-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the first-upper-word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-word probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_word =
        (uint64_t)expected_boundary[8] |
        ((uint64_t)expected_boundary[9] << 8U) |
        ((uint64_t)expected_boundary[10] << 16U) |
        ((uint64_t)expected_boundary[11] << 24U);
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_first_upper_word =
        ai_micro_mab_retry_high_bridge_translated_lwu(
            first_upper_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-upper-word translated load at boundary+8 should clear stale exception metadata and read the retried high-half bytes0-3 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_first_upper_word == expected_first_upper_word
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the first-upper-word probe has already shown the retried high-half bytes0-3 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bridge_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the first-upper-word probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_bridge_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_word_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x0d, 0xfb, 0xe9, 0xd7, 0xc5, 0xb3, 0xa1, 0x8f,
        0x7d, 0x6b, 0x59, 0x47, 0x35, 0x23, 0x11, 0xff,
    };
    static const uint8_t guard_before_seed[8] = {
        0xad, 0xbf, 0xd1, 0xe3, 0xe5, 0xf7, 0x09, 0x1b,
    };
    static const uint8_t guard_after_seed[8] = {
        0x2c, 0x3e, 0x50, 0x62, 0x74, 0x86, 0x98, 0xaa,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t upper_tail_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x3c4d5e6f708192a3ULL;
    const uint64_t refault_store_value = 0xcbdcedfe0f102132ULL;
    const uint64_t aligned_upper_store_value_first = 0x5e708294a6b8cadcULL;
    const uint64_t aligned_upper_store_value_second = 0xef1133557799bbcdULL;
    const uint64_t aligned_upper_store_value_third = 0x7f91a3b5c7d9ebfdULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1022446688aacceeULL;
    const uint64_t retry_same_template_store_value = 0xc4d5e6f708192a3bULL;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_word = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_upper_tail_word = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_bridge_reset_env();
    ai_micro_mab_retry_high_bridge_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_bridge_prepare_supervisor_translation();
    ai_micro_mab_retry_high_bridge_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_bridge_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the upper-tail-word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-upper-tail-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-upper-tail-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-upper-tail-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the upper-tail-word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing upper-tail-word probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_upper_tail_word =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U) |
        ((uint64_t)expected_boundary[14] << 16U) |
        ((uint64_t)expected_boundary[15] << 24U);
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word =
        ai_micro_mab_retry_high_bridge_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, an upper-tail-word translated load at boundary+12 should clear stale exception metadata and read the retried high-half bytes4-7 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_upper_tail_word == expected_upper_tail_word
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already shown the retried high-half bytes4-7 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bridge_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the upper-tail-word probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_bridge_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_word_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_last_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x1e, 0x0c, 0xfa, 0xe8, 0xd6, 0xc4, 0xb2, 0xa0,
        0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10,
    };
    static const uint8_t guard_before_seed[8] = {
        0xbe, 0xd0, 0xe2, 0xf4, 0xf6, 0x08, 0x1a, 0x2c,
    };
    static const uint8_t guard_after_seed[8] = {
        0x3d, 0x4f, 0x61, 0x73, 0x85, 0x97, 0xa9, 0xbb,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_adjacent_bridge_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        11U;
    const uintptr_t tail_last_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        14U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x4d5e6f708192a3b4ULL;
    const uint64_t refault_store_value = 0xdcedfe0f10213243ULL;
    const uint64_t aligned_upper_store_value_first = 0x6f8193a5b7c9dbedULL;
    const uint64_t aligned_upper_store_value_second = 0x0022446688aacceeULL;
    const uint64_t aligned_upper_store_value_third = 0x90a2b4c6d8eafc1eULL;
    const uint64_t aligned_upper_store_value_fourth = 0x21436587a9cbed0fULL;
    const uint64_t retry_same_template_store_value = 0xd5e6f708192a3b4cULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_adjacent_bridge_word = 0ULL;
    uint64_t expected_tail_last_halfword = 0ULL;
    uint64_t observed_tail_adjacent_bridge_word = 0ULL;
    uint64_t observed_tail_last_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_bridge_reset_env();
    ai_micro_mab_retry_high_bridge_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_bridge_prepare_supervisor_translation();
    ai_micro_mab_retry_high_bridge_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_bridge_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the tail-adjacent bridge word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the tail-adjacent bridge word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-tail-adjacent-bridge-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-tail-adjacent-bridge-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-tail-adjacent-bridge-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-tail-adjacent-bridge-word-and-upper-tail-last-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the tail-adjacent bridge word probe, and the final upper-tail-last-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing tail-adjacent bridge word probe and final upper-tail-last-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_adjacent_bridge_word =
        (uint64_t)expected_boundary[11] |
        ((uint64_t)expected_boundary[12] << 8U) |
        ((uint64_t)expected_boundary[13] << 16U) |
        ((uint64_t)expected_boundary[14] << 24U);
    expected_tail_last_halfword =
        (uint64_t)expected_boundary[14] |
        ((uint64_t)expected_boundary[15] << 8U);

    TEST_SETUP_EXCEPT();
    observed_tail_adjacent_bridge_word =
        ai_micro_mab_retry_high_bridge_translated_lwu(
            tail_adjacent_bridge_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a tail-adjacent bridge word translated load at boundary+11 should clear stale exception metadata and read the retried high-half bytes3-6 before the following upper-tail-last-halfword probe",
        excpt.triggered == false &&
        observed_tail_adjacent_bridge_word ==
            expected_tail_adjacent_bridge_word
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the tail-adjacent bridge word probe has already shown the retried high-half bytes3-6 visible, an upper-tail-last-halfword translated load at boundary+14 should still clear stale exception metadata and read the combined retried high-half bytes6-7",
        excpt.triggered == false &&
        observed_tail_last_halfword == expected_tail_last_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bridge_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the tail-adjacent bridge word probe, and the final upper-tail-last-halfword load sequence",
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_bridge_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xfb, 0xe9, 0xd7, 0xc5, 0xb3, 0xa1, 0x8f, 0x7d,
        0x6b, 0x59, 0x47, 0x35, 0x23, 0x11, 0xff, 0xed,
    };
    static const uint8_t guard_before_seed[8] = {
        0x9c, 0xae, 0xc0, 0xd2, 0xd4, 0xe6, 0xf8, 0x0a,
    };
    static const uint8_t guard_after_seed[8] = {
        0x1b, 0x2d, 0x3f, 0x51, 0x63, 0x75, 0x87, 0x99,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t first_upper_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t upper_tail_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t aligned_upper_store_vaddr = first_upper_word_load_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2b3c4d5e6f708192ULL;
    const uint64_t refault_store_value = 0xbacbdcedfe0f1021ULL;
    const uint64_t aligned_upper_store_value_first = 0x4d5f718395a7b9cbULL;
    const uint64_t aligned_upper_store_value_second = 0xde10224456687a8cULL;
    const uint64_t aligned_upper_store_value_third = 0x6e8092a4b6c8daecULL;
    const uint64_t aligned_upper_store_value_fourth = 0xff1133557799bbcdULL;
    const uint64_t retry_same_template_store_value = 0xb3c4d5e6f708192aULL;
    uint8_t expected_boundary[16];
    uint64_t expected_first_upper_word = 0ULL;
    uint64_t expected_upper_tail_halfword = 0ULL;
    uint64_t observed_first_upper_word = 0ULL;
    uint64_t observed_upper_tail_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_bridge_reset_env();
    ai_micro_mab_retry_high_bridge_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_bridge_prepare_supervisor_translation();
    ai_micro_mab_retry_high_bridge_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_bridge_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the first-upper-word probe, and the final upper-tail-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the first-upper-word probe, and the final upper-tail-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-first-upper-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-first-upper-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-first-upper-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the first-upper-word probe, and the final upper-tail-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing first-upper-word probe and final upper-tail-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_first_upper_word =
        (uint64_t)expected_boundary[8] |
        ((uint64_t)expected_boundary[9] << 8U) |
        ((uint64_t)expected_boundary[10] << 16U) |
        ((uint64_t)expected_boundary[11] << 24U);
    expected_upper_tail_halfword =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U);

    TEST_SETUP_EXCEPT();
    observed_first_upper_word =
        ai_micro_mab_retry_high_bridge_translated_lwu(
            first_upper_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a first-upper-word translated load at boundary+8 should clear stale exception metadata and read the retried high-half bytes0-3 before the following upper-tail-halfword probe",
        excpt.triggered == false &&
        observed_first_upper_word == expected_first_upper_word
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            upper_tail_halfword_load_vaddr);
    TEST_ASSERT("after the first-upper-word probe has already shown the retried high-half bytes0-3 visible, an upper-tail-halfword translated load at boundary+12 should still clear stale exception metadata and read the combined retried high-half bytes4-5",
        excpt.triggered == false &&
        observed_upper_tail_halfword == expected_upper_tail_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bridge_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the first-upper-word probe, and the final upper-tail-halfword load sequence",
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_bridge_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xea, 0xd8, 0xc6, 0xb4, 0xa2, 0x90, 0x7e, 0x6c,
        0x5a, 0x48, 0x36, 0x24, 0x12, 0x00, 0xee, 0xdc,
    };
    static const uint8_t guard_before_seed[8] = {
        0x8b, 0x9d, 0xaf, 0xc1, 0xc3, 0xd5, 0xe7, 0xf9,
    };
    static const uint8_t guard_after_seed[8] = {
        0x0a, 0x1c, 0x2e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t middle_tail_bridge_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        10U;
    const uintptr_t upper_tail_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_store_value = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_first = 0x3c4e60728496a8baULL;
    const uint64_t aligned_upper_store_value_second = 0xcddff1031527394bULL;
    const uint64_t aligned_upper_store_value_third = 0x5d6f8193a5b7c9dbULL;
    const uint64_t aligned_upper_store_value_fourth = 0xee00122436586a7cULL;
    const uint64_t retry_same_template_store_value = 0xa2b3c4d5e6f70819ULL;
    uint8_t expected_boundary[16];
    uint64_t expected_middle_tail_bridge_word = 0ULL;
    uint64_t expected_upper_tail_halfword = 0ULL;
    uint64_t observed_middle_tail_bridge_word = 0ULL;
    uint64_t observed_upper_tail_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_bridge_reset_env();
    ai_micro_mab_retry_high_bridge_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_bridge_prepare_supervisor_translation();
    ai_micro_mab_retry_high_bridge_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_bridge_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the middle-tail bridge word probe, and the final upper-tail-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the middle-tail bridge word probe, and the final upper-tail-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-middle-tail-bridge-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the middle-tail bridge word probe, and the final upper-tail-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing middle-tail bridge word probe and final upper-tail-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_middle_tail_bridge_word =
        (uint64_t)expected_boundary[10] |
        ((uint64_t)expected_boundary[11] << 8U) |
        ((uint64_t)expected_boundary[12] << 16U) |
        ((uint64_t)expected_boundary[13] << 24U);
    expected_upper_tail_halfword =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U);

    TEST_SETUP_EXCEPT();
    observed_middle_tail_bridge_word =
        ai_micro_mab_retry_high_bridge_translated_lwu(
            middle_tail_bridge_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a middle-tail bridge word translated load at boundary+10 should clear stale exception metadata and read the retried high-half bytes2-5 before the following upper-tail-halfword probe",
        excpt.triggered == false &&
        observed_middle_tail_bridge_word == expected_middle_tail_bridge_word
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            upper_tail_halfword_load_vaddr);
    TEST_ASSERT("after the middle-tail bridge word probe has already shown the retried high-half bytes2-5 visible, an upper-tail-halfword translated load at boundary+12 should still clear stale exception metadata and read the combined retried high-half bytes4-5",
        excpt.triggered == false &&
        observed_upper_tail_halfword == expected_upper_tail_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bridge_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the middle-tail bridge word probe, and the final upper-tail-halfword load sequence",
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_bridge_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_halfword_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x1e, 0x0c, 0xfa, 0xe8, 0xd6, 0xc4, 0xb2, 0xa0,
        0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10,
    };
    static const uint8_t guard_before_seed[8] = {
        0xbe, 0xd0, 0xe2, 0xf4, 0xf6, 0x08, 0x1a, 0x2c,
    };
    static const uint8_t guard_after_seed[8] = {
        0x3d, 0x4f, 0x61, 0x73, 0x85, 0x97, 0xa9, 0xbb,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t tail_adjacent_bridge_word_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        11U;
    const uintptr_t upper_tail_halfword_load_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        12U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_HIGH_BRIDGE_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x4d5e6f708192a3b4ULL;
    const uint64_t refault_store_value = 0xdcedfe0f10213243ULL;
    const uint64_t aligned_upper_store_value_first = 0x6f8193a5b7c9dbedULL;
    const uint64_t aligned_upper_store_value_second = 0x0022446688aacceeULL;
    const uint64_t aligned_upper_store_value_third = 0x90a2b4c6d8eafc1eULL;
    const uint64_t aligned_upper_store_value_fourth = 0x21436587a9cbed0fULL;
    const uint64_t retry_same_template_store_value = 0xd5e6f708192a3b4cULL;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_adjacent_bridge_word = 0ULL;
    uint64_t expected_upper_tail_halfword = 0ULL;
    uint64_t observed_tail_adjacent_bridge_word = 0ULL;
    uint64_t observed_upper_tail_halfword = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_retry_high_bridge_reset_env();
    ai_micro_mab_retry_high_bridge_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_high_bridge_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);
    ai_micro_mab_retry_high_bridge_prepare_supervisor_translation();
    ai_micro_mab_retry_high_bridge_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_high_bridge_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the prolonged upper-half producer, refault, retried same-template success store, the tail-adjacent bridge word probe, and the final upper-tail-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the prolonged upper-half producer, refault, retried same-template success store, the tail-adjacent bridge word probe, and the final upper-tail-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete without keeping stale exception metadata from the earlier MAB fault episode before the refault-followed-by-retried-same-template-store-then-tail-adjacent-bridge-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-tail-adjacent-bridge-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refault-followed-by-retried-same-template-store-then-tail-adjacent-bridge-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refault-followed-by-retried-same-template-store-then-tail-adjacent-bridge-word-and-upper-tail-halfword probe chain",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_HIGH_BRIDGE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the retried same-template success store, the tail-adjacent bridge word probe, and the final upper-tail-halfword probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_high_bridge_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_high_bridge_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata from that refault instead of inheriting the prior store access fault before the trailing tail-adjacent bridge word probe and final upper-tail-halfword probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_high_bridge_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    expected_tail_adjacent_bridge_word =
        (uint64_t)expected_boundary[11] |
        ((uint64_t)expected_boundary[12] << 8U) |
        ((uint64_t)expected_boundary[13] << 16U) |
        ((uint64_t)expected_boundary[14] << 24U);
    expected_upper_tail_halfword =
        (uint64_t)expected_boundary[12] |
        ((uint64_t)expected_boundary[13] << 8U);

    TEST_SETUP_EXCEPT();
    observed_tail_adjacent_bridge_word =
        ai_micro_mab_retry_high_bridge_translated_lwu(
            tail_adjacent_bridge_word_load_vaddr);
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a tail-adjacent bridge word translated load at boundary+11 should clear stale exception metadata and read the retried high-half bytes3-6 before the following upper-tail-halfword probe",
        excpt.triggered == false &&
        observed_tail_adjacent_bridge_word ==
            expected_tail_adjacent_bridge_word
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_halfword =
        ai_micro_mab_retry_high_bridge_translated_lhu(
            upper_tail_halfword_load_vaddr);
    TEST_ASSERT("after the tail-adjacent bridge word probe has already shown the retried high-half bytes3-6 visible, an upper-tail-halfword translated load at boundary+12 should still clear stale exception metadata and read the combined retried high-half bytes4-5",
        excpt.triggered == false &&
        observed_upper_tail_halfword == expected_upper_tail_halfword
    );

    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store",
        ai_micro_mab_retry_high_bridge_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the tail-adjacent bridge word probe, and the final upper-tail-halfword load sequence",
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_high_bridge_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_high_bridge_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_halfword_corner");
}
