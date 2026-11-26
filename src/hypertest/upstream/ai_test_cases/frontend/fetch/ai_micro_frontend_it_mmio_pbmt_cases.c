#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_FRONTEND_IT_PMP_TOR_RWX  0x0fULL
#define AI_FRONTEND_IT_PMP_TOR_NO_X 0x0bULL
#define AI_FRONTEND_IT_PMP_TOR_RWX_L 0x8fULL

#define AI_FRONTEND_IT_PMA_R      (1ULL << 0)
#define AI_FRONTEND_IT_PMA_W      (1ULL << 1)
#define AI_FRONTEND_IT_PMA_X      (1ULL << 2)
#define AI_FRONTEND_IT_PMA_A_TOR  (1ULL << 3)
#define AI_FRONTEND_IT_PMA_ATOMIC (1ULL << 5)
#define AI_FRONTEND_IT_PMA_C      (1ULL << 6)
#define AI_FRONTEND_IT_PMA_L      (1ULL << 7)

#define AI_FRONTEND_IT_MMIO_OK \
    (AI_FRONTEND_IT_PMA_R | AI_FRONTEND_IT_PMA_W | \
     AI_FRONTEND_IT_PMA_X | AI_FRONTEND_IT_PMA_A_TOR)
#define AI_FRONTEND_IT_MMIO_NO_X \
    (AI_FRONTEND_IT_PMA_R | AI_FRONTEND_IT_PMA_W | \
     AI_FRONTEND_IT_PMA_A_TOR)
#define AI_FRONTEND_IT_MEM_OK \
    (AI_FRONTEND_IT_PMA_R | AI_FRONTEND_IT_PMA_W | \
     AI_FRONTEND_IT_PMA_X | AI_FRONTEND_IT_PMA_A_TOR | \
     AI_FRONTEND_IT_PMA_ATOMIC | AI_FRONTEND_IT_PMA_C)

#define AI_FRONTEND_IT_ASSERT_NO_FAULT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_FRONTEND_IT_VMODULE_REG_AXI_ERROR 0x1060ULL

static inline pte_t *ai_frontend_it_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uint64_t ai_frontend_it_pma_cfg_entry(unsigned entry,
    uint64_t bits)
{
    return bits << (entry * 8U);
}

static inline void ai_frontend_it_restore_pma_default(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFFULL);
    CSRW(CSR_PMACFG0, 0x6FULL);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000ULL);
    CSRW(CSR_PMAADDR12, 0x20000000ULL);
    CSRW(CSR_PMAADDR13, 0x120000000ULL);
    CSRW(CSR_PMAADDR14, 0xC000000000ULL);
    CSRW(CSR_PMAADDR15, 0x10000000000ULL);
    fence_iorw_iorw();
}

static inline void ai_frontend_it_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_FRONTEND_IT_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    fence_iorw_iorw();
}

static inline void ai_frontend_it_configure_pmp_window(uintptr_t start,
    uintptr_t end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, start >> 2);
    CSRW(CSR_PMPADDR1, end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_FRONTEND_IT_PMP_TOR_RWX |
        (deny_cfg << 8U) | (AI_FRONTEND_IT_PMP_TOR_RWX << 16U));
    fence_iorw_iorw();
}

static inline void ai_frontend_it_configure_pmp_locked_allow_window(
    uintptr_t start, uintptr_t end)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, start >> 2);
    CSRW(CSR_PMPADDR1, end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_FRONTEND_IT_PMP_TOR_RWX |
        (AI_FRONTEND_IT_PMP_TOR_RWX_L << 8U) |
        (AI_FRONTEND_IT_PMP_TOR_RWX << 16U));
    fence_iorw_iorw();
}

static inline void ai_frontend_it_configure_pma_tor1(uintptr_t start,
    uintptr_t end, uint64_t entry_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMACFG0, 0ULL);
    CSRW(CSR_PMAADDR0, start >> 2);
    CSRW(CSR_PMAADDR1, end >> 2);
    CSRW(CSR_PMACFG0, ai_frontend_it_pma_cfg_entry(1, entry_bits));
    fence_iorw_iorw();
}

