#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_INSTR_UNCACHE_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline pte_t *ai_instr_uncache_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_instr_uncache_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}

static inline uint32_t ai_instr_uncache_addi_a0_imm(uint64_t imm)
{
    return (uint32_t)(((imm & 0xfffULL) << 20) | (10U << 7) | 0x13U);
}

static inline uint32_t ai_instr_uncache_jal_x0(uint32_t offset)
{
    return (((offset >> 20) & 0x1U) << 31) |
        (((offset >> 1) & 0x3ffU) << 21) |
        (((offset >> 11) & 0x1U) << 20) |
        (((offset >> 12) & 0xffU) << 12) |
        0x6fU;
}

static inline void ai_instr_uncache_write32_halfwise(uintptr_t paddr,
    uint32_t inst)
{
    write16(paddr, (uint16_t)(inst & 0xffffU));
    write16(paddr + sizeof(uint16_t), (uint16_t)(inst >> 16));
}

static inline uint64_t ai_instr_uncache_call_ret64(uintptr_t vaddr)
{
    uint64_t (*fn)(void) = (uint64_t (*)(void))vaddr;
    return fn();
}

static inline void ai_instr_uncache_set_pbmt_nc_leaf(enum test_page va_page,
    enum test_page backing_page)
{
    *ai_instr_uncache_hs_leaf_entry(va_page) =
        (phys_page_base(backing_page) >> 2) |
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;
}

static inline void ai_instr_uncache_seed_branch_image(uintptr_t paddr,
    uintptr_t off, uint64_t good_value, uint64_t poison_value)
{
    write32(paddr + off, ai_instr_uncache_jal_x0(12U));
    write32(paddr + off + 4U, ai_instr_uncache_addi_a0_imm(poison_value));
    write32(paddr + off + 8U, 0x00008067U);
    write32(paddr + off + 12U, ai_instr_uncache_addi_a0_imm(good_value));
    write32(paddr + off + 16U, 0x00008067U);
}

static inline void ai_instr_uncache_seed_resend_image(uintptr_t paddr,
    uintptr_t off, uint64_t value)
{
    ai_instr_uncache_write32_halfwise(paddr + off,
        ai_instr_uncache_addi_a0_imm(value));
    ai_instr_uncache_write32_halfwise(paddr + off + 4U, 0x00008067U);
}

static inline void ai_instr_uncache_prepare_nc_aliases(enum test_page page_a,
    enum test_page backing_a, enum test_page page_b, enum test_page backing_b)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_instr_uncache_set_pbmt_nc_leaf(page_a, backing_a);
    ai_instr_uncache_set_pbmt_nc_leaf(page_b, backing_b);
    sfence_vma();
    ai_instr_uncache_fence_i();
    goto_priv(PRIV_HS);
}

/*
 * P1-215 manual/nongate smoke.
 *
 * The observable part checks that PBMT=NC instruction fetches with a taken
 * branch and a cross-8B 32-bit instruction retire the current-path image only.
 * LinkNan closure should additionally correlate this stream with
 * IFU.MmioFsm/InstrUncache path markers and verify late wrong-path grants do
 * not drive mmioFlushWb, IBuffer enqueue, or the next fetch owner's metadata.
 */
