#include "ai_micro_prefetch_common.h"

bool ai_micro_hs_prefetch_tlb_miss_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE);
    const uint64_t expected = 0x1021324354657687ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a software prefetch on a cold HS translation retires silently while the TLB refills",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("the same HS address still loads the expected payload after the silent prefetch TLB-miss path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_repeated_tlb_miss_silent_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x40U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x40U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x1021324354657687ULL;
    const uint64_t adjacent = 0x5566778899aabbccULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first software prefetch on a cold HS translation retires silently while the TLB refills",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("after resetting the translation context, repeating software prefetch on the same HS address also retires silently across a second cold-TLB path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("the same HS address still loads the expected payload after the repeated cold-translation silent prefetch probes",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated cold-translation silent prefetch probes preserve both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_repeated_same_address_silent_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x80U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x80U;
    const uint64_t expected = 0x3141592653589793ULL;
    const uint64_t adjacent = 0x2718281828459045ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(paddr + sizeof(uint64_t), adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first software prefetch to a translated HS cacheable line retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("repeating software prefetch on the same translated HS cacheable line also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("repeated software prefetch on the same HS address leaves the target payload readable and unchanged",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated software prefetch on the same HS line preserves both the target word and its adjacent backing-memory word",
        read64(paddr) == expected &&
        read64(paddr + sizeof(uint64_t)) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_tlb_exception_silent()
{
    TEST_START();

    uintptr_t pf_vaddr = hs_page_base(VSI_GI);
    uintptr_t af_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(pf_vaddr);
    TEST_ASSERT("a software prefetch to an invalid leaf retires silently instead of surfacing a page fault",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(pf_vaddr);
    TEST_ASSERT("the same invalid-leaf address still reports a load page fault on a real load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_prefetch_hs_leaf_entry(AI_MICRO_PREFETCH_PAGE) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(af_vaddr);
    TEST_ASSERT("a software prefetch to a leaf with overflowing PPN retires silently instead of surfacing an access fault",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(af_vaddr);
    TEST_ASSERT("the same overflowing translation still reports a load access fault on a real load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == af_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_page_fault_then_valid_access_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x40U;
    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0xc8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0xc8U;
    const uint64_t expected = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a software prefetch to an invalid HS leaf still retires silently before a later valid-address recovery probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later software prefetch to a valid HS address still retires silently after the earlier page-fault-class prefetch path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the valid HS address still loads its expected payload after the earlier page-fault-class silent prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same invalid HS address still reports load page fault on a real load after the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_page_fault_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x60U;
    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1c8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1c8U;
    uintptr_t adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t expected = 0x89abcdef01234567ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a software prefetch to an invalid HS leaf still retires silently before the adjacent-preservation recovery probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later software prefetch to a valid HS address still retires silently after the earlier page-fault-class prefetch path in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the valid HS address still loads its expected payload after the earlier page-fault-class silent prefetch path in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the adjacent HS word remains unchanged after the page-fault-class silent prefetch recovery path",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same invalid HS address still reports load page fault on a real load after the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the page-fault-class silent prefetch recovery corner preserves both the valid target and adjacent backing-memory word",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_repeated_page_fault_silent_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x48U;
    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0xe8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0xe8U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t expected = 0x89abcdef01234567ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a first software prefetch to an invalid HS leaf still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("repeating software prefetch to the same invalid HS leaf also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("a later valid HS address still loads its expected payload after the repeated page-fault-class silent prefetch probes",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same invalid HS address still reports load page fault on a real load after the repeated silent page-fault-class prefetch probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated page-fault-class silent prefetch probes leave the later valid target and adjacent backing-memory word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_prefetch_access_fault_then_valid_access_recovers_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0xd8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0xd8U;
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_overflow_alias(
        SWITCH1, AI_MICRO_PREFETCH_PAGE) + 0xd8U;
    const uint64_t expected = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a software prefetch to a translated HS leaf with overflowing PPN still retires silently before a later valid-address recovery probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later software prefetch to a valid HS address still retires silently after the earlier access-fault-class prefetch path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the valid HS address still loads its expected payload after the earlier access-fault-class silent prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same overflowing HS translation still reports load access fault on a real load after the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_access_fault_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1d8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x1d8U;
    uintptr_t adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_overflow_alias(
        SWITCH1, AI_MICRO_PREFETCH_PAGE) + 0x1d8U;
    const uint64_t expected = 0x76543210fedcba98ULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a software prefetch to a translated HS leaf with overflowing PPN still retires silently before the adjacent-preservation access-fault recovery probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later software prefetch to a valid HS address still retires silently after the earlier access-fault-class prefetch path in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the valid HS address still loads its expected payload after the earlier access-fault-class silent prefetch path in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the adjacent HS word remains unchanged after the access-fault-class silent prefetch recovery path",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same overflowing HS translation still reports load access fault on a real load after the adjacent-preservation access-fault recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the access-fault-class silent prefetch recovery corner preserves both the valid target and adjacent backing-memory word",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_repeated_access_fault_silent_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0xf8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0xf8U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_overflow_alias(
        SWITCH1, AI_MICRO_PREFETCH_PAGE) + 0xf8U;
    const uint64_t expected = 0x76543210fedcba98ULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a first software prefetch to a translated HS leaf with overflowing PPN still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("repeating software prefetch to the same translated HS leaf with overflowing PPN also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("a later valid HS address still loads its expected payload after the repeated access-fault-class silent prefetch probes",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same overflowing HS alias still reports load access fault on a real load after the repeated silent access-fault-class prefetch probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated access-fault-class silent prefetch probes leave the later valid target and adjacent backing-memory word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_dcache_miss_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE);
    const uint64_t initial = 0x0123456789abcdefULL;
    const uint64_t refreshed = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    TEST_ASSERT("an HS load first warms the translation and cache state before the prefetch dcache-miss probe",
        excpt.triggered == false &&
        warmed == initial
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr);
    TEST_ASSERT("invalidating the warmed cache line succeeds before the prefetch dcache-miss probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    write64(paddr, refreshed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a software prefetch with a hot translation and a cold cache line retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("the same HS address still loads the refreshed payload after the silent prefetch dcache-miss path",
        excpt.triggered == false &&
        observed == refreshed
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_repeated_dcache_miss_silent_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x160U;
    uintptr_t paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x160U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x0123456789abcdefULL;
    const uint64_t refreshed_once = 0xfedcba9876543210ULL;
    const uint64_t refreshed_twice = 0x0badf00d2468ace1ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    TEST_ASSERT("an HS load first warms the translation and cache state before the repeated prefetch dcache-miss probes",
        excpt.triggered == false &&
        warmed == initial
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr);
    TEST_ASSERT("invalidating the warmed cache line succeeds before the first repeated dcache-miss prefetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    write64(paddr, refreshed_once);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("a first software prefetch with a hot translation and a cold cache line retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_observed = ld(vaddr);
    TEST_ASSERT("the same HS address loads the first refreshed payload after the first silent prefetch dcache-miss probe",
        excpt.triggered == false &&
        first_observed == refreshed_once
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr);
    TEST_ASSERT("invalidating the line again succeeds before the second repeated dcache-miss prefetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    write64(paddr, refreshed_twice);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("repeating software prefetch on the same HS address after re-cooling the line also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_observed = ld(vaddr);
    TEST_ASSERT("the same HS address loads the second refreshed payload after the repeated silent dcache-miss prefetch probes",
        excpt.triggered == false &&
        second_observed == refreshed_twice
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent dcache-miss prefetch probes preserve the adjacent backing-memory word while letting the target track the latest backing payload",
        read64(paddr) == refreshed_twice &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_preserves_latest_corner()
{
    TEST_START();

    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x98U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x98U;
    uintptr_t paddr = phys_page_base(base_page) + 0x98U;
    const uint64_t initial = 0x1122334455667788ULL;
    const uint64_t updated = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(base_vaddr);
    TEST_ASSERT("a software prefetch through the first HS alias retires silently before the shared-line update",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(base_vaddr, updated);
    TEST_ASSERT("an HS store through the first alias still updates the shared physical line normally with a prefetched alias present",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("a software prefetch through the second HS alias to the same physical line also retires silently after the shared-line update",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    TEST_ASSERT("the second HS alias still reads the latest payload after software prefetches on both aliases",
        excpt.triggered == false &&
        observed_alias == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    TEST_ASSERT("the original HS alias also reads back the same latest shared payload after the alias-prefetch sequence",
        excpt.triggered == false &&
        observed_base == updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("software prefetches across two HS aliases leave the shared backing memory at the latest stored payload",
        read64(paddr) == updated
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x1a8U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x1a8U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x1a8U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122334455667788ULL;
    const uint64_t updated = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(base_vaddr);
    TEST_ASSERT("a software prefetch through the first HS alias retires silently before the shared-line adjacent-preservation update",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(base_vaddr, updated);
    TEST_ASSERT("an HS store through the first alias still updates the shared physical line normally with a prefetched alias present in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("a software prefetch through the second HS alias to the same physical line also retires silently after the shared-line update in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    TEST_ASSERT("the second HS alias still reads the latest payload after software prefetches on both aliases in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_alias == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    TEST_ASSERT("the original HS alias also reads back the same latest shared payload after the alias-prefetch adjacent-preservation sequence",
        excpt.triggered == false &&
        observed_base == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the alias-prefetch sequence",
        excpt.triggered == false &&
        observed_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the remapped alias also remains unchanged after the alias-prefetch sequence",
        excpt.triggered == false &&
        observed_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("software prefetches across two HS aliases update only the shared target word while preserving the adjacent backing-memory word",
        read64(paddr) == updated &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_repeated_silent_corner()
{
    TEST_START();

    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x2a8U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x2a8U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x2a8U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x123456789abcdef0ULL;
    const uint64_t updated = 0x0fedcba987654321ULL;
    const uint64_t adjacent = 0xa55aa55aa55aa55aULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(base_vaddr);
    TEST_ASSERT("a first software prefetch through the base HS alias retires silently before the repeated alias-prefetch sequence",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("a first software prefetch through the switched HS alias also retires silently before the repeated alias-prefetch sequence",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(base_vaddr, updated);
    TEST_ASSERT("an HS store through the base alias still updates the shared physical line normally after silent prefetches on both aliases",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("repeating software prefetch through the switched HS alias remains silent after the shared-line update",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(base_vaddr);
    TEST_ASSERT("repeating software prefetch through the base HS alias also remains silent after the shared-line update",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    TEST_ASSERT("the switched HS alias still reads the latest shared payload after repeated software prefetches on both aliases",
        excpt.triggered == false &&
        observed_alias == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    TEST_ASSERT("the base HS alias also reads back the same latest shared payload after the repeated alias-prefetch sequence",
        excpt.triggered == false &&
        observed_base == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after repeated alias-prefetches and the shared-line update",
        excpt.triggered == false &&
        observed_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after repeated alias-prefetches and the shared-line update",
        excpt.triggered == false &&
        observed_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated software prefetches across two HS aliases update only the shared target word while preserving the adjacent backing-memory word",
        read64(paddr) == updated &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_then_pmp_deny_restore_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x348U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x348U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x348U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x89abcdef01234567ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(base_vaddr);
    TEST_ASSERT("a software prefetch through the base HS alias still retires silently before the same-paddr alias plus PMP-deny restore probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("a software prefetch through the switched HS alias remains silent even after read permission is denied on the shared physical page",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(alias_vaddr);
    TEST_ASSERT("the same switched HS alias still reports load access fault on a real read while the shared physical page is PMP-denied",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-paddr alias PMP-deny silent prefetch probe leaves both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    TEST_ASSERT("restoring PMP read permission lets the base HS alias load the shared target payload again in the same-paddr alias restore corner",
        excpt.triggered == false &&
        observed_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    TEST_ASSERT("restoring PMP read permission also lets the switched HS alias load the same shared target payload again in the same-paddr alias restore corner",
        excpt.triggered == false &&
        observed_alias == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the same-paddr alias PMP-deny restore probe",
        excpt.triggered == false &&
        observed_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the same-paddr alias PMP-deny restore probe",
        excpt.triggered == false &&
        observed_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-paddr alias PMP-deny restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_then_pmp_resp_ld_restore_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x388U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x388U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x388U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x2468ace013579bdfULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    TEST_ASSERT("an HS load through the base alias first warms the translated cacheable line before the same-paddr alias resp_ld restore probe",
        excpt.triggered == false &&
        warmed_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    TEST_ASSERT("an HS load through the switched alias also warms the same translated physical line before the same-paddr alias resp_ld restore probe",
        excpt.triggered == false &&
        warmed_alias == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("a software prefetch through the switched HS alias remains silent when the already-hot shared line later reaches the PMP resp_ld path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(alias_vaddr);
    TEST_ASSERT("the same switched HS alias still reports load access fault on a real read after the silent same-paddr alias resp_ld prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-paddr alias resp_ld silent prefetch probe leaves both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored_base = ld(base_vaddr);
    TEST_ASSERT("restoring PMP read permission lets the base HS alias read the shared target payload again in the same-paddr alias resp_ld restore corner",
        excpt.triggered == false &&
        restored_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_alias = ld(alias_vaddr);
    TEST_ASSERT("restoring PMP read permission also lets the switched HS alias read the same shared target payload again in the same-paddr alias resp_ld restore corner",
        excpt.triggered == false &&
        restored_alias == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the same-paddr alias resp_ld restore probe",
        excpt.triggered == false &&
        restored_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the same-paddr alias resp_ld restore probe",
        excpt.triggered == false &&
        restored_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-paddr alias resp_ld restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_repeated_then_pmp_resp_ld_restore_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH2;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x3c8U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x3c8U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x3c8U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    TEST_ASSERT("an HS load through the base alias first warms the translated cacheable line before the repeated same-paddr alias resp_ld restore probe",
        excpt.triggered == false &&
        warmed_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    TEST_ASSERT("an HS load through the switched alias also warms the same translated physical line before the repeated same-paddr alias resp_ld restore probe",
        excpt.triggered == false &&
        warmed_alias == expected
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(base_vaddr);
    TEST_ASSERT("a first software prefetch through the base HS alias remains silent when the hot shared line later reaches the PMP resp_ld path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("a second software prefetch through the switched HS alias also remains silent under the same repeated same-paddr alias resp_ld conditions",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(alias_vaddr);
    TEST_ASSERT("the switched HS alias still reports load access fault on a real read after the repeated same-paddr alias resp_ld silent prefetch probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated same-paddr alias resp_ld silent prefetch probes leave both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored_base = ld(base_vaddr);
    TEST_ASSERT("restoring PMP read permission still lets the base HS alias read the shared target payload again after the repeated same-paddr alias resp_ld probes",
        excpt.triggered == false &&
        restored_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_alias = ld(alias_vaddr);
    TEST_ASSERT("restoring PMP read permission still lets the switched HS alias read the same shared target payload again after the repeated same-paddr alias resp_ld probes",
        excpt.triggered == false &&
        restored_alias == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the repeated same-paddr alias resp_ld restore probe",
        excpt.triggered == false &&
        restored_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the repeated same-paddr alias resp_ld restore probe",
        excpt.triggered == false &&
        restored_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated same-paddr alias resp_ld restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_then_pmp_resp_st_restore_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH2;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x3e8U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x3e8U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x3e8U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122446688aacceeULL;
    const uint64_t warmed = 0x99bbddff11335577ULL;
    const uint64_t stored = 0xf0e1d2c3b4a59687ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(base_vaddr, warmed);
    TEST_ASSERT("an HS store through the base alias first warms the translated cacheable line before the same-paddr alias resp_st restore probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the warm-up HS store through the base alias updates the shared backing memory before write permission is removed in the same-paddr alias resp_st corner",
        read64(paddr) == warmed
    );
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    TEST_ASSERT("an HS load through the switched alias also warms the same translated physical line before the same-paddr alias resp_st restore probe",
        excpt.triggered == false &&
        warmed_alias == initial
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(alias_vaddr);
    TEST_ASSERT("a software prefetch through the switched HS alias remains silent when the already-hot shared line later reaches the PMP resp_st path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(alias_vaddr, stored);
    TEST_ASSERT("the same switched HS alias still reports store access fault on a real write after the silent same-paddr alias resp_st prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-paddr alias resp_st silent prefetch probe leaves both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(alias_vaddr, stored);
    TEST_ASSERT("restoring PMP write permission lets the switched HS alias commit the shared target payload again in the same-paddr alias resp_st restore corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_base = ld(base_vaddr);
    TEST_ASSERT("after the recovered same-paddr alias resp_st path, the base HS alias reads back the newly written shared target payload",
        excpt.triggered == false &&
        restored_base == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("after the recovered same-paddr alias resp_st path, the adjacent word through the base alias remains unchanged",
        excpt.triggered == false &&
        restored_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("after the recovered same-paddr alias resp_st path, the adjacent word through the switched alias also remains unchanged",
        excpt.triggered == false &&
        restored_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-paddr alias resp_st restore corner updates only the shared target word while preserving the adjacent backing-memory word",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_silent()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const uint64_t seed = 0x1100220033004400ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(vaddr);
        TEST_ASSERT("each HS probe page is readable before the prefetch MSHR-pressure sequence starts",
            excpt.triggered == false &&
            warmed == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed probe line can be invalidated before the prefetch MSHR-pressure sequence",
            excpt.triggered == false
        );
    }

    // Over-subscribe cold-line prefetches to approximate an MSHR-full probe while
    // still asserting only the architecturally visible silent behavior.
    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch retires silently while preceding cold-line requests are already in flight",
            excpt.triggered == false
        );
    }

    uintptr_t target_vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[page_count - 1]);
    const uint64_t expected = seed + ((page_count - 1) * 0x101ULL);

    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("an HS software prefetch remains silent even after a burst of cold-line prefetch traffic pressures the miss machinery",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(target_vaddr);
    TEST_ASSERT("the target page still loads its expected payload after the silent prefetch MSHR-pressure probe",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    const uint64_t seed = 0x3300440055006600ULL;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x300U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x300U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x300U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122334455667788ULL;
    const uint64_t updated = 0xcafebabedeadf00dULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(base_vaddr);
    TEST_ASSERT("an HS load first warms the target alias line before the alias+MSHR-pressure prefetch probe",
        excpt.triggered == false &&
        warmed == initial
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    TEST_ASSERT("invalidating the warmed target alias line succeeds before the alias+MSHR-pressure prefetch probe",
        excpt.triggered == false
    );

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the alias+MSHR-pressure sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the alias+MSHR-pressure sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the alias+MSHR-pressure probe",
            excpt.triggered == false
        );
    }

    TEST_SETUP_EXCEPT();
    prefetch_r(base_vaddr);
    TEST_ASSERT("a software prefetch through the base HS alias remains silent even while miss-machinery pressure is already building",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(alias_vaddr, updated);
    TEST_ASSERT("an HS store through the switched alias still updates the shared physical line normally during the alias+MSHR-pressure probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("a software prefetch through the switched HS alias also remains silent after the shared-line update under miss-machinery pressure",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    TEST_ASSERT("the base HS alias still reads the latest shared payload after the alias+MSHR-pressure prefetch sequence",
        excpt.triggered == false &&
        observed_base == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    TEST_ASSERT("the switched HS alias also reads the latest shared payload after the alias+MSHR-pressure prefetch sequence",
        excpt.triggered == false &&
        observed_alias == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the alias+MSHR-pressure prefetch sequence",
        excpt.triggered == false &&
        observed_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the alias+MSHR-pressure prefetch sequence",
        excpt.triggered == false &&
        observed_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("alias-prefetch traffic under miss-machinery pressure updates only the shared target word while preserving the adjacent backing-memory word",
        read64(paddr) == updated &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_then_pmp_deny_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    const uint64_t seed = 0x4400550066007700ULL;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x440U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x440U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x440U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x89abcdef01234567ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(base_vaddr);
    TEST_ASSERT("an HS load first warms the target alias line before the alias+MSHR-pressure-plus-PMP-deny probe",
        excpt.triggered == false &&
        warmed == expected
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    TEST_ASSERT("invalidating the warmed target alias line succeeds before the alias+MSHR-pressure-plus-PMP-deny probe",
        excpt.triggered == false
    );

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the alias+MSHR-pressure-plus-PMP-deny sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the alias+MSHR-pressure-plus-PMP-deny sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the alias+MSHR-pressure-plus-PMP-deny probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("even with miss-machinery pressure already building, a software prefetch through the switched alias remains silent after read permission is denied on the shared physical page",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(alias_vaddr);
    TEST_ASSERT("the switched HS alias still reports load access fault on a real read before PMP read permission is restored after the alias+MSHR-pressure probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias+MSHR-pressure-plus-PMP-deny probe leaves both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    TEST_ASSERT("restoring PMP read permission lets the base HS alias load the shared target payload again after the alias+MSHR-pressure-plus-PMP-deny probe",
        excpt.triggered == false &&
        observed_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    TEST_ASSERT("restoring PMP read permission also lets the switched HS alias load the same shared target payload again after the alias+MSHR-pressure-plus-PMP-deny probe",
        excpt.triggered == false &&
        observed_alias == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the alias+MSHR-pressure-plus-PMP-deny restore path",
        excpt.triggered == false &&
        observed_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the alias+MSHR-pressure-plus-PMP-deny restore path",
        excpt.triggered == false &&
        observed_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias+MSHR-pressure-plus-PMP-deny restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_then_pmp_resp_ld_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    const uint64_t seed = 0x5500660077008800ULL;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x480U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x480U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x480U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x2468ace013579bdfULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    TEST_ASSERT("an HS load through the base alias first warms the translated cacheable line before the alias+MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        warmed_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    TEST_ASSERT("an HS load through the switched alias also warms the same translated physical line before the alias+MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        warmed_alias == expected
    );

    for (size_t i = 0; i + 1 < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the alias+MSHR-pressure-plus-resp_ld sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the alias+MSHR-pressure-plus-resp_ld sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the alias+MSHR-pressure-plus-resp_ld probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("even with miss-machinery pressure already building, a software prefetch through the switched alias remains silent when the hot shared line later reaches the PMP resp_ld path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(alias_vaddr);
    TEST_ASSERT("the switched HS alias still reports load access fault on a real read after the silent alias+MSHR-pressure-plus-resp_ld prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias+MSHR-pressure-plus-resp_ld silent prefetch probe leaves both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored_base = ld(base_vaddr);
    TEST_ASSERT("restoring PMP read permission lets the base HS alias read the shared target payload again after the alias+MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        restored_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_alias = ld(alias_vaddr);
    TEST_ASSERT("restoring PMP read permission also lets the switched HS alias read the same shared target payload again after the alias+MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        restored_alias == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the alias+MSHR-pressure-plus-resp_ld restore path",
        excpt.triggered == false &&
        restored_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the alias+MSHR-pressure-plus-resp_ld restore path",
        excpt.triggered == false &&
        restored_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias+MSHR-pressure-plus-resp_ld restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_then_pmp_resp_st_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH2;
    const uint64_t seed = 0x6600770088009900ULL;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x4c0U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x4c0U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x4c0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122446688aacceeULL;
    const uint64_t warmed = 0x99bbddff11335577ULL;
    const uint64_t stored = 0xf0e1d2c3b4a59687ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(base_vaddr, warmed);
    TEST_ASSERT("an HS store through the base alias first warms the translated cacheable line before the alias+MSHR-pressure-plus-resp_st probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the warm-up HS store through the base alias updates the shared backing memory before write permission is removed in the alias+MSHR-pressure-plus-resp_st corner",
        read64(paddr) == warmed
    );
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    TEST_ASSERT("an HS load through the switched alias also warms the same translated physical line before the alias+MSHR-pressure-plus-resp_st probe",
        excpt.triggered == false &&
        warmed_alias == initial
    );

    for (size_t i = 0; i + 1 < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the alias+MSHR-pressure-plus-resp_st sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the alias+MSHR-pressure-plus-resp_st sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the alias+MSHR-pressure-plus-resp_st probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(alias_vaddr);
    TEST_ASSERT("even with miss-machinery pressure already building, a software prefetch through the switched alias remains silent when the hot shared line later reaches the PMP resp_st path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(alias_vaddr, stored);
    TEST_ASSERT("the switched HS alias still reports store access fault on a real write after the silent alias+MSHR-pressure-plus-resp_st prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias+MSHR-pressure-plus-resp_st silent prefetch probe leaves both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(alias_vaddr, stored);
    TEST_ASSERT("restoring PMP write permission lets the switched HS alias commit the shared target payload again after the alias+MSHR-pressure-plus-resp_st probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_base = ld(base_vaddr);
    TEST_ASSERT("after the recovered alias+MSHR-pressure-plus-resp_st path, the base HS alias reads back the newly written shared target payload",
        excpt.triggered == false &&
        restored_base == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_alias = ld(alias_vaddr);
    TEST_ASSERT("after the recovered alias+MSHR-pressure-plus-resp_st path, the switched HS alias also reads back the newly written shared target payload",
        excpt.triggered == false &&
        restored_alias == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the alias+MSHR-pressure-plus-resp_st restore path",
        excpt.triggered == false &&
        restored_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the alias+MSHR-pressure-plus-resp_st restore path",
        excpt.triggered == false &&
        restored_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias+MSHR-pressure-plus-resp_st restore corner updates only the shared target backing word while preserving the adjacent word",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_repeated_silent_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const uint64_t seed = 0x2200330044005500ULL;
    uintptr_t target_vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[page_count - 1]);
    uintptr_t target_paddr = phys_page_base(ai_micro_prefetch_mshr_pages[page_count - 1]);
    uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t expected = seed + ((page_count - 1) * 0x101ULL);
    const uint64_t adjacent = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(vaddr);
        TEST_ASSERT("each HS probe page is readable before the repeated prefetch MSHR-pressure sequence starts",
            excpt.triggered == false &&
            warmed == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed probe line can be invalidated before the repeated prefetch MSHR-pressure sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the repeated-pressure probe",
            excpt.triggered == false
        );
    }

    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("an HS software prefetch to the pressure target remains silent even after a burst of cold-line prefetch traffic pressures the miss machinery",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("repeating the same pressure-target HS software prefetch also remains silent under the same miss-machinery pressure probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(target_vaddr);
    TEST_ASSERT("the pressure target still loads its expected payload after the repeated silent prefetch MSHR-pressure probes",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent prefetch probes under MSHR pressure preserve both the target and adjacent backing-memory words",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_then_store_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page target_page = SWITCH2;
    uintptr_t target_vaddr = hs_page_base(target_page) + 0x300U;
    uintptr_t target_paddr = phys_page_base(target_page) + 0x300U;
    uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t seed = 0x4400550066007700ULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(vaddr);
        TEST_ASSERT("each HS probe page is readable before the MSHR-pressure-plus-store sequence starts",
            excpt.triggered == false &&
            warmed == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed probe line can be invalidated before the MSHR-pressure-plus-store sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the MSHR-pressure-plus-store probe",
            excpt.triggered == false
        );
    }

    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("the target HS software prefetch remains silent even after the miss machinery is already under pressure before the recovery store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, stored);
    TEST_ASSERT("after the silent MSHR-pressure probe, a real HS store to the same target line still retires successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("a follow-on software prefetch to the just-updated target line also remains silent after the MSHR-pressure-plus-store sequence",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target = ld(target_vaddr);
    TEST_ASSERT("the target line reads back only the architecturally committed store payload after the MSHR-pressure-plus-store sequence",
        excpt.triggered == false &&
        observed_target == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the adjacent word remains unchanged after the MSHR-pressure-plus-store sequence",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the MSHR-pressure-plus-store corner updates only the target backing word while preserving the adjacent word",
        read64(target_paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_then_bus_error_silent_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    uintptr_t target_vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[page_count - 1]) + 0x80U;
    uintptr_t target_paddr =
        phys_page_base(ai_micro_prefetch_mshr_pages[page_count - 1]) + 0x80U;
    uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_bad_paddr_alias(
        SWITCH2, AI_MICRO_PREFETCH_BAD_PADDR);
    const uint64_t seed = 0x5500660077008800ULL;
    const uint64_t expected = 0x2468ace013579bdfULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(target_paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(vaddr);
        TEST_ASSERT("each HS probe page is readable before the MSHR-pressure-plus-bus-error sequence starts",
            excpt.triggered == false &&
            warmed == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed probe line can be invalidated before the MSHR-pressure-plus-bus-error sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the MSHR-pressure-plus-bus-error probe",
            excpt.triggered == false
        );
    }

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("even with the miss machinery already under pressure, a software prefetch to a translated bad-physical alias still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("after the pressure-plus-bus-error silent prefetch path, a later software prefetch to a valid HS target still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(target_vaddr);
    TEST_ASSERT("the valid HS target still loads its expected payload after the pressure-plus-bus-error silent prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same translated bad-physical alias still reports load access fault on a real load after the pressure-plus-bus-error silent prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the pressure-plus-bus-error silent prefetch corner leaves the valid target and adjacent backing-memory word unchanged",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
