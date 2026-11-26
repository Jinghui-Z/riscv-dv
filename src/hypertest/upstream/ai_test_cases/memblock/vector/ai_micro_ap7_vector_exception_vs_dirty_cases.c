#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_AP7_VS_OFF   (9U)
#define AI_AP7_VS_MASK  (3ULL << AI_AP7_VS_OFF)
#define AI_AP7_VS_CLEAN (2ULL << AI_AP7_VS_OFF)
#define AI_AP7_VS_DIRTY (3ULL << AI_AP7_VS_OFF)

#define AI_AP7_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_AP7_MCONTROL6_LOAD     0x1ULL

static inline pte_t *ai_ap7_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ap7_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ai_ap7_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_ap7_enable_vector_dirty(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MSTATUS, AI_AP7_VS_MASK);
    CSRS(CSR_MSTATUS, AI_AP7_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_ap7_force_vs_clean(void)
{
    CSRC(CSR_MSTATUS, AI_AP7_VS_MASK);
    CSRS(CSR_MSTATUS, AI_AP7_VS_CLEAN);
}

static inline uint64_t ai_ap7_sstatus_vs(void)
{
    return CSRR(CSR_SSTATUS) & AI_AP7_VS_MASK;
}

static inline void ai_ap7_config_e64(unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m1, ta, ma\n\t"
        ".option pop\n\t"
        :
        : [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap7_config_e64_m2(unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        ".option pop\n\t"
        :
        : [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap7_seed_v6_e64(const uint64_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m1, ta, ma\n\t"
        "vle64.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap7_vle64_v6_no_vset(const uint64_t *src)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vle64.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src)
        : "memory");
}

static inline void ai_ap7_vse64_v6_no_vset(uint64_t *dst)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vse64.v v6, (%[dst])\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst)
        : "memory");
}

static inline void ai_ap7_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0" ::: "memory");
    CSRW(CSR_TDATA2, 0ULL);
}

static inline void ai_ap7_arm_mcontrol6_slot(size_t slot, uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0" ::: "memory");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, AI_AP7_MCONTROL6_MSU_BASE | access_bits);
}

static inline uint64_t ai_ap7_enable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
    return saved_mstatus;
}

static inline void ai_ap7_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline uint64_t ai_ap7_scalar_rob_pressure(uint64_t seed)
{
    uint64_t acc = seed;

    for (unsigned i = 0; i < 384U; i++) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "addi %[acc], %[acc], 17\n\t"
            "xori %[acc], %[acc], 0x5a\n\t"
            "addi %[acc], %[acc], -9\n\t"
            ".option pop\n\t"
            : [acc] "+r"(acc)
            :
            : "memory");
    }

    return acc;
}

