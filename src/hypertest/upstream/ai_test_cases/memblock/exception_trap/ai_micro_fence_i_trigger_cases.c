#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_FENCE_I_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_MICRO_FENCE_I_MCONTROL6_EXECUTE 0x4ULL

extern void ai_micro_fence_i_trigger_once(void);
extern char ai_micro_fence_i_trigger_site[];
extern void ai_micro_sfence_vma_trigger_once(uintptr_t vaddr, uint64_t asid);
extern char ai_micro_sfence_vma_trigger_site[];

asm(
    ".balign 4\n"
    ".global ai_micro_fence_i_trigger_once\n"
    ".global ai_micro_fence_i_trigger_site\n"
    "ai_micro_fence_i_trigger_once:\n"
    ".option push\n"
    ".option norvc\n"
    "ai_micro_fence_i_trigger_site:\n"
    "  .4byte 0x0000100f\n"
    "  ret\n"
    ".option pop\n"
);

asm(
    ".balign 4\n"
    ".global ai_micro_sfence_vma_trigger_once\n"
    ".global ai_micro_sfence_vma_trigger_site\n"
    "ai_micro_sfence_vma_trigger_once:\n"
    ".option push\n"
    ".option norvc\n"
    "ai_micro_sfence_vma_trigger_site:\n"
    "  sfence.vma a0, a1\n"
    "  ret\n"
    ".option pop\n"
);

static inline pte_t *ai_micro_fence_i_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_fence_i_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_micro_fence_i_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline uint64_t ai_micro_special_trigger_set_asid(uint64_t asid)
{
    uint64_t satp = CSRR(CSR_SATP);
    satp &= ~(0xffffULL << 44);
    satp |= (asid & 0xffffULL) << 44;
    CSRW(CSR_SATP, satp);
    return satp;
}

static inline uint64_t ai_micro_special_trigger_ld_rd_probe(uintptr_t addr,
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

static inline void ai_micro_fence_i_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_micro_fence_i_arm_execute_trigger(size_t slot,
    uintptr_t pc)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, pc);
    CSRW(CSR_TDATA1,
        AI_MICRO_FENCE_I_MCONTROL6_MSU_BASE |
        AI_MICRO_FENCE_I_MCONTROL6_EXECUTE);
}

