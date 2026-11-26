#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_MMIO_ALIAS_PAGE   VSRWX_GRWX
#define AI_MICRO_MMIO_BACKING_PAGE SCRATCHPAD
#define AI_MICRO_MMIO_MEM_BASE     0x30000000000ULL

#define AI_MICRO_MMIO_PMP_TOR_RWX  0x0fULL
#define AI_MICRO_MMIO_PMP_TOR_NO_R 0x0eULL
#define AI_MICRO_MMIO_PMP_TOR_NO_W 0x0dULL

static inline pte_t *ai_micro_mmio_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uintptr_t ai_micro_mmio_alias_anchor(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_micro_mmio_backing_paddr(void)
{
    return phys_page_base_offset(AI_MICRO_MMIO_BACKING_PAGE,
        AI_MICRO_MMIO_MEM_BASE);
}

static inline void ai_micro_mmio_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0x0fULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_micro_reset_mmio_env(void)
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
    ai_micro_mmio_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_prepare_mmio_alias(enum test_page alias_page,
    enum test_page backing_page)
{
    uintptr_t backing_paddr = phys_page_base_offset(backing_page,
        AI_MICRO_MMIO_MEM_BASE);

    ai_micro_reset_mmio_env();

    CSRW(CSR_PMAADDR0, backing_paddr >> 2);
    CSRW(CSR_PMAADDR1, (backing_paddr + PAGE_SIZE) >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);

    hspt_init();
    hspt_leaf_change_base_paddr(alias_page,
        ai_micro_mmio_alias_anchor(alias_page, backing_paddr));
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline uintptr_t ai_micro_prepare_mmio_leaf_overflow(enum test_page alias_page,
    enum test_page backing_page)
{
    uintptr_t vaddr = ai_micro_prepare_mmio_alias(alias_page, backing_page);

    goto_priv(PRIV_M);
    *ai_micro_mmio_leaf_entry(alias_page) |= (1ULL << 46);
    sfence_vma();

    return vaddr;
}

static inline uintptr_t ai_micro_prepare_mmio_invalid_leaf(enum test_page alias_page,
    enum test_page backing_page)
{
    uintptr_t vaddr = ai_micro_prepare_mmio_alias(alias_page, backing_page);

    goto_priv(PRIV_M);
    *ai_micro_mmio_leaf_entry(alias_page) = 0ULL;
    sfence_vma();

    return vaddr;
}

static inline void ai_micro_configure_mmio_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MMIO_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_MMIO_PMP_TOR_RWX << 16));
}

bool ai_micro_hs_load_mmio_data_visible()
{
    TEST_START();

    uintptr_t paddr = ai_micro_mmio_backing_paddr();
    uintptr_t vaddr = ai_micro_prepare_mmio_alias(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE);
    const uint64_t expected = 0x1021324354657687ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("an aligned HS load on a PMA=Device alias completes and returns the backing payload",
        excpt.triggered == false &&
        observed == expected
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_load_mmio_misaligned_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prepare_mmio_alias(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE) + 7U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a misaligned HS load on a PMA=Device alias reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_load_mmio_tlb_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prepare_mmio_leaf_overflow(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an aligned HS load on a PMA=Device alias reports load access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_load_mmio_page_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_prepare_mmio_invalid_leaf(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an aligned HS load on a PMA=Device alias reports load page fault when the leaf PTE is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_load_mmio_pmp_read_access_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_mmio_backing_paddr();
    uintptr_t vaddr = ai_micro_prepare_mmio_alias(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE);

    goto_priv(PRIV_M);
    write64(paddr, 0xabcdef0123456789ULL);
    ai_micro_configure_mmio_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_MMIO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an aligned HS load on a PMA=Device alias reports load access fault when PMP denies reads",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_data_visible()
{
    TEST_START();

    uintptr_t paddr = ai_micro_mmio_backing_paddr();
    uintptr_t vaddr = ai_micro_prepare_mmio_alias(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE);
    const uint64_t expected = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, 0x0102030405060708ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, expected);
    TEST_ASSERT("an aligned HS store on a PMA=Device alias completes without exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("reading back from the PMA=Device alias observes the payload written by the store",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("an aligned HS store on a PMA=Device alias updates the shared backing memory",
        read64(paddr) == expected
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_misaligned_access_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_mmio_backing_paddr();
    uintptr_t base_vaddr = ai_micro_prepare_mmio_alias(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE);
    uintptr_t sd_vaddr = base_vaddr + 7U;
    uintptr_t sh_vaddr = base_vaddr + 1U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t adjacent_value = 0x99aabbccddeeff00ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(paddr + sizeof(uint64_t), adjacent_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(sd_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("a misaligned HS store on a PMA=Device alias reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == sd_vaddr
    );

    TEST_SETUP_EXCEPT();
    sh(sh_vaddr, 0xbeefU);
    TEST_ASSERT("a misaligned HS halfword store on the PMA=Device alias reports store access fault with the exact halfword VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == sh_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the misaligned PMA=Device sd and sh probes leave the target and adjacent backing doublewords unchanged",
        read64(paddr) == initial_value &&
        read64(paddr + sizeof(uint64_t)) == adjacent_value
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_tlb_access_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_mmio_backing_paddr();
    const uint64_t initial_value = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    uintptr_t vaddr = ai_micro_prepare_mmio_leaf_overflow(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("an aligned HS store on a PMA=Device alias reports store access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PMA=Device leaf-overflow store fault leaves the shared backing unchanged",
        read64(paddr) == initial_value
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_page_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_mmio_backing_paddr();
    const uint64_t initial_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    uintptr_t vaddr = ai_micro_prepare_mmio_invalid_leaf(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xf0e1d2c3b4a59687ULL);
    TEST_ASSERT("an aligned HS store on a PMA=Device alias reports store page fault when the leaf PTE is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PMA=Device invalid-leaf store page fault leaves the shared backing unchanged",
        read64(paddr) == initial_value
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_pmp_write_access_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_mmio_backing_paddr();
    uintptr_t vaddr = ai_micro_prepare_mmio_alias(
        AI_MICRO_MMIO_ALIAS_PAGE, AI_MICRO_MMIO_BACKING_PAGE);
    const uint64_t initial_value = 0x22446688aaccee00ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    ai_micro_configure_mmio_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_MMIO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x13579bdf2468ace0ULL);
    TEST_ASSERT("an aligned HS store on a PMA=Device alias reports store access fault when PMP denies writes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PMA=Device store blocked by PMP leaves the shared backing unchanged",
        read64(paddr) == initial_value
    );

    ai_micro_reset_mmio_env();

    TEST_END();
}
