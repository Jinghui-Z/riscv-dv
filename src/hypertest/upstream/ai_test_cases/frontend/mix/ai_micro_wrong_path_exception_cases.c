#include <rvh_test.h>
#include <page_tables.h>
#include <stdbool.h>
#include <stdint.h>

#define AI_WP_PTE_PPN_OVERFLOW_BIT (1ULL << 46U)
#define AI_WP_MARKER_SEED           0x4e6f47726f737450ULL
#define AI_WP_RIGHT_SEED            0x5269676874506174ULL
#define AI_WP_FETCH_RIGHT_VALUE     0x5aULL
#define AI_WP_DELAY_SEED            0xfedcba9876543210ULL

extern uint64_t ecall_args[2];

typedef void (*ai_wp_probe_fn)(uintptr_t cond_addr,
    uintptr_t target_addr, uintptr_t wrong_marker_addr,
    uintptr_t right_marker_addr, uint64_t wrong_value,
    uint64_t right_value);

/*
 * Every generated probe owns one static conditional branch. The probe is
 * called exactly once with cond=0, so the branch's first prediction is taken
 * while its architectural direction is not-taken. The branch waits on DIVU;
 * this gives the predicted target enough time to enter the pipeline.
 */
#define AI_WP_DECLARE_PROBE(name) \
    extern void name(uintptr_t, uintptr_t, uintptr_t, uintptr_t, \
        uint64_t, uint64_t); \
    extern void name##_target(uintptr_t, uintptr_t, uintptr_t, uintptr_t, \
        uint64_t, uint64_t)

#define AI_WP_DEFINE_PROBE(name, target_body) \
    asm( \
        ".pushsection .text\n" \
        ".balign 64\n" \
        ".global " #name "\n" \
        ".type " #name ", @function\n" \
        #name ":\n" \
        "  .option push\n" \
        "  .option norvc\n" \
        "  ld t0, 0(a0)\n" \
        "  li t1, 97\n" \
        "  divu t2, a4, t1\n" \
        "  and t0, t0, t2\n" \
        "  bnez t0, " #name "_target\n" \
        "  sd a5, 0(a3)\n" \
        "  ret\n" \
        "  .option pop\n" \
        ".size " #name ", .-" #name "\n" \
        ".balign 4\n" \
        ".global " #name "_target\n" \
        ".type " #name "_target, @function\n" \
        #name "_target:\n" \
        "  .option push\n" \
        "  .option norvc\n" \
        target_body \
        "  sd a4, 0(a2)\n" \
        "  ret\n" \
        "  .option pop\n" \
        ".size " #name "_target, .-" #name "_target\n" \
        ".popsection\n")

AI_WP_DECLARE_PROBE(ai_wp_m_illegal_probe);
AI_WP_DECLARE_PROBE(ai_wp_m_ebreak_probe);
AI_WP_DECLARE_PROBE(ai_wp_m_cebreak_probe);
AI_WP_DECLARE_PROBE(ai_wp_m_ecall_probe);
AI_WP_DECLARE_PROBE(ai_wp_hs_csr_probe);
AI_WP_DECLARE_PROBE(ai_wp_hs_sfence_probe);
AI_WP_DECLARE_PROBE(ai_wp_hs_mret_probe);
AI_WP_DECLARE_PROBE(ai_wp_hs_ecall_probe);
AI_WP_DECLARE_PROBE(ai_wp_load_pf_probe);
AI_WP_DECLARE_PROBE(ai_wp_store_pf_probe);
AI_WP_DECLARE_PROBE(ai_wp_load_af_probe);
AI_WP_DECLARE_PROBE(ai_wp_store_af_probe);

AI_WP_DEFINE_PROBE(ai_wp_m_illegal_probe,
    "  .4byte 0xffffffff\n");
AI_WP_DEFINE_PROBE(ai_wp_m_ebreak_probe,
    "  ebreak\n");
AI_WP_DEFINE_PROBE(ai_wp_m_cebreak_probe,
    "  .2byte 0x9002\n"
    "  .2byte 0x0001\n");
