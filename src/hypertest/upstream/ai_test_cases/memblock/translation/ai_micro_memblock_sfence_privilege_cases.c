#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MEMBLOCK_SFENCE_PRIV_ASID 0x15a5ULL
#define AI_MEMBLOCK_SFENCE_PRIV_ASID_NEXT 0x25a5ULL
#define AI_MEMBLOCK_SFENCE_PRIV_MSTATUS_MPP_MASK (3ULL << 11)
#define AI_MEMBLOCK_SFENCE_PRIV_MSTATUS_MPP_S (1ULL << 11)
#define AI_MEMBLOCK_SFENCE_PRIV_MSTATUS_MPP_U 0ULL
#define AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_SHIFT 60
#define AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_SHIFT 44
#define AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_MASK \
    (0xfULL << AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_SHIFT)
#define AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_MASK \
    (0xffffULL << AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_SHIFT)
#define AI_MEMBLOCK_SFENCE_PRIV_SATP_PPN_MASK \
    ((1ULL << AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_SHIFT) - 1ULL)
#define AI_MEMBLOCK_SFENCE_PRIV_CSRR_INST(csr) \
    (((uint32_t)(csr) << 20) | (2U << 12) | (5U << 7) | 0x73U)
#define AI_MEMBLOCK_SFENCE_PRIV_CSRW_INST(csr) \
    (((uint32_t)(csr) << 20) | (5U << 15) | (1U << 12) | 0x73U)
#define AI_MEMBLOCK_SFENCE_PRIV_SFENCE_X0_X0_INST 0x12000073U

#define AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline bool ai_memblock_sfence_priv_check_illegal_with_inst(
    uint32_t expected_inst)
{
    return excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        (uint32_t)excpt.tval == expected_inst &&
        excpt.epc != 0;
}

static inline void ai_memblock_sfence_priv_fixed_sfence_x0_x0(void)
{
    asm volatile(".word 0x12000073\n\t" ::: "memory");
}

static inline void ai_memblock_sfence_priv_fixed_csrr_satp(void)
{
    asm volatile(".word 0x180022f3\n\t" ::: "t0", "memory");
}

static inline void ai_memblock_sfence_priv_fixed_csrw_satp(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        ".word 0x18029073\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline void ai_memblock_sfence_priv_sinval_vma(void)
{
    asm volatile(
        ".insn r 0x73, 0x0, 0x0B, x0, x0, x0\n\t"
        ::: "memory");
}

static inline void ai_memblock_sfence_priv_sfence_w_inval(void)
{
    asm volatile(
        ".word 0x18000073\n\t"
        ::: "memory");
}

static inline void ai_memblock_sfence_priv_sfence_inval_ir(void)
{
    asm volatile(
        ".word 0x18100073\n\t"
        ::: "memory");
}

static inline pte_t *ai_memblock_sfence_priv_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_sfence_priv_set_global(enum test_page page,
    bool enabled)
{
    goto_priv(PRIV_M);
    if (enabled) {
        *ai_memblock_sfence_priv_leaf_entry(page) |= PTE_G;
    } else {
        *ai_memblock_sfence_priv_leaf_entry(page) &= ~PTE_G;
    }
}

static inline uint64_t ai_memblock_sfence_priv_set_asid(uint64_t asid)
{
    uint64_t satp = CSRR(CSR_SATP);
    satp &= ~(0xffffULL << 44);
    satp |= ((asid & 0xffffULL) << 44);
    CSRW(CSR_SATP, satp);
    return satp;
}

static inline uint64_t ai_memblock_sfence_priv_get_asid(uint64_t satp)
{
    return (satp >> 44) & 0xffffULL;
}

static inline bool ai_memblock_sfence_priv_sfence_no_trap(uintptr_t vaddr,
    uint64_t asid)
{
    TEST_SETUP_EXCEPT();
    sfence_vma_rs(vaddr, asid);
    return excpt.triggered == false;
}

static inline bool ai_memblock_sfence_priv_sfence_illegal(uintptr_t vaddr,
    uint64_t asid)
{
    TEST_SETUP_EXCEPT();
    sfence_vma_rs(vaddr, asid);
    return excpt.triggered == true && excpt.cause == CAUSE_ILI;
}

static inline bool ai_memblock_sfence_priv_satp_read_no_trap(uint64_t *satp_value)
{
    TEST_SETUP_EXCEPT();
    *satp_value = CSRR(CSR_SATP);
    return excpt.triggered == false;
}

static inline bool ai_memblock_sfence_priv_satp_write_no_trap(uint64_t satp_value)
{
    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, satp_value);
    return excpt.triggered == false;
}

static inline bool ai_memblock_sfence_priv_satp_read_illegal(void)
{
    TEST_SETUP_EXCEPT();
    (void)CSRR(CSR_SATP);
    return excpt.triggered == true && excpt.cause == CAUSE_ILI;
}

static inline bool ai_memblock_sfence_priv_satp_write_illegal(uint64_t satp_value)
{
    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, satp_value);
    return excpt.triggered == true && excpt.cause == CAUSE_ILI;
}

static inline uint64_t ai_memblock_sfence_priv_make_mprv_mpp_status(
    uint64_t base_mstatus, uint64_t mpp)
{
    return (base_mstatus &
        ~(MSTATUS_MPRV | AI_MEMBLOCK_SFENCE_PRIV_MSTATUS_MPP_MASK)) |
        MSTATUS_MPRV |
        (mpp & AI_MEMBLOCK_SFENCE_PRIV_MSTATUS_MPP_MASK);
}

