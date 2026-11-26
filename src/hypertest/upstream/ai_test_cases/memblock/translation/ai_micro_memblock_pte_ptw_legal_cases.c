#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MEMBLOCK_PTE_PTW_ASID_A 0x0311ULL
#define AI_MEMBLOCK_PTE_PTW_ASID_B 0x0523ULL
#define AI_MEMBLOCK_PTE_PTW_SATP_ASID_SHIFT 44
#define AI_MEMBLOCK_PTE_PTW_SATP_ASID_MASK (0xffffULL << AI_MEMBLOCK_PTE_PTW_SATP_ASID_SHIFT)

#define AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MEMBLOCK_ASSERT_PAGE_FAULT(message, expected_cause, vaddr) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == (expected_cause) && \
        excpt.tval == (vaddr))

static inline pte_t *ai_memblock_pte_ptw_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_pte_ptw_set_hs_leaf_to_phys(
    enum test_page va_page, enum test_page phys_page, uint64_t flags)
{
    *ai_memblock_pte_ptw_hs_leaf_entry(va_page) =
        (phys_page_base(phys_page) >> 2) | flags;
}

static inline uint64_t ai_memblock_pte_ptw_set_asid(uint64_t asid)
{
    uint64_t satp = CSRR(CSR_SATP);
    satp &= ~AI_MEMBLOCK_PTE_PTW_SATP_ASID_MASK;
    satp |= (asid << AI_MEMBLOCK_PTE_PTW_SATP_ASID_SHIFT) &
        AI_MEMBLOCK_PTE_PTW_SATP_ASID_MASK;
    CSRW(CSR_SATP, satp);
    return satp;
}

static inline uint64_t ai_memblock_pte_ptw_get_asid(void)
{
    return (CSRR(CSR_SATP) & AI_MEMBLOCK_PTE_PTW_SATP_ASID_MASK) >>
        AI_MEMBLOCK_PTE_PTW_SATP_ASID_SHIFT;
}

static inline uint64_t ai_memblock_pte_ptw_leaf_flags(enum test_page page)
{
    return *ai_memblock_pte_ptw_hs_leaf_entry(page) & PTE_FLAGS_MSK;
}

static inline void ai_memblock_pte_ptw_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}

static inline uint64_t ai_memblock_pte_ptw_ld_rd_probe(uintptr_t addr,
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

static inline uint64_t ai_memblock_pte_ptw_lrsc_d_pair(uintptr_t addr,
    uint64_t new_value, uint64_t *old_value)
{
    uint64_t old_reg = 0ULL;
    uint64_t status = ~0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %0, 0(%2)\n\t"
        "sc.d %1, %3, 0(%2)\n\t"
        ".option pop\n\t"
        : "=&r"(old_reg), "=&r"(status)
        : "r"(addr), "r"(new_value)
        : "memory");
    *old_value = old_reg;
    return status;
}