static inline void ai_frontend_it_configure_pma_two_pages(uintptr_t first,
    uintptr_t second, uint64_t first_bits, uint64_t second_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMACFG0, 0ULL);
    CSRW(CSR_PMAADDR0, first >> 2);
    CSRW(CSR_PMAADDR1, (first + PAGE_SIZE) >> 2);
    CSRW(CSR_PMAADDR2, (second + PAGE_SIZE) >> 2);
    CSRW(CSR_PMACFG0,
        ai_frontend_it_pma_cfg_entry(1, first_bits) |
        ai_frontend_it_pma_cfg_entry(2, second_bits));
    fence_iorw_iorw();
}

static inline void ai_frontend_it_set_hs_leaf(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    *ai_frontend_it_hs_leaf_entry(page) =
        ((paddr & PAGE_ADDR_MSK) >> 2) | flags;
}

static inline void ai_frontend_it_write32_bytes(uintptr_t paddr, uint32_t value)
{
    for (unsigned i = 0; i < sizeof(value); ++i) {
        write8(paddr + i, (uint8_t)(value >> (8U * i)));
    }
}

static inline void ai_frontend_it_write_probe(uintptr_t paddr, uint64_t imm)
{
    const uint32_t addi_a0 = ((uint32_t)(imm & 0xfffULL) << 20) |
        (10U << 7) | 0x13U;
    const uint32_t jr_t6 = 0x000f8067U;

    ai_frontend_it_write32_bytes(paddr, addi_a0);
    ai_frontend_it_write32_bytes(paddr + sizeof(uint32_t), jr_t6);
}

static inline void ai_frontend_it_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}

static inline void ai_frontend_it_vmodule_write_reg(uint64_t offset,
    uint64_t data)
{
    const uint64_t addr_cmd = 0xffff000100000000ULL |
        (offset & 0xffffffffULL);
    const uint64_t data_cmd = 0xffff000000000000ULL |
        (data & 0xffffffffULL);

    asm volatile(
        "mv t0, %[addr]\n\t"
        "mv t1, %[data]\n\t"
        "fence.i\n\t"
        "add x0, x0, t0\n\t"
        "fence.i\n\t"
        "add x0, x0, t1\n\t"
        "fence.i\n\t"
        :
        : [addr] "r"(addr_cmd),
          [data] "r"(data_cmd)
        : "t0", "t1", "memory");
}

static inline uint64_t ai_frontend_it_exec_probe(uintptr_t addr)
{
    uint64_t observed;

    asm volatile(
        "la t0, 1f\n\t"
        "sd t0, %1\n\t"
        "mv t6, t0\n\t"
        "jr %2\n\t"
        "1:\n\t"
        "mv %0, a0\n\t"
        : "=r"(observed), "=m"(excpt.fault_inst)
        : "r"(addr)
        : "t0", "t6", "a0", "memory");

    return observed;
}

static inline void ai_frontend_it_prepare_hs_pbmt_nc(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_frontend_it_set_hs_leaf(page, paddr, flags | PTE_Pbmt1);
    sfence_vma();
    ai_frontend_it_fence_i();
}

static inline void ai_frontend_it_cleanup(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    ai_frontend_it_restore_pma_default();
    ai_frontend_it_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
    ai_frontend_it_fence_i();
}

