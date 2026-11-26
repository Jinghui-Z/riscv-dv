#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MEMBLOCK_TLBCSR_ASSERT_PAGE_FAULT(message, expected_cause, vaddr) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == (expected_cause) && \
        excpt.tval == (vaddr))

#define AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline pte_t *ai_memblock_tlbcsr_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static pte_t ai_memblock_tlbcsr_high_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_tlbcsr_high_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_tlbcsr_high_l3[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

#ifdef sv48
#define AI_MEMBLOCK_TLBCSR_HIGH_BASE 0xffff800000000000ULL
#define AI_MEMBLOCK_TLBCSR_EXPECTED_MODE 9ULL
#else
#define AI_MEMBLOCK_TLBCSR_HIGH_BASE 0xffffffc000000000ULL
#define AI_MEMBLOCK_TLBCSR_EXPECTED_MODE 8ULL
#endif

static inline void ai_memblock_tlbcsr_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0;
    }
}

static inline void ai_memblock_tlbcsr_install_high_aliases(
    enum test_page load_page, enum test_page store_page, enum test_page guard_page)
{
    ai_memblock_tlbcsr_zero_pt(ai_memblock_tlbcsr_high_l1);
    ai_memblock_tlbcsr_zero_pt(ai_memblock_tlbcsr_high_l2);
    ai_memblock_tlbcsr_zero_pt(ai_memblock_tlbcsr_high_l3);

#ifdef sv48
    hspt[0][256] = PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l1) >> 2);
    ai_memblock_tlbcsr_high_l1[0] =
        PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l2) >> 2);
    ai_memblock_tlbcsr_high_l2[0] =
        PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l3) >> 2);
#else
    hspt[0][256] = PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l2) >> 2);
    ai_memblock_tlbcsr_high_l2[0] =
        PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l3) >> 2);
#endif

    ai_memblock_tlbcsr_high_l3[0] =
        (phys_page_base(load_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    ai_memblock_tlbcsr_high_l3[1] =
        (phys_page_base(store_page) >> 2) | PTE_V | PTE_AD | PTE_R;
    ai_memblock_tlbcsr_high_l3[2] =
        (phys_page_base(guard_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline void ai_memblock_tlbcsr_install_high_cross_page_aliases(
    enum test_page load_low_page, enum test_page load_high_page,
    enum test_page store_low_page, enum test_page store_high_page,
    enum test_page guard_page)
{
    ai_memblock_tlbcsr_zero_pt(ai_memblock_tlbcsr_high_l1);
    ai_memblock_tlbcsr_zero_pt(ai_memblock_tlbcsr_high_l2);
    ai_memblock_tlbcsr_zero_pt(ai_memblock_tlbcsr_high_l3);

#ifdef sv48
    hspt[0][256] = PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l1) >> 2);
    ai_memblock_tlbcsr_high_l1[0] =
        PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l2) >> 2);
    ai_memblock_tlbcsr_high_l2[0] =
        PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l3) >> 2);
#else
    hspt[0][256] = PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l2) >> 2);
    ai_memblock_tlbcsr_high_l2[0] =
        PTE_V | (((uintptr_t)ai_memblock_tlbcsr_high_l3) >> 2);