AI_WP_DEFINE_PROBE(ai_wp_m_ecall_probe,
    "  ecall\n");
AI_WP_DEFINE_PROBE(ai_wp_hs_csr_probe,
    "  csrr t3, mstatus\n");
AI_WP_DEFINE_PROBE(ai_wp_hs_sfence_probe,
    "  sfence.vma x0, x0\n");
AI_WP_DEFINE_PROBE(ai_wp_hs_mret_probe,
    "  mret\n");
AI_WP_DEFINE_PROBE(ai_wp_hs_ecall_probe,
    "  ecall\n");
AI_WP_DEFINE_PROBE(ai_wp_load_pf_probe,
    "  ld a4, 0(a1)\n");
AI_WP_DEFINE_PROBE(ai_wp_store_pf_probe,
    "  sd a4, 0(a1)\n");
AI_WP_DEFINE_PROBE(ai_wp_load_af_probe,
    "  ld a4, 0(a1)\n");
AI_WP_DEFINE_PROBE(ai_wp_store_af_probe,
    "  sd a4, 0(a1)\n");

struct ai_wp_scenario {
    ai_wp_probe_fn probe;
    ai_wp_probe_fn target;
    uint64_t cause;
};

static inline pte_t *ai_wp_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_wp_set_hs_leaf(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    *ai_wp_hs_leaf_entry(page) = (paddr >> 2U) | flags;
}

static inline void ai_wp_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}

bool ai_micro_m_first_taken_prediction_wrong_path_illegal_ebreak_ecall_squash_corner()
{
    TEST_START();

    static const struct ai_wp_scenario scenarios[] = {
        { ai_wp_m_illegal_probe, ai_wp_m_illegal_probe_target,
            CAUSE_ILLEGAL_INSTRUCTION },
        { ai_wp_m_ebreak_probe, ai_wp_m_ebreak_probe_target, CAUSE_BKP },
        { ai_wp_m_cebreak_probe, ai_wp_m_cebreak_probe_target, CAUSE_BKP },
        { ai_wp_m_ecall_probe, ai_wp_m_ecall_probe_target,
            CAUSE_MACHINE_ECALL },
    };
    volatile uint64_t cond = 0ULL;
    volatile uint64_t wrong_marker = AI_WP_MARKER_SEED;
    volatile uint64_t right_marker = AI_WP_RIGHT_SEED;
    const uint64_t saved_ecall_arg0 = ecall_args[0];
    const uint64_t saved_ecall_arg1 = ecall_args[1];

    goto_priv(PRIV_M);
    for (unsigned index = 0;
         index < sizeof(scenarios) / sizeof(scenarios[0]); ++index) {
        const uint64_t wrong_value = 0x1100000000000000ULL | index;
        const uint64_t right_value = 0x1200000000000000ULL | index;

        if (index == 3U) {
            ecall_args[0] = 0ULL;
            ecall_args[1] = 0ULL;
        }

        wrong_marker = AI_WP_MARKER_SEED;
        right_marker = AI_WP_RIGHT_SEED;
        TEST_SETUP_EXCEPT();
        scenarios[index].probe((uintptr_t)&cond, 0ULL,
            (uintptr_t)&wrong_marker, (uintptr_t)&right_marker,
            wrong_value, right_value);
        TEST_ASSERT("a first-seen M-mode branch is architecturally not-taken and squashes its predicted-taken synchronous exception",
            excpt.triggered == false &&
            wrong_marker == AI_WP_MARKER_SEED &&
            right_marker == right_value,
            "index=%u wrong=0x%llx right=0x%llx",
            index, (unsigned long long)wrong_marker,
            (unsigned long long)right_marker);

        wrong_marker = AI_WP_MARKER_SEED;
        right_marker = AI_WP_RIGHT_SEED;
        TEST_SETUP_EXCEPT();
        scenarios[index].target((uintptr_t)&cond, 0ULL,
            (uintptr_t)&wrong_marker, (uintptr_t)&right_marker,
            wrong_value, right_value);
        TEST_ASSERT("the M-mode predicted target independently raises the intended exception when executed architecturally",
            excpt.triggered == true &&
            excpt.cause == scenarios[index].cause &&
            wrong_marker == wrong_value &&
            right_marker == AI_WP_RIGHT_SEED,
            "index=%u cause=0x%llx expected=0x%llx",
            index, (unsigned long long)excpt.cause,
            (unsigned long long)scenarios[index].cause);
    }

    ecall_args[0] = saved_ecall_arg0;
    ecall_args[1] = saved_ecall_arg1;
    TEST_END();
}

