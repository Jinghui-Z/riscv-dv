#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_LINKNAN_BAD_PA_IFETCH_VA 0x0000000000400002ULL
#define AI_LINKNAN_BAD_PA_IFETCH_PA 0x0000000037000000ULL
#define AI_LINKNAN_SV39_MODE 8ULL
#define AI_LINKNAN_CODE_ROOT_INDEX 2U
#define AI_LINKNAN_TARGET_ROOT_INDEX 0U
#define AI_LINKNAN_TARGET_L1_INDEX \
    ((AI_LINKNAN_BAD_PA_IFETCH_VA >> 21U) & 0x1ffU)

static pte_t ai_linknan_bad_pa_ifetch_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_linknan_bad_pa_ifetch_target_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

static inline void ai_linknan_bad_pa_ifetch_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0ULL;
    }
}

static inline pte_t ai_linknan_bad_pa_ifetch_table_pte(const pte_t *next)
{
    return PTE_V | (((uintptr_t)next) >> 2U);
}

static inline pte_t ai_linknan_bad_pa_ifetch_leaf_pte(uintptr_t paddr)
{
    return ((paddr & PAGE_ADDR_MSK) >> 2U) | PTE_V | PTE_AD | PTE_RWX;
}

__attribute__((noinline, optimize("no-tree-vectorize",
    "no-tree-loop-distribute-patterns")))
static void ai_linknan_bad_pa_ifetch_install_page_tables(void)
{
    ai_linknan_bad_pa_ifetch_zero_pt(ai_linknan_bad_pa_ifetch_root);
    ai_linknan_bad_pa_ifetch_zero_pt(ai_linknan_bad_pa_ifetch_target_l1);

    /* Keep the test harness and trap-return continuation identity-mapped. */
    ai_linknan_bad_pa_ifetch_root[AI_LINKNAN_CODE_ROOT_INDEX] =
        ai_linknan_bad_pa_ifetch_leaf_pte((uintptr_t)MEM_BASE);

    /* VA 0x400002 is in the second 2 MiB leaf of root entry zero. */
    ai_linknan_bad_pa_ifetch_root[AI_LINKNAN_TARGET_ROOT_INDEX] =
        ai_linknan_bad_pa_ifetch_table_pte(
            ai_linknan_bad_pa_ifetch_target_l1);
    ai_linknan_bad_pa_ifetch_target_l1[AI_LINKNAN_TARGET_L1_INDEX] =
        ai_linknan_bad_pa_ifetch_leaf_pte(AI_LINKNAN_BAD_PA_IFETCH_PA);
}

bool ai_micro_linknan_sv39_mret_bad_pa_ifetch_flash_alias_access_fault_corner()
{
    TEST_START();

    const uint64_t satp = (AI_LINKNAN_SV39_MODE << 60U) |
        (((uintptr_t)ai_linknan_bad_pa_ifetch_root) >> 12U);

    goto_priv(PRIV_M);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_IPF);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();
    ai_linknan_bad_pa_ifetch_install_page_tables();

    TEST_ASSERT("the Sv39 root contains an executable 2 MiB leaf for VA 0x400002 and an identity map for the harness",
        (ai_linknan_bad_pa_ifetch_root[AI_LINKNAN_CODE_ROOT_INDEX] &
            (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (ai_linknan_bad_pa_ifetch_root[AI_LINKNAN_TARGET_ROOT_INDEX] &
            PTE_V) != 0ULL &&
        (ai_linknan_bad_pa_ifetch_target_l1[AI_LINKNAN_TARGET_L1_INDEX] &
            (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX));

    CSRW(CSR_SATP, satp);
    sfence_vma();

    /* Enter S-mode through the harness so its software privilege tracker stays
     * synchronized with the hardware while the target fetch faults. */
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(AI_LINKNAN_BAD_PA_IFETCH_VA);

    /* The exception handler returns to the instruction after TEST_EXEC_EXCEPT.
     * Re-enter M-mode before inspecting state and disabling translation. */
    goto_priv(PRIV_M);

    const bool observed_triggered = excpt.triggered;
    const uint64_t observed_priv = excpt.priv;
    const uint64_t observed_cause = excpt.cause;
    const uint64_t observed_epc = excpt.epc;
    const uint64_t observed_tval = excpt.tval;

    CSRW(CSR_SATP, 0ULL);
    sfence_vma();

    TEST_ASSERT("mret into S-mode must report instruction access fault for the unmapped physical fetch address",
        observed_triggered == true &&
        observed_priv == PRIV_M &&
        observed_cause == CAUSE_IAF &&
        observed_epc == AI_LINKNAN_BAD_PA_IFETCH_VA &&
        observed_tval == AI_LINKNAN_BAD_PA_IFETCH_VA,
        "priv=%llu cause=0x%llx epc=0x%llx tval=0x%llx",
        (unsigned long long)observed_priv,
        (unsigned long long)observed_cause,
        (unsigned long long)observed_epc,
        (unsigned long long)observed_tval);

    TEST_END();
}
