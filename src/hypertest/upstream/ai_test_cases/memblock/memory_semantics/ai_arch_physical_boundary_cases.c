#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ARCH_BOUNDARY_PAGE VSRWX_GRWX
#define AI_ARCH_BOUNDARY_REGION_START 0x30000000000ULL
#define AI_ARCH_BOUNDARY_REGION_END   0x40000000000ULL
#define AI_ARCH_BOUNDARY_REGION_SPLIT (AI_ARCH_BOUNDARY_REGION_START + PAGE_SIZE)
#define AI_ARCH_LAST_DWORD_OFFSET     (PAGE_SIZE - sizeof(uint64_t))

static inline void ai_arch_reset_default_pma_env(void)
{
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
}

static inline void ai_arch_configure_boundary_pma_region(bool cacheable, bool atomic)
{
    goto_priv(PRIV_M);
    ai_arch_reset_default_pma_env();

    CSRW(CSR_PMAADDR0, AI_ARCH_BOUNDARY_REGION_START >> 2);
    CSRW(CSR_PMAADDR1, AI_ARCH_BOUNDARY_REGION_END >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);

    if (cacheable) {
        CSRS(CSR_PMACFG0, 1ULL << 14);
    } else {
        CSRC(CSR_PMACFG0, 1ULL << 14);
    }

    if (atomic) {
        CSRS(CSR_PMACFG0, 1ULL << 13);
    } else {
        CSRC(CSR_PMACFG0, 1ULL << 13);
    }
}

static inline void ai_arch_configure_boundary_split_pma_regions(
    bool first_cacheable, bool second_cacheable)
{
    goto_priv(PRIV_M);
    ai_arch_reset_default_pma_env();

    CSRW(CSR_PMAADDR0, AI_ARCH_BOUNDARY_REGION_START >> 2);
    CSRW(CSR_PMAADDR1, AI_ARCH_BOUNDARY_REGION_SPLIT >> 2);
    CSRW(CSR_PMAADDR2, AI_ARCH_BOUNDARY_REGION_END >> 2);

    /* Entry 1: [REGION_START, REGION_SPLIT) as MEM/cacheable. */
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    if (first_cacheable) {
        CSRS(CSR_PMACFG0, 1ULL << 14);
    } else {
        CSRC(CSR_PMACFG0, 1ULL << 14);
    }

    CSRS(CSR_PMACFG0, 1ULL << 16);
    CSRS(CSR_PMACFG0, 1ULL << 17);
    CSRS(CSR_PMACFG0, 1ULL << 18);
    CSRS(CSR_PMACFG0, 1ULL << 19);
    if (second_cacheable) {
        CSRS(CSR_PMACFG0, 1ULL << 22);
    } else {
        CSRC(CSR_PMACFG0, 1ULL << 22);
    }
}

static inline void ai_arch_prepare_boundary_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
}

static inline uintptr_t ai_arch_boundary_anchor(uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)AI_ARCH_BOUNDARY_PAGE * PAGE_SIZE);
}

