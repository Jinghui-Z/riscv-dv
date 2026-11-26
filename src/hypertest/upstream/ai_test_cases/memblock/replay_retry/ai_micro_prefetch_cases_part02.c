#include "ai_micro_prefetch_common.h"

bool ai_micro_hs_prefetch_bus_error_silent()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prefetch_prepare_bad_paddr_alias(
        AI_MICRO_PREFETCH_PAGE, AI_MICRO_PREFETCH_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a software prefetch to a translated alias backed by a high invalid physical page retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a real HS load to the same translated bad-physical alias still reports load access fault with the original virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_bus_error_repeated_silent_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1b8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1b8U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_PREFETCH_BAD_PADDR);
    const uint64_t expected = 0x2468ace013579bdfULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a first software prefetch to a translated bad-physical HS alias still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("repeating software prefetch to the same translated bad-physical HS alias also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("a later valid HS address still loads its expected payload after the repeated bus-error-class silent prefetch probes",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same bad-physical HS alias still reports load access fault on a real load after the repeated silent bus-error-class prefetch probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated bus-error-class silent prefetch probes leave the later valid target and adjacent backing-memory word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_bus_error_then_valid_access_recovers_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0xb8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0xb8U;
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_PREFETCH_BAD_PADDR);
    const uint64_t expected = 0x2468ace013579bdfULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a software prefetch to a translated bad-physical HS alias still retires silently before a later valid-address recovery probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later software prefetch to a valid HS address still retires silently after the earlier bus-error-class prefetch path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the valid HS address still loads its expected payload after the earlier bus-error-class silent prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same bad-physical HS alias still reports load access fault on a real load after the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_bus_error_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x168U;
    uintptr_t adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x168U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_PREFETCH_BAD_PADDR);
    const uint64_t expected = 0x3579bdf02468ace1ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a software prefetch to a translated bad-physical HS alias still retires silently before the adjacent-preservation recovery probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later software prefetch to a valid HS address still retires silently after the earlier bus-error-class prefetch path in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the valid HS address still loads its expected payload after the earlier bus-error-class silent prefetch path in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the adjacent HS word remains unchanged after the bus-error-class silent prefetch recovery path",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same bad-physical HS alias still reports load access fault on a real load after the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the bus-error-class silent prefetch adjacent-preservation recovery corner preserves both the valid target and adjacent backing-memory word",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_deny_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE);
    const uint64_t expected = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a software prefetch whose translated address fails the PMP read check retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same HS load still reports load access fault once the read is architecturally committed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("restoring PMP access lets the same address load the original payload again",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_deny_repeated_silent_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1c0U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1c0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x89abcdef01234567ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first HS software prefetch whose translated address fails the PMP read check still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("repeating the same HS software prefetch on the PMP-denied translated address also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same HS load still reports load access fault after the repeated silent PMP-deny prefetch probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent HS prefetch probes on the denied line preserve both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("restoring PMP access still lets the same HS address load the original payload again after the repeated silent deny path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_deny_then_restore_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x208U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x208U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x9abcdef001234567ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a software prefetch whose translated address fails the PMP read check still retires silently before the adjacent-preservation restore probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same HS load still reports load access fault before PMP read permission is restored in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied HS load leaves both the target and adjacent backing-memory words unchanged before PMP restore",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("restoring PMP access lets the same HS target word load the original payload again in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("restoring PMP access also leaves the adjacent HS word unchanged in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMP-deny silent prefetch restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_then_pmp_deny_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page target_page = ai_micro_prefetch_mshr_pages[page_count - 1];
    uintptr_t target_vaddr = hs_page_base(target_page) + 0x280U;
    uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    uintptr_t target_paddr = phys_page_base(target_page) + 0x280U;
    uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t seed = 0x6600770088009900ULL;
    const uint64_t expected = 0x9abcdef001234567ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(target_paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(vaddr);
        TEST_ASSERT("each HS probe page is readable before the MSHR-pressure-plus-PMP-deny sequence starts",
            excpt.triggered == false &&
            warmed == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed probe line can be invalidated before the MSHR-pressure-plus-PMP-deny sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the MSHR-pressure-plus-PMP-deny probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        target_paddr & PAGE_ADDR_MSK,
        (target_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("even with the miss machinery already under pressure, a software prefetch whose translated address later fails the PMP read check still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(target_vaddr);
    TEST_ASSERT("the same HS load still reports load access fault before PMP read permission is restored after the MSHR-pressure probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the MSHR-pressure-plus-PMP-deny probe leaves both the target and adjacent backing-memory words unchanged before restore",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored = ld(target_vaddr);
    TEST_ASSERT("restoring PMP access after the MSHR-pressure-plus-PMP-deny path lets the same HS target word load the original payload again",
        excpt.triggered == false &&
        restored == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("restoring PMP access after the MSHR-pressure-plus-PMP-deny path also leaves the adjacent HS word unchanged",
        excpt.triggered == false &&
        restored_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the MSHR-pressure-plus-PMP-deny restore corner preserves both the target and adjacent backing-memory words",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_mmio_silent()
{
    TEST_START();

    uintptr_t paddr = ai_micro_prefetch_io_backing_paddr();
    uintptr_t vaddr = ai_micro_prefetch_prepare_io_alias();
    const uint64_t expected = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a software prefetch routed through a PBMT=IO alias retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("the same PBMT=IO alias still loads the backing payload after the silent prefetch MMIO-route path",
        excpt.triggered == false &&
        observed == expected
    );

    ai_micro_prefetch_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_prefetch_mmio_repeated_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t paddr = ai_micro_prefetch_io_backing_paddr() + 0x80U;
    uintptr_t vaddr = ai_micro_prefetch_prepare_io_alias() + 0x80U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x123456789abcdef0ULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first software prefetch routed through a PBMT=IO alias retires silently in the repeated-adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("repeating the same software prefetch through the PBMT=IO alias also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("the same PBMT=IO alias still loads the backing payload after the repeated silent MMIO-route prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the adjacent PBMT=IO-backed word remains unchanged after the repeated silent MMIO-route prefetch path",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent PBMT=IO prefetch probes preserve both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_prefetch_pbmt_nc_repeated_adjacent_preserved_corner()
{
    TEST_START();

    static uint64_t backing[] __attribute__((aligned(64))) = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
    };
    uintptr_t paddr = (uintptr_t)&backing[0];
    uintptr_t adjacent_paddr = (uintptr_t)&backing[1];
    uintptr_t vaddr = 0;
    uintptr_t adjacent_vaddr = 0;
    const uint64_t expected = 0x1021324354657687ULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;
    const uint64_t updated = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    *ai_micro_prefetch_hs_leaf_entry(AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE) =
        (ai_micro_prefetch_alias_anchor(
            AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE,
            paddr & PAGE_ADDR_MSK) >> 2) |
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;
    sfence_vma();
    vaddr = hs_page_base(AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE) +
        (paddr & (PAGE_SIZE - 1));
    adjacent_vaddr = vaddr + sizeof(uint64_t);

    goto_priv(PRIV_HS);
    CSRW(CSR_SCAUSE, 0x5a5ULL);
    CSRW(CSR_STVAL, 0ULL);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first software prefetch.r routed through a PBMT=NC alias retires silently",
        excpt.triggered == false &&
        CSRR(CSR_SCAUSE) == 0x5a5ULL &&
        CSRR(CSR_STVAL) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("repeating prefetch.r through the same PBMT=NC alias also retires silently",
        excpt.triggered == false &&
        CSRR(CSR_SCAUSE) == 0x5a5ULL &&
        CSRR(CSR_STVAL) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("software prefetch.w through the same PBMT=NC alias is also silent",
        excpt.triggered == false &&
        CSRR(CSR_SCAUSE) == 0x5a5ULL &&
        CSRR(CSR_STVAL) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("the PBMT=NC alias still loads the original payload after silent prefetch.r/w probes",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, updated);
    TEST_ASSERT("a real PBMT=NC store still completes after silent prefetch.r/w probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT("the PBMT=NC alias reads back only the real store payload after the silent prefetch probes",
        excpt.triggered == false &&
        observed == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("silent PBMT=NC prefetch probes and the later real store preserve the adjacent word",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=NC prefetch probes do not create a visible writeback side effect in backing memory",
        read64(paddr) == updated &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_resp_ld_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE);
    const uint64_t expected = 0x2468ace013579bdfULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    TEST_ASSERT("an HS load first warms the translated cacheable line before the resp_ld software-prefetch probe",
        excpt.triggered == false &&
        warmed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a software prefetch remains silent when a hot HS translation later reaches the PMP resp_ld path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same hot HS address still reports load access fault on a real read after the silent resp_ld prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored = ld(vaddr);
    TEST_ASSERT("restoring PMP read permission makes the same hot HS line readable again",
        excpt.triggered == false &&
        restored == expected
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_resp_ld_repeated_silent_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x200U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x200U;
    const uint64_t expected = 0x2468ace013579bdfULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    TEST_ASSERT("an HS load first warms the translated cacheable line before the repeated resp_ld software-prefetch probes",
        excpt.triggered == false &&
        warmed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first HS software prefetch remains silent when a hot translation later reaches the PMP resp_ld path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("repeating the same HS software prefetch on the hot resp_ld line also remains silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same hot HS address still reports load access fault on a real read after the repeated silent resp_ld prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored = ld(vaddr);
    TEST_ASSERT("restoring PMP read permission still makes the same hot HS line readable again after the repeated resp_ld probes",
        excpt.triggered == false &&
        restored == expected
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_resp_ld_then_restore_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x280U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x280U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x2468ace013579bdfULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    TEST_ASSERT("an HS load first warms the translated cacheable line before the resp_ld adjacent-preservation restore probe",
        excpt.triggered == false &&
        warmed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a software prefetch remains silent when a hot HS translation later reaches the PMP resp_ld path in the adjacent-preservation restore corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same hot HS address still reports load access fault before PMP read permission is restored in the resp_ld adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied hot-HS load leaves both the target and adjacent backing-memory words unchanged before resp_ld restore",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored = ld(vaddr);
    TEST_ASSERT("restoring PMP read permission makes the same hot HS line readable again in the resp_ld adjacent-preservation corner",
        excpt.triggered == false &&
        restored == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("restoring PMP read permission also leaves the adjacent hot HS word unchanged in the resp_ld adjacent-preservation corner",
        excpt.triggered == false &&
        restored_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the resp_ld silent prefetch restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_resp_st_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE);
    const uint64_t initial = 0x1122446688aacceeULL;
    const uint64_t warmed = 0x99bbddff11335577ULL;
    const uint64_t denied = 0xf0e1d2c3b4a59687ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, warmed);
    TEST_ASSERT("an HS store first warms the translated cacheable line before the resp_st software-prefetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the warm-up HS store updates the backing memory before write permission is removed",
        read64(paddr) == warmed
    );
    write64(paddr, initial);
    ai_micro_prefetch_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("a software prefetch remains silent when a hot HS translation later reaches the PMP resp_st path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("the same hot HS address still reports store access fault on a real write after the silent resp_st prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied HS store leaves the backing memory unchanged after the silent resp_st prefetch path",
        read64(paddr) == initial
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("restoring PMP write permission lets the same hot HS line accept the store again",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the restored-permission HS store updates the backing memory with the denied payload",
        read64(paddr) == denied
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_resp_st_repeated_silent_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x240U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x240U;
    const uint64_t initial = 0x1122446688aacceeULL;
    const uint64_t warmed = 0x99bbddff11335577ULL;
    const uint64_t denied = 0xf0e1d2c3b4a59687ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, warmed);
    TEST_ASSERT("an HS store first warms the translated cacheable line before the repeated resp_st software-prefetch probes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the warm-up HS store updates the backing memory before write permission is removed for the repeated resp_st probes",
        read64(paddr) == warmed
    );
    write64(paddr, initial);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("a first HS software prefetch remains silent when a hot translation later reaches the PMP resp_st path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("repeating the same HS software prefetch on the hot resp_st line also remains silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("the same hot HS address still reports store access fault on a real write after the repeated silent resp_st prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied HS store still leaves the backing memory unchanged after the repeated silent resp_st probes",
        read64(paddr) == initial
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("restoring PMP write permission still lets the same hot HS line accept the store again after the repeated resp_st probes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the restored-permission HS store updates the backing memory with the denied payload after the repeated resp_st probes",
        read64(paddr) == denied
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_resp_st_then_restore_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x2c0U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x2c0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122446688aacceeULL;
    const uint64_t warmed = 0x99bbddff11335577ULL;
    const uint64_t denied = 0xf0e1d2c3b4a59687ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, warmed);
    TEST_ASSERT("an HS store first warms the translated cacheable line before the resp_st adjacent-preservation restore probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the warm-up HS store updates the backing memory before write permission is removed in the resp_st adjacent-preservation corner",
        read64(paddr) == warmed
    );
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("a software prefetch remains silent when a hot HS translation later reaches the PMP resp_st path in the adjacent-preservation restore corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("the same hot HS address still reports store access fault before PMP write permission is restored in the resp_st adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied hot-HS store leaves both the target and adjacent backing-memory words unchanged before resp_st restore",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("restoring PMP write permission lets the same hot HS line accept the store again in the resp_st adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the resp_st silent prefetch restore corner updates only the target word while preserving the adjacent backing-memory word",
        read64(paddr) == denied &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_then_pmp_resp_st_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page target_page = SWITCH2;
    uintptr_t target_vaddr = hs_page_base(target_page) + 0x300U;
    uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    uintptr_t target_paddr = phys_page_base(target_page) + 0x300U;
    uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t seed = 0x770088009900aa00ULL;
    const uint64_t initial = 0x1122446688aacceeULL;
    const uint64_t warmed = 0x99bbddff11335577ULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(target_paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, warmed);
    TEST_ASSERT("an HS store first warms the translated cacheable target line before the MSHR-pressure-plus-resp_st probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the warm-up HS store updates the backing memory before write permission is removed in the MSHR-pressure-plus-resp_st corner",
        read64(target_paddr) == warmed
    );
    write64(target_paddr, initial);

    goto_priv(PRIV_HS);
    for (size_t i = 0; i + 1 < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the MSHR-pressure-plus-resp_st sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the MSHR-pressure-plus-resp_st sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the MSHR-pressure-plus-resp_st probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        target_paddr & PAGE_ADDR_MSK,
        (target_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(target_vaddr);
    TEST_ASSERT("even with the miss machinery already under pressure, a software prefetch on a hot HS line that later reaches the PMP resp_st path still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, 0xf0e1d2c3b4a59687ULL);
    TEST_ASSERT("the same hot HS address still reports store access fault before PMP write permission is restored after the MSHR-pressure probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied hot-HS store leaves both the target and adjacent backing-memory words unchanged before resp_st restore under MSHR pressure",
        read64(target_paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(target_vaddr, stored);
    TEST_ASSERT("restoring PMP write permission after the MSHR-pressure-plus-resp_st path lets the same HS target word store succeed again",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ld(target_vaddr);
    TEST_ASSERT("after the recovered store path, the same HS target word reads back the newly written payload in the MSHR-pressure-plus-resp_st corner",
        excpt.triggered == false &&
        restored == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered store path, the adjacent HS word remains unchanged in the MSHR-pressure-plus-resp_st corner",
        excpt.triggered == false &&
        restored_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the MSHR-pressure-plus-resp_st restore corner updates only the target backing word while preserving the adjacent word",
        read64(target_paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_pmp_resp_st_repeated_then_restore_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x300U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x300U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122446688aacceeULL;
    const uint64_t warmed = 0x99bbddff11335577ULL;
    const uint64_t denied = 0xf0e1d2c3b4a59687ULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, warmed);
    TEST_ASSERT("an HS store first warms the translated cacheable line before the repeated resp_st adjacent-preservation restore probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the warm-up HS store updates the backing memory before write permission is removed for the repeated resp_st adjacent-preservation corner",
        read64(paddr) == warmed
    );
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("a first software prefetch remains silent when a hot HS translation later reaches the PMP resp_st path in the repeated adjacent-preservation restore corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("repeating the same software prefetch on the hot resp_st line also remains silent in the repeated adjacent-preservation restore corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("the same hot HS address still reports store access fault before PMP write permission is restored in the repeated resp_st adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied hot-HS store still leaves both the target and adjacent backing-memory words unchanged after the repeated resp_st silent probes",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("restoring PMP write permission still lets the same hot HS line accept the store again after the repeated resp_st adjacent-preservation probes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated resp_st silent prefetch restore corner updates only the target word while preserving the adjacent backing-memory word",
        read64(paddr) == denied &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_then_pmp_resp_ld_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page target_page = ai_micro_prefetch_mshr_pages[page_count - 1];
    uintptr_t target_vaddr = hs_page_base(target_page) + 0x380U;
    uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    uintptr_t target_paddr = phys_page_base(target_page) + 0x380U;
    uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t seed = 0x88009900aa00bb00ULL;
    const uint64_t expected = 0x2468ace013579bdfULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(target_paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_vaddr);
    TEST_ASSERT("an HS load first warms the translated cacheable target line before the MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        warmed == expected
    );

    for (size_t i = 0; i + 1 < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the MSHR-pressure-plus-resp_ld sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the MSHR-pressure-plus-resp_ld sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the MSHR-pressure-plus-resp_ld probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        target_paddr & PAGE_ADDR_MSK,
        (target_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("even with the miss machinery already under pressure, a software prefetch on a hot HS line that later reaches the PMP resp_ld path still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(target_vaddr);
    TEST_ASSERT("the same hot HS address still reports load access fault before PMP read permission is restored after the MSHR-pressure probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied hot-HS load leaves both the target and adjacent backing-memory words unchanged before resp_ld restore under MSHR pressure",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored = ld(target_vaddr);
    TEST_ASSERT("restoring PMP read permission after the MSHR-pressure-plus-resp_ld path makes the same HS target word readable again",
        excpt.triggered == false &&
        restored == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered resp_ld path, the adjacent HS word remains unchanged in the MSHR-pressure-plus-resp_ld corner",
        excpt.triggered == false &&
        restored_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the MSHR-pressure-plus-resp_ld restore corner preserves both the target and adjacent backing-memory words",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_repeated_then_pmp_resp_ld_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page target_page = ai_micro_prefetch_mshr_pages[page_count - 1];
    uintptr_t target_vaddr = hs_page_base(target_page) + 0x3c0U;
    uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    uintptr_t target_paddr = phys_page_base(target_page) + 0x3c0U;
    uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t seed = 0x9900aa00bb00cc00ULL;
    const uint64_t expected = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(target_paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_vaddr);
    TEST_ASSERT("an HS load first warms the translated cacheable target line before the repeated MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        warmed == expected
    );

    for (size_t i = 0; i + 1 < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the repeated MSHR-pressure-plus-resp_ld sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the repeated MSHR-pressure-plus-resp_ld sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the repeated MSHR-pressure-plus-resp_ld probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        target_paddr & PAGE_ADDR_MSK,
        (target_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("a first software prefetch on the hot HS target still remains silent even with the miss machinery already under pressure before the resp_ld path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("repeating the same hot-target software prefetch also remains silent under the same MSHR-pressure-plus-resp_ld conditions",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(target_vaddr);
    TEST_ASSERT("the same hot HS address still reports load access fault before PMP read permission is restored after the repeated MSHR-pressure resp_ld probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied hot-HS load still leaves both the target and adjacent backing-memory words unchanged after the repeated resp_ld silent probes under MSHR pressure",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored = ld(target_vaddr);
    TEST_ASSERT("restoring PMP read permission still makes the same HS target word readable again after the repeated MSHR-pressure-plus-resp_ld probes",
        excpt.triggered == false &&
        restored == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the repeated recovered resp_ld path, the adjacent HS word remains unchanged in the MSHR-pressure-plus-resp_ld corner",
        excpt.triggered == false &&
        restored_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated MSHR-pressure-plus-resp_ld restore corner preserves both the target and adjacent backing-memory words",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_m_prefetch_pmp_allow_silent()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE);
    const uint64_t expected = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_restore_pmp_full_access();
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("an M-mode software prefetch with an allowed PMP window retires without surfacing an exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(paddr);
    TEST_ASSERT("the same physical address still loads normally after the silent M-mode prefetch allow path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_m_prefetch_unlocked_pmp_u_bypass_silent_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1c0U;
    const uint64_t expected = 0x2468ace013579bdfULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("an M-mode software prefetch remains silent with MPRV=0 and MPP=U when the deny-read PMP entry is not locked, because the access still runs with current M privilege",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(paddr);
    TEST_ASSERT("the same direct physical address still loads normally after the unlocked-PMP bypass prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_m_prefetch_locked_pmp_u_silent()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE);

    goto_priv(PRIV_M);
    write64(paddr, 0x0123456789abcdefULL);
    ai_micro_prefetch_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("an M-mode software prefetch stays silent even when a locked PMP entry denies reads and MPP is set to U",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("the same physical address still raises load access fault on a real M-mode load with the locked deny entry active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool ai_micro_m_prefetch_locked_pmp_u_repeated_silent_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x200U;
    const uint64_t expected = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("a first M-mode software prefetch stays silent when a locked PMP entry denies reads and MPP is set to U",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("repeating the same locked-PMP-denied M-mode software prefetch also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("the same physical address still raises load access fault on a real M-mode load after the repeated silent locked-PMP prefetch probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == paddr
    );

    TEST_END();
}

bool ai_micro_m_prefetch_locked_pmp_m_silent()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE);

    goto_priv(PRIV_M);
    write64(paddr, 0xfedcba9876543210ULL);
    ai_micro_prefetch_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R_LOCKED);
    CSRS(CSR_MSTATUS, MSTATUS_MPP);

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("an M-mode software prefetch stays silent even when a locked PMP entry denies reads and MPP is set to M",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("the same physical address still raises load access fault on a real M-mode load with MPP=M and the locked deny entry active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool ai_micro_m_prefetch_locked_pmp_m_repeated_silent_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x220U;
    const uint64_t expected = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R_LOCKED);
    CSRS(CSR_MSTATUS, MSTATUS_MPP);

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("a first M-mode software prefetch stays silent even when a locked PMP entry denies reads and MPP is set to M",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("repeating the same M-mode software prefetch with MPP=M and the locked deny entry also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("the same physical address still raises load access fault on a real M-mode load with MPP=M after the repeated silent prefetch probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == paddr
    );

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_u_pmp_deny_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE);
    const uint64_t expected = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("an M-mode software prefetch with MPRV=1 and MPP=U stays silent when PMP denies the translated read",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same translated address still raises load access fault on a real M-mode load with MPRV=1 and MPP=U",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("restoring PMP access lets the same translated address load the original payload again after the silent MPRV prefetch deny path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_u_pmp_deny_repeated_silent_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x240U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x240U;
    const uint64_t expected = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first M-mode translated software prefetch stays silent when PMP denies reads with MPRV=1 and MPP=U",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("repeating the same M-mode translated software prefetch on the denied page also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same translated address still reports load access fault on a real M-mode load after the repeated silent deny-prefetch probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_enable_mprv_user_translation();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("restoring PMP read permission lets the same translated address load the original payload again after the repeated silent deny-prefetch probes",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_u_same_page_deny_then_restore_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x140U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x140U;
    const uint64_t expected = 0xcafebabedeadbeefULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("an M-mode translated software prefetch first stays silent when PMP denies reads to the selected translated page",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same translated address still reports load access fault on a real M-mode load while the deny window remains active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_enable_mprv_user_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("after restoring PMP read permission, retrying the same translated software prefetch remains silent without rebuilding the translation context",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission, the same translated address loads the original payload again in the same MPRV=1 MPP=U mode",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_u_same_page_deny_then_restore_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x140U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x140U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0xcafebabedeadbeefULL;
    const uint64_t adjacent = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("an M-mode translated software prefetch first stays silent when PMP denies reads to the selected translated page in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same translated address still reports load access fault on a real M-mode load while the deny window remains active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the translated deny-prefetch path leaves both the target and adjacent backing word unchanged before PMP permission is restored",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_enable_mprv_user_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("after restoring PMP read permission, retrying the same translated software prefetch remains silent without rebuilding the translation context in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission, the same translated address loads the original payload again in the same MPRV=1 MPP=U mode in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the translated deny-prefetch path is repaired, the adjacent translated word also still returns its original payload",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the translated deny-prefetch repair corner leaves only the target word visible to later reads while preserving the adjacent word",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_u_same_page_deny_then_sfence_restore_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x180U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x180U;
    const uint64_t expected = 0x6c7d8e9fa0b1c2d3ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("an M-mode translated software prefetch stays silent when PMP first denies reads to the selected translated page before the explicit sfence restore probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same translated address still reports load access fault on a real M-mode load while the deny window remains active before the explicit sfence restore probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_enable_mprv_user_translation();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("after restoring PMP read permission and issuing sfence.vma, retrying the same translated software prefetch still remains silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission and issuing sfence.vma, the same translated address loads the original payload again in MPRV=1 MPP=U mode",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_u_same_page_deny_then_sfence_restore_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x180U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x180U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x6c7d8e9fa0b1c2d3ULL;
    const uint64_t adjacent = 0x1029384756abcdefULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("an M-mode translated software prefetch stays silent when PMP first denies reads to the selected translated page in the explicit sfence adjacent-preservation probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same translated address still reports load access fault on a real M-mode load while the deny window remains active in the explicit sfence adjacent-preservation probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before PMP permission is restored in the explicit sfence adjacent-preservation probe, both the target and adjacent backing words remain unchanged",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_enable_mprv_user_translation();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("after restoring PMP read permission and issuing sfence.vma, retrying the same translated software prefetch still remains silent in the adjacent-preservation probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission and issuing sfence.vma, the same translated address loads the original payload again in the explicit adjacent-preservation probe",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the explicit sfence restore path repairs the translated deny-prefetch probe, the adjacent translated word also still returns its original payload",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the explicit sfence translated deny-prefetch restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_u_same_page_repeated_deny_then_sfence_restore_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1c0U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1c0U;
    const uint64_t expected = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first M-mode translated software prefetch stays silent when PMP denies reads to the selected translated page before the repeated explicit sfence restore probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("repeating the same M-mode translated software prefetch on the denied page also stays silent before the repeated explicit sfence restore probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same translated address still reports load access fault on a real M-mode load after the repeated silent deny-prefetch probes before the explicit sfence restore step",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_enable_mprv_user_translation();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("after restoring PMP read permission and issuing sfence.vma, retrying the same translated software prefetch remains silent after the repeated denied-page probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission and issuing sfence.vma, the same translated address loads the original payload again after the repeated denied-page probes",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_u_same_page_repeated_deny_then_sfence_restore_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1c0U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1c0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x89abcdef01234567ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first M-mode translated software prefetch stays silent when PMP denies reads to the selected translated page in the repeated explicit sfence adjacent-preservation probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("repeating the same M-mode translated software prefetch on the denied page also stays silent in the repeated explicit sfence adjacent-preservation probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same translated address still reports load access fault on a real M-mode load after the repeated silent deny-prefetch probes in the repeated explicit sfence adjacent-preservation probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before PMP permission is restored in the repeated explicit sfence adjacent-preservation probe, both the target and adjacent backing words remain unchanged",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_enable_mprv_user_translation();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("after restoring PMP read permission and issuing sfence.vma, retrying the same translated software prefetch remains silent after the repeated denied-page probes in the adjacent-preservation probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission and issuing sfence.vma, the same translated address loads the original payload again after the repeated denied-page probes in the adjacent-preservation probe",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the repeated explicit sfence restore path repairs the translated deny-prefetch probe, the adjacent translated word also still returns its original payload",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated explicit sfence translated deny-prefetch restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
