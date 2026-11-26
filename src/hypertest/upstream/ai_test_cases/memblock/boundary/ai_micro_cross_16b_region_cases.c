#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_CROSS_16B_OFFSET 15U
#define AI_MICRO_CROSS_16B_WORD_OFFSET 13U
#define AI_MICRO_IO_REGION_END   0x20000000ULL
#define AI_MICRO_MEM_REGION_BASE  0x30000000000ULL
#define AI_MICRO_MEM_REGION_END   0x40000000000ULL

static inline void ai_micro_reset_default_pma_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
}

static inline void ai_micro_configure_tor_pma(uintptr_t start, uintptr_t end, bool cacheable)
{
    CSRW(CSR_PMAADDR0, start >> 2);
    CSRW(CSR_PMAADDR1, end >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    if (cacheable) {
        CSRS(CSR_PMACFG0, 1ULL << 14);
    } else {
        CSRC(CSR_PMACFG0, 1ULL << 14);
    }
    CSRS(CSR_PMACFG0, 1ULL << 11);
}

static inline uintptr_t ai_micro_prepare_cross_16b_io_region_page(void)
{
    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();
    ai_micro_configure_tor_pma(0x0, AI_MICRO_IO_REGION_END, false);

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_leaf_change_base_paddr(VSRWXPbmt0_GURWXPbmt0, 0x0);
    sfence_vma();
    goto_priv(PRIV_HS);

    return vs_page_base(VSRWXPbmt0_GURWXPbmt0) + AI_MICRO_CROSS_16B_OFFSET;
}

static inline uintptr_t ai_micro_prepare_cross_16b_pbmt_mem_page(enum test_page page)
{
    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();
    ai_micro_configure_tor_pma(AI_MICRO_MEM_REGION_BASE, AI_MICRO_MEM_REGION_END, true);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);

    goto_priv(PRIV_HS);
    hspt_init();
    hspt_leaf_change_base_paddr(page, AI_MICRO_MEM_REGION_BASE);
    sfence_vma();
    goto_priv(PRIV_HS);

    return vs_page_base(page) + AI_MICRO_CROSS_16B_OFFSET;
}

static inline uintptr_t ai_micro_cross_16b_mapped_paddr(enum test_page page,
    uintptr_t base_paddr, uintptr_t offset)
{
    return phys_page_base_offset(page, base_paddr) + offset;
}

static inline uint64_t ai_micro_cross_16b_sign_extend(uint64_t value,
    unsigned size_bytes)
{
    if (size_bytes == sizeof(uint16_t)) {
        return (uint64_t)(int64_t)(int16_t)value;
    }
    if (size_bytes == sizeof(uint32_t)) {
        return (uint64_t)(int64_t)(int32_t)value;
    }
    return value;
}

static inline uint64_t ai_micro_cross_16b_expected_scalar(uint64_t low,
    uint64_t high, unsigned access_offset, unsigned size_bytes,
    bool signed_load)
{
    unsigned __int128 combined =
        ((unsigned __int128)high << 64) | (unsigned __int128)low;
    const unsigned shift = (access_offset - 8U) * 8U;
    uint64_t extracted = (uint64_t)(combined >> shift);

    if (size_bytes == sizeof(uint16_t)) {
        extracted &= 0xffffULL;
    } else if (size_bytes == sizeof(uint32_t)) {
        extracted &= 0xffffffffULL;
    }

    return signed_load ?
        ai_micro_cross_16b_sign_extend(extracted, size_bytes) :
        extracted;
}

static inline uint64_t ai_micro_cross_16b_observed_scalar(uintptr_t vaddr,
    unsigned size_bytes, bool signed_load)
{
    if (size_bytes == sizeof(uint16_t)) {
        uint64_t value = (uint64_t)lhu(vaddr);
        return signed_load ?
            ai_micro_cross_16b_sign_extend(value, size_bytes) :
            value;
    }
    {
        uint64_t value = (uint64_t)lwu(vaddr);
        return signed_load ?
            ai_micro_cross_16b_sign_extend(value, size_bytes) :
            value;
    }
}

static inline uint64_t ai_micro_cross_16b_faulting_ld_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value) : "r"(addr) : "memory");
    return value;
}

