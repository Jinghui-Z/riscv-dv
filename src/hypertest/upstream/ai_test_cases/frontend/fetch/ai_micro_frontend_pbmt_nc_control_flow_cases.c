#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_PBMT_NC_CTRL_CODE_OFFSET  0x180ULL
#define AI_PBMT_NC_CTRL_GUARD_OFFSET 0x300ULL
#define AI_PBMT_NC_CTRL_PMP_TOR_RWX  0x0fULL
#define AI_PBMT_NC_CTRL_EXPECTED     15ULL

extern const uint8_t ai_pbmt_nc_ctrl_template_start[];
extern const uint8_t ai_pbmt_nc_ctrl_template_cjr[];
extern const uint8_t ai_pbmt_nc_ctrl_template_cebreak[];
extern const uint8_t ai_pbmt_nc_ctrl_template_resume[];
extern const uint8_t ai_pbmt_nc_ctrl_template_end[];

asm(
    ".pushsection .text.ai_pbmt_nc_ctrl_template,\"ax\",@progbits\n"
    ".balign 4\n"
    ".global ai_pbmt_nc_ctrl_template_start\n"
    "ai_pbmt_nc_ctrl_template_start:\n"
    ".option push\n"
    ".option rvc\n"
    ".option norelax\n"
    "addi a0, a0, 1\n"
    "call ai_pbmt_nc_ctrl_template_callee\n"
    "addi a0, a0, 2\n"
    ".global ai_pbmt_nc_ctrl_template_cjr\n"
    "ai_pbmt_nc_ctrl_template_cjr:\n"
    "c.jr t0\n"
    "addi a0, a0, 256\n"
    ".global ai_pbmt_nc_ctrl_template_cebreak\n"
    "ai_pbmt_nc_ctrl_template_cebreak:\n"
    "c.ebreak\n"
    ".global ai_pbmt_nc_ctrl_template_resume\n"
    "ai_pbmt_nc_ctrl_template_resume:\n"
    "addi a0, a0, 8\n"
    "mv ra, t6\n"
    "ret\n"
    "ai_pbmt_nc_ctrl_template_callee:\n"
    "addi a0, a0, 4\n"
    "ret\n"
    ".option pop\n"
    ".global ai_pbmt_nc_ctrl_template_end\n"
    "ai_pbmt_nc_ctrl_template_end:\n"
    ".popsection\n");

static inline pte_t *ai_pbmt_nc_ctrl_hs_leaf(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uint16_t ai_pbmt_nc_ctrl_read16(const uint8_t *addr)
{
    return (uint16_t)addr[0] | ((uint16_t)addr[1] << 8U);
}

__attribute__((noinline, optimize("no-tree-vectorize")))
static void ai_pbmt_nc_ctrl_copy_template(uintptr_t paddr)
{
    const uintptr_t size = (uintptr_t)ai_pbmt_nc_ctrl_template_end -
        (uintptr_t)ai_pbmt_nc_ctrl_template_start;

    for (uintptr_t i = 0; i < size; ++i) {
        write8(paddr + i, ai_pbmt_nc_ctrl_template_start[i]);
    }
}

static uint64_t ai_pbmt_nc_ctrl_execute(uintptr_t entry,
    uintptr_t cjr_target)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lla t6, 1f\n\t"
        "mv t0, %[target]\n\t"
        "li a0, 0\n\t"
        "jr %[entry]\n\t"
        "1:\n\t"
        "mv %[observed], a0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [entry] "r"(entry), [target] "r"(cjr_target)
        : "ra", "t0", "t6", "a0", "memory");

    return observed;
}