bool ai_micro_frontend_mmio_fetch_pma_uncache_single_line_executes_manual_corner()
{
    TEST_START();

    const enum test_page code_page = SWITCH1;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x180U;
    const uintptr_t guard_paddr = code_paddr + 16U;
    const uint64_t guard = 0x4d4d494f53494e31ULL;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x284U);
    write64(guard_paddr, guard);
    ai_frontend_it_configure_pma_tor1(phys_page_base(code_page),
        phys_page_base(code_page) + PAGE_SIZE, AI_FRONTEND_IT_MMIO_OK);
    ai_frontend_it_fence_i();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_frontend_it_exec_probe(code_paddr);
    AI_FRONTEND_IT_ASSERT_NO_FAULT_AND("PMA C=0 executable code page is fetched through the frontend uncache/MMIO path and returns the single-line marker",
        observed == 0x284ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("PMA-uncache single-line fetch does not alter the nearby guard word",
        read64(guard_paddr) == guard);

    ai_frontend_it_configure_pmp_window(phys_page_base(code_page),
        phys_page_base(code_page) + PAGE_SIZE, AI_FRONTEND_IT_PMP_TOR_RWX);

    TEST_SETUP_EXCEPT();
    observed = ai_frontend_it_exec_probe(code_paddr);
    AI_FRONTEND_IT_ASSERT_NO_FAULT_AND("the same PMA-uncache fetch remains executable when an explicit allowing PMP TOR entry is present",
        observed == 0x284ULL);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_mmio_fetch_pma_uncache_cross_64b_refetch_manual_corner()
{
    TEST_START();

    const enum test_page code_page = SWITCH1;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x1feU;
    const uintptr_t guard_paddr = phys_page_base(code_page) + 0x220U;
    const uint64_t guard = 0x4d4d494f58363431ULL;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x285U);
    write64(guard_paddr, guard);
    ai_frontend_it_configure_pma_tor1(phys_page_base(code_page),
        phys_page_base(code_page) + PAGE_SIZE, AI_FRONTEND_IT_MMIO_OK);
    ai_frontend_it_fence_i();

    TEST_SETUP_EXCEPT();
    uint64_t first = ai_frontend_it_exec_probe(code_paddr);
    AI_FRONTEND_IT_ASSERT_NO_FAULT_AND("PMA-uncache fetch of a 32-bit instruction starting at 8B-2 completes after the frontend refetch window",
        first == 0x285ULL);

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_frontend_it_exec_probe(code_paddr);
    AI_FRONTEND_IT_ASSERT_NO_FAULT_AND("reissuing the same cross-8B PMA-uncache instruction returns the same marker without stale replay data",
        second == 0x285ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-8B PMA-uncache refetch preserves the guard word",
        read64(guard_paddr) == guard);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_mmio_fetch_pma_uncache_locked_pmp_pass_manual_corner()
{
    TEST_START();

    const enum test_page code_page = SWITCH1;
    const uintptr_t page_base = phys_page_base(code_page);
    const uintptr_t code_paddr = page_base + 0x200U;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x287U);
    ai_frontend_it_configure_pma_tor1(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_MMIO_OK);
    ai_frontend_it_configure_pmp_locked_allow_window(page_base,
        page_base + PAGE_SIZE);
    ai_frontend_it_fence_i();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_frontend_it_exec_probe(code_paddr);
    TEST_ASSERT("with the PMP TOR entry locked but allowing execute, the PMA-uncache/MMIO fetch still completes and returns the marker",
        excpt.triggered == false &&
        observed == 0x287ULL);

    TEST_SETUP_EXCEPT();
    observed = ai_frontend_it_exec_probe(code_paddr);
    TEST_ASSERT("the locked-PMP PMA-uncache path also completes a repeated fetch without treating the locked entry as a frontend fault",
        excpt.triggered == false &&
        observed == 0x287ULL);

    ai_frontend_it_restore_pma_default();
    TEST_END();
}

