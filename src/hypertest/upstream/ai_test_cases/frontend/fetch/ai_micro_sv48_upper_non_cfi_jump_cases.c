#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/*
 * Sv48 positive canonical addresses end at 0x00007fff_ffff_ffff.
 * The aligned 32B target block ends at that boundary; the jump enters at
 * 0x00007fff_ffff_fff0, so its next PC exercises the noncanonical boundary.
 */
#define AI_SV48_UPPER_MODE              9ULL
#define AI_SV48_UPPER_POS_BOUNDARY      (1ULL << 47)
#define AI_SV48_UPPER_TARGET_PAGE       \
    (AI_SV48_UPPER_POS_BOUNDARY - PAGE_SIZE)
#define AI_SV48_UPPER_TARGET_BLOCK      \
    (AI_SV48_UPPER_TARGET_PAGE + 0xfe0ULL)
#define AI_SV48_UPPER_TARGET_PC         \
    (AI_SV48_UPPER_TARGET_PAGE + 0xff0ULL)
#define AI_SV48_UPPER_NEXT_PC           AI_SV48_UPPER_POS_BOUNDARY
#define AI_SV48_UPPER_BOUNDARY_ALIAS    0xffff800000000000ULL

#define AI_SV48_UPPER_ADDI_A0_1         0x00150513U
#define AI_SV48_UPPER_ADDI_A0_2         0x00250513U
#define AI_SV48_UPPER_ADDI_A0_4         0x00450513U
#define AI_SV48_UPPER_ADDI_A0_8         0x00850513U
#define AI_SV48_UPPER_ADDI_A0_16        0x01050513U
#define AI_SV48_UPPER_ADDI_A0_32        0x02050513U
#define AI_SV48_UPPER_SD_A0_0_A1        0x00a5b023U
#define AI_SV48_UPPER_ADDI_A0_64        0x04050513U
#define AI_SV48_UPPER_NOP               0x00000013U
#define AI_SV48_UPPER_RET               0x00008067U

static pte_t ai_sv48_upper_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_sv48_upper_low_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_sv48_upper_high_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_sv48_upper_high_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_sv48_upper_high_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_sv48_upper_boundary_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_sv48_upper_boundary_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_sv48_upper_boundary_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static uint32_t ai_sv48_upper_boundary_code[PAGE_SIZE / sizeof(uint32_t)]
    __attribute__((aligned(PAGE_SIZE)));

/* The PF handler writes this exact address into mepc before mret. */
static volatile uintptr_t ai_sv48_upper_pf_resume_pc;

extern uint64_t mhandler(void);
extern void ai_sv48_upper_pf_handler_entry(void);

/*
 * Keep this trap entry local to the case.  The normal framework handler
 * advances mepc by decoding the faulting instruction; this case needs the
 * precise post-JALR label instead, because the fault is on the following
 * noncanonical fetch.
 */