static inline uint64_t ai_memblock_sfence_priv_mprv_read_satp(
    uint64_t probe_mstatus, uint64_t restore_mstatus)
{
    uint64_t satp_value;

    asm volatile(
        "csrw mstatus, %[probe]\n\t"
        "csrr %[satp], satp\n\t"
        "csrw mstatus, %[restore]\n\t"
        : [satp] "=r"(satp_value)
        : [probe] "r"(probe_mstatus),
          [restore] "r"(restore_mstatus)
        : "memory");

    return satp_value;
}

static inline void ai_memblock_sfence_priv_mprv_write_satp(
    uint64_t probe_mstatus, uint64_t satp_value, uint64_t restore_mstatus)
{
    asm volatile(
        "csrw mstatus, %[probe]\n\t"
        "csrw satp, %[satp]\n\t"
        "csrw mstatus, %[restore]\n\t"
        :
        : [probe] "r"(probe_mstatus),
          [satp] "r"(satp_value),
          [restore] "r"(restore_mstatus)
        : "memory");
}

static inline void ai_memblock_sfence_priv_mprv_sfence(
    uint64_t probe_mstatus, uint64_t restore_mstatus)
{
    asm volatile(
        "csrw mstatus, %[probe]\n\t"
        "sfence.vma x0, x0\n\t"
        "csrw mstatus, %[restore]\n\t"
        :
        : [probe] "r"(probe_mstatus),
          [restore] "r"(restore_mstatus)
        : "memory");
}

static inline void ai_memblock_sfence_priv_mprv_sfence_rs(
    uint64_t probe_mstatus, uint64_t restore_mstatus, uintptr_t vaddr,
    uint64_t asid)
{
    if (vaddr == (uintptr_t)-1 && asid == (uint64_t)-1) {
        asm volatile(
            "csrw mstatus, %[probe]\n\t"
            "sfence.vma x0, x0\n\t"
            "csrw mstatus, %[restore]\n\t"
            :
            : [probe] "r"(probe_mstatus),
              [restore] "r"(restore_mstatus)
            : "memory");
    } else if (vaddr == (uintptr_t)-1) {
        asm volatile(
            "csrw mstatus, %[probe]\n\t"
            "sfence.vma x0, %[asid]\n\t"
            "csrw mstatus, %[restore]\n\t"
            :
            : [probe] "r"(probe_mstatus),
              [asid] "r"(asid),
              [restore] "r"(restore_mstatus)
            : "memory");
    } else if (asid == (uint64_t)-1) {
        asm volatile(
            "csrw mstatus, %[probe]\n\t"
            "sfence.vma %[vaddr], x0\n\t"
            "csrw mstatus, %[restore]\n\t"
            :
            : [probe] "r"(probe_mstatus),
              [vaddr] "r"(vaddr),
              [restore] "r"(restore_mstatus)
            : "memory");
    } else {
        asm volatile(
            "csrw mstatus, %[probe]\n\t"
            "sfence.vma %[vaddr], %[asid]\n\t"
            "csrw mstatus, %[restore]\n\t"
            :
            : [probe] "r"(probe_mstatus),
              [vaddr] "r"(vaddr),
              [asid] "r"(asid),
              [restore] "r"(restore_mstatus)
            : "memory");
    }
}

static inline bool ai_memblock_sfence_priv_mprv_sfence_rs_no_trap(
    uint64_t probe_mstatus, uint64_t restore_mstatus, uintptr_t vaddr,
    uint64_t asid)
{
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_mprv_sfence_rs(probe_mstatus, restore_mstatus,
        vaddr, asid);
    return excpt.triggered == false;
}

static inline uint64_t ai_memblock_sfence_priv_prepare_env(uint64_t asid)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    hspt_init();
    uint64_t satp = ai_memblock_sfence_priv_set_asid(asid);
    sfence_vma();
    return satp;
}

