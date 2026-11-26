#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_M_TLB_VALID_PAGE VSRWX_GRWX
#define AI_MICRO_M_TLB_BAD_PADDR 0x1fffffffffff8ULL

static inline void ai_micro_m_tlb_reset_machine_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);

    CSRW(CSR_PMPCFG0, 0x0fULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

bool ai_micro_m_load_direct_paddr_hit_data_visible()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE);
    const uint64_t expected = 0x1021324354657687ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(paddr, expected);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(paddr);
    TEST_ASSERT("an M-mode direct load with a canonical in-range physical address completes without exception and returns the backing data",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_m_load_direct_high_addr_access_fault_raw_tval()
{
    TEST_START();

    ai_micro_m_tlb_reset_machine_env();

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("an M-mode direct load to a high invalid physical address reports load access fault and preserves the raw faulting address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_END();
}

bool ai_micro_m_load_direct_high_addr_then_valid_paddr_recovers_corner()
{
    TEST_START();

    uintptr_t good_paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x68U;
    const uint64_t expected = 0x5566778899aabbccULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(good_paddr, expected);

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("an M-mode direct load first reports load access fault on a high invalid physical address before the later valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_paddr);
    TEST_ASSERT("a later M-mode direct load to a canonical in-range physical address still completes without exception after the earlier high-address fault",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("the same high invalid physical address still reports load access fault with the raw address in tval after the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_END();
}

bool ai_micro_m_load_direct_high_addr_then_valid_paddr_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t good_paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x178U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t expected = 0x5566778899aabbccULL;
    const uint64_t adjacent = 0x1122334455667788ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("an M-mode direct load first reports load access fault on a high invalid physical address before the later valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_paddr);
    TEST_ASSERT("a later M-mode direct load to a canonical in-range physical address still completes without exception after the earlier high-address fault in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("the same high invalid physical address still reports load access fault with the raw address in tval after the valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_ASSERT("the earlier high-address direct-load fault and the later valid recovery load leave both the target backing word and its adjacent word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_m_load_direct_repeated_high_addr_access_fault_stable_corner()
{
    TEST_START();

    uintptr_t good_paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x118U;
    const uint64_t expected = 0x89abcdef76543210ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(good_paddr, expected);

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("a first M-mode direct load to a high invalid physical address reports load access fault and preserves the raw bad address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("repeating the same M-mode direct load to the high invalid physical address still reports the same load access fault with the same raw tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_paddr);
    TEST_ASSERT("a later direct load to a canonical in-range physical address still succeeds after the repeated high-address load faults",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_m_prefetch_direct_paddr_hit_silent()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE);
    const uint64_t expected = 0x89abcdef01234567ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(paddr, expected);

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("an M-mode software prefetch to a canonical in-range physical address retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(paddr);
    TEST_ASSERT("the same physical address still loads the original payload after the silent direct-prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_m_prefetch_direct_repeated_paddr_hit_silent_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x80U;
    const uint64_t expected = 0x2468ace013579bdfULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(paddr, expected);
    write64(paddr + sizeof(uint64_t), adjacent);

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("a first M-mode software prefetch to a canonical in-range physical address retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(paddr);
    TEST_ASSERT("repeating M-mode software prefetch on the same canonical in-range physical address also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(paddr);
    TEST_ASSERT("the same physical address still loads the original payload after repeated silent direct-prefetch probes",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_ASSERT("repeated silent direct-prefetch probes preserve both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(paddr + sizeof(uint64_t)) == adjacent
    );

    TEST_END();
}

bool ai_micro_m_prefetch_direct_high_addr_fault_silent()
{
    TEST_START();

    ai_micro_m_tlb_reset_machine_env();

    TEST_SETUP_EXCEPT();
    prefetch_r(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("an M-mode software prefetch to a high invalid physical address is silently dropped",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("a real M-mode load to the same high invalid physical address still reports load access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_END();
}

bool ai_micro_m_prefetch_direct_high_addr_then_valid_paddr_recovers_corner()
{
    TEST_START();

    uintptr_t good_paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x98U;
    const uint64_t expected = 0x3141592653589793ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(good_paddr, expected);

    TEST_SETUP_EXCEPT();
    prefetch_r(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("an M-mode software prefetch to a high invalid physical address is silently dropped before a later valid-address recovery probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_paddr);
    TEST_ASSERT("a later M-mode software prefetch to a canonical in-range physical address still retires silently after the earlier high-address probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_paddr);
    TEST_ASSERT("the later valid physical address still loads the expected payload after the earlier high-address silent prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("the same high invalid physical address still reports load access fault with the raw address in tval after the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_END();
}

bool ai_micro_m_prefetch_direct_high_addr_then_valid_paddr_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t good_paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x1a8U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t expected = 0x3141592653589793ULL;
    const uint64_t adjacent = 0x2718281828459045ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);

    TEST_SETUP_EXCEPT();
    prefetch_r(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("an M-mode software prefetch to a high invalid physical address is silently dropped before a later valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_paddr);
    TEST_ASSERT("a later M-mode software prefetch to a canonical in-range physical address still retires silently after the earlier high-address probe in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_paddr);
    TEST_ASSERT("the later valid physical address still loads the expected payload after the earlier high-address silent prefetch path in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("the same high invalid physical address still reports load access fault with the raw address in tval after the valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_ASSERT("the earlier high-address silent prefetch and the later valid prefetch/load path leave both the target backing word and its adjacent word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_m_prefetch_direct_repeated_high_addr_silent_corner()
{
    TEST_START();

    uintptr_t good_paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x158U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t expected = 0x0badf00d2468ace1ULL;
    const uint64_t adjacent = 0x13579bdf02468aceULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);

    TEST_SETUP_EXCEPT();
    prefetch_r(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("a first M-mode software prefetch to a high invalid physical address is silently dropped",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("repeating M-mode software prefetch to the same high invalid physical address is also silently dropped",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_paddr);
    TEST_ASSERT("a later direct load to a canonical in-range physical address still succeeds after the repeated high-address silent-prefetch probes",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(AI_MICRO_M_TLB_BAD_PADDR);
    TEST_ASSERT("a real M-mode load to the same high invalid physical address still reports load access fault with the raw address in tval after the repeated silent-prefetch probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_ASSERT("repeated high-address silent-prefetch probes preserve both the later valid target and its adjacent backing-memory word",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_m_store_direct_paddr_hit_data_visible()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE);
    const uint64_t initial = 0x1122334455667788ULL;
    const uint64_t stored = 0xa1b2c3d4e5f60718ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(paddr, initial);

    TEST_SETUP_EXCEPT();
    sd(paddr, stored);
    TEST_ASSERT("an M-mode direct store with a canonical in-range physical address completes without exception",
        excpt.triggered == false
    );

    TEST_ASSERT("the same backing location reflects the stored value after the direct M-mode store path",
        read64(paddr) == stored
    );

    TEST_END();
}

bool ai_micro_m_store_then_load_direct_paddr_latest_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x88U;
    const uint64_t initial = 0x0123456789abcdefULL;
    const uint64_t stored = 0xfedcba9876543210ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(paddr, initial);

    TEST_SETUP_EXCEPT();
    sd(paddr, stored);
    TEST_ASSERT("an M-mode direct store to a canonical in-range physical address completes without exception before the younger direct load",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(paddr);
    TEST_ASSERT("the younger M-mode direct load from the same physical address observes the latest payload written by the older direct store",
        excpt.triggered == false &&
        observed == stored
    );

    TEST_ASSERT("the direct store-then-load sequence leaves backing memory updated with the stored payload",
        read64(paddr) == stored
    );

    TEST_END();
}

bool ai_micro_m_store_direct_repeated_paddr_last_writer_wins_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0xa8U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1021324354657687ULL;
    const uint64_t first_stored = 0x1122334455667788ULL;
    const uint64_t second_stored = 0x99aabbccddeeff00ULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);

    TEST_SETUP_EXCEPT();
    sd(paddr, first_stored);
    TEST_ASSERT("the first M-mode direct store to the canonical in-range physical address completes without exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(paddr, second_stored);
    TEST_ASSERT("the second M-mode direct store to the same physical address also completes without exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(paddr);
    TEST_ASSERT("reloading the same physical address after two direct stores returns the last writer's payload",
        excpt.triggered == false &&
        observed == second_stored
    );

    TEST_ASSERT("repeated direct stores update only the target physical address while the adjacent word remains unchanged",
        read64(paddr) == second_stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_m_store_direct_high_addr_access_fault_raw_tval()
{
    TEST_START();

    ai_micro_m_tlb_reset_machine_env();

    TEST_SETUP_EXCEPT();
    sd(AI_MICRO_M_TLB_BAD_PADDR, 0x5566778899aabbccULL);
    TEST_ASSERT("an M-mode direct store to a high invalid physical address reports store access fault and preserves the raw faulting address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_END();
}

bool ai_micro_m_store_direct_high_addr_then_valid_paddr_recovers_corner()
{
    TEST_START();

    uintptr_t good_paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0xc8U;
    const uint64_t initial = 0x2468ace013579bdfULL;
    const uint64_t stored = 0x76543210fedcba98ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(good_paddr, initial);

    TEST_SETUP_EXCEPT();
    sd(AI_MICRO_M_TLB_BAD_PADDR, 0x5566778899aabbccULL);
    TEST_ASSERT("an M-mode direct store first reports store access fault on a high invalid physical address before the later valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_ASSERT("the earlier high-address direct-store fault leaves the later valid target unchanged before recovery",
        read64(good_paddr) == initial
    );

    TEST_SETUP_EXCEPT();
    sd(good_paddr, stored);
    TEST_ASSERT("a later M-mode direct store to a canonical in-range physical address still completes without exception after the earlier high-address fault",
        excpt.triggered == false
    );

    TEST_ASSERT("the later valid direct store updates backing memory with its stored payload after the earlier high-address fault",
        read64(good_paddr) == stored
    );

    TEST_SETUP_EXCEPT();
    sd(AI_MICRO_M_TLB_BAD_PADDR, 0x0badc0de12345678ULL);
    TEST_ASSERT("the same high invalid physical address still reports store access fault with the raw address in tval after the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_END();
}

bool ai_micro_m_store_direct_high_addr_then_valid_paddr_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t good_paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x1e8U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t initial = 0x2468ace013579bdfULL;
    const uint64_t stored = 0x76543210fedcba98ULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(good_paddr, initial);
    write64(adjacent_paddr, adjacent);

    TEST_SETUP_EXCEPT();
    sd(AI_MICRO_M_TLB_BAD_PADDR, 0x5566778899aabbccULL);
    TEST_ASSERT("an M-mode direct store first reports store access fault on a high invalid physical address before the later valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_ASSERT("the earlier high-address direct-store fault leaves both the later valid target and its adjacent word unchanged before recovery",
        read64(good_paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(good_paddr, stored);
    TEST_ASSERT("a later M-mode direct store to a canonical in-range physical address still completes without exception after the earlier high-address fault in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_ASSERT("the later valid direct store updates only its target backing word after the earlier high-address fault while preserving the adjacent word",
        read64(good_paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(AI_MICRO_M_TLB_BAD_PADDR, 0x0badc0de12345678ULL);
    TEST_ASSERT("the same high invalid physical address still reports store access fault with the raw address in tval after the valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_END();
}

bool ai_micro_m_store_direct_repeated_high_addr_access_fault_stable_corner()
{
    TEST_START();

    uintptr_t good_paddr = phys_page_base(AI_MICRO_M_TLB_VALID_PAGE) + 0x1d8U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122446688aacceeULL;
    const uint64_t adjacent = 0xf0e1d2c3b4a59687ULL;
    const uint64_t stored = 0x89abcdef01234567ULL;

    ai_micro_m_tlb_reset_machine_env();
    write64(good_paddr, initial);
    write64(adjacent_paddr, adjacent);

    TEST_SETUP_EXCEPT();
    sd(AI_MICRO_M_TLB_BAD_PADDR, 0x5566778899aabbccULL);
    TEST_ASSERT("a first M-mode direct store to a high invalid physical address reports store access fault and preserves the raw bad address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_SETUP_EXCEPT();
    sd(AI_MICRO_M_TLB_BAD_PADDR, 0x0badc0de12345678ULL);
    TEST_ASSERT("repeating the same M-mode direct store to the high invalid physical address still reports the same store access fault with the same raw tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == AI_MICRO_M_TLB_BAD_PADDR
    );

    TEST_ASSERT("repeated high-address direct-store faults leave the later valid target and its adjacent word unchanged before recovery",
        read64(good_paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(good_paddr, stored);
    TEST_ASSERT("a later direct store to a canonical in-range physical address still succeeds after the repeated high-address store faults",
        excpt.triggered == false
    );

    TEST_ASSERT("the later valid direct store updates only its target backing word after the repeated high-address faults",
        read64(good_paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}
