#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_AP52_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_AP52_ASSERT_IPF(message, vaddr) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == CAUSE_IPF && \
        excpt.tval == (vaddr))

#define AI_AP52_SATP_MODE_SV39 8ULL
#define AI_AP52_SV39_L2_INDEX(addr) (((addr) >> 30) & 0x1ffULL)
#define AI_AP52_SV39_L1_INDEX(addr) (((addr) >> 21) & 0x1ffULL)
#define AI_AP52_SV39_L0_INDEX(addr) (((addr) >> 12) & 0x1ffULL)
#define AI_AP52_SVINVAL_ASID 0x52ULL

static pte_t ai_ap52_root_a[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ap52_l1_a[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ap52_l0_a[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ap52_root_b[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ap52_l1_b[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ap52_l0_b[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ap53_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ap53_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

static inline pte_t *ai_ap52_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ap52_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0;
    }
}

static inline uint64_t ai_ap52_ret_imm(uint64_t imm)
{
    return 0x0000806700000513ULL | ((imm & 0xfffULL) << 20);
}

static inline void ai_ap52_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}

static inline uint64_t ai_ap52_call_ret64(uintptr_t vaddr)
{
    uint64_t (*fn)(void) = (uint64_t (*)(void))vaddr;
    return fn();
}

static inline void ai_ap52_set_hs_leaf_to_paddr(enum test_page va_page,
    uintptr_t paddr, uint64_t flags)
{
    *ai_ap52_hs_leaf_entry(va_page) = ((paddr & PAGE_ADDR_MSK) >> 2) | flags;
}

static inline void ai_ap52_install_sv39_4k_root(pte_t *root, pte_t *l1,
    pte_t *l0, uintptr_t vaddr, uintptr_t paddr, uint64_t leaf_flags)
{
    ai_ap52_zero_pt(root);
    ai_ap52_zero_pt(l1);
    ai_ap52_zero_pt(l0);

    root[AI_AP52_SV39_L2_INDEX(vaddr)] = PTE_V | (((uintptr_t)l1) >> 2);
    root[AI_AP52_SV39_L2_INDEX(MEM_BASE)] = (MEM_BASE >> 2) | leaf_flags;
    l1[AI_AP52_SV39_L1_INDEX(vaddr)] = PTE_V | (((uintptr_t)l0) >> 2);
    l0[AI_AP52_SV39_L0_INDEX(vaddr)] =
        ((paddr & PAGE_ADDR_MSK) >> 2) | leaf_flags;
}

static inline void ai_ap53_install_sv39_2m_root(uintptr_t vaddr_base,
    uintptr_t paddr_base, uint64_t leaf_flags)
{
    ai_ap52_zero_pt(ai_ap53_root);
    ai_ap52_zero_pt(ai_ap53_l1);

    ai_ap53_root[AI_AP52_SV39_L2_INDEX(vaddr_base)] =
        PTE_V | (((uintptr_t)ai_ap53_l1) >> 2);
    ai_ap53_root[AI_AP52_SV39_L2_INDEX(MEM_BASE)] =
        (MEM_BASE >> 2) | leaf_flags;
    ai_ap53_l1[AI_AP52_SV39_L1_INDEX(vaddr_base)] =
        (paddr_base >> 2) | leaf_flags;
}

static inline void ai_ap52_sinval_vma(void)
{
    asm volatile(
        ".insn r 0x73, 0x0, 0x0B, x0, x0, x0\n\t"
        ::: "memory");
}

static inline void ai_ap52_sfence_w_inval(void)
{
    asm volatile(".word 0x18000073\n\t" ::: "memory");
}

static inline void ai_ap52_sfence_inval_ir(void)
{
    asm volatile(".word 0x18100073\n\t" ::: "memory");
}

bool ai_micro_ap52_legal_sfence_ifetch_rewalk_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWX_GRWX;
    const enum test_page new_phys_page = SWITCH1;
    const uintptr_t off = 0x240U;
    const uintptr_t code_vaddr = hs_page_base(code_page) + off;
    const uintptr_t old_paddr = phys_page_base(code_page) + off;
    const uintptr_t new_paddr = phys_page_base(new_phys_page) + off;
    const uint64_t old_ret = ai_ap52_ret_imm(0x52U);
    const uint64_t new_ret = ai_ap52_ret_imm(0x53U);
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(old_paddr, old_ret);
    write64(new_paddr, new_ret);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_ap52_fence_i();

    TEST_SETUP_EXCEPT();
    uint64_t old_observed = ai_ap52_call_ret64(code_vaddr);
    AI_AP52_ASSERT_NO_EXCEPT_AND("AP-52 legal single-SFENCE IFU probe first fetches the original executable mapping",
        old_observed == 0x52U);

    goto_priv(PRIV_M);
    ai_ap52_set_hs_leaf_to_paddr(code_page, phys_page_base(new_phys_page),
        leaf_flags);
    const uint64_t remap_leaf = *ai_ap52_hs_leaf_entry(code_page);
    fence_iorw_iorw();
    sfence_vma();
    ai_ap52_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t new_observed = ai_ap52_call_ret64(code_vaddr);
    AI_AP52_ASSERT_NO_EXCEPT_AND("after remapping the same fetch VA and executing one legal sfence.vma, IFU rewalks and executes the new physical code image",
        new_observed == 0x53U);

    goto_priv(PRIV_M);
    ai_ap52_set_hs_leaf_to_paddr(code_page, phys_page_base(new_phys_page),
        PTE_V | PTE_AD | PTE_RW);
    const uint64_t no_exec_leaf = *ai_ap52_hs_leaf_entry(code_page);
    fence_iorw_iorw();
    sfence_vma();
    ai_ap52_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    AI_AP52_ASSERT_IPF("after the post-remap leaf is made non-executable and flushed, IFU does not use a stale executable translation",
        code_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-52 single-SFENCE manual probe preserves code images and records the expected remap/no-X leaves",
        read64(old_paddr) == old_ret &&
        read64(new_paddr) == new_ret &&
        (remap_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (no_exec_leaf & (PTE_V | PTE_AD | PTE_RW)) ==
            (PTE_V | PTE_AD | PTE_RW) &&
        (no_exec_leaf & PTE_X) == 0);

    TEST_END();
}

bool ai_micro_ap52_satp_context_ifetch_rewalk_manual_corner()
{
    TEST_START();

    const uintptr_t vaddr = 0x0067a240ULL;
    const uintptr_t off = 0x240U;
    const uintptr_t paddr_a = phys_page_base(SWITCH1) + off;
    const uintptr_t paddr_b = phys_page_base(SWITCH2) + off;
    const uint64_t ret_a = ai_ap52_ret_imm(0x5aU);
    const uint64_t ret_b = ai_ap52_ret_imm(0x5bU);
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t satp_a =
        (AI_AP52_SATP_MODE_SV39 << 60) |
        (((uintptr_t)ai_ap52_root_a) >> 12);
    const uint64_t satp_b =
        (AI_AP52_SATP_MODE_SV39 << 60) |
        (((uintptr_t)ai_ap52_root_b) >> 12);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(paddr_a, ret_a);
    write64(paddr_b, ret_b);
    ai_ap52_install_sv39_4k_root(ai_ap52_root_a, ai_ap52_l1_a,
        ai_ap52_l0_a, vaddr, paddr_a, leaf_flags);
    ai_ap52_install_sv39_4k_root(ai_ap52_root_b, ai_ap52_l1_b,
        ai_ap52_l0_b, vaddr, paddr_b, leaf_flags);
    const uint64_t l0_a_leaf =
        ai_ap52_l0_a[AI_AP52_SV39_L0_INDEX(vaddr)];
    const uint64_t l0_b_leaf =
        ai_ap52_l0_b[AI_AP52_SV39_L0_INDEX(vaddr)];
    fence_iorw_iorw();
    ai_ap52_fence_i();

    TEST_ASSERT("AP-52 satp-context fetch probe installs two roots mapping the same fetch VA to different physical code pages",
        satp_a != satp_b &&
        ((l0_a_leaf << 2) & PAGE_ADDR_MSK) == (paddr_a & PAGE_ADDR_MSK) &&
        ((l0_b_leaf << 2) & PAGE_ADDR_MSK) == (paddr_b & PAGE_ADDR_MSK));

    CSRW(CSR_SATP, satp_a);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_a = ai_ap52_call_ret64(vaddr);
    AI_AP52_ASSERT_NO_EXCEPT_AND("the first IFU fetch walks satp root A and executes root-A code",
        observed_a == 0x5aU);

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, satp_b);
    sfence_vma();
    ai_ap52_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_b = ai_ap52_call_ret64(vaddr);
    AI_AP52_ASSERT_NO_EXCEPT_AND("after the legal satp root switch plus sfence.vma, IFU fetches from root B instead of stale root A",
        observed_b == 0x5bU);

    goto_priv(PRIV_M);
    ai_ap52_l0_b[AI_AP52_SV39_L0_INDEX(vaddr)] =
        ((paddr_b & PAGE_ADDR_MSK) >> 2) | (PTE_V | PTE_AD | PTE_RW);
    const uint64_t l0_b_no_exec =
        ai_ap52_l0_b[AI_AP52_SV39_L0_INDEX(vaddr)];
    fence_iorw_iorw();
    sfence_vma();
    ai_ap52_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    AI_AP52_ASSERT_IPF("after root B clears execute permission and flushes, the same fetch VA faults instead of using root A's stale executable entry",
        vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-52 satp-context manual probe preserves both code pages and the final non-executable root-B leaf",
        CSRR(CSR_SATP) == satp_b &&
        read64(paddr_a) == ret_a &&
        read64(paddr_b) == ret_b &&
        ai_ap52_l0_a[AI_AP52_SV39_L0_INDEX(vaddr)] == l0_a_leaf &&
        ai_ap52_l0_b[AI_AP52_SV39_L0_INDEX(vaddr)] == l0_b_no_exec);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap52_svinval_ifetch_closeout_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWX_GRWX;
    const enum test_page new_phys_page = SWITCH1;
    const uintptr_t off = 0x340U;
    const uintptr_t code_vaddr = hs_page_base(code_page) + off;
    const uintptr_t old_paddr = phys_page_base(code_page) + off;
    const uintptr_t new_paddr = phys_page_base(new_phys_page) + off;
    const uint64_t old_ret = ai_ap52_ret_imm(0x62U);
    const uint64_t new_ret = ai_ap52_ret_imm(0x63U);
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(old_paddr, old_ret);
    write64(new_paddr, new_ret);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    uint64_t satp_before = CSRR(CSR_SATP);
    satp_before &= ~(0xffffULL << 44);
    satp_before |= (AI_AP52_SVINVAL_ASID << 44);
    CSRW(CSR_SATP, satp_before);
    sfence_vma();
    ai_ap52_fence_i();

    TEST_SETUP_EXCEPT();
    uint64_t old_observed = ai_ap52_call_ret64(code_vaddr);
    AI_AP52_ASSERT_NO_EXCEPT_AND("AP-52 Svinval closeout probe first fetches through the original executable leaf",
        old_observed == 0x62U);

    goto_priv(PRIV_M);
    ai_ap52_set_hs_leaf_to_paddr(code_page, phys_page_base(new_phys_page),
        leaf_flags);
    const uint64_t remap_leaf = *ai_ap52_hs_leaf_entry(code_page);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap52_sfence_w_inval();
    AI_AP52_ASSERT_NO_EXCEPT_AND("legal HS sfence.w.inval begins the Svinval invalidation sequence",
        true);

    TEST_SETUP_EXCEPT();
    ai_ap52_sinval_vma();
    AI_AP52_ASSERT_NO_EXCEPT_AND("legal HS sinval.vma body retires during the Svinval invalidation sequence",
        true);

    TEST_SETUP_EXCEPT();
    ai_ap52_sfence_inval_ir();
    AI_AP52_ASSERT_NO_EXCEPT_AND("legal HS sfence.inval.ir closes the Svinval invalidation window",
        true);

    ai_ap52_fence_i();
    TEST_SETUP_EXCEPT();
    uint64_t new_observed = ai_ap52_call_ret64(code_vaddr);
    AI_AP52_ASSERT_NO_EXCEPT_AND("after the legal Svinval begin/body/closeout sequence, IFU fetch resumes on the remapped executable code",
        new_observed == 0x63U);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-52 Svinval manual probe leaves satp and code images stable after the closeout fetch",
        CSRR(CSR_SATP) == satp_before &&
        read64(old_paddr) == old_ret &&
        read64(new_paddr) == new_ret &&
        *ai_ap52_hs_leaf_entry(code_page) == remap_leaf);

    TEST_END();
}

bool ai_micro_ap53_largepage_ifetch_flush_stress_manual_corner()
{
    TEST_START();

    const uintptr_t vaddr_base = 0x00400000ULL;
    const uintptr_t paddr_base = TEST_PPAGE_BASE;
    const uintptr_t off0 = 0x1000ULL;
    const uintptr_t off1 = 0x3000ULL;
    const uintptr_t off2 = 0x5000ULL;
    const uintptr_t vaddr0 = vaddr_base + off0;
    const uintptr_t vaddr1 = vaddr_base + off1;
    const uintptr_t vaddr2 = vaddr_base + off2;
    const uintptr_t paddr0 = paddr_base + off0;
    const uintptr_t paddr1 = paddr_base + off1;
    const uintptr_t paddr2 = paddr_base + off2;
    const uint64_t ret0 = ai_ap52_ret_imm(0x71U);
    const uint64_t ret1 = ai_ap52_ret_imm(0x72U);
    const uint64_t ret2 = ai_ap52_ret_imm(0x73U);
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t satp =
        (AI_AP52_SATP_MODE_SV39 << 60) |
        (((uintptr_t)ai_ap53_root) >> 12);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(paddr0, ret0);
    write64(paddr1, ret1);
    write64(paddr2, ret2);
    ai_ap53_install_sv39_2m_root(vaddr_base, paddr_base, leaf_flags);
    const uint64_t l1_leaf =
        ai_ap53_l1[AI_AP52_SV39_L1_INDEX(vaddr_base)];
    fence_iorw_iorw();
    ai_ap52_fence_i();

    TEST_ASSERT("AP-53 manual stress starts from a legal Sv39 2MB IFU superpage mapping for three code targets",
        (((l1_leaf & ~PTE_FLAGS_MSK) << 2) &
            (SUPERPAGE_SIZE(2) - 1ULL)) == 0 &&
        ((l1_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)));

    CSRW(CSR_SATP, satp);
    sfence_vma();

    goto_priv(PRIV_HS);
    uint64_t sum = 0;
    TEST_SETUP_EXCEPT();
    for (unsigned i = 0; i < 12; ++i) {
        sum += ai_ap52_call_ret64(vaddr0);
        asm volatile("jal 1f\n1:\n\t" ::: "memory");
        sum += ai_ap52_call_ret64(vaddr1);
        sfence_vma();
        ai_ap52_fence_i();
        sum += ai_ap52_call_ret64(vaddr2);
    }
    AI_AP52_ASSERT_NO_EXCEPT_AND("AP-53 large-page IFU stress keeps making progress across repeated fetch redirects, sfence.vma, and fence.i flushes",
        sum == 12ULL * (0x71U + 0x72U + 0x73U));

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-53 manual stress preserves the single 2MB TLBFA-candidate leaf and all backing code images",
        CSRR(CSR_SATP) == satp &&
        ai_ap53_l1[AI_AP52_SV39_L1_INDEX(vaddr_base)] == l1_leaf &&
        read64(paddr0) == ret0 &&
        read64(paddr1) == ret1 &&
        read64(paddr2) == ret2);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();

    TEST_END();
}