bool ai_micro_memblock_sfence_tvm_illegal_preserves_translation_state_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page guard_page = VSRW_GRWX;
    const uintptr_t target_vaddr = hs_page_base(target_page);
    const uintptr_t guard_vaddr = hs_page_base(guard_page);
    const uintptr_t target_paddr = phys_page_base(target_page);
    const uintptr_t guard_paddr = phys_page_base(guard_page);
    const uint64_t target_initial = 0x1021324354657687ULL;
    const uint64_t guard_initial = 0x8877665544332211ULL;
    const uint64_t target_tvm_store = 0x13579bdf2468ace0ULL;
    const uint64_t target_updated = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRW(CSR_SATP, 0ULL);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    sfence_vma();
    TEST_ASSERT("HU-mode sfence.vma reports illegal instruction before any translated HS environment is installed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_M);
    uint64_t satp_before = ai_memblock_sfence_priv_prepare_env(
        AI_MEMBLOCK_SFENCE_PRIV_ASID);
    uint64_t satp_probe = satp_before & ~(0xffffULL << 44);
    satp_probe |= ((AI_MEMBLOCK_SFENCE_PRIV_ASID_NEXT & 0xffffULL) << 44);
    ai_memblock_sfence_priv_set_global(target_page, false);
    ai_memblock_sfence_priv_set_global(guard_page, true);

    goto_priv(PRIV_M);
    write64(target_paddr, target_initial);
    write64(guard_paddr, guard_initial);
    sfence_vma();

    uint64_t target_pte_before = *ai_memblock_sfence_priv_leaf_entry(target_page);
    uint64_t guard_pte_before = *ai_memblock_sfence_priv_leaf_entry(guard_page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t target_warm = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the HS target translation is readable before the TVM-illegal sfence.vma probe",
        target_warm == target_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_warm = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the HS global guard translation is readable before the TVM-illegal sfence.vma probe",
        guard_warm == guard_initial
    );

    uint64_t hs_satp_read = 0;
    TEST_ASSERT("HS-mode csrr satp retires normally before mstatus.TVM is set",
        ai_memblock_sfence_priv_satp_read_no_trap(&hs_satp_read) &&
        hs_satp_read == satp_before
    );

    TEST_ASSERT("HS-mode csrw satp ASID switch retires normally before mstatus.TVM is set",
        ai_memblock_sfence_priv_satp_write_no_trap(satp_probe)
    );

    hs_satp_read = 0;
    TEST_ASSERT("HS-mode csrr satp observes the TVM=0 ASID switch",
        ai_memblock_sfence_priv_satp_read_no_trap(&hs_satp_read) &&
        hs_satp_read == satp_probe
    );

    TEST_ASSERT("HS-mode csrw satp restores the original ASID before the TVM=0 sfence.vma matrix",
        ai_memblock_sfence_priv_satp_write_no_trap(satp_before)
    );

    TEST_ASSERT("HS-mode sfence.vma x0,x0 retires normally before mstatus.TVM is set",
        ai_memblock_sfence_priv_sfence_no_trap((uintptr_t)-1, (uint64_t)-1)
    );

    TEST_ASSERT("HS-mode sfence.vma vaddr,x0 retires normally before mstatus.TVM is set",
        ai_memblock_sfence_priv_sfence_no_trap(target_vaddr, (uint64_t)-1)
    );

    TEST_ASSERT("HS-mode sfence.vma x0,asid retires normally before mstatus.TVM is set",
        ai_memblock_sfence_priv_sfence_no_trap((uintptr_t)-1,
            AI_MEMBLOCK_SFENCE_PRIV_ASID)
    );

    TEST_ASSERT("HS-mode sfence.vma vaddr,asid retires normally before mstatus.TVM is set",
        ai_memblock_sfence_priv_sfence_no_trap(target_vaddr,
            AI_MEMBLOCK_SFENCE_PRIV_ASID)
    );

    TEST_SETUP_EXCEPT();
    target_warm = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after the TVM=0 legal sfence.vma quadrant matrix, the target mapping still returns its original payload",
        target_warm == target_initial
    );

    TEST_SETUP_EXCEPT();
    guard_warm = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after the TVM=0 legal sfence.vma quadrant matrix, the global guard mapping still returns its original payload",
        guard_warm == guard_initial
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_TVM);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_fixed_csrr_satp();
    TEST_ASSERT("HS-mode csrr satp reports illegal instruction when mstatus.TVM=1",
        ai_memblock_sfence_priv_check_illegal_with_inst(
            AI_MEMBLOCK_SFENCE_PRIV_CSRR_INST(CSR_SATP))
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_fixed_csrw_satp(satp_probe);
    TEST_ASSERT("HS-mode csrw satp reports illegal instruction when mstatus.TVM=1",
        ai_memblock_sfence_priv_check_illegal_with_inst(
            AI_MEMBLOCK_SFENCE_PRIV_CSRW_INST(CSR_SATP))
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_fixed_sfence_x0_x0();
    TEST_ASSERT("HS-mode sfence.vma x0,x0 reports illegal instruction when mstatus.TVM=1",
        ai_memblock_sfence_priv_check_illegal_with_inst(
            AI_MEMBLOCK_SFENCE_PRIV_SFENCE_X0_X0_INST)
    );

    TEST_ASSERT("HS-mode sfence.vma vaddr,x0 reports illegal instruction when mstatus.TVM=1",
        ai_memblock_sfence_priv_sfence_illegal(target_vaddr, (uint64_t)-1)
    );

    TEST_ASSERT("HS-mode sfence.vma x0,asid reports illegal instruction when mstatus.TVM=1",
        ai_memblock_sfence_priv_sfence_illegal((uintptr_t)-1,
            AI_MEMBLOCK_SFENCE_PRIV_ASID)
    );

    TEST_ASSERT("HS-mode sfence.vma vaddr,asid reports illegal instruction when mstatus.TVM=1",
        ai_memblock_sfence_priv_sfence_illegal(target_vaddr,
            AI_MEMBLOCK_SFENCE_PRIV_ASID)
    );

    goto_priv(PRIV_M);
    uint64_t satp_after_hs_illegal = CSRR(CSR_SATP);
    TEST_ASSERT("the TVM-blocked HS satp and sfence.vma probes leave satp, non-global/global PTE flags, and backing memory unchanged",
        satp_after_hs_illegal == satp_before &&
        *ai_memblock_sfence_priv_leaf_entry(target_page) == target_pte_before &&
        *ai_memblock_sfence_priv_leaf_entry(guard_page) == guard_pte_before &&
        read64(target_paddr) == target_initial &&
        read64(guard_paddr) == guard_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t target_after_hs_illegal = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after the TVM-blocked HS sfence.vma, the original HS target translation still returns the preserved payload",
        target_after_hs_illegal == target_initial
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, target_tvm_store);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("mstatus.TVM=1 does not block an ordinary HS translated store after illegal satp and sfence.vma probes",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t target_after_tvm_store = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("mstatus.TVM=1 does not block an ordinary HS translated load after the TVM-period store",
        target_after_tvm_store == target_tvm_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("ordinary TVM=1 HS load/store updates only the target backing word and keeps satp plus PTE flags stable",
        CSRR(CSR_SATP) == satp_before &&
        *ai_memblock_sfence_priv_leaf_entry(target_page) == target_pte_before &&
        *ai_memblock_sfence_priv_leaf_entry(guard_page) == guard_pte_before &&
        read64(target_paddr) == target_tvm_store &&
        read64(guard_paddr) == guard_initial
    );

    CSRC(CSR_MSTATUS, MSTATUS_TVM);

    goto_priv(PRIV_HS);
    hs_satp_read = 0;
    TEST_ASSERT("after clearing mstatus.TVM, HS-mode csrr satp retires normally again",
        ai_memblock_sfence_priv_satp_read_no_trap(&hs_satp_read) &&
        hs_satp_read == satp_before
    );

    TEST_ASSERT("after clearing mstatus.TVM, HS-mode csrw satp ASID switch retires normally again",
        ai_memblock_sfence_priv_satp_write_no_trap(satp_probe)
    );

    TEST_ASSERT("after clearing mstatus.TVM, HS-mode csrw satp restores the original ASID normally",
        ai_memblock_sfence_priv_satp_write_no_trap(satp_before)
    );

    TEST_ASSERT("after clearing mstatus.TVM, HS-mode sfence.vma x0,x0 retires normally",
        ai_memblock_sfence_priv_sfence_no_trap((uintptr_t)-1, (uint64_t)-1)
    );

    TEST_ASSERT("after clearing mstatus.TVM, HS-mode sfence.vma vaddr,x0 retires normally",
        ai_memblock_sfence_priv_sfence_no_trap(target_vaddr, (uint64_t)-1)
    );

    TEST_ASSERT("after clearing mstatus.TVM, HS-mode sfence.vma x0,asid retires normally",
        ai_memblock_sfence_priv_sfence_no_trap((uintptr_t)-1,
            AI_MEMBLOCK_SFENCE_PRIV_ASID)
    );

    TEST_ASSERT("after clearing mstatus.TVM, HS-mode sfence.vma vaddr,asid retires normally",
        ai_memblock_sfence_priv_sfence_no_trap(target_vaddr,
            AI_MEMBLOCK_SFENCE_PRIV_ASID)
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, target_updated);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the post-clear HS translated store succeeds through the same target alias",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t target_after_store = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the post-clear HS translated load observes the updated target payload",
        target_after_store == target_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the illegal control probes do not corrupt the guard mapping while the later legal translated stores update only the target backing word",
        read64(target_paddr) == target_updated &&
        read64(guard_paddr) == guard_initial &&
        *ai_memblock_sfence_priv_leaf_entry(target_page) == target_pte_before &&
        *ai_memblock_sfence_priv_leaf_entry(guard_page) == guard_pte_before
    );

    TEST_END();
}

bool ai_micro_memblock_sfence_legal_retire_preserves_translation_state_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page guard_page = VSRW_GRWX;
    const uintptr_t target_vaddr = hs_page_base(target_page);
    const uintptr_t guard_vaddr = hs_page_base(guard_page);
    const uintptr_t target_paddr = phys_page_base(target_page);
    const uintptr_t guard_paddr = phys_page_base(guard_page);
    const uint64_t target_initial = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t guard_initial = 0x8877665544332211ULL;
    const uint64_t target_after_asid = 0x1122334455667788ULL;

    uint64_t satp_initial = ai_memblock_sfence_priv_prepare_env(
        AI_MEMBLOCK_SFENCE_PRIV_ASID);
    ai_memblock_sfence_priv_set_global(target_page, false);
    ai_memblock_sfence_priv_set_global(guard_page, true);

    goto_priv(PRIV_M);
    write64(target_paddr, target_initial);
    write64(guard_paddr, guard_initial);
    sfence_vma();

    uint64_t target_pte_before = *ai_memblock_sfence_priv_leaf_entry(target_page);
    uint64_t guard_pte_before = *ai_memblock_sfence_priv_leaf_entry(guard_page);

    TEST_SETUP_EXCEPT();
    sfence_vma_rs(target_vaddr, AI_MEMBLOCK_SFENCE_PRIV_ASID);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("M-mode sfence.vma vaddr,asid retires normally when mstatus.TVM=0",
        ai_memblock_sfence_priv_get_asid(CSRR(CSR_SATP)) ==
            AI_MEMBLOCK_SFENCE_PRIV_ASID
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_TVM);
    TEST_SETUP_EXCEPT();
    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("M-mode sfence.vma x0,x0 remains legal even when mstatus.TVM=1",
        ai_memblock_sfence_priv_get_asid(CSRR(CSR_SATP)) ==
            AI_MEMBLOCK_SFENCE_PRIV_ASID
    );
    CSRC(CSR_MSTATUS, MSTATUS_TVM);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t target_warm = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the HS target translation is readable before the legal sfence retirement matrix",
        target_warm == target_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_warm = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the HS global guard translation is readable before the legal sfence retirement matrix",
        guard_warm == guard_initial
    );

    TEST_SETUP_EXCEPT();
    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS-mode sfence.vma x0,x0 retires normally when mstatus.TVM=0",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t target_after_all = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after legal HS sfence.vma x0,x0, the target mapping still returns its original payload",
        target_after_all == target_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_after_all = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after legal HS sfence.vma x0,x0, the global guard mapping still returns its original payload",
        guard_after_all == guard_initial
    );

    uint64_t satp_next = satp_initial & ~(0xffffULL << 44);
    satp_next |= (AI_MEMBLOCK_SFENCE_PRIV_ASID_NEXT << 44);
    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, satp_next);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS-mode satp ASID switch retires normally when mstatus.TVM=0",
        ai_memblock_sfence_priv_get_asid(CSRR(CSR_SATP)) ==
            AI_MEMBLOCK_SFENCE_PRIV_ASID_NEXT
    );

    TEST_SETUP_EXCEPT();
    sfence_vma_rs(target_vaddr, AI_MEMBLOCK_SFENCE_PRIV_ASID_NEXT);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("HS-mode sfence.vma vaddr,asid retires normally after switching to a new ASID context",
        true
    );

    goto_priv(PRIV_M);
    write64(target_paddr, target_after_asid);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t target_after_switch = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after the legal ASID switch and targeted sfence, the target translation reads the current backing payload",
        target_after_switch == target_after_asid
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_after_switch = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after the legal ASID switch and targeted sfence, the global guard translation remains usable",
        guard_after_switch == guard_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("legal sfence.vma and satp-ASID operations preserve PTE flags and only the explicit backing-memory update changes target data",
        *ai_memblock_sfence_priv_leaf_entry(target_page) == target_pte_before &&
        *ai_memblock_sfence_priv_leaf_entry(guard_page) == guard_pte_before &&
        read64(target_paddr) == target_after_asid &&
        read64(guard_paddr) == guard_initial &&
        ai_memblock_sfence_priv_get_asid(CSRR(CSR_SATP)) ==
            AI_MEMBLOCK_SFENCE_PRIV_ASID_NEXT
    );

    TEST_END();
}