static inline uintptr_t ai_arch_boundary_anchor_for(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_arch_remap_boundary_page(uintptr_t target_page_start)
{
    goto_priv(PRIV_M);
    hspt_leaf_change_base_paddr(AI_ARCH_BOUNDARY_PAGE,
        ai_arch_boundary_anchor(target_page_start));
    sfence_vma();
    return hs_page_base(AI_ARCH_BOUNDARY_PAGE);
}

static inline void ai_arch_remap_boundary_page_pair(enum test_page first_page,
    uintptr_t first_page_start, enum test_page second_page,
    uintptr_t second_page_start)
{
    goto_priv(PRIV_M);
    hspt_leaf_change_base_paddr(first_page,
        ai_arch_boundary_anchor_for(first_page, first_page_start));
    hspt_leaf_change_base_paddr(second_page,
        ai_arch_boundary_anchor_for(second_page, second_page_start));
    sfence_vma();
}

bool ai_arch_io_boundary_atomic_faults()
{
    TEST_START();

    uintptr_t first_page_start = AI_ARCH_BOUNDARY_REGION_START;
    uintptr_t last_page_start = AI_ARCH_BOUNDARY_REGION_END - PAGE_SIZE;

    ai_arch_prepare_boundary_translation();

    TEST_SETUP_EXCEPT();
    ai_arch_configure_boundary_pma_region(false, false);
    TEST_ASSERT("configuring the IO PMA boundary region succeeds",
        excpt.triggered == false
    );
    if (excpt.triggered) {
        goto_priv(PRIV_M);
        ai_arch_reset_default_pma_env();
        goto failed;
    }

    uintptr_t start_vaddr = ai_arch_remap_boundary_page(first_page_start);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sc_d(start_vaddr, 0x0102030405060708ULL);
    TEST_ASSERT("sc.d at the first doubleword inside the IO PMA boundary reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == start_vaddr
    );

    TEST_SETUP_EXCEPT();
    lr_d(start_vaddr);
    TEST_ASSERT("lr.d at the first doubleword inside the IO PMA boundary reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == start_vaddr
    );

    uintptr_t end_vaddr = ai_arch_remap_boundary_page(last_page_start) +
        AI_ARCH_LAST_DWORD_OFFSET;
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sc_d(end_vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("sc.d at the last doubleword inside the IO PMA boundary reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == end_vaddr
    );

    TEST_SETUP_EXCEPT();
    lr_d(end_vaddr);
    TEST_ASSERT("lr.d at the last doubleword inside the IO PMA boundary reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == end_vaddr
    );

    goto_priv(PRIV_M);
    ai_arch_reset_default_pma_env();

    TEST_END();
}

bool ai_arch_mem_boundary_data_correct()
{
    TEST_START();

    uintptr_t first_page_start = AI_ARCH_BOUNDARY_REGION_START;
    uintptr_t last_page_start = AI_ARCH_BOUNDARY_REGION_END - PAGE_SIZE;
    uint64_t start_initial = 0x1011121314151617ULL;
    uint64_t end_initial = 0x8877665544332211ULL;

    ai_arch_prepare_boundary_translation();

    TEST_SETUP_EXCEPT();
    ai_arch_configure_boundary_pma_region(true, false);
    TEST_ASSERT("configuring the MEM PMA boundary region succeeds",
        excpt.triggered == false
    );
    if (excpt.triggered) {
        goto_priv(PRIV_M);
        ai_arch_reset_default_pma_env();
        goto failed;
    }

    uintptr_t start_vaddr = ai_arch_remap_boundary_page(first_page_start);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(start_vaddr, start_initial);
    TEST_ASSERT("storing to the first doubleword inside the MEM PMA boundary succeeds",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t start_value = ld(start_vaddr);
    TEST_ASSERT("loading from the first doubleword inside the MEM PMA boundary returns the stored payload",
        excpt.triggered == false &&
        start_value == start_initial
    );

    uintptr_t end_vaddr = ai_arch_remap_boundary_page(last_page_start) +
        AI_ARCH_LAST_DWORD_OFFSET;
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(end_vaddr, end_initial);
    TEST_ASSERT("storing to the last doubleword inside the MEM PMA boundary succeeds",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t end_value = ld(end_vaddr);
    TEST_ASSERT("loading from the last doubleword inside the MEM PMA boundary returns the stored payload",
        excpt.triggered == false &&
        end_value == end_initial
    );

    goto_priv(PRIV_M);
    ai_arch_reset_default_pma_env();

    TEST_END();
}

bool ai_arch_io_boundary_scalar_data_correct()
{
    TEST_START();

    uintptr_t first_page_start = AI_ARCH_BOUNDARY_REGION_START;
    uintptr_t last_page_start = AI_ARCH_BOUNDARY_REGION_END - PAGE_SIZE;
    uintptr_t first_paddr = first_page_start;
    uintptr_t last_paddr = last_page_start + AI_ARCH_LAST_DWORD_OFFSET;
    uint64_t start_initial = 0x2122232425262728ULL;
    uint64_t start_updated = 0x3132333435363738ULL;
    uint64_t end_initial = 0x9897969594939291ULL;
    uint64_t end_updated = 0x8988878685848382ULL;

    ai_arch_prepare_boundary_translation();

    TEST_SETUP_EXCEPT();
    ai_arch_configure_boundary_pma_region(false, false);
    TEST_ASSERT("configuring the IO PMA boundary region for scalar data access succeeds",
        excpt.triggered == false
    );
    if (excpt.triggered) {
        goto_priv(PRIV_M);
        ai_arch_reset_default_pma_env();
        goto failed;
    }

    goto_priv(PRIV_M);
    write64(first_paddr, start_initial);
    write64(last_paddr, end_initial);

    uintptr_t start_vaddr = ai_arch_remap_boundary_page(first_page_start);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    uint64_t start_before = ld(start_vaddr);
    TEST_ASSERT("loading from the first doubleword inside the IO PMA boundary returns the responder payload",
        excpt.triggered == false &&
        start_before == start_initial
    );

    TEST_SETUP_EXCEPT();
    sd(start_vaddr, start_updated);
    TEST_ASSERT("storing to the first doubleword inside the IO PMA boundary updates the responder path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t start_after = ld(start_vaddr);
    TEST_ASSERT("reloading the first IO boundary doubleword observes the scalar store payload",
        excpt.triggered == false &&
        start_after == start_updated
    );

    uintptr_t end_vaddr = ai_arch_remap_boundary_page(last_page_start) +
        AI_ARCH_LAST_DWORD_OFFSET;
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    uint64_t end_before = ld(end_vaddr);
    TEST_ASSERT("loading from the last doubleword inside the IO PMA boundary returns the responder payload",
        excpt.triggered == false &&
        end_before == end_initial
    );

    TEST_SETUP_EXCEPT();
    sd(end_vaddr, end_updated);
    TEST_ASSERT("storing to the last doubleword inside the IO PMA boundary updates the responder path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t end_after = ld(end_vaddr);
    TEST_ASSERT("reloading the last IO boundary doubleword observes the scalar store payload",
        excpt.triggered == false &&
        end_after == end_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("IO boundary scalar stores update only the selected boundary doublewords",
        read64(first_paddr) == start_updated &&
        read64(last_paddr) == end_updated);
    ai_arch_reset_default_pma_env();

    TEST_END();
}

bool ai_arch_pma_mem_io_cross_boundary_scalar_fault_no_partial_corner()
{
    TEST_START();

    const enum test_page mem_to_io_page = SWITCH1;
    const enum test_page mem_to_io_high_page = SWITCH2;
    const enum test_page io_to_mem_page = VSRWX_GRWX;
    const enum test_page io_to_mem_high_page = VSRWX_GRW;
    const uintptr_t mem_page_start = AI_ARCH_BOUNDARY_REGION_START;
    const uintptr_t io_page_start = AI_ARCH_BOUNDARY_REGION_SPLIT;
    const uintptr_t mem_to_io_low_paddr = mem_page_start + PAGE_SIZE - 8U;
    const uintptr_t mem_to_io_high_paddr = io_page_start;
    const uintptr_t io_to_mem_low_paddr = mem_page_start + PAGE_SIZE - 8U;
    const uintptr_t io_to_mem_high_paddr = io_page_start;
    const uint64_t mem_low_seed = 0x1122334455667788ULL;
    const uint64_t io_high_seed = 0x8877665544332211ULL;
    const uint64_t io_low_seed = 0x99aabbccddeeff00ULL;
    const uint64_t mem_high_seed = 0x0102030405060708ULL;
    uintptr_t mem_to_io_base_vaddr = 0;
    uintptr_t io_to_mem_base_vaddr = 0;
    uintptr_t mem_to_io_bad_vaddr = 0;
    uintptr_t io_to_mem_bad_vaddr = 0;

    ai_arch_prepare_boundary_translation();
    ai_arch_configure_boundary_split_pma_regions(true, false);

    goto_priv(PRIV_M);
    write64(mem_to_io_low_paddr, mem_low_seed);
    write64(mem_to_io_high_paddr, io_high_seed);
    write64(io_to_mem_low_paddr, io_low_seed);
    write64(io_to_mem_high_paddr, mem_high_seed);

    ai_arch_remap_boundary_page_pair(mem_to_io_page, mem_page_start,
        mem_to_io_high_page, io_page_start);
    mem_to_io_base_vaddr = hs_page_base(mem_to_io_page);
    mem_to_io_bad_vaddr = mem_to_io_base_vaddr + PAGE_SIZE - 4U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(mem_to_io_bad_vaddr);
    TEST_ASSERT("a scalar load crossing from PMA=MEM into PMA=IO reports load access fault instead of stitching an ordinary-memory result",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == mem_to_io_bad_vaddr);

    TEST_SETUP_EXCEPT();
    sd(mem_to_io_bad_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("a scalar store crossing from PMA=MEM into PMA=IO reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == mem_to_io_bad_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the MEM-to-IO cross-boundary fault leaves both the MEM-side word and IO-side responder word unchanged",
        read64(mem_to_io_low_paddr) == mem_low_seed &&
        read64(mem_to_io_high_paddr) == io_high_seed);

    ai_arch_configure_boundary_split_pma_regions(false, true);

    goto_priv(PRIV_M);
    write64(io_to_mem_low_paddr, io_low_seed);
    write64(io_to_mem_high_paddr, mem_high_seed);

    ai_arch_remap_boundary_page_pair(io_to_mem_page, mem_page_start,
        io_to_mem_high_page, io_page_start);
    io_to_mem_base_vaddr = hs_page_base(io_to_mem_page);
    io_to_mem_bad_vaddr = io_to_mem_base_vaddr + PAGE_SIZE - 4U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(io_to_mem_bad_vaddr);
    TEST_ASSERT("a scalar load crossing from PMA=IO into PMA=MEM reports load access fault instead of falling through to ordinary backing",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == io_to_mem_bad_vaddr);

    TEST_SETUP_EXCEPT();
    sd(io_to_mem_bad_vaddr, 0x0f1e2d3c4b5a6978ULL);
    TEST_ASSERT("a scalar store crossing from PMA=IO into PMA=MEM reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == io_to_mem_bad_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the IO-to-MEM cross-boundary fault leaves both the IO-side responder word and MEM-side word unchanged",
        read64(io_to_mem_low_paddr) == io_low_seed &&
        read64(io_to_mem_high_paddr) == mem_high_seed);

    ai_arch_reset_default_pma_env();
    hspt_init();
    sfence_vma();

    TEST_END();
}
