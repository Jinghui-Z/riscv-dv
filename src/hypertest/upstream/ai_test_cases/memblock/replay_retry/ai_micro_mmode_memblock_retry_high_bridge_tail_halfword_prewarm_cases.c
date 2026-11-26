#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/*
 * Keep the boundary+14 halfword prewarm follow-up in a dedicated file so we
 * can isolate whether a successful 2-byte tail-window consumer remains valid
 * after a later word probe that otherwise leads to a final boundary+14
 * halfword timeout.
 */

#define AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_CROSS_16B_SD_OFFSET 15U

static inline pte_t *
ai_micro_mab_retry_high_tail_halfword_prewarm_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void
ai_micro_mab_retry_high_tail_halfword_prewarm_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_high_tail_halfword_prewarm_hs_leaf_entry(page) =
        (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_high_tail_halfword_prewarm_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_high_tail_halfword_prewarm_set_leaf_paddr(
        page, phys_page_base(page), flags);
}

static inline void
ai_micro_mab_retry_high_tail_halfword_prewarm_prepare_supervisor_translation(
    void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void
ai_micro_mab_retry_high_tail_halfword_prewarm_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_PMP_TOR_RWX);
}

static inline void
ai_micro_mab_retry_high_tail_halfword_prewarm_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void
ai_micro_mab_retry_high_tail_halfword_prewarm_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_PMP_TOR_RWX |
            (deny_cfg << 8) |
            (AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_PMP_TOR_RWX
                << 16));
}

static inline void ai_micro_mab_retry_high_tail_halfword_prewarm_reset_env(void)
{
    ai_micro_mab_retry_high_tail_halfword_prewarm_restore_pmp_full_access();
    ai_micro_mab_retry_high_tail_halfword_prewarm_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_high_tail_halfword_prewarm_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_high_tail_halfword_prewarm_bytes_match(
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
ai_micro_mab_retry_high_tail_halfword_prewarm_overlay_little_endian_bytes(
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

static inline uint64_t
ai_micro_mab_retry_high_tail_halfword_prewarm_read_little_endian(
    const uint8_t *bytes, unsigned start_byte, unsigned size)
{
    uint64_t value = 0ULL;
    for (unsigned i = 0; i < size; ++i) {
        value |= (uint64_t)bytes[start_byte + i] << (8U * i);
    }
    return value;
}

#define AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_TRANSLATED_STORE_INSTRUCTION( \
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

AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_sd, "sd",
    uint64_t)

#define AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_TRANSLATED_LOAD_INSTRUCTION( \
    name, inst) \
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

AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_lhu, "lhu")
AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_lwu, "lwu")