static pte_t ai_memblock_pte_ptw_sv39_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_pte_ptw_sv39_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_pte_ptw_sv39_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_pte_ptw_root_a[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_pte_ptw_l1_a[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_pte_ptw_l0_a[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_pte_ptw_root_b[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_pte_ptw_l1_b[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_memblock_pte_ptw_l0_b[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

static inline void ai_memblock_pte_ptw_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0;
    }
}

#define AI_MEMBLOCK_PTE_PTW_SATP_MODE_SV39 8ULL
#define AI_MEMBLOCK_PTE_PTW_SATP_MODE_SV48 9ULL
#define AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(addr) (((addr) >> 30) & 0x1ffULL)
#define AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(addr) (((addr) >> 21) & 0x1ffULL)
#define AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(addr) (((addr) >> 12) & 0x1ffULL)

bool ai_micro_memblock_superpage_leaf_alignment_matrix_corner()
{
    TEST_START();

    const uintptr_t vaddr_4k_base = 0x00600000ULL;
    const uintptr_t vaddr_2m_base = 0x00400000ULL;
    const uintptr_t vaddr_1g_base = 0x40000000ULL;
    const uintptr_t vaddr_512g_base = 0x8000000000ULL;
    const uintptr_t paddr_4k = phys_page_base(SWITCH2) + 0x80ULL;
    const uintptr_t paddr_2m_base = TEST_PPAGE_BASE;
    const uintptr_t paddr_1g_base = MEM_BASE;
    const uintptr_t paddr_512g_base = 0ULL;
    const uintptr_t vaddr_4k = vaddr_4k_base + 0x80ULL;
    const uintptr_t offset_2m = 0x1000ULL;
    const uintptr_t offset_1g = (TEST_PPAGE_BASE - MEM_BASE) + 0x2000ULL;
    const uintptr_t offset_512g = TEST_PPAGE_BASE + 0x3000ULL;
    const uintptr_t vaddr_2m = vaddr_2m_base + offset_2m;
    const uintptr_t vaddr_1g = vaddr_1g_base + offset_1g;
    const uintptr_t vaddr_512g = vaddr_512g_base + offset_512g;
    const uintptr_t paddr_2m = paddr_2m_base + offset_2m;
    const uintptr_t paddr_1g = paddr_1g_base + offset_1g;
    const uintptr_t paddr_512g = paddr_512g_base + offset_512g;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t value_4k = 0x0404040404040404ULL;
    const uint64_t value_2m = 0x0202020202020202ULL;
    const uint64_t value_1g = 0x1010101010101010ULL;
    const uint64_t value_512g = 0x5151515151515151ULL;
    const uint64_t update_2m = 0x2222222222222222ULL;
    const uint64_t update_1g = 0x1111111111111111ULL;
    const uint64_t update_512g = 0x5555555555555555ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    ai_memblock_pte_ptw_zero_pt(ai_memblock_pte_ptw_sv39_root);
    ai_memblock_pte_ptw_zero_pt(ai_memblock_pte_ptw_sv39_l1);
    ai_memblock_pte_ptw_zero_pt(ai_memblock_pte_ptw_sv39_l0);
    write64(paddr_4k, value_4k);
    write64(paddr_2m, value_2m);
    write64(paddr_1g, value_1g);
    write64(paddr_512g, value_512g);
    fence_iorw_iorw();

    ai_memblock_pte_ptw_sv39_root[
        AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr_2m_base)] =
            PTE_V | (((uintptr_t)ai_memblock_pte_ptw_sv39_l1) >> 2);
    ai_memblock_pte_ptw_sv39_l1[
        AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr_2m_base)] =
            (paddr_2m_base >> 2) | leaf_flags;
    ai_memblock_pte_ptw_sv39_l1[
        AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr_4k_base)] =
            PTE_V | (((uintptr_t)ai_memblock_pte_ptw_sv39_l0) >> 2);
    ai_memblock_pte_ptw_sv39_l0[
        (vaddr_4k_base >> PAGE_SHIFT) & 0x1ffULL] =
            ((paddr_4k & PAGE_ADDR_MSK) >> 2) | leaf_flags;
    ai_memblock_pte_ptw_sv39_root[
        AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr_1g_base)] =
            (paddr_1g_base >> 2) | leaf_flags;
    ai_memblock_pte_ptw_sv39_root[
        AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(MEM_BASE)] =
            (MEM_BASE >> 2) | leaf_flags;
    const uint64_t sv39_satp =
        (AI_MEMBLOCK_PTE_PTW_SATP_MODE_SV39 << 60) |
        (((uintptr_t)ai_memblock_pte_ptw_sv39_root) >> 12);
    CSRW(CSR_SATP, sv39_satp);
    sfence_vma();

    const uint64_t leaf_2m = ai_memblock_pte_ptw_sv39_l1[
        AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr_2m_base)];
    const uint64_t leaf_4k = ai_memblock_pte_ptw_sv39_l0[
        (vaddr_4k_base >> PAGE_SHIFT) & 0x1ffULL];
    const uint64_t leaf_1g = ai_memblock_pte_ptw_sv39_root[
        AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr_1g_base)];
    TEST_ASSERT("Sv39 setup installs legal 4KB, 2MB, and 1GB leaves with aligned PPN fields",
        CSRR(CSR_SATP) == sv39_satp &&
        (leaf_4k & leaf_flags) == leaf_flags &&
        (((leaf_4k & ~PTE_FLAGS_MSK) << 2) & (PAGE_SIZE - 1ULL)) == 0 &&
        ((leaf_4k & ~PTE_FLAGS_MSK) << 2) ==
            (paddr_4k & PAGE_ADDR_MSK) &&
        (leaf_2m & leaf_flags) == leaf_flags &&
        (((leaf_2m & ~PTE_FLAGS_MSK) << 2) &
            (SUPERPAGE_SIZE(2) - 1ULL)) == 0 &&
        ((leaf_2m & ~PTE_FLAGS_MSK) << 2) == paddr_2m_base &&
        (leaf_1g & leaf_flags) == leaf_flags &&
        (((leaf_1g & ~PTE_FLAGS_MSK) << 2) &
            (SUPERPAGE_SIZE(1) - 1ULL)) == 0 &&
        ((leaf_1g & ~PTE_FLAGS_MSK) << 2) == paddr_1g_base
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_4k = ld(vaddr_4k);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS load through the Sv39 4KB leaf completes a three-level walk and returns the mapped payload",
        load_4k == value_4k
    );

    TEST_SETUP_EXCEPT();
    uint64_t load_2m = ld(vaddr_2m);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS load through the Sv39 2MB superpage leaf stops at L1 and returns the mapped payload",
        load_2m == value_2m
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr_2m, update_2m);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS store through the legal Sv39 2MB superpage leaf updates the aligned physical superpage",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t load_1g = ld(vaddr_1g);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS load through the Sv39 1GB superpage leaf stops at L2 and returns the mapped payload",
        load_1g == value_1g
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr_1g, update_1g);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS store through the legal Sv39 1GB superpage leaf updates the aligned physical superpage",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("legal Sv39 superpage accesses update only the expected aligned backing words and keep leaf encodings stable",
        read64(paddr_4k) == value_4k &&
        read64(paddr_2m) == update_2m &&
        read64(paddr_1g) == update_1g &&
        ai_memblock_pte_ptw_sv39_l0[
            (vaddr_4k_base >> PAGE_SHIFT) & 0x1ffULL] == leaf_4k &&
        ai_memblock_pte_ptw_sv39_l1[
            AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr_2m_base)] == leaf_2m &&
        ai_memblock_pte_ptw_sv39_root[
            AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr_1g_base)] == leaf_1g
    );

    ai_memblock_pte_ptw_sv39_l1[
        AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr_2m_base)] =
            ((paddr_2m_base + PAGE_SIZE) >> 2) | leaf_flags;
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t rd_2m_fault = ai_memblock_pte_ptw_ld_rd_probe(vaddr_2m,
        0xd00d000000002222ULL);
    TEST_ASSERT("a Sv39 2MB superpage leaf with a nonzero lower PPN field faults as LPF and preserves rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr_2m &&
        rd_2m_fault == 0xd00d000000002222ULL
    );

    goto_priv(PRIV_M);
    ai_memblock_pte_ptw_sv39_l1[
        AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr_2m_base)] =
            (paddr_2m_base >> 2) | leaf_flags;
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_2m_repaired = ld(vaddr_2m);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after repairing the Sv39 2MB superpage PPN alignment and SFENCE.VMA, the same VA loads successfully",
        load_2m_repaired == update_2m
    );

    goto_priv(PRIV_M);
    ai_memblock_pte_ptw_sv39_root[
        AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr_1g_base)] =
            ((paddr_1g_base + SUPERPAGE_SIZE(2)) >> 2) | leaf_flags;
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t rd_1g_fault = ai_memblock_pte_ptw_ld_rd_probe(vaddr_1g,
        0xd00d000000001111ULL);
    TEST_ASSERT("a Sv39 1GB superpage leaf with a nonzero lower PPN field faults as LPF and preserves rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr_1g &&
        rd_1g_fault == 0xd00d000000001111ULL
    );

    goto_priv(PRIV_M);
    ai_memblock_pte_ptw_sv39_root[
        AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr_1g_base)] =
            (paddr_1g_base >> 2) | leaf_flags;
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_1g_repaired = ld(vaddr_1g);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after repairing the Sv39 1GB superpage PPN alignment and SFENCE.VMA, the same VA loads successfully",
        load_1g_repaired == update_1g
    );

    goto_priv(PRIV_M);
    hspt_init();
    hspt[0][1] = (paddr_512g_base >> 2) | leaf_flags;
    sfence_vma();

    const uint64_t sv48_satp = CSRR(CSR_SATP);
    const uint64_t leaf_512g = hspt[0][1];
    TEST_ASSERT("Sv48 setup installs a legal 512GB top-level leaf with an aligned PPN field",
        (sv48_satp >> 60) == AI_MEMBLOCK_PTE_PTW_SATP_MODE_SV48 &&
        (leaf_512g & leaf_flags) == leaf_flags &&
        (((leaf_512g & ~PTE_FLAGS_MSK) << 2) &
            (SUPERPAGE_SIZE(0) - 1ULL)) == 0 &&
        ((leaf_512g & ~PTE_FLAGS_MSK) << 2) == paddr_512g_base
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_512g = ld(vaddr_512g);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS load through the Sv48 512GB superpage leaf follows the project-supported top-level leaf path",
        load_512g == value_512g
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr_512g, update_512g);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS store through the Sv48 512GB superpage leaf updates the aligned physical superpage",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("legal Sv48 512GB superpage access updates the expected backing word and keeps the top-level leaf stable",
        read64(paddr_512g) == update_512g &&
        hspt[0][1] == leaf_512g
    );

    hspt[0][1] = ((paddr_512g_base + MEM_BASE) >> 2) | leaf_flags;
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t rd_512g_fault = ai_memblock_pte_ptw_ld_rd_probe(vaddr_512g,
        0xd00d000000005555ULL);
    TEST_ASSERT("a Sv48 512GB superpage leaf with a nonzero lower PPN field faults as LPF and preserves rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr_512g &&
        rd_512g_fault == 0xd00d000000005555ULL
    );

    goto_priv(PRIV_M);
    hspt[0][1] = (paddr_512g_base >> 2) | leaf_flags;
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_512g_repaired = ld(vaddr_512g);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after repairing the Sv48 512GB superpage PPN alignment and SFENCE.VMA, the same VA loads successfully",
        load_512g_repaired == update_512g
    );

    TEST_END();
}

