#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep forced-MAB same-page follow-up store probes separate so the retry-high
 * consumer files stay readable.
 */

#define AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_FOLLOWUP_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_FOLLOWUP_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_retry_followup_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_followup_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_followup_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_followup_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_followup_set_leaf_paddr(page, phys_page_base(page),
        flags);
}

static inline void ai_micro_mab_retry_followup_prepare_supervisor_translation(
    void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_retry_followup_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_RWX);
}

static inline void ai_micro_mab_retry_followup_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_retry_followup_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_RWX |
        (deny_cfg << 8) | (AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_followup_reset_env(void)
{
    ai_micro_mab_retry_followup_restore_pmp_full_access();
    ai_micro_mab_retry_followup_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_followup_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_followup_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_mab_retry_followup_overlay_little_endian_bytes(
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

#define AI_MICRO_MAB_RETRY_FOLLOWUP_TRANSLATED_STORE_INSTRUCTION( \
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

AI_MICRO_MAB_RETRY_FOLLOWUP_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_followup_translated_sd, "sd", uint64_t)
AI_MICRO_MAB_RETRY_FOLLOWUP_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_followup_translated_sh, "sh", uint16_t)
AI_MICRO_MAB_RETRY_FOLLOWUP_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_followup_translated_sw, "sw", uint32_t)

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_template_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x91, 0x7f, 0x6d, 0x5b, 0x49, 0x37, 0x25, 0x13,
        0x02, 0xf0, 0xde, 0xcc, 0xba, 0xa8, 0x96, 0x84,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1d, 0x2f, 0x41, 0x53, 0x65, 0x77, 0x89, 0x9b,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc4, 0xd6, 0xe8, 0xfa, 0x0c, 0x1e, 0x20, 0x32,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x31425364758697a8ULL;
    const uint64_t refault_store_value = 0xc5d6e7f8091a2b3cULL;
    const uint64_t aligned_upper_store_value_first = 0x1021324354657687ULL;
    const uint64_t aligned_upper_store_value_second = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_third = 0x2132435465768798ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xa9bacbdcedfe1021ULL;
    const uint64_t retry_same_template_store_value = 0x425364758697a8b9ULL;
    const uint64_t followup_same_template_store_value = 0xd4e5f60718293a4bULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_followup_reset_env();
    ai_micro_mab_retry_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_retry_followup_prepare_supervisor_translation();
    ai_micro_mab_retry_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the retried-success then same-template-follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the retried-success then same-template-follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-store then same-template follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-store then same-template follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-store then same-template follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-store then same-template follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the same-template follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata before the immediate same-template follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, followup_same_template_store_value);
    TEST_ASSERT("after the retried original-template 1B+7B store has already succeeded on the same boundary, the immediate same-address same-template translated store should also stay trap-free instead of inheriting fake-crosspage or stale split-store state",
        excpt.triggered == false
    );

    ai_micro_mab_retry_followup_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_followup_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_followup_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        followup_same_template_store_value, sizeof(uint64_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried original-template success store, and immediate same-template follow-up store, the final boundary bytes should match only the latest same-template overlay plus the latest high-half residue",
        ai_micro_mab_retry_followup_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried success store, and immediate same-template follow-up store sequence",
        ai_micro_mab_retry_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_template_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_address_width_switch_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x84, 0x72, 0x60, 0x4e, 0x3c, 0x2a, 0x18, 0x06,
        0xf5, 0xe3, 0xd1, 0xcf, 0xbd, 0xab, 0x99, 0x87,
    };
    static const uint8_t guard_before_seed[8] = {
        0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x70, 0x82, 0x94,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd3, 0xe5, 0xf7, 0x09, 0x1b, 0x2d, 0x3f, 0x41,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5364758697a8b9caULL;
    const uint64_t refault_store_value = 0xe7f8091a2b3c4d5eULL;
    const uint64_t aligned_upper_store_value_first = 0x2435465768798a9bULL;
    const uint64_t aligned_upper_store_value_second = 0xbdcedff001122334ULL;
    const uint64_t aligned_upper_store_value_third = 0x35465768798a9bacULL;
    const uint64_t aligned_upper_store_value_fourth = 0xcedff00112233445ULL;
    const uint64_t retry_same_template_store_value = 0x64758697a8b9cadbULL;
    const uint32_t followup_width_switch_store_value = 0x7f8e9dacU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_followup_reset_env();
    ai_micro_mab_retry_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_retry_followup_prepare_supervisor_translation();
    ai_micro_mab_retry_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the retried-success then width-switch follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the retried-success then width-switch follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-store then width-switch follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-store then width-switch follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-store then width-switch follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-store then width-switch follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the width-switch follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata before the immediate width-switch follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sw(
        fault_vaddr, followup_width_switch_store_value);
    TEST_ASSERT("after the retried original-template 1B+7B store has already succeeded on the same boundary, the immediate same-address width-switched translated store should also stay trap-free and refresh the active split template instead of inheriting the older fake-crosspage shape",
        excpt.triggered == false
    );

    ai_micro_mab_retry_followup_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_followup_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_followup_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        (uint64_t)followup_width_switch_store_value, sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried original-template success store, and immediate same-address width-switch store, the final boundary bytes should preserve the retried tail bytes while refreshing only the current narrow overlay",
        ai_micro_mab_retry_followup_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried success store, and immediate width-switch follow-up store sequence",
        ai_micro_mab_retry_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_address_width_switch_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_address_halfword_width_switch_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x73, 0x61, 0x4f, 0x3d, 0x2b, 0x19, 0x07, 0xf5,
        0xe4, 0xd2, 0xc0, 0xae, 0x9c, 0x8a, 0x78, 0x66,
    };
    static const uint8_t guard_before_seed[8] = {
        0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd1, 0xe3, 0xf5, 0x07, 0x19, 0x2b, 0x3d, 0x4f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_FOLLOWUP_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_FOLLOWUP_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr =
        page_vaddr + AI_MICRO_MAB_RETRY_FOLLOWUP_BOUNDARY_BASE_OFFSET +
        8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x445566778899aabbULL;
    const uint64_t refault_store_value = 0xccddeeff10213243ULL;
    const uint64_t aligned_upper_store_value_first = 0x31425364758697a8ULL;
    const uint64_t aligned_upper_store_value_second = 0xb9cadbecfd0e1f20ULL;
    const uint64_t aligned_upper_store_value_third = 0x5263748596a7b8c9ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xdbecfd0e1f203142ULL;
    const uint64_t retry_same_template_store_value = 0x6a7b8c9daebfd0e1ULL;
    const uint16_t followup_halfword_width_switch_store_value = 0x93a4U;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_followup_reset_env();
    ai_micro_mab_retry_followup_write_bytes(boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_followup_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_followup_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_NO_W);
    ai_micro_mab_retry_followup_prepare_supervisor_translation();
    ai_micro_mab_retry_followup_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_followup_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the retried-success then halfword width-switch follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the retried-success then halfword width-switch follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-store then halfword width-switch follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-store then halfword width-switch follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-store then halfword width-switch follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-store then halfword width-switch follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_followup_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_FOLLOWUP_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the halfword width-switch follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_followup_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata before the immediate halfword width-switch follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_followup_translated_sh(
        fault_vaddr, followup_halfword_width_switch_store_value);
    TEST_ASSERT("after the retried original-template 1B+7B store has already succeeded on the same boundary, the immediate same-address halfword width-switched translated store should also stay trap-free and refresh only the current bridge-halfword template instead of inheriting the older fake-crosspage split shape",
        excpt.triggered == false
    );

    ai_micro_mab_retry_followup_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_followup_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_followup_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        (uint64_t)followup_halfword_width_switch_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried original-template success store, and immediate same-address halfword width-switch store, the final boundary bytes should preserve the retried tail bytes while refreshing only the current 1B+1B bridge overlay",
        ai_micro_mab_retry_followup_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried success store, and immediate halfword width-switch follow-up store sequence",
        ai_micro_mab_retry_followup_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_followup_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_followup_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_address_halfword_width_switch_store_corner");
}