__asm__(
    ".section .text\n"
    ".align 2\n"
    ".global ai_sv48_upper_pf_handler_entry\n"
    ".type ai_sv48_upper_pf_handler_entry, @function\n"
    "ai_sv48_upper_pf_handler_entry:\n"
    "  addi sp, sp, -248\n"
    "  sd x1, 0(sp)\n"
    "  sd x3, 16(sp)\n"
    "  sd x4, 24(sp)\n"
    "  sd x5, 32(sp)\n"
    "  sd x6, 40(sp)\n"
    "  sd x7, 48(sp)\n"
    "  sd x8, 56(sp)\n"
    "  sd x9, 64(sp)\n"
    "  sd x10, 72(sp)\n"
    "  sd x11, 80(sp)\n"
    "  sd x12, 88(sp)\n"
    "  sd x13, 96(sp)\n"
    "  sd x14, 104(sp)\n"
    "  sd x15, 112(sp)\n"
    "  sd x16, 120(sp)\n"
    "  sd x17, 128(sp)\n"
    "  sd x18, 136(sp)\n"
    "  sd x19, 144(sp)\n"
    "  sd x20, 152(sp)\n"
    "  sd x21, 160(sp)\n"
    "  sd x22, 168(sp)\n"
    "  sd x23, 176(sp)\n"
    "  sd x24, 184(sp)\n"
    "  sd x25, 192(sp)\n"
    "  sd x26, 200(sp)\n"
    "  sd x27, 208(sp)\n"
    "  sd x28, 216(sp)\n"
    "  sd x29, 224(sp)\n"
    "  sd x30, 232(sp)\n"
    "  sd x31, 240(sp)\n"
    "  call ai_sv48_upper_pf_handler_body\n"
    "  ld x1, 0(sp)\n"
    "  ld x3, 16(sp)\n"
    "  ld x4, 24(sp)\n"
    "  ld x5, 32(sp)\n"
    "  ld x6, 40(sp)\n"
    "  ld x7, 48(sp)\n"
    "  ld x8, 56(sp)\n"
    "  ld x9, 64(sp)\n"
    "  ld x10, 72(sp)\n"
    "  ld x11, 80(sp)\n"
    "  ld x12, 88(sp)\n"
    "  ld x13, 96(sp)\n"
    "  ld x14, 104(sp)\n"
    "  ld x15, 112(sp)\n"
    "  ld x16, 120(sp)\n"
    "  ld x17, 128(sp)\n"
    "  ld x18, 136(sp)\n"
    "  ld x19, 144(sp)\n"
    "  ld x20, 152(sp)\n"
    "  ld x21, 160(sp)\n"
    "  ld x22, 168(sp)\n"
    "  ld x23, 176(sp)\n"
    "  ld x24, 184(sp)\n"
    "  ld x25, 192(sp)\n"
    "  ld x26, 200(sp)\n"
    "  ld x27, 208(sp)\n"
    "  ld x28, 216(sp)\n"
    "  ld x29, 224(sp)\n"
    "  ld x30, 232(sp)\n"
    "  ld x31, 240(sp)\n"
    "  addi sp, sp, 248\n"
    "  mret\n"
    ".size ai_sv48_upper_pf_handler_entry, .-ai_sv48_upper_pf_handler_entry\n"
    ".previous\n"
);

__attribute__((used, noinline, target("arch=rv64imac_zicsr")))
uint64_t ai_sv48_upper_pf_handler_body(void)
{
    const uint64_t cause = CSRR(CSR_MCAUSE);

    /* Preserve the framework's normal path for setup/privilege ECALLs. */
    if (cause != CAUSE_IPF) {
        return mhandler();
    }

    const uint64_t mstatus = CSRR(CSR_MSTATUS);
    const uint64_t epc = CSRR(CSR_MEPC);
    const uint64_t tval = CSRR(CSR_MTVAL);

    /* Match mhandler's trap-context rule before touching C objects. */
    if (mstatus & MSTATUS_MPRV) {
        CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    }

    excpt.triggered = true;
    excpt.priv = PRIV_M;
    excpt.cause = cause;
    excpt.epc = epc;
    excpt.tval = tval;
    excpt.tinst = 0;
    excpt.tval2 = 0;
    excpt.gva = !!((mstatus >> MSTATUS_GVA_OFF) & 0x1U);
    excpt.xpv = !!((mstatus >> 39U) & 0x1U);
    excpt.testing = false;
    excpt.fault_inst = 0;

    /* Resume at the instruction immediately after the original JALR. */
    CSRW(CSR_MEPC, ai_sv48_upper_pf_resume_pc);
    return PRIV_M;
}

static inline void ai_sv48_upper_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0ULL;
    }
}

static inline pte_t ai_sv48_upper_table_pte(const pte_t *next)
{
    return PTE_V | (((uintptr_t)next) >> 2U);
}

static inline pte_t ai_sv48_upper_leaf_pte_flags(uintptr_t paddr,
    pte_t flags)
{
    return ((paddr & PAGE_ADDR_MSK) >> 2U) |
        flags;
}

static inline pte_t ai_sv48_upper_leaf_pte(uintptr_t paddr)
{
    return ai_sv48_upper_leaf_pte_flags(paddr, PTE_V | PTE_AD | PTE_RWX);
}

__attribute__((noinline, optimize("no-tree-vectorize",
    "no-tree-loop-distribute-patterns")))