static inline void ai_memblock_pte_ptw_install_sv39_4k_root(
    pte_t *root, pte_t *l1, pte_t *l0, uintptr_t vaddr,
    uintptr_t paddr, uint64_t leaf_flags)
{
    ai_memblock_pte_ptw_zero_pt(root);
    ai_memblock_pte_ptw_zero_pt(l1);
    ai_memblock_pte_ptw_zero_pt(l0);

    root[AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr)] =
        PTE_V | (((uintptr_t)l1) >> 2);
    l1[AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr)] =
        PTE_V | (((uintptr_t)l0) >> 2);
    l0[AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(vaddr)] =
        ((paddr & PAGE_ADDR_MSK) >> 2) | leaf_flags;
}

bool ai_micro_memblock_satp_root_switch_refills_new_ptw_path_corner()
{
    TEST_START();

    const uintptr_t vaddr = 0x0067a240ULL;
    const uintptr_t paddr_a = phys_page_base(SWITCH1) + 0x240U;
    const uintptr_t paddr_b = phys_page_base(SWITCH2) + 0x240U;
    const uintptr_t guard_a_paddr = paddr_a + sizeof(uint64_t);
    const uintptr_t guard_b_paddr = paddr_b + sizeof(uint64_t);
    const uint64_t value_a = 0x53565054524f4f41ULL;
    const uint64_t value_b = 0x53565054524f4f42ULL;
    const uint64_t update_b = 0x53565054524f5732ULL;
    const uint64_t guard_a = 0x0badf00d11223344ULL;
    const uint64_t guard_b = 0x55667788cafebeefULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t satp_a =
        (AI_MEMBLOCK_PTE_PTW_SATP_MODE_SV39 << 60) |
        (((uintptr_t)ai_memblock_pte_ptw_root_a) >> 12);
    const uint64_t satp_b =
        (AI_MEMBLOCK_PTE_PTW_SATP_MODE_SV39 << 60) |
        (((uintptr_t)ai_memblock_pte_ptw_root_b) >> 12);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(paddr_a, value_a);
    write64(paddr_b, value_b);
    write64(guard_a_paddr, guard_a);
    write64(guard_b_paddr, guard_b);
    ai_memblock_pte_ptw_install_sv39_4k_root(
        ai_memblock_pte_ptw_root_a, ai_memblock_pte_ptw_l1_a,
        ai_memblock_pte_ptw_l0_a, vaddr, paddr_a, leaf_flags);
    ai_memblock_pte_ptw_install_sv39_4k_root(
        ai_memblock_pte_ptw_root_b, ai_memblock_pte_ptw_l1_b,
        ai_memblock_pte_ptw_l0_b, vaddr, paddr_b, leaf_flags);
    ai_memblock_pte_ptw_root_a[AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(MEM_BASE)] =
        (MEM_BASE >> 2) | leaf_flags;
    ai_memblock_pte_ptw_root_b[AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(MEM_BASE)] =
        (MEM_BASE >> 2) | leaf_flags;
    const uint64_t root_a_entry =
        ai_memblock_pte_ptw_root_a[AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr)];
    const uint64_t l1_a_entry =
        ai_memblock_pte_ptw_l1_a[AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr)];
    const uint64_t l0_a_leaf =
        ai_memblock_pte_ptw_l0_a[AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(vaddr)];
    const uint64_t root_b_entry =
        ai_memblock_pte_ptw_root_b[AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr)];
    const uint64_t l1_b_entry =
        ai_memblock_pte_ptw_l1_b[AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr)];
    const uint64_t l0_b_leaf =
        ai_memblock_pte_ptw_l0_b[AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(vaddr)];
    fence_iorw_iorw();

    TEST_ASSERT("the satp root-switch PTW probe installs two Sv39 roots mapping the same VA to different physical pages",
        satp_a != satp_b &&
        (root_a_entry & PTE_V) != 0 &&
        (root_b_entry & PTE_V) != 0 &&
        (l1_a_entry & PTE_V) != 0 &&
        (l1_b_entry & PTE_V) != 0 &&
        ((l0_a_leaf << 2) & PAGE_ADDR_MSK) == (paddr_a & PAGE_ADDR_MSK) &&
        ((l0_b_leaf << 2) & PAGE_ADDR_MSK) == (paddr_b & PAGE_ADDR_MSK)
    );

    CSRW(CSR_SATP, satp_a);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the first HS load walks root A and reads the root-A physical payload",
        first_load == value_a);

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, satp_b);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t second_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after writing satp to root B and executing sfence.vma, the same VA walks the new root and reads the root-B payload",
        second_load == value_b);

    TEST_SETUP_EXCEPT();
    sd(vaddr, update_b);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("a store after the root switch updates only the physical page selected by root B",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t after_store = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the post-switch load observes the update through root B without falling back to root A",
        after_store == update_b);

    goto_priv(PRIV_M);
    TEST_ASSERT("satp root switching preserves both page-table roots and updates only root B's backing page",
        CSRR(CSR_SATP) == satp_b &&
        ai_memblock_pte_ptw_root_a[AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr)] ==
            root_a_entry &&
        ai_memblock_pte_ptw_l1_a[AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr)] ==
            l1_a_entry &&
        ai_memblock_pte_ptw_l0_a[AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(vaddr)] ==
            l0_a_leaf &&
        ai_memblock_pte_ptw_root_b[AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(vaddr)] ==
            root_b_entry &&
        ai_memblock_pte_ptw_l1_b[AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(vaddr)] ==
            l1_b_entry &&
        ai_memblock_pte_ptw_l0_b[AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(vaddr)] ==
            l0_b_leaf &&
        read64(paddr_a) == value_a &&
        read64(paddr_b) == update_b &&
        read64(guard_a_paddr) == guard_a &&
        read64(guard_b_paddr) == guard_b
    );

    TEST_END();
}