bool ai_micro_memblock_u_mode_sfence_satp_illegal_preserves_satp_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRW(CSR_SATP, 0ULL);
    uint64_t satp_before = CSRR(CSR_SATP);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_fixed_sfence_x0_x0();
    TEST_ASSERT("HU-mode sfence.vma reports illegal instruction when mstatus.TVM=0",
        ai_memblock_sfence_priv_check_illegal_with_inst(
            AI_MEMBLOCK_SFENCE_PRIV_SFENCE_X0_X0_INST)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HU-mode sfence.vma trap leaves satp unchanged when mstatus.TVM=0",
        CSRR(CSR_SATP) == satp_before
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_fixed_csrr_satp();
    TEST_ASSERT("HU-mode csrr satp reports illegal instruction when mstatus.TVM=0",
        ai_memblock_sfence_priv_check_illegal_with_inst(
            AI_MEMBLOCK_SFENCE_PRIV_CSRR_INST(CSR_SATP))
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HU-mode satp read trap leaves satp unchanged when mstatus.TVM=0",
        CSRR(CSR_SATP) == satp_before
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_fixed_csrw_satp(satp_before ^ (1ULL << 44));
    TEST_ASSERT("HU-mode csrw satp reports illegal instruction when mstatus.TVM=0",
        ai_memblock_sfence_priv_check_illegal_with_inst(
            AI_MEMBLOCK_SFENCE_PRIV_CSRW_INST(CSR_SATP))
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HU-mode satp write trap leaves satp unchanged when mstatus.TVM=0",
        CSRR(CSR_SATP) == satp_before
    );

    CSRS(CSR_MSTATUS, MSTATUS_TVM);
    uint64_t satp_tvm_before = CSRR(CSR_SATP);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_fixed_sfence_x0_x0();
    TEST_ASSERT("HU-mode sfence.vma reports illegal instruction when mstatus.TVM=1",
        ai_memblock_sfence_priv_check_illegal_with_inst(
            AI_MEMBLOCK_SFENCE_PRIV_SFENCE_X0_X0_INST)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HU-mode sfence.vma trap leaves satp unchanged when mstatus.TVM=1",
        CSRR(CSR_SATP) == satp_tvm_before
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_fixed_csrr_satp();
    TEST_ASSERT("HU-mode csrr satp reports illegal instruction when mstatus.TVM=1",
        ai_memblock_sfence_priv_check_illegal_with_inst(
            AI_MEMBLOCK_SFENCE_PRIV_CSRR_INST(CSR_SATP))
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HU-mode satp read trap leaves satp unchanged when mstatus.TVM=1",
        CSRR(CSR_SATP) == satp_tvm_before
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_fixed_csrw_satp(
        satp_tvm_before ^ (1ULL << 44));
    TEST_ASSERT("HU-mode csrw satp reports illegal instruction when mstatus.TVM=1",
        ai_memblock_sfence_priv_check_illegal_with_inst(
            AI_MEMBLOCK_SFENCE_PRIV_CSRW_INST(CSR_SATP))
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HU-mode satp write trap leaves satp unchanged when mstatus.TVM=1",
        CSRR(CSR_SATP) == satp_tvm_before
    );

    CSRC(CSR_MSTATUS, MSTATUS_TVM);

    TEST_END();
}

bool ai_micro_memblock_m_mode_tvm_satp_access_legal_corner()
{
    TEST_START();

    const uintptr_t sfence_probe_vaddr = hs_page_base(VSRWX_GRWX);
    const uint64_t sfence_probe_asid = AI_MEMBLOCK_SFENCE_PRIV_ASID;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRW(CSR_SATP, 0ULL);
    uint64_t satp_before = CSRR(CSR_SATP);

    CSRS(CSR_MSTATUS, MSTATUS_TVM);
    TEST_SETUP_EXCEPT();
    uint64_t satp_read = CSRR(CSR_SATP);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("M-mode reads satp normally even when mstatus.TVM=1",
        satp_read == satp_before
    );

    uint64_t satp_next = satp_before & ~(0xffffULL << 44);
    satp_next |= ((AI_MEMBLOCK_SFENCE_PRIV_ASID_NEXT & 0xffffULL) << 44);
    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, satp_next);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("M-mode writes satp normally even when mstatus.TVM=1",
        CSRR(CSR_SATP) == satp_next
    );

    TEST_SETUP_EXCEPT();
    sfence_vma();
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("M-mode sfence.vma remains legal after a TVM=1 satp write",
        true
    );

    uint64_t mstatus_probe_base = CSRR(CSR_MSTATUS);
    uint64_t mprv_mpp_s = ai_memblock_sfence_priv_make_mprv_mpp_status(
        mstatus_probe_base, AI_MEMBLOCK_SFENCE_PRIV_MSTATUS_MPP_S);
    uint64_t mprv_mpp_u = ai_memblock_sfence_priv_make_mprv_mpp_status(
        mstatus_probe_base, AI_MEMBLOCK_SFENCE_PRIV_MSTATUS_MPP_U);
    uint64_t mprv_mpp_s_sum_mxr = mprv_mpp_s | MSTATUS_SUM | MSTATUS_MXR;
    uint64_t mprv_mpp_u_sum_mxr = mprv_mpp_u | MSTATUS_SUM | MSTATUS_MXR;
    uint64_t satp_mprv_s = satp_next ^ (1ULL << 44);
    uint64_t satp_mprv_u = satp_next ^ (2ULL << 44);

    TEST_SETUP_EXCEPT();
    uint64_t satp_read_mprv_s = ai_memblock_sfence_priv_mprv_read_satp(
        mprv_mpp_s, mstatus_probe_base);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("M-mode csrr satp remains legal with MPRV=1 and MPP=S",
        satp_read_mprv_s == satp_next &&
        CSRR(CSR_MSTATUS) == mstatus_probe_base
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_mprv_write_satp(mprv_mpp_s, satp_mprv_s,
        mstatus_probe_base);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("M-mode csrw satp remains legal with MPRV=1 and MPP=S",
        CSRR(CSR_SATP) == satp_mprv_s &&
        CSRR(CSR_MSTATUS) == mstatus_probe_base
    );

    TEST_ASSERT("M-mode sfence.vma rs1/rs2 quadrant matrix remains legal with MPRV=1, MPP=S, SUM=1, and MXR=1",
        ai_memblock_sfence_priv_mprv_sfence_rs_no_trap(
            mprv_mpp_s_sum_mxr, mstatus_probe_base, (uintptr_t)-1,
            (uint64_t)-1) &&
        ai_memblock_sfence_priv_mprv_sfence_rs_no_trap(
            mprv_mpp_s_sum_mxr, mstatus_probe_base, sfence_probe_vaddr,
            (uint64_t)-1) &&
        ai_memblock_sfence_priv_mprv_sfence_rs_no_trap(
            mprv_mpp_s_sum_mxr, mstatus_probe_base, (uintptr_t)-1,
            sfence_probe_asid) &&
        ai_memblock_sfence_priv_mprv_sfence_rs_no_trap(
            mprv_mpp_s_sum_mxr, mstatus_probe_base, sfence_probe_vaddr,
            sfence_probe_asid) &&
        CSRR(CSR_SATP) == satp_mprv_s &&
        CSRR(CSR_MSTATUS) == mstatus_probe_base
    );

    TEST_SETUP_EXCEPT();
    uint64_t satp_read_mprv_u = ai_memblock_sfence_priv_mprv_read_satp(
        mprv_mpp_u, mstatus_probe_base);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("M-mode csrr satp remains legal with MPRV=1 and MPP=U",
        satp_read_mprv_u == satp_mprv_s &&
        CSRR(CSR_MSTATUS) == mstatus_probe_base
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_mprv_write_satp(mprv_mpp_u, satp_mprv_u,
        mstatus_probe_base);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("M-mode csrw satp remains legal with MPRV=1 and MPP=U",
        CSRR(CSR_SATP) == satp_mprv_u &&
        CSRR(CSR_MSTATUS) == mstatus_probe_base
    );

    TEST_ASSERT("M-mode sfence.vma rs1/rs2 quadrant matrix remains legal with MPRV=1, MPP=U, SUM=1, and MXR=1",
        ai_memblock_sfence_priv_mprv_sfence_rs_no_trap(
            mprv_mpp_u_sum_mxr, mstatus_probe_base, (uintptr_t)-1,
            (uint64_t)-1) &&
        ai_memblock_sfence_priv_mprv_sfence_rs_no_trap(
            mprv_mpp_u_sum_mxr, mstatus_probe_base, sfence_probe_vaddr,
            (uint64_t)-1) &&
        ai_memblock_sfence_priv_mprv_sfence_rs_no_trap(
            mprv_mpp_u_sum_mxr, mstatus_probe_base, (uintptr_t)-1,
            sfence_probe_asid) &&
        ai_memblock_sfence_priv_mprv_sfence_rs_no_trap(
            mprv_mpp_u_sum_mxr, mstatus_probe_base, sfence_probe_vaddr,
            sfence_probe_asid) &&
        CSRR(CSR_SATP) == satp_mprv_u &&
        CSRR(CSR_MSTATUS) == mstatus_probe_base
    );

    CSRW(CSR_SATP, satp_before);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | MSTATUS_SUM | MSTATUS_MXR |
        AI_MEMBLOCK_SFENCE_PRIV_MSTATUS_MPP_MASK);

    TEST_END();
}

bool ai_micro_memblock_satp_mode_warl_rejects_reserved_modes_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page guard_page = VSRW_GRWX;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x2c0U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x2c0U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x2c0U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x2c0U;
    const uint64_t target_value = 0x5a5a123456789abcULL;
    const uint64_t guard_value = 0xc3c3fedcba987654ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    uint64_t base_satp = ai_memblock_sfence_priv_prepare_env(
        AI_MEMBLOCK_SFENCE_PRIV_ASID);
    ai_memblock_sfence_priv_set_global(target_page, false);
    ai_memblock_sfence_priv_set_global(guard_page, true);
    write64(target_paddr, target_value);
    write64(guard_paddr, guard_value);
    sfence_vma();

    const uint64_t target_pte_before =
        *ai_memblock_sfence_priv_leaf_entry(target_page);
    const uint64_t guard_pte_before =
        *ai_memblock_sfence_priv_leaf_entry(guard_page);
    const uint64_t base_mode =
        base_satp & AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_MASK;
    const uint64_t base_ppn =
        base_satp & AI_MEMBLOCK_SFENCE_PRIV_SATP_PPN_MASK;
    const uint64_t asid_pattern =
        (0x5a5aULL << AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_SHIFT) & AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_MASK;
    const uint64_t mode_clear_mask = ~(0xfULL << 60);
    const uint64_t reserved_modes[] = {
        1ULL, 2ULL, 3ULL, 4ULL, 5ULL, 6ULL, 7ULL, 10ULL, 15ULL
    };

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t target_before = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the non-global HS target translation is readable before the reserved satp.MODE WARL probes",
        target_before == target_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_before = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the global HS guard translation is readable before the reserved satp.MODE WARL probes",
        guard_before == guard_value
    );

    goto_priv(PRIV_M);
    for (size_t i = 0; i < sizeof(reserved_modes) / sizeof(reserved_modes[0]); i++) {
        uint64_t attempted = (base_satp & mode_clear_mask) |
            (reserved_modes[i] << 60);

        TEST_SETUP_EXCEPT();
        CSRW(CSR_SATP, attempted);
        CSRR(CSR_SATP);
        AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("writing and reading a reserved satp.MODE probe follows WARL behavior without raising a trap",
            true
        );
    }

    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, base_mode | AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_MASK | base_ppn);
    uint64_t asid_max_readback = CSRR(CSR_SATP);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("writing satp with ASID=max and the current root PPN converges without changing MODE or PPN",
        (asid_max_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_MASK) ==
            base_mode &&
        (asid_max_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_PPN_MASK) ==
            base_ppn
    );
    const uint64_t implemented_asid_mask =
        asid_max_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_MASK;

    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, base_mode | base_ppn);
    uint64_t asid_zero_readback = CSRR(CSR_SATP);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("writing satp with ASID=0 clears the implemented ASID bits and preserves MODE plus root PPN",
        (asid_zero_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_MASK) ==
            base_mode &&
        (asid_zero_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_MASK) == 0 &&
        (asid_zero_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_PPN_MASK) ==
            base_ppn
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, base_mode | asid_pattern | base_ppn);
    uint64_t asid_pattern_readback = CSRR(CSR_SATP);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("writing satp with a mixed ASID pattern follows the discovered implemented ASID mask",
        (asid_pattern_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_MASK) ==
            base_mode &&
        (asid_pattern_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_MASK) ==
            (asid_pattern & implemented_asid_mask) &&
        (asid_pattern_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_PPN_MASK) ==
            base_ppn
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, base_mode | implemented_asid_mask |
        AI_MEMBLOCK_SFENCE_PRIV_SATP_PPN_MASK);
    uint64_t ppn_max_readback = CSRR(CSR_SATP);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("writing satp with PPN=max converges without changing MODE or the implemented ASID bits",
        (ppn_max_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_MASK) ==
            base_mode &&
        (ppn_max_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_MASK) == implemented_asid_mask
    );
    const uint64_t implemented_ppn_mask =
        ppn_max_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_PPN_MASK;

    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, base_mode | implemented_asid_mask);
    uint64_t ppn_zero_readback = CSRR(CSR_SATP);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("writing satp with PPN=0 clears the implemented PPN bits while preserving MODE and ASID",
        (ppn_zero_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_MASK) ==
            base_mode &&
        (ppn_zero_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_MASK) == implemented_asid_mask &&
        (ppn_zero_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_PPN_MASK) == 0
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_SATP, base_mode | implemented_asid_mask |
        (base_ppn & implemented_ppn_mask));
    uint64_t ppn_restore_readback = CSRR(CSR_SATP);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("restoring the original root PPN through the discovered PPN mask produces the expected satp fields",
        (ppn_restore_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_MODE_MASK) ==
            base_mode &&
        (ppn_restore_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_ASID_MASK) == implemented_asid_mask &&
        (ppn_restore_readback & AI_MEMBLOCK_SFENCE_PRIV_SATP_PPN_MASK) ==
            (base_ppn & implemented_ppn_mask)
    );

    CSRW(CSR_SATP, base_satp);
    TEST_SETUP_EXCEPT();
    sfence_vma();
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after restoring the original satp value, sfence.vma retires normally and the ASID context is restored",
        CSRR(CSR_SATP) == base_satp
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t target_after = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after reserved satp.MODE WARL probes and restore, the old non-global translation returns the preserved payload",
        target_after == target_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_after = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after reserved satp.MODE WARL probes and restore, the global guard translation remains usable",
        guard_after == guard_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("reserved satp.MODE WARL probes do not corrupt satp, PTE.G state, or backing memory for the restored context",
        CSRR(CSR_SATP) == base_satp &&
        *ai_memblock_sfence_priv_leaf_entry(target_page) == target_pte_before &&
        *ai_memblock_sfence_priv_leaf_entry(guard_page) == guard_pte_before &&
        read64(target_paddr) == target_value &&
        read64(guard_paddr) == guard_value
    );

    TEST_END();
}