static inline void ai_micro_prepare_cross_16b_mixed_pbmt_window(
    enum test_page warm_page, uintptr_t warm_paddr_base,
    enum test_page fault_page, uintptr_t fault_paddr_base,
    const enum test_page *recover_pages,
    const uintptr_t *recover_paddr_bases, unsigned recover_count)
{
    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();
    ai_micro_configure_tor_pma(
        AI_MICRO_MEM_REGION_BASE, AI_MICRO_MEM_REGION_END, true);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_leaf_change_base_paddr(warm_page, warm_paddr_base);
    hspt_leaf_change_base_paddr(fault_page, fault_paddr_base);
    for (unsigned i = 0; i < recover_count; ++i) {
        hspt_leaf_change_base_paddr(recover_pages[i], recover_paddr_bases[i]);
    }
    sfence_vma();
    goto_priv(PRIV_HS);
}

bool ai_micro_hs_load_cross_16b_io_region_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prepare_cross_16b_io_region_page();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS load that is misaligned across 16B into an IO region reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_pbmt_nc_misaligned()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prepare_cross_16b_pbmt_mem_page(VSRWXPbmt1_GURWXPbmt0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS load that is misaligned across 16B on a PBMT=NC page reports load misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM
    );

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_pbmt_io_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prepare_cross_16b_pbmt_mem_page(VSRWXPbmt2_GURWXPbmt0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS load that is misaligned across 16B on a PBMT=IO page reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_io_region_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prepare_cross_16b_io_region_page();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS store that is misaligned across 16B into an IO region reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_pbmt_nc_misaligned()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prepare_cross_16b_pbmt_mem_page(VSRWXPbmt1_GURWXPbmt0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("an HS store that is misaligned across 16B on a PBMT=NC page reports store misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_pbmt_io_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prepare_cross_16b_pbmt_mem_page(VSRWXPbmt2_GURWXPbmt0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("an HS store that is misaligned across 16B on a PBMT=IO page reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_pbmt_nc_repeated_misaligned_then_aligned_load_recovers_corner()
{
    TEST_START();

    const uintptr_t base_vaddr = ai_micro_prepare_cross_16b_pbmt_mem_page(
        VSRWXPbmt1_GURWXPbmt0) - AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t bad_vaddr = base_vaddr + AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t good_vaddr = bad_vaddr + 1U;
    const uint64_t expected = 0x1122334455667788ULL;
    const uint64_t first_sentinel = 0x5a6b7c8d9eafb001ULL;
    const uint64_t second_sentinel = 0x8877665544332211ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_vaddr, expected);
    TEST_ASSERT("the aligned PBMT=NC recovery target is seeded through the same HS alias before the repeated misaligned probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_probe =
        ai_micro_cross_16b_faulting_ld_rd_probe(bad_vaddr, first_sentinel);
    TEST_ASSERT("the first HS cross-16B PBMT=NC load reports load misaligned with tval bound to the original bad VA and suppresses rd writeback before the aligned recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == bad_vaddr &&
        first_probe == first_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_probe =
        ai_micro_cross_16b_faulting_ld_rd_probe(bad_vaddr, second_sentinel);
    TEST_ASSERT("repeating the same HS cross-16B PBMT=NC load keeps the same cause/tval classification and still suppresses rd writeback before the aligned recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == bad_vaddr &&
        second_probe == second_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("after two prior PBMT=NC cross-16B misaligned faults, the aligned HS load on the same page pair still succeeds immediately",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_pbmt_io_repeated_access_fault_then_aligned_access_recovers_corner()
{
    TEST_START();

    const uintptr_t base_vaddr = ai_micro_prepare_cross_16b_pbmt_mem_page(
        VSRWXPbmt2_GURWXPbmt0) - AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t bad_vaddr = base_vaddr + AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t good_vaddr = bad_vaddr + 1U;
    const uintptr_t good_paddr = ai_micro_cross_16b_mapped_paddr(
        VSRWXPbmt2_GURWXPbmt0, AI_MICRO_MEM_REGION_BASE, 16U);
    const uint64_t expected = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first HS cross-16B PBMT=IO load still reports load access fault before the aligned recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("repeating the same HS cross-16B PBMT=IO load still reports load access fault before the aligned recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("after two prior PBMT=IO cross-16B access faults, the aligned HS load on the same page pair still succeeds immediately",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_pbmt_nc_repeated_misaligned_then_aligned_store_recovers_corner()
{
    TEST_START();

    const uintptr_t base_vaddr = ai_micro_prepare_cross_16b_pbmt_mem_page(
        VSRWXPbmt1_GURWXPbmt0) - AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t bad_vaddr = base_vaddr + AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t good_vaddr = base_vaddr + 0x20U;
    const uintptr_t adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial = 0x0123456789abcdefULL;
    const uint64_t adjacent = 0xfedcba9876543210ULL;
    const uint64_t updated = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_vaddr, initial);
    sd(adjacent_vaddr, adjacent);
    TEST_ASSERT("the PBMT=NC recovery target and adjacent word are seeded through the same HS alias before the repeated misaligned probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x1111222233334444ULL);
    TEST_ASSERT("the first HS cross-16B PBMT=NC store still reports store misaligned before the aligned recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x5555666677778888ULL);
    TEST_ASSERT("repeating the same HS cross-16B PBMT=NC store still reports store misaligned before the aligned recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_good = ld(good_vaddr);
    uint64_t preserved_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated HS cross-16B PBMT=NC store faults leave both the target doubleword and its adjacent doubleword unchanged before recovery",
        excpt.triggered == false &&
        preserved_good == initial &&
        preserved_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated);
    TEST_ASSERT("after two prior PBMT=NC cross-16B misaligned faults, the aligned HS store on the same page pair still succeeds immediately",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_good = ld(good_vaddr);
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the PBMT=NC cross-16B recovery probe, only the aligned target doubleword is updated while the adjacent doubleword stays unchanged",
        excpt.triggered == false &&
        observed_good == updated &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_pbmt_io_repeated_access_fault_then_aligned_access_recovers_corner()
{
    TEST_START();

    const uintptr_t base_vaddr = ai_micro_prepare_cross_16b_pbmt_mem_page(
        VSRWXPbmt2_GURWXPbmt0) - AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t bad_vaddr = base_vaddr + AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t good_vaddr = bad_vaddr + 1U;
    const uintptr_t good_paddr = ai_micro_cross_16b_mapped_paddr(
        VSRWXPbmt2_GURWXPbmt0, AI_MICRO_MEM_REGION_BASE, 16U);
    const uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t initial = 0x89abcdef01234567ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;
    const uint64_t updated = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x0102030405060708ULL);
    TEST_ASSERT("the first HS cross-16B PBMT=IO store still reports store access fault before the aligned recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x1112131415161718ULL);
    TEST_ASSERT("repeating the same HS cross-16B PBMT=IO store still reports store access fault before the aligned recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated HS cross-16B PBMT=IO store faults leave both the target doubleword and its adjacent doubleword unchanged before recovery",
        read64(good_paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated);
    TEST_ASSERT("after two prior PBMT=IO cross-16B access faults, the aligned HS store on the same page pair still succeeds immediately",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the PBMT=IO cross-16B recovery probe, only the aligned target doubleword is updated while the adjacent doubleword stays unchanged",
        read64(good_paddr) == updated &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_reset_default_pma_env();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_pbmt_fault_then_cacheable_signedness_switch_recovery_no_stale_mab_state_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES = 4
    };
    const enum test_page warm_page = SWITCH1;
    const enum test_page fault_pages[2] = {
        VSRWXPbmt1_GURWXPbmt0,
        VSRWXPbmt2_GURWXPbmt0,
    };
    const uint64_t fault_causes[2] = {
        CAUSE_LAM,
        CAUSE_LAF,
    };
    const enum test_page recover_pages[AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES] = {
        SWITCH2,
        VSRWXPbmt0_GURWXPbmt0,
        VSRWX_GRW,
        VSRW_GRW,
    };
    const uintptr_t warm_paddr_base = AI_MICRO_MEM_REGION_BASE;
    const uintptr_t fault_paddr_base = AI_MICRO_MEM_REGION_BASE + PAGE_SIZE;
    const uintptr_t recover_paddr_bases[AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES] = {
        AI_MICRO_MEM_REGION_BASE + 2U * PAGE_SIZE,
        AI_MICRO_MEM_REGION_BASE + 3U * PAGE_SIZE,
        AI_MICRO_MEM_REGION_BASE + 4U * PAGE_SIZE,
        AI_MICRO_MEM_REGION_BASE + 5U * PAGE_SIZE,
    };
    const uintptr_t warm_vaddr = vs_page_base(warm_page) + AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t warm_low_paddr = ai_micro_cross_16b_mapped_paddr(
        warm_page, warm_paddr_base, 8U);
    const uintptr_t warm_high_paddr = ai_micro_cross_16b_mapped_paddr(
        warm_page, warm_paddr_base, 16U);
    const uintptr_t fault_vaddr_nc = vs_page_base(fault_pages[0]) + AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t fault_vaddr_io = vs_page_base(fault_pages[1]) + AI_MICRO_CROSS_16B_OFFSET;
    const uintptr_t fault_vaddrs[2] = {
        fault_vaddr_nc,
        fault_vaddr_io,
    };
    const uintptr_t recover_vaddrs[AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES] = {
        vs_page_base(recover_pages[0]) + AI_MICRO_CROSS_16B_OFFSET,
        vs_page_base(recover_pages[1]) + AI_MICRO_CROSS_16B_OFFSET,
        vs_page_base(recover_pages[2]) + AI_MICRO_CROSS_16B_WORD_OFFSET,
        vs_page_base(recover_pages[3]) + AI_MICRO_CROSS_16B_WORD_OFFSET,
    };
    const unsigned recover_sizes[AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES] = {
        sizeof(uint16_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint32_t),
    };
    const unsigned recover_offsets[AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES] = {
        AI_MICRO_CROSS_16B_OFFSET,
        AI_MICRO_CROSS_16B_OFFSET,
        AI_MICRO_CROSS_16B_WORD_OFFSET,
        AI_MICRO_CROSS_16B_WORD_OFFSET,
    };
    const bool recover_signed[AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES] = {
        true,
        false,
        true,
        false,
    };
    const uint64_t warm_low_patterns[2] = {
        0x8070605040302010ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t warm_high_patterns[2] = {
        0x0123456789abcdefULL,
        0x1021324354657687ULL,
    };
    const uint64_t fault_low_patterns[2] = {
        0xdeadbeefcafef00dULL,
        0x89abcdef01234567ULL,
    };
    const uint64_t fault_high_patterns[2] = {
        0xddeeff0011223344ULL,
        0x76543210fedcbafeULL,
    };
    const uint64_t recover_low_patterns[AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES][2] = {
        { 0x34aa996633cc77eeULL, 0x23bb887755dd66aaULL },
        { 0xbc11223344556677ULL, 0xd05566778899aabbULL },
        { 0xabcdef6655443322ULL, 0x2345671122334455ULL },
        { 0xdcba987766554433ULL, 0xc0dec07766554433ULL },
    };
    const uint64_t recover_high_patterns[AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES][2] = {
        { 0x77665544332211f2ULL, 0x6655443322111081ULL },
        { 0x89abcdef0123459aULL, 0x76543210fedcbafeULL },
        { 0x7766554433221189ULL, 0x88776655443322f1ULL },
        { 0x0123456789abc0feULL, 0x11223344556677c0ULL },
    };

    for (unsigned round = 0; round < 2U; ++round) {
        uint64_t expected_recover[AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES];

        ai_micro_prepare_cross_16b_mixed_pbmt_window(
            warm_page, warm_paddr_base,
            fault_pages[round], fault_paddr_base,
            recover_pages, recover_paddr_bases,
            AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES);

        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES; ++i) {
            expected_recover[i] = ai_micro_cross_16b_expected_scalar(
                recover_low_patterns[i][round],
                recover_high_patterns[i][round],
                recover_offsets[i], recover_sizes[i], recover_signed[i]);
        }

        goto_priv(PRIV_M);
        write64(warm_low_paddr, warm_low_patterns[round]);
        write64(warm_high_paddr, warm_high_patterns[round]);
        write64(ai_micro_cross_16b_mapped_paddr(fault_pages[round],
            fault_paddr_base, 8U), fault_low_patterns[round]);
        write64(ai_micro_cross_16b_mapped_paddr(fault_pages[round],
            fault_paddr_base, 16U), fault_high_patterns[round]);
        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_SIGN_RECOVER_CASES; ++i) {
            write64(ai_micro_cross_16b_mapped_paddr(recover_pages[i],
                recover_paddr_bases[i], 8U), recover_low_patterns[i][round]);
            write64(ai_micro_cross_16b_mapped_paddr(recover_pages[i],
                recover_paddr_bases[i], 16U), recover_high_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t warm_observed = ld(warm_vaddr);
        TEST_ASSERT("before the PBMT cross-16B fault-cleanup probe, the priming cacheable cross-16B load completes with its own merged payload",
            excpt.triggered == false &&
            warm_observed == ai_micro_cross_16b_expected_scalar(
                warm_low_patterns[round], warm_high_patterns[round],
                AI_MICRO_CROSS_16B_OFFSET, sizeof(uint64_t), false)
        );

        TEST_SETUP_EXCEPT();
        ld(fault_vaddrs[round]);
        TEST_ASSERT("after the successful priming cacheable cross-16B load, the later PBMT cross-16B fault still reports its own precise exception class before signedness-switch recovery",
            excpt.triggered == true &&
            excpt.cause == fault_causes[round] &&
            excpt.tval == fault_vaddrs[round],
            "round=%u triggered=%d cause=0x%llx tval=0x%llx expected_tval=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, fault_vaddrs[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t forward_signed_half = ai_micro_cross_16b_observed_scalar(
            recover_vaddrs[0], recover_sizes[0], recover_signed[0]);
        uint64_t forward_unsigned_half = ai_micro_cross_16b_observed_scalar(
            recover_vaddrs[1], recover_sizes[1], recover_signed[1]);
        uint64_t forward_signed_word = ai_micro_cross_16b_observed_scalar(
            recover_vaddrs[2], recover_sizes[2], recover_signed[2]);
        uint64_t forward_unsigned_word = ai_micro_cross_16b_observed_scalar(
            recover_vaddrs[3], recover_sizes[3], recover_signed[3]);
        TEST_ASSERT("after the PBMT cross-16B fault episode is cleared, back-to-back cacheable signed and unsigned recovery loads refresh their own merge and data-select state instead of inheriting stale PBMT-fault state",
            excpt.triggered == false &&
            forward_signed_half == expected_recover[0] &&
            forward_unsigned_half == expected_recover[1] &&
            forward_signed_word == expected_recover[2] &&
            forward_unsigned_word == expected_recover[3]
        );

        TEST_SETUP_EXCEPT();
        uint64_t reverse_unsigned_word = ai_micro_cross_16b_observed_scalar(
            recover_vaddrs[3], recover_sizes[3], recover_signed[3]);
        uint64_t reverse_signed_word = ai_micro_cross_16b_observed_scalar(
            recover_vaddrs[2], recover_sizes[2], recover_signed[2]);
        uint64_t reverse_unsigned_half = ai_micro_cross_16b_observed_scalar(
            recover_vaddrs[1], recover_sizes[1], recover_signed[1]);
        uint64_t reverse_signed_half = ai_micro_cross_16b_observed_scalar(
            recover_vaddrs[0], recover_sizes[0], recover_signed[0]);
        TEST_ASSERT("reversing the cacheable post-PBMT-fault signedness-switch recovery order still keeps each cross-16B request's own payload and sign behavior with no stale carryover",
            excpt.triggered == false &&
            reverse_signed_half == expected_recover[0] &&
            reverse_unsigned_half == expected_recover[1] &&
            reverse_signed_word == expected_recover[2] &&
            reverse_unsigned_word == expected_recover[3]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("through the priming-success then PBMT-fault then cacheable signedness-switch recovery sequence, every participating boundary doubleword keeps its original backing bytes",
            read64(warm_low_paddr) == warm_low_patterns[round] &&
            read64(warm_high_paddr) == warm_high_patterns[round] &&
            read64(ai_micro_cross_16b_mapped_paddr(fault_pages[round],
                fault_paddr_base, 8U)) == fault_low_patterns[round] &&
            read64(ai_micro_cross_16b_mapped_paddr(fault_pages[round],
                fault_paddr_base, 16U)) == fault_high_patterns[round] &&
            read64(ai_micro_cross_16b_mapped_paddr(recover_pages[0],
                recover_paddr_bases[0], 8U)) == recover_low_patterns[0][round] &&
            read64(ai_micro_cross_16b_mapped_paddr(recover_pages[0],
                recover_paddr_bases[0], 16U)) == recover_high_patterns[0][round] &&
            read64(ai_micro_cross_16b_mapped_paddr(recover_pages[1],
                recover_paddr_bases[1], 8U)) == recover_low_patterns[1][round] &&
            read64(ai_micro_cross_16b_mapped_paddr(recover_pages[1],
                recover_paddr_bases[1], 16U)) == recover_high_patterns[1][round] &&
            read64(ai_micro_cross_16b_mapped_paddr(recover_pages[2],
                recover_paddr_bases[2], 8U)) == recover_low_patterns[2][round] &&
            read64(ai_micro_cross_16b_mapped_paddr(recover_pages[2],
                recover_paddr_bases[2], 16U)) == recover_high_patterns[2][round] &&
            read64(ai_micro_cross_16b_mapped_paddr(recover_pages[3],
                recover_paddr_bases[3], 8U)) == recover_low_patterns[3][round] &&
            read64(ai_micro_cross_16b_mapped_paddr(recover_pages[3],
                recover_paddr_bases[3], 16U)) == recover_high_patterns[3][round]
        );
    }

    goto_priv(PRIV_M);
    ai_micro_reset_default_pma_env();

    TEST_END();
}
