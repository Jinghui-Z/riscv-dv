#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_CROSS16_SCALAR_LOW_BYTES 1U
#define AI_MICRO_CROSS16_SCALAR_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_CROSS16_SCALAR_PMP_TOR_NO_R 0x0eULL
#define AI_MICRO_CROSS16_SCALAR_PMP_TOR_NO_W 0x0dULL

static inline uintptr_t ai_micro_cross16_scalar_addr(enum test_page page)
{
    return hs_page_base(page) + PAGE_SIZE - AI_MICRO_CROSS16_SCALAR_LOW_BYTES;
}

static inline void ai_micro_cross16_scalar_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end,
    uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_CROSS16_SCALAR_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_CROSS16_SCALAR_PMP_TOR_RWX << 16));
}

static inline void ai_micro_cross16_scalar_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_CROSS16_SCALAR_PMP_TOR_RWX);
}

bool ai_micro_hs_load_cross_16b_samepage_second_split_pmp_access_fault_manual_rtl_only_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross16_scalar_addr(low_page);
    const uintptr_t second_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x7766554433221100ULL;
    const uint64_t high_pattern = 0xffeeddccbbaa9988ULL;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross16_scalar_restore_pmp_full_access();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_cross16_scalar_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_CROSS16_SCALAR_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT("a cross-page scalar ld starting at the low-page last byte reports load access fault on the high-page second split when PMP denies the upper page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-page scalar load second-split access fault leaves both boundary source doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross16_scalar_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_samepage_second_split_pmp_access_fault_no_side_effect_manual_rtl_only_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross16_scalar_addr(low_page);
    const uintptr_t second_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x7766554433221100ULL;
    const uint64_t high_pattern = 0xffeeddccbbaa9988ULL;
    const uint64_t store_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_micro_cross16_scalar_restore_pmp_full_access();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_cross16_scalar_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_CROSS16_SCALAR_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("a cross-page scalar sd starting at the low-page last byte reports store access fault on the high-page second split when PMP denies the upper page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-page scalar store second-split access fault does not partially update either boundary doubleword",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross16_scalar_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