bool ai_micro_memblock_sfence_repeated_flush_idempotent_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page guard_page = VSRW_GRWX;
    const uintptr_t target_vaddr = hs_page_base(target_page);
    const uintptr_t guard_vaddr = hs_page_base(guard_page);
    const uintptr_t target_paddr = phys_page_base(target_page);
    const uintptr_t guard_paddr = phys_page_base(guard_page);
    const uint64_t target_value = 0x4466778899aabbccULL;
    const uint64_t guard_value = 0xccbbaa9988776644ULL;

    ai_memblock_sfence_priv_prepare_env(AI_MEMBLOCK_SFENCE_PRIV_ASID);
    ai_memblock_sfence_priv_set_global(target_page, false);
    ai_memblock_sfence_priv_set_global(guard_page, true);

    goto_priv(PRIV_M);
    write64(target_paddr, target_value);
    write64(guard_paddr, guard_value);
    uint64_t target_pte_before = *ai_memblock_sfence_priv_leaf_entry(target_page);
    uint64_t guard_pte_before = *ai_memblock_sfence_priv_leaf_entry(guard_page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t target_before = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the target page is readable before the repeated sfence.vma x0,x0 probe",
        target_before == target_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_before = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the guard page is readable before the repeated sfence.vma x0,x0 probe",
        guard_before == guard_value
    );

    TEST_SETUP_EXCEPT();
    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the first sfence.vma x0,x0 retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the second identical sfence.vma x0,x0 retires normally without introducing a trap",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t target_after = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after repeated sfence.vma x0,x0, the target mapping remains readable with the same payload",
        target_after == target_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_after = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after repeated sfence.vma x0,x0, the global guard mapping remains readable with the same payload",
        guard_after == guard_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated sfence.vma x0,x0 does not modify PTE flags or backing memory",
        *ai_memblock_sfence_priv_leaf_entry(target_page) == target_pte_before &&
        *ai_memblock_sfence_priv_leaf_entry(guard_page) == guard_pte_before &&
        read64(target_paddr) == target_value &&
        read64(guard_paddr) == guard_value
    );

    TEST_END();
}

