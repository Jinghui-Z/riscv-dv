#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Frontend IT rows 343+: nhv5_1_ap, no-H. See docs/frontend_it_case_mapping.md.
 * Software signatures check architecture; FTQ/MSHR timing needs RTL fcov.
 */
static inline pte_t *fe_leaf(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}
static inline void fe_map(enum test_page page, uintptr_t pa, uint64_t flags)
{
    *fe_leaf(page) = (pa >> 2) | PTE_V | PTE_AD | flags;
}
static inline void fe_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}
static void fe_put32(uintptr_t pa, uint32_t inst)
{
    write16(pa, inst);
    write16(pa + 2, inst >> 16);
}
static void fe_split_code(void)
{
    uintptr_t pa = phys_page_base(SWITCH1) + PAGE_SIZE - 6;
    fe_put32(pa, 0x00150513U);       /* preceding addi a0,a0,1 */
    fe_put32(pa + 4, 0x00150513U);   /* split addi a0,a0,1 */
    fe_put32(pa + 8, 0x00008067U);   /* ret at second page +2 */
}
/* The exception handler skips the saved jalr, not an inaccessible target PC. */
static uint64_t fe_call(uintptr_t pc, uint64_t seed, uint64_t selector)
{
    uint64_t result;
    asm volatile(
        ".option push\n\t.option norvc\n\t"
        "lla t0, 1f\n\tsd t0, %[fault]\n\t"
        "mv a0, %[seed]\n\tmv a1, %[selector]\n\t"
        "1: jalr ra, 0(%[pc])\n\t"
        "mv %[result], a0\n\t.option pop\n\t"
        : [result] "=r"(result), [fault] "=m"(excpt.fault_inst)
        : [pc] "r"(pc), [seed] "r"(seed), [selector] "r"(selector)
        : "t0", "ra", "a0", "a1", "memory");
    excpt.fault_inst = 0;
    return result;
}

bool ai_micro_frontend_cross_page_fetch_fault_attribution_corner()
{
    TEST_START();
    const uintptr_t pc = hs_page_base(SWITCH1) + PAGE_SIZE - 2;
    goto_priv(PRIV_M);
    hspt_init();
    fe_split_code();
    fe_map(SWITCH1, phys_page_base(SWITCH1), PTE_RWX);
    fe_map(SWITCH2, phys_page_base(SWITCH2), PTE_RW);
    sfence_vma();
    fe_fence_i();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = fe_call(pc - 4, 40, 0);
    TEST_ASSERT("second-page IPF names the split PC and high halfword, preserving the preceding complete instruction",
        excpt.triggered && excpt.cause == CAUSE_IPF &&
        excpt.epc == pc && excpt.tval == pc + 2 && value == 41,
        "cause=%llx epc=%llx tval=%llx value=%llu",
        excpt.cause, excpt.epc, excpt.tval, value);
    goto_priv(PRIV_M);
    fe_map(SWITCH2, phys_page_base(SWITCH2), PTE_RWX);
    sfence_vma();
    fe_fence_i();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = fe_call(pc - 4, 40, 0);
    TEST_ASSERT("after repairing X permission both addi instructions execute exactly once",
        !excpt.triggered && value == 42);
    goto_priv(PRIV_M);
    hspt_init();
    TEST_END();
}

bool ai_micro_frontend_split_page_pmp_first_second_fault_corner()
{
    TEST_START();
    const uintptr_t pc = hs_page_base(SWITCH1) + PAGE_SIZE - 2;
    goto_priv(PRIV_M);
    const uint64_t cfg = CSRR(CSR_PMPCFG0);
    const uint64_t addr0 = CSRR(CSR_PMPADDR0);
    const uint64_t addr1 = CSRR(CSR_PMPADDR1);
    const uint64_t addr2 = CSRR(CSR_PMPADDR2);
    hspt_init();
    fe_split_code();
    fe_map(SWITCH1, phys_page_base(SWITCH1), PTE_RWX);
    fe_map(SWITCH2, phys_page_base(SWITCH2), PTE_RWX);
    for (unsigned second = 0; second < 2; ++second) {
        uintptr_t deny = phys_page_base(second ? SWITCH2 : SWITCH1);
        CSRW(CSR_PMPCFG0, 0);
        CSRW(CSR_PMPADDR0, deny >> 2);
        CSRW(CSR_PMPADDR1, (deny + PAGE_SIZE) >> 2);
        CSRW(CSR_PMPADDR2, ~0ULL);
        CSRW(CSR_PMPCFG0, 0x0f0b0fULL); /* RWX / RW / RWX, 4KB TOR */
        sfence_vma();
        fe_fence_i();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t value = fe_call(second ? pc - 4 : pc, 40, 0);
        TEST_ASSERT("first/second-page PMP deny reports the faulting instruction portion and preserves earlier work",
            excpt.triggered && excpt.cause == CAUSE_IAF &&
            excpt.epc == pc && excpt.tval == pc + 2 * second &&
            value == 40 + second,
            "second=%u cause=%llx epc=%llx tval=%llx value=%llu",
            second, excpt.cause, excpt.epc, excpt.tval, value);
        goto_priv(PRIV_M);
    }
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, addr0);
    CSRW(CSR_PMPADDR1, addr1);
    CSRW(CSR_PMPADDR2, addr2);
    CSRW(CSR_PMPCFG0, cfg);
    sfence_vma();
    fe_fence_i();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = fe_call(pc - 4, 40, 0);
    TEST_ASSERT("restored PMP permits both halves without residual fault",
        !excpt.triggered && value == 42);
    goto_priv(PRIV_M);
    hspt_init();
    TEST_END();
}

