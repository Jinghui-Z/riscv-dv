#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_ARCH_SFENCE_ASSERT_PAGE_FAULT(message, expected_cause, vaddr) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == (expected_cause) && \
        excpt.tval == (vaddr))

#define AI_ARCH_SFENCE_SATP_MODE_SV39 8ULL
#define AI_ARCH_SFENCE_SV39_L2_INDEX(addr) (((addr) >> 30) & 0x1ffULL)
#define AI_ARCH_SFENCE_SV39_L1_INDEX(addr) (((addr) >> 21) & 0x1ffULL)
#define AI_ARCH_SFENCE_SV39_L0_INDEX(addr) (((addr) >> 12) & 0x1ffULL)
#define AI_ARCH_SFENCE_PMP_TOR_RWX 0x0fULL
#define AI_ARCH_SFENCE_PMP_TOR_NO_R 0x0cULL
#define AI_ARCH_SFENCE_PMP_TOR_NO_W 0x0dULL
#define AI_ARCH_SFENCE_ASID_A 0x0611ULL
#define AI_ARCH_SFENCE_SATP_ASID_SHIFT 44
#define AI_ARCH_SFENCE_SATP_ASID_MASK \
    (0xffffULL << AI_ARCH_SFENCE_SATP_ASID_SHIFT)

static pte_t ai_arch_sfence_sv39_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_arch_sfence_sv39_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_arch_sfence_sv39_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

static inline pte_t *ai_arch_sfence_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_sfence_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0;
    }
}

static inline void ai_arch_sfence_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}

static inline uint64_t ai_arch_sfence_ld_rd_probe(uintptr_t addr,
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

static inline void ai_arch_sfence_set_leaf(enum test_page page,
    enum test_page phys_page, uint64_t flags)
{
    *ai_arch_sfence_hs_leaf_entry(page) =
        (phys_page_base(phys_page) >> 2) | flags;
}

static inline void ai_arch_sfence_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_ARCH_SFENCE_PMP_TOR_RWX);
}

static inline void ai_arch_sfence_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_ARCH_SFENCE_PMP_TOR_RWX |
        (deny_cfg << 8) | (AI_ARCH_SFENCE_PMP_TOR_RWX << 16));
}

static inline void ai_arch_sfence_ordinary_fence(void)
{
    asm volatile("fence rw, rw" ::: "memory");
}

static inline uint64_t ai_arch_sfence_set_asid(uint64_t asid)
{
    uint64_t satp = CSRR(CSR_SATP);

    satp &= ~AI_ARCH_SFENCE_SATP_ASID_MASK;
    satp |= (asid << AI_ARCH_SFENCE_SATP_ASID_SHIFT) &
        AI_ARCH_SFENCE_SATP_ASID_MASK;
    CSRW(CSR_SATP, satp);
    return satp;
}

static inline uint64_t ai_arch_sfence_get_asid(void)
{
    return (CSRR(CSR_SATP) & AI_ARCH_SFENCE_SATP_ASID_MASK) >>
        AI_ARCH_SFENCE_SATP_ASID_SHIFT;
}