bool ai_micro_memblock_sfence_unknown_asid_no_match_preserves_translation_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t target_vaddr = hs_page_base(target_page);
    const uintptr_t target_paddr = phys_page_base(target_page);
    const uint64_t target_value = 0x1122446688aacceeULL;
    const uint64_t unused_asid = AI_MEMBLOCK_SFENCE_PRIV_ASID_NEXT + 0x55ULL;

    uint64_t satp_before = ai_memblock_sfence_priv_prepare_env(
        AI_MEMBLOCK_SFENCE_PRIV_ASID);
    ai_memblock_sfence_priv_set_global(target_page, false);

    goto_priv(PRIV_M);
    write64(target_paddr, target_value);
    uint64_t target_pte_before = *ai_memblock_sfence_priv_leaf_entry(target_page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t target_before = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("the target page is readable before the unknown-ASID sfence.vma probe",
        target_before == target_value
    );

    TEST_SETUP_EXCEPT();
    sfence_vma_rs((uintptr_t)-1, unused_asid);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("sfence.vma x0,unused-asid retires normally even when no matching ASID exists",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t target_after = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_EXCEPT_AND("after sfence.vma x0,unused-asid, the current ASID's translation still returns the same payload",
        target_after == target_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("sfence.vma x0,unused-asid does not alter satp, the target PTE, or backing memory",
        CSRR(CSR_SATP) == satp_before &&
        *ai_memblock_sfence_priv_leaf_entry(target_page) == target_pte_before &&
        read64(target_paddr) == target_value
    );

    TEST_END();
}

bool ai_micro_memblock_svinval_supported_privilege_and_satp_stability_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRW(CSR_SATP, 0ULL);
    uint64_t satp_before = CSRR(CSR_SATP);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_sinval_vma();
    TEST_ASSERT("HU-mode sinval.vma still reports illegal instruction even when Svinval is implemented",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HU-mode sinval.vma illegal-instruction trap leaves satp unchanged",
        CSRR(CSR_SATP) == satp_before
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_sinval_vma();
    TEST_ASSERT("HS-mode sinval.vma retires normally when Svinval is implemented",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HS-mode sinval.vma probe leaves satp unchanged",
        CSRR(CSR_SATP) == satp_before
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_sinval_vma();
    TEST_ASSERT("M-mode sinval.vma retires normally when Svinval is implemented",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_sfence_w_inval();
    TEST_ASSERT("M-mode sfence.w.inval retires normally when Svinval is implemented",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_sfence_priv_sfence_inval_ir();
    TEST_ASSERT("M-mode sfence.inval.ir retires normally when Svinval is implemented",
        excpt.triggered == false
    );

    TEST_ASSERT("implemented Svinval probes do not alter satp in the bare satp stability check",
        CSRR(CSR_SATP) == satp_before
    );

    TEST_END();
}
