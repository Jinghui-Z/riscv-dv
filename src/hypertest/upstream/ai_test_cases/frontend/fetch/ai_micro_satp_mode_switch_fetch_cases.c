#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_SATP_SWITCH_SV39_MODE 8ULL
#define AI_SATP_SWITCH_SV48_MODE 9ULL
#define AI_SATP_SWITCH_HIGH_VA 0xffff800000000000ULL
#define AI_SATP_SWITCH_NEXT_PC (AI_SATP_SWITCH_HIGH_VA + 4ULL)

static pte_t ai_satp_switch_sv48_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_satp_switch_sv48_low_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_satp_switch_sv48_high_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_satp_switch_sv48_high_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_satp_switch_sv48_high_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_satp_switch_sv39_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static volatile uint64_t ai_satp_switch_side_effect_marker;

static inline void ai_satp_switch_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0;
    }
}

static inline pte_t ai_satp_switch_table_pte(const pte_t *next)
{
    return PTE_V | (((uintptr_t)next) >> 2);
}

static inline pte_t ai_satp_switch_leaf_pte(uintptr_t paddr)
{
    return ((paddr & PAGE_ADDR_MSK) >> 2) |
        (PTE_V | PTE_AD | PTE_RWX);
}

__attribute__((noinline, optimize("no-tree-vectorize",
    "no-tree-loop-distribute-patterns")))
static void ai_satp_switch_install_roots(uintptr_t code_paddr)
{
    ai_satp_switch_zero_pt(ai_satp_switch_sv48_root);
    ai_satp_switch_zero_pt(ai_satp_switch_sv48_low_l2);
    ai_satp_switch_zero_pt(ai_satp_switch_sv48_high_l2);
    ai_satp_switch_zero_pt(ai_satp_switch_sv48_high_l1);
    ai_satp_switch_zero_pt(ai_satp_switch_sv48_high_l0);
    ai_satp_switch_zero_pt(ai_satp_switch_sv39_root);

    ai_satp_switch_sv48_root[0] =
        ai_satp_switch_table_pte(ai_satp_switch_sv48_low_l2);
    for (unsigned i = 0; i < 4; ++i) {
        ai_satp_switch_sv48_low_l2[i] =
            ai_satp_switch_leaf_pte((uintptr_t)i << 30);
        ai_satp_switch_sv39_root[i] =
            ai_satp_switch_leaf_pte((uintptr_t)i << 30);
    }

    ai_satp_switch_sv48_root[256] =
        ai_satp_switch_table_pte(ai_satp_switch_sv48_high_l2);
    ai_satp_switch_sv48_high_l2[0] =
        ai_satp_switch_table_pte(ai_satp_switch_sv48_high_l1);
    ai_satp_switch_sv48_high_l1[0] =
        ai_satp_switch_table_pte(ai_satp_switch_sv48_high_l0);
    ai_satp_switch_sv48_high_l0[0] =
        ai_satp_switch_leaf_pte(code_paddr);
}

__attribute__((noinline))
static void ai_satp_switch_execute_high_sequence(uintptr_t target,
    uint64_t sv39_satp, uintptr_t marker)
{
    asm volatile(
        "la t0, 1f\n\t"
        "sd t0, %0\n\t"
        "la ra, 2f\n\t"
        "mv a0, %1\n\t"
        "mv a1, %2\n\t"
        "1:\n\t"
        "jr %3\n\t"
        "2:\n\t"
        : "+m"(excpt.fault_inst)
        : "r"(sv39_satp), "r"(marker), "r"(target)
        : "t0", "ra", "a0", "a1", "memory"
    );
}

bool ai_micro_hs_satp_sv48_to_sv39_next_pc_noncanonical_ifetch_page_fault_corner()
{
    TEST_START();

    const uintptr_t code_paddr = phys_page_base(SWITCH1);
    const uint64_t sv48_satp =
        (AI_SATP_SWITCH_SV48_MODE << 60) |
        (((uintptr_t)ai_satp_switch_sv48_root) >> 12);
    const uint64_t sv39_satp =
        (AI_SATP_SWITCH_SV39_MODE << 60) |
        (((uintptr_t)ai_satp_switch_sv39_root) >> 12);
    const uint64_t marker_seed = 0x5a7f39f400000004ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_IPF);
    CSRW(CSR_SATP, 0);
    sfence_vma();

    ai_satp_switch_install_roots(code_paddr);
    write32(code_paddr + 0, 0x18051073U); /* csrw satp, a0 */
    write32(code_paddr + 4, 0x0005b023U); /* sd zero, 0(a1) */
    write32(code_paddr + 8, 0x00008067U); /* ret */
    ai_satp_switch_side_effect_marker = marker_seed;
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");

    TEST_ASSERT("the Sv48 root maps the high canonical target page and the Sv39 root maps the low recovery environment",
        (sv48_satp >> 60) == AI_SATP_SWITCH_SV48_MODE &&
        (sv39_satp >> 60) == AI_SATP_SWITCH_SV39_MODE &&
        (ai_satp_switch_sv48_high_l0[0] &
            (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        ((ai_satp_switch_sv48_high_l0[0] << 2) & PAGE_ADDR_MSK) ==
            (code_paddr & PAGE_ADDR_MSK));

    CSRW(CSR_SATP, sv48_satp);
    sfence_vma();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    ai_satp_switch_execute_high_sequence(AI_SATP_SWITCH_HIGH_VA,
        sv39_satp, (uintptr_t)&ai_satp_switch_side_effect_marker);

    const bool observed_triggered = excpt.triggered;
    const uint64_t observed_cause = excpt.cause;
    const uint64_t observed_epc = excpt.epc;
    const uint64_t observed_tval = excpt.tval;
    const uint64_t observed_satp = CSRR(CSR_SATP);
    const uint64_t observed_marker = ai_satp_switch_side_effect_marker;

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, 0);
    sfence_vma();

    TEST_ASSERT("after the high-address csrw satp commits Sv48-to-Sv39, the adjacent noncanonical PC raises instruction page fault at PC+4",
        observed_triggered == true &&
        observed_cause == CAUSE_IPF &&
        observed_epc == AI_SATP_SWITCH_NEXT_PC &&
        observed_tval == AI_SATP_SWITCH_NEXT_PC,
        "cause=0x%llx epc=0x%llx tval=0x%llx",
        (unsigned long long)observed_cause,
        (unsigned long long)observed_epc,
        (unsigned long long)observed_tval);

    TEST_ASSERT("the fault is attributed to the first post-SATP-flush fetch under Sv39 and suppresses the stale-Sv48 PC+4 store",
        observed_satp == sv39_satp &&
        observed_marker == marker_seed,
        "satp=0x%llx expected=0x%llx marker=0x%llx",
        (unsigned long long)observed_satp,
        (unsigned long long)sv39_satp,
        (unsigned long long)observed_marker);

    TEST_END();
}
