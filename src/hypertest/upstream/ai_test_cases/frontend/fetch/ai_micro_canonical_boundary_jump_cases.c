#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_CANONICAL_SV39_MODE        8ULL
#define AI_CANONICAL_SV48_MODE        9ULL
#define AI_CANONICAL_SV39_SOURCE      0x0000003ffffffffcULL
#define AI_CANONICAL_SV39_TARGET      0x0000004000000000ULL
#define AI_CANONICAL_SV48_SOURCE      0x00007ffffffffffcULL
#define AI_CANONICAL_SV48_TARGET      0x0000800000000000ULL
#define AI_CANONICAL_JAL_ZERO_PLUS4   0x0040006fU
#define AI_CANONICAL_SV48_BRANCH_SRC  0x00007ffffffffff8ULL
#define AI_CANONICAL_BEQ_A0_ZERO_PLUS8 0x00050463U
#define AI_CANONICAL_JR_T1            0x00030067U

static pte_t ai_canonical_sv39_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_canonical_sv39_high_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_canonical_sv39_high_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_canonical_sv48_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_canonical_sv48_low_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_canonical_sv48_high_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_canonical_sv48_high_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_canonical_sv48_high_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

static inline void ai_canonical_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0ULL;
    }
}

static inline pte_t ai_canonical_table_pte(const pte_t *next)
{
    return PTE_V | (((uintptr_t)next) >> 2U);
}

static inline pte_t ai_canonical_leaf_pte(uintptr_t paddr)
{
    return ((paddr & PAGE_ADDR_MSK) >> 2U) |
        PTE_V | PTE_AD | PTE_RWX;
}

__attribute__((noinline, optimize("no-tree-vectorize",
    "no-tree-loop-distribute-patterns")))
static void ai_canonical_install_page_tables(uintptr_t code_paddr)
{
    ai_canonical_zero_pt(ai_canonical_sv39_root);
    ai_canonical_zero_pt(ai_canonical_sv39_high_l1);
    ai_canonical_zero_pt(ai_canonical_sv39_high_l0);
    ai_canonical_zero_pt(ai_canonical_sv48_root);
    ai_canonical_zero_pt(ai_canonical_sv48_low_l2);
    ai_canonical_zero_pt(ai_canonical_sv48_high_l2);
    ai_canonical_zero_pt(ai_canonical_sv48_high_l1);
    ai_canonical_zero_pt(ai_canonical_sv48_high_l0);

    for (unsigned i = 0; i < 4U; ++i) {
        ai_canonical_sv39_root[i] =
            ai_canonical_leaf_pte((uintptr_t)i << 30U);
    }
    ai_canonical_sv39_root[255] =
        ai_canonical_table_pte(ai_canonical_sv39_high_l1);
    ai_canonical_sv39_high_l1[511] =
        ai_canonical_table_pte(ai_canonical_sv39_high_l0);
    ai_canonical_sv39_high_l0[511] =
        ai_canonical_leaf_pte(code_paddr);

    ai_canonical_sv48_root[0] =
        ai_canonical_table_pte(ai_canonical_sv48_low_l2);
    for (unsigned i = 0; i < 4U; ++i) {
        ai_canonical_sv48_low_l2[i] =
            ai_canonical_leaf_pte((uintptr_t)i << 30U);
    }
    ai_canonical_sv48_root[255] =
        ai_canonical_table_pte(ai_canonical_sv48_high_l2);
    ai_canonical_sv48_high_l2[511] =
        ai_canonical_table_pte(ai_canonical_sv48_high_l1);
    ai_canonical_sv48_high_l1[511] =
        ai_canonical_table_pte(ai_canonical_sv48_high_l0);
    ai_canonical_sv48_high_l0[511] =
        ai_canonical_leaf_pte(code_paddr);
}

__attribute__((noinline))
static void ai_canonical_execute_boundary_jump(uintptr_t source)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lla t0, 1f\n\t"
        "sd t0, %0\n\t"
        "jr %1\n\t"
        "1:\n\t"
        ".option pop\n\t"
        : "+m"(excpt.fault_inst)
        : "r"(source)
        : "t0", "memory");
}

__attribute__((noinline))
static void ai_canonical_execute_boundary_branch(uintptr_t source,
    uintptr_t take)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lla t0, 1f\n\t"
        "sd t0, %0\n\t"
        "lla t1, 1f\n\t"
        "mv a0, %2\n\t"
        "jr %1\n\t"
        "1:\n\t"
        ".option pop\n\t"
        : "+m"(excpt.fault_inst)
        : "r"(source), "r"(take)
        : "t0", "t1", "a0", "memory");
}