#define AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_CASE_BODY( \
    prewarm_halfword_offset, word_probe_offset, prewarm_label, \
    word_probe_label) \
    static const uint8_t boundary_seed[16] = { \
        0xfb, 0xe9, 0xd7, 0xc5, 0xb3, 0xa1, 0x8f, 0x7d, \
        0x6b, 0x59, 0x47, 0x35, 0x23, 0x11, 0xff, 0xed, \
    }; \
    static const uint8_t guard_before_seed[8] = { \
        0x9c, 0xae, 0xc0, 0xd2, 0xd4, 0xe6, 0xf8, 0x0a, \
    }; \
    static const uint8_t guard_after_seed[8] = { \
        0x1b, 0x2d, 0x3f, 0x51, 0x63, 0x75, 0x87, 0x99, \
    }; \
    const enum test_page low_page = SWITCH1; \
    const enum test_page high_page = SWITCH2; \
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U; \
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U; \
    const uintptr_t boundary_phys = page_phys + \
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_BOUNDARY_BASE_OFFSET; \
    const uintptr_t guard_before_phys = page_phys; \
    const uintptr_t guard_after_phys = page_phys + 24U; \
    const uintptr_t fault_vaddr = page_vaddr + \
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_CROSS_16B_SD_OFFSET; \
    const uintptr_t second_half_vaddr = page_vaddr + 16U; \
    const uintptr_t prewarm_halfword_load_vaddr = page_vaddr + \
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_BOUNDARY_BASE_OFFSET + \
        (prewarm_halfword_offset); \
    const uintptr_t word_probe_load_vaddr = page_vaddr + \
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_BOUNDARY_BASE_OFFSET + \
        (word_probe_offset); \
    const uintptr_t upper_tail_last_halfword_load_vaddr = page_vaddr + \
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_BOUNDARY_BASE_OFFSET + \
        14U; \
    const uintptr_t aligned_upper_store_vaddr = page_vaddr + \
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_BOUNDARY_BASE_OFFSET + \
        8U; \
    const uintptr_t deny_start = boundary_phys + 8U; \
    const uintptr_t deny_end = deny_start + PAGE_SIZE; \
    const uint64_t first_fault_store_value = 0x2b3c4d5e6f708192ULL; \
    const uint64_t refault_store_value = 0xbacbdcedfe0f1021ULL; \
    const uint64_t aligned_upper_store_value_first = 0x4d5f718395a7b9cbULL; \
    const uint64_t aligned_upper_store_value_second = 0xde10224456687a8cULL; \
    const uint64_t aligned_upper_store_value_third = 0x6e8092a4b6c8daecULL; \
    const uint64_t aligned_upper_store_value_fourth = 0xff1133557799bbcdULL; \
    const uint64_t retry_same_template_store_value = 0xb3c4d5e6f708192aULL; \
    uint8_t expected_boundary[16]; \
    uint64_t expected_prewarm_halfword = 0ULL; \
    uint64_t expected_word_probe = 0ULL; \
    uint64_t expected_upper_tail_last_halfword = 0ULL; \
    uint64_t observed_prewarm_halfword = 0ULL; \
    uint64_t observed_word_probe = 0ULL; \
    uint64_t observed_upper_tail_last_halfword = 0ULL; \
    goto_priv(PRIV_M); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_reset_env(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_write_bytes( \
        boundary_phys, boundary_seed, 16U); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_write_bytes( \
        guard_before_phys, guard_before_seed, 8U); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_write_bytes( \
        guard_after_phys, guard_after_seed, 8U); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_configure_pmp_window( \
        deny_start, deny_end, \
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_PMP_TOR_NO_W); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_prepare_supervisor_translation(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_set_leaf( \
        low_page, PTE_V | PTE_AD | PTE_RWX); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_set_leaf( \
        high_page, PTE_V | PTE_AD | PTE_RWX); \
    sfence_vma(); \
    TEST_SETUP_EXCEPT(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_sd( \
        fault_vaddr, first_fault_store_value); \
    TEST_ASSERT("the first second-split write fault must report store access fault before the configured halfword-prewarm to word-probe follow-up sequence", \
        excpt.triggered == true && \
        excpt.cause == CAUSE_SAF && \
        excpt.tval == second_half_vaddr); \
    TEST_SETUP_EXCEPT(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_sd( \
        fault_vaddr, first_fault_store_value); \
    TEST_ASSERT("repeating the same second-split write fault must keep precise store access fault metadata before the configured halfword-prewarm to word-probe follow-up sequence", \
        excpt.triggered == true && \
        excpt.cause == CAUSE_SAF && \
        excpt.tval == second_half_vaddr); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_restore_pmp_full_access(); \
    TEST_SETUP_EXCEPT(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_sd( \
        aligned_upper_store_vaddr, aligned_upper_store_value_first); \
    TEST_ASSERT("after repair, the first upper-half aligned translated store should complete without stale exception metadata before the configured halfword-prewarm to word-probe follow-up sequence", \
        excpt.triggered == false); \
    TEST_SETUP_EXCEPT(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_sd( \
        aligned_upper_store_vaddr, aligned_upper_store_value_second); \
    TEST_ASSERT("after one upper-half aligned translated store has already completed, the second upper-half aligned store should stay trap-free before the configured halfword-prewarm to word-probe follow-up sequence", \
        excpt.triggered == false); \
    TEST_SETUP_EXCEPT(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_sd( \
        aligned_upper_store_vaddr, aligned_upper_store_value_third); \
    TEST_ASSERT("after two upper-half aligned translated stores have already completed, the third upper-half aligned store should stay trap-free before the configured halfword-prewarm to word-probe follow-up sequence", \
        excpt.triggered == false); \
    TEST_SETUP_EXCEPT(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_sd( \
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth); \
    TEST_ASSERT("after three upper-half aligned translated stores have already completed, the fourth upper-half aligned store should stay trap-free and become the latest committed high-half payload before the configured halfword-prewarm to word-probe follow-up sequence", \
        excpt.triggered == false); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_configure_pmp_window( \
        deny_start, deny_end, \
        AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_PMP_TOR_NO_W); \
    TEST_SETUP_EXCEPT(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_sd( \
        fault_vaddr, refault_store_value); \
    TEST_ASSERT("after four upper-half aligned stores have already completed, restoring the deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the configured halfword-prewarm to word-probe follow-up sequence", \
        excpt.triggered == true && \
        excpt.cause == CAUSE_SAF && \
        excpt.tval == second_half_vaddr); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_restore_pmp_full_access(); \
    TEST_SETUP_EXCEPT(); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_translated_sd( \
        fault_vaddr, retry_same_template_store_value); \
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata before the configured halfword-prewarm to word-probe follow-up sequence", \
        excpt.triggered == false); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_overlay_little_endian_bytes( \
        expected_boundary, boundary_seed, 16U, 8U, \
        aligned_upper_store_value_fourth, sizeof(uint64_t)); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_overlay_little_endian_bytes( \
        expected_boundary, expected_boundary, 16U, 7U, \
        retry_same_template_store_value, sizeof(uint64_t)); \
    expected_prewarm_halfword = \
        ai_micro_mab_retry_high_tail_halfword_prewarm_read_little_endian( \
            expected_boundary, (prewarm_halfword_offset), 2U); \
    expected_word_probe = \
        ai_micro_mab_retry_high_tail_halfword_prewarm_read_little_endian( \
            expected_boundary, (word_probe_offset), 4U); \
    expected_upper_tail_last_halfword = \
        ai_micro_mab_retry_high_tail_halfword_prewarm_read_little_endian( \
            expected_boundary, 14U, 2U); \
    TEST_SETUP_EXCEPT(); \
    observed_prewarm_halfword = \
        ai_micro_mab_retry_high_tail_halfword_prewarm_translated_lhu( \
            prewarm_halfword_load_vaddr); \
    TEST_ASSERT("after the prolonged upper-half producer continuation, repaired refault, and retried same-template success store, a " prewarm_label " translated halfword load should clear stale exception metadata and read the selected retried high-half bytes before the following " word_probe_label " probe", \
        excpt.triggered == false && \
        observed_prewarm_halfword == expected_prewarm_halfword); \
    TEST_SETUP_EXCEPT(); \
    observed_word_probe = \
        ai_micro_mab_retry_high_tail_halfword_prewarm_translated_lwu( \
            word_probe_load_vaddr); \
    TEST_ASSERT("after the " prewarm_label " prewarm probe has already shown the selected retried high-half bytes visible, a " word_probe_label " translated word load should still clear stale exception metadata and read the selected retried high-half bytes before the final upper-tail-last-halfword probe", \
        excpt.triggered == false && \
        observed_word_probe == expected_word_probe); \
    TEST_SETUP_EXCEPT(); \
    observed_upper_tail_last_halfword = \
        ai_micro_mab_retry_high_tail_halfword_prewarm_translated_lhu( \
            upper_tail_last_halfword_load_vaddr); \
    TEST_ASSERT("after the " prewarm_label " prewarm probe and " word_probe_label " probe have already shown the selected retried high-half bytes visible, an upper-tail-last-halfword translated load at boundary+14 should now clear stale exception metadata and read the combined retried high-half bytes6-7", \
        excpt.triggered == false && \
        observed_upper_tail_last_halfword == \
            expected_upper_tail_last_halfword); \
    TEST_ASSERT("after the prolonged upper-half producer continuation, refault, and retried same-template success store, the final boundary bytes should match the mixed overlay of the latest upper-half payload and the retried 1B+7B store", \
        ai_micro_mab_retry_high_tail_halfword_prewarm_bytes_match( \
            boundary_phys, expected_boundary, 16U)); \
    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried same-template success store, the configured halfword prewarm probe, the configured word probe, and the final upper-tail-last-halfword load sequence", \
        ai_micro_mab_retry_high_tail_halfword_prewarm_bytes_match( \
            guard_before_phys, guard_before_seed, 8U) && \
        ai_micro_mab_retry_high_tail_halfword_prewarm_bytes_match( \
            guard_after_phys, guard_after_seed, 8U)); \
    ai_micro_mab_retry_high_tail_halfword_prewarm_reset_env();

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_upper_tail_last_halfword_corner()
{
    TEST_START();
    AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_CASE_BODY(
        12U, 8U, "upper-tail-halfword", "first-upper-word");
    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_bridge_halfword_then_first_upper_word_then_upper_tail_last_halfword_corner()
{
    TEST_START();
    AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_CASE_BODY(
        13U, 8U, "upper-tail-bridge-halfword", "first-upper-word");
    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_bridge_halfword_then_first_upper_word_then_upper_tail_last_halfword_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner()
{
    TEST_START();
    AI_MICRO_MAB_RETRY_HIGH_TAIL_HALFWORD_PREWARM_CASE_BODY(
        12U, 10U, "upper-tail-halfword", "middle-tail-bridge-word");
    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner");
}