bool ai_micro_frontend_mmio_fetch_pma_fail_then_pmp_fail_manual_corner()
{
    TEST_START();

    const enum test_page code_page = SWITCH1;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x240U;
    const uintptr_t page_base = phys_page_base(code_page);

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x291U);
    ai_frontend_it_configure_pma_tor1(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_MMIO_NO_X);
    ai_frontend_it_fence_i();

    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_paddr);
    TEST_ASSERT("PMA MMIO/uncache executable-bit denial on an M-mode fetch reports instruction access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_paddr);

    goto_priv(PRIV_M);
    ai_frontend_it_configure_pma_tor1(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_MMIO_OK);
    ai_frontend_it_configure_pmp_window(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_PMP_TOR_NO_X);
    ai_frontend_it_fence_i();

    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_paddr);
    TEST_ASSERT("after PMA is repaired, a PMP execute denial on the same uncache fetch also reports instruction access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_paddr);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_mmio_fetch_wait_last_commit_manual_corner()
{
    TEST_START();

    const enum test_page code_page = SWITCH1;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x300U;
    volatile uint64_t commit_marker = 0ULL;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x295U);
    ai_frontend_it_configure_pma_tor1(phys_page_base(code_page),
        phys_page_base(code_page) + PAGE_SIZE, AI_FRONTEND_IT_MMIO_OK);
    ai_frontend_it_fence_i();

    for (unsigned i = 0; i < 64U; ++i) {
        commit_marker += (uint64_t)i + 1ULL;
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_frontend_it_exec_probe(code_paddr);
    TEST_ASSERT("after a committed arithmetic window, the PMA-uncache fetch enters the wait-last-commit frontend path and executes the marker block",
        excpt.triggered == false &&
        observed == 0x295ULL &&
        commit_marker == 2080ULL);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_mmio_fetch_mem_attr_cross_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t low_base = phys_page_base(low_page);
    const uintptr_t high_base = phys_page_base(high_page);
    const uintptr_t low_to_mem = low_base + PAGE_SIZE - 2U;
    const uintptr_t mem_to_low = low_base + PAGE_SIZE - 2U;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(low_to_mem, 0x297U);
    ai_frontend_it_configure_pma_two_pages(low_base, high_base,
        AI_FRONTEND_IT_MMIO_OK, AI_FRONTEND_IT_MEM_OK);
    ai_frontend_it_fence_i();

    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(low_to_mem);
    TEST_ASSERT("a split fetch whose first half is PMA-uncache/MMIO and second half is cacheable MEM reports instruction access fault instead of merging attributes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        (excpt.tval == low_to_mem || excpt.tval == high_base));

    goto_priv(PRIV_M);
    ai_frontend_it_write_probe(mem_to_low, 0x298U);
    ai_frontend_it_configure_pma_two_pages(low_base, high_base,
        AI_FRONTEND_IT_MEM_OK, AI_FRONTEND_IT_MMIO_OK);
    ai_frontend_it_fence_i();

    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(mem_to_low);
    TEST_ASSERT("a split fetch whose first half is cacheable MEM and second half is PMA-uncache/MMIO reports instruction access fault on the mixed-attribute block",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        (excpt.tval == mem_to_low || excpt.tval == high_base));

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_fetch_hit_miss_executes_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t code_vaddr = hs_page_base(code_page) + 0x180U;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x180U;
    const uintptr_t guard_paddr = code_paddr + 16U;
    const uint64_t guard = 0x50424d544e433031ULL;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x301U);
    write64(guard_paddr, guard);
    ai_frontend_it_prepare_hs_pbmt_nc(code_page, phys_page_base(code_page),
        PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t cold = ai_frontend_it_exec_probe(code_vaddr);
    AI_FRONTEND_IT_ASSERT_NO_FAULT_AND("the first HS fetch through a PBMT=NC executable leaf walks ITLB and executes the uncache marker",
        cold == 0x301ULL);

    TEST_SETUP_EXCEPT();
    uint64_t hot = ai_frontend_it_exec_probe(code_vaddr);
    AI_FRONTEND_IT_ASSERT_NO_FAULT_AND("the repeated HS fetch through the same PBMT=NC executable leaf hits the refreshed ITLB path and returns the same marker",
        hot == 0x301ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=NC hit/miss fetch probe preserves the nearby code-page guard",
        read64(guard_paddr) == guard);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_fetch_cross_64b_refetch_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t code_vaddr = hs_page_base(code_page) + 0x1feU;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x1feU;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x302U);
    ai_frontend_it_prepare_hs_pbmt_nc(code_page, phys_page_base(code_page),
        PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first = ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("a PBMT=NC 32-bit instruction starting at 8B-2 completes through the frontend refetch path",
        excpt.triggered == false &&
        first == 0x302ULL);

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("reissuing the PBMT=NC cross-8B instruction returns the same marker without stale uncache response data",
        excpt.triggered == false &&
        second == 0x302ULL);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_fetch_pmp_fault_priority_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t code_vaddr = hs_page_base(code_page) + 0x240U;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x240U;
    const uintptr_t page_base = phys_page_base(code_page);

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x305U);
    ai_frontend_it_prepare_hs_pbmt_nc(code_page, page_base,
        PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("before denying PMP execute permission, the PBMT=NC instruction fetch warms the ITLB and executes normally",
        excpt.triggered == false &&
        warmed == 0x305ULL);

    goto_priv(PRIV_M);
    ai_frontend_it_configure_pmp_window(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_PMP_TOR_NO_X);
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("PBMT=NC fetch with an ITLB-hit executable leaf but PMP execute denial reports instruction access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr);

    goto_priv(PRIV_M);
    ai_frontend_it_restore_pmp_full_access();
    ai_frontend_it_configure_pmp_window(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_PMP_TOR_NO_X);
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("after an sfence forces ITLB miss, the same PBMT=NC fetch still reports instruction access fault when PMP execute remains denied",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr);

    goto_priv(PRIV_M);
    ai_frontend_it_restore_pmp_full_access();
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t repaired = ai_frontend_it_exec_probe(code_vaddr);
    AI_FRONTEND_IT_ASSERT_NO_FAULT_AND("after PMP execute permission is restored, the same PBMT=NC fetch executes normally",
        repaired == 0x305ULL);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_itlb_miss_pmp_fail_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t code_vaddr = hs_page_base(code_page) + 0x380U;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x380U;
    const uintptr_t page_base = phys_page_base(code_page);

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x319U);
    ai_frontend_it_prepare_hs_pbmt_nc(code_page, page_base,
        PTE_V | PTE_AD | PTE_RWX);
    ai_frontend_it_configure_pmp_window(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_PMP_TOR_NO_X);
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("cold HS fetch through a PBMT=NC executable leaf should miss ITLB, return a non-page-fault translation, and report PMP execute denial as instruction access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr);

    goto_priv(PRIV_M);
    ai_frontend_it_restore_pmp_full_access();
    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_fetch_page_fault_priority_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t code_vaddr = hs_page_base(code_page) + 0x280U;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x280U;
    const uintptr_t page_base = phys_page_base(code_page);

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x307U);
    ai_frontend_it_prepare_hs_pbmt_nc(code_page, page_base,
        PTE_V | PTE_AD | PTE_RW);
    ai_frontend_it_configure_pmp_window(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_PMP_TOR_NO_X);
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("PBMT=NC fetch with no execute permission and simultaneous PMP execute denial reports instruction page fault first",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == code_vaddr);

    goto_priv(PRIV_M);
    ai_frontend_it_set_hs_leaf(code_page, page_base,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1 | (1ULL << 46));
    ai_frontend_it_restore_pmp_full_access();
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("after repairing X permission, a PBMT=NC leaf with high PPN overflow reports instruction access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr);

    ai_frontend_it_configure_pmp_window(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_PMP_TOR_NO_X);
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("with PBMT=NC high-PPN overflow and a simultaneous PMP execute denial, the frontend still reports instruction access fault from the translation/access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_fetch_access_fault_priority_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t code_vaddr = hs_page_base(code_page) + 0x2c0U;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x2c0U;
    const uintptr_t page_base = phys_page_base(code_page);

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x309U);
    ai_frontend_it_prepare_hs_pbmt_nc(code_page, page_base,
        PTE_V | PTE_AD | PTE_RWX);
    ai_frontend_it_set_hs_leaf(code_page, page_base,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1 | (1ULL << 46));
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("PBMT=NC fetch whose ITLB walk returns a high-PPN access fault reports instruction access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr);

    goto_priv(PRIV_M);
    ai_frontend_it_configure_pmp_window(page_base, page_base + PAGE_SIZE,
        AI_FRONTEND_IT_PMP_TOR_NO_X);
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("PBMT=NC fetch with simultaneous ITLB access fault and PMP execute denial keeps the ITLB access fault as the reported frontend exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr);

    goto_priv(PRIV_M);
    ai_frontend_it_restore_pmp_full_access();
    ai_frontend_it_prepare_hs_pbmt_nc(code_page, page_base,
        PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t repaired = ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("after repairing the PBMT=NC leaf PPN and PMP execute permission, the same fetch executes normally again",
        excpt.triggered == false &&
        repaired == 0x309ULL);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_fetch_wait_last_commit_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t code_vaddr = hs_page_base(code_page) + 0x300U;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x300U;
    volatile uint64_t commit_marker = 0ULL;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x313U);
    ai_frontend_it_prepare_hs_pbmt_nc(code_page, phys_page_base(code_page),
        PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    for (unsigned i = 0; i < 64U; ++i) {
        commit_marker += (uint64_t)i + 1ULL;
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("after a committed HS arithmetic window, the PBMT=NC fetch waits for the prior block to drain and then returns the marker block",
        excpt.triggered == false &&
        observed == 0x313ULL &&
        commit_marker == 2080ULL);

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_fetch_bus_corrupt_special_run_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t code_vaddr = hs_page_base(code_page) + 0x340U;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x340U;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(code_paddr, 0x311U);
    ai_frontend_it_prepare_hs_pbmt_nc(code_page, phys_page_base(code_page),
        PTE_V | PTE_AD | PTE_RWX);

#if VMODULE
    ai_frontend_it_vmodule_write_reg(AI_FRONTEND_IT_VMODULE_REG_AXI_ERROR, 0ULL);
    ai_frontend_it_vmodule_write_reg(AI_FRONTEND_IT_VMODULE_REG_AXI_ERROR, 1ULL);
#endif

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_frontend_it_exec_probe(code_vaddr);
    TEST_ASSERT("PBMT=NC bus-corrupt row is a special-run hook: without a forced corrupt fetch response the marker executes, while AP-IT waveform must confirm the corrupt response path raises frontend access fault",
        excpt.triggered == false &&
        observed == 0x311ULL);

#if VMODULE
    goto_priv(PRIV_M);
    ai_frontend_it_vmodule_write_reg(AI_FRONTEND_IT_VMODULE_REG_AXI_ERROR, 0ULL);
#endif

    ai_frontend_it_cleanup();
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_fetch_mem_attr_cross_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page high_page = VSRWXPbmt1_GURWXPbmt2;
    const uintptr_t low_vaddr = hs_page_base(low_page) + PAGE_SIZE - 2U;
    const uintptr_t high_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 2U;

    goto_priv(PRIV_M);
    ai_frontend_it_cleanup();
    ai_frontend_it_write_probe(low_paddr, 0x315U);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_frontend_it_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_frontend_it_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(low_vaddr);
    TEST_ASSERT("a split fetch whose first half is PBMT=NC and second half is ordinary PMA/MEM reports instruction access fault on the mixed-attribute block",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        (excpt.tval == low_vaddr || excpt.tval == high_vaddr));

    goto_priv(PRIV_M);
    ai_frontend_it_write_probe(low_paddr, 0x316U);
    ai_frontend_it_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_frontend_it_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    sfence_vma();
    ai_frontend_it_fence_i();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_frontend_it_exec_probe(low_vaddr);
    TEST_ASSERT("a split fetch whose first half is ordinary PMA/MEM and second half is PBMT=NC reports instruction access fault on the mixed-attribute block",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        (excpt.tval == low_vaddr || excpt.tval == high_vaddr));

    ai_frontend_it_cleanup();
    TEST_END();
}
