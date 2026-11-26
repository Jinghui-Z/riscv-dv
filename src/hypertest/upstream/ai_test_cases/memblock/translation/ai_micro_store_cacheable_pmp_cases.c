#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_STORE_PMP_PAGE VSRWX_GRWX

#define AI_MICRO_STORE_PMP_TOR_RWX  0x0fULL
#define AI_MICRO_STORE_PMP_TOR_NO_W 0x0dULL

static inline void ai_micro_store_pmp_restore_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_MICRO_STORE_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_micro_store_pmp_configure_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_STORE_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_STORE_PMP_TOR_RWX << 16));
}

bool ai_micro_hs_store_pmp_allow_data_visible()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_STORE_PMP_PAGE);
    const uintptr_t paddr = phys_page_base(AI_MICRO_STORE_PMP_PAGE);
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t stored_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    ai_micro_store_pmp_restore_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("an aligned HS cacheable store completes normally when PMP allows the translated write",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the stored payload is immediately readable through the same HS cacheable alias when PMP allows the write",
        observed == stored_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMP-allowed HS cacheable store updates backing memory with the expected payload",
        read64(paddr) == stored_value
    );

    TEST_END();
}

bool ai_micro_hs_store_pmp_write_access_fault()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_STORE_PMP_PAGE);
    const uintptr_t paddr = phys_page_base(AI_MICRO_STORE_PMP_PAGE);
    const uint64_t initial_value = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    ai_micro_store_pmp_restore_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_store_pmp_configure_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_STORE_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("an aligned HS cacheable store reports store access fault when PMP denies the translated write",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied HS cacheable store leaves backing memory unchanged when PMP blocks the write",
        read64(paddr) == initial_value
    );
    ai_micro_store_pmp_restore_full_access();

    TEST_END();
}
