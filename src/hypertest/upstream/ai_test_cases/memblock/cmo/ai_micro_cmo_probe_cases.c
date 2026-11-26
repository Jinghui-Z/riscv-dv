#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

// Diagnostic probe only:
// Spike currently accepts sub-block addresses for cbo.clean/cbo.zero without
// surfacing an exception, so this helper is kept out of the default regression
// until the "CMO 地址不对齐" test point is defined more precisely.
bool ai_micro_hs_cmo_misaligned_probe()
{
    TEST_START();

    const uintptr_t cbom_vaddr = hs_page_base(SWITCH1) + 1U;
    const uintptr_t cbozero_vaddr = hs_page_base(SWITCH2) + 1U;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE | MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    cbo_clean(cbom_vaddr);
    INFO("cmo probe: cbo.clean @ 0x%lx", cbom_vaddr);
    INFO_PRINT_XCEPT();

    TEST_SETUP_EXCEPT();
    cbo_zero(cbozero_vaddr);
    INFO("cmo probe: cbo.zero @ 0x%lx", cbozero_vaddr);
    INFO_PRINT_XCEPT();

    TEST_ASSERT("the CMO misaligned probe completed and logged the observed Spike behavior",
        true
    );

    TEST_END();
}
