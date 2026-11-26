#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_ARCH_PBMT_IO_MEM_BASE  0x30000000000ULL
#define AI_ARCH_PBMT_IO_MEM_LIMIT 0x40000000000ULL
#define AI_ARCH_PBMT_CBO_BLOCK_WORDS 8U
#define AI_ARCH_PBMT_PMP_TOR_RWX  0x0fULL
#define AI_ARCH_PBMT_PMP_TOR_NO_R 0x0eULL
#define AI_ARCH_PBMT_PMP_TOR_NO_W 0x0dULL

#ifndef HYPTEST_HAS_3T_MMIO_SCRATCH
#define HYPTEST_HAS_3T_MMIO_SCRATCH 0
#endif

#define AI_ARCH_PBMT_PMA_R      (1ULL << 0)
#define AI_ARCH_PBMT_PMA_W      (1ULL << 1)
#define AI_ARCH_PBMT_PMA_X      (1ULL << 2)
#define AI_ARCH_PBMT_PMA_A_TOR  (1ULL << 3)
#define AI_ARCH_PBMT_PMA_C      (1ULL << 6)

static inline uint64_t ai_arch_pbmt_pma_cfg_entry(unsigned entry, uint64_t bits)
{
    return bits << (entry * 8);
}

static inline void ai_arch_pbmt_pma_cfg0_tor_entry1_encoded(uint64_t lower,
    uint64_t upper, uint64_t entry1_bits)
{
    CSRW(CSR_PMACFG0, 0ULL);
    CSRW(CSR_PMAADDR0, lower);
    CSRW(CSR_PMAADDR1, upper);
    CSRW(CSR_PMACFG0, ai_arch_pbmt_pma_cfg_entry(1, entry1_bits));
}

static inline uintptr_t ai_arch_pbmt_backing_paddr(void)
{
    return phys_page_base(X);
}

static inline uintptr_t ai_arch_pbmt_io_backing_paddr(void)
{
    return phys_page_base_offset(X, AI_ARCH_PBMT_IO_MEM_BASE);
}

static inline uintptr_t ai_arch_prepare_pbmt_alias(enum test_page page)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline uintptr_t ai_arch_prepare_pbmt_alias_at(enum test_page page,
    uintptr_t base_paddr)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x_base_paddr(page, base_paddr);
    sfence_vma();
    return hs_page_base(page);
}

static inline pte_t *ai_arch_pbmt_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_pbmt_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (unsigned i = 0; i < AI_ARCH_PBMT_CBO_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}

static inline bool ai_arch_pbmt_block_matches_seed(uintptr_t paddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < AI_ARCH_PBMT_CBO_BLOCK_WORDS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) !=
            seed + i * 0x1111111111111111ULL) {
            return false;
        }
    }
    return true;
}

static inline bool ai_arch_pbmt_block_all_zero(uintptr_t paddr)
{
    for (unsigned i = 0; i < AI_ARCH_PBMT_CBO_BLOCK_WORDS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != 0ULL) {
            return false;
        }
    }
    return true;
}

static inline uint64_t ai_arch_pbmt_leaf_value(uintptr_t paddr,
    uint64_t flags)
{
    return ((paddr & PAGE_ADDR_MSK) >> 2) | flags;
}

static inline uintptr_t ai_arch_pbmt_cross_page_addr(enum test_page low_page,
    uintptr_t low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - low_bytes;
}

static inline uintptr_t ai_arch_pbmt_page_end_phys(enum test_page page)
{
    return phys_page_base(page) + PAGE_SIZE - sizeof(uint64_t);
}

static inline uint64_t ai_arch_pbmt_expected_cross_load_4b(uint64_t low_word,
    uint64_t high_word)
{
    return (low_word >> 32) | ((high_word & 0xffffffffULL) << 32);
}

static inline uint64_t ai_arch_pbmt_expected_cross_store_low_4b(
    uint64_t old_low, uint64_t payload)
{
    return (old_low & 0xffffffffULL) |
        ((payload & 0xffffffffULL) << 32);
}

static inline uint64_t ai_arch_pbmt_expected_cross_store_high_4b(
    uint64_t old_high, uint64_t payload)
{
    return (old_high & 0xffffffff00000000ULL) | (payload >> 32);
}

static inline void ai_arch_pbmt_set_hs_leaf(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_arch_pbmt_hs_leaf_entry(page) =
        ai_arch_pbmt_leaf_value(paddr, flags);
    sfence_vma();
}

static inline void ai_arch_pbmt_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_ARCH_PBMT_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_arch_pbmt_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_ARCH_PBMT_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_ARCH_PBMT_PMP_TOR_RWX << 16));
}

static inline void ai_arch_prepare_pbmt_mem_3t_4t_window(void)
{
    goto_priv(PRIV_M);
    /*
     * PBMT=IO is a device/MMIO memory type.  Keep the backing page in the
     * LinkNan 3T IO/uncache window and program that PMA entry as
     * non-cacheable.  The caller must only issue real loads/stores here when
     * the active testbench provides a memory-like 3T responder.
     */
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    ai_arch_pbmt_pma_cfg0_tor_entry1_encoded(AI_ARCH_PBMT_IO_MEM_BASE >> 2,
        AI_ARCH_PBMT_IO_MEM_LIMIT >> 2,
        AI_ARCH_PBMT_PMA_R | AI_ARCH_PBMT_PMA_W |
            AI_ARCH_PBMT_PMA_X | AI_ARCH_PBMT_PMA_A_TOR);
    fence_iorw_iorw();
}