bool ai_micro_hs_first_taken_prediction_wrong_path_privileged_exception_squash_corner()
{
    TEST_START();

    static const struct ai_wp_scenario scenarios[] = {
        { ai_wp_hs_csr_probe, ai_wp_hs_csr_probe_target,
            CAUSE_ILLEGAL_INSTRUCTION },
        { ai_wp_hs_sfence_probe, ai_wp_hs_sfence_probe_target,
            CAUSE_ILLEGAL_INSTRUCTION },
        { ai_wp_hs_ecall_probe, ai_wp_hs_ecall_probe_target,
            CAUSE_SUPERVISOR_ECALL },
        { ai_wp_hs_mret_probe, ai_wp_hs_mret_probe_target,
            CAUSE_ILLEGAL_INSTRUCTION },
    };
    volatile uint64_t cond = 0ULL;
    volatile uint64_t wrong_marker = AI_WP_MARKER_SEED;
    volatile uint64_t right_marker = AI_WP_RIGHT_SEED;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_ecall_arg0 = ecall_args[0];
    const uint64_t saved_ecall_arg1 = ecall_args[1];

    for (unsigned index = 0;
         index < sizeof(scenarios) / sizeof(scenarios[0]); ++index) {
        goto_priv(PRIV_M);
        if (index == 1U) {
            CSRS(CSR_MSTATUS, MSTATUS_TVM);
        } else {
            CSRC(CSR_MSTATUS, MSTATUS_TVM);
        }
        goto_priv(PRIV_HS);
        if (index == 2U) {
            ecall_args[0] = 0ULL;
            ecall_args[1] = 0ULL;
        }
        const uint64_t wrong_value = 0x2100000000000000ULL | index;
        const uint64_t right_value = 0x2200000000000000ULL | index;

        wrong_marker = AI_WP_MARKER_SEED;
        right_marker = AI_WP_RIGHT_SEED;
        TEST_SETUP_EXCEPT();
        scenarios[index].probe((uintptr_t)&cond, 0ULL,
            (uintptr_t)&wrong_marker, (uintptr_t)&right_marker,
            wrong_value, right_value);
        TEST_ASSERT("a first-seen HS branch is architecturally not-taken and squashes its predicted-taken privileged exception",
            excpt.triggered == false &&
            wrong_marker == AI_WP_MARKER_SEED &&
            right_marker == right_value,
            "index=%u wrong=0x%llx right=0x%llx",
            index, (unsigned long long)wrong_marker,
            (unsigned long long)right_marker);

        wrong_marker = AI_WP_MARKER_SEED;
        right_marker = AI_WP_RIGHT_SEED;
        TEST_SETUP_EXCEPT();
        scenarios[index].target((uintptr_t)&cond, 0ULL,
            (uintptr_t)&wrong_marker, (uintptr_t)&right_marker,
            wrong_value, right_value);
        TEST_ASSERT("the HS predicted target independently raises its intended privileged exception when executed architecturally",
            excpt.triggered == true &&
            excpt.cause == scenarios[index].cause &&
            wrong_marker == wrong_value &&
            right_marker == AI_WP_RIGHT_SEED,
            "index=%u cause=0x%llx expected=0x%llx",
            index, (unsigned long long)excpt.cause,
            (unsigned long long)scenarios[index].cause);
    }

    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
    ecall_args[0] = saved_ecall_arg0;
    ecall_args[1] = saved_ecall_arg1;
    TEST_END();
}