bool ai_micro_hs_instr_uncache_pbmt_nc_branch_resend_quarantine_manual_corner()
{
    TEST_START();

    const enum test_page nc_branch_page = VSRWXPbmt1_GURWXPbmt0;
    const enum test_page nc_resend_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page branch_backing_page = VSRWX_GRWX;
    const enum test_page resend_backing_page = VSRW_GRWX;
    const uintptr_t branch_off = 0x280U;
    const uintptr_t resend_off = 0x386U;
    const uintptr_t branch_vaddr = hs_page_base(nc_branch_page) + branch_off;
    const uintptr_t resend_vaddr = hs_page_base(nc_resend_page) + resend_off;
    const uintptr_t branch_paddr =
        phys_page_base(branch_backing_page) + branch_off;
    const uintptr_t resend_paddr =
        phys_page_base(resend_backing_page) + resend_off;
    const uintptr_t branch_guard_paddr =
        phys_page_base(branch_backing_page) + 0x3c0U;
    const uintptr_t resend_guard_paddr =
        phys_page_base(resend_backing_page) + 0x3c0U;
    const uint64_t branch_good = 0x215U;
    const uint64_t branch_poison = 0x0a1U;
    const uint64_t resend_old = 0x216U;
    const uint64_t resend_new = 0x217U;
    const uint64_t branch_guard = 0x215b6a4d00000001ULL;
    const uint64_t resend_guard = 0x215b6a4d00000002ULL;

    goto_priv(PRIV_M);
    ai_instr_uncache_seed_branch_image(
        phys_page_base(branch_backing_page), branch_off, branch_good,
        branch_poison);
    ai_instr_uncache_seed_resend_image(
        phys_page_base(resend_backing_page), resend_off, resend_old);
    write64(branch_guard_paddr, branch_guard);
    write64(resend_guard_paddr, resend_guard);
    fence_iorw_iorw();

    ai_instr_uncache_prepare_nc_aliases(nc_branch_page, branch_backing_page,
        nc_resend_page, resend_backing_page);

    TEST_SETUP_EXCEPT();
    uint64_t branch_first = ai_instr_uncache_call_ret64(branch_vaddr);
    AI_INSTR_UNCACHE_ASSERT_NO_EXCEPT_AND("PBMT=NC taken-branch instruction fetch retires the current-path image and not the fall-through poison image",
        branch_first == branch_good);

    TEST_SETUP_EXCEPT();
    uint64_t resend_first = ai_instr_uncache_call_ret64(resend_vaddr);
    AI_INSTR_UNCACHE_ASSERT_NO_EXCEPT_AND("PBMT=NC cross-8B 32-bit instruction fetch assembles the low and high halves from the same owner",
        resend_first == resend_old);

    for (unsigned i = 0; i < 4U; ++i) {
        TEST_SETUP_EXCEPT();
        uint64_t branch_value = ai_instr_uncache_call_ret64(branch_vaddr);
        uint64_t resend_value = ai_instr_uncache_call_ret64(resend_vaddr);
        AI_INSTR_UNCACHE_ASSERT_NO_EXCEPT_AND("back-to-back PBMT=NC branch/resend fetches keep the correct owner values across repeated uncache episodes",
            branch_value == branch_good &&
            resend_value == resend_old);
    }

    goto_priv(PRIV_M);
    ai_instr_uncache_seed_resend_image(
        phys_page_base(resend_backing_page), resend_off, resend_new);
    fence_iorw_iorw();
    ai_instr_uncache_fence_i();
    sfence_vma();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    uint64_t resend_after_rewrite = ai_instr_uncache_call_ret64(resend_vaddr);
    AI_INSTR_UNCACHE_ASSERT_NO_EXCEPT_AND("after rewriting the PBMT=NC resend target and flushing, the next fetch observes only the new instruction image",
        resend_after_rewrite == resend_new);

    TEST_SETUP_EXCEPT();
    uint64_t branch_after_resend_rewrite =
        ai_instr_uncache_call_ret64(branch_vaddr);
    AI_INSTR_UNCACHE_ASSERT_NO_EXCEPT_AND("a late response from the rewritten PBMT=NC resend stream must not corrupt the neighboring taken-branch fetch owner",
        branch_after_resend_rewrite == branch_good);

    goto_priv(PRIV_M);
    TEST_ASSERT("the InstrUncache PBMT=NC manual smoke preserves both backing code images and guard words",
        read32(branch_paddr) == ai_instr_uncache_jal_x0(12U) &&
        read32(branch_paddr + 12U) ==
            ai_instr_uncache_addi_a0_imm(branch_good) &&
        read16(resend_paddr) ==
            (uint16_t)(ai_instr_uncache_addi_a0_imm(resend_new) & 0xffffU) &&
        read16(resend_paddr + sizeof(uint16_t)) ==
            (uint16_t)(ai_instr_uncache_addi_a0_imm(resend_new) >> 16) &&
        read64(branch_guard_paddr) == branch_guard &&
        read64(resend_guard_paddr) == resend_guard);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);

    TEST_END();
}