static inline void ai_arch_reset_pbmt_io_env(void)
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
    hspt_init();
    sfence_vma();
}

bool ai_arch_pbmt_pma_cacheable_data_correct()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t original_menvcfg = CSRR(CSR_MENVCFG);
    uintptr_t paddr = ai_arch_pbmt_backing_paddr();
    uintptr_t vaddr = ai_arch_prepare_pbmt_alias(VSRWXPbmt0_GURWXPbmt0);
    const uintptr_t amo_paddr = paddr + 8U;
    const uintptr_t amo_vaddr = vaddr + 8U;
    const uintptr_t cbo_paddr = paddr + 0x80U;
    const uintptr_t cbo_vaddr = vaddr + 0x80U;
    const uint64_t amo_seed = 0x0102030405060708ULL;
    const uint64_t amo_addend = 0x0011002200330044ULL;
    const uint64_t cbo_seed = 0x617263682d63626fULL;

    write64(paddr, 0x1122334455667788ULL);
    write64(amo_paddr, amo_seed);
    ai_arch_pbmt_seed_block(cbo_paddr, cbo_seed);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE | MENVCFG_CBZFE | MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x8877665544332211ULL);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=PMA cacheable alias accepts a full-width store",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=PMA cacheable alias reads back the stored payload",
        value == 0x8877665544332211ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=PMA cacheable alias updates the backing memory with the same payload",
        read64(paddr) == 0x8877665544332211ULL
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoadd_d(amo_vaddr, amo_addend);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=PMA cacheable AMO returns the old doubleword and commits the RMW result",
        amo_old == amo_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_new = ld(amo_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=PMA cacheable load observes the AMO-updated doubleword",
        amo_new == amo_seed + amo_addend
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=PMA cacheable AMO updates backing memory with the same RMW result",
        read64(amo_paddr) == amo_seed + amo_addend
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(cbo_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=PMA cacheable CBO clean is legal on an ordinary-memory block",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=PMA cacheable CBO clean preserves the ordinary-memory block contents",
        ai_arch_pbmt_block_matches_seed(cbo_paddr, cbo_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(cbo_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=PMA cacheable CBO zero is legal on an ordinary-memory block",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=PMA cacheable CBO zero clears the whole ordinary-memory block",
        ai_arch_pbmt_block_all_zero(cbo_paddr)
    );

    CSRW(CSR_MENVCFG, original_menvcfg);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_pbmt_nc_data_correct()
{
    TEST_START();

    const uint64_t first_payload = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_payload = 0x1122aabb3344ccddULL;
    uintptr_t paddr = ai_arch_pbmt_backing_paddr();
    uintptr_t vaddr = ai_arch_prepare_pbmt_alias(VSRWXPbmt1_GURWXPbmt1);

    write64(paddr, 0x0102030405060708ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_payload);
    TEST_ASSERT("PBMT=NC alias accepts a full-width store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("PBMT=NC alias reads back the written payload correctly",
        excpt.triggered == false &&
        value == first_payload
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated_value = ld(vaddr);
    TEST_ASSERT("a repeated PBMT=NC alias load returns the same current payload without falling onto a stale Device responder result",
        excpt.triggered == false &&
        repeated_value == first_payload
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_payload);
    TEST_ASSERT("a later PBMT=NC alias store updates the current memory-like value",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_value = ld(vaddr);
    TEST_ASSERT("after the later PBMT=NC store, the next PBMT=NC load returns the refreshed payload",
        excpt.triggered == false &&
        refreshed_value == second_payload
    );

    /*
     * Do not use a later M-mode direct read as the oracle for PBMT=NC data.
     * That read is a normal cacheable view and can legally observe an older
     * cached copy on this profile; the PBMT=NC alias load above is the
     * intended architectural check.
     */

    TEST_END();
}

bool ai_arch_pbmt_nc_narrow_load_store_mask_extension_corner()
{
    TEST_START();

    const uintptr_t vaddr = ai_arch_prepare_pbmt_alias(VSRWXPbmt1_GURWXPbmt1);
    const uint64_t load_seed = 0x807f80017f00ff80ULL;
    const uint64_t store_seed = 0x1122334455667788ULL;
    const uint64_t after_sb = 0x112233445566aa88ULL;
    const uint64_t after_sh = 0x1122beef5566aa88ULL;
    const uint64_t after_sw = 0x1122beef12345678ULL;
    const uint64_t full_value = 0xdeadbeefcafebabeULL;
    const uint64_t guard_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, load_seed);
    TEST_ASSERT("PBMT=NC setup seeds the narrow-load source through the same NC alias",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr + 8U, store_seed);
    TEST_ASSERT("PBMT=NC setup seeds the narrow-store target through the same NC alias",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr + 16U, guard_value);
    TEST_ASSERT("PBMT=NC setup seeds the adjacent guard through the same NC alias",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t byte_signed = (uint64_t)(int64_t)(int8_t)lb(vaddr);
    TEST_ASSERT("PBMT=NC byte load sign-extends the selected byte lane",
        excpt.triggered == false &&
        byte_signed == (uint64_t)(int64_t)(int8_t)0x80
    );

    TEST_SETUP_EXCEPT();
    uint64_t byte_zero = lbu(vaddr);
    TEST_ASSERT("PBMT=NC byte load unsigned zero-extends the selected byte lane",
        excpt.triggered == false &&
        byte_zero == 0x80U
    );

    TEST_SETUP_EXCEPT();
    uint64_t half_signed = (uint64_t)(int64_t)(int16_t)lh(vaddr);
    TEST_ASSERT("PBMT=NC halfword load sign-extends the selected halfword lane",
        excpt.triggered == false &&
        half_signed == (uint64_t)(int64_t)(int16_t)0xff80
    );

    TEST_SETUP_EXCEPT();
    uint64_t half_zero = lhu(vaddr);
    TEST_ASSERT("PBMT=NC halfword load unsigned zero-extends the selected halfword lane",
        excpt.triggered == false &&
        half_zero == 0xff80U
    );

    TEST_SETUP_EXCEPT();
    uint64_t word_signed = (uint64_t)(int64_t)(int32_t)lw(vaddr + 4U);
    TEST_ASSERT("PBMT=NC word load sign-extends the selected word lane",
        excpt.triggered == false &&
        word_signed == (uint64_t)(int64_t)(int32_t)0x807f8001U
    );

    TEST_SETUP_EXCEPT();
    uint64_t word_zero = lwu(vaddr + 4U);
    TEST_ASSERT("PBMT=NC word load unsigned zero-extends the selected word lane",
        excpt.triggered == false &&
        word_zero == 0x807f8001U
    );

    TEST_SETUP_EXCEPT();
    uint64_t full_load = ld(vaddr);
    TEST_ASSERT("PBMT=NC doubleword load returns the complete uncache payload",
        excpt.triggered == false &&
        full_load == load_seed
    );

    TEST_SETUP_EXCEPT();
    sb(vaddr + 9U, 0xaaU);
    TEST_ASSERT("PBMT=NC byte store updates only the addressed byte lane",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr + 8U);
    TEST_ASSERT("PBMT=NC byte-store mask leaves the surrounding bytes unchanged",
        excpt.triggered == false &&
        observed == after_sb
    );

    TEST_SETUP_EXCEPT();
    sh(vaddr + 12U, 0xbeefU);
    TEST_ASSERT("PBMT=NC halfword store updates only the selected two byte lanes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed = ld(vaddr + 8U);
    TEST_ASSERT("PBMT=NC halfword-store mask preserves bytes outside the halfword",
        excpt.triggered == false &&
        observed == after_sh
    );

    TEST_SETUP_EXCEPT();
    sw(vaddr + 8U, 0x12345678U);
    TEST_ASSERT("PBMT=NC word store updates exactly the selected word lanes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed = ld(vaddr + 8U);
    TEST_ASSERT("PBMT=NC word-store mask preserves the upper word from earlier narrow stores",
        excpt.triggered == false &&
        observed == after_sw
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr + 8U, full_value);
    TEST_ASSERT("PBMT=NC doubleword store updates the full selected doubleword",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t full_observed = ld(vaddr + 8U);
    TEST_ASSERT("PBMT=NC doubleword load returns the latest full-width store payload",
        excpt.triggered == false &&
        full_observed == full_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t full_byte_signed = (uint64_t)(int64_t)(int8_t)lb(vaddr + 8U);
    TEST_ASSERT("after a PBMT=NC full-width store, a byte load sign-extends the selected lane from the current doubleword",
        excpt.triggered == false &&
        full_byte_signed == (uint64_t)(int64_t)(int8_t)0xbeU
    );

    TEST_SETUP_EXCEPT();
    uint64_t full_byte_zero = lbu(vaddr + 8U);
    TEST_ASSERT("after a PBMT=NC full-width store, an unsigned byte load zero-extends the selected lane from the current doubleword",
        excpt.triggered == false &&
        full_byte_zero == 0xbeU
    );

    TEST_SETUP_EXCEPT();
    uint64_t full_half_signed = (uint64_t)(int64_t)(int16_t)lh(vaddr + 10U);
    TEST_ASSERT("after a PBMT=NC full-width store, a halfword load sign-extends the selected lane from the current doubleword",
        excpt.triggered == false &&
        full_half_signed == (uint64_t)(int64_t)(int16_t)0xcafeU
    );

    TEST_SETUP_EXCEPT();
    uint64_t full_half_zero = lhu(vaddr + 10U);
    TEST_ASSERT("after a PBMT=NC full-width store, an unsigned halfword load zero-extends the selected lane from the current doubleword",
        excpt.triggered == false &&
        full_half_zero == 0xcafeU
    );

    TEST_SETUP_EXCEPT();
    uint64_t full_word_signed = (uint64_t)(int64_t)(int32_t)lw(vaddr + 12U);
    TEST_ASSERT("after a PBMT=NC full-width store, a word load sign-extends the selected lane from the current doubleword",
        excpt.triggered == false &&
        full_word_signed == (uint64_t)(int64_t)(int32_t)0xdeadbeefU
    );

    TEST_SETUP_EXCEPT();
    uint64_t full_word_zero = lwu(vaddr + 12U);
    TEST_ASSERT("after a PBMT=NC full-width store, an unsigned word load zero-extends the selected lane from the current doubleword",
        excpt.triggered == false &&
        full_word_zero == 0xdeadbeefU
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_observed = ld(vaddr + 16U);
    TEST_ASSERT("PBMT=NC narrow and full stores do not pollute the adjacent doubleword",
        excpt.triggered == false &&
        guard_observed == guard_value
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_pbmt_nc_fault_repair_load_store_current_value_corner()
{
    TEST_START();

    const enum test_page page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t paddr = ai_arch_pbmt_backing_paddr();
    const uintptr_t vaddr = hs_page_base(page);
    const uintptr_t guard_vaddr = vaddr + sizeof(uint64_t);
    const uint64_t nc_leaf = PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;
    const uint64_t nc_no_r_leaf = PTE_V | PTE_AD | PTE_W | PTE_Pbmt1;
    const uint64_t nc_no_w_leaf = PTE_V | PTE_AD | PTE_R | PTE_Pbmt1;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t after_page_fault_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t after_pmp_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t guard_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    ai_arch_pbmt_restore_pmp_full_access();
    hspt_init();
    ai_arch_pbmt_set_hs_leaf(page, paddr, nc_leaf);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, initial_value);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC repair probe seeds its target through the NC alias before injecting faults",
        true);

    TEST_SETUP_EXCEPT();
    sd(guard_vaddr, guard_value);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC repair probe seeds an adjacent guard through the NC alias",
        true);

    ai_arch_pbmt_set_hs_leaf(page, paddr, nc_no_r_leaf);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a PBMT=NC leaf with read permission removed reports load page fault at the target VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr);

    ai_arch_pbmt_set_hs_leaf(page, paddr, nc_leaf);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t after_load_pf = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after repairing the PBMT=NC read permission and executing SFENCE.VMA, an NC load sees the current value",
        after_load_pf == initial_value);

    ai_arch_pbmt_set_hs_leaf(page, paddr, nc_no_w_leaf);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, after_page_fault_value);
    TEST_ASSERT("a PBMT=NC leaf with write permission removed reports store page fault without accepting the new value",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr);

    ai_arch_pbmt_set_hs_leaf(page, paddr, nc_leaf);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t after_store_pf = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after repairing the PBMT=NC write permission, the failed store has not changed the current value",
        after_store_pf == initial_value);

    TEST_SETUP_EXCEPT();
    sd(vaddr, after_page_fault_value);
    AI_ASSERT_NO_EXCEPT_AND("after page-fault repair, a PBMT=NC store can publish a new current value",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t after_repaired_store = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a PBMT=NC load after the repaired store reads back the new current value",
        after_repaired_store == after_page_fault_value);

    ai_arch_pbmt_configure_pmp_window(paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_ARCH_PBMT_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a PBMT=NC load reports load access fault when PMP denies reads to the translated physical page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr);

    ai_arch_pbmt_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t after_pmp_read_repair = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after restoring PMP read permission, the PBMT=NC load sees the current pre-fault value",
        after_pmp_read_repair == after_page_fault_value);

    ai_arch_pbmt_configure_pmp_window(paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_ARCH_PBMT_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, after_pmp_value);
    TEST_ASSERT("a PBMT=NC store reports store access fault when PMP denies writes and does not publish the new value",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr);

    ai_arch_pbmt_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t after_pmp_write_fault = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after restoring PMP write permission, the failed PMP-denied store has not changed the PBMT=NC current value",
        after_pmp_write_fault == after_page_fault_value);

    TEST_SETUP_EXCEPT();
    sd(vaddr, after_pmp_value);
    AI_ASSERT_NO_EXCEPT_AND("after PMP write repair, a PBMT=NC store can publish the next current value",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t final_value = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the page-fault and PMP repair sequence, the PBMT=NC load returns the final current value",
        final_value == after_pmp_value);

    TEST_SETUP_EXCEPT();
    uint64_t guard_observed = ld(guard_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=NC page-fault and PMP repair probes leave the adjacent guard doubleword unchanged",
        guard_observed == guard_value);

    goto_priv(PRIV_M);
    ai_arch_pbmt_restore_pmp_full_access();
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cross_page_pbmt_half_attr_fault_repair_rejudge_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_arch_pbmt_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_paddr = ai_arch_pbmt_page_end_phys(low_page);
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t low_guard_paddr =
        phys_page_base(low_page) + PAGE_SIZE - (2U * sizeof(uint64_t));
    const uintptr_t high_guard_paddr = high_paddr + sizeof(uint64_t);
    const uintptr_t high_io_paddr =
        phys_page_base_offset(high_page, AI_ARCH_PBMT_IO_MEM_BASE);
    const uint64_t leaf_rwx = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t leaf_no_r = PTE_V | PTE_AD | PTE_W | PTE_X;
    const uint64_t leaf_no_w = PTE_V | PTE_AD | PTE_RX;
    const uint64_t low_guard = 0x1021324354657687ULL;
    const uint64_t high_guard = 0x89abcdef01234567ULL;
    const uint64_t low_seed = 0x1122334455667788ULL;
    const uint64_t high_seed = 0xaabbccdd00112233ULL;
    const uint64_t store_payload = 0x0123456789abcdefULL;
    const uint64_t second_store = 0xfedcba9876543210ULL;
    const uint64_t expected_load =
        ai_arch_pbmt_expected_cross_load_4b(low_seed, high_seed);
    const uint64_t expected_low_after =
        ai_arch_pbmt_expected_cross_store_low_4b(low_seed, store_payload);
    const uint64_t expected_high_after =
        ai_arch_pbmt_expected_cross_store_high_4b(high_seed, store_payload);
    const uint64_t expected_low_second =
        ai_arch_pbmt_expected_cross_store_low_4b(low_seed, second_store);
    const uint64_t expected_high_second =
        ai_arch_pbmt_expected_cross_store_high_4b(high_seed, second_store);
    const uint64_t original_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    ai_arch_pbmt_restore_pmp_full_access();
    hspt_init();
    ai_arch_pbmt_set_hs_leaf(low_page, phys_page_base(low_page), leaf_rwx);
    ai_arch_pbmt_set_hs_leaf(high_page, phys_page_base(high_page), leaf_rwx);
    write64(low_guard_paddr, low_guard);
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    write64(high_guard_paddr, high_guard);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t ordinary_load = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("ordinary+ordinary cross-page scalar load stitches the two page halves in little-endian order",
        ordinary_load == expected_load);

    TEST_SETUP_EXCEPT();
    sd(vaddr, store_payload);
    AI_ASSERT_NO_EXCEPT_AND("ordinary+ordinary cross-page scalar store retires through the split store path",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("ordinary+ordinary cross-page store updates exactly the two target fragments and preserves both page guards",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == expected_low_after &&
        read64(high_paddr) == expected_high_after &&
        read64(high_guard_paddr) == high_guard);

    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    ai_arch_pbmt_set_hs_leaf(low_page, phys_page_base(low_page), leaf_rwx);
    ai_arch_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        leaf_rwx | PTE_Pbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("ordinary+PBMT=NC cross-page scalar load is rejudged after SFENCE.VMA and reports the NC misaligned-load profile result instead of stitching a false ordinary value",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM);

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store);
    TEST_ASSERT("ordinary+PBMT=NC cross-page scalar store is rejudged after SFENCE.VMA and reports the NC misaligned-store profile result",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM);

    goto_priv(PRIV_M);
    TEST_ASSERT("ordinary+PBMT=NC faulting split accesses leave both halves and guards unchanged",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed &&
        read64(high_guard_paddr) == high_guard);

    ai_arch_pbmt_set_hs_leaf(low_page, phys_page_base(low_page),
        leaf_rwx | PTE_Pbmt1);
    ai_arch_pbmt_set_hs_leaf(high_page, phys_page_base(high_page), leaf_rwx);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("PBMT=NC+ordinary cross-page scalar load selects the low-half NC misaligned profile result after both PBMT leaves are reread",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr);

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store);
    TEST_ASSERT("PBMT=NC+ordinary cross-page scalar store selects the low-half NC misaligned profile result and keeps the original VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=NC low-half faults still do not dirty either side of the cross-page target window",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed &&
        read64(high_guard_paddr) == high_guard);

    ai_arch_pbmt_set_hs_leaf(low_page, phys_page_base(low_page), leaf_rwx);
    ai_arch_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        leaf_no_r | PTE_Pbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("ordinary+PBMT=NC no-read high half reports load page fault at the second-half VA before final NC routing",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("high-half PBMT=NC load page fault preserves the non-faulting low half, the faulting high half, and both guards",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed &&
        read64(high_guard_paddr) == high_guard);

    ai_arch_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        leaf_rwx | PTE_Pbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after repairing only the high-half read permission while keeping PBMT=NC, the split load is rejudged as the NC misaligned-load profile result",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM);

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired-to-NC split-load rejudge still leaves both page fragments and guards unchanged",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed &&
        read64(high_guard_paddr) == high_guard);

    ai_arch_pbmt_set_hs_leaf(high_page, phys_page_base(high_page), leaf_rwx);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t repaired_load = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after repairing the high read permission and executing SFENCE.VMA, the same split load writes the full stitched rd value",
        repaired_load == expected_load);

    goto_priv(PRIV_M);
    ai_arch_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        leaf_no_w | PTE_Pbmt1);
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_payload);
    TEST_ASSERT("ordinary+PBMT=NC no-write high half reports store page fault at the second-half VA before final NC routing",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("high-half PBMT=NC store page fault has no partial side effect on either page half or guard",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed &&
        read64(high_guard_paddr) == high_guard);

    ai_arch_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        leaf_rwx | PTE_Pbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_payload);
    TEST_ASSERT("after repairing only the high-half write permission while keeping PBMT=NC, the split store is rejudged as the NC misaligned-store profile result",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM);

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired-to-NC split-store rejudge still suppresses partial writes on both halves",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed &&
        read64(high_guard_paddr) == high_guard);

    ai_arch_pbmt_set_hs_leaf(high_page, phys_page_base(high_page), leaf_rwx);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_payload);
    AI_ASSERT_NO_EXCEPT_AND("after repairing the high write permission and executing SFENCE.VMA, the same split store retires",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired split store writes the full target byte range and preserves both surrounding guards",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == expected_low_after &&
        read64(high_paddr) == expected_high_after &&
        read64(high_guard_paddr) == high_guard);

    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    ai_arch_pbmt_configure_pmp_window(high_paddr & PAGE_ADDR_MSK,
        (high_paddr & PAGE_ADDR_MSK) + PAGE_SIZE,
        AI_ARCH_PBMT_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("ordinary+PMP-denied high half reports load access fault at the second-half VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("high-half load access fault leaves the non-fault half and fault half data isolated",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed &&
        read64(high_guard_paddr) == high_guard);

    ai_arch_pbmt_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    repaired_load = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after PMP read repair, the same split load regenerates the stitched rd value without stale AF metadata",
        repaired_load == expected_load);

    goto_priv(PRIV_M);
    ai_arch_pbmt_configure_pmp_window(high_paddr & PAGE_ADDR_MSK,
        (high_paddr & PAGE_ADDR_MSK) + PAGE_SIZE,
        AI_ARCH_PBMT_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store);
    TEST_ASSERT("ordinary+PMP-denied high half reports store access fault at the second-half VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("high-half store access fault suppresses partial writes on both halves before repair",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed &&
        read64(high_guard_paddr) == high_guard);

    ai_arch_pbmt_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store);
    AI_ASSERT_NO_EXCEPT_AND("after PMP write repair, the same split store regenerates the full target write",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMP-repaired split store updates both page fragments and still preserves both guards",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == expected_low_second &&
        read64(high_paddr) == expected_high_second &&
        read64(high_guard_paddr) == high_guard);

    ai_arch_prepare_pbmt_mem_3t_4t_window();
    write64(low_paddr, low_seed);
    write64(high_io_paddr, high_seed);
    ai_arch_pbmt_set_hs_leaf(low_page, phys_page_base(low_page), leaf_rwx);
    ai_arch_pbmt_set_hs_leaf(high_page, high_io_paddr, leaf_rwx | PTE_Pbmt2);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("ordinary+PBMT=IO high half reports a Device/profile load access fault instead of stitching ordinary backing data",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF);

    TEST_SETUP_EXCEPT();
    sd(vaddr, store_payload);
    TEST_ASSERT("ordinary+PBMT=IO high half reports a Device/profile store access fault instead of writing either backing half",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF);

    goto_priv(PRIV_M);
    TEST_ASSERT("Device-half cross-page faults keep both the ordinary backing word and the IO responder word unchanged",
        read64(low_paddr) == low_seed &&
        read64(high_io_paddr) == high_seed);

    ai_arch_pbmt_restore_pmp_full_access();
    ai_arch_reset_pbmt_io_env();
    CSRW(CSR_MENVCFG, original_menvcfg);
    sfence_vma();

    TEST_END();
}

