#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_TLB_HIGH_PPN_OVERFLOW_BIT (1ULL << 46)

static pte_t ai_micro_fullva_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

static inline pte_t *ai_micro_tlb_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline pte_t *ai_micro_tlb_gstage_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hpt[5][page];
#else
    return &hpt[2][page];
#endif
}

static inline void ai_micro_tlb_set_hs_leaf(enum test_page va_page,
    enum test_page phys_page, uint64_t flags)
{
    *ai_micro_tlb_hs_leaf_entry(va_page) =
        (phys_page_base(phys_page) >> 2) | flags;
}

static inline void ai_micro_tlb_set_gstage_leaf(enum test_page guest_page,
    enum test_page phys_page, uint64_t flags)
{
    *ai_micro_tlb_gstage_leaf_entry(guest_page) =
        (phys_page_base(phys_page) >> 2) | flags;
}

static inline void ai_micro_tlb_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}

static inline uint64_t ai_micro_tlb_ld_rd_probe(uintptr_t addr,
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

static inline uint64_t ai_micro_tlb_amoadd_d_rd_probe(uintptr_t addr,
    uint64_t sentinel, uint64_t addend)
{
    register uint64_t value asm("a4") = sentinel;
    register uint64_t operand asm("a5") = addend;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value) : "r"(addr), "r"(operand) : "memory");
    return value;
}