bool ai_micro_hs_pbmt_nc_mixed_cjr_cebreak_call_ret_fetch_manual_corner()
{
    TEST_START();

    const enum test_page code_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t page_paddr = phys_page_base(code_page);
    const uintptr_t code_paddr = page_paddr + AI_PBMT_NC_CTRL_CODE_OFFSET;
    const uintptr_t code_vaddr = hs_page_base(code_page) +
        AI_PBMT_NC_CTRL_CODE_OFFSET;
    const uintptr_t guard_paddr = page_paddr + AI_PBMT_NC_CTRL_GUARD_OFFSET;
    const uint64_t guard_seed = 0x50424d5443464931ULL;
    const uintptr_t template_size =
        (uintptr_t)ai_pbmt_nc_ctrl_template_end -
        (uintptr_t)ai_pbmt_nc_ctrl_template_start;
    const uintptr_t cjr_offset =
        (uintptr_t)ai_pbmt_nc_ctrl_template_cjr -
        (uintptr_t)ai_pbmt_nc_ctrl_template_start;
    const uintptr_t cebreak_offset =
        (uintptr_t)ai_pbmt_nc_ctrl_template_cebreak -
        (uintptr_t)ai_pbmt_nc_ctrl_template_start;
    const uintptr_t resume_offset =
        (uintptr_t)ai_pbmt_nc_ctrl_template_resume -
        (uintptr_t)ai_pbmt_nc_ctrl_template_start;
    const uintptr_t cjr_target = code_vaddr + cebreak_offset;
    const uintptr_t cebreak_pc = code_vaddr + cebreak_offset;

    goto_priv(PRIV_M);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);

    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_BKP);
    CSRW(CSR_PMPCFG0, AI_PBMT_NC_CTRL_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    hspt_init();
    *ai_pbmt_nc_ctrl_hs_leaf(code_page) =
        ((page_paddr & PAGE_ADDR_MSK) >> 2) |
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;

    ai_pbmt_nc_ctrl_copy_template(code_paddr);
    write64(guard_paddr, guard_seed);
    fence_iorw_iorw();
    cbo_flush(code_paddr);
    fence_iorw_iorw();
    sfence_vma();
    asm volatile("fence.i" ::: "memory");

    TEST_ASSERT("the copied PBMT=NC instruction template contains c.jr t0 and c.ebreak as adjacent compressed control-flow events within one executable page",
        template_size > 0ULL &&
        AI_PBMT_NC_CTRL_CODE_OFFSET + template_size <
            AI_PBMT_NC_CTRL_GUARD_OFFSET &&
        cjr_offset < cebreak_offset &&
        resume_offset == cebreak_offset + 2ULL &&
        ai_pbmt_nc_ctrl_read16(ai_pbmt_nc_ctrl_template_cjr) == 0x8282U &&
        ai_pbmt_nc_ctrl_read16(ai_pbmt_nc_ctrl_template_cebreak) == 0x9002U &&
        (*ai_pbmt_nc_ctrl_hs_leaf(code_page) &
            (PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1)) ==
            (PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    const uint64_t cold_result = ai_pbmt_nc_ctrl_execute(code_vaddr,
        cjr_target);
    const bool cold_triggered = excpt.triggered;
    const uint64_t cold_cause = excpt.cause;
    const uint64_t cold_epc = excpt.epc;
    const enum priv cold_priv = excpt.priv;

    TEST_ASSERT("the cold PBMT=NC fetch executes call/ret, takes c.jr over the poison instruction, reports c.ebreak at its compressed PC, and resumes to the final ret",
        cold_triggered == true &&
        cold_priv == PRIV_HS &&
        cold_cause == CAUSE_BKP &&
        cold_epc == cebreak_pc &&
        cold_result == AI_PBMT_NC_CTRL_EXPECTED,
        "cause=0x%llx epc=0x%llx expected_epc=0x%llx result=0x%llx",
        (unsigned long long)cold_cause,
        (unsigned long long)cold_epc,
        (unsigned long long)cebreak_pc,
        (unsigned long long)cold_result);

    TEST_SETUP_EXCEPT();
    const uint64_t hot_result = ai_pbmt_nc_ctrl_execute(code_vaddr,
        cjr_target);
    const bool hot_triggered = excpt.triggered;
    const uint64_t hot_cause = excpt.cause;
    const uint64_t hot_epc = excpt.epc;

    TEST_ASSERT("the repeated PBMT=NC control-flow fetch preserves the same mixed-width redirect, breakpoint, call and return behavior without stale uncache data",
        hot_triggered == true &&
        excpt.priv == PRIV_HS &&
        hot_cause == CAUSE_BKP &&
        hot_epc == cebreak_pc &&
        hot_result == AI_PBMT_NC_CTRL_EXPECTED,
        "cause=0x%llx epc=0x%llx expected_epc=0x%llx result=0x%llx",
        (unsigned long long)hot_cause,
        (unsigned long long)hot_epc,
        (unsigned long long)cebreak_pc,
        (unsigned long long)hot_result);

    goto_priv(PRIV_M);
    TEST_ASSERT("mixed c.jr, c.ebreak, call and ret fetches leave the adjacent PBMT=NC code-page guard unchanged",
        read64(guard_paddr) == guard_seed);

    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    hspt_init();
    sfence_vma();
    asm volatile("fence.i" ::: "memory");

    TEST_END();
}