bool ai_arch_pbmt_io_data_correct()
{
    TEST_START();

    ai_arch_prepare_pbmt_mem_3t_4t_window();

    uintptr_t paddr = ai_arch_pbmt_io_backing_paddr();
    uintptr_t vaddr = ai_arch_prepare_pbmt_alias_at(
        VSRWXPbmt2_GURWXPbmt1, AI_ARCH_PBMT_IO_MEM_BASE);

    write64(paddr, 0x99aabbccddeeff00ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0fedcba987654321ULL);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=IO alias accepts a full-width store",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=IO alias reads back the written payload correctly",
        value == 0x0fedcba987654321ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=IO alias updates the backing memory with the expected payload",
        read64(paddr) == 0x0fedcba987654321ULL
    );
    ai_arch_reset_pbmt_io_env();

    TEST_END();
}

bool ai_arch_pbmt_nc_over_pma_io_aligned_load_store_profile_result_corner()
{
    TEST_START();

    ai_arch_prepare_pbmt_mem_3t_4t_window();

    const uintptr_t paddr = ai_arch_pbmt_io_backing_paddr() + 0x100U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t vaddr = ai_arch_prepare_pbmt_alias_at(
        VSRWXPbmt1_GURWXPbmt1, AI_ARCH_PBMT_IO_MEM_BASE) + 0x100U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x4e432d6f76657249ULL;
    const uint64_t updated_value = 0x504d412d494f2d4eULL;
    const uint64_t adjacent_value = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(vaddr);
    TEST_ASSERT("PBMT=NC over a PMA-IO physical window uses the profile-defined aligned load result instead of raising a stale Device access fault",
        excpt.triggered == false &&
        observed_before == initial_value);

    TEST_SETUP_EXCEPT();
    sd(vaddr, updated_value);
    TEST_ASSERT("PBMT=NC over a PMA-IO physical window accepts an aligned scalar store on the profile-defined memory-like path",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(vaddr);
    TEST_ASSERT("a later aligned load through the same PBMT=NC-over-PMA-IO alias observes the store-updated payload",
        excpt.triggered == false &&
        observed_after == updated_value);

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the PBMT=NC-over-PMA-IO aligned store leaves the adjacent doubleword unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("the PBMT=NC-over-PMA-IO profile-defined aligned path updates only the target backing word",
        read64(paddr) == updated_value &&
        read64(adjacent_paddr) == adjacent_value);
    ai_arch_reset_pbmt_io_env();

    TEST_END();
}

bool ai_arch_pbmt_pbmte_toggle_rejudges_nc_alias_corner()
{
    TEST_START();

    const uintptr_t paddr = ai_arch_pbmt_backing_paddr();
    const uintptr_t vaddr = hs_page_base(VSRWXPbmt1_GURWXPbmt1);
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t nc_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(VSRWXPbmt1_GURWXPbmt1);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("with MENVCFG.PBMTE clear, an HS load through a PBMT=NC leaf reports load page fault before the enable-side rejudge",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, nc_value);
    TEST_ASSERT("after setting MENVCFG.PBMTE and executing SFENCE.VMA, the same PBMT=NC HS store is rejudged as legal",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("the enabled PBMT=NC alias load reads back the payload written through the same alias",
        excpt.triggered == false &&
        observed == nc_value
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after clearing MENVCFG.PBMTE again and executing SFENCE.VMA, the same PBMT=NC leaf faults instead of reusing the enabled interpretation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_pbmt_reserved_leaf_faults_before_final_attr_route_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(VSRWXPbmt3_GURWXPbmt1);
    const uintptr_t paddr = phys_page_base(VSRWXPbmt3_GURWXPbmt1);
    const uint32_t ret_inst = 0x00008067U;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    write32(paddr, ret_inst);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("PBMT=3 reserved leaf blocks an HS load with load page fault before final memory-type routing",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x13579bdf2468ace0ULL);
    TEST_ASSERT("PBMT=3 reserved leaf blocks an HS store with store page fault before final memory-type routing",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 1);
    TEST_ASSERT("PBMT=3 reserved leaf blocks an HS AMO with store page fault before any cacheable, NC, or Device route is selected",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("PBMT=3 reserved leaf blocks an HS instruction fetch with instruction page fault before final memory-type routing",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    pte_t *reserved_leaf = ai_arch_pbmt_hs_leaf_entry(VSRWXPbmt3_GURWXPbmt1);
    *reserved_leaf &= ~PTE_Pbmt3;
    *reserved_leaf |= PTE_Pbmt1;
    sfence_vma();

    const uint64_t repaired_payload = 0x2468ace013579bdfULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, repaired_payload);
    TEST_ASSERT("after repairing the reserved PBMT leaf to PBMT=NC and executing SFENCE.VMA, a new HS store is rejudged as legal",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after reserved PBMT repair, a new HS load reads the repaired-path payload without reusing old fault metadata",
        excpt.triggered == false &&
        observed == repaired_payload
    );

    TEST_SETUP_EXCEPT();
    write32(vaddr, ret_inst);
    TEST_ASSERT("after reserved PBMT repair, the HS PBMT=NC alias can install a return instruction without reusing old store-fault metadata",
        excpt.triggered == false
    );

    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");

    TEST_SETUP_EXCEPT();
    ((void (*)(void))vaddr)();
    TEST_ASSERT("after repairing the reserved PBMT leaf to PBMT=NC, an HS instruction fetch through the same alias retires the installed return instruction",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint32_t installed_inst = read32(vaddr);
    TEST_ASSERT("the repaired PBMT=NC HS alias observes the installed instruction word without corrupting it",
        excpt.triggered == false &&
        installed_inst == ret_inst
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_pbmt_attr_switch_load_store_amo_cbo_rejudge_corner()
{
    TEST_START();

    const enum test_page page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t vaddr = hs_page_base(page) + 0x80U;
    const uintptr_t paddr = phys_page_base(X) + 0x80U;
    const uintptr_t zero_vaddr = hs_page_base(page) + 0x100U;
    const uintptr_t zero_paddr = phys_page_base(X) + 0x100U;
    const uintptr_t zero_guard_paddr = zero_paddr +
        AI_ARCH_PBMT_CBO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t io_paddr = phys_page_base_offset(X,
        AI_ARCH_PBMT_IO_MEM_BASE) + 0x80U;
    const uint64_t cacheable_seed = 0x63616368652d7030ULL;
    const uint64_t zero_seed = 0x7a65726f2d70626dULL;
    const uint64_t zero_guard = 0x67756172642d7a30ULL;
    const uint64_t io_seed = 0x696f2d6465767032ULL;
    const uint64_t nc_store = 0x6e632d73746f7265ULL;
    const uint64_t nc_amo_addend = 0x0001000100010001ULL;
    const uint64_t repaired_store = 0x70626d742d703030ULL;
    const uint64_t original_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t original_pmaaddr0 = CSRR(CSR_PMAADDR0);
    const uint64_t original_pmacfg0 = CSRR(CSR_PMACFG0);
    const uint64_t base_leaf_flags = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    write64(paddr, cacheable_seed);
    ai_arch_pbmt_seed_block(zero_paddr, zero_seed);
    write64(zero_guard_paddr, zero_guard);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBZE | MENVCFG_CBZFE |
        MENVCFG_CBIE01);
    hspt_init();
    pte_t *leaf = ai_arch_pbmt_hs_leaf_entry(page);
    *leaf = (phys_page_base(X) >> 2) | base_leaf_flags;
    const uint64_t cacheable_leaf = *leaf;
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the PBMT attribute-switch probe starts with a cacheable PBMT=0 leaf mapping the HS VA to the shared backing page",
        (cacheable_leaf & (PTE_Pbmt1 | PTE_Pbmt2)) == 0 &&
        (cacheable_leaf & base_leaf_flags) == base_leaf_flags &&
        ((cacheable_leaf << 2) & PAGE_ADDR_MSK) == (phys_page_base(X) & PAGE_ADDR_MSK));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t cacheable_load = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=0 cacheable load uses the initial ordinary-memory interpretation",
        cacheable_load == cacheable_seed);

    TEST_SETUP_EXCEPT();
    cbo_clean(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("PBMT=0 cacheable CBO clean is legal before the attribute switch",
        true);

    goto_priv(PRIV_M);
    *leaf = (phys_page_base(X) >> 2) | base_leaf_flags | PTE_Pbmt1;
    const uint64_t nc_leaf = *leaf;
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, nc_store);
    AI_ASSERT_NO_EXCEPT_AND("after switching the PTE to PBMT=NC and executing SFENCE.VMA, a store is rejudged on the NC path",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t nc_load = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC alias load reads back the NC store through the same translated alias",
        nc_load == nc_store);

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, nc_amo_addend);
    TEST_ASSERT("after switching the PTE to PBMT=NC and executing SFENCE.VMA, AMO is rejudged as an attribute fault in this profile",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t nc_after_amo_fault = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC AMO attribute fault leaves the scalar NC payload unchanged before CBO probes",
        nc_after_amo_fault == nc_store);

    TEST_SETUP_EXCEPT();
    cbo_flush(vaddr);
    TEST_ASSERT("PBMT=NC CBO flush is rejudged as an attribute fault once PBMTE and CBOM are enabled",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr);

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_vaddr);
    TEST_ASSERT("PBMT=NC CBO.ZERO is rejudged as an attribute fault once PBMTE and CBZE are enabled",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == zero_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t nc_after_cbo_fault = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=NC CBO fault probes leave the scalar NC payload unchanged",
        nc_after_cbo_fault == nc_store);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    TEST_ASSERT("PBMT=NC CBO fault probes leave the M-visible backing block and adjacent guard unchanged before switching to PBMT=IO",
        ai_arch_pbmt_block_matches_seed(zero_paddr, zero_seed) &&
        read64(zero_guard_paddr) == zero_guard);

#if !HYPTEST_HAS_3T_MMIO_SCRATCH
    TEST_ASSERT("PBMT=IO rejudge phase is blocked on the current LinkNan testbench because the 3T Device window has no memory-like scratch responder; the cacheable/NC rejudge prefix completed without using a fake DRAM substitute",
        true);
#else
    goto_priv(PRIV_M);
    ai_arch_prepare_pbmt_mem_3t_4t_window();
    write64(io_paddr, io_seed);
    *leaf = ((phys_page_base_offset(X, AI_ARCH_PBMT_IO_MEM_BASE) &
        PAGE_ADDR_MSK) >> 2) | base_leaf_flags | PTE_Pbmt2;
    const uint64_t io_leaf = *leaf;
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr);
    TEST_ASSERT("after switching the same PTE to PBMT=IO and fencing, CBO is rejudged against Device attributes and reports an access fault instead of using stale NC/cacheable state",
        excpt.triggered == true &&
        (excpt.cause == CAUSE_SAF || excpt.cause == CAUSE_LAF) &&
        excpt.tval == vaddr);

    goto_priv(PRIV_M);
    *leaf = (phys_page_base(X) >> 2) | base_leaf_flags;
    const uint64_t repaired_cacheable_leaf = *leaf;
    CSRW(CSR_PMAADDR0, original_pmaaddr0);
    CSRW(CSR_PMACFG0, original_pmacfg0);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, repaired_store);
    AI_ASSERT_NO_EXCEPT_AND("after switching PBMT back to cacheable and executing SFENCE.VMA, an ordinary store succeeds without stale Device/NC fault metadata",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t final_load = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the final cacheable load observes only the repaired ordinary-memory store",
        final_load == repaired_store);

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT cacheable/NC/IO/cacheable switching preserves the intended final leaf and exposes each final memory type only after SFENCE.VMA",
        (nc_leaf & PTE_Pbmt1) != 0 &&
        (io_leaf & PTE_Pbmt2) != 0 &&
        ((io_leaf << 2) & PAGE_ADDR_MSK) ==
            (phys_page_base_offset(X, AI_ARCH_PBMT_IO_MEM_BASE) &
                PAGE_ADDR_MSK) &&
        (repaired_cacheable_leaf & (PTE_Pbmt1 | PTE_Pbmt2)) == 0 &&
        *leaf == repaired_cacheable_leaf);
#endif

    CSRW(CSR_MENVCFG, original_menvcfg);
    CSRW(CSR_PMAADDR0, original_pmaaddr0);
    CSRW(CSR_PMACFG0, original_pmacfg0);
    hspt_init();
    sfence_vma();

    TEST_END();
}