static inline uint64_t ai_micro_fence_i_enable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static inline void ai_micro_fence_i_restore_mstatus(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

bool ai_micro_hs_fence_i_self_execute_trigger_blocks_fence_effects_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWX_GRWX;
    const uintptr_t code_vaddr = hs_page_base(code_page) + 0x180U;
    const uintptr_t code_paddr = phys_page_base(code_page) + 0x180U;
    const uintptr_t fence_i_pc = (uintptr_t)ai_micro_fence_i_trigger_site;
    const uint64_t warm_ret3 = 0x0000806700300513ULL;
    const uint64_t patched_ret7 = 0x0000806700700513ULL;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;

    goto_priv(PRIV_M);
    ai_micro_fence_i_clear_mcontrol6_slot(0);
    write64(code_paddr, warm_ret3);

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    ai_micro_fence_i_set_hs_leaf(code_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_fence_i_trigger_once();
    TEST_SETUP_EXCEPT();
    uint64_t observed = fetch_func();
    TEST_ASSERT("the warm HS executable block returns the original value before the self-triggered fence.i probe",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, patched_ret7);
    TEST_ASSERT("the HS store patches the executable block before the fence.i instruction is allowed to retire",
        excpt.triggered == false
    );

    ai_micro_fence_i_arm_execute_trigger(0, fence_i_pc);
    uint64_t saved_mstatus = ai_micro_fence_i_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_fence_i_trigger_once();
    TEST_ASSERT("an execute trigger armed on the actual fence.i instruction reports breakpoint with epc equal to the fence.i PC",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == fence_i_pc
    );

    ai_micro_fence_i_restore_mstatus(saved_mstatus);
    ai_micro_fence_i_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the breakpoint skips the fence.i instruction, the warmed fetch path still observes the pre-patch instruction image",
        excpt.triggered == false &&
        observed == 3ULL
    );

    ai_micro_fence_i_trigger_once();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the same fence.i instruction is retried without the execute trigger, the patched instruction image becomes visible",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fence.i execute-trigger episode does not corrupt the patched backing instruction word",
        read64(code_paddr) == patched_ret7
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_self_execute_trigger_preserves_old_tlb_entry_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t vaddr = hs_page_base(page) + 0x318U;
    const uintptr_t paddr = phys_page_base(page) + 0x318U;
    const uintptr_t sfence_pc = (uintptr_t)ai_micro_sfence_vma_trigger_site;
    const uint64_t asid = 0x0693ULL;
    const uint64_t initial = 0x7366656e63653131ULL;
    const uint64_t hu_update = 0x7366656e63653232ULL;
    const uint64_t denied_sentinel = 0xd00d00000000a111ULL;
    const uint64_t stale_sentinel = 0xd00d00000000b222ULL;
    const uint64_t blocked_sentinel = 0xd00d00000000c333ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_ECU);
    ai_micro_fence_i_clear_mcontrol6_slot(0);
    write64(paddr, initial);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow_no_leaf();
    ai_micro_special_trigger_set_asid(asid);
    *ai_micro_fence_i_hs_leaf_entry(page) =
        (*ai_micro_fence_i_hs_leaf_entry(page) & ~PTE_U) |
        PTE_V | PTE_AD | PTE_RWX;
    const uint64_t supervisor_leaf = *ai_micro_fence_i_hs_leaf_entry(page);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t hs_warm = ld(vaddr);
    TEST_ASSERT("HS load warms the supervisor-only translation before the sfence.vma execute-trigger probe",
        excpt.triggered == false &&
        hs_warm == initial
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t denied_rd = ai_micro_special_trigger_ld_rd_probe(vaddr,
        denied_sentinel);
    TEST_ASSERT("HU load through the warmed supervisor-only translation reports LPF and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        denied_rd == denied_sentinel
    );

    goto_priv_u_to_m_direct(PRIV_M);
    *ai_micro_fence_i_hs_leaf_entry(page) = supervisor_leaf | PTE_U;
    const uint64_t user_leaf = *ai_micro_fence_i_hs_leaf_entry(page);
    fence_iorw_iorw();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t stale_rd = ai_micro_special_trigger_ld_rd_probe(vaddr,
        stale_sentinel);
    TEST_ASSERT("after software sets PTE.U=1 without SFENCE.VMA, the old TLB entry is still allowed to fault in HU mode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        stale_rd == stale_sentinel
    );

    goto_priv_u_to_m_direct(PRIV_M);
    ai_micro_fence_i_arm_execute_trigger(0, sfence_pc);
    uint64_t saved_mstatus = ai_micro_fence_i_enable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_micro_sfence_vma_trigger_once(vaddr, asid);
    TEST_ASSERT("an execute trigger armed on the actual sfence.vma instruction reports breakpoint with epc equal to the sfence.vma PC",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == sfence_pc
    );

    ai_micro_fence_i_restore_mstatus(saved_mstatus);
    ai_micro_fence_i_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t blocked_rd = ai_micro_special_trigger_ld_rd_probe(vaddr,
        blocked_sentinel);
    TEST_ASSERT("because the triggered sfence.vma was skipped, the stale supervisor-only TLB entry still faults in HU mode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        blocked_rd == blocked_sentinel
    );

    goto_priv_u_to_m_direct(PRIV_M);
    ai_micro_sfence_vma_trigger_once(vaddr, asid);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t refreshed_load = ld(vaddr);
    TEST_ASSERT("after the same sfence.vma instruction retires without the execute trigger, HU load observes the refreshed PTE.U=1 translation",
        excpt.triggered == false &&
        refreshed_load == initial
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, hu_update);
    TEST_ASSERT("the refreshed PTE.U=1 translation also permits the follow-up HU store",
        excpt.triggered == false
    );

    goto_priv_u_to_m_direct(PRIV_M);
    TEST_ASSERT("the sfence.vma execute-trigger episode preserves the final leaf and only the legal HU store updates backing memory",
        *ai_micro_fence_i_hs_leaf_entry(page) == user_leaf &&
        read64(paddr) == hu_update
    );

    TEST_END();
}