#define AI_WP_DYN_LD_T0_0_A0      0x00053283U
#define AI_WP_DYN_LI_T1_97         0x06100313U
#define AI_WP_DYN_DIVU_T2_A1_T1    0x0265d3b3U
#define AI_WP_DYN_AND_T0_T0_T2     0x0072f2b3U
#define AI_WP_DYN_LI_A0_0X5A       0x05a00513U
#define AI_WP_DYN_LI_A0_7          0x00700513U
#define AI_WP_DYN_RET              0x00008067U
#define AI_WP_DYN_BRANCH_OFFSET    16U

static uint32_t ai_wp_encode_bne_t0(uintptr_t branch_pc,
    uintptr_t target_pc)
{
    const int64_t delta = (int64_t)target_pc - (int64_t)branch_pc;
    const uint32_t immediate = (uint32_t)delta;

    return (((immediate >> 12U) & 0x1U) << 31U) |
        (((immediate >> 5U) & 0x3fU) << 25U) |
        (5U << 15U) |
        (1U << 12U) |
        (((immediate >> 1U) & 0xfU) << 8U) |
        (((immediate >> 11U) & 0x1U) << 7U) |
        0x63U;
}

static void ai_wp_emit_fetch_probe(uintptr_t probe_paddr,
    uintptr_t probe_vaddr, uintptr_t target_vaddr)
{
    write32(probe_paddr + 0U, AI_WP_DYN_LD_T0_0_A0);
    write32(probe_paddr + 4U, AI_WP_DYN_LI_T1_97);
    write32(probe_paddr + 8U, AI_WP_DYN_DIVU_T2_A1_T1);
    write32(probe_paddr + 12U, AI_WP_DYN_AND_T0_T0_T2);
    write32(probe_paddr + AI_WP_DYN_BRANCH_OFFSET,
        ai_wp_encode_bne_t0(probe_vaddr + AI_WP_DYN_BRANCH_OFFSET,
            target_vaddr));
    write32(probe_paddr + 20U, AI_WP_DYN_LI_A0_0X5A);
    write32(probe_paddr + 24U, AI_WP_DYN_RET);
}