static uint8_t fe_stream[128 * 1024] __attribute__((aligned(4096)));

static uint32_t fe_jal(int32_t offset)
{
    uint32_t u = (uint32_t)offset;
    return ((u >> 20 & 1) << 31) | ((u >> 1 & 1023) << 21) |
        ((u >> 11 & 1) << 20) | (u & 0xff000U) | 0x6fU;
}
__attribute__((optimize("no-tree-vectorize")))
static void fe_build_stream(unsigned count, unsigned step, unsigned increment,
    bool conditional)
{
    for (unsigned visit = 0; visit < count; ++visit) {
        unsigned slot = visit * step % count;
        uintptr_t pc = (uintptr_t)fe_stream + slot * 64;
        for (unsigned off = 0; off < 64; off += 4)
            fe_put32(pc + off, 0x00000013U);
        fe_put32(pc, (increment << 20) | 0x50513U);
        if (conditional) {
            fe_put32(pc + 4, 0x00058463U);  /* beq a1,zero,+8 */
            fe_put32(pc + 8, 0x00250513U);  /* addi a0,a0,2 */
            fe_put32(pc + 12, 0x0015c593U); /* xori a1,a1,1 */
        }
        unsigned next = (visit + 1) * step % count;
        fe_put32(pc + 16, visit + 1 == count ? 0x00008067U :
            fe_jal((int32_t)(next * 64) - (int32_t)(slot * 64 + 16)));
    }
    fence_iorw_iorw();
    fe_fence_i();
}

bool ai_micro_frontend_ftq_redirect_and_pointer_pressure_corner()
{
    TEST_START();
    goto_priv(PRIV_M);
    fe_build_stream(128, 1, 1, true);
    bool correct = true;
    TEST_SETUP_EXCEPT();
    for (unsigned round = 0; round < 128; ++round) {
        unsigned phase = round % 64, count = 128 - phase;
        unsigned selector = (round / 64) & 1;
        uint64_t expected = 7 + count + 2 * ((count + selector) / 2);
        uint64_t value = fe_call((uintptr_t)fe_stream + phase * 64, 7, selector);
        correct &= value == expected && !excpt.triggered;
    }
    TEST_ASSERT("128 branch blocks with all 64 entry phases and both selectors preserve the exact path sum", correct);
    TEST_END();
}

bool ai_micro_frontend_refill_fencei_repeated_fetch_corner()
{
    TEST_START();
    goto_priv(PRIV_M);
    bool correct = true;
    for (unsigned pass = 0; pass < 2; ++pass) {
        unsigned increment = 2 * pass + 1;
        /* 128KB exceeds the current 64KB ICache; step 17 visits every line. */
        fe_build_stream(2048, pass & 1 ? 17 : 1, increment, false);
        TEST_SETUP_EXCEPT();
        for (unsigned round = 0; round < 2; ++round) {
            uint64_t value = fe_call((uintptr_t)fe_stream, round, 0);
            correct &= !excpt.triggered && value == round + 2048 * increment;
        }
    }
    TEST_ASSERT("cold/warm/refetched 128KB code uses the new instruction image after each fence.i", correct);
    TEST_END();
}