bool ai_micro_memblock_s_mode_4k_hit_miss_pte_consistent_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x180U;
    const uintptr_t paddr = phys_page_base(page) + 0x180U;
    const uintptr_t amo_vaddr = hs_page_base(page) + 0x188U;
    const uintptr_t amo_paddr = phys_page_base(page) + 0x188U;
    const uintptr_t fetch_vaddr = hs_page_base(page) + 0x190U;
    const uintptr_t fetch_paddr = phys_page_base(page) + 0x190U;
    const uintptr_t lrsc_vaddr = hs_page_base(page) + 0x198U;
    const uintptr_t lrsc_paddr = phys_page_base(page) + 0x198U;
    const uint64_t initial = 0x1021324354657687ULL;
    const uint64_t updated = 0x8877665544332211ULL;
    const uint64_t amo_initial = 0x1122334455667788ULL;
    const uint64_t amo_addend = 0x0101010101010101ULL;
    const uint64_t lrsc_initial = 0x5a5a112233445566ULL;
    const uint64_t lrsc_updated = 0xa5a5665544332211ULL;
    const uint64_t expected_flags = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(paddr, initial);
    write64(amo_paddr, amo_initial);
    write64(lrsc_paddr, lrsc_initial);
    write32(fetch_paddr, 0x00008067U);
    fence_iorw_iorw();
    ai_memblock_pte_ptw_fence_i();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_pte_ptw_set_asid(AI_MEMBLOCK_PTE_PTW_ASID_A);
    sfence_vma();

    goto_priv(PRIV_M);
    const uint64_t leaf_before = *ai_memblock_pte_ptw_hs_leaf_entry(page);
    TEST_ASSERT("the S-mode legal 4K leaf carries V/R/W/X/A/D, clears U/G, and is tagged by the selected ASID context",
        ai_memblock_pte_ptw_get_asid() == AI_MEMBLOCK_PTE_PTW_ASID_A &&
        ai_memblock_pte_ptw_leaf_flags(page) == expected_flags &&
        (leaf_before & PTE_U) == 0 &&
        (leaf_before & PTE_G) == 0 &&
        ((leaf_before << 2) & PAGE_ADDR_MSK) == phys_page_base(page)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t miss_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the first S-mode 4K load walks a cold legal PTE and returns the backing payload",
        miss_load == initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t hit_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the repeated S-mode 4K load hits the established legal translation and returns the same payload",
        hit_load == initial
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, updated);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the S-mode 4K store through the legal translated hit path retires without a fault",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t after_store = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the S-mode 4K alias observes the value stored through the same legal translation",
        after_store == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoadd_d(amo_vaddr, amo_addend);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the S-mode 4K AMO uses the same legal translated permission path and returns the old payload",
        amo_old == amo_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t lrsc_old = 0ULL;
    uint64_t lrsc_status = ai_memblock_pte_ptw_lrsc_d_pair(lrsc_vaddr,
        lrsc_updated, &lrsc_old);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the S-mode 4K LR/SC pair uses the same legal translated permission path and commits the SC update",
        lrsc_old == lrsc_initial && lrsc_status == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ((void (*)(void))fetch_vaddr)();
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the S-mode 4K fetch through the same legal leaf retires the installed ret instruction",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("S-mode legal 4K translation keeps the PTE snapshot stable and updates the expected physical word",
        *ai_memblock_pte_ptw_hs_leaf_entry(page) == leaf_before &&
        read64(paddr) == updated &&
        read64(amo_paddr) == amo_initial + amo_addend &&
        read64(lrsc_paddr) == lrsc_updated &&
        read32(fetch_paddr) == 0x00008067U
    );

    TEST_END();
}