bool ai_micro_hs_first_taken_prediction_wrong_path_fetch_ipf_iaf_squash_corner()
{
    TEST_START();

    const enum test_page exec_page = SWITCH1;
    const enum test_page target_page = SWITCH2;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t ipf_probe_offset = 0x400U;
    const uintptr_t iaf_probe_offset = 0x800U;
    const uintptr_t target_offset = 0x100U;
    const uintptr_t cond_offset = 0x200U;
    const uintptr_t exec_paddr = phys_page_base(exec_page);
    const uintptr_t exec_vaddr = hs_page_base(exec_page);
    const uintptr_t target_paddr = phys_page_base(target_page) + target_offset;
    const uintptr_t target_vaddr = hs_page_base(target_page) + target_offset;
    const uintptr_t cond_paddr = phys_page_base(marker_page) + cond_offset;
    const uintptr_t cond_vaddr = hs_page_base(marker_page) + cond_offset;
    typedef uint64_t (*ai_wp_fetch_probe_fn)(uintptr_t, uint64_t);
    typedef uint64_t (*ai_wp_fetch_target_fn)(void);
    ai_wp_fetch_probe_fn ipf_probe = (ai_wp_fetch_probe_fn)(
        exec_vaddr + ipf_probe_offset);
    ai_wp_fetch_probe_fn iaf_probe = (ai_wp_fetch_probe_fn)(
        exec_vaddr + iaf_probe_offset);
    ai_wp_fetch_target_fn target = (ai_wp_fetch_target_fn)target_vaddr;
    uint64_t observed = 0ULL;

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    write64(cond_paddr, 0ULL);
    ai_wp_emit_fetch_probe(exec_paddr + ipf_probe_offset,
        exec_vaddr + ipf_probe_offset, target_vaddr);
    ai_wp_emit_fetch_probe(exec_paddr + iaf_probe_offset,
        exec_vaddr + iaf_probe_offset, target_vaddr);
    write32(target_paddr + 0U, AI_WP_DYN_LI_A0_7);
    write32(target_paddr + 4U, AI_WP_DYN_RET);
    ai_wp_set_hs_leaf(exec_page, phys_page_base(exec_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_wp_set_hs_leaf(target_page, phys_page_base(target_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();
    goto_priv(PRIV_HS);
    ai_wp_fence_i();

    TEST_SETUP_EXCEPT();
    observed = ipf_probe(cond_vaddr, AI_WP_DELAY_SEED);
    TEST_ASSERT("the first execution of the IPF probe resolves not-taken and hides the predicted-taken no-X fetch fault",
        excpt.triggered == false &&
        observed == AI_WP_FETCH_RIGHT_VALUE);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(target_vaddr);
    TEST_ASSERT("the no-X predicted target independently raises instruction page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == target_vaddr,
        "cause=0x%llx tval=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval);

    goto_priv(PRIV_M);
    ai_wp_set_hs_leaf(target_page, phys_page_base(target_page),
        PTE_V | PTE_AD | PTE_RWX);
    *ai_wp_hs_leaf_entry(target_page) |= AI_WP_PTE_PPN_OVERFLOW_BIT;
    sfence_vma();
    goto_priv(PRIV_HS);
    ai_wp_fence_i();

    TEST_SETUP_EXCEPT();
    observed = iaf_probe(cond_vaddr, AI_WP_DELAY_SEED ^ 0x13579bdfULL);
    TEST_ASSERT("the first execution of the IAF probe resolves not-taken and hides the predicted-taken bad-PA fetch fault",
        excpt.triggered == false &&
        observed == AI_WP_FETCH_RIGHT_VALUE);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(target_vaddr);
    TEST_ASSERT("the bad-PA predicted target independently raises instruction access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == target_vaddr,
        "cause=0x%llx tval=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval);

    goto_priv(PRIV_M);
    ai_wp_set_hs_leaf(target_page, phys_page_base(target_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    goto_priv(PRIV_HS);
    ai_wp_fence_i();
    TEST_SETUP_EXCEPT();
    observed = target();
    TEST_ASSERT("restoring the target leaf permits the generated target code to execute",
        excpt.triggered == false && observed == 7ULL);

    hspt_init();
    sfence_vma();
    TEST_END();
}

static void ai_wp_prepare_data_markers(uintptr_t cond_paddr,
    uintptr_t wrong_marker_paddr, uintptr_t right_marker_paddr)
{
    write64(cond_paddr, 0ULL);
    write64(wrong_marker_paddr, AI_WP_MARKER_SEED);
    write64(right_marker_paddr, AI_WP_RIGHT_SEED);
}

bool ai_micro_hs_first_taken_prediction_wrong_path_load_store_pf_af_squash_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t target_offset = 0x500U;
    const uintptr_t cond_offset = 0x600U;
    const uintptr_t wrong_marker_offset = 0x640U;
    const uintptr_t right_marker_offset = 0x680U;
    const uintptr_t target_paddr = phys_page_base(target_page) + target_offset;
    const uintptr_t target_vaddr = hs_page_base(target_page) + target_offset;
    const uintptr_t target_guard_before = target_paddr - sizeof(uint64_t);
    const uintptr_t target_guard_after = target_paddr + sizeof(uint64_t);
    const uintptr_t cond_paddr = phys_page_base(marker_page) + cond_offset;
    const uintptr_t cond_vaddr = hs_page_base(marker_page) + cond_offset;
    const uintptr_t wrong_marker_paddr = phys_page_base(marker_page) +
        wrong_marker_offset;
    const uintptr_t wrong_marker_vaddr = hs_page_base(marker_page) +
        wrong_marker_offset;
    const uintptr_t right_marker_paddr = phys_page_base(marker_page) +
        right_marker_offset;
    const uintptr_t right_marker_vaddr = hs_page_base(marker_page) +
        right_marker_offset;
    const uint64_t target_seed = 0x1021324354657687ULL;
    const uint64_t guard_before_seed = 0x89abcdef01234567ULL;
    const uint64_t guard_after_seed = 0x76543210fedcba98ULL;
    static const struct ai_wp_scenario scenarios[] = {
        { ai_wp_load_pf_probe, ai_wp_load_pf_probe_target, CAUSE_LPF },
        { ai_wp_store_pf_probe, ai_wp_store_pf_probe_target, CAUSE_SPF },
        { ai_wp_load_af_probe, ai_wp_load_af_probe_target, CAUSE_LAF },
        { ai_wp_store_af_probe, ai_wp_store_af_probe_target, CAUSE_SAF },
    };

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    write64(target_paddr, target_seed);
    write64(target_guard_before, guard_before_seed);
    write64(target_guard_after, guard_after_seed);

    for (unsigned index = 0;
         index < sizeof(scenarios) / sizeof(scenarios[0]); ++index) {
        const bool access_fault = index >= 2U;
        const bool store_probe = (index & 1U) != 0U;
        const uint64_t wrong_value = 0x4100000000000000ULL | index;
        const uint64_t right_value = 0x4200000000000000ULL | index;

        ai_wp_prepare_data_markers(cond_paddr, wrong_marker_paddr,
            right_marker_paddr);
        if (access_fault) {
            ai_wp_set_hs_leaf(target_page, phys_page_base(target_page),
                PTE_V | PTE_AD | PTE_RWX);
            *ai_wp_hs_leaf_entry(target_page) |=
                AI_WP_PTE_PPN_OVERFLOW_BIT;
        } else {
            ai_wp_set_hs_leaf(target_page, phys_page_base(target_page),
                PTE_V | PTE_AD | PTE_X);
        }
        sfence_vma();
        goto_priv(PRIV_HS);

        TEST_SETUP_EXCEPT();
        scenarios[index].probe(cond_vaddr, target_vaddr,
            wrong_marker_vaddr, right_marker_vaddr,
            wrong_value, right_value);
        const bool wrong_path_clean =
            excpt.triggered == false &&
            ld(wrong_marker_vaddr) == AI_WP_MARKER_SEED &&
            ld(right_marker_vaddr) == right_value;

        goto_priv(PRIV_M);
        const bool target_image_clean =
            read64(target_paddr) == target_seed &&
            read64(target_guard_before) == guard_before_seed &&
            read64(target_guard_after) == guard_after_seed;
        goto_priv(PRIV_HS);
        TEST_ASSERT("a first-seen not-taken branch squashes its predicted-taken data fault and all younger memory side effects",
            wrong_path_clean && target_image_clean,
            "index=%u store=%u access_fault=%u",
            index, store_probe, access_fault);

        TEST_SETUP_EXCEPT();
        scenarios[index].target(cond_vaddr, target_vaddr,
            wrong_marker_vaddr, right_marker_vaddr,
            wrong_value, right_value);
        TEST_ASSERT("the same predicted target independently raises the intended load or store fault on the architectural path",
            excpt.triggered == true &&
            excpt.cause == scenarios[index].cause &&
            excpt.tval == target_vaddr,
            "index=%u cause=0x%llx expected=0x%llx tval=0x%llx",
            index, (unsigned long long)excpt.cause,
            (unsigned long long)scenarios[index].cause,
            (unsigned long long)excpt.tval);

        goto_priv(PRIV_M);
        TEST_ASSERT("faulting store controls and wrong-path stores preserve the target and both adjacent guards",
            read64(target_paddr) == target_seed &&
            read64(target_guard_before) == guard_before_seed &&
            read64(target_guard_after) == guard_after_seed,
            "index=%u", index);
    }

    ai_wp_set_hs_leaf(target_page, phys_page_base(target_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    TEST_END();
}