bool ai_micro_frontend_delayed_store_redirect_fetch_pressure_corner()
{
    TEST_START();
    goto_priv(PRIV_M);
    static volatile uintptr_t chain[5] __attribute__((aligned(64)));
    static volatile uint64_t target __attribute__((aligned(64)));
    for (unsigned i = 0; i < 4; ++i)
        chain[i] = (uintptr_t)&chain[i + 1];
    chain[4] = (uintptr_t)&target;
    bool correct = true;
    TEST_SETUP_EXCEPT();
    for (unsigned i = 0; i < 4096; ++i) {
        uint64_t observed, expected = 0x12340000ULL + i;
        target = ~expected;
        asm volatile(
            ".option push\n\t.option norvc\n\t"
            "ld t0, 0(%[chain])\n\tld t0, 0(t0)\n\t"
            "ld t0, 0(t0)\n\tld t0, 0(t0)\n\tld t0, 0(t0)\n\t"
            "sd %[value], 0(t0)\n\tld %[result], 0(%[target])\n\t"
            "beq %[result], %[value], 1f\n\t"
            "addi %[result], %[result], 1\n\t1:\n\t.option pop\n\t"
            : [result] "=&r"(observed)
            : [chain] "r"(chain), [target] "r"(&target), [value] "r"(expected)
            : "t0", "memory");
        correct &= observed == expected && target == expected;
    }
    TEST_ASSERT("a younger load observes the older delayed-address store across replay/branch pressure",
        correct && !excpt.triggered);
    TEST_END();
}

bool ai_micro_frontend_same_pa_different_vset_fetch_corner()
{
    TEST_START();
    goto_priv(PRIV_M);
    hspt_init();
    uintptr_t pa = phys_page_base(SWITCH1);
    fe_put32(pa, 0x00950513U);
    fe_put32(pa + 4, 0x00008067U);
    fe_map(SWITCH1, pa, PTE_RWX);
    fe_map(SWITCH2, pa, PTE_RWX);
    sfence_vma();
    fe_fence_i();
    uintptr_t va0 = hs_page_base(SWITCH1), va1 = hs_page_base(SWITCH2);
    TEST_ASSERT("aliases have equal line offset and different current 256-set virtual index",
        ((va0 >> 6) & 255) != ((va1 >> 6) & 255) &&
        ((*fe_leaf(SWITCH1) >> 10) == (*fe_leaf(SWITCH2) >> 10)));
    goto_priv(PRIV_HS);
    bool correct = true;
    for (unsigned i = 0; i < 128; ++i) {
        if (!(i & 7))
            fe_fence_i();
        TEST_SETUP_EXCEPT();
        uint64_t value = fe_call(i & 1 ? va1 : va0, i, 0);
        correct &= !excpt.triggered && value == i + 9;
    }
    TEST_ASSERT("both virtual-set aliases return the same physical instruction result", correct);
    goto_priv(PRIV_M);
    hspt_init();
    TEST_END();
}