bool ai_micro_memblock_legal_pte_permission_hit_matrix_corner()
{
    TEST_START();

    const enum test_page read_page = VSR_GR;
    const enum test_page read_write_page = VSRW_GRW;
    const enum test_page execute_page = VSX_GX;
    const enum test_page read_execute_page = VSRX_GRX;
    const uintptr_t read_vaddr = hs_page_base(read_page) + 0x300U;
    const uintptr_t read_paddr = phys_page_base(read_page) + 0x300U;
    const uintptr_t read_write_vaddr = hs_page_base(read_write_page) + 0x308U;
    const uintptr_t read_write_paddr = phys_page_base(read_write_page) + 0x308U;
    const uintptr_t amo_vaddr = hs_page_base(read_write_page) + 0x330U;
    const uintptr_t amo_paddr = phys_page_base(read_write_page) + 0x330U;
    const uintptr_t execute_vaddr = hs_page_base(execute_page) + 0x310U;
    const uintptr_t execute_paddr = phys_page_base(execute_page) + 0x310U;
    const uintptr_t read_execute_vaddr = hs_page_base(read_execute_page) + 0x318U;
    const uintptr_t read_execute_paddr = phys_page_base(read_execute_page) + 0x318U;
    const uint64_t read_value = 0x726561642d6f6e6cULL;
    const uint64_t read_write_initial = 0x72772d696e697431ULL;
    const uint64_t read_write_updated = 0x72772d7570647432ULL;
    const uint64_t amo_initial = 0x0102030405060708ULL;
    const uint64_t amo_addend = 0x1010101010101010ULL;
    const uint64_t read_execute_value = 0x72782d6869743031ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(read_paddr, read_value);
    write64(read_write_paddr, read_write_initial);
    write64(amo_paddr, amo_initial);
    write32(execute_paddr, 0x00008067U);
    write32(read_execute_paddr, 0x00008067U);
    write64(read_execute_paddr + 8U, read_execute_value);
    fence_iorw_iorw();
    ai_memblock_pte_ptw_fence_i();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_pte_ptw_set_asid(AI_MEMBLOCK_PTE_PTW_ASID_A);
    sfence_vma();

    goto_priv(PRIV_M);
    const uint64_t read_leaf = *ai_memblock_pte_ptw_hs_leaf_entry(read_page);
    const uint64_t read_write_leaf =
        *ai_memblock_pte_ptw_hs_leaf_entry(read_write_page);
    const uint64_t execute_leaf =
        *ai_memblock_pte_ptw_hs_leaf_entry(execute_page);
    const uint64_t read_execute_leaf =
        *ai_memblock_pte_ptw_hs_leaf_entry(read_execute_page);
    TEST_ASSERT("the legal PTE permission matrix starts with explicit read-only, read-write, execute-only, and read-execute leaves in one ASID context",
        ai_memblock_pte_ptw_get_asid() == AI_MEMBLOCK_PTE_PTW_ASID_A &&
        (read_leaf & (PTE_V | PTE_A | PTE_D | PTE_R)) ==
            (PTE_V | PTE_A | PTE_D | PTE_R) &&
        (read_leaf & (PTE_W | PTE_X | PTE_U | PTE_G)) == 0 &&
        (read_write_leaf & (PTE_V | PTE_A | PTE_D | PTE_R | PTE_W)) ==
            (PTE_V | PTE_A | PTE_D | PTE_R | PTE_W) &&
        (read_write_leaf & (PTE_X | PTE_U | PTE_G)) == 0 &&
        (execute_leaf & (PTE_V | PTE_A | PTE_D | PTE_X)) ==
            (PTE_V | PTE_A | PTE_D | PTE_X) &&
        (execute_leaf & (PTE_R | PTE_W | PTE_U | PTE_G)) == 0 &&
        (read_execute_leaf & (PTE_V | PTE_A | PTE_D | PTE_R | PTE_X)) ==
            (PTE_V | PTE_A | PTE_D | PTE_R | PTE_X) &&
        (read_execute_leaf & (PTE_W | PTE_U | PTE_G)) == 0
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    uint64_t read_observed = ld(read_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS load from the read-only legal leaf succeeds without using MXR",
        read_observed == read_value);

    TEST_SETUP_EXCEPT();
    uint64_t read_write_observed = ld(read_write_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS load from the read-write legal leaf returns the initial payload",
        read_write_observed == read_write_initial);

    TEST_SETUP_EXCEPT();
    sd(read_write_vaddr, read_write_updated);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS store through the read-write legal leaf retires without a fault",
        true);

    TEST_SETUP_EXCEPT();
    read_write_observed = ld(read_write_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the read-write legal leaf observes the stored payload on a repeated hit",
        read_write_observed == read_write_updated);

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoadd_d(amo_vaddr, amo_addend);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS AMO through the read-write legal leaf uses the write-class permission path and returns the old payload",
        amo_old == amo_initial);

    TEST_SETUP_EXCEPT();
    ((void (*)(void))execute_vaddr)();
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS fetch from the execute-only legal leaf retires the installed ret instruction",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t read_execute_observed = ld(read_execute_vaddr + 8U);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS load from the read-execute legal leaf succeeds with MXR cleared",
        read_execute_observed == read_execute_value);

    TEST_SETUP_EXCEPT();
    ((void (*)(void))read_execute_vaddr)();
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS fetch from the read-execute legal leaf also retires the installed ret instruction",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("legal PTE permission hits preserve the leaf encodings and update only the read-write backing word",
        *ai_memblock_pte_ptw_hs_leaf_entry(read_page) == read_leaf &&
        *ai_memblock_pte_ptw_hs_leaf_entry(read_write_page) == read_write_leaf &&
        *ai_memblock_pte_ptw_hs_leaf_entry(execute_page) == execute_leaf &&
        *ai_memblock_pte_ptw_hs_leaf_entry(read_execute_page) ==
            read_execute_leaf &&
        read64(read_paddr) == read_value &&
        read64(read_write_paddr) == read_write_updated &&
        read64(amo_paddr) == amo_initial + amo_addend &&
        read32(execute_paddr) == 0x00008067U &&
        read32(read_execute_paddr) == 0x00008067U &&
        read64(read_execute_paddr + 8U) == read_execute_value
    );

    TEST_END();
}

bool ai_micro_memblock_u_mode_4k_hit_miss_pte_consistent_corner()
{
    TEST_START();

    const enum test_page page = VSURWX_GURWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x1c0U;
    const uintptr_t paddr = phys_page_base(page) + 0x1c0U;
    const uintptr_t amo_vaddr = hs_page_base(page) + 0x1c8U;
    const uintptr_t amo_paddr = phys_page_base(page) + 0x1c8U;
    const uintptr_t fetch_vaddr = hs_page_base(page) + 0x1d0U;
    const uintptr_t fetch_paddr = phys_page_base(page) + 0x1d0U;
    const uintptr_t lrsc_vaddr = hs_page_base(page) + 0x1d8U;
    const uintptr_t lrsc_paddr = phys_page_base(page) + 0x1d8U;
    const uint64_t initial = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t updated = 0x7766554433221100ULL;
    const uint64_t amo_initial = 0x0908070605040302ULL;
    const uint64_t amo_addend = 0x0001000100010001ULL;
    const uint64_t lrsc_initial = 0x13572468ace0bdf0ULL;
    const uint64_t lrsc_updated = 0x24681357bdf0ace0ULL;
    const uint64_t expected_flags = PTE_V | PTE_U | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_ECU);
    write64(paddr, initial);
    write64(amo_paddr, amo_initial);
    write64(lrsc_paddr, lrsc_initial);
    write32(fetch_paddr, 0x00008067U);
    fence_iorw_iorw();
    ai_memblock_pte_ptw_fence_i();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    ai_memblock_pte_ptw_set_asid(AI_MEMBLOCK_PTE_PTW_ASID_A);
    sfence_vma();

    goto_priv(PRIV_M);
    const uint64_t leaf_before = *ai_memblock_pte_ptw_hs_leaf_entry(page);
    TEST_ASSERT("the U-mode legal 4K leaf carries V/R/W/X/U/A/D, clears G, and is tagged by the selected ASID context",
        ai_memblock_pte_ptw_get_asid() == AI_MEMBLOCK_PTE_PTW_ASID_A &&
        ai_memblock_pte_ptw_leaf_flags(page) == expected_flags &&
        (leaf_before & PTE_G) == 0 &&
        ((leaf_before << 2) & PAGE_ADDR_MSK) == phys_page_base(page)
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t miss_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the first HU-mode 4K load walks a cold legal U leaf and returns the backing payload",
        miss_load == initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t hit_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the repeated HU-mode 4K load hits the established legal U translation and returns the same payload",
        hit_load == initial
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, updated);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the HU-mode 4K store through the legal U translated hit path retires without a fault",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t after_store = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the HU-mode 4K alias observes the value stored through the same legal U translation",
        after_store == updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoadd_d(amo_vaddr, amo_addend);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the HU-mode 4K AMO uses the same legal U translated permission path and returns the old payload",
        amo_old == amo_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t lrsc_old = 0ULL;
    uint64_t lrsc_status = ai_memblock_pte_ptw_lrsc_d_pair(lrsc_vaddr,
        lrsc_updated, &lrsc_old);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the HU-mode 4K LR/SC pair uses the same legal U translated permission path and commits the SC update",
        lrsc_old == lrsc_initial && lrsc_status == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ((void (*)(void))fetch_vaddr)();
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the HU-mode 4K fetch through the same legal U leaf retires the installed ret instruction",
        true
    );

    goto_priv_u_to_m_direct(PRIV_M);
    TEST_SETUP_EXCEPT();
    TEST_ASSERT("U-mode legal 4K translation keeps the PTE snapshot stable and updates the expected physical words",
        *ai_memblock_pte_ptw_hs_leaf_entry(page) == leaf_before &&
        read64(paddr) == updated &&
        read64(amo_paddr) == amo_initial + amo_addend &&
        read64(lrsc_paddr) == lrsc_updated &&
        read32(fetch_paddr) == 0x00008067U
    );

    TEST_END();
}