static void ai_sv48_upper_install_page_tables(uintptr_t code_paddr)
{
    ai_sv48_upper_zero_pt(ai_sv48_upper_root);
    ai_sv48_upper_zero_pt(ai_sv48_upper_low_l2);
    ai_sv48_upper_zero_pt(ai_sv48_upper_high_l2);
    ai_sv48_upper_zero_pt(ai_sv48_upper_high_l1);
    ai_sv48_upper_zero_pt(ai_sv48_upper_high_l0);
    ai_sv48_upper_zero_pt(ai_sv48_upper_boundary_l2);
    ai_sv48_upper_zero_pt(ai_sv48_upper_boundary_l1);
    ai_sv48_upper_zero_pt(ai_sv48_upper_boundary_l0);

    /* Keep the test harness, stack, and marker in an identity-mapped range. */
    ai_sv48_upper_root[0] =
        ai_sv48_upper_table_pte(ai_sv48_upper_low_l2);
    for (unsigned i = 0; i < 4U; ++i) {
        ai_sv48_upper_low_l2[i] =
            ai_sv48_upper_leaf_pte((uintptr_t)i << 30U);
    }

    /*
     * 0x00008000_00000000 has VPN[3:0] = 256, 0, 0, 0.  Keep this
     * noncanonical boundary address on an executable translation path so
     * the case observes the canonical-address fault itself, rather than an
     * invalid/non-executable PTE or a physical access fault at PA zero.
     * The canonical alias 0xffff8000_00000000 uses the same PTEs.
     */
    ai_sv48_upper_root[256] =
        ai_sv48_upper_table_pte(ai_sv48_upper_boundary_l2);
    ai_sv48_upper_boundary_l2[0] =
        ai_sv48_upper_table_pte(ai_sv48_upper_boundary_l1);
    ai_sv48_upper_boundary_l1[0] =
        ai_sv48_upper_table_pte(ai_sv48_upper_boundary_l0);
    ai_sv48_upper_boundary_l0[0] =
        ai_sv48_upper_leaf_pte((uintptr_t)ai_sv48_upper_boundary_code);

    /* VPN[3:0] = 255, 511, 511, 511 selects the last Sv48 page. */
    ai_sv48_upper_root[255] =
        ai_sv48_upper_table_pte(ai_sv48_upper_high_l2);
    ai_sv48_upper_high_l2[511] =
        ai_sv48_upper_table_pte(ai_sv48_upper_high_l1);
    ai_sv48_upper_high_l1[511] =
        ai_sv48_upper_table_pte(ai_sv48_upper_high_l0);
    ai_sv48_upper_high_l0[511] = ai_sv48_upper_leaf_pte(code_paddr);
}

__attribute__((noinline, optimize("no-tree-vectorize",
    "no-tree-loop-distribute-patterns")))
static void ai_sv48_upper_install_code(uintptr_t code_paddr)
{
    const uintptr_t target_paddr = code_paddr +
        (AI_SV48_UPPER_TARGET_BLOCK - AI_SV48_UPPER_TARGET_PAGE);

    /* The complete aligned block contains no CFI instruction. */
    write32(target_paddr + 0x00U, AI_SV48_UPPER_ADDI_A0_1);
    write32(target_paddr + 0x04U, AI_SV48_UPPER_ADDI_A0_2);
    write32(target_paddr + 0x08U, AI_SV48_UPPER_ADDI_A0_4);
    write32(target_paddr + 0x0cU, AI_SV48_UPPER_ADDI_A0_8);
    write32(target_paddr + 0x10U, AI_SV48_UPPER_ADDI_A0_16);
    write32(target_paddr + 0x14U, AI_SV48_UPPER_ADDI_A0_32);
    write32(target_paddr + 0x18U, AI_SV48_UPPER_SD_A0_0_A1);
    write32(target_paddr + 0x1cU, AI_SV48_UPPER_ADDI_A0_64);

    for (unsigned i = 0; i < PAGE_SIZE / sizeof(uint32_t); ++i) {
        ai_sv48_upper_boundary_code[i] = AI_SV48_UPPER_NOP;
    }
    /* A missed PF changes the marker, then returns without a later fault.
     * Both the original target block and the first boundary block are non-CFI.
     */
    ai_sv48_upper_boundary_code[0] = AI_SV48_UPPER_SD_A0_0_A1;
    ai_sv48_upper_boundary_code[0x20U / sizeof(uint32_t)] = AI_SV48_UPPER_RET;
}