static pte_t fe_roots[2][3][512] __attribute__((aligned(4096)));
__attribute__((optimize("no-tree-vectorize")))
static void fe_root(unsigned context, uintptr_t va, uintptr_t pa)
{
    for (unsigned l = 0; l < 3; ++l)
        for (unsigned i = 0; i < 512; ++i)
            fe_roots[context][l][i] = 0;
    pte_t (*pt)[512] = fe_roots[context];
    pt[0][(MEM_BASE >> 30) & 511] = (MEM_BASE >> 2) | PTE_V | PTE_AD | PTE_RWX;
    pt[0][(va >> 30) & 511] = ((uintptr_t)pt[1] >> 2) | PTE_V;
    pt[1][(va >> 21) & 511] = ((uintptr_t)pt[2] >> 2) | PTE_V;
    pt[2][(va >> 12) & 511] = (pa >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

bool ai_micro_frontend_new_asid_root_no_sfence_fetch_corner()
{
    TEST_START();
    goto_priv(PRIV_M);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uintptr_t va = 0x100200000ULL;
    for (unsigned context = 0; context < 2; ++context) {
        uintptr_t pa = phys_page_base(context ? SWITCH2 : SWITCH1);
        fe_put32(pa, ((context ? 37U : 11U) << 20) | 0x00513U);
        fe_put32(pa + 4, 0x00008067U);
        fe_root(context, va, pa);
    }
    const uint64_t a = (8ULL << 60) | (1ULL << 44) | ((uintptr_t)fe_roots[0] >> 12);
    const uint64_t b = (8ULL << 60) | (2ULL << 44) | ((uintptr_t)fe_roots[1] >> 12);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    fence_iorw_iorw();
    sfence_vma(); /* Initial invalidation makes ASID 2 genuinely unused. */
    fe_fence_i();
    CSRW(CSR_SATP, a);
    goto_priv(PRIV_HS);
    bool correct = true;
    TEST_SETUP_EXCEPT();
    for (unsigned i = 0; i < 32; ++i)
        correct &= fe_call(va, 0, 0) == 11 && !excpt.triggered;
    const bool warm_ok = correct;
    CSRW(CSR_SATP, b);
    /* No sfence.vma/fence.i/goto_priv between SATP and the new fetch. */
    uint64_t value = fe_call(va, 0, 0);
    const bool new_ok = !excpt.triggered && value == 37 && CSRR(CSR_SATP) == b;
    CSRW(CSR_SATP, a);
    value = fe_call(va, 0, 0);
    const bool return_ok = !excpt.triggered && value == 11;
    goto_priv(PRIV_M);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    TEST_ASSERT("ASID 1 non-global mapping is warmed before the context switch", warm_ok);
    TEST_ASSERT("a new root and unused ASID select the new PA on the first fetch without sfence", new_ok);
    TEST_ASSERT("switching back preserves isolation of non-global ASID mappings", return_ok);
    TEST_END();
}

bool ai_micro_frontend_pbmt_nc_rvc_at_8byte_end_corner()
{
    TEST_START();
    /* Profile: MEM+NC in responding DRAM; Spike cannot gate uncached requests. */
    goto_priv(PRIV_M);
    const uint64_t env = CSRR(CSR_MENVCFG);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    uintptr_t pa = phys_page_base(SWITCH1), va = hs_page_base(SWITCH1);
    write16(pa + 6, 0x0505U); /* c.addi a0,1, complete at the 8-byte boundary */
    write16(pa + 8, 0x8082U); /* c.jr ra */
    fe_map(SWITCH1, pa, PTE_RWX | PTE_Pbmt1);
    fence_iorw_iorw();
    cbo_flush(pa);
    fence_iorw_iorw();
    sfence_vma();
    fe_fence_i();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = fe_call(va + 6, 41, 0);
    TEST_ASSERT("a complete PBMT.NC RVC at byte 6 executes once and returns",
        !excpt.triggered && value == 42 && ((va + 6) & 7) == 6);
    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, env);
    hspt_init();
    sfence_vma();
    TEST_END();
}

extern uint64_t fe_split_ret_probe(uint64_t seed);
extern const uint8_t fe_split_ret_pc[];
asm(
    ".pushsection .text.fe_split_ret,\"ax\",@progbits\n"
    ".option push\n.option norvc\n.option norelax\n"
    ".balign 32\n.global fe_split_ret_probe\n"
    "fe_split_ret_probe:\n"
    "mv t0, ra\njal ra, 1f\naddi a0,a0,2\njalr zero,0(t0)\n"
    ".balign 32\n1:\n.rept 13\n.2byte 0x0001\n.endr\n"
    "addi a0,a0,1\n"
    ".global fe_split_ret_pc\nfe_split_ret_pc:\n.4byte 0x00008067\n"
    ".option pop\n.popsection\n");

bool ai_micro_frontend_split_ret_ras_return_corner()
{
    TEST_START();
    goto_priv(PRIV_M);
    TEST_ASSERT("the 32-bit RET starts at offset 30 and is reached through a real JAL call",
        ((uintptr_t)fe_split_ret_pc & 31) == 30);
    bool correct = true;
    TEST_SETUP_EXCEPT();
    for (unsigned i = 0; i < 256; ++i)
        correct &= fe_split_ret_probe(i) == i + 3 && !excpt.triggered;
    TEST_ASSERT("split RET preserves the architectural return address and executes the caller suffix once", correct);
    TEST_END();
}

extern uint64_t fe_split_branch_probe(uint64_t condition);
extern const uint8_t fe_split_branch_pc[];
asm(
    ".pushsection .text.fe_split_branch,\"ax\",@progbits\n"
    ".option push\n.option norvc\n.option norelax\n"
    ".balign 32\n.global fe_split_branch_probe\nfe_split_branch_probe:\n"
    ".rept 15\n.2byte 0x0001\n.endr\n"
    ".global fe_split_branch_pc\nfe_split_branch_pc:\n"
    "beq a0,zero,1f\nli a0,7\nret\n1: li a0,19\nret\n"
    ".option pop\n.popsection\n");

bool ai_micro_frontend_split_branch_train_taken_then_not_taken_corner()
{
    TEST_START();
    goto_priv(PRIV_M);
    bool correct = true;
    TEST_SETUP_EXCEPT();
    for (unsigned phase = 0; phase < 32; ++phase) {
        for (unsigned train = 0; train < 64; ++train)
            correct &= fe_split_branch_probe(0) == 19;
        correct &= fe_split_branch_probe(1) == 7;
    }
    TEST_ASSERT("taken training followed by not-taken at the same split branch preserves fallthrough",
        correct && !excpt.triggered && ((uintptr_t)fe_split_branch_pc & 31) == 30);
    /* Keep the same block entry and offset while replacing a trained CFI.
     * This also exercises row 388; an observed IFU repair is still required. */
    uintptr_t base = (uintptr_t)fe_stream;
    for (unsigned off = 0; off < 30; off += 2)
        write16(base + off, 0x0001U);
    fe_put32(base + 34, 0x00450513U); /* fallthrough adds 4 */
    fe_put32(base + 38, 0x00008067U);
    fe_put32(base + 64, 0x01050513U); /* branch target adds 16 */
    fe_put32(base + 68, 0x00008067U);
    TEST_SETUP_EXCEPT();
    for (unsigned phase = 0; phase < 16; ++phase) {
        fe_put32(base + 30, 0x02000163U); /* beq zero,zero,+34 */
        fe_fence_i();
        for (unsigned train = 0; train < 64; ++train)
            correct &= fe_call(base, 41, 0) == 57;
        fe_put32(base + 30, 0x00150513U);
        fe_fence_i();
        correct &= fe_call(base, 41, 0) == 46;
    }
    TEST_ASSERT("fence.i followed by a branch-to-addi rewrite at offset 30 uses the sequential path",
        correct && !excpt.triggered);
    TEST_END();
}

bool ai_micro_frontend_split_lui_fake_cjr_cebreak_then_rvc_corner()
{
    TEST_START();
    goto_priv(PRIV_M);
    const uint16_t highhalves[] = {0x8082U, 0x9002U}; /* c.jr ra / c.ebreak */
    uintptr_t pc = (uintptr_t)fe_stream + 30;
    for (unsigned off = 0; off < 30; off += 2)
        write16((uintptr_t)fe_stream + off, 0x0001U);
    bool correct = true;
    for (unsigned i = 0; i < 2; ++i) {
        uint32_t inst = ((uint32_t)highhalves[i] << 16) | 0x0537U; /* lui a0 */
        fe_put32(pc, inst);
        write16(pc + 4, 0x0505U); /* real c.addi a0,1 in the next lane */
        write16(pc + 6, 0x8082U);
        fe_fence_i();
        TEST_SETUP_EXCEPT();
        uint64_t expected = (uint64_t)(int64_t)(int32_t)(inst & 0xfffff000U) + 1;
        for (unsigned round = 0; round < 32; ++round)
            correct &= fe_call((uintptr_t)fe_stream, 0, 0) == expected && !excpt.triggered;
    }
    TEST_ASSERT("c.jr/c.ebreak-looking high halves stay part of LUI while the following real RVC executes",
        correct && (pc & 31) == 30);
    TEST_END();
}

extern uint64_t fe_mshr_redirect_probe(uintptr_t target, uint64_t branch, uint64_t seed);
asm(
    ".pushsection .text.fe_mshr_redirect,\"ax\",@progbits\n"
    ".option push\n.option norvc\n.option norelax\n"
    ".balign 64\n.global fe_mshr_redirect_probe\nfe_mshr_redirect_probe:\n"
    "addi sp,sp,-16\nsd ra,8(sp)\nmv t2,a0\nmv a0,a2\nli t3,1\n"
    ".rept 8\ndivu a1,a1,t3\n.endr\n"
    "beqz a1,1f\naddi a0,a0,2\nj 2f\n1: addi a0,a0,1\n"
    "2: jalr ra,0(t2)\nld ra,8(sp)\naddi sp,sp,16\nret\n"
    ".option pop\n.popsection\n");

bool ai_micro_frontend_mshr_redirect_same_line_revisit_corner()
{
    TEST_START();
    goto_priv(PRIV_M);
    uintptr_t target = (uintptr_t)fe_stream;
    fe_put32(target, 0x00950513U);
    fe_put32(target + 4, 0x00008067U);
    fe_fence_i();
    bool correct = true;
    TEST_SETUP_EXCEPT();
    for (unsigned phase = 0; phase < 128; ++phase) {
        for (unsigned train = 0; train < 32; ++train)
            correct &= fe_mshr_redirect_probe(target, 0, phase) == phase + 10;
        /* A predicted taken path can request target while DIV delays resolution.
         * The real not-taken path redirects and requests the very same line.
         * The IT responder must hold its grant until the second request. */
        fe_fence_i();
        correct &= fe_mshr_redirect_probe(target, 1, phase) == phase + 11;
    }
    TEST_ASSERT("redirected paths revisiting the same cold line preserve the exact chosen-path result",
        correct && !excpt.triggered);
    TEST_END();
}