bool ai_micro_hs_load_pte_invalid_page_fault()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSI_GI);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    lb(vaddr);
    TEST_ASSERT("an HS load on a PTE.V=0 page reports load page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_store_pte_invalid_page_fault()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSI_GI);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("an HS store on a PTE.V=0 page reports store page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_store_noncanonical_page_fault_tval_raw()
{
    TEST_START();

    uintptr_t canonical_vaddr = 0xFFFF800000000000ULL;
    uintptr_t noncanonical_vaddr = 0x0000800000000000ULL;

    goto_priv(PRIV_HS);
    hspt_init();

    for (int i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        ai_micro_fullva_l2[i] = 0;
    }

    ai_micro_fullva_l2[0] = PTE_V | PTE_AD | PTE_RWX | (0x80000000ULL >> 2);
    ai_micro_fullva_l2[511] = PTE_V | PTE_AD | PTE_RWX | (0x80000000ULL >> 2);
    hspt[0][255] = PTE_V | (((uintptr_t)ai_micro_fullva_l2) >> 2);
    hspt[0][256] = PTE_V | (((uintptr_t)ai_micro_fullva_l2) >> 2);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sb(canonical_vaddr, 0x5aU);
    TEST_ASSERT("a canonical high-half Sv48 store address still translates successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sb(noncanonical_vaddr, 0xa5U);
    TEST_ASSERT("a non-canonical Sv48 store address reports store page fault and preserves the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == noncanonical_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_noncanonical_page_fault_tval_raw()
{
    TEST_START();

    uintptr_t canonical_vaddr = 0xFFFF800000000000ULL;
    uintptr_t noncanonical_vaddr = 0x0000800000000000ULL;

    goto_priv(PRIV_HS);
    hspt_init();

    for (int i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        ai_micro_fullva_l2[i] = 0;
    }

    ai_micro_fullva_l2[0] = PTE_V | PTE_AD | PTE_RWX | (0x80000000ULL >> 2);
    ai_micro_fullva_l2[511] = PTE_V | PTE_AD | PTE_RWX | (0x80000000ULL >> 2);
    hspt[0][255] = PTE_V | (((uintptr_t)ai_micro_fullva_l2) >> 2);
    hspt[0][256] = PTE_V | (((uintptr_t)ai_micro_fullva_l2) >> 2);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    lb(canonical_vaddr);
    TEST_ASSERT("a canonical high-half Sv48 load address still translates successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    lb(noncanonical_vaddr);
    TEST_ASSERT("a non-canonical Sv48 load address reports load page fault and preserves the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == noncanonical_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_tlb_access_fault()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS load reports load access fault when the upper-level PTE PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS load reports load access fault when the leaf-level PTE PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_store_tlb_access_fault()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("an HS store reports store access fault when the upper-level PTE PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS store reports store access fault when the leaf-level PTE PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_leaf_high_ppn_access_fault_ad_unchanged_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x128U;
    const uintptr_t paddr = phys_page_base(page) + 0x128U;
    const uint64_t initial = 0x61702d39312d6c64ULL;
    const uint64_t load_sentinel = 0xfeedface91a00001ULL;
    const uint64_t overflow_leaf =
        (phys_page_base(page) >> 2) |
        PTE_V | PTE_RWX | AI_MICRO_TLB_HIGH_PPN_OVERFLOW_BIT;

    goto_priv(PRIV_M);
    write64(paddr, initial);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_tlb_hs_leaf_entry(page) = overflow_leaf;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_tlb_ld_rd_probe(vaddr, load_sentinel);
    TEST_ASSERT("an HS load through a legal leaf except for unsupported high PPN bits reports load access fault, keeps the original VA in tval, and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr &&
        observed == load_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-PPN load access-fault branch runs before A/D update and leaves the leaf PTE and backing data unchanged",
        *ai_micro_tlb_hs_leaf_entry(page) == overflow_leaf &&
        (*ai_micro_tlb_hs_leaf_entry(page) & (PTE_A | PTE_D)) == 0 &&
        read64(paddr) == initial
    );

    ai_micro_tlb_set_hs_leaf(page, page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT("after repairing the leaf PPN into the supported physical range, the same HS load succeeds and returns the seeded value",
        excpt.triggered == false &&
        observed == initial
    );

    TEST_END();
}

bool ai_micro_hs_store_leaf_high_ppn_access_fault_no_side_effect_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRW;
    const uintptr_t vaddr = hs_page_base(page) + 0x1a0U;
    const uintptr_t paddr = phys_page_base(page) + 0x1a0U;
    const uintptr_t guard_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x61702d39312d7374ULL;
    const uint64_t guard = 0x61702d39312d6775ULL;
    const uint64_t denied_payload = 0xdeadbeefcafef00dULL;
    const uint64_t repaired_payload = 0x1021324354657687ULL;
    const uint64_t overflow_leaf =
        (phys_page_base(page) >> 2) |
        PTE_V | PTE_AD | PTE_RW | AI_MICRO_TLB_HIGH_PPN_OVERFLOW_BIT;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(guard_paddr, guard);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_tlb_hs_leaf_entry(page) = overflow_leaf;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied_payload);
    TEST_ASSERT("an HS store through a leaf with unsupported high PPN bits reports store access fault with the original VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-PPN store access fault leaves the target, guard, and leaf PTE unchanged",
        *ai_micro_tlb_hs_leaf_entry(page) == overflow_leaf &&
        read64(paddr) == initial &&
        read64(guard_paddr) == guard
    );

    ai_micro_tlb_set_hs_leaf(page, page, PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, repaired_payload);
    TEST_ASSERT("after repairing the high PPN bit, the same HS store retires normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t readback = ld(vaddr);
    TEST_ASSERT("the repaired store writes exactly the target doubleword and is readable through the same VA",
        excpt.triggered == false &&
        readback == repaired_payload
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired store updates only the target doubleword while the adjacent guard remains unchanged",
        read64(paddr) == repaired_payload &&
        read64(guard_paddr) == guard
    );

    TEST_END();
}

bool ai_micro_hs_amo_leaf_high_ppn_access_fault_no_side_effect_corner()
{
    TEST_START();

    const enum test_page page = VSRW_GRW;
    const uintptr_t vaddr = hs_page_base(page) + 0x220U;
    const uintptr_t paddr = phys_page_base(page) + 0x220U;
    const uintptr_t guard_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x61702d39312d616dULL;
    const uint64_t guard = 0x61702d39312d6167ULL;
    const uint64_t addend = 0x0102030405060708ULL;
    const uint64_t amo_sentinel = 0xfeedface91a00002ULL;
    const uint64_t overflow_leaf =
        (phys_page_base(page) >> 2) |
        PTE_V | PTE_AD | PTE_RW | AI_MICRO_TLB_HIGH_PPN_OVERFLOW_BIT;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(guard_paddr, guard);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_tlb_hs_leaf_entry(page) = overflow_leaf;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value =
        ai_micro_tlb_amoadd_d_rd_probe(vaddr, amo_sentinel, addend);
    TEST_ASSERT("an HS AMO through a leaf with unsupported high PPN bits reports store access fault and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        old_value == amo_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-PPN AMO access fault performs no read-modify-write and preserves the adjacent guard",
        *ai_micro_tlb_hs_leaf_entry(page) == overflow_leaf &&
        read64(paddr) == initial &&
        read64(guard_paddr) == guard
    );

    ai_micro_tlb_set_hs_leaf(page, page, PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    old_value = ai_micro_tlb_amoadd_d_rd_probe(vaddr, 0ULL, addend);
    TEST_ASSERT("after repairing the high PPN bit, the same HS AMO succeeds and returns the original value",
        excpt.triggered == false &&
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired AMO updates only the target doubleword and keeps the guard unchanged",
        read64(paddr) == (initial + addend) &&
        read64(guard_paddr) == guard
    );

    TEST_END();
}

bool ai_micro_hs_fetch_leaf_high_ppn_access_fault_then_repair_executes_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x300U;
    const uintptr_t paddr = phys_page_base(page) + 0x300U;
    const uintptr_t guard_paddr = paddr + sizeof(uint32_t);
    const uint32_t ret_inst = 0x00008067U;
    const uint32_t guard_inst = 0x00000013U;
    const uint64_t overflow_leaf =
        (phys_page_base(page) >> 2) |
        PTE_V | PTE_AD | PTE_RWX | AI_MICRO_TLB_HIGH_PPN_OVERFLOW_BIT;

    goto_priv(PRIV_M);
    write32(paddr, ret_inst);
    write32(guard_paddr, guard_inst);
    fence_iorw_iorw();
    ai_micro_tlb_fence_i();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_tlb_hs_leaf_entry(page) = overflow_leaf;
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_tlb_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("an HS instruction fetch through a leaf with unsupported high PPN bits reports instruction access fault at the fetch VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.epc == vaddr &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-PPN fetch access fault leaves the executable leaf and backing instruction words unchanged",
        *ai_micro_tlb_hs_leaf_entry(page) == overflow_leaf &&
        read32(paddr) == ret_inst &&
        read32(guard_paddr) == guard_inst
    );

    ai_micro_tlb_set_hs_leaf(page, page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_tlb_fence_i();
    TEST_SETUP_EXCEPT();
    ((void (*)(void))vaddr)();
    TEST_ASSERT("after repairing the high PPN bit, the same executable leaf fetches and returns normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired fetch path does not modify the target or guard instruction words",
        read32(paddr) == ret_inst &&
        read32(guard_paddr) == guard_inst
    );

    TEST_END();
}

bool ai_micro_vs_gstage_leaf_high_ppn_access_fault_not_guest_page_fault_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = vs_page_base(page) + 0x190U;
    const uintptr_t paddr = phys_page_base(page) + 0x190U;
    const uintptr_t guard_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x61702d39312d6773ULL;
    const uint64_t guard = 0x61702d39312d6867ULL;
    const uint64_t load_sentinel = 0xfeedface91a00003ULL;
    const uint64_t overflow_leaf =
        (phys_page_base(page) >> 2) |
        PTE_V | PTE_U | PTE_AD | PTE_RWX |
        AI_MICRO_TLB_HIGH_PPN_OVERFLOW_BIT;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(guard_paddr, guard);
    hspt_init();
    hpt_init();
    vspt_init();

    *ai_micro_tlb_gstage_leaf_entry(page) = overflow_leaf;
    sfence_vma();
    hfence();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_tlb_ld_rd_probe(vaddr, load_sentinel);
    TEST_ASSERT("a VS load whose G-stage leaf has unsupported high PPN bits reports load access fault rather than load guest-page fault",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_LAF &&
        excpt.cause != CAUSE_LGPF &&
        excpt.tval == vaddr &&
        observed == load_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the G-stage high-PPN load access fault leaves the G-stage leaf and backing data unchanged",
        *ai_micro_tlb_gstage_leaf_entry(page) == overflow_leaf &&
        read64(paddr) == initial &&
        read64(guard_paddr) == guard
    );

    ai_micro_tlb_set_gstage_leaf(page, page,
        PTE_V | PTE_U | PTE_AD | PTE_RWX);
    sfence_vma();
    hfence();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT("after repairing the G-stage high PPN bit, the same VS load succeeds through the two-stage translation",
        excpt.triggered == false &&
        observed == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired G-stage load path keeps the backing guard unchanged",
        read64(paddr) == initial &&
        read64(guard_paddr) == guard
    );
    CSRW(CSR_HGATP, 0ULL);
    CSRW(CSR_VSATP, 0ULL);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();
    hfence();

    TEST_END();
}