__attribute__((noinline))
static uint64_t ai_sv48_upper_jump_and_capture(uintptr_t target,
    uintptr_t marker, uint64_t seed)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lla t0, 2f\n\t"
        "sd t0, %[resume]\n\t"
        "mv a0, %[seed]\n\t"
        "mv a1, %[marker]\n\t"
        "1: jalr ra, 0(%[pc])\n\t"
        "2:\n\t"
        "mv %[value], a0\n\t"
        ".option pop\n\t"
        : [value] "=r"(value), [resume] "=m"(ai_sv48_upper_pf_resume_pc)
        : [pc] "r"(target), [marker] "r"(marker), [seed] "r"(seed)
        : "t0", "ra", "a0", "a1", "memory");
    excpt.fault_inst = 0;
    return value;
}

bool ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_corner()
{
    TEST_START();

    const uintptr_t code_paddr = phys_page_base(SWITCH1);
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD) + 0x180U;
    const uint64_t sv48_satp = (AI_SV48_UPPER_MODE << 60U) |
        (((uintptr_t)ai_sv48_upper_root) >> 12U);
    const uint64_t marker_seed = 0x5a7f39f400000004ULL;
    uint64_t saved_mstatus;
    uint64_t saved_medeleg;
    uint64_t saved_satp;
    uint64_t saved_mtvec;

    goto_priv(PRIV_M);
    saved_mstatus = CSRR(CSR_MSTATUS);
    saved_medeleg = CSRR(CSR_MEDELEG);
    saved_satp = CSRR(CSR_SATP);
    saved_mtvec = CSRR(CSR_MTVEC);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_IPF);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();

    ai_sv48_upper_install_page_tables(code_paddr);
    ai_sv48_upper_install_code(code_paddr);
    write64(marker_paddr, marker_seed);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");

    TEST_ASSERT("the boundary VPN maps to executable RAM instead of PA zero",
        ai_sv48_upper_root[256] ==
            ai_sv48_upper_table_pte(ai_sv48_upper_boundary_l2) &&
        ai_sv48_upper_boundary_l2[0] ==
            ai_sv48_upper_table_pte(ai_sv48_upper_boundary_l1) &&
        ai_sv48_upper_boundary_l1[0] ==
            ai_sv48_upper_table_pte(ai_sv48_upper_boundary_l0) &&
        ai_sv48_upper_boundary_l0[0] ==
            ai_sv48_upper_leaf_pte((uintptr_t)ai_sv48_upper_boundary_code) &&
        (uintptr_t)ai_sv48_upper_boundary_code >= (uintptr_t)MEM_BASE &&
        (uintptr_t)ai_sv48_upper_boundary_code + PAGE_SIZE <=
            (uintptr_t)MEM_BASE + MEM_SIZE &&
        ai_sv48_upper_boundary_code[0] == AI_SV48_UPPER_SD_A0_0_A1 &&
        ai_sv48_upper_boundary_code[7] == AI_SV48_UPPER_NOP &&
        ai_sv48_upper_boundary_code[8] == AI_SV48_UPPER_RET);
    INFO("Sv48 boundary VA=0x%llx PA=0x%llx leaf=0x%llx",
        (unsigned long long)AI_SV48_UPPER_NEXT_PC,
        (unsigned long long)(uintptr_t)ai_sv48_upper_boundary_code,
        (unsigned long long)ai_sv48_upper_boundary_l0[0]);

    TEST_ASSERT("Sv48 maps the highest positive-canonical page and the aligned target block contains only non-CFI instructions",
        (sv48_satp >> 60U) == AI_SV48_UPPER_MODE &&
        ((ai_sv48_upper_high_l0[511] &
            (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)) &&
        (((ai_sv48_upper_high_l0[511] << 2U) & PAGE_ADDR_MSK) ==
            (code_paddr & PAGE_ADDR_MSK)) &&
        read32(code_paddr + 0xfe0U) == AI_SV48_UPPER_ADDI_A0_1 &&
        read32(code_paddr + 0xfe4U) == AI_SV48_UPPER_ADDI_A0_2 &&
        read32(code_paddr + 0xfe8U) == AI_SV48_UPPER_ADDI_A0_4 &&
        read32(code_paddr + 0xfecU) == AI_SV48_UPPER_ADDI_A0_8 &&
        read32(code_paddr + 0xff0U) == AI_SV48_UPPER_ADDI_A0_16 &&
        read32(code_paddr + 0xff4U) == AI_SV48_UPPER_ADDI_A0_32 &&
        read32(code_paddr + 0xff8U) == AI_SV48_UPPER_SD_A0_0_A1 &&
        read32(code_paddr + 0xffcU) == AI_SV48_UPPER_ADDI_A0_64);

    CSRW(CSR_MTVEC, (uintptr_t)ai_sv48_upper_pf_handler_entry);
    CSRW(CSR_SATP, sv48_satp);
    sfence_vma();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    const uint64_t first_value = ai_sv48_upper_jump_and_capture(
        AI_SV48_UPPER_TARGET_PC, marker_paddr, 1ULL);
    const bool first_triggered = excpt.triggered;
    const uint64_t first_cause = excpt.cause;
    const uint64_t first_epc = excpt.epc;
    const uint64_t first_tval = excpt.tval;
    const uint64_t first_marker = read64(marker_paddr);

    TEST_ASSERT("the first jump to 0x00007ffffffffff0 executes the non-CFI tail and faults at the noncanonical next PC",
        first_triggered == true &&
        first_cause == CAUSE_IPF &&
        first_epc == AI_SV48_UPPER_NEXT_PC &&
        first_tval == AI_SV48_UPPER_NEXT_PC &&
        first_value == 113ULL &&
        first_marker == 49ULL,
        "cause=0x%llx epc=0x%llx tval=0x%llx value=%llu marker=0x%llx",
        (unsigned long long)first_cause,
        (unsigned long long)first_epc,
        (unsigned long long)first_tval,
        (unsigned long long)first_value,
        (unsigned long long)first_marker);

    write64(marker_paddr, marker_seed);
    TEST_SETUP_EXCEPT();
    const uint64_t second_value = ai_sv48_upper_jump_and_capture(
        AI_SV48_UPPER_TARGET_PC, marker_paddr, 10ULL);
    const bool second_triggered = excpt.triggered;
    const uint64_t second_cause = excpt.cause;
    const uint64_t second_epc = excpt.epc;
    const uint64_t second_tval = excpt.tval;
    const uint64_t second_marker = read64(marker_paddr);

    TEST_ASSERT("a second warm jump to 0x00007ffffffffff0 repeats the non-CFI tail and precise boundary fault",
        second_triggered == true &&
        second_cause == CAUSE_IPF &&
        second_epc == AI_SV48_UPPER_NEXT_PC &&
        second_tval == AI_SV48_UPPER_NEXT_PC &&
        second_value == 122ULL &&
        second_marker == 58ULL,
        "cause=0x%llx epc=0x%llx tval=0x%llx value=%llu marker=0x%llx",
        (unsigned long long)second_cause,
        (unsigned long long)second_epc,
        (unsigned long long)second_tval,
        (unsigned long long)second_value,
        (unsigned long long)second_marker);

    /* Probe the same PTEs with a legal VA only after the boundary checks,
     * so this control does not warm the original cold-fetch path.
     */
    write64(marker_paddr, marker_seed);
    TEST_SETUP_EXCEPT();
    const uint64_t alias_value = ai_sv48_upper_jump_and_capture(
        AI_SV48_UPPER_BOUNDARY_ALIAS, marker_paddr, 0x55ULL);
    TEST_ASSERT("the canonical alias executes the boundary RAM page without PF or AF",
        excpt.triggered == false && alias_value == 0x55ULL &&
        read64(marker_paddr) == 0x55ULL);
    excpt.testing = false;

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_END();
}