bool ai_arch_pte_perm_valid_sfence_update_matrix_corner()
{
    TEST_START();

    const enum test_page read_page = VSX_GX;
    const enum test_page write_page = VSR_GR;
    const enum test_page fetch_page = VSRW_GRW;
    const enum test_page valid_page = SWITCH1;
    const uintptr_t read_vaddr = hs_page_base(read_page) + 0x118U;
    const uintptr_t read_paddr = phys_page_base(read_page) + 0x118U;
    const uintptr_t write_vaddr = hs_page_base(write_page) + 0x128U;
    const uintptr_t write_paddr = phys_page_base(write_page) + 0x128U;
    const uintptr_t fetch_vaddr = hs_page_base(fetch_page) + 0x180U;
    const uintptr_t fetch_paddr = phys_page_base(fetch_page) + 0x180U;
    const uintptr_t valid_vaddr = hs_page_base(valid_page) + 0x208U;
    const uintptr_t valid_paddr = phys_page_base(valid_page) + 0x208U;
    const uint64_t read_value = 0x726561642d706572ULL;
    const uint64_t write_initial = 0x77726974652d3030ULL;
    const uint64_t write_open_payload = 0x77726974652d6f6bULL;
    const uint64_t write_tight_payload = 0x77726974652d6e6fULL;
    const uint64_t valid_value = 0x76616c69642d3030ULL;
    const uint64_t read_sentinel = 0xd00d00000000a101ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    write64(read_paddr, read_value);
    write64(write_paddr, write_initial);
    write32(fetch_paddr, 0x00008067U);
    write64(valid_paddr, valid_value);
    fence_iorw_iorw();
    ai_arch_sfence_fence_i();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(read_page, read_page,
        PTE_V | PTE_AD | PTE_X);
    ai_arch_sfence_set_leaf(write_page, write_page,
        PTE_V | PTE_AD | PTE_R);
    ai_arch_sfence_set_leaf(fetch_page, fetch_page,
        PTE_V | PTE_AD | PTE_RW);
    ai_arch_sfence_set_leaf(valid_page, valid_page,
        PTE_V | PTE_AD | PTE_R);
    const uint64_t read_xonly_leaf =
        *ai_arch_sfence_hs_leaf_entry(read_page);
    const uint64_t write_readonly_leaf =
        *ai_arch_sfence_hs_leaf_entry(write_page);
    const uint64_t fetch_noexec_leaf =
        *ai_arch_sfence_hs_leaf_entry(fetch_page);
    const uint64_t valid_leaf =
        *ai_arch_sfence_hs_leaf_entry(valid_page);
    sfence_vma();

    TEST_ASSERT("the PTE update matrix starts with explicit no-R, no-W, no-X, and valid readable leaves",
        (read_xonly_leaf & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (read_xonly_leaf & PTE_R) == 0 &&
        (write_readonly_leaf & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R) &&
        (write_readonly_leaf & PTE_W) == 0 &&
        (fetch_noexec_leaf & (PTE_V | PTE_AD | PTE_RW)) ==
            (PTE_V | PTE_AD | PTE_RW) &&
        (fetch_noexec_leaf & PTE_X) == 0 &&
        (valid_leaf & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R));

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    uint64_t read_fault_rd =
        ai_arch_sfence_ld_rd_probe(read_vaddr, read_sentinel);
    TEST_ASSERT("before opening PTE.R, HS load from the execute-only leaf reports LPF and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == read_vaddr &&
        read_fault_rd == read_sentinel);

    TEST_SETUP_EXCEPT();
    sd(write_vaddr, write_tight_payload);
    AI_ARCH_SFENCE_ASSERT_PAGE_FAULT("before opening PTE.W, HS store to the read-only leaf reports SPF",
        CAUSE_SPF, write_vaddr);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(fetch_vaddr);
    AI_ARCH_SFENCE_ASSERT_PAGE_FAULT("before opening PTE.X, HS fetch from the non-executable leaf reports IPF",
        CAUSE_IPF, fetch_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t valid_before = ld(valid_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the control valid leaf is readable before clearing PTE.V",
        valid_before == valid_value);

    goto_priv(PRIV_M);
    *ai_arch_sfence_hs_leaf_entry(read_page) = read_xonly_leaf | PTE_R;
    *ai_arch_sfence_hs_leaf_entry(write_page) =
        write_readonly_leaf | PTE_W;
    *ai_arch_sfence_hs_leaf_entry(fetch_page) =
        fetch_noexec_leaf | PTE_X;
    *ai_arch_sfence_hs_leaf_entry(valid_page) = valid_leaf & ~PTE_V;
    const uint64_t read_open_leaf =
        *ai_arch_sfence_hs_leaf_entry(read_page);
    const uint64_t write_open_leaf =
        *ai_arch_sfence_hs_leaf_entry(write_page);
    const uint64_t fetch_open_leaf =
        *ai_arch_sfence_hs_leaf_entry(fetch_page);
    const uint64_t invalid_leaf =
        *ai_arch_sfence_hs_leaf_entry(valid_page);
    fence_iorw_iorw();
    sfence_vma();
    ai_arch_sfence_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t read_open = ld(read_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after setting PTE.R and executing SFENCE.VMA, the same HS load succeeds",
        read_open == read_value);

    TEST_SETUP_EXCEPT();
    sd(write_vaddr, write_open_payload);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after setting PTE.W and executing SFENCE.VMA, the same HS store succeeds",
        true);

    TEST_SETUP_EXCEPT();
    ((void (*)(void))fetch_vaddr)();
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after setting PTE.X and executing SFENCE.VMA, the same HS fetch retires the installed ret",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t invalid_rd =
        ai_arch_sfence_ld_rd_probe(valid_vaddr, read_sentinel + 1ULL);
    TEST_ASSERT("after clearing PTE.V and executing SFENCE.VMA, the previously valid leaf reports LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == valid_vaddr &&
        invalid_rd == read_sentinel + 1ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("the opened permission phase updates only the writable backing word and keeps the expected PTE image",
        *ai_arch_sfence_hs_leaf_entry(read_page) == read_open_leaf &&
        *ai_arch_sfence_hs_leaf_entry(write_page) == write_open_leaf &&
        *ai_arch_sfence_hs_leaf_entry(fetch_page) == fetch_open_leaf &&
        *ai_arch_sfence_hs_leaf_entry(valid_page) == invalid_leaf &&
        read64(read_paddr) == read_value &&
        read64(write_paddr) == write_open_payload &&
        read32(fetch_paddr) == 0x00008067U &&
        read64(valid_paddr) == valid_value);

    *ai_arch_sfence_hs_leaf_entry(read_page) = read_open_leaf & ~PTE_R;
    *ai_arch_sfence_hs_leaf_entry(write_page) = write_open_leaf & ~PTE_W;
    *ai_arch_sfence_hs_leaf_entry(fetch_page) = fetch_open_leaf & ~PTE_X;
    *ai_arch_sfence_hs_leaf_entry(valid_page) = valid_leaf;
    const uint64_t read_tight_leaf =
        *ai_arch_sfence_hs_leaf_entry(read_page);
    const uint64_t write_tight_leaf =
        *ai_arch_sfence_hs_leaf_entry(write_page);
    const uint64_t fetch_tight_leaf =
        *ai_arch_sfence_hs_leaf_entry(fetch_page);
    const uint64_t valid_restored_leaf =
        *ai_arch_sfence_hs_leaf_entry(valid_page);
    fence_iorw_iorw();
    sfence_vma();
    ai_arch_sfence_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t read_tight_rd =
        ai_arch_sfence_ld_rd_probe(read_vaddr, read_sentinel + 2ULL);
    TEST_ASSERT("after clearing PTE.R and executing SFENCE.VMA, the old allowed load is reclassified as LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == read_vaddr &&
        read_tight_rd == read_sentinel + 2ULL);

    TEST_SETUP_EXCEPT();
    sd(write_vaddr, write_tight_payload);
    AI_ARCH_SFENCE_ASSERT_PAGE_FAULT("after clearing PTE.W and executing SFENCE.VMA, the old allowed store is reclassified as SPF",
        CAUSE_SPF, write_vaddr);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(fetch_vaddr);
    AI_ARCH_SFENCE_ASSERT_PAGE_FAULT("after clearing PTE.X and executing SFENCE.VMA, the old allowed fetch is reclassified as IPF",
        CAUSE_IPF, fetch_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t valid_after = ld(valid_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after restoring PTE.V and executing SFENCE.VMA, the same VA loads successfully again",
        valid_after == valid_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("permission tightening and V-bit restoration end with the expected leaves and no forbidden store side effects",
        *ai_arch_sfence_hs_leaf_entry(read_page) == read_tight_leaf &&
        *ai_arch_sfence_hs_leaf_entry(write_page) == write_tight_leaf &&
        *ai_arch_sfence_hs_leaf_entry(fetch_page) == fetch_tight_leaf &&
        *ai_arch_sfence_hs_leaf_entry(valid_page) == valid_restored_leaf &&
        read64(read_paddr) == read_value &&
        read64(write_paddr) == write_open_payload &&
        read32(fetch_paddr) == 0x00008067U &&
        read64(valid_paddr) == valid_value);

    TEST_END();
}

bool ai_arch_pte_ppn_leaflevel_sfence_rewalk_corner()
{
    TEST_START();

    const enum test_page va_page = SWITCH1;
    const enum test_page old_page = SWITCH1;
    const enum test_page new_page = SWITCH2;
    const uintptr_t remap_vaddr = hs_page_base(va_page) + 0x240U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x240U;
    const uintptr_t new_paddr = phys_page_base(new_page) + 0x240U;
    const uint64_t old_value = 0x70706e2d6f6c6431ULL;
    const uint64_t new_value = 0x70706e2d6e657732ULL;
    const uint64_t new_store = 0x70706e2d77726974ULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uintptr_t switch_vaddr_base = 0x00400000ULL;
    const uintptr_t switch_offset = 0x3200ULL;
    const uintptr_t switch_vaddr = switch_vaddr_base + switch_offset;
    const uintptr_t super_paddr_base = TEST_PPAGE_BASE;
    const uintptr_t super_paddr = super_paddr_base + switch_offset;
    const uintptr_t small_paddr_base = phys_page_base(SCRATCHPAD);
    const uintptr_t small_paddr =
        small_paddr_base + (switch_offset & (PAGE_SIZE - 1ULL));
    const uint64_t super_value = 0x73757065722d3270ULL;
    const uint64_t small_value = 0x736d616c6c2d346bULL;
    const uint64_t small_store = 0x736d616c6c2d7772ULL;
    const uint64_t sv39_satp =
        (AI_ARCH_SFENCE_SATP_MODE_SV39 << 60) |
        (((uintptr_t)ai_arch_sfence_sv39_root) >> 12);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(old_paddr, old_value);
    write64(new_paddr, new_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(va_page, old_page, leaf_flags);
    const uint64_t old_leaf = *ai_arch_sfence_hs_leaf_entry(va_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t remap_old = ld(remap_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the baseline VA first walks the old PPN mapping",
        remap_old == old_value);

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(va_page, new_page, leaf_flags);
    const uint64_t new_leaf = *ai_arch_sfence_hs_leaf_entry(va_page);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t remap_new = ld(remap_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after updating the leaf PPN and executing SFENCE.VMA, the same VA walks the new PPN",
        remap_new == new_value);

    TEST_SETUP_EXCEPT();
    sd(remap_vaddr, new_store);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("a store after the PPN remap updates only the new physical page",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("the PPN remap preserves the old physical page and leaves the remapped leaf installed",
        old_leaf != new_leaf &&
        *ai_arch_sfence_hs_leaf_entry(va_page) == new_leaf &&
        read64(old_paddr) == old_value &&
        read64(new_paddr) == new_store);

    write64(super_paddr, super_value);
    write64(small_paddr, small_value);
    ai_arch_sfence_zero_pt(ai_arch_sfence_sv39_root);
    ai_arch_sfence_zero_pt(ai_arch_sfence_sv39_l1);
    ai_arch_sfence_zero_pt(ai_arch_sfence_sv39_l0);
    ai_arch_sfence_sv39_root[
        AI_ARCH_SFENCE_SV39_L2_INDEX(switch_vaddr_base)] =
            PTE_V | (((uintptr_t)ai_arch_sfence_sv39_l1) >> 2);
    ai_arch_sfence_sv39_root[AI_ARCH_SFENCE_SV39_L2_INDEX(MEM_BASE)] =
        (MEM_BASE >> 2) | leaf_flags;
    ai_arch_sfence_sv39_l1[
        AI_ARCH_SFENCE_SV39_L1_INDEX(switch_vaddr_base)] =
            (super_paddr_base >> 2) | leaf_flags;
    ai_arch_sfence_sv39_l0[
        AI_ARCH_SFENCE_SV39_L0_INDEX(switch_vaddr)] =
            (small_paddr_base >> 2) | leaf_flags;
    const uint64_t l1_super_leaf = ai_arch_sfence_sv39_l1[
        AI_ARCH_SFENCE_SV39_L1_INDEX(switch_vaddr_base)];
    const uint64_t l0_small_leaf = ai_arch_sfence_sv39_l0[
        AI_ARCH_SFENCE_SV39_L0_INDEX(switch_vaddr)];
    fence_iorw_iorw();
    CSRW(CSR_SATP, sv39_satp);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t super_observed = ld(switch_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the Sv39 L1 leaf first maps the VA through the 2MB superpage PPN",
        super_observed == super_value);

    goto_priv(PRIV_M);
    ai_arch_sfence_sv39_l1[
        AI_ARCH_SFENCE_SV39_L1_INDEX(switch_vaddr_base)] =
            PTE_V | (((uintptr_t)ai_arch_sfence_sv39_l0) >> 2);
    const uint64_t l1_nonleaf = ai_arch_sfence_sv39_l1[
        AI_ARCH_SFENCE_SV39_L1_INDEX(switch_vaddr_base)];
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t small_observed = ld(switch_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after switching the L1 leaf to a non-leaf and executing SFENCE.VMA, the same VA walks the 4KB leaf",
        small_observed == small_value);

    TEST_SETUP_EXCEPT();
    sd(switch_vaddr, small_store);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("a store after the leaf-to-nonleaf switch updates the 4KB target page",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("the leaf-to-nonleaf switch leaves the old superpage backing word isolated from the 4KB store",
        ai_arch_sfence_sv39_l1[
            AI_ARCH_SFENCE_SV39_L1_INDEX(switch_vaddr_base)] ==
            l1_nonleaf &&
        ai_arch_sfence_sv39_l0[
            AI_ARCH_SFENCE_SV39_L0_INDEX(switch_vaddr)] ==
            l0_small_leaf &&
        read64(super_paddr) == super_value &&
        read64(small_paddr) == small_store);

    ai_arch_sfence_sv39_l1[
        AI_ARCH_SFENCE_SV39_L1_INDEX(switch_vaddr_base)] = l1_super_leaf;
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t super_again = ld(switch_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after restoring the L1 superpage leaf and executing SFENCE.VMA, the same VA returns to the 2MB mapping",
        super_again == super_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("the final superpage restore preserves both page-table shapes and both backing images",
        CSRR(CSR_SATP) == sv39_satp &&
        ai_arch_sfence_sv39_l1[
            AI_ARCH_SFENCE_SV39_L1_INDEX(switch_vaddr_base)] ==
            l1_super_leaf &&
        read64(super_paddr) == super_value &&
        read64(small_paddr) == small_store);

    TEST_END();
}

bool ai_arch_pte_ad_sfence_update_rejudge_corner()
{
    TEST_START();

    const enum test_page load_page = VSR_GR;
    const enum test_page store_page = VSRW_GRW;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x140U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x140U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x188U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x188U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x1c0U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x1c0U;
    const uint64_t load_value = 0x41442d6c6f616431ULL;
    const uint64_t store_initial = 0x41442d73746f7231ULL;
    const uint64_t store_payload = 0x41442d73746f7232ULL;
    const uint64_t denied_payload = 0x41442d64656e7933ULL;
    const uint64_t guard_value = 0x41442d6775617264ULL;
    const uint64_t load_sentinel = 0xadad00000000a101ULL;
    const uint64_t a0_load_leaf = PTE_V | PTE_D | PTE_R;
    const uint64_t d0_store_leaf = PTE_V | PTE_A | PTE_RW;
    const uint64_t legal_load_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_store_leaf = PTE_V | PTE_AD | PTE_RW;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(load_paddr, load_value);
    write64(store_paddr, store_initial);
    write64(guard_paddr, guard_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(load_page, load_page, a0_load_leaf);
    ai_arch_sfence_set_leaf(store_page, store_page, d0_store_leaf);
    ai_arch_sfence_set_leaf(guard_page, guard_page,
        PTE_V | PTE_AD | PTE_RWX);
    const uint64_t load_a0_leaf =
        *ai_arch_sfence_hs_leaf_entry(load_page);
    const uint64_t store_d0_leaf =
        *ai_arch_sfence_hs_leaf_entry(store_page);
    const uint64_t guard_leaf =
        *ai_arch_sfence_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();

    TEST_ASSERT("the A/D update probe starts with an A=0 readable leaf, a D=0 writable leaf, and a legal guard leaf",
        (load_a0_leaf & (PTE_V | PTE_D | PTE_R)) ==
            (PTE_V | PTE_D | PTE_R) &&
        (load_a0_leaf & PTE_A) == 0 &&
        (store_d0_leaf & (PTE_V | PTE_A | PTE_RW)) ==
            (PTE_V | PTE_A | PTE_RW) &&
        (store_d0_leaf & PTE_D) == 0 &&
        (guard_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_a0_rd =
        ai_arch_sfence_ld_rd_probe(load_vaddr, load_sentinel);
    TEST_ASSERT("with PTE.A=0, the HS load reports LPF and does not update rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        load_a0_rd == load_sentinel);

    TEST_SETUP_EXCEPT();
    uint64_t d0_load = ld(store_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("with PTE.D=0 but read permission present, an HS load from the store page still succeeds",
        d0_load == store_initial);

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, denied_payload);
    AI_ARCH_SFENCE_ASSERT_PAGE_FAULT("with PTE.D=0, the HS store reports SPF before updating memory",
        CAUSE_SPF, store_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("A=0 and D=0 faults leave the PTE words and backing memory unchanged",
        *ai_arch_sfence_hs_leaf_entry(load_page) == load_a0_leaf &&
        *ai_arch_sfence_hs_leaf_entry(store_page) == store_d0_leaf &&
        *ai_arch_sfence_hs_leaf_entry(guard_page) == guard_leaf &&
        read64(load_paddr) == load_value &&
        read64(store_paddr) == store_initial &&
        read64(guard_paddr) == guard_value);

    ai_arch_sfence_set_leaf(load_page, load_page, legal_load_leaf);
    ai_arch_sfence_set_leaf(store_page, store_page, legal_store_leaf);
    const uint64_t load_a1_leaf =
        *ai_arch_sfence_hs_leaf_entry(load_page);
    const uint64_t store_d1_leaf =
        *ai_arch_sfence_hs_leaf_entry(store_page);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_a1 = ld(load_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after setting PTE.A and executing SFENCE.VMA, the same HS load succeeds",
        load_a1 == load_value);

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_payload);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after setting PTE.D and executing SFENCE.VMA, the same HS store succeeds",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t guard_after_repair = ld(guard_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the independent guard leaf remains readable after setting A/D bits and fencing",
        guard_after_repair == guard_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("setting A/D bits restores access and updates only the store target",
        *ai_arch_sfence_hs_leaf_entry(load_page) == load_a1_leaf &&
        *ai_arch_sfence_hs_leaf_entry(store_page) == store_d1_leaf &&
        *ai_arch_sfence_hs_leaf_entry(guard_page) == guard_leaf &&
        read64(load_paddr) == load_value &&
        read64(store_paddr) == store_payload &&
        read64(guard_paddr) == guard_value);

    ai_arch_sfence_set_leaf(load_page, load_page, a0_load_leaf);
    ai_arch_sfence_set_leaf(store_page, store_page, d0_store_leaf);
    const uint64_t load_a0_again_leaf =
        *ai_arch_sfence_hs_leaf_entry(load_page);
    const uint64_t store_d0_again_leaf =
        *ai_arch_sfence_hs_leaf_entry(store_page);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_a0_again_rd =
        ai_arch_sfence_ld_rd_probe(load_vaddr, load_sentinel + 1ULL);
    TEST_ASSERT("after clearing PTE.A again and executing SFENCE.VMA, the old allowed load is reclassified as LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        load_a0_again_rd == load_sentinel + 1ULL);

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, denied_payload);
    AI_ARCH_SFENCE_ASSERT_PAGE_FAULT("after clearing PTE.D again and executing SFENCE.VMA, the old allowed store is reclassified as SPF",
        CAUSE_SPF, store_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing A/D again restores the faulting state without corrupting repaired data or guard state",
        *ai_arch_sfence_hs_leaf_entry(load_page) == load_a0_again_leaf &&
        *ai_arch_sfence_hs_leaf_entry(store_page) == store_d0_again_leaf &&
        *ai_arch_sfence_hs_leaf_entry(guard_page) == guard_leaf &&
        read64(load_paddr) == load_value &&
        read64(store_paddr) == store_payload &&
        read64(guard_paddr) == guard_value);

    TEST_END();
}

bool ai_arch_pte_ppn_remap_fault_metadata_rebind_corner()
{
    TEST_START();

    const enum test_page va_page = SWITCH1;
    const enum test_page old_page = SWITCH1;
    const enum test_page pf_page = SWITCH2;
    const enum test_page af_page = SCRATCHPAD;
    const enum test_page guard_page = VSRW_GRW;
    const uintptr_t vaddr = hs_page_base(va_page) + 0x2c8U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x2c8U;
    const uintptr_t pf_paddr = phys_page_base(pf_page) + 0x2c8U;
    const uintptr_t af_paddr = phys_page_base(af_page) + 0x2c8U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x2c8U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x2c8U;
    const uint64_t old_value = 0x70706e2d6f6c6421ULL;
    const uint64_t pf_value = 0x70706e2d70662121ULL;
    const uint64_t af_value = 0x70706e2d61662121ULL;
    const uint64_t guard_value = 0x70706e2d67756172ULL;
    const uint64_t af_payload = 0x70706e2d61667772ULL;
    const uint64_t load_sentinel = 0xafaf00000000a101ULL;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(old_paddr, old_value);
    write64(pf_paddr, pf_value);
    write64(af_paddr, af_value);
    write64(guard_paddr, guard_value);
    ai_arch_sfence_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(va_page, old_page, legal_leaf);
    ai_arch_sfence_set_leaf(guard_page, guard_page, legal_leaf);
    const uint64_t old_leaf = *ai_arch_sfence_hs_leaf_entry(va_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_load = ld(vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the remap metadata probe first walks the old legal PPN",
        old_load == old_value);

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(va_page, pf_page, PTE_V | PTE_AD | PTE_X);
    const uint64_t pf_leaf = *ai_arch_sfence_hs_leaf_entry(va_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the page-fault phase remaps the same VA to a new PPN with read permission removed",
        old_leaf != pf_leaf &&
        ((pf_leaf << 2) & PAGE_ADDR_MSK) ==
            (phys_page_base(pf_page) & PAGE_ADDR_MSK) &&
        (pf_leaf & PTE_R) == 0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t pf_rd = ai_arch_sfence_ld_rd_probe(vaddr, load_sentinel);
    TEST_ASSERT("after PPN remap and SFENCE.VMA, the new no-read leaf regenerates LPF metadata for the same VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        pf_rd == load_sentinel);

    goto_priv(PRIV_M);
    TEST_ASSERT("the remapped page-fault access leaves old, new, and guard backing images unchanged",
        *ai_arch_sfence_hs_leaf_entry(va_page) == pf_leaf &&
        read64(old_paddr) == old_value &&
        read64(pf_paddr) == pf_value &&
        read64(guard_paddr) == guard_value);

    ai_arch_sfence_set_leaf(va_page, af_page, legal_leaf);
    const uint64_t af_leaf = *ai_arch_sfence_hs_leaf_entry(va_page);
    ai_arch_sfence_configure_pmp_window(phys_page_base(af_page),
        phys_page_base(af_page) + PAGE_SIZE, AI_ARCH_SFENCE_PMP_TOR_NO_R);
    sfence_vma();
    TEST_ASSERT("the access-fault phase remaps the same VA to another legal PTE whose translated PA is PMP denied",
        af_leaf != pf_leaf &&
        ((af_leaf << 2) & PAGE_ADDR_MSK) ==
            (phys_page_base(af_page) & PAGE_ADDR_MSK));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t af_rd =
        ai_arch_sfence_ld_rd_probe(vaddr, load_sentinel + 1ULL);
    TEST_ASSERT("after remapping to the PMP-denied PA and executing SFENCE.VMA, the same VA regenerates load access-fault metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr &&
        af_rd == load_sentinel + 1ULL);

    goto_priv(PRIV_M);
    ai_arch_sfence_configure_pmp_window(phys_page_base(af_page),
        phys_page_base(af_page) + PAGE_SIZE, AI_ARCH_SFENCE_PMP_TOR_NO_W);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, af_payload);
    TEST_ASSERT("changing the translated PA denial from read to write regenerates store access-fault metadata for the remapped VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr);

    goto_priv(PRIV_M);
    ai_arch_sfence_restore_pmp_full_access();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovered_load = ld(vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after restoring PMP access and fencing, the same remapped VA loads from the access-fault PA",
        recovered_load == af_value);

    TEST_SETUP_EXCEPT();
    sd(vaddr, af_payload);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after access-fault repair, the same remapped VA stores only to the final PA",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t guard_load = ld(guard_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("an unrelated guard mapping remains readable after PF-to-AF metadata rebinding",
        guard_load == guard_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("PPN remap metadata rebinding isolates old, PF, AF, and guard physical images",
        *ai_arch_sfence_hs_leaf_entry(va_page) == af_leaf &&
        read64(old_paddr) == old_value &&
        read64(pf_paddr) == pf_value &&
        read64(af_paddr) == af_payload &&
        read64(guard_paddr) == guard_value);

    TEST_END();
}

bool ai_arch_sfence_vma_data_fence_separation_corner()
{
    TEST_START();

    const enum test_page data_page = VSRW_GRW;
    const enum test_page alt_page = SWITCH2;
    const uintptr_t data_vaddr = hs_page_base(data_page) + 0x348U;
    const uintptr_t alias_vaddr = hs_page_base(alt_page) + 0x348U;
    const uintptr_t data_paddr = phys_page_base(data_page) + 0x348U;
    const uintptr_t alt_paddr = phys_page_base(alt_page) + 0x348U;
    const uint64_t seed = 0x7366656e63652d30ULL;
    const uint64_t before_sfence = 0x7366656e63652d31ULL;
    const uint64_t after_sfence = 0x7366656e63652d32ULL;
    const uint64_t alias_payload = 0x7366656e63652d33ULL;
    const uint64_t final_payload = 0x7366656e63652d34ULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(data_paddr, seed);
    write64(alt_paddr, alias_payload);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(data_page, data_page, leaf_flags);
    ai_arch_sfence_set_leaf(alt_page, alt_page, leaf_flags);
    const uint64_t original_leaf = *ai_arch_sfence_hs_leaf_entry(data_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(data_vaddr, before_sfence);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the ordinary data store before SFENCE.VMA retires on the original mapping",
        true);

    TEST_SETUP_EXCEPT();
    sfence_vma();
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("SFENCE.VMA itself retires but is not used as the ordinary data-ordering oracle",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t same_addr_after_sfence = ld(data_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after Store -> SFENCE.VMA -> same-address Load, only normal same-hart visibility is checked",
        same_addr_after_sfence == before_sfence);

    TEST_SETUP_EXCEPT();
    sd(data_vaddr, after_sfence);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("a younger data store after SFENCE.VMA still commits through the same translation",
        true);

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(data_page, alt_page, leaf_flags);
    const uint64_t remapped_leaf = *ai_arch_sfence_hs_leaf_entry(data_page);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t remapped_load = ld(data_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the translation effect of SFENCE.VMA remains limited to rewalking the updated PTE",
        remapped_load == alias_payload);

    TEST_SETUP_EXCEPT();
    sd(data_vaddr, final_payload);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("a store after the translation update writes the newly mapped physical page",
        true);

    TEST_SETUP_EXCEPT();
    ai_arch_sfence_ordinary_fence();
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("an explicit ordinary FENCE is a separate data-ordering primitive after the SFENCE.VMA translation phase",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t alias_after_fence = ld(alias_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after the explicit ordinary FENCE, the data visibility check observes the final payload through the alias",
        alias_after_fence == final_payload);

    goto_priv(PRIV_M);
    TEST_ASSERT("SFENCE.VMA and ordinary FENCE separation leaves the original and remapped physical images distinct",
        original_leaf != remapped_leaf &&
        *ai_arch_sfence_hs_leaf_entry(data_page) == remapped_leaf &&
        read64(data_paddr) == after_sfence &&
        read64(alt_paddr) == final_payload);

    TEST_END();
}

bool ai_arch_sfence_vma_faulting_pte_store_keeps_old_translation_corner()
{
    TEST_START();

    const enum test_page va_page = SWITCH1;
    const enum test_page old_page = SWITCH1;
    const enum test_page new_page = SWITCH2;
    const enum test_page pte_alias_page = VSR_GR;
    const uintptr_t vaddr = hs_page_base(va_page) + 0x3c8U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x3c8U;
    const uintptr_t new_paddr = phys_page_base(new_page) + 0x3c8U;
    const uint64_t old_value = 0x7074652d6f6c6431ULL;
    const uint64_t new_value = 0x7074652d6e657732ULL;
    const uint64_t new_store = 0x7074652d77726974ULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t pte_read_sentinel = 0x5f5f00000000a101ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(old_paddr, old_value);
    write64(new_paddr, new_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    pte_t *target_leaf = ai_arch_sfence_hs_leaf_entry(va_page);
    pte_t *alias_leaf = ai_arch_sfence_hs_leaf_entry(pte_alias_page);
    const uint64_t original_alias_leaf = *alias_leaf;
    ai_arch_sfence_set_leaf(va_page, old_page, leaf_flags);
    const uint64_t old_leaf = *target_leaf;
    const uint64_t new_leaf =
        (phys_page_base(new_page) >> 2) | leaf_flags;
    const uintptr_t target_leaf_addr = (uintptr_t)target_leaf;
    const uintptr_t target_leaf_page = target_leaf_addr & PAGE_ADDR_MSK;
    const uintptr_t pte_alias_vaddr =
        hs_page_base(pte_alias_page) +
        (target_leaf_addr & (PAGE_SIZE - 1ULL));
    *alias_leaf = (target_leaf_page >> 2) |
        PTE_V | PTE_AD | PTE_R;
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the faulting PTE-store probe starts with an old VA->PA leaf and a read-only alias to that PTE word",
        old_leaf != new_leaf &&
        ((old_leaf << 2) & PAGE_ADDR_MSK) ==
            (phys_page_base(old_page) & PAGE_ADDR_MSK) &&
        ((new_leaf << 2) & PAGE_ADDR_MSK) ==
            (phys_page_base(new_page) & PAGE_ADDR_MSK) &&
        (*alias_leaf & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R) &&
        (*alias_leaf & PTE_W) == 0);

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    uint64_t old_load = ld(vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("before the faulting PTE store, the target VA reads through the old translation",
        old_load == old_value);

    TEST_SETUP_EXCEPT();
    uint64_t alias_read =
        ai_arch_sfence_ld_rd_probe(pte_alias_vaddr, pte_read_sentinel);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the read-only PTE alias exposes the old leaf value before the denied store",
        alias_read == old_leaf);

    TEST_SETUP_EXCEPT();
    sd(pte_alias_vaddr, new_leaf);
    AI_ARCH_SFENCE_ASSERT_PAGE_FAULT("the attempted page-table store through the read-only alias reports SPF on the alias VA",
        CAUSE_SPF, pte_alias_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting page-table store does not commit the candidate PTE image or touch either data page",
        *target_leaf == old_leaf &&
        read64(old_paddr) == old_value &&
        read64(new_paddr) == new_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sfence_vma();
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("SFENCE.VMA after the faulting PTE store retires without creating a translation side effect",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t old_after_sfence = ld(vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after the faulting PTE store and SFENCE.VMA, the target VA still resolves to the old physical page",
        old_after_sfence == old_value);

    goto_priv(PRIV_M);
    *target_leaf = new_leaf;
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t new_after_commit = ld(vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after committing the same PTE update in M-mode and executing SFENCE.VMA, the target VA resolves to the new physical page",
        new_after_commit == new_value);

    TEST_SETUP_EXCEPT();
    sd(vaddr, new_store);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("a store after the committed PTE update writes only the new physical page",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("only the committed PTE update changes translation; the old physical image is not rewritten by the earlier faulting store",
        *target_leaf == new_leaf &&
        read64(old_paddr) == old_value &&
        read64(new_paddr) == new_store);

    *target_leaf = old_leaf;
    *alias_leaf = original_alias_leaf;
    fence_iorw_iorw();
    sfence_vma();

    TEST_END();
}

bool ai_arch_sfence_vma_same_ppn_alias_asid_refresh_corner()
{
    TEST_START();

    const enum test_page alias0_page = SWITCH1;
    const enum test_page alias1_page = SWITCH2;
    const enum test_page guard_page = VSRW_GRW;
    const enum test_page shared_phys_page = VSRWX_GRWX;
    const enum test_page new_phys_page = SCRATCHPAD;
    const uintptr_t alias0_vaddr = hs_page_base(alias0_page) + 0x448U;
    const uintptr_t alias1_vaddr = hs_page_base(alias1_page) + 0x448U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x448U;
    const uintptr_t shared_paddr = phys_page_base(shared_phys_page) + 0x448U;
    const uintptr_t new_paddr = phys_page_base(new_phys_page) + 0x448U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x448U;
    const uint64_t shared_value = 0x73616d652d70706eULL;
    const uint64_t new_value = 0x73666e632d6e6577ULL;
    const uint64_t new_store = 0x73666e632d777269ULL;
    const uint64_t guard_value = 0x73666e632d677264ULL;
    const uint64_t load_sentinel = 0x5e5e00000000a101ULL;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t no_read_leaf = PTE_V | PTE_AD | PTE_X;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(shared_paddr, shared_value);
    write64(new_paddr, new_value);
    write64(guard_paddr, guard_value);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(alias0_page, shared_phys_page, legal_leaf);
    ai_arch_sfence_set_leaf(alias1_page, shared_phys_page, legal_leaf);
    ai_arch_sfence_set_leaf(guard_page, guard_page, legal_leaf);
    ai_arch_sfence_set_asid(AI_ARCH_SFENCE_ASID_A);
    const uint64_t alias0_initial_leaf =
        *ai_arch_sfence_hs_leaf_entry(alias0_page);
    const uint64_t alias1_initial_leaf =
        *ai_arch_sfence_hs_leaf_entry(alias1_page);
    const uint64_t guard_initial_leaf =
        *ai_arch_sfence_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("same-PPN alias setup starts from two non-global VAs mapped to one physical page under a known ASID",
        ai_arch_sfence_get_asid() == AI_ARCH_SFENCE_ASID_A &&
        alias0_initial_leaf == alias1_initial_leaf &&
        (alias0_initial_leaf & PTE_G) == 0 &&
        ((alias0_initial_leaf << 2) & PAGE_ADDR_MSK) ==
            phys_page_base(shared_phys_page) &&
        ((guard_initial_leaf << 2) & PAGE_ADDR_MSK) ==
            phys_page_base(guard_page)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t alias0_before = ld(alias0_vaddr);
    uint64_t alias1_before = ld(alias1_vaddr);
    uint64_t guard_before = ld(guard_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("both same-PPN aliases and the guard VA are warmed under the same ASID before the targeted flush",
        alias0_before == shared_value &&
        alias1_before == shared_value &&
        guard_before == guard_value);

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(alias0_page, new_phys_page, no_read_leaf);
    const uint64_t alias0_no_read_leaf =
        *ai_arch_sfence_hs_leaf_entry(alias0_page);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sfence_vma_rs(alias0_vaddr, AI_ARCH_SFENCE_ASID_A);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("targeted sfence.vma va,asid retires after alias0 is remapped to a no-read PTE",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t alias0_fault_rd =
        ai_arch_sfence_ld_rd_probe(alias0_vaddr, load_sentinel);
    TEST_ASSERT("after targeted sfence.vma va,asid, alias0 rewalks the updated no-read leaf and reports LPF on that VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == alias0_vaddr &&
        alias0_fault_rd == load_sentinel);

    TEST_SETUP_EXCEPT();
    uint64_t alias1_after_alias0_flush = ld(alias1_vaddr);
    uint64_t guard_after_alias0_flush = ld(guard_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the same-PPN alias that was not remapped and the unrelated guard VA remain readable after alias0-only flush",
        alias1_after_alias0_flush == shared_value &&
        guard_after_alias0_flush == guard_value);

    goto_priv(PRIV_M);
    ai_arch_sfence_set_leaf(alias0_page, new_phys_page, legal_leaf);
    const uint64_t alias0_legal_new_leaf =
        *ai_arch_sfence_hs_leaf_entry(alias0_page);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sfence_vma_rs(alias0_vaddr, AI_ARCH_SFENCE_ASID_A);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("a second targeted sfence.vma va,asid revalidates only the repaired alias0 mapping",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t alias0_repaired = ld(alias0_vaddr);
    uint64_t alias1_still_shared = ld(alias1_vaddr);
    uint64_t guard_still = ld(guard_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("after repair, alias0 reads the new physical page while alias1 still reads the original shared PPN",
        alias0_repaired == new_value &&
        alias1_still_shared == shared_value &&
        guard_still == guard_value);

    TEST_SETUP_EXCEPT();
    sd(alias0_vaddr, new_store);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("a store through the repaired alias0 writes only the new physical page",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t alias1_final = ld(alias1_vaddr);
    AI_ARCH_SFENCE_ASSERT_NO_EXCEPT_AND("the old same-PPN alias still observes the original physical page after alias0 store",
        alias1_final == shared_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("targeted sfence.vma same-PPN alias refresh isolates PTE words and physical images",
        *ai_arch_sfence_hs_leaf_entry(alias0_page) == alias0_legal_new_leaf &&
        *ai_arch_sfence_hs_leaf_entry(alias1_page) == alias1_initial_leaf &&
        *ai_arch_sfence_hs_leaf_entry(guard_page) == guard_initial_leaf &&
        alias0_no_read_leaf != alias0_initial_leaf &&
        alias0_legal_new_leaf != alias1_initial_leaf &&
        read64(shared_paddr) == shared_value &&
        read64(new_paddr) == new_store &&
        read64(guard_paddr) == guard_value
    );

    TEST_END();
}