bool ai_micro_ap7_vector_store_late_element_page_fault_dirty_vs_corner()
{
    TEST_START();

    const enum test_page source_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x180U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x180U;
    const uintptr_t target_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t target_paddr = phys_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t source0 = 0x1020304050607080ULL;
    const uint64_t source1 = 0x8877665544332211ULL;
    const uint64_t old0 = 0xa5a5a5a55a5a5a5aULL;
    const uint64_t old1 = 0x0badf00d12345678ULL;
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    goto_priv(PRIV_M);
    write64(source_paddr, source0);
    write64(source_paddr + sizeof(uint64_t), source1);
    write64(target_paddr, old0);
    write64(fault_paddr, old1);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap7_set_hs_leaf(source_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap7_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap7_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap7_seed_v6_e64((const uint64_t *)source_vaddr, 2U);
    TEST_ASSERT("AP-7 setup can stage a two-element e64 vector store source before forcing VS back to Clean",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap7_vse64_v6_no_vset((uint64_t *)target_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_ap7_sstatus_vs();
    TEST_ASSERT("AP-7 vector store late element page fault writes vstart and dirties VS through the ROB exception path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL &&
        observed_vs == AI_AP7_VS_DIRTY,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart, observed_vs
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-7 vector store fault commits the prefix element but leaves the faulting element backing word unchanged",
        read64(target_paddr) == source0 &&
        read64(fault_paddr) == old1,
        "prefix=0x%lx expected=0x%lx fault_word=0x%lx old=0x%lx",
        read64(target_paddr), source0, read64(fault_paddr), old1
    );

    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap7_vector_load_fault_after_address_trigger_redirect_dirty_vs_manual_smoke_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t load_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t load_paddr = phys_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t low_seed = 0x7111222233334444ULL;
    const uint64_t high_seed = 0x7555666677778888ULL;
    uint64_t saved_mstatus = 0ULL;
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    goto_priv(PRIV_M);
    write64(load_paddr, low_seed);
    write64(fault_paddr, high_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap7_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap7_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    ai_ap7_config_e64(2U);

    ai_ap7_clear_mcontrol6_slot(0);
    ai_ap7_arm_mcontrol6_slot(0, load_vaddr, AI_AP7_MCONTROL6_LOAD);
    saved_mstatus = ai_ap7_enable_breakpoint_delivery();

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap7_vle64_v6_no_vset((const uint64_t *)load_vaddr);
    TEST_ASSERT("AP-7 manual smoke first forces a vector-load address trigger redirect before the later real vector exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP,
        "cause=0x%lx tval=0x%lx epc=0x%lx",
        excpt.cause, excpt.tval, excpt.epc
    );

    ai_ap7_clear_mcontrol6_slot(0);
    ai_ap7_restore_machine_status(saved_mstatus);
    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    ai_ap7_config_e64(2U);

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap7_vle64_v6_no_vset((const uint64_t *)load_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_ap7_sstatus_vs();
    TEST_ASSERT("AP-7 manual smoke clears the trigger and then verifies the following vector load page fault still writes vstart and dirties VS",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL &&
        observed_vs == AI_AP7_VS_DIRTY,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart, observed_vs
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-7 trigger-redirect manual smoke leaves both vector-load backing words unchanged",
        read64(load_paddr) == low_seed &&
        read64(fault_paddr) == high_seed
    );

    ai_ap7_clear_mcontrol6_slot(0);
    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap7_vector_load_fault_after_scalar_rob_pressure_dirty_vs_manual_smoke_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t load_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t load_paddr = phys_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t low_seed = 0x7a0123456789abcdULL;
    const uint64_t high_seed = 0x7b0fedcba9876543ULL;
    uint64_t pressure_signature = 0ULL;
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    goto_priv(PRIV_M);
    write64(load_paddr, low_seed);
    write64(fault_paddr, high_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap7_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap7_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    ai_ap7_config_e64(2U);

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_force_vs_clean();

    goto_priv(PRIV_HS);
    pressure_signature = ai_ap7_scalar_rob_pressure(0x7c55aa55ULL);
    TEST_SETUP_EXCEPT();
    ai_ap7_vle64_v6_no_vset((const uint64_t *)load_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_ap7_sstatus_vs();
    TEST_ASSERT("AP-7 manual smoke advances ROB indices with scalar pressure before the vector exception and still observes vstart plus VS Dirty together",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL &&
        observed_vs == AI_AP7_VS_DIRTY &&
        pressure_signature != 0ULL,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu vs=0x%lx pressure=0x%lx",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart,
        observed_vs, pressure_signature
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-7 ROB-pressure manual smoke is read-only for the vector load backing words",
        read64(load_paddr) == low_seed &&
        read64(fault_paddr) == high_seed
    );

    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap7_vector_load_late_element_page_fault_dirty_vs_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t load_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t load_paddr = phys_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t low_seed = 0x1122334455667788ULL;
    const uint64_t high_seed = 0x99aabbccddeeff00ULL;
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    goto_priv(PRIV_M);
    write64(load_paddr, low_seed);
    write64(fault_paddr, high_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap7_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap7_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    ai_ap7_config_e64(2U);

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap7_vle64_v6_no_vset((const uint64_t *)load_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_ap7_sstatus_vs();
    TEST_ASSERT("AP-7 vector load late element page fault writes vstart and dirties VS through the ROB exception path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL &&
        observed_vs == AI_AP7_VS_DIRTY,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart, observed_vs
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-7 vector load dirty-path probe does not modify the faulting backing word",
        read64(load_paddr) == low_seed &&
        read64(fault_paddr) == high_seed,
        "low=0x%lx expected_low=0x%lx high=0x%lx expected_high=0x%lx",
        read64(load_paddr), low_seed, read64(fault_paddr), high_seed
    );

    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap7_scalar_load_page_fault_keeps_vs_clean_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI);
    volatile uint64_t sink = 0ULL;
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sink = ld(bad_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_ap7_sstatus_vs();
    TEST_ASSERT("AP-7 scalar load page fault is the negative control: it reports the scalar exception without dirtying VS or changing vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr &&
        observed_vstart == 0ULL &&
        observed_vs == AI_AP7_VS_CLEAN,
        "sink=0x%lx cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu vs=0x%lx",
        (uint64_t)sink, excpt.cause, excpt.tval, bad_vaddr,
        observed_vstart, observed_vs
    );

    goto_priv(PRIV_M);
    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap7_repeated_vector_load_faults_dirty_vs_after_clean_reset_corner()
{
    TEST_START();

    const enum test_page first_low_page = SWITCH1;
    const enum test_page first_high_page = SWITCH2;
    const enum test_page second_low_page = VSRWX_GRWX;
    const enum test_page second_high_page = VSRWX_GRW;
    const uintptr_t first_vaddr = hs_page_base(first_low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t first_low_paddr = phys_page_base(first_low_page) +
        PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t first_fault_vaddr = hs_page_base(first_high_page);
    const uintptr_t first_fault_paddr = phys_page_base(first_high_page);
    const uintptr_t second_vaddr = hs_page_base(second_low_page) + PAGE_SIZE -
        2U * sizeof(uint64_t);
    const uintptr_t second_low0_paddr = phys_page_base(second_low_page) +
        PAGE_SIZE - 2U * sizeof(uint64_t);
    const uintptr_t second_low1_paddr = phys_page_base(second_low_page) +
        PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t second_fault_vaddr = hs_page_base(second_high_page);
    const uintptr_t second_fault_paddr = phys_page_base(second_high_page);
    uint64_t first_vstart = 0ULL;
    uint64_t first_vs = 0ULL;
    uint64_t second_vstart = 0ULL;
    uint64_t second_vs = 0ULL;

    goto_priv(PRIV_M);
    write64(first_low_paddr, 0x0102030405060708ULL);
    write64(first_fault_paddr, 0x1112131415161718ULL);
    write64(second_low0_paddr, 0x2122232425262728ULL);
    write64(second_low1_paddr, 0x3132333435363738ULL);
    write64(second_fault_paddr, 0x4142434445464748ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap7_set_hs_leaf(first_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap7_set_hs_leaf(first_high_page, PTE_V | PTE_AD | PTE_X);
    ai_ap7_set_hs_leaf(second_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap7_set_hs_leaf(second_high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    ai_ap7_config_e64(2U);

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap7_vle64_v6_no_vset((const uint64_t *)first_vaddr);
    first_vstart = CSRR(CSR_VSTART);
    first_vs = ai_ap7_sstatus_vs();
    TEST_ASSERT("AP-7 first vector load fault dirties VS after software set it to Clean",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_fault_vaddr &&
        first_vstart == 1ULL &&
        first_vs == AI_AP7_VS_DIRTY,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, first_fault_vaddr, first_vstart, first_vs
    );

    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    ai_ap7_config_e64_m2(4U);

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap7_vle64_v6_no_vset((const uint64_t *)second_vaddr);
    second_vstart = CSRR(CSR_VSTART);
    second_vs = ai_ap7_sstatus_vs();
    TEST_ASSERT("AP-7 second vector load fault also dirties VS after a fresh software Clean reset, proving the exception dirty pulse is not one-shot",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_fault_vaddr &&
        second_vstart == 2ULL &&
        second_vs == AI_AP7_VS_DIRTY,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu vs=0x%lx first_vstart=%lu first_vs=0x%lx",
        excpt.cause, excpt.tval, second_fault_vaddr, second_vstart,
        second_vs, first_vstart, first_vs
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-7 repeated vector load dirty-path probes are read-only for all backing words",
        read64(first_low_paddr) == 0x0102030405060708ULL &&
        read64(first_fault_paddr) == 0x1112131415161718ULL &&
        read64(second_low0_paddr) == 0x2122232425262728ULL &&
        read64(second_low1_paddr) == 0x3132333435363738ULL &&
        read64(second_fault_paddr) == 0x4142434445464748ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    ai_ap7_enable_vector_dirty();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