#endif

    ai_memblock_tlbcsr_high_l3[0] =
        (phys_page_base(load_low_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    ai_memblock_tlbcsr_high_l3[1] =
        (phys_page_base(load_high_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    ai_memblock_tlbcsr_high_l3[2] =
        (phys_page_base(store_low_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    ai_memblock_tlbcsr_high_l3[3] =
        (phys_page_base(store_high_page) >> 2) | PTE_V | PTE_AD | PTE_R;
    ai_memblock_tlbcsr_high_l3[4] =
        (phys_page_base(guard_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline uint64_t ai_memblock_tlbcsr_cross_page_ld_expected(
    uint64_t low_word, uint64_t high_word)
{
    return ((high_word & 0xffffffffULL) << 32) |
        ((low_word >> 32) & 0xffffffffULL);
}

static inline uint64_t ai_memblock_tlbcsr_cross_page_store_low_expected(
    uint64_t low_word, uint64_t store_value)
{
    return ((store_value & 0xffffffffULL) << 32) |
        (low_word & 0xffffffffULL);
}

static inline uint64_t ai_memblock_tlbcsr_cross_page_store_high_expected(
    uint64_t high_word, uint64_t store_value)
{
    return (high_word & 0xffffffff00000000ULL) |
        ((store_value >> 32) & 0xffffffffULL);
}

static inline uint64_t ai_memblock_tlbcsr_ld_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    register uint64_t value asm("a4") = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value) : "r"(addr) : "memory");
    return value;
}

bool ai_micro_hs_memblock_bare_to_sv48_highhalf_perm_fault_tval_current_context_corner()
{
    TEST_START();

    const enum test_page load_page = VSRWX_GRWX;
    const enum test_page store_page = VSRW_GRWX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x1b8U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x1b8U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x2c0U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x2c0U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x3d0U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x3d0U;
    const uintptr_t high_load_vaddr =
        AI_MEMBLOCK_TLBCSR_HIGH_BASE + 0x1b8U;
    const uintptr_t high_store_vaddr =
        AI_MEMBLOCK_TLBCSR_HIGH_BASE + PAGE_SIZE + 0x2c0U;
    const uintptr_t high_guard_vaddr =
        AI_MEMBLOCK_TLBCSR_HIGH_BASE + 2U * PAGE_SIZE + 0x3d0U;
    const uint64_t load_seed = 0x1021324354657687ULL;
    const uint64_t store_seed = 0x8877665544332211ULL;
    const uint64_t fault_store_attempt = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t store_update = 0xf0e1d2c3b4a59687ULL;
    const uint64_t guard_seed = 0x55aa66bb77cc88ddULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(load_paddr, load_seed);
    write64(store_paddr, store_seed);
    write64(guard_paddr, guard_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    ai_memblock_tlbcsr_install_high_aliases(load_page, store_page,
        guard_page);
    const uint64_t sv48_satp = CSRR(CSR_SATP);
    const uint64_t load_leaf_before =
        *ai_memblock_tlbcsr_hs_leaf_entry(load_page);
    const uint64_t store_leaf_before =
        *ai_memblock_tlbcsr_hs_leaf_entry(store_page);
    const uint64_t guard_leaf_before =
        *ai_memblock_tlbcsr_hs_leaf_entry(guard_page);
    const uint64_t high_load_leaf_before = ai_memblock_tlbcsr_high_l3[0];
    const uint64_t high_store_leaf_before = ai_memblock_tlbcsr_high_l3[1];
    const uint64_t high_guard_leaf_before = ai_memblock_tlbcsr_high_l3[2];
    sfence_vma();

    TEST_ASSERT("the tlbcsr context probe starts in a translated HS context with legal low aliases and high-half permission-fault aliases",
        (sv48_satp >> 60) == AI_MEMBLOCK_TLBCSR_EXPECTED_MODE &&
        (load_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (store_leaf_before & (PTE_V | PTE_AD | PTE_RW)) ==
            (PTE_V | PTE_AD | PTE_RW) &&
        (guard_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (high_load_leaf_before & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (high_load_leaf_before & PTE_R) == 0 &&
        (high_store_leaf_before & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R) &&
        (high_store_leaf_before & PTE_W) == 0 &&
        (high_guard_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_load = ld(load_vaddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("the canonical low-half HS load warms a legal translation before switching satp.MODE",
        warm_load == load_seed);

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t bare_load = ld(load_paddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("after switching to Bare, an HS load through the physical DRAM address retires normally",
        bare_load == load_seed && CSRR(CSR_SATP) == 0ULL);

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, sv48_satp);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ld(high_load_vaddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_PAGE_FAULT("immediately after Bare->translated restore, a high-half HS load permission fault reports stval with the current sign-extended VA",
        CAUSE_LPF, high_load_vaddr);

    TEST_SETUP_EXCEPT();
    sd(high_store_vaddr, fault_store_attempt);
    AI_MEMBLOCK_TLBCSR_ASSERT_PAGE_FAULT("the following high-half HS store permission fault refreshes stval using the current translated context instead of stale Bare zero-extension",
        CAUSE_SPF, high_store_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-half store permission fault does not update the backing store page before the later canonical store",
        read64(store_paddr) == store_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t high_guard_load = ld(high_guard_vaddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("a legal high-half guard alias remains usable after the load/store tval reconstruction faults",
        high_guard_load == guard_seed);

    TEST_SETUP_EXCEPT();
    uint64_t final_load = ld(load_vaddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("canonical low-half HS load still retires after high-half tval reconstruction probes",
        final_load == load_seed);

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_update);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("canonical low-half HS store still retires after high-half tval reconstruction probes",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t final_store = ld(store_vaddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("the canonical store payload is readable through the restored Sv48 mapping",
        final_store == store_update);

    TEST_SETUP_EXCEPT();
    uint64_t guard_load = ld(guard_vaddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("the unrelated guard mapping remains readable after Bare/Sv48 exception-context switching",
        guard_load == guard_seed);

    goto_priv(PRIV_M);
    TEST_ASSERT("Bare/translated tval-context switching leaves PTEs stable and only the intended canonical store updates backing memory",
        CSRR(CSR_SATP) == sv48_satp &&
        *ai_memblock_tlbcsr_hs_leaf_entry(load_page) == load_leaf_before &&
        *ai_memblock_tlbcsr_hs_leaf_entry(store_page) == store_leaf_before &&
        *ai_memblock_tlbcsr_hs_leaf_entry(guard_page) == guard_leaf_before &&
        ai_memblock_tlbcsr_high_l3[0] == high_load_leaf_before &&
        ai_memblock_tlbcsr_high_l3[1] == high_store_leaf_before &&
        ai_memblock_tlbcsr_high_l3[2] == high_guard_leaf_before &&
        read64(load_paddr) == load_seed &&
        read64(store_paddr) == store_update &&
        read64(guard_paddr) == guard_seed
    );

    TEST_END();
}

bool ai_micro_hs_memblock_bare_to_sv48_highhalf_cross_page_fault_tval_current_context_corner()
{
    TEST_START();

    const enum test_page load_low_page = VSRWX_GRWX;
    const enum test_page load_high_page = VSRW_GRWX;
    const enum test_page store_low_page = SWITCH1;
    const enum test_page store_high_page = SWITCH2;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t load_low_paddr =
        phys_page_base(load_low_page) + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t load_high_paddr = phys_page_base(load_high_page);
    const uintptr_t store_low_paddr =
        phys_page_base(store_low_page) + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t store_high_paddr = phys_page_base(store_high_page);
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x160U;
    const uintptr_t canonical_warm_vaddr =
        hs_page_base(load_low_page) + 0x180U;
    const uintptr_t canonical_warm_paddr =
        phys_page_base(load_low_page) + 0x180U;
    const uintptr_t high_load_vaddr =
        AI_MEMBLOCK_TLBCSR_HIGH_BASE + PAGE_SIZE - sizeof(uint32_t);
    const uintptr_t high_load_fault_vaddr =
        AI_MEMBLOCK_TLBCSR_HIGH_BASE + PAGE_SIZE;
    const uintptr_t high_store_vaddr =
        AI_MEMBLOCK_TLBCSR_HIGH_BASE + 3U * PAGE_SIZE - sizeof(uint32_t);
    const uintptr_t high_store_fault_vaddr =
        AI_MEMBLOCK_TLBCSR_HIGH_BASE + 3U * PAGE_SIZE;
    const uintptr_t high_guard_vaddr =
        AI_MEMBLOCK_TLBCSR_HIGH_BASE + 4U * PAGE_SIZE + 0x160U;
    const uint64_t warm_seed = 0x445566778899aabbULL;
    const uint64_t load_low_seed = 0x1021324354657687ULL;
    const uint64_t load_high_seed = 0x8877665544332211ULL;
    const uint64_t store_low_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t store_high_seed = 0x99aabbccddeeff00ULL;
    const uint64_t guard_seed = 0x55aa66bb77cc88ddULL;
    const uint64_t fault_store_attempt = 0x0123456789abcdefULL;
    const uint64_t repaired_store = 0xfedcba9876543210ULL;
    const uint64_t rd_sentinel = 0xa55a123456789abcULL;
    uint64_t rd_after_fault = 0;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(canonical_warm_paddr, warm_seed);
    write64(load_low_paddr, load_low_seed);
    write64(load_high_paddr, load_high_seed);
    write64(store_low_paddr, store_low_seed);
    write64(store_high_paddr, store_high_seed);
    write64(guard_paddr, guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    ai_memblock_tlbcsr_install_high_cross_page_aliases(load_low_page,
        load_high_page, store_low_page, store_high_page, guard_page);
    const uint64_t sv48_satp = CSRR(CSR_SATP);
    const uint64_t high_load_low_leaf_before = ai_memblock_tlbcsr_high_l3[0];
    const uint64_t high_load_fault_leaf_before = ai_memblock_tlbcsr_high_l3[1];
    const uint64_t high_store_low_leaf_before = ai_memblock_tlbcsr_high_l3[2];
    const uint64_t high_store_fault_leaf_before = ai_memblock_tlbcsr_high_l3[3];
    const uint64_t high_guard_leaf_before = ai_memblock_tlbcsr_high_l3[4];
    sfence_vma();

    TEST_ASSERT("the high-half cross-page context probe starts with legal first halves and permission-denied second halves",
        (sv48_satp >> 60) == AI_MEMBLOCK_TLBCSR_EXPECTED_MODE &&
        (high_load_low_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (high_load_fault_leaf_before & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (high_load_fault_leaf_before & PTE_R) == 0 &&
        (high_store_low_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (high_store_fault_leaf_before & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R) &&
        (high_store_fault_leaf_before & PTE_W) == 0 &&
        (high_guard_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_load = ld(canonical_warm_vaddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("a canonical low-half load warms the translated context before the Bare perturbation",
        warm_load == warm_seed);

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t bare_load = ld(canonical_warm_paddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("after switching satp.MODE to Bare, an HS load through the physical DRAM address retires normally",
        bare_load == warm_seed && CSRR(CSR_SATP) == 0ULL);

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, sv48_satp);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    rd_after_fault = ai_memblock_tlbcsr_ld_rd_probe(high_load_vaddr,
        rd_sentinel);
    TEST_ASSERT("immediately after Bare->translated restore, a high-half cross-page load reports the second-half load page fault with current Sv48 sign-extension and does not write rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == high_load_fault_vaddr &&
        rd_after_fault == rd_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(high_store_vaddr, fault_store_attempt);
    AI_MEMBLOCK_TLBCSR_ASSERT_PAGE_FAULT("the following high-half cross-page store reports the second-half store page fault with current Sv48 sign-extension",
        CAUSE_SPF, high_store_fault_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-half cross-page store fault preserves both split fragments before repair",
        read64(load_low_paddr) == load_low_seed &&
        read64(load_high_paddr) == load_high_seed &&
        read64(store_low_paddr) == store_low_seed &&
        read64(store_high_paddr) == store_high_seed &&
        ai_memblock_tlbcsr_high_l3[1] == high_load_fault_leaf_before &&
        ai_memblock_tlbcsr_high_l3[3] == high_store_fault_leaf_before
    );

    ai_memblock_tlbcsr_high_l3[1] =
        (phys_page_base(load_high_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    ai_memblock_tlbcsr_high_l3[3] =
        (phys_page_base(store_high_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    const uint64_t high_load_repaired_leaf = ai_memblock_tlbcsr_high_l3[1];
    const uint64_t high_store_repaired_leaf = ai_memblock_tlbcsr_high_l3[3];
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t repaired_load = ld(high_load_vaddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("after repairing the second-half load leaf, the same high-half cross-page load merges the two translated halves",
        repaired_load == ai_memblock_tlbcsr_cross_page_ld_expected(
            load_low_seed, load_high_seed));

    TEST_SETUP_EXCEPT();
    sd(high_store_vaddr, repaired_store);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("after repairing the second-half store leaf, the same high-half cross-page store retires without stale MAB exception state",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t high_guard_load = ld(high_guard_vaddr);
    AI_MEMBLOCK_TLBCSR_ASSERT_NO_FAULT_AND("an unrelated high-half guard alias remains readable after cross-page MAB fault recovery",
        high_guard_load == guard_seed);

    goto_priv(PRIV_M);
    TEST_ASSERT("Bare/translated cross-page MAB context switching leaves PTE ownership stable and applies only the repaired store bytes",
        CSRR(CSR_SATP) == sv48_satp &&
        ai_memblock_tlbcsr_high_l3[0] == high_load_low_leaf_before &&
        ai_memblock_tlbcsr_high_l3[1] == high_load_repaired_leaf &&
        ai_memblock_tlbcsr_high_l3[2] == high_store_low_leaf_before &&
        ai_memblock_tlbcsr_high_l3[3] == high_store_repaired_leaf &&
        ai_memblock_tlbcsr_high_l3[4] == high_guard_leaf_before &&
        read64(load_low_paddr) == load_low_seed &&
        read64(load_high_paddr) == load_high_seed &&
        read64(store_low_paddr) ==
            ai_memblock_tlbcsr_cross_page_store_low_expected(
                store_low_seed, repaired_store) &&
        read64(store_high_paddr) ==
            ai_memblock_tlbcsr_cross_page_store_high_expected(
                store_high_seed, repaired_store) &&
        read64(guard_paddr) == guard_seed
    );

    TEST_END();
}
