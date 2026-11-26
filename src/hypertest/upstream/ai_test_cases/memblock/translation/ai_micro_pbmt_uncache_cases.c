#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_PBMT_IO_MEM_BASE  0x30000000000ULL
#define AI_MICRO_PBMT_IO_MEM_LIMIT 0x40000000000ULL
#define AI_MICRO_PBMT_RESP_MMIO64_BASE  0x40070000ULL
#define AI_MICRO_PBMT_RESP_MMIO64_LIMIT 0x40080000ULL
#define AI_MICRO_PBMT_PMP_TOR_RWX  0x0fULL
#define AI_MICRO_PBMT_PMP_TOR_NO_R 0x0eULL
#define AI_MICRO_PBMT_PMP_TOR_NO_W 0x0dULL
#define AI_MICRO_PBMT_CACHE_BLOCK_BYTES 64U
#define AI_MICRO_PBMT_CROSS_16B_OFFSET 15U

static inline pte_t *ai_micro_pbmt_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uintptr_t ai_micro_pbmt_uncache_backing_paddr(void)
{
    return phys_page_base(X);
}

static inline uintptr_t ai_micro_pbmt_uncache_io_backing_paddr(void)
{
    return phys_page_base_offset(X, AI_MICRO_PBMT_IO_MEM_BASE);
}

static inline uintptr_t ai_micro_pbmt_uncache_resp_mmio64_paddr(uintptr_t offset)
{
    /*
     * LinkNan SimMMIO IntrGen is a real 8-byte MMIO responder.  Use it for
     * PBMT=IO good load/store probes that need a returning device response.
     */
    return AI_MICRO_PBMT_RESP_MMIO64_BASE +
        (offset & (AI_MICRO_PBMT_RESP_MMIO64_LIMIT -
            AI_MICRO_PBMT_RESP_MMIO64_BASE - sizeof(uint64_t)));
}

static inline uintptr_t ai_micro_pbmt_uncache_alias_anchor(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_micro_pbmt_uncache_alias_vaddr(enum test_page page,
    uintptr_t paddr)
{
    return hs_page_base(page) + (paddr & (PAGE_SIZE - 1));
}

static inline void ai_micro_pbmt_uncache_map_alias_to_paddr(
    enum test_page page, uintptr_t paddr)
{
    const uintptr_t page_start = paddr & PAGE_ADDR_MSK;

    pbmt_hspt_to_x(page);
    hspt_leaf_change_base_paddr(page,
        ai_micro_pbmt_uncache_alias_anchor(page, page_start));
}

static inline void ai_micro_pbmt_uncache_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0x0fULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_micro_reset_pbmt_uncache_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    ai_micro_pbmt_uncache_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_reset_pbmt_uncache_mmio_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFFULL);
    CSRW(CSR_PMACFG0, 0x6FULL);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000ULL);
    CSRW(CSR_PMAADDR12, 0x20000000ULL);
    CSRW(CSR_PMAADDR13, 0x120000000ULL);
    CSRW(CSR_PMAADDR14, 0xC000000000ULL);
    CSRW(CSR_PMAADDR15, 0x10000000000ULL);

    CSRW(CSR_PMACFG0, 0x0ULL);
    ai_micro_pbmt_uncache_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_prepare_pbmt_uncache_alias(enum test_page page)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline uintptr_t ai_micro_prepare_pbmt_uncache_alias_to_paddr(
    enum test_page page, uintptr_t paddr)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_uncache_map_alias_to_paddr(page, paddr);
    sfence_vma();

    return ai_micro_pbmt_uncache_alias_vaddr(page, paddr);
}

static inline void ai_micro_prepare_pbmt_mem_3t_4t_window(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
    CSRW(CSR_PMAADDR0, AI_MICRO_PBMT_IO_MEM_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_MICRO_PBMT_IO_MEM_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    fence_iorw_iorw();
}

static inline uintptr_t ai_micro_prepare_pbmt_uncache_io_alias(enum test_page page)
{
    ai_micro_prepare_pbmt_mem_3t_4t_window();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x_base_paddr(page, AI_MICRO_PBMT_IO_MEM_BASE);
    sfence_vma();
    return hs_page_base(page);
}

static inline uintptr_t ai_micro_prepare_pbmt_uncache_leaf_overflow(enum test_page page)
{
    uintptr_t vaddr = ai_micro_prepare_pbmt_uncache_alias(page);

    goto_priv(PRIV_M);
    *ai_micro_pbmt_hs_leaf_entry(page) |= (1ULL << 46);
    sfence_vma();

    return vaddr;
}

static inline uintptr_t ai_micro_prepare_pbmt_uncache_io_leaf_overflow(enum test_page page)
{
    uintptr_t vaddr = ai_micro_prepare_pbmt_uncache_io_alias(page);

    goto_priv(PRIV_M);
    *ai_micro_pbmt_hs_leaf_entry(page) |= (1ULL << 46);
    sfence_vma();

    return vaddr;
}

static inline uintptr_t ai_micro_prepare_pbmt_uncache_invalid_leaf(enum test_page page)
{
    uintptr_t vaddr = ai_micro_prepare_pbmt_uncache_alias(page);

    goto_priv(PRIV_M);
    *ai_micro_pbmt_hs_leaf_entry(page) = 0ULL;
    sfence_vma();

    return vaddr;
}

static inline uintptr_t ai_micro_prepare_pbmt_uncache_io_invalid_leaf(enum test_page page)
{
    uintptr_t vaddr = ai_micro_prepare_pbmt_uncache_io_alias(page);

    goto_priv(PRIV_M);
    *ai_micro_pbmt_hs_leaf_entry(page) = 0ULL;
    sfence_vma();

    return vaddr;
}

static inline void ai_micro_pbmt_uncache_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_PBMT_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_PBMT_PMP_TOR_RWX << 16));
}

static inline void ai_micro_pbmt_uncache_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t offset = 0; offset < AI_MICRO_PBMT_CACHE_BLOCK_BYTES;
        offset += sizeof(uint64_t)) {
        write64(paddr + offset, seed);
    }
}

static inline bool ai_micro_pbmt_uncache_block_all_zero(uintptr_t paddr)
{
    for (uintptr_t offset = 0; offset < AI_MICRO_PBMT_CACHE_BLOCK_BYTES;
        offset += sizeof(uint64_t)) {
        if (read64(paddr + offset) != 0ULL) {
            return false;
        }
    }
    return true;
}

static inline uintptr_t ai_micro_pbmt_cross_16b_addr(enum test_page page)
{
    return hs_page_base(page) + AI_MICRO_PBMT_CROSS_16B_OFFSET;
}