bool ai_micro_hs_sv39_sv48_boundary_jal_to_noncanonical_ipf_corner()
{
    TEST_START();

    const uintptr_t code_page = phys_page_base(SWITCH1);
    const uintptr_t code_tail = code_page + PAGE_SIZE - sizeof(uint32_t);
    const uint64_t sv39_satp = (AI_CANONICAL_SV39_MODE << 60U) |
        (((uintptr_t)ai_canonical_sv39_root) >> 12U);
    const uint64_t sv48_satp = (AI_CANONICAL_SV48_MODE << 60U) |
        (((uintptr_t)ai_canonical_sv48_root) >> 12U);

    goto_priv(PRIV_M);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_satp = CSRR(CSR_SATP);

    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_IPF);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();
    ai_canonical_install_page_tables(code_page);
    write32(code_tail, AI_CANONICAL_JAL_ZERO_PLUS4);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");

    TEST_ASSERT("Sv39 and Sv48 roots map each last positive-canonical page to the JAL-at-page-tail template",
        read32(code_tail) == AI_CANONICAL_JAL_ZERO_PLUS4 &&
        (ai_canonical_sv39_high_l0[511] &
            (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (ai_canonical_sv48_high_l0[511] &
            (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX));

    CSRW(CSR_SATP, sv39_satp);
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_canonical_execute_boundary_jump(AI_CANONICAL_SV39_SOURCE);
    const bool sv39_triggered = excpt.triggered;
    const uint64_t sv39_cause = excpt.cause;
    const uint64_t sv39_epc = excpt.epc;
    const uint64_t sv39_tval = excpt.tval;

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, sv48_satp);
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_canonical_execute_boundary_jump(AI_CANONICAL_SV48_SOURCE);
    const bool sv48_triggered = excpt.triggered;
    const uint64_t sv48_cause = excpt.cause;
    const uint64_t sv48_epc = excpt.epc;
    const uint64_t sv48_tval = excpt.tval;

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("Sv39 JAL +4 from 0x0000003ffffffffc faults on the noncanonical 0x0000004000000000 target",
        sv39_triggered == true && sv39_cause == CAUSE_IPF &&
        sv39_epc == AI_CANONICAL_SV39_TARGET &&
        sv39_tval == AI_CANONICAL_SV39_TARGET,
        "cause=0x%llx epc=0x%llx tval=0x%llx",
        (unsigned long long)sv39_cause,
        (unsigned long long)sv39_epc,
        (unsigned long long)sv39_tval);
    TEST_ASSERT("Sv48 JAL +4 from 0x00007ffffffffffc faults on the noncanonical 0x0000800000000000 target",
        sv48_triggered == true && sv48_cause == CAUSE_IPF &&
        sv48_epc == AI_CANONICAL_SV48_TARGET &&
        sv48_tval == AI_CANONICAL_SV48_TARGET,
        "cause=0x%llx epc=0x%llx tval=0x%llx",
        (unsigned long long)sv48_cause,
        (unsigned long long)sv48_epc,
        (unsigned long long)sv48_tval);

    TEST_END();
}

bool ai_micro_hs_sv48_boundary_taken_branch_to_noncanonical_backend_ipf_corner()
{
    TEST_START();

    const uintptr_t code_page = phys_page_base(SWITCH1);
    const uintptr_t code_branch = code_page + PAGE_SIZE - 2U * sizeof(uint32_t);
    const uint64_t sv48_satp = (AI_CANONICAL_SV48_MODE << 60U) |
        (((uintptr_t)ai_canonical_sv48_root) >> 12U);

    goto_priv(PRIV_M);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_satp = CSRR(CSR_SATP);

    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_IPF);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();
    ai_canonical_install_page_tables(code_page);
    write32(code_branch, AI_CANONICAL_BEQ_A0_ZERO_PLUS8);
    write32(code_branch + sizeof(uint32_t), AI_CANONICAL_JR_T1);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");

    TEST_ASSERT("the last eight bytes of the mapped Sv48 positive-canonical page contain beq a0,zero,+8 followed by jr t1",
        read32(code_branch) == AI_CANONICAL_BEQ_A0_ZERO_PLUS8 &&
        read32(code_branch + sizeof(uint32_t)) == AI_CANONICAL_JR_T1);

    CSRW(CSR_SATP, sv48_satp);
    sfence_vma();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    ai_canonical_execute_boundary_branch(
        AI_CANONICAL_SV48_BRANCH_SRC, 1ULL);
    const bool training_triggered = excpt.triggered;

    TEST_SETUP_EXCEPT();
    ai_canonical_execute_boundary_branch(
        AI_CANONICAL_SV48_BRANCH_SRC, 0ULL);
    const bool taken_triggered = excpt.triggered;
    const uint64_t taken_cause = excpt.cause;
    const uint64_t taken_epc = excpt.epc;
    const uint64_t taken_tval = excpt.tval;

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("the first execution makes the boundary branch not taken and squashes any predicted noncanonical-target fetch fault",
        training_triggered == false);
    TEST_ASSERT("the second execution takes the same Sv48 branch to 0x0000800000000000 and reports instruction page fault at the branch target",
        taken_triggered == true && taken_cause == CAUSE_IPF &&
        taken_epc == AI_CANONICAL_SV48_TARGET &&
        taken_tval == AI_CANONICAL_SV48_TARGET,
        "cause=0x%llx epc=0x%llx tval=0x%llx",
        (unsigned long long)taken_cause,
        (unsigned long long)taken_epc,
        (unsigned long long)taken_tval);

    TEST_END();
}