bool ai_micro_memblock_pte_ad_one_load_store_preserves_bits_corner()
{
    TEST_START();

    const enum test_page load_page = VSR_GR;
    const enum test_page store_page = VSRW_GRW;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x208U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x208U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x210U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x210U;
    const uint64_t load_value = 0x3141592653589793ULL;
    const uint64_t store_initial = 0x2718281828459045ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(load_paddr, load_value);
    write64(store_paddr, store_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    const uint64_t load_leaf_before = *ai_memblock_pte_ptw_hs_leaf_entry(load_page);
    const uint64_t store_leaf_before = *ai_memblock_pte_ptw_hs_leaf_entry(store_page);
    TEST_ASSERT("the legal A/D baseline starts with PTE.A=1 for the load leaf and PTE.D=1 for the store leaf",
        (load_leaf_before & PTE_A) != 0 &&
        (store_leaf_before & PTE_A) != 0 &&
        (store_leaf_before & PTE_D) != 0
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(load_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("loading from a legal PTE.A=1 leaf succeeds without requesting another A-bit update",
        loaded == load_value
    );

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_value);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("storing through a legal PTE.D=1 leaf succeeds without requesting another D-bit update",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t stored = ld(store_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the D=1 store path leaves the translated alias readable with the new payload",
        stored == store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("legal PTE.A=1 load and PTE.D=1 store leave the PTE words unchanged while preserving data visibility",
        *ai_memblock_pte_ptw_hs_leaf_entry(load_page) == load_leaf_before &&
        *ai_memblock_pte_ptw_hs_leaf_entry(store_page) == store_leaf_before &&
        read64(load_paddr) == load_value &&
        read64(store_paddr) == store_value
    );

    TEST_END();
}

bool ai_micro_memblock_asid_switch_refills_non_global_translation_corner()
{
    TEST_START();

    const enum test_page va_page = SWITCH1;
    const enum test_page asid_zero_page = VSRWX_GRWX;
    const enum test_page asid_b_page = VSRW_GRW;
    const uintptr_t vaddr = hs_page_base(va_page) + 0x260U;
    const uintptr_t asid_zero_paddr = phys_page_base(asid_zero_page) + 0x260U;
    const uintptr_t asid_b_paddr = phys_page_base(asid_b_page) + 0x260U;
    const uint64_t asid_zero_value = 0xa0a1a2a3a4a5a6a7ULL;
    const uint64_t asid_b_value = 0xb0b1b2b3b4b5b6b7ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(asid_zero_paddr, asid_zero_value);
    write64(asid_b_paddr, asid_b_value);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_pte_ptw_set_hs_leaf_to_phys(va_page, asid_zero_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_pte_ptw_set_asid(0);
    sfence_vma();

    goto_priv(PRIV_M);
    const uint64_t asid_zero_leaf = *ai_memblock_pte_ptw_hs_leaf_entry(va_page);
    TEST_ASSERT("the ASID=0 non-global leaf maps the VA to the first physical page before the refill probe",
        ai_memblock_pte_ptw_get_asid() == 0 &&
        (asid_zero_leaf & PTE_G) == 0 &&
        ((asid_zero_leaf << 2) & PAGE_ADDR_MSK) == phys_page_base(asid_zero_page)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_zero = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("ASID=0 first access builds a non-global translation for the old physical page",
        observed_zero == asid_zero_value
    );

    goto_priv(PRIV_M);
    ai_memblock_pte_ptw_set_hs_leaf_to_phys(va_page, asid_b_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_pte_ptw_set_asid(AI_MEMBLOCK_PTE_PTW_ASID_B);
    const uint64_t asid_b_leaf = *ai_memblock_pte_ptw_hs_leaf_entry(va_page);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_b = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("switching from ASID=0 to ASID-B does not match the old non-global TLB entry and refills from the current PTE",
        observed_b == asid_b_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the ASID-B refill uses the remapped physical page while the old page remains untouched",
        ai_memblock_pte_ptw_get_asid() == AI_MEMBLOCK_PTE_PTW_ASID_B &&
        (asid_b_leaf & PTE_G) == 0 &&
        ((asid_b_leaf << 2) & PAGE_ADDR_MSK) == phys_page_base(asid_b_page) &&
        read64(asid_zero_paddr) == asid_zero_value &&
        read64(asid_b_paddr) == asid_b_value
    );

    TEST_END();
}

bool ai_micro_memblock_global_pte_cross_asid_cached_hit_corner()
{
    TEST_START();

    const enum test_page va_page = SWITCH2;
    const enum test_page global_old_page = VSRWX_GRWX;
    const enum test_page global_new_page = VSRW_GRW;
    const uintptr_t vaddr = hs_page_base(va_page) + 0x2a0U;
    const uintptr_t old_paddr = phys_page_base(global_old_page) + 0x2a0U;
    const uintptr_t new_paddr = phys_page_base(global_new_page) + 0x2a0U;
    const uint64_t old_value = 0x474c4f42414c4131ULL;
    const uint64_t new_value = 0x474c4f42414c4232ULL;
    const uint64_t global_leaf_flags = PTE_V | PTE_AD | PTE_RWX | PTE_G;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(old_paddr, old_value);
    write64(new_paddr, new_value);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_pte_ptw_set_hs_leaf_to_phys(va_page, global_old_page,
        global_leaf_flags);
    ai_memblock_pte_ptw_set_asid(0);
    sfence_vma();

    goto_priv(PRIV_M);
    const uint64_t old_global_leaf =
        *ai_memblock_pte_ptw_hs_leaf_entry(va_page);
    TEST_ASSERT("the ASID=0 setup installs a global leaf mapping the VA to the old physical page",
        ai_memblock_pte_ptw_get_asid() == 0 &&
        (old_global_leaf & PTE_G) != 0 &&
        ((old_global_leaf << 2) & PAGE_ADDR_MSK) ==
            phys_page_base(global_old_page)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_old = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("ASID=0 first access warms the PTE.G translation from the old physical page",
        observed_old == old_value
    );

    goto_priv(PRIV_M);
    ai_memblock_pte_ptw_set_hs_leaf_to_phys(va_page, global_new_page,
        global_leaf_flags);
    const uint64_t new_global_leaf =
        *ai_memblock_pte_ptw_hs_leaf_entry(va_page);
    ai_memblock_pte_ptw_set_asid(AI_MEMBLOCK_PTE_PTW_ASID_B);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_cross_asid = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("switching from ASID=0 to ASID-B without sfence.vma still hits the cached PTE.G translation instead of walking the remapped leaf",
        observed_cross_asid == old_value
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    sfence_vma();
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("sfence.vma x0,x0 retires before forcing the global mapping to observe the remapped leaf",
        ai_memblock_pte_ptw_get_asid() == AI_MEMBLOCK_PTE_PTW_ASID_B
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_after_flush = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after the explicit all-entry sfence, the same VA walks the current PTE.G leaf and reads the new physical page",
        observed_after_flush == new_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the global cross-ASID probe preserves both backing pages and ends with the remapped global PTE installed",
        (new_global_leaf & PTE_G) != 0 &&
        ((new_global_leaf << 2) & PAGE_ADDR_MSK) ==
            phys_page_base(global_new_page) &&
        read64(old_paddr) == old_value &&
        read64(new_paddr) == new_value
    );

    TEST_END();
}

bool ai_micro_memblock_pte_u_update_requires_sfence_old_entry_boundary_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const enum test_page guard_page = SWITCH2;
    const uintptr_t vaddr = hs_page_base(page) + 0x318U;
    const uintptr_t paddr = phys_page_base(page) + 0x318U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x348U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x348U;
    const uint64_t initial = 0x505445552d4f4c44ULL;
    const uint64_t hu_update = 0x505445552d485531ULL;
    const uint64_t hs_update = 0x505445552d485332ULL;
    const uint64_t guard_initial = 0x47554152442d5054ULL;
    const uint64_t denied_sentinel = 0xd00d00000000a111ULL;
    const uint64_t stale_sentinel = 0xd00d00000000b222ULL;
    const uint64_t cleared_sentinel = 0xd00d00000000c333ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_ECU);
    write64(paddr, initial);
    write64(guard_paddr, guard_initial);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow_no_leaf();
    ai_memblock_pte_ptw_set_asid(AI_MEMBLOCK_PTE_PTW_ASID_A);
    *ai_memblock_pte_ptw_hs_leaf_entry(page) =
        (*ai_memblock_pte_ptw_hs_leaf_entry(page) & ~PTE_U) |
        PTE_V | PTE_AD | PTE_RWX;
    *ai_memblock_pte_ptw_hs_leaf_entry(guard_page) =
        (*ai_memblock_pte_ptw_hs_leaf_entry(guard_page) & ~PTE_U) |
        PTE_V | PTE_AD | PTE_RWX;
    const uint64_t supervisor_leaf =
        *ai_memblock_pte_ptw_hs_leaf_entry(page);
    const uint64_t guard_leaf =
        *ai_memblock_pte_ptw_hs_leaf_entry(guard_page);
    sfence_vma();
    TEST_ASSERT("the PTE.U/SFENCE boundary probe starts with a supervisor leaf under U-enabled non-leaf page-table entries",
        ai_memblock_pte_ptw_get_asid() == AI_MEMBLOCK_PTE_PTW_ASID_A &&
        (supervisor_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (supervisor_leaf & PTE_U) == 0 &&
        (guard_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (guard_leaf & PTE_U) == 0
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t hs_warm = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS load warms the PTE.U=0 supervisor translation before switching current privilege",
        hs_warm == initial);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t denied_rd = ai_memblock_pte_ptw_ld_rd_probe(vaddr,
        denied_sentinel);
    TEST_ASSERT("HU load that hits the supervisor translation still rechecks current privilege/PTE.U and reports LPF without writing rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        denied_rd == denied_sentinel);

    goto_priv_u_to_m_direct(PRIV_M);
    *ai_memblock_pte_ptw_hs_leaf_entry(page) = supervisor_leaf | PTE_U;
    const uint64_t user_leaf = *ai_memblock_pte_ptw_hs_leaf_entry(page);
    fence_iorw_iorw();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t stale_rd = ai_memblock_pte_ptw_ld_rd_probe(vaddr,
        stale_sentinel);
    TEST_ASSERT("after software sets PTE.U=1 without SFENCE.VMA, the old supervisor translation boundary is still allowed to fault until the entry is explicitly invalidated",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        stale_rd == stale_sentinel);

    goto_priv_u_to_m_direct(PRIV_M);
    sfence_vma_rs(vaddr, AI_MEMBLOCK_PTE_PTW_ASID_A);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t hu_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after exact SFENCE.VMA, the same HU load observes the new PTE.U=1 permission",
        hu_load == initial);

    TEST_SETUP_EXCEPT();
    sd(vaddr, hu_update);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after exact SFENCE.VMA, the HU store also uses the refreshed user leaf",
        true);

    goto_priv_u_to_m_direct(PRIV_M);
    *ai_memblock_pte_ptw_hs_leaf_entry(page) = user_leaf & ~PTE_U;
    const uint64_t cleared_leaf = *ai_memblock_pte_ptw_hs_leaf_entry(page);
    sfence_vma_rs(vaddr, AI_MEMBLOCK_PTE_PTW_ASID_A);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t cleared_rd = ai_memblock_pte_ptw_ld_rd_probe(vaddr,
        cleared_sentinel);
    TEST_ASSERT("after clearing PTE.U and invalidating the VA/ASID entry, HU access faults again instead of using the stale user permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        cleared_rd == cleared_sentinel);

    goto_priv_u_to_m_direct(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t hs_after_clear = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS access remains legal after the final PTE.U clear and proves the refreshed leaf is still a supervisor mapping",
        hs_after_clear == hu_update);

    TEST_SETUP_EXCEPT();
    sd(vaddr, hs_update);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS store through the final supervisor leaf succeeds after the HU fault path",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t guard_load = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("an unrelated supervisor guard translation remains usable after repeated PTE.U updates and SFENCE.VMA",
        guard_load == guard_initial);

    goto_priv(PRIV_M);
    TEST_ASSERT("PTE.U update boundary checks preserve the final leaf words and update only the target backing word",
        *ai_memblock_pte_ptw_hs_leaf_entry(page) == cleared_leaf &&
        *ai_memblock_pte_ptw_hs_leaf_entry(guard_page) == guard_leaf &&
        read64(paddr) == hs_update &&
        read64(guard_paddr) == guard_initial
    );

    TEST_END();
}

bool ai_micro_memblock_pte_structure_remap_sfence_rejudge_corner()
{
    TEST_START();

    const uintptr_t vaddr = 0x0067c380ULL;
    const uintptr_t leaf_base_vaddr = vaddr & PAGE_ADDR_MSK;
    const uintptr_t superpage_2m_size = PAGE_SIZE << 9;
    const uintptr_t paddr_old = phys_page_base(SWITCH1) + 0x380U;
    const uintptr_t paddr_new = phys_page_base(SWITCH2) + 0x380U;
    const uintptr_t paddr_2m_base = TEST_PPAGE_BASE;
    const uintptr_t paddr_2m = paddr_2m_base + (vaddr & (superpage_2m_size - 1ULL));
    const uintptr_t old_guard_paddr = paddr_old + sizeof(uint64_t);
    const uintptr_t new_guard_paddr = paddr_new + sizeof(uint64_t);
    const uint64_t old_value = 0x7074652d70706e31ULL;
    const uint64_t new_value = 0x7074652d70706e32ULL;
    const uint64_t update_new = 0x7074652d73747732ULL;
    const uint64_t super_value = 0x73757065722d326dULL;
    const uint64_t update_super = 0x73757065722d7570ULL;
    const uint64_t old_guard = 0x1111222233334444ULL;
    const uint64_t new_guard = 0x5555666677778888ULL;
    const uint64_t flags = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(paddr_old, old_value);
    write64(paddr_new, new_value);
    write64(paddr_2m, super_value);
    write64(old_guard_paddr, old_guard);
    write64(new_guard_paddr, new_guard);
    ai_memblock_pte_ptw_zero_pt(ai_memblock_pte_ptw_sv39_root);
    ai_memblock_pte_ptw_zero_pt(ai_memblock_pte_ptw_sv39_l1);
    ai_memblock_pte_ptw_zero_pt(ai_memblock_pte_ptw_sv39_l0);
    ai_memblock_pte_ptw_sv39_root[
        AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(leaf_base_vaddr)] =
            PTE_V | (((uintptr_t)ai_memblock_pte_ptw_sv39_l1) >> 2);
    ai_memblock_pte_ptw_sv39_l1[
        AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(leaf_base_vaddr)] =
            PTE_V | (((uintptr_t)ai_memblock_pte_ptw_sv39_l0) >> 2);
    ai_memblock_pte_ptw_sv39_l0[
        AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(leaf_base_vaddr)] =
            ((paddr_old & PAGE_ADDR_MSK) >> 2) | flags;
    ai_memblock_pte_ptw_sv39_root[AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(MEM_BASE)] =
        (MEM_BASE >> 2) | flags;
    const uint64_t root_entry =
        ai_memblock_pte_ptw_sv39_root[
            AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(leaf_base_vaddr)];
    const uint64_t nonleaf_l1_entry =
        ai_memblock_pte_ptw_sv39_l1[
            AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(leaf_base_vaddr)];
    const uint64_t old_l0_leaf =
        ai_memblock_pte_ptw_sv39_l0[
            AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(leaf_base_vaddr)];
    CSRW(CSR_SATP,
        (AI_MEMBLOCK_PTE_PTW_SATP_MODE_SV39 << 60) |
        (((uintptr_t)ai_memblock_pte_ptw_sv39_root) >> 12));
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the structure-remap probe starts with a three-level 4KB leaf mapping the VA to the old physical page",
        (root_entry & PTE_V) != 0 &&
        (nonleaf_l1_entry & (PTE_V | PTE_RWX)) == PTE_V &&
        (old_l0_leaf & flags) == flags &&
        ((old_l0_leaf << 2) & PAGE_ADDR_MSK) == (paddr_old & PAGE_ADDR_MSK));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the first HS load walks the original 4KB leaf and reads the old physical payload",
        first_load == old_value);

    goto_priv(PRIV_M);
    ai_memblock_pte_ptw_sv39_l0[
        AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(leaf_base_vaddr)] =
            ((paddr_new & PAGE_ADDR_MSK) >> 2) | flags;
    const uint64_t new_l0_leaf =
        ai_memblock_pte_ptw_sv39_l0[
            AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(leaf_base_vaddr)];
    fence_iorw_iorw();
    sfence_vma_rs(vaddr, (uint64_t)-1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t remap_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after changing the 4KB PPN and executing SFENCE.VMA, the same VA reads the new physical page",
        remap_load == new_value);

    TEST_SETUP_EXCEPT();
    sd(vaddr, update_new);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("a store after the 4KB remap updates only the new physical page",
        true);

    goto_priv(PRIV_M);
    ai_memblock_pte_ptw_sv39_l1[
        AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(leaf_base_vaddr)] =
            (paddr_2m_base >> 2) | flags;
    const uint64_t super_l1_leaf =
        ai_memblock_pte_ptw_sv39_l1[
            AI_MEMBLOCK_PTE_PTW_SV39_L1_INDEX(leaf_base_vaddr)];
    fence_iorw_iorw();
    sfence_vma_rs(vaddr, (uint64_t)-1);
    TEST_ASSERT("the page-table store converts the same VA from non-leaf+L0 walk into a 2MB leaf before the next access",
        (super_l1_leaf & flags) == flags &&
        (super_l1_leaf & PTE_RWX) != 0 &&
        (((super_l1_leaf & ~PTE_FLAGS_MSK) << 2) &
            (superpage_2m_size - 1ULL)) == 0 &&
        ((super_l1_leaf << 2) & ~(superpage_2m_size - 1ULL)) ==
            paddr_2m_base);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t super_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after replacing the non-leaf with a legal 2MB leaf and fencing, the same VA uses the superpage PPN concatenation",
        super_load == super_value);

    TEST_SETUP_EXCEPT();
    sd(vaddr, update_super);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the superpage translation accepts a store after the SFENCE.VMA rewalk",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("PPN remap and leaf/non-leaf page-size switching isolate old, new, and superpage physical images",
        read64(paddr_old) == old_value &&
        read64(paddr_new) == update_new &&
        read64(paddr_2m) == update_super &&
        read64(old_guard_paddr) == old_guard &&
        read64(new_guard_paddr) == new_guard &&
        ai_memblock_pte_ptw_sv39_root[
            AI_MEMBLOCK_PTE_PTW_SV39_L2_INDEX(leaf_base_vaddr)] ==
            root_entry &&
        ai_memblock_pte_ptw_sv39_l0[
            AI_MEMBLOCK_PTE_PTW_SV39_L0_INDEX(leaf_base_vaddr)] ==
            new_l0_leaf);

    TEST_END();
}