/*
 * Companion version of the boundary case.  The boundary translation is
 * deliberately valid and readable/writable, but has no execute permission.
 * This preserves the original X-permission page-fault behavior at
 * 0x00008000_00000000 for comparison with the executable-boundary version.
 */
bool ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_x_deny_corner()
{
    TEST_START();

    const uintptr_t code_paddr = phys_page_base(SWITCH1);
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD) + 0x180U;
    const uint64_t sv48_satp = (AI_SV48_UPPER_MODE << 60U) |
        (((uintptr_t)ai_sv48_upper_root) >> 12U);
    const uint64_t marker_seed = 0x5a7f39f400000004ULL;
    uint64_t saved_mstatus;
    uint64_t saved_medeleg;
    uint64_t saved_satp;
    uint64_t saved_mtvec;

    goto_priv(PRIV_M);
    saved_mstatus = CSRR(CSR_MSTATUS);
    saved_medeleg = CSRR(CSR_MEDELEG);
    saved_satp = CSRR(CSR_SATP);
    saved_mtvec = CSRR(CSR_MTVEC);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_IPF);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();

    ai_sv48_upper_install_page_tables(code_paddr);
    ai_sv48_upper_install_code(code_paddr);
    ai_sv48_upper_boundary_l0[0] = ai_sv48_upper_leaf_pte_flags(
        (uintptr_t)ai_sv48_upper_boundary_code,
        PTE_V | PTE_AD | PTE_RW);
    write64(marker_paddr, marker_seed);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");

    TEST_ASSERT("the boundary leaf is valid RW and explicitly lacks X permission",
        ai_sv48_upper_boundary_l0[0] == ai_sv48_upper_leaf_pte_flags(
            (uintptr_t)ai_sv48_upper_boundary_code,
            PTE_V | PTE_AD | PTE_RW) &&
        (ai_sv48_upper_boundary_l0[0] & PTE_X) == 0 &&
        (ai_sv48_upper_boundary_l0[0] & (PTE_V | PTE_AD | PTE_RW)) ==
            (PTE_V | PTE_AD | PTE_RW) &&
        (((ai_sv48_upper_boundary_l0[0] << 2U) & PAGE_ADDR_MSK) ==
            ((uintptr_t)ai_sv48_upper_boundary_code & PAGE_ADDR_MSK)) &&
        read32(code_paddr + 0xfe0U) == AI_SV48_UPPER_ADDI_A0_1 &&
        read32(code_paddr + 0xffcU) == AI_SV48_UPPER_ADDI_A0_64);

    CSRW(CSR_MTVEC, (uintptr_t)ai_sv48_upper_pf_handler_entry);
    CSRW(CSR_SATP, sv48_satp);
    sfence_vma();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    const uint64_t first_value = ai_sv48_upper_jump_and_capture(
        AI_SV48_UPPER_TARGET_PC, marker_paddr, 1ULL);
    const bool first_triggered = excpt.triggered;
    const uint64_t first_cause = excpt.cause;
    const uint64_t first_epc = excpt.epc;
    const uint64_t first_tval = excpt.tval;
    const uint64_t first_marker = read64(marker_paddr);

    TEST_ASSERT("the non-CFI tail reaches the boundary and X denial raises IPF",
        first_triggered == true &&
        first_cause == CAUSE_IPF &&
        first_epc == AI_SV48_UPPER_NEXT_PC &&
        first_tval == AI_SV48_UPPER_NEXT_PC &&
        first_value == 113ULL &&
        first_marker == 49ULL,
        "cause=0x%llx epc=0x%llx tval=0x%llx value=%llu marker=0x%llx",
        (unsigned long long)first_cause,
        (unsigned long long)first_epc,
        (unsigned long long)first_tval,
        (unsigned long long)first_value,
        (unsigned long long)first_marker);

    write64(marker_paddr, marker_seed);
    TEST_SETUP_EXCEPT();
    const uint64_t second_value = ai_sv48_upper_jump_and_capture(
        AI_SV48_UPPER_TARGET_PC, marker_paddr, 10ULL);
    const bool second_triggered = excpt.triggered;
    const uint64_t second_cause = excpt.cause;
    const uint64_t second_epc = excpt.epc;
    const uint64_t second_tval = excpt.tval;
    const uint64_t second_marker = read64(marker_paddr);

    TEST_ASSERT("a warm retry still raises IPF from the missing X permission",
        second_triggered == true &&
        second_cause == CAUSE_IPF &&
        second_epc == AI_SV48_UPPER_NEXT_PC &&
        second_tval == AI_SV48_UPPER_NEXT_PC &&
        second_value == 122ULL &&
        second_marker == 58ULL,
        "cause=0x%llx epc=0x%llx tval=0x%llx value=%llu marker=0x%llx",
        (unsigned long long)second_cause,
        (unsigned long long)second_epc,
        (unsigned long long)second_tval,
        (unsigned long long)second_value,
        (unsigned long long)second_marker);

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_END();
}