static inline void ai_micro_pbmt_set_hs_leaf(enum test_page page,
    uintptr_t paddr,
    uint64_t flags)
{
    *ai_micro_pbmt_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline uint64_t ai_micro_pbmt_cross_16b_expected_ld_value(
    uint64_t low_pattern,
    uint64_t high_pattern)
{
    return ((high_pattern & 0x00ffffffffffffffULL) << 8) |
        ((low_pattern >> 56) & 0xffULL);
}

bool ai_micro_hs_load_pbmt_uncache_data_visible()
{
    TEST_START();

    static const uint64_t nc_value = 0x1122334455667788ULL;
    static const uint64_t io_value = 0xa1b2c3d4e5f60718ULL;
    uintptr_t io_paddr = ai_micro_pbmt_uncache_io_backing_paddr();
    uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_alias_to_paddr(
        VSRWXPbmt1_GURWXPbmt1, (uintptr_t)&nc_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_nc = ld(nc_vaddr);
    TEST_ASSERT("an aligned HS load on a PBMT=NC alias completes and returns the backing payload",
        excpt.triggered == false &&
        observed_nc == nc_value
    );

    uintptr_t io_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(VSRWXPbmt2_GURWXPbmt1);

    goto_priv(PRIV_M);
    write64(io_paddr, io_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_io = ld(io_vaddr);
    TEST_ASSERT("an aligned HS load on a PBMT=IO alias completes and returns the backing payload",
        excpt.triggered == false &&
        observed_io == io_value
    );

    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_uncache_tlb_access_fault()
{
    TEST_START();

    uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_leaf_overflow(VSRWXPbmt1_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(nc_vaddr);
    TEST_ASSERT("an aligned HS load on a PBMT=NC alias reports load access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == nc_vaddr
    );

    uintptr_t io_vaddr = ai_micro_prepare_pbmt_uncache_io_leaf_overflow(VSRWXPbmt2_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(io_vaddr);
    TEST_ASSERT("an aligned HS load on a PBMT=IO alias reports load access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == io_vaddr
    );

    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_uncache_page_fault()
{
    TEST_START();

    uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_invalid_leaf(VSRWXPbmt1_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(nc_vaddr);
    TEST_ASSERT("an aligned HS load on a PBMT=NC alias reports load page fault when the leaf PTE is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == nc_vaddr
    );

    uintptr_t io_vaddr = ai_micro_prepare_pbmt_uncache_io_invalid_leaf(VSRWXPbmt2_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(io_vaddr);
    TEST_ASSERT("an aligned HS load on a PBMT=IO alias reports load page fault when the leaf PTE is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == io_vaddr
    );

    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_uncache_data_visible()
{
    TEST_START();

    uintptr_t paddr = ai_micro_pbmt_uncache_backing_paddr();
    uintptr_t io_paddr = ai_micro_pbmt_uncache_io_backing_paddr();
    uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_alias(VSRWXPbmt1_GURWXPbmt1);
    const uint64_t nc_value = 0x8877665544332211ULL;
    const uint64_t io_value = 0x0fedcba987654321ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, nc_value);
    TEST_ASSERT("an aligned HS store on a PBMT=NC alias completes without exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=NC aligned store updates the shared backing memory",
        read64(paddr) == nc_value
    );

    uintptr_t io_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(VSRWXPbmt2_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(io_vaddr, io_value);
    TEST_ASSERT("an aligned HS store on a PBMT=IO alias completes without exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=IO aligned store updates the shared backing memory",
        read64(io_paddr) == io_value
    );

    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_uncache_tlb_access_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_pbmt_uncache_backing_paddr();
    uintptr_t io_paddr = ai_micro_pbmt_uncache_io_backing_paddr();
    const uint64_t initial_value = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_leaf_overflow(VSRWXPbmt1_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("an aligned HS store on a PBMT=NC alias reports store access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == nc_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=NC leaf-overflow store fault leaves the shared backing unchanged",
        read64(paddr) == initial_value
    );

    write64(io_paddr, initial_value);
    uintptr_t io_vaddr = ai_micro_prepare_pbmt_uncache_io_leaf_overflow(VSRWXPbmt2_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(io_vaddr, 0xaabbccddeeff0011ULL);
    TEST_ASSERT("an aligned HS store on a PBMT=IO alias reports store access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == io_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=IO leaf-overflow store fault leaves the shared backing unchanged",
        read64(io_paddr) == initial_value
    );

    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_uncache_page_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_pbmt_uncache_backing_paddr();
    uintptr_t io_paddr = ai_micro_pbmt_uncache_io_backing_paddr();
    const uint64_t initial_value = 0x22446688aaccee00ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_invalid_leaf(VSRWXPbmt1_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, 0x0102030405060708ULL);
    TEST_ASSERT("an aligned HS store on a PBMT=NC alias reports store page fault when the leaf PTE is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == nc_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=NC invalid-leaf store page fault leaves the shared backing unchanged",
        read64(paddr) == initial_value
    );

    write64(io_paddr, initial_value);
    uintptr_t io_vaddr = ai_micro_prepare_pbmt_uncache_io_invalid_leaf(VSRWXPbmt2_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(io_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("an aligned HS store on a PBMT=IO alias reports store page fault when the leaf PTE is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == io_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=IO invalid-leaf store page fault leaves the shared backing unchanged",
        read64(io_paddr) == initial_value
    );

    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_uncache_pmp_read_access_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_pbmt_uncache_backing_paddr();
    uintptr_t io_paddr = ai_micro_pbmt_uncache_io_backing_paddr();

    goto_priv(PRIV_M);
    write64(paddr, 0x1122334455667788ULL);

    uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_alias(VSRWXPbmt1_GURWXPbmt1);
    ai_micro_pbmt_uncache_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_PBMT_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(nc_vaddr);
    TEST_ASSERT("an aligned HS load on a PBMT=NC alias reports load access fault when PMP denies reads",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == nc_vaddr
    );

    goto_priv(PRIV_M);
    write64(io_paddr, 0x1122334455667788ULL);
    uintptr_t io_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(VSRWXPbmt2_GURWXPbmt1);
    ai_micro_pbmt_uncache_configure_pmp_window(
        io_paddr, io_paddr + PAGE_SIZE, AI_MICRO_PBMT_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(io_vaddr);
    TEST_ASSERT("an aligned HS load on a PBMT=IO alias reports load access fault when PMP denies reads",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == io_vaddr
    );

    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_uncache_pmp_write_access_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_pbmt_uncache_backing_paddr();
    uintptr_t io_paddr = ai_micro_pbmt_uncache_io_backing_paddr();
    const uint64_t initial_value = 0x99aabbccddeeff00ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_alias(VSRWXPbmt1_GURWXPbmt1);
    ai_micro_pbmt_uncache_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_PBMT_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an aligned HS store on a PBMT=NC alias reports store access fault when PMP denies writes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == nc_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=NC store blocked by PMP leaves the shared backing unchanged",
        read64(paddr) == initial_value
    );

    write64(io_paddr, initial_value);
    uintptr_t io_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(VSRWXPbmt2_GURWXPbmt1);
    ai_micro_pbmt_uncache_configure_pmp_window(
        io_paddr, io_paddr + PAGE_SIZE, AI_MICRO_PBMT_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(io_vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("an aligned HS store on a PBMT=IO alias reports store access fault when PMP denies writes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == io_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=IO store blocked by PMP leaves the shared backing unchanged",
        read64(io_paddr) == initial_value
    );

    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_vector_load_pbmt_nc_repeated_access_fault_preserves_register_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH1) + 0x100U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH1) + 0x100U;
    const uintptr_t fault_paddr = ai_micro_pbmt_uncache_backing_paddr() + 0x1c0U;
    const uintptr_t fault_vaddr = ai_micro_prepare_pbmt_uncache_alias(
        VSRWXPbmt1_GURWXPbmt1) + 0x1c0U;
    const uintptr_t scratch_paddr = phys_page_base(SWITCH2) + 0x200U;
    const uintptr_t scratch_vaddr = hs_page_base(SWITCH2) + 0x200U;
    const uintptr_t adjacent_paddr = scratch_paddr + 16U;
    const uint8_t source_pattern[16] = {
        0x10U, 0x32U, 0x54U, 0x76U, 0x98U, 0xbaU, 0xdcU, 0xfeU,
        0xefU, 0xcdU, 0xabU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U
    };
    const uint64_t expected_low = 0xfedcba9876543210ULL;
    const uint64_t expected_high = 0x0123456789abcdefULL;
    const uint64_t scratch_init_low = 0x8877665544332211ULL;
    const uint64_t scratch_init_high = 0x1021324354657687ULL;
    const uint64_t adjacent_low = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent_high = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(source_pattern); ++i) {
        ((volatile uint8_t *)source_paddr)[i] = source_pattern[i];
    }
    write64(fault_paddr, 0x5566778899aabbccULL);
    write64(fault_paddr + sizeof(uint64_t), 0xddeeff0011223344ULL);
    write64(scratch_paddr, scratch_init_low);
    write64(scratch_paddr + sizeof(uint64_t), scratch_init_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("a vector unit-stride load can stage the reference bytes before the repeated PBMT=NC vector-load fault probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)fault_vaddr, 16);
    TEST_ASSERT("the first vector unit-stride load from a PBMT=NC alias reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)fault_vaddr, 16);
    TEST_ASSERT("repeating the same vector unit-stride load from a PBMT=NC alias still reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated PBMT=NC vector-load faults, storing the still-staged vector register into a normal scratch page succeeds",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=NC vector-load faults leave the vector register contents unchanged, so the scratch copy still matches the preloaded reference bytes while the adjacent scratch region stays untouched",
        read64(scratch_paddr) == expected_low &&
        read64(scratch_paddr + sizeof(uint64_t)) == expected_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_vector_store_pbmt_nc_repeated_access_fault_stable_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH1) + 0x100U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH1) + 0x100U;
    const uintptr_t target_paddr = ai_micro_pbmt_uncache_backing_paddr() + 0x180U;
    const uintptr_t adjacent_paddr = target_paddr + 16U;
    uintptr_t target_vaddr = ai_micro_prepare_pbmt_uncache_alias(
        VSRWXPbmt1_GURWXPbmt1) + 0x180U;
    const uint8_t source_pattern[16] = {
        0x10U, 0x32U, 0x54U, 0x76U, 0x98U, 0xbaU, 0xdcU, 0xfeU,
        0xefU, 0xcdU, 0xabU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U
    };
    const uint64_t initial_low = 0x8877665544332211ULL;
    const uint64_t initial_high = 0x1021324354657687ULL;
    const uint64_t adjacent_low = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent_high = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(source_pattern); ++i) {
        ((volatile uint8_t *)source_paddr)[i] = source_pattern[i];
    }
    write64(target_paddr, initial_low);
    write64(target_paddr + sizeof(uint64_t), initial_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("a vector unit-stride load can stage the source bytes before the repeated PBMT=NC vector-store fault probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)target_vaddr, 16);
    TEST_ASSERT("the first vector unit-stride store to a PBMT=NC alias reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("the source bytes can be reloaded into the vector register before the repeated PBMT=NC vector-store fault probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)target_vaddr, 16);
    TEST_ASSERT("repeating the same vector unit-stride store to a PBMT=NC alias still reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=NC vector-store faults leave both the 16-byte target region and its adjacent 16-byte region unchanged",
        read64(target_paddr) == initial_low &&
        read64(target_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_vector_load_pbmt_nc_repeated_access_fault_then_scalar_same_alias_recovers_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH1) + 0x1a0U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH1) + 0x1a0U;
    const uintptr_t fault_paddr = ai_micro_pbmt_uncache_backing_paddr() + 0x340U;
    const uintptr_t fault_vaddr = ai_micro_prepare_pbmt_uncache_alias(
        VSRWXPbmt1_GURWXPbmt1) + 0x340U;
    const uintptr_t second_word_vaddr = fault_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = fault_paddr + 16U;
    const uintptr_t adjacent_vaddr = fault_vaddr + 16U;
    const uint8_t source_pattern[16] = {
        0x10U, 0x32U, 0x54U, 0x76U, 0x98U, 0xbaU, 0xdcU, 0xfeU,
        0xefU, 0xcdU, 0xabU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U
    };
    const uint64_t initial_low = 0x5566778899aabbccULL;
    const uint64_t initial_high = 0xddeeff0011223344ULL;
    const uint64_t updated_low = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent_low = 0x0fedcba987654321ULL;
    const uint64_t adjacent_high = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(source_pattern); ++i) {
        ((volatile uint8_t *)source_paddr)[i] = source_pattern[i];
    }
    write64(fault_paddr, initial_low);
    write64(fault_paddr + sizeof(uint64_t), initial_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("a vector unit-stride load can stage the reference bytes before the repeated PBMT=NC vector-load fault then scalar-recovery probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)fault_vaddr, 16);
    TEST_ASSERT("the first vector unit-stride load from a PBMT=NC alias reports load access fault before the later scalar same-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)fault_vaddr, 16);
    TEST_ASSERT("repeating the same vector unit-stride load from a PBMT=NC alias still reports load access fault before the later scalar same-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(fault_vaddr);
    TEST_ASSERT("after the repeated PBMT=NC vector-load faults, a normal scalar load through the same alias still returns the original low doubleword",
        excpt.triggered == false &&
        observed_before == initial_low
    );

    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, updated_low);
    TEST_ASSERT("after the repeated PBMT=NC vector-load faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(fault_vaddr);
    TEST_ASSERT("after the scalar recovery store on the PBMT=NC alias, a later scalar load through the same alias observes the updated low doubleword immediately",
        excpt.triggered == false &&
        observed_after == updated_low
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_high = ld(second_word_vaddr);
    TEST_ASSERT("the repeated PBMT=NC vector-load faults and the later scalar recovery store leave the high doubleword in the same 16-byte target region unchanged",
        excpt.triggered == false &&
        observed_high == initial_high
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=NC vector-load faults and the later scalar recovery store also leave the adjacent 16-byte region unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_low
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=NC vector-load faults do not poison the later scalar same-alias path, so backing memory ends with only the low doubleword updated",
        read64(fault_paddr) == updated_low &&
        read64(fault_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_vector_store_pbmt_nc_repeated_access_fault_then_scalar_same_alias_recovers_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH2) + 0x1a0U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH2) + 0x1a0U;
    const uintptr_t target_paddr = ai_micro_pbmt_uncache_backing_paddr() + 0x3c0U;
    const uintptr_t target_vaddr = ai_micro_prepare_pbmt_uncache_alias(
        VSRWXPbmt1_GURWXPbmt1) + 0x3c0U;
    const uintptr_t second_word_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = target_paddr + 16U;
    const uintptr_t adjacent_vaddr = target_vaddr + 16U;
    const uint8_t source_pattern[16] = {
        0x10U, 0x32U, 0x54U, 0x76U, 0x98U, 0xbaU, 0xdcU, 0xfeU,
        0xefU, 0xcdU, 0xabU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U
    };
    const uint64_t initial_low = 0x8877665544332211ULL;
    const uint64_t initial_high = 0x1021324354657687ULL;
    const uint64_t updated_low = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent_low = 0x0fedcba987654321ULL;
    const uint64_t adjacent_high = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(source_pattern); ++i) {
        ((volatile uint8_t *)source_paddr)[i] = source_pattern[i];
    }
    write64(target_paddr, initial_low);
    write64(target_paddr + sizeof(uint64_t), initial_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("a vector unit-stride load can stage the source bytes before the repeated PBMT=NC vector-store fault then scalar-recovery probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)target_vaddr, 16);
    TEST_ASSERT("the first vector unit-stride store to a PBMT=NC alias reports store access fault before the later scalar same-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("the source bytes can be reloaded into the vector register before the repeated PBMT=NC vector-store fault then scalar-recovery probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)target_vaddr, 16);
    TEST_ASSERT("repeating the same vector unit-stride store to a PBMT=NC alias still reports store access fault before the later scalar same-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(target_vaddr);
    TEST_ASSERT("after the repeated PBMT=NC vector-store faults, a normal scalar load through the same alias still returns the original low doubleword",
        excpt.triggered == false &&
        observed_before == initial_low
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, updated_low);
    TEST_ASSERT("after the repeated PBMT=NC vector-store faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(target_vaddr);
    TEST_ASSERT("after the scalar recovery store on the PBMT=NC alias, a later scalar load through the same alias observes the updated low doubleword immediately",
        excpt.triggered == false &&
        observed_after == updated_low
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_high = ld(second_word_vaddr);
    TEST_ASSERT("the repeated PBMT=NC vector-store faults and the later scalar recovery store leave the high doubleword in the same 16-byte target region unchanged",
        excpt.triggered == false &&
        observed_high == initial_high
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=NC vector-store faults and the later scalar recovery store also leave the adjacent 16-byte region unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_low
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=NC vector-store faults do not poison the later scalar same-alias path, so backing memory ends with only the low doubleword updated",
        read64(target_paddr) == updated_low &&
        read64(target_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_vector_load_pbmt_io_repeated_access_fault_preserves_register_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH1) + 0x180U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH1) + 0x180U;
    const uintptr_t fault_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x240U;
    const uintptr_t fault_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(
        VSRWXPbmt2_GURWXPbmt1) + 0x240U;
    const uintptr_t scratch_paddr = phys_page_base(SWITCH2) + 0x280U;
    const uintptr_t scratch_vaddr = hs_page_base(SWITCH2) + 0x280U;
    const uintptr_t adjacent_paddr = scratch_paddr + 16U;
    const uint8_t source_pattern[16] = {
        0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU,
        0xf0U, 0xdeU, 0xbcU, 0x9aU, 0x78U, 0x56U, 0x34U, 0x12U
    };
    const uint64_t expected_low = 0xefcdab8967452301ULL;
    const uint64_t expected_high = 0x123456789abcdef0ULL;
    const uint64_t scratch_init_low = 0x7766554433221100ULL;
    const uint64_t scratch_init_high = 0x99aabbccddeeff00ULL;
    const uint64_t adjacent_low = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent_high = 0x02468ace13579bdfULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(source_pattern); ++i) {
        ((volatile uint8_t *)source_paddr)[i] = source_pattern[i];
    }
    write64(fault_paddr, 0x2233445566778899ULL);
    write64(fault_paddr + sizeof(uint64_t), 0xaabbccddeeff0011ULL);
    write64(scratch_paddr, scratch_init_low);
    write64(scratch_paddr + sizeof(uint64_t), scratch_init_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("a vector unit-stride load can stage the reference bytes before the repeated PBMT=IO vector-load fault probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)fault_vaddr, 16);
    TEST_ASSERT("the first vector unit-stride load from a PBMT=IO alias reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)fault_vaddr, 16);
    TEST_ASSERT("repeating the same vector unit-stride load from a PBMT=IO alias still reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated PBMT=IO vector-load faults, storing the still-staged vector register into a normal scratch page succeeds",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=IO vector-load faults leave the vector register contents unchanged, so the scratch copy still matches the preloaded reference bytes while the adjacent scratch region stays untouched",
        read64(scratch_paddr) == expected_low &&
        read64(scratch_paddr + sizeof(uint64_t)) == expected_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_vector_store_pbmt_io_repeated_access_fault_stable_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH2) + 0x180U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH2) + 0x180U;
    const uintptr_t target_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x200U;
    const uintptr_t adjacent_paddr = target_paddr + 16U;
    uintptr_t target_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(
        VSRWXPbmt2_GURWXPbmt1) + 0x200U;
    const uint8_t source_pattern[16] = {
        0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU,
        0xf0U, 0xdeU, 0xbcU, 0x9aU, 0x78U, 0x56U, 0x34U, 0x12U
    };
    const uint64_t initial_low = 0x7766554433221100ULL;
    const uint64_t initial_high = 0x99aabbccddeeff00ULL;
    const uint64_t adjacent_low = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent_high = 0x02468ace13579bdfULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(source_pattern); ++i) {
        ((volatile uint8_t *)source_paddr)[i] = source_pattern[i];
    }
    write64(target_paddr, initial_low);
    write64(target_paddr + sizeof(uint64_t), initial_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("a vector unit-stride load can stage the source bytes before the repeated PBMT=IO vector-store fault probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)target_vaddr, 16);
    TEST_ASSERT("the first vector unit-stride store to a PBMT=IO alias reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("the source bytes can be reloaded into the vector register before the repeated PBMT=IO vector-store fault probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)target_vaddr, 16);
    TEST_ASSERT("repeating the same vector unit-stride store to a PBMT=IO alias still reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=IO vector-store faults leave both the 16-byte target region and its adjacent 16-byte region unchanged",
        read64(target_paddr) == initial_low &&
        read64(target_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_vector_load_pbmt_io_repeated_access_fault_then_scalar_same_alias_recovers_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH1) + 0x1e0U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH1) + 0x1e0U;
    const uintptr_t fault_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x380U;
    const uintptr_t fault_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(
        VSRWXPbmt2_GURWXPbmt1) + 0x380U;
    const uintptr_t second_word_vaddr = fault_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = fault_paddr + 16U;
    const uintptr_t adjacent_vaddr = fault_vaddr + 16U;
    const uint8_t source_pattern[16] = {
        0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU,
        0xf0U, 0xdeU, 0xbcU, 0x9aU, 0x78U, 0x56U, 0x34U, 0x12U
    };
    const uint64_t initial_low = 0x2233445566778899ULL;
    const uint64_t initial_high = 0xaabbccddeeff0011ULL;
    const uint64_t updated_low = 0x76543210fedcba98ULL;
    const uint64_t adjacent_low = 0x02468ace13579bdfULL;
    const uint64_t adjacent_high = 0x99aabbccddeeff00ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(source_pattern); ++i) {
        ((volatile uint8_t *)source_paddr)[i] = source_pattern[i];
    }
    write64(fault_paddr, initial_low);
    write64(fault_paddr + sizeof(uint64_t), initial_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("a vector unit-stride load can stage the reference bytes before the repeated PBMT=IO vector-load fault then scalar-recovery probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)fault_vaddr, 16);
    TEST_ASSERT("the first vector unit-stride load from a PBMT=IO alias reports load access fault before the later scalar same-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)fault_vaddr, 16);
    TEST_ASSERT("repeating the same vector unit-stride load from a PBMT=IO alias still reports load access fault before the later scalar same-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(fault_vaddr);
    TEST_ASSERT("after the repeated PBMT=IO vector-load faults, a normal scalar load through the same alias still returns the original low doubleword",
        excpt.triggered == false &&
        observed_before == initial_low
    );

    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, updated_low);
    TEST_ASSERT("after the repeated PBMT=IO vector-load faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(fault_vaddr);
    TEST_ASSERT("after the scalar recovery store on the PBMT=IO alias, a later scalar load through the same alias observes the updated low doubleword immediately",
        excpt.triggered == false &&
        observed_after == updated_low
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_high = ld(second_word_vaddr);
    TEST_ASSERT("the repeated PBMT=IO vector-load faults and the later scalar recovery store leave the high doubleword in the same 16-byte target region unchanged",
        excpt.triggered == false &&
        observed_high == initial_high
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=IO vector-load faults and the later scalar recovery store also leave the adjacent 16-byte region unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_low
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=IO vector-load faults do not poison the later scalar same-alias path, so backing memory ends with only the low doubleword updated",
        read64(fault_paddr) == updated_low &&
        read64(fault_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_vector_store_pbmt_io_repeated_access_fault_then_scalar_same_alias_recovers_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH2) + 0x1e0U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH2) + 0x1e0U;
    const uintptr_t target_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x400U;
    const uintptr_t target_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(
        VSRWXPbmt2_GURWXPbmt1) + 0x400U;
    const uintptr_t second_word_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = target_paddr + 16U;
    const uintptr_t adjacent_vaddr = target_vaddr + 16U;
    const uint8_t source_pattern[16] = {
        0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU,
        0xf0U, 0xdeU, 0xbcU, 0x9aU, 0x78U, 0x56U, 0x34U, 0x12U
    };
    const uint64_t initial_low = 0x7766554433221100ULL;
    const uint64_t initial_high = 0x99aabbccddeeff00ULL;
    const uint64_t updated_low = 0x76543210fedcba98ULL;
    const uint64_t adjacent_low = 0x02468ace13579bdfULL;
    const uint64_t adjacent_high = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(source_pattern); ++i) {
        ((volatile uint8_t *)source_paddr)[i] = source_pattern[i];
    }
    write64(target_paddr, initial_low);
    write64(target_paddr + sizeof(uint64_t), initial_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("a vector unit-stride load can stage the source bytes before the repeated PBMT=IO vector-store fault then scalar-recovery probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)target_vaddr, 16);
    TEST_ASSERT("the first vector unit-stride store to a PBMT=IO alias reports store access fault before the later scalar same-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    AI_ASSERT_NO_EXCEPT_AND("the source bytes can be reloaded into the vector register before the repeated PBMT=IO vector-store fault then scalar-recovery probe",
        true
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)target_vaddr, 16);
    TEST_ASSERT("repeating the same vector unit-stride store to a PBMT=IO alias still reports store access fault before the later scalar same-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(target_vaddr);
    TEST_ASSERT("after the repeated PBMT=IO vector-store faults, a normal scalar load through the same alias still returns the original low doubleword",
        excpt.triggered == false &&
        observed_before == initial_low
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, updated_low);
    TEST_ASSERT("after the repeated PBMT=IO vector-store faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(target_vaddr);
    TEST_ASSERT("after the scalar recovery store on the PBMT=IO alias, a later scalar load through the same alias observes the updated low doubleword immediately",
        excpt.triggered == false &&
        observed_after == updated_low
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_high = ld(second_word_vaddr);
    TEST_ASSERT("the repeated PBMT=IO vector-store faults and the later scalar recovery store leave the high doubleword in the same 16-byte target region unchanged",
        excpt.triggered == false &&
        observed_high == initial_high
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=IO vector-store faults and the later scalar recovery store also leave the adjacent 16-byte region unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_low
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=IO vector-store faults do not poison the later scalar same-alias path, so backing memory ends with only the low doubleword updated",
        read64(target_paddr) == updated_low &&
        read64(target_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

static bool ai_micro_pbmt_dual_nc_load_same_window(bool extra_young_pressure)
{
    bool test_status = true;
    static const uint64_t old_values[] __attribute__((aligned(64))) = {
        0x1111222233334444ULL, 0x5555666677778888ULL,
        0x9999aaaabbbbccccULL, 0xddddeeeeffff0001ULL
    };
    static const uint64_t young_values[] __attribute__((aligned(64))) = {
        0x0123456789abcdefULL, 0xfedcba9876543210ULL,
        0x89abcdef01234567ULL, 0x76543210fedcba98ULL
    };
    const enum test_page old_page = VSRWXPbmt1_GURWXPbmt0;
    const enum test_page young_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t old_paddr = (uintptr_t)&old_values[0];
    const uintptr_t young_paddr = (uintptr_t)&young_values[0];

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_uncache_map_alias_to_paddr(old_page, old_paddr);
    ai_micro_pbmt_uncache_map_alias_to_paddr(young_page, young_paddr);
    sfence_vma();

    for (unsigned i = 0; i < sizeof(old_values) / sizeof(old_values[0]); ++i) {
        const uintptr_t old_vaddr = ai_micro_pbmt_uncache_alias_vaddr(
            old_page, old_paddr + i * sizeof(uint64_t));
        const uintptr_t young_vaddr = ai_micro_pbmt_uncache_alias_vaddr(
            young_page, young_paddr + i * sizeof(uint64_t));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t observed_old = ld(old_vaddr);
        TEST_ASSERT("the older PBMT=NC load in the dual-load same-window probe returns the statically backed payload first without relying on cacheable alias visibility",
            excpt.triggered == false &&
            observed_old == old_values[i]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_young = ld(young_vaddr);
        TEST_ASSERT("the following younger PBMT=NC load in the same window returns its own statically backed payload instead of reusing the older response context",
            excpt.triggered == false &&
            observed_young == young_values[i]
        );

        if (extra_young_pressure) {
            TEST_SETUP_EXCEPT();
            uint64_t observed_young_again = ld(young_vaddr);
            TEST_ASSERT("repeating the younger PBMT=NC load under the same dual-load pressure window still returns the younger payload and does not starve or swap with the older request",
                excpt.triggered == false &&
                observed_young_again == young_values[i]
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_old_again = ld(old_vaddr);
            TEST_ASSERT("after the repeated younger-pressure accesses, the older PBMT=NC load still returns its own payload immediately when reissued",
                excpt.triggered == false &&
                observed_old_again == old_values[i]
            );
        } else {
            TEST_SETUP_EXCEPT();
            uint64_t observed_old_again = ld(old_vaddr);
            TEST_ASSERT("re-reading the older PBMT=NC target in the same dual-load window still sees the original older payload and does not inherit the younger response",
                excpt.triggered == false &&
                observed_old_again == old_values[i]
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_young_again = ld(young_vaddr);
            TEST_ASSERT("re-reading the younger PBMT=NC target in the same dual-load window still sees the original younger payload and does not inherit the older response",
                excpt.triggered == false &&
                observed_young_again == young_values[i]
            );
        }
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated dual PBMT=NC load windows, the static old/young backing slots still hold their independent payloads",
        read64(old_paddr + (sizeof(old_values) / sizeof(old_values[0]) - 1U) * sizeof(uint64_t)) ==
            old_values[(sizeof(old_values) / sizeof(old_values[0])) - 1U] &&
        read64(young_paddr + (sizeof(young_values) / sizeof(young_values[0]) - 1U) * sizeof(uint64_t)) ==
            young_values[(sizeof(young_values) / sizeof(young_values[0])) - 1U]
    );
    ai_micro_reset_pbmt_uncache_env();

    return test_status;
}

static bool ai_micro_pbmt_dual_io_load_same_window(bool extra_young_pressure)
{
    bool test_status = true;
    const uint64_t old_values[] = {
        0x1111222233334444ULL, 0x5555666677778888ULL,
        0x9999aaaabbbbccccULL, 0xddddeeeeffff0001ULL
    };
    const uint64_t young_values[] = {
        0x0123456789abcdefULL, 0xfedcba9876543210ULL,
        0x89abcdef01234567ULL, 0x76543210fedcba98ULL
    };
    const enum test_page old_page = VSRWXPbmt2_GURWXPbmt0;
    const enum test_page young_page = VSRWXPbmt2_GURWXPbmt1;
    const uintptr_t old_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x500U;
    const uintptr_t young_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x580U;
    const uintptr_t old_vaddr = ai_micro_pbmt_uncache_alias_vaddr(old_page, old_paddr);
    const uintptr_t young_vaddr = ai_micro_pbmt_uncache_alias_vaddr(young_page, young_paddr);

    ai_micro_prepare_pbmt_mem_3t_4t_window();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_uncache_map_alias_to_paddr(old_page, old_paddr);
    ai_micro_pbmt_uncache_map_alias_to_paddr(young_page, young_paddr);
    sfence_vma();

    for (unsigned i = 0; i < sizeof(old_values) / sizeof(old_values[0]); ++i) {
        goto_priv(PRIV_M);
        write64(old_paddr, old_values[i]);
        write64(young_paddr, young_values[i]);
        fence_iorw_iorw();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t observed_old = ld(old_vaddr);
        TEST_ASSERT("the older PBMT=IO load in the dual-load same-window probe returns its own device-window payload first",
            excpt.triggered == false &&
            observed_old == old_values[i]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_young = ld(young_vaddr);
        TEST_ASSERT("the following younger PBMT=IO load in the same window returns its own device-window payload instead of reusing the older response context",
            excpt.triggered == false &&
            observed_young == young_values[i]
        );

        if (extra_young_pressure) {
            TEST_SETUP_EXCEPT();
            uint64_t observed_young_again = ld(young_vaddr);
            TEST_ASSERT("repeating the younger PBMT=IO load under the same device-window pressure still returns the younger payload and does not starve or swap with the older request",
                excpt.triggered == false &&
                observed_young_again == young_values[i]
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_old_again = ld(old_vaddr);
            TEST_ASSERT("after repeated younger PBMT=IO pressure, the older device-window load still returns its own payload immediately when reissued",
                excpt.triggered == false &&
                observed_old_again == old_values[i]
            );
        } else {
            TEST_SETUP_EXCEPT();
            uint64_t observed_old_again = ld(old_vaddr);
            TEST_ASSERT("re-reading the older PBMT=IO target in the same device-window load window still sees the original older payload and does not inherit the younger response",
                excpt.triggered == false &&
                observed_old_again == old_values[i]
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_young_again = ld(young_vaddr);
            TEST_ASSERT("re-reading the younger PBMT=IO target in the same device-window load window still sees the original younger payload and does not inherit the older response",
                excpt.triggered == false &&
                observed_young_again == young_values[i]
            );
        }
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated dual PBMT=IO load windows, both device-window backing locations still hold their latest independent payloads",
        read64(old_paddr) == old_values[(sizeof(old_values) / sizeof(old_values[0])) - 1U] &&
        read64(young_paddr) == young_values[(sizeof(young_values) / sizeof(young_values[0])) - 1U]
    );
    ai_micro_reset_pbmt_uncache_env();

    return test_status;
}

bool ai_micro_hs_robhead_two_mmio_replay_entries_same_row_serialize_by_uopidx_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_pbmt_dual_io_load_same_window(false);
    TEST_END();
}

bool ai_micro_hs_same_rob_row_dual_uncache_load_replay_no_popcount_assert_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_pbmt_dual_nc_load_same_window(true);
    TEST_END();
}

bool ai_micro_hs_replayq_head_mmio_not_freed_when_direct_exu_mmio_wins_uncachebuffer_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_pbmt_dual_io_load_same_window(false);
    TEST_END();
}

bool ai_micro_hs_shared_uncachebuffer_mmio_mux_no_false_schedule_or_free_of_replay_head_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_pbmt_dual_io_load_same_window(true);
    TEST_END();
}

bool ai_micro_hs_replayq_delayed_mmio_request_not_rebound_to_reused_entry_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWXPbmt2_GURWXPbmt0;
    const enum test_page young_page = VSRWXPbmt2_GURWXPbmt1;
    const uintptr_t old_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x640U;
    const uintptr_t young_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x680U;
    const uintptr_t young_adjacent_paddr = young_paddr + sizeof(uint64_t);
    const uintptr_t old_vaddr = ai_micro_pbmt_uncache_alias_vaddr(old_page, old_paddr);
    const uintptr_t young_vaddr = ai_micro_pbmt_uncache_alias_vaddr(young_page, young_paddr);
    const uintptr_t young_adjacent_vaddr = ai_micro_pbmt_uncache_alias_vaddr(
        young_page, young_adjacent_paddr);
    const uint64_t old_value = 0x1032547698badcfeULL;
    const uint64_t young_initial = 0x2233445566778899ULL;
    const uint64_t young_updated = 0x76543210fedcba98ULL;
    const uint64_t young_adjacent = 0xa1b2c3d4e5f60718ULL;

    ai_micro_prepare_pbmt_mem_3t_4t_window();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_uncache_map_alias_to_paddr(old_page, old_paddr);
    ai_micro_pbmt_uncache_map_alias_to_paddr(young_page, young_paddr);
    write64(old_paddr, old_value);
    write64(young_paddr, young_initial);
    write64(young_adjacent_paddr, young_adjacent);
    *ai_micro_pbmt_hs_leaf_entry(old_page) = 0ULL;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(old_vaddr);
    TEST_ASSERT("the older invalid-leaf PBMT=IO load first reports load page fault while the delayed-replay ownership probe keeps only the younger device-window alias live",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == old_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(old_vaddr);
    TEST_ASSERT("repeating the same older invalid-leaf PBMT=IO load still reports the older page fault instead of rebinding to the younger device-window episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == old_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_young_before = ld(young_vaddr);
    TEST_ASSERT("while the older PBMT=IO alias is still invalid, the younger live PBMT=IO alias first returns only its own device-window payload",
        excpt.triggered == false &&
        observed_young_before == young_initial
    );

    TEST_SETUP_EXCEPT();
    sd(young_vaddr, young_updated);
    TEST_ASSERT("the younger live PBMT=IO alias can then complete its own direct store without exception in the delayed-rebind device-window probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_micro_pbmt_uncache_map_alias_to_paddr(old_page, old_paddr);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_old = ld(old_vaddr);
    TEST_ASSERT("after repairing only the older PBMT=IO alias, the recovered older load still returns only its own device-window payload rather than the younger alias metadata",
        excpt.triggered == false &&
        observed_old == old_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_old_again = ld(old_vaddr);
    TEST_ASSERT("reissuing the recovered older PBMT=IO alias still returns the same older payload and does not inherit the younger update",
        excpt.triggered == false &&
        observed_old_again == old_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_young_after = ld(young_vaddr);
    TEST_ASSERT("the younger PBMT=IO alias still retains its own later direct-store payload after the older alias recovers",
        excpt.triggered == false &&
        observed_young_after == young_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_young_adjacent = ld(young_adjacent_vaddr);
    TEST_ASSERT("the nearby younger PBMT=IO word remains unchanged across the delayed-rebind device-window window",
        excpt.triggered == false &&
        observed_young_adjacent == young_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the older repeated PBMT=IO page-fault episode and the younger direct PBMT=IO update, the older and younger device-window backing locations still keep only their own final payloads",
        read64(old_paddr) == old_value &&
        read64(young_paddr) == young_updated &&
        read64(young_adjacent_paddr) == young_adjacent
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

static bool ai_micro_pbmt_misalign_and_nc_same_window(
    bool extra_uncache_pressure)
{
    bool test_status = true;
    const enum test_page older_page = VSI_GI;
    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t older_vaddr = ai_micro_pbmt_cross_16b_addr(older_page);
    const uintptr_t older_phys = phys_page_base(older_page);
    const uintptr_t older_leading_phys = older_phys;
    const uintptr_t older_low_phys = older_phys + 8U;
    const uintptr_t older_high_phys = older_phys + 16U;
    const uintptr_t older_trailing_phys = older_phys + 24U;
    static const uint64_t nc_window[] __attribute__((aligned(64))) = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
        0xddddeeeeffff0001ULL,
        0x123456789abcdef0ULL
    };
    const unsigned nc_value_count = 4U;
    const uintptr_t nc_paddr = (uintptr_t)&nc_window[0];
    const uintptr_t nc_adjacent_paddr = (uintptr_t)&nc_window[nc_value_count];
    const uintptr_t nc_base_vaddr = ai_micro_pbmt_uncache_alias_vaddr(
        nc_page, nc_paddr);
    const uintptr_t nc_adjacent_vaddr = ai_micro_pbmt_uncache_alias_vaddr(
        nc_page, nc_adjacent_paddr);
    const uint64_t older_leading_pattern = 0x1020304050607080ULL;
    const uint64_t older_low_pattern = 0x8172635445362718ULL;
    const uint64_t older_high_pattern = 0x90a0b0c0d0e0f000ULL;
    const uint64_t older_trailing_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t older_expected = ai_micro_pbmt_cross_16b_expected_ld_value(
        older_low_pattern, older_high_pattern);
    const uint64_t nc_adjacent_pattern = 0x123456789abcdef0ULL;
    uint64_t observed_old = 0ULL;
    uint64_t observed_nc = 0ULL;

    goto_priv(PRIV_M);
    write64(older_leading_phys, older_leading_pattern);
    write64(older_low_phys, older_low_pattern);
    write64(older_high_phys, older_high_pattern);
    write64(older_trailing_phys, older_trailing_pattern);

    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_uncache_map_alias_to_paddr(nc_page, nc_paddr);
    *ai_micro_pbmt_hs_leaf_entry(older_page) = 0ULL;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(older_vaddr);
    TEST_ASSERT("the older cacheable cross-16B load first reports load page fault before any same-window PBMT=NC service is observed in the mixed replay probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == older_vaddr
    );

    TEST_SETUP_EXCEPT();
    observed_nc = ld(nc_base_vaddr);
    TEST_ASSERT("while the older misalign-path load remains faulting, the PBMT=NC load in the same execution window still returns only its own statically backed payload",
        excpt.triggered == false &&
        observed_nc == nc_window[0]
    );

    for (unsigned i = 1; i < nc_value_count; ++i) {
        const uintptr_t nc_vaddr = ai_micro_pbmt_uncache_alias_vaddr(
            nc_page, nc_paddr + i * sizeof(uint64_t));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ld(older_vaddr);
        TEST_ASSERT("reissuing the still-invalid older cross-16B load continues to report its own load page fault instead of being dropped or rebound to the PBMT=NC request context",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == older_vaddr
        );

        TEST_SETUP_EXCEPT();
        observed_nc = ld(nc_vaddr);
        TEST_ASSERT("after each repeated older misalign fault episode, the PBMT=NC load still observes its selected static backing payload without swapping with the older request",
            excpt.triggered == false &&
            observed_nc == nc_window[i]
        );

        if (extra_uncache_pressure) {
            TEST_SETUP_EXCEPT();
            observed_nc = ld(nc_vaddr);
            TEST_ASSERT("with extra younger-side PBMT=NC pressure applied in the same mixed window, the NC load still completes immediately with its own payload",
                excpt.triggered == false &&
                observed_nc == nc_window[i]
            );
        }
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the older invalid leaf, the mixed misalign-plus-PBMT=NC probe leaves both the older split-line backing bytes and the NC adjacent word unchanged",
        read64(older_leading_phys) == older_leading_pattern &&
        read64(older_low_phys) == older_low_pattern &&
        read64(older_high_phys) == older_high_pattern &&
        read64(older_trailing_phys) == older_trailing_pattern &&
        read64(nc_paddr + (nc_value_count - 1U) * sizeof(uint64_t)) ==
            nc_window[nc_value_count - 1U] &&
        read64(nc_adjacent_paddr) == nc_adjacent_pattern
    );

    ai_micro_pbmt_set_hs_leaf(older_page, older_phys, PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed_old = ld(older_vaddr);
    TEST_ASSERT("after repairing only the older cacheable leaf, the older cross-16B load recovers with its own merged payload rather than any PBMT=NC response context",
        excpt.triggered == false &&
        observed_old == older_expected
    );

    TEST_SETUP_EXCEPT();
    observed_nc = ld(ai_micro_pbmt_uncache_alias_vaddr(
        nc_page, nc_paddr + (nc_value_count - 1U) * sizeof(uint64_t)));
    TEST_ASSERT("after the older cross-16B load recovers, the PBMT=NC alias still returns the final selected static NC payload and does not inherit older misalign state",
        excpt.triggered == false &&
        observed_nc == nc_window[nc_value_count - 1U]
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_nc_adjacent = ld(nc_adjacent_vaddr);
    TEST_ASSERT("the mixed replay-service serialization probe also leaves the adjacent PBMT=NC word unchanged throughout recovery",
        excpt.triggered == false &&
        observed_nc_adjacent == nc_adjacent_pattern
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("through the mixed misalign-fault and PBMT=NC service window, the older split-line backing data and the NC line contents remain independently intact after recovery",
        read64(older_leading_phys) == older_leading_pattern &&
        read64(older_low_phys) == older_low_pattern &&
        read64(older_high_phys) == older_high_pattern &&
        read64(older_trailing_phys) == older_trailing_pattern &&
        read64(nc_paddr + (nc_value_count - 1U) * sizeof(uint64_t)) ==
            nc_window[nc_value_count - 1U] &&
        read64(nc_adjacent_paddr) == nc_adjacent_pattern
    );
    ai_micro_reset_pbmt_uncache_env();

    return test_status;
}

bool ai_micro_hs_robhead_mmio_and_misalign_replay_same_window_no_assert_or_drop_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_pbmt_misalign_and_nc_same_window(false);
    TEST_END();
}

bool ai_micro_hs_replayq_stale_mmio_plus_fresh_misalign_head_service_serializes_correctly_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_pbmt_misalign_and_nc_same_window(true);
    TEST_END();
}

bool ai_micro_hs_uncache_load_store_same_window_no_starvation_corner()
{
    TEST_START();

    static const uint64_t load_values[] __attribute__((aligned(64))) = {
        0x1111222233334444ULL, 0x5555666677778888ULL,
        0x9999aaaabbbbccccULL, 0xddddeeeeffff0001ULL
    };
    const uint64_t store_values[] = {
        0x0123456789abcdefULL, 0xfedcba9876543210ULL,
        0x89abcdef01234567ULL, 0x76543210fedcba98ULL
    };
    const uintptr_t nc_paddr = (uintptr_t)&load_values[0];
    const uintptr_t io_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x300U;
    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page io_page = VSRWXPbmt2_GURWXPbmt1;
    const uintptr_t io_vaddr = hs_page_base(io_page) + 0x300U;

    ai_micro_prepare_pbmt_mem_3t_4t_window();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_uncache_map_alias_to_paddr(nc_page, nc_paddr);
    pbmt_hspt_to_x_base_paddr(io_page, AI_MICRO_PBMT_IO_MEM_BASE);
    sfence_vma();

    for (unsigned i = 0; i < sizeof(load_values) / sizeof(load_values[0]); ++i) {
        const uintptr_t nc_vaddr = ai_micro_pbmt_uncache_alias_vaddr(
            nc_page, nc_paddr + i * sizeof(uint64_t));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t observed_load = ld(nc_vaddr);
        TEST_ASSERT("the shared uncache port still lets the NC-side load complete and observe the statically backed payload in the alternating load/store window",
            excpt.triggered == false &&
            observed_load == load_values[i]
        );

        TEST_SETUP_EXCEPT();
        sd(io_vaddr, store_values[i]);
        TEST_ASSERT("the shared uncache port still lets the IO-side store retire in the same alternating load/store window",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alternating NC-load plus IO-store pressure window, both uncache paths make forward progress and leave the expected final backing values",
        read64(nc_paddr + (sizeof(load_values) / sizeof(load_values[0]) - 1U) *
            sizeof(uint64_t)) == load_values[(sizeof(load_values) / sizeof(load_values[0])) - 1U] &&
        read64(io_paddr) == store_values[(sizeof(store_values) / sizeof(store_values[0])) - 1U]
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_uncache_load_store_back_to_back_response_not_swapped_corner()
{
    TEST_START();

    const uint64_t nc_store_values[] = {
        0x0f1e2d3c4b5a6978ULL, 0x1021324354657687ULL,
        0x8877665544332211ULL, 0xa1b2c3d4e5f60718ULL
    };
    const uint64_t io_load_values[] = {
        0x13579bdf2468ace0ULL, 0x02468ace13579bdfULL,
        0xf0e1d2c3b4a59687ULL, 0x89abcdef76543210ULL
    };
    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page io_page = VSRWXPbmt2_GURWXPbmt1;
    const uintptr_t nc_paddr = ai_micro_pbmt_uncache_backing_paddr() + 0x340U;
    const uintptr_t io_paddr = ai_micro_pbmt_uncache_resp_mmio64_paddr(0x380U);
    const uintptr_t nc_vaddr = ai_micro_pbmt_uncache_alias_vaddr(nc_page, nc_paddr);
    const uintptr_t io_vaddr = ai_micro_pbmt_uncache_alias_vaddr(io_page, io_paddr);

    ai_micro_reset_pbmt_uncache_mmio_env();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_uncache_map_alias_to_paddr(nc_page, nc_paddr);
    ai_micro_pbmt_uncache_map_alias_to_paddr(io_page, io_paddr);
    sfence_vma();

    for (unsigned i = 0; i < sizeof(nc_store_values) / sizeof(nc_store_values[0]); ++i) {
        goto_priv(PRIV_M);
        write64(io_paddr, io_load_values[i]);
        fence_iorw_iorw();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(nc_vaddr, nc_store_values[i]);
        TEST_ASSERT("the NC-side store still retires before the following IO-side load in the back-to-back response-typing window",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_io = ld(io_vaddr);
        TEST_ASSERT("the following IO-side load still returns its own backing payload instead of any stale or swapped store-side information",
            excpt.triggered == false &&
            observed_io == io_load_values[i]
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the back-to-back NC-store plus IO-load response-typing window, the final NC store payload and IO load backing payload both remain correct",
        read64(nc_paddr) == nc_store_values[(sizeof(nc_store_values) / sizeof(nc_store_values[0])) - 1U] &&
        read64(io_paddr) == io_load_values[(sizeof(io_load_values) / sizeof(io_load_values[0])) - 1U]
    );
    ai_micro_reset_pbmt_uncache_mmio_env();

    TEST_END();
}

bool ai_micro_hs_fence_waits_for_sbuffer_and_pbmt_nc_uncache_empty_corner()
{
    TEST_START();

    const uint64_t cacheable_values[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL
    };
    const uint64_t uncache_values[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0xa1b2c3d4e5f60718ULL
    };
    const uintptr_t cacheable_paddr = phys_page_base(SWITCH1) + 0x3c0U;
    const uintptr_t cacheable_vaddr = hs_page_base(SWITCH1) + 0x3c0U;
    static uint64_t uncache_window[] = {
        0x1021324354657687ULL,
        0x8877665544332211ULL
    };
    const uintptr_t uncache_paddr = (uintptr_t)&uncache_window[0];
    const uintptr_t uncache_vaddr = ai_micro_prepare_pbmt_uncache_alias_to_paddr(
        VSRWXPbmt1_GURWXPbmt1, uncache_paddr);
    const uintptr_t cacheable_adjacent_paddr = cacheable_paddr + sizeof(uint64_t);
    const uintptr_t cacheable_adjacent_vaddr = cacheable_vaddr + sizeof(uint64_t);
    const uintptr_t uncache_adjacent_paddr = (uintptr_t)&uncache_window[1];
    const uintptr_t uncache_adjacent_vaddr = uncache_vaddr + sizeof(uint64_t);
    const uint64_t cacheable_adjacent = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t uncache_adjacent = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(cacheable_paddr, 0x0102030405060708ULL);
    write64(cacheable_adjacent_paddr, cacheable_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm_cacheable = ld(cacheable_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable HS load can first observe the original payload before the mixed sbuffer-plus-PBMT=NC fence-drain probe",
        warm_cacheable == 0x0102030405060708ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_uncache = ld(uncache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC HS load can first observe the original payload before the mixed sbuffer-plus-PBMT=NC fence-drain probe",
        warm_uncache == 0x1021324354657687ULL
    );

    for (unsigned i = 0; i < sizeof(cacheable_values) / sizeof(cacheable_values[0]); ++i) {
        TEST_SETUP_EXCEPT();
        sd(cacheable_vaddr, cacheable_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the cacheable HS store retires before the mixed sbuffer-plus-PBMT=NC fence-drain probe",
            true
        );

        TEST_SETUP_EXCEPT();
        sd(uncache_vaddr, uncache_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC HS store also retires before the mixed sbuffer-plus-PBMT=NC fence-drain probe",
            true
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the cacheable store plus PBMT=NC store completes without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_cacheable = ld(cacheable_vaddr);
        TEST_ASSERT("after the mixed cacheable-store plus PBMT=NC-store fence, the cacheable HS load observes the latest cacheable payload immediately",
            excpt.triggered == false &&
            observed_cacheable == cacheable_values[i]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_uncache = ld(uncache_vaddr);
        TEST_ASSERT("after the mixed cacheable-store plus PBMT=NC-store fence, the PBMT=NC HS load observes the latest uncache payload immediately",
            excpt.triggered == false &&
            observed_uncache == uncache_values[i]
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed_cacheable_adjacent = ld(cacheable_adjacent_vaddr);
    TEST_ASSERT("the adjacent cacheable HS word remains unchanged across the mixed sbuffer-plus-PBMT=NC fence-drain probe",
        excpt.triggered == false &&
        observed_cacheable_adjacent == cacheable_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_uncache_adjacent = ld(uncache_adjacent_vaddr);
    TEST_ASSERT("the adjacent PBMT=NC HS word remains unchanged across the mixed sbuffer-plus-PBMT=NC fence-drain probe",
        excpt.triggered == false &&
        observed_uncache_adjacent == uncache_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t final_uncache = ld(uncache_vaddr);
    uint64_t final_uncache_adjacent = ld(uncache_adjacent_vaddr);
    TEST_ASSERT("after the repeated cacheable-store plus PBMT=NC-store fence-drain window, the PBMT=NC alias still observes the latest payload and adjacent word",
        excpt.triggered == false &&
        final_uncache == uncache_values[(sizeof(uncache_values) / sizeof(uncache_values[0])) - 1U] &&
        final_uncache_adjacent == uncache_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated cacheable-store plus PBMT=NC-store fence-drain window, the cacheable backing location contains its latest payload and adjacent word stays unchanged",
        read64(cacheable_paddr) == cacheable_values[(sizeof(cacheable_values) / sizeof(cacheable_values[0])) - 1U] &&
        read64(cacheable_adjacent_paddr) == cacheable_adjacent
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_fence_waits_for_sbuffer_and_pbmt_io_uncache_empty_corner()
{
    TEST_START();

    const uint64_t cacheable_values[] = {
        0x2233445566778899ULL,
        0xaabbccddeeff0011ULL,
        0x13579bdf2468ace0ULL
    };
    const uint64_t uncache_values[] = {
        0x7766554433221100ULL,
        0x99aabbccddeeff00ULL,
        0x02468ace13579bdfULL
    };
    const uintptr_t cacheable_paddr = phys_page_base(SWITCH1) + 0x420U;
    const uintptr_t cacheable_vaddr = hs_page_base(SWITCH1) + 0x420U;
    const uintptr_t uncache_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x420U;
    const uintptr_t uncache_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(
        VSRWXPbmt2_GURWXPbmt1) + 0x420U;
    const uintptr_t cacheable_adjacent_paddr = cacheable_paddr + sizeof(uint64_t);
    const uintptr_t cacheable_adjacent_vaddr = cacheable_vaddr + sizeof(uint64_t);
    const uintptr_t uncache_adjacent_paddr = uncache_paddr + sizeof(uint64_t);
    const uintptr_t uncache_adjacent_vaddr = uncache_vaddr + sizeof(uint64_t);
    const uint64_t cacheable_adjacent = 0xf0e1d2c3b4a59687ULL;
    const uint64_t uncache_adjacent = 0x89abcdef76543210ULL;

    goto_priv(PRIV_M);
    write64(cacheable_paddr, 0x0123456789abcdefULL);
    write64(uncache_paddr, 0xfedcba9876543210ULL);
    write64(cacheable_adjacent_paddr, cacheable_adjacent);
    write64(uncache_adjacent_paddr, uncache_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm_cacheable = ld(cacheable_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable HS load can first observe the original payload before the mixed sbuffer-plus-PBMT=IO fence-drain probe",
        warm_cacheable == 0x0123456789abcdefULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_uncache = ld(uncache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=IO HS load can first observe the original payload before the mixed sbuffer-plus-PBMT=IO fence-drain probe",
        warm_uncache == 0xfedcba9876543210ULL
    );

    for (unsigned i = 0; i < sizeof(cacheable_values) / sizeof(cacheable_values[0]); ++i) {
        TEST_SETUP_EXCEPT();
        sd(cacheable_vaddr, cacheable_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the cacheable HS store retires before the mixed sbuffer-plus-PBMT=IO fence-drain probe",
            true
        );

        TEST_SETUP_EXCEPT();
        sd(uncache_vaddr, uncache_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the PBMT=IO HS store also retires before the mixed sbuffer-plus-PBMT=IO fence-drain probe",
            true
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the cacheable store plus PBMT=IO store completes without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_cacheable = ld(cacheable_vaddr);
        TEST_ASSERT("after the mixed cacheable-store plus PBMT=IO-store fence, the cacheable HS load observes the latest cacheable payload immediately",
            excpt.triggered == false &&
            observed_cacheable == cacheable_values[i]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_uncache = ld(uncache_vaddr);
        TEST_ASSERT("after the mixed cacheable-store plus PBMT=IO-store fence, the PBMT=IO HS load observes the latest IO-style payload immediately",
            excpt.triggered == false &&
            observed_uncache == uncache_values[i]
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed_cacheable_adjacent = ld(cacheable_adjacent_vaddr);
    TEST_ASSERT("the adjacent cacheable HS word remains unchanged across the mixed sbuffer-plus-PBMT=IO fence-drain probe",
        excpt.triggered == false &&
        observed_cacheable_adjacent == cacheable_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_uncache_adjacent = ld(uncache_adjacent_vaddr);
    TEST_ASSERT("the adjacent PBMT=IO HS word remains unchanged across the mixed sbuffer-plus-PBMT=IO fence-drain probe",
        excpt.triggered == false &&
        observed_uncache_adjacent == uncache_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated cacheable-store plus PBMT=IO-store fence-drain window, both backing locations contain their latest payloads and both adjacent words stay unchanged",
        read64(cacheable_paddr) == cacheable_values[(sizeof(cacheable_values) / sizeof(cacheable_values[0])) - 1U] &&
        read64(uncache_paddr) == uncache_values[(sizeof(uncache_values) / sizeof(uncache_values[0])) - 1U] &&
        read64(cacheable_adjacent_paddr) == cacheable_adjacent &&
        read64(uncache_adjacent_paddr) == uncache_adjacent
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_fence_with_cbo_zero_and_pbmt_nc_store_same_window_corner()
{
    TEST_START();

    const uintptr_t cacheable_store_paddr = phys_page_base(SWITCH1) + 0x480U;
    const uintptr_t cacheable_store_vaddr = hs_page_base(SWITCH1) + 0x480U;
    const uintptr_t cmo_block_paddr = phys_page_base(SWITCH1) + 0x500U;
    const uintptr_t cmo_block_vaddr = hs_page_base(SWITCH1) + 0x500U;
    static uint64_t uncache_window[] = {
        0x1021324354657687ULL,
        0x0fedcba987654321ULL
    };
    const uintptr_t uncache_paddr = (uintptr_t)&uncache_window[0];
    const uintptr_t uncache_vaddr = ai_micro_prepare_pbmt_uncache_alias_to_paddr(
        VSRWXPbmt1_GURWXPbmt1, uncache_paddr);
    const uintptr_t uncache_adjacent_vaddr = uncache_vaddr + sizeof(uint64_t);
    const uint64_t cacheable_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t uncache_store_value = 0x0123456789abcdefULL;
    const uint64_t cmo_seed = 0x8877665544332211ULL;
    const uint64_t uncache_adjacent = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    write64(cacheable_store_paddr, 0x1122334455667788ULL);
    ai_micro_pbmt_uncache_seed_block(cmo_block_paddr, cmo_seed);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm_store = ld(cacheable_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable HS load can first observe the original payload before the cbo.zero-plus-PBMT=NC mixed fence window",
        warm_store == 0x1122334455667788ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_uncache = ld(uncache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC HS load can first observe the original payload before the cbo.zero-plus-PBMT=NC mixed fence window",
        warm_uncache == 0x1021324354657687ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_cmo_head = ld(cmo_block_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable block targeted by cbo.zero is readable before the cbo.zero-plus-PBMT=NC mixed fence window",
        warm_cmo_head == cmo_seed
    );

    TEST_SETUP_EXCEPT();
    sd(cacheable_store_vaddr, cacheable_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable HS store retires before the cbo.zero-plus-PBMT=NC mixed fence window",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(uncache_vaddr, uncache_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC HS store retires before the cbo.zero-plus-PBMT=NC mixed fence window",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(cmo_block_vaddr + 24U);
    AI_ASSERT_NO_EXCEPT_AND("the HS cbo.zero retires before the mixed fence window that also contains a PBMT=NC store",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the cacheable store, PBMT=NC store, and cbo.zero completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_store = ld(cacheable_store_vaddr);
    TEST_ASSERT("after the cacheable store plus PBMT=NC store plus cbo.zero sequence and the following fence, the cacheable store payload is still visible immediately",
        excpt.triggered == false &&
        observed_store == cacheable_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_uncache = ld(uncache_vaddr);
    TEST_ASSERT("after the cacheable store plus PBMT=NC store plus cbo.zero sequence and the following fence, the PBMT=NC store payload is still visible immediately",
        excpt.triggered == false &&
        observed_uncache == uncache_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_uncache_adjacent = ld(uncache_adjacent_vaddr);
    TEST_ASSERT("the adjacent PBMT=NC shared word remains unchanged across the cbo.zero-plus-PBMT=NC mixed fence window",
        excpt.triggered == false &&
        observed_uncache_adjacent == uncache_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_cmo_head = ld(cmo_block_vaddr);
    TEST_ASSERT("after the mixed fence window, the head of the cbo.zero target block reads back as zero",
        excpt.triggered == false &&
        observed_cmo_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_cmo_tail = ld(cmo_block_vaddr + AI_MICRO_PBMT_CACHE_BLOCK_BYTES - sizeof(uint64_t));
    TEST_ASSERT("after the mixed fence window, the tail of the cbo.zero target block also reads back as zero",
        excpt.triggered == false &&
        observed_cmo_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t final_uncache_alias = ld(uncache_vaddr);
    uint64_t final_uncache_adjacent_alias = ld(uncache_adjacent_vaddr);
    TEST_ASSERT("after the final mixed fence window, the PBMT=NC alias still observes the stored payload and its adjacent word is preserved",
        excpt.triggered == false &&
        final_uncache_alias == uncache_store_value &&
        final_uncache_adjacent_alias == uncache_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the mixed cacheable-store plus PBMT=NC-store plus cbo.zero fence window leaves the cacheable store updated and the cbo.zero block fully zeroed without requiring a cacheable alias reread of the NC backing word",
        read64(cacheable_store_paddr) == cacheable_store_value &&
        ai_micro_pbmt_uncache_block_all_zero(cmo_block_paddr)
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_fence_with_cbo_zero_and_pbmt_io_store_same_window_corner()
{
    TEST_START();

    const uintptr_t cacheable_store_paddr = phys_page_base(SWITCH1) + 0x580U;
    const uintptr_t cacheable_store_vaddr = hs_page_base(SWITCH1) + 0x580U;
    const uintptr_t cmo_block_paddr = phys_page_base(SWITCH1) + 0x5c0U;
    const uintptr_t cmo_block_vaddr = hs_page_base(SWITCH1) + 0x5c0U;
    const uintptr_t uncache_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x540U;
    const uintptr_t uncache_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(
        VSRWXPbmt2_GURWXPbmt1) + 0x540U;
    const uintptr_t uncache_adjacent_paddr = uncache_paddr + sizeof(uint64_t);
    const uintptr_t uncache_adjacent_vaddr = uncache_vaddr + sizeof(uint64_t);
    const uint64_t cacheable_store_value = 0x76543210fedcba98ULL;
    const uint64_t uncache_store_value = 0x99aabbccddeeff00ULL;
    const uint64_t cmo_seed = 0x13579bdf2468ace0ULL;
    const uint64_t uncache_adjacent = 0x02468ace13579bdfULL;

    goto_priv(PRIV_M);
    write64(cacheable_store_paddr, 0x2233445566778899ULL);
    ai_micro_pbmt_uncache_seed_block(cmo_block_paddr, cmo_seed);
    write64(uncache_paddr, 0xfedcba9876543210ULL);
    write64(uncache_adjacent_paddr, uncache_adjacent);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm_store = ld(cacheable_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable HS load can first observe the original payload before the cbo.zero-plus-PBMT=IO mixed fence window",
        warm_store == 0x2233445566778899ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_uncache = ld(uncache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=IO HS load can first observe the original payload before the cbo.zero-plus-PBMT=IO mixed fence window",
        warm_uncache == 0xfedcba9876543210ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_cmo_head = ld(cmo_block_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable block targeted by cbo.zero is readable before the cbo.zero-plus-PBMT=IO mixed fence window",
        warm_cmo_head == cmo_seed
    );

    TEST_SETUP_EXCEPT();
    sd(cacheable_store_vaddr, cacheable_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable HS store retires before the cbo.zero-plus-PBMT=IO mixed fence window",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(uncache_vaddr, uncache_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=IO HS store retires before the cbo.zero-plus-PBMT=IO mixed fence window",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(cmo_block_vaddr + 24U);
    AI_ASSERT_NO_EXCEPT_AND("the HS cbo.zero retires before the mixed fence window that also contains a PBMT=IO store",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the cacheable store, PBMT=IO store, and cbo.zero completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_store = ld(cacheable_store_vaddr);
    TEST_ASSERT("after the cacheable store plus PBMT=IO store plus cbo.zero sequence and the following fence, the cacheable store payload is still visible immediately",
        excpt.triggered == false &&
        observed_store == cacheable_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_uncache = ld(uncache_vaddr);
    TEST_ASSERT("after the cacheable store plus PBMT=IO store plus cbo.zero sequence and the following fence, the PBMT=IO store payload is still visible immediately",
        excpt.triggered == false &&
        observed_uncache == uncache_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_uncache_adjacent = ld(uncache_adjacent_vaddr);
    TEST_ASSERT("the adjacent PBMT=IO shared word remains unchanged across the cbo.zero-plus-PBMT=IO mixed fence window",
        excpt.triggered == false &&
        observed_uncache_adjacent == uncache_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_cmo_head = ld(cmo_block_vaddr);
    TEST_ASSERT("after the mixed fence window, the head of the cbo.zero target block reads back as zero in the PBMT=IO variant",
        excpt.triggered == false &&
        observed_cmo_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_cmo_tail = ld(cmo_block_vaddr + AI_MICRO_PBMT_CACHE_BLOCK_BYTES - sizeof(uint64_t));
    TEST_ASSERT("after the mixed fence window, the tail of the cbo.zero target block also reads back as zero in the PBMT=IO variant",
        excpt.triggered == false &&
        observed_cmo_tail == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the mixed cacheable-store plus PBMT=IO-store plus cbo.zero fence window leaves the cacheable store updated, the IO-style store updated, and the cbo.zero block fully zeroed in backing memory",
        read64(cacheable_store_paddr) == cacheable_store_value &&
        read64(uncache_paddr) == uncache_store_value &&
        read64(uncache_adjacent_paddr) == uncache_adjacent &&
        ai_micro_pbmt_uncache_block_all_zero(cmo_block_paddr)
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_fence_with_cacheable_amo_and_pbmt_nc_store_same_window_corner()
{
    TEST_START();

    const uint64_t amo_masks[] = {
        0x00000000000000ffULL,
        0x00000000ff000000ULL,
        0x00ff000000000000ULL
    };
    const uint64_t uncache_values[] = {
        0x7766554433221100ULL,
        0x99aabbccddeeff00ULL,
        0x02468ace13579bdfULL
    };
    const uintptr_t cacheable_paddr = phys_page_base(SWITCH1) + 0x640U;
    const uintptr_t cacheable_vaddr = hs_page_base(SWITCH1) + 0x640U;
    static uint64_t uncache_window[] = {
        0xfedcba9876543210ULL,
        0x89abcdef76543210ULL
    };
    const uintptr_t uncache_paddr = (uintptr_t)&uncache_window[0];
    const uintptr_t uncache_vaddr = ai_micro_prepare_pbmt_uncache_alias_to_paddr(
        VSRWXPbmt1_GURWXPbmt1, uncache_paddr);
    const uintptr_t cacheable_adjacent_paddr = cacheable_paddr + sizeof(uint64_t);
    const uintptr_t cacheable_adjacent_vaddr = cacheable_vaddr + sizeof(uint64_t);
    const uintptr_t uncache_adjacent_paddr = (uintptr_t)&uncache_window[1];
    const uintptr_t uncache_adjacent_vaddr = uncache_vaddr + sizeof(uint64_t);
    const uint64_t initial_cacheable = 0x2233445566778800ULL;
    const uint64_t cacheable_adjacent = 0xf0e1d2c3b4a59687ULL;
    const uint64_t uncache_adjacent = 0x89abcdef76543210ULL;
    uint64_t expected_cacheable = initial_cacheable;

    goto_priv(PRIV_M);
    write64(cacheable_paddr, initial_cacheable);
    write64(cacheable_adjacent_paddr, cacheable_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm_cacheable = ld(cacheable_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable HS load can first observe the original payload before the cacheable-AMO-plus-PBMT=NC mixed fence window",
        warm_cacheable == initial_cacheable
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_uncache = ld(uncache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC HS load can first observe the original payload before the cacheable-AMO-plus-PBMT=NC mixed fence window",
        warm_uncache == 0xfedcba9876543210ULL
    );

    for (unsigned i = 0; i < sizeof(amo_masks) / sizeof(amo_masks[0]); ++i) {
        TEST_SETUP_EXCEPT();
        uint64_t amo_old = amoor_d(cacheable_vaddr, amo_masks[i]);
        TEST_ASSERT("the cacheable HS AMO retires before the mixed fence window that also contains a PBMT=NC store",
            excpt.triggered == false &&
            amo_old == expected_cacheable
        );
        expected_cacheable |= amo_masks[i];

        TEST_SETUP_EXCEPT();
        sd(uncache_vaddr, uncache_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC HS store also retires before the cacheable-AMO-plus-PBMT=NC mixed fence window",
            true
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the cacheable AMO plus PBMT=NC store completes without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_cacheable = ld(cacheable_vaddr);
        TEST_ASSERT("after the cacheable-AMO plus PBMT=NC-store fence window, the cacheable HS load observes the latest AMO-updated payload immediately",
            excpt.triggered == false &&
            observed_cacheable == expected_cacheable
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_uncache = ld(uncache_vaddr);
        TEST_ASSERT("after the cacheable-AMO plus PBMT=NC-store fence window, the PBMT=NC HS load observes the latest uncache payload immediately",
            excpt.triggered == false &&
            observed_uncache == uncache_values[i]
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed_cacheable_adjacent = ld(cacheable_adjacent_vaddr);
    TEST_ASSERT("the adjacent cacheable HS word remains unchanged across the cacheable-AMO-plus-PBMT=NC mixed fence window",
        excpt.triggered == false &&
        observed_cacheable_adjacent == cacheable_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_uncache_adjacent = ld(uncache_adjacent_vaddr);
    TEST_ASSERT("the adjacent PBMT=NC HS word remains unchanged across the cacheable-AMO-plus-PBMT=NC mixed fence window",
        excpt.triggered == false &&
        observed_uncache_adjacent == uncache_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t final_uncache = ld(uncache_vaddr);
    uint64_t final_uncache_adjacent = ld(uncache_adjacent_vaddr);
    TEST_ASSERT("after the repeated cacheable-AMO plus PBMT=NC-store fence window, the PBMT=NC alias still observes the latest payload and adjacent word",
        excpt.triggered == false &&
        final_uncache == uncache_values[(sizeof(uncache_values) / sizeof(uncache_values[0])) - 1U] &&
        final_uncache_adjacent == uncache_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated cacheable-AMO plus PBMT=NC-store fence window, the cacheable target contains its latest payload and adjacent word stays unchanged",
        read64(cacheable_paddr) == expected_cacheable &&
        read64(cacheable_adjacent_paddr) == cacheable_adjacent
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_fence_with_cacheable_amo_and_pbmt_io_store_same_window_corner()
{
    TEST_START();

    const uint64_t amo_masks[] = {
        0x000000000000f0f0ULL,
        0x00000000f0000000ULL,
        0x0f00000000000000ULL
    };
    const uint64_t uncache_values[] = {
        0x66554433221100ffULL,
        0x8877665544332211ULL,
        0xaabbccddeeff0011ULL
    };
    const uintptr_t cacheable_paddr = phys_page_base(SWITCH1) + 0x740U;
    const uintptr_t cacheable_vaddr = hs_page_base(SWITCH1) + 0x740U;
    const uintptr_t uncache_paddr = ai_micro_pbmt_uncache_io_backing_paddr() + 0x780U;
    const uintptr_t uncache_vaddr = ai_micro_prepare_pbmt_uncache_io_alias(
        VSRWXPbmt2_GURWXPbmt1) + 0x780U;
    const uintptr_t cacheable_adjacent_paddr = cacheable_paddr + sizeof(uint64_t);
    const uintptr_t cacheable_adjacent_vaddr = cacheable_vaddr + sizeof(uint64_t);
    const uintptr_t uncache_adjacent_paddr = uncache_paddr + sizeof(uint64_t);
    const uintptr_t uncache_adjacent_vaddr = uncache_vaddr + sizeof(uint64_t);
    const uint64_t initial_cacheable = 0x1032547698badc00ULL;
    const uint64_t cacheable_adjacent = 0x0123456789abcdefULL;
    const uint64_t uncache_adjacent = 0xfedcba9876543210ULL;
    uint64_t expected_cacheable = initial_cacheable;

    goto_priv(PRIV_M);
    write64(cacheable_paddr, initial_cacheable);
    write64(uncache_paddr, 0x0f1e2d3c4b5a6978ULL);
    write64(cacheable_adjacent_paddr, cacheable_adjacent);
    write64(uncache_adjacent_paddr, uncache_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm_cacheable = ld(cacheable_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable HS load can first observe the original payload before the cacheable-AMO-plus-PBMT=IO mixed fence window",
        warm_cacheable == initial_cacheable
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_uncache = ld(uncache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=IO HS load can first observe the original payload before the cacheable-AMO-plus-PBMT=IO mixed fence window",
        warm_uncache == 0x0f1e2d3c4b5a6978ULL
    );

    for (unsigned i = 0; i < sizeof(amo_masks) / sizeof(amo_masks[0]); ++i) {
        TEST_SETUP_EXCEPT();
        uint64_t amo_old = amoor_d(cacheable_vaddr, amo_masks[i]);
        TEST_ASSERT("the cacheable HS AMO retires before the mixed fence window that also contains a PBMT=IO store",
            excpt.triggered == false &&
            amo_old == expected_cacheable
        );
        expected_cacheable |= amo_masks[i];

        TEST_SETUP_EXCEPT();
        sd(uncache_vaddr, uncache_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the PBMT=IO HS store also retires before the cacheable-AMO-plus-PBMT=IO mixed fence window",
            true
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the cacheable AMO plus PBMT=IO store completes without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_cacheable = ld(cacheable_vaddr);
        TEST_ASSERT("after the cacheable-AMO plus PBMT=IO-store fence window, the cacheable HS load observes the latest AMO-updated payload immediately",
            excpt.triggered == false &&
            observed_cacheable == expected_cacheable
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_uncache = ld(uncache_vaddr);
        TEST_ASSERT("after the cacheable-AMO plus PBMT=IO-store fence window, the PBMT=IO HS load observes the latest IO-style payload immediately",
            excpt.triggered == false &&
            observed_uncache == uncache_values[i]
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed_cacheable_adjacent = ld(cacheable_adjacent_vaddr);
    TEST_ASSERT("the adjacent cacheable HS word remains unchanged across the cacheable-AMO-plus-PBMT=IO mixed fence window",
        excpt.triggered == false &&
        observed_cacheable_adjacent == cacheable_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_uncache_adjacent = ld(uncache_adjacent_vaddr);
    TEST_ASSERT("the adjacent PBMT=IO HS word remains unchanged across the cacheable-AMO-plus-PBMT=IO mixed fence window",
        excpt.triggered == false &&
        observed_uncache_adjacent == uncache_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated cacheable-AMO plus PBMT=IO-store fence window, both target locations contain their latest payloads and both adjacent words stay unchanged",
        read64(cacheable_paddr) == expected_cacheable &&
        read64(uncache_paddr) == uncache_values[(sizeof(uncache_values) / sizeof(uncache_values[0])) - 1U] &&
        read64(cacheable_adjacent_paddr) == cacheable_adjacent &&
        read64(uncache_adjacent_paddr) == uncache_adjacent
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_storequeue_pbmt_nc_req_not_rebound_when_next_cacheable_entry_advances_corner()
{
    TEST_START();

    const uint64_t nc_values[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0xa1b2c3d4e5f60718ULL
    };
    const uint64_t cacheable0_values[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL
    };
    const uint64_t cacheable1_values[] = {
        0x2233445566778899ULL,
        0xaabbccddeeff0011ULL,
        0x13579bdf2468ace0ULL
    };
    static uint64_t nc_window[] __attribute__((aligned(64))) = {
        0x1021324354657687ULL,
        0x0f1e2d3c4b5a6978ULL
    };
    const uintptr_t nc_paddr = (uintptr_t)&nc_window[0];
    const uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_alias_to_paddr(
        VSRWXPbmt1_GURWXPbmt1, nc_paddr);
    const uintptr_t cacheable0_paddr = phys_page_base(SWITCH1) + 0x7c0U;
    const uintptr_t cacheable0_vaddr = hs_page_base(SWITCH1) + 0x7c0U;
    const uintptr_t cacheable1_paddr = phys_page_base(SWITCH2) + 0x7c0U;
    const uintptr_t cacheable1_vaddr = hs_page_base(SWITCH2) + 0x7c0U;
    const uintptr_t nc_adjacent_paddr = (uintptr_t)&nc_window[1];
    const uintptr_t nc_adjacent_vaddr = ai_micro_pbmt_uncache_alias_vaddr(
        VSRWXPbmt1_GURWXPbmt1, nc_adjacent_paddr);
    const uintptr_t cacheable0_adjacent_paddr = cacheable0_paddr + sizeof(uint64_t);
    const uintptr_t cacheable0_adjacent_vaddr = cacheable0_vaddr + sizeof(uint64_t);
    const uintptr_t cacheable1_adjacent_paddr = cacheable1_paddr + sizeof(uint64_t);
    const uintptr_t cacheable1_adjacent_vaddr = cacheable1_vaddr + sizeof(uint64_t);
    const uint64_t nc_adjacent = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t cacheable0_adjacent = 0x8877665544332211ULL;
    const uint64_t cacheable1_adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(cacheable0_paddr, 0x0102030405060708ULL);
    write64(cacheable1_paddr, 0xf0e1d2c3b4a59687ULL);
    write64(cacheable0_adjacent_paddr, cacheable0_adjacent);
    write64(cacheable1_adjacent_paddr, cacheable1_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm_nc = ld(nc_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC HS load first observes the original payload before the next-cacheable-entry pointer-advance window",
        warm_nc == 0x1021324354657687ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_cacheable0 = ld(cacheable0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first cacheable HS follower also observes its original payload before the next-cacheable-entry pointer-advance window",
        warm_cacheable0 == 0x0102030405060708ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_cacheable1 = ld(cacheable1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second cacheable HS follower also observes its original payload before the next-cacheable-entry pointer-advance window",
        warm_cacheable1 == 0xf0e1d2c3b4a59687ULL
    );

    for (unsigned i = 0; i < sizeof(nc_values) / sizeof(nc_values[0]); ++i) {
        TEST_SETUP_EXCEPT();
        sd(nc_vaddr, nc_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the older PBMT=NC store retires before the next cacheable entries advance behind it",
            true
        );

        TEST_SETUP_EXCEPT();
        sd(cacheable0_vaddr, cacheable0_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the first younger cacheable store retires behind the older PBMT=NC store in the pointer-advance window",
            true
        );

        TEST_SETUP_EXCEPT();
        sd(cacheable1_vaddr, cacheable1_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the second younger cacheable store also retires behind the older PBMT=NC store in the pointer-advance window",
            true
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the PBMT=NC store plus two younger cacheable stores retires without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_nc = ld(nc_vaddr);
        TEST_ASSERT("after the next-cacheable-entry pointer-advance window, the older PBMT=NC store still returns only its own latest payload",
            excpt.triggered == false &&
            observed_nc == nc_values[i]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_cacheable0 = ld(cacheable0_vaddr);
        TEST_ASSERT("after the same window, the first younger cacheable follower still returns only its own latest payload",
            excpt.triggered == false &&
            observed_cacheable0 == cacheable0_values[i]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_cacheable1 = ld(cacheable1_vaddr);
        TEST_ASSERT("after the same window, the second younger cacheable follower still returns only its own latest payload",
            excpt.triggered == false &&
            observed_cacheable1 == cacheable1_values[i]
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed_nc_adjacent = ld(nc_adjacent_vaddr);
    TEST_ASSERT("the PBMT=NC adjacent word remains unchanged across the next-cacheable-entry pointer-advance window",
        excpt.triggered == false &&
        observed_nc_adjacent == nc_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_cacheable0_adjacent = ld(cacheable0_adjacent_vaddr);
    TEST_ASSERT("the first cacheable adjacent word remains unchanged across the next-cacheable-entry pointer-advance window",
        excpt.triggered == false &&
        observed_cacheable0_adjacent == cacheable0_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_cacheable1_adjacent = ld(cacheable1_adjacent_vaddr);
    TEST_ASSERT("the second cacheable adjacent word remains unchanged across the next-cacheable-entry pointer-advance window",
        excpt.triggered == false &&
        observed_cacheable1_adjacent == cacheable1_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated PBMT=NC-store plus next-cacheable-entry pointer-advance windows, all three backing locations contain only their own latest payloads and all adjacent words stay unchanged",
        read64(nc_paddr) == nc_values[(sizeof(nc_values) / sizeof(nc_values[0])) - 1U] &&
        read64(cacheable0_paddr) == cacheable0_values[(sizeof(cacheable0_values) / sizeof(cacheable0_values[0])) - 1U] &&
        read64(cacheable1_paddr) == cacheable1_values[(sizeof(cacheable1_values) / sizeof(cacheable1_values[0])) - 1U] &&
        read64(nc_adjacent_paddr) == nc_adjacent &&
        read64(cacheable0_adjacent_paddr) == cacheable0_adjacent &&
        read64(cacheable1_adjacent_paddr) == cacheable1_adjacent
    );
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_storequeue_pbmt_nc_req_not_rebound_when_next_cacheable_cmo_advances_corner()
{
    TEST_START();

    const uint64_t nc_values[] = {
        0x7766554433221100ULL,
        0x99aabbccddeeff00ULL,
        0x02468ace13579bdfULL
    };
    const uint64_t cacheable_store_values[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0xa1b2c3d4e5f60718ULL
    };
    const uint64_t cmo_seeds[] = {
        0x0102030405060708ULL,
        0x1021324354657687ULL,
        0x3141592653589793ULL
    };
    const unsigned cmo_offsets[] = {
        0U,
        24U,
        AI_MICRO_PBMT_CACHE_BLOCK_BYTES - 1U
    };
    static uint64_t nc_window[] __attribute__((aligned(64))) = {
        0x0f1e2d3c4b5a6978ULL,
        0xf0e1d2c3b4a59687ULL
    };
    const uintptr_t nc_paddr = (uintptr_t)&nc_window[0];
    const uintptr_t nc_vaddr = ai_micro_prepare_pbmt_uncache_alias_to_paddr(
        VSRWXPbmt1_GURWXPbmt1, nc_paddr);
    const uintptr_t cacheable_store_paddr = phys_page_base(SWITCH1) + 0x880U;
    const uintptr_t cacheable_store_vaddr = hs_page_base(SWITCH1) + 0x880U;
    const uintptr_t cmo_block_paddr = phys_page_base(SWITCH1) + 0x8c0U;
    const uintptr_t cmo_block_vaddr = hs_page_base(SWITCH1) + 0x8c0U;
    const uintptr_t nc_adjacent_paddr = (uintptr_t)&nc_window[1];
    const uintptr_t nc_adjacent_vaddr = ai_micro_pbmt_uncache_alias_vaddr(
        VSRWXPbmt1_GURWXPbmt1, nc_adjacent_paddr);
    const uintptr_t cacheable_adjacent_paddr = cacheable_store_paddr + sizeof(uint64_t);
    const uintptr_t cacheable_adjacent_vaddr = cacheable_store_vaddr + sizeof(uint64_t);
    const uint64_t nc_adjacent = 0xf0e1d2c3b4a59687ULL;
    const uint64_t cacheable_adjacent = 0x89abcdef76543210ULL;

    goto_priv(PRIV_M);
    write64(cacheable_store_paddr, 0xfedcba9876543210ULL);
    write64(cacheable_adjacent_paddr, cacheable_adjacent);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm_nc = ld(nc_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC HS load first observes the original payload before the next-cacheable-CMO pointer-advance window",
        warm_nc == 0x0f1e2d3c4b5a6978ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_cacheable = ld(cacheable_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable HS follower also observes its original payload before the next-cacheable-CMO pointer-advance window",
        warm_cacheable == 0xfedcba9876543210ULL
    );

    for (unsigned i = 0; i < sizeof(nc_values) / sizeof(nc_values[0]); ++i) {
        goto_priv(PRIV_M);
        ai_micro_pbmt_uncache_seed_block(cmo_block_paddr, cmo_seeds[i]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(nc_vaddr, nc_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the older PBMT=NC store retires before the younger cacheable store plus CBOM activity advances behind it",
            true
        );

        TEST_SETUP_EXCEPT();
        sd(cacheable_store_vaddr, cacheable_store_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("the younger cacheable store retires behind the older PBMT=NC store in the cacheable-CMO pointer-advance window",
            true
        );

        TEST_SETUP_EXCEPT();
        cbo_flush(cmo_block_vaddr + cmo_offsets[i]);
        AI_ASSERT_NO_EXCEPT_AND("the younger cacheable cbo.flush retires behind the older PBMT=NC store in the cacheable-CMO pointer-advance window",
            true
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the PBMT=NC store plus younger cacheable store plus cacheable cbo.flush retires without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_nc = ld(nc_vaddr);
        TEST_ASSERT("after the next-cacheable-CMO pointer-advance window, the older PBMT=NC store still returns only its own latest payload",
            excpt.triggered == false &&
            observed_nc == nc_values[i]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_cacheable = ld(cacheable_store_vaddr);
        TEST_ASSERT("after the same window, the younger cacheable store still returns only its own latest payload",
            excpt.triggered == false &&
            observed_cacheable == cacheable_store_values[i]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_cmo_head = ld(cmo_block_vaddr);
        TEST_ASSERT("after the same window, the cacheable cbo.flush target still returns its seeded head word without cross-binding",
            excpt.triggered == false &&
            observed_cmo_head == cmo_seeds[i]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_cmo_tail = ld(cmo_block_vaddr +
            AI_MICRO_PBMT_CACHE_BLOCK_BYTES - sizeof(uint64_t));
        TEST_ASSERT("after the same window, the cacheable cbo.flush target still returns its seeded tail word without cross-binding",
            excpt.triggered == false &&
            observed_cmo_tail == cmo_seeds[i]
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed_nc_adjacent = ld(nc_adjacent_vaddr);
    TEST_ASSERT("the PBMT=NC adjacent word remains unchanged across the next-cacheable-CMO pointer-advance window",
        excpt.triggered == false &&
        observed_nc_adjacent == nc_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_cacheable_adjacent = ld(cacheable_adjacent_vaddr);
    TEST_ASSERT("the cacheable adjacent word remains unchanged across the next-cacheable-CMO pointer-advance window",
        excpt.triggered == false &&
        observed_cacheable_adjacent == cacheable_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated PBMT=NC-store plus younger cacheable-store plus cacheable-CBOM pointer-advance windows, the PBMT=NC target, cacheable follower, and cacheable CMO target all keep only their own architectural state",
        read64(nc_paddr) == nc_values[(sizeof(nc_values) / sizeof(nc_values[0])) - 1U] &&
        read64(cacheable_store_paddr) == cacheable_store_values[(sizeof(cacheable_store_values) / sizeof(cacheable_store_values[0])) - 1U] &&
        read64(nc_adjacent_paddr) == nc_adjacent &&
        read64(cacheable_adjacent_paddr) == cacheable_adjacent &&
        read64(cmo_block_paddr) == cmo_seeds[(sizeof(cmo_seeds) / sizeof(cmo_seeds[0])) - 1U] &&
        read64(cmo_block_paddr + AI_MICRO_PBMT_CACHE_BLOCK_BYTES - sizeof(uint64_t)) ==
            cmo_seeds[(sizeof(cmo_seeds) / sizeof(cmo_seeds[0])) - 1U]
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE);
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}

bool ai_micro_hs_pbmt_nc_burst_load_store_resource_pressure_recovers_corner()
{
    TEST_START();

    static uint64_t nc_window[] __attribute__((aligned(64))) = {
        0x0011223344556677ULL,
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
        0x435465768798a9baULL,
        0x5465768798a9bacbULL,
        0x65768798a9bacbdcULL,
        0x768798a9bacbdcedULL,
        0x8798a9bacbdcedfeULL,
        0x98a9bacbdcedfe0fULL
    };
    const uint64_t expected_loads[] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
        0x435465768798a9baULL,
        0x5465768798a9bacbULL,
        0x65768798a9bacbdcULL,
        0x768798a9bacbdcedULL,
        0x8798a9bacbdcedfeULL
    };
    const uint64_t store_values[] = {
        0xa001a002a003a004ULL,
        0xb101b102b103b104ULL,
        0xc201c202c203c204ULL,
        0xd301d302d303d304ULL,
        0xe401e402e403e404ULL,
        0xf501f502f503f504ULL,
        0x0601060206030604ULL,
        0x1701170217031704ULL
    };
    const uintptr_t base_paddr = (uintptr_t)&nc_window[0];
    const uintptr_t first_target_paddr = (uintptr_t)&nc_window[1];
    const uintptr_t first_cache_block_vaddr = base_paddr &
        ~(uintptr_t)(AI_MICRO_PBMT_CACHE_BLOCK_BYTES - 1U);
    const uintptr_t second_cache_block_vaddr =
        first_cache_block_vaddr + AI_MICRO_PBMT_CACHE_BLOCK_BYTES;
    const uintptr_t first_target_vaddr = ai_micro_prepare_pbmt_uncache_alias_to_paddr(
        VSRWXPbmt1_GURWXPbmt1, first_target_paddr);
    const uint64_t lower_sentinel = nc_window[0];
    const uint64_t upper_sentinel = nc_window[9];
    uint64_t observed_loads[sizeof(expected_loads) / sizeof(expected_loads[0])] = {0};
    uint64_t observed_after_store[sizeof(store_values) / sizeof(store_values[0])] = {0};

    goto_priv(PRIV_HS);
    for (unsigned i = 0; i < sizeof(expected_loads) / sizeof(expected_loads[0]); ++i) {
        TEST_SETUP_EXCEPT();
        observed_loads[i] = ld(first_target_vaddr + i * sizeof(uint64_t));
        AI_ASSERT_NO_EXCEPT_AND("each PBMT=NC load in the burst pressure window retires without a trap",
            true
        );
    }

    for (unsigned i = 0; i < sizeof(expected_loads) / sizeof(expected_loads[0]); ++i) {
        TEST_ASSERT("the PBMT=NC burst load window returns each word from its own NC backing slot",
            observed_loads[i] == expected_loads[i]
        );
    }

    for (unsigned i = 0; i < sizeof(store_values) / sizeof(store_values[0]); ++i) {
        TEST_SETUP_EXCEPT();
        sd(first_target_vaddr + i * sizeof(uint64_t), store_values[i]);
        AI_ASSERT_NO_EXCEPT_AND("each PBMT=NC store in the burst pressure window retires without a trap",
            true
        );
    }

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence after the PBMT=NC burst store window retires after all visible stores complete",
        true
    );

    for (unsigned i = 0; i < sizeof(store_values) / sizeof(store_values[0]); ++i) {
        TEST_SETUP_EXCEPT();
        observed_after_store[i] = ld(first_target_vaddr + i * sizeof(uint64_t));
        AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC post-burst readback retires without a stale exception",
            true
        );
    }

    for (unsigned i = 0; i < sizeof(store_values) / sizeof(store_values[0]); ++i) {
        TEST_ASSERT("the PBMT=NC post-burst readback returns the latest store payload for each independent slot",
            observed_after_store[i] == store_values[i]
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed_lower_sentinel = ld(first_target_vaddr - sizeof(uint64_t));
    TEST_ASSERT("the PBMT=NC burst pressure window does not underflow into the lower adjacent word",
        excpt.triggered == false &&
        observed_lower_sentinel == lower_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_upper_sentinel = ld(first_target_vaddr +
        sizeof(store_values) / sizeof(store_values[0]) * sizeof(uint64_t));
    TEST_ASSERT("the PBMT=NC burst pressure window does not overflow into the upper adjacent word",
        excpt.triggered == false &&
        observed_upper_sentinel == upper_sentinel
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    TEST_SETUP_EXCEPT();
    cbo_inval(first_cache_block_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("invalidating the first cacheable backing block before the M-mode direct backing check completes",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(second_cache_block_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("invalidating the second cacheable backing block before the M-mode direct backing check completes",
        true
    );
    fence_iorw_iorw();

    bool backing_ok = read64(base_paddr) == lower_sentinel &&
        read64(base_paddr + 9U * sizeof(uint64_t)) == upper_sentinel;
    for (unsigned i = 0; i < sizeof(store_values) / sizeof(store_values[0]); ++i) {
        backing_ok = backing_ok &&
            read64(base_paddr + (i + 1U) * sizeof(uint64_t)) == store_values[i];
    }
    TEST_ASSERT("after the PBMT=NC burst pressure and recovery window, backing memory contains every latest store and both sentinels remain intact",
        backing_ok
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);
    ai_micro_reset_pbmt_uncache_env();

    TEST_END();
}
