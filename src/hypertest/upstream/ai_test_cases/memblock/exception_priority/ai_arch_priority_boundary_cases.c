#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

static inline pte_t *ai_arch_priority_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_log_unexpected_priority_result(const char *message)
{
    INFO("%s", message);
    INFO_PRINT_XCEPT();
}

static inline void ai_arch_log_cross_page_store_state(uintptr_t low_phys,
    uintptr_t high_phys)
{
    INFO("low_phys = 0x%lx", low_phys);
    INFO("high_phys = 0x%lx", high_phys);
    INFO("low_value = 0x%llx", read64(low_phys));
    INFO("high_value = 0x%llx", read64(high_phys));
}

static inline uintptr_t ai_arch_cross_page_addr(enum test_page low_page,
    uintptr_t low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - low_bytes;
}

static inline uintptr_t ai_arch_page_end_phys(enum test_page page)
{
    return phys_page_base(page) + PAGE_SIZE - sizeof(uint64_t);
}

static inline void ai_arch_set_hs_leaf(enum test_page page, uintptr_t paddr,
    uint64_t flags)
{
    *ai_arch_priority_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline uintptr_t ai_arch_prepare_pbmt_nc_alias(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(VSRWXPbmt1_GURWXPbmt0);
    sfence_vma();

    return hs_page_base(VSRWXPbmt1_GURWXPbmt0);
}

static inline void ai_arch_reset_pbmt_only_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();
}

static inline void ai_arch_reset_svpbmt_pma_env(void)
{
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
}

static inline uintptr_t ai_arch_prepare_nc_page_over_io_region(void)
{
    goto_priv(PRIV_M);

    ai_arch_reset_svpbmt_pma_env();

    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);

    hspt_leaf_change_base_paddr(VSRWXPbmt1_GURWXPbmt0, 0x30000000000ULL);
    sfence_vma();

    return hs_page_base(VSRWXPbmt1_GURWXPbmt0);
}

static inline uintptr_t ai_arch_nc_page_over_io_backing_paddr(void)
{
    return phys_page_base_offset(VSRWXPbmt1_GURWXPbmt0, 0x30000000000ULL);
}

static inline uintptr_t ai_arch_prepare_pf_af_priority_page(enum test_page page)
{
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_priority_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | PTE_V | PTE_W | (1ULL << 46);
    sfence_vma();

    return hs_page_base(page);
}

static inline uintptr_t ai_arch_make_noncanonical_vaddr(uintptr_t canonical_vaddr)
{
#ifdef sv48
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit47 = (canonical_vaddr >> 47) & 1ULL;

    if (bit47) {
        bad_vaddr &= ~(1ULL << 48);
    } else {
        bad_vaddr |= (1ULL << 48);
    }
#else
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit38 = (canonical_vaddr >> 38) & 1ULL;

    if (bit38) {
        bad_vaddr &= ~(1ULL << 39);
    } else {
        bad_vaddr |= (1ULL << 39);
    }
#endif

    return bad_vaddr;
}

static inline void ai_arch_seed_hs_fetch_recovery_csrs(uintptr_t exec_vaddr)
{
    CSRW(CSR_SEPC, exec_vaddr + sizeof(uint32_t));
    CSRW(CSR_SCAUSE, CAUSE_LPF);
    CSRW(CSR_STVAL, 0ULL);
}

static inline void ai_arch_seed_hs_recovery_csrs(uintptr_t epc,
    uint64_t cause, uint64_t tval)
{
    CSRW(CSR_SEPC, epc);
    CSRW(CSR_SCAUSE, cause);
    CSRW(CSR_STVAL, tval);
}

static inline bool ai_arch_hs_fetch_recovery_csrs_unchanged(
    uintptr_t exec_vaddr)
{
    return CSRR(CSR_SEPC) == exec_vaddr + sizeof(uint32_t) &&
        CSRR(CSR_SCAUSE) == CAUSE_LPF &&
        CSRR(CSR_STVAL) == 0ULL;
}

static inline bool ai_arch_hs_recovery_csrs_match(uintptr_t epc,
    uint64_t cause, uint64_t tval)
{
    return CSRR(CSR_SEPC) == epc &&
        CSRR(CSR_SCAUSE) == cause &&
        CSRR(CSR_STVAL) == tval;
}

static inline uint64_t ai_arch_faulting_ld_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value) : "r"(addr) : "memory");
    return value;
}

static inline void ai_arch_reserved_amoadd_w_probe(uintptr_t addr,
    uint32_t value)
{
    asm volatile(
        "mv a0, %0\n\t"
        "mv a1, %1\n\t"
        ".word 0xf8b5362f\n\t"
        :: "r"(addr), "r"((uint64_t)value)
        : "a0", "a1", "a2", "memory");
}

static inline void ai_arch_exec_store_func_expect_fetch_fault(uintptr_t func,
    uintptr_t addr, uint64_t value)
{
    asm volatile(
        "mv a0, %0\n\t"
        "mv a1, %1\n\t"
        "la t0, 1f\n\t"
        "sd t0, %2\n\t"
        "jr %3\n\t"
        "1:\n\t"
        :: "r"(addr), "r"(value), "m"(excpt.fault_inst), "r"(func)
        : "a0", "a1", "t0", "memory");
}

#define AI_ARCH_IALIGN_PROBES 3U
#define AI_ARCH_IALIGN_STATUS_C_STUCK 1ULL
#define AI_ARCH_IALIGN_STATUS_NO_JALR_TRAP 0x101ULL
#define AI_ARCH_IALIGN_STATUS_NO_BRANCH_TRAP 0x102ULL
#define AI_ARCH_IALIGN_STATUS_NO_JAL_TRAP 0x103ULL

static volatile uint64_t ai_arch_ialign_hits;
static volatile uint64_t ai_arch_ialign_status;
static volatile uint64_t ai_arch_ialign_resume_pc;
static volatile uint64_t ai_arch_ialign_old_misa;
static volatile uint64_t ai_arch_ialign_old_mtvec;
static volatile uint64_t ai_arch_ialign_cause[AI_ARCH_IALIGN_PROBES];
static volatile uint64_t ai_arch_ialign_epc[AI_ARCH_IALIGN_PROBES];
static volatile uint64_t ai_arch_ialign_tval[AI_ARCH_IALIGN_PROBES];
static volatile uint64_t ai_arch_ialign_source[AI_ARCH_IALIGN_PROBES];
static volatile uint64_t ai_arch_ialign_target[AI_ARCH_IALIGN_PROBES];
static volatile uint64_t ai_arch_ialign16_hits;
static volatile uint64_t ai_arch_ialign16_source[AI_ARCH_IALIGN_PROBES];
static volatile uint64_t ai_arch_ialign16_target[AI_ARCH_IALIGN_PROBES];

extern void ai_arch_ialign_m_trap_entry(void);

asm(
    ".balign 4\n"
    ".global ai_arch_ialign_m_trap_entry\n"
    "ai_arch_ialign_m_trap_entry:\n"
    "  .option push\n"
    "  .option norelax\n"
    "  .option norvc\n"
    "  csrr t0, mcause\n"
    "  la t1, ai_arch_ialign_hits\n"
    "  ld t2, 0(t1)\n"
    "  li t3, 3\n"
    "  bgeu t2, t3, 1f\n"
    "  slli t3, t2, 3\n"
    "  la t4, ai_arch_ialign_cause\n"
    "  add t4, t4, t3\n"
    "  sd t0, 0(t4)\n"
    "  csrr t0, mepc\n"
    "  la t4, ai_arch_ialign_epc\n"
    "  add t4, t4, t3\n"
    "  sd t0, 0(t4)\n"
    "  csrr t0, mtval\n"
    "  la t4, ai_arch_ialign_tval\n"
    "  add t4, t4, t3\n"
    "  sd t0, 0(t4)\n"
    "  addi t2, t2, 1\n"
    "  sd t2, 0(t1)\n"
    "1:\n"
    "  la t1, ai_arch_ialign_resume_pc\n"
    "  ld t0, 0(t1)\n"
    "  csrw mepc, t0\n"
    "  mret\n"
    "  .option pop\n"
);

__attribute__((naked, noinline, aligned(4))) static uint64_t
ai_arch_ialign32_run_all(void)
{
    asm volatile(
        ".option push\n"
        ".option norelax\n"
        ".option norvc\n"
        "li t0, 0\n"
        "la t1, ai_arch_ialign_status\n"
        "sd t0, 0(t1)\n"
        "csrr t0, mtvec\n"
        "la t1, ai_arch_ialign_old_mtvec\n"
        "sd t0, 0(t1)\n"
        "csrr t0, misa\n"
        "la t1, ai_arch_ialign_old_misa\n"
        "sd t0, 0(t1)\n"
        "la t0, ai_arch_ialign_m_trap_entry\n"
        "csrw mtvec, t0\n"
        "csrr t0, misa\n"
        "andi t0, t0, -5\n"
        "csrw misa, t0\n"
        "fence.i\n"
        "csrr t0, misa\n"
        "andi t0, t0, 4\n"
        "beqz t0, 1f\n"
        "li t0, 1\n"
        "la t1, ai_arch_ialign_status\n"
        "sd t0, 0(t1)\n"
        "j 90f\n"
        "1:\n"

        "lla t0, 12f\n"
        "la t1, ai_arch_ialign_resume_pc\n"
        "sd t0, 0(t1)\n"
        "lla t0, 10f\n"
        "la t1, ai_arch_ialign_source\n"
        "sd t0, 0(t1)\n"
        "lla t2, 13f\n"
        "la t1, ai_arch_ialign_target\n"
        "sd t2, 0(t1)\n"
        "10:\n"
        "jalr zero, 0(t2)\n"
        "12:\n"
        "j 14f\n"
        ".2byte 0x0001\n"
        "13:\n"
        "li t0, 0x101\n"
        "la t1, ai_arch_ialign_status\n"
        "sd t0, 0(t1)\n"
        "j 14f\n"
        ".2byte 0x0001\n"
        "14:\n"

        "lla t0, 22f\n"
        "la t1, ai_arch_ialign_resume_pc\n"
        "sd t0, 0(t1)\n"
        "lla t0, 20f\n"
        "la t1, ai_arch_ialign_source\n"
        "sd t0, 8(t1)\n"
        "lla t0, 23f\n"
        "la t1, ai_arch_ialign_target\n"
        "sd t0, 8(t1)\n"
        "20:\n"
        "beq zero, zero, 23f\n"
        "22:\n"
        "j 24f\n"
        ".2byte 0x0001\n"
        "23:\n"
        "li t0, 0x102\n"
        "la t1, ai_arch_ialign_status\n"
        "sd t0, 0(t1)\n"
        "j 24f\n"
        ".2byte 0x0001\n"
        "24:\n"

        "lla t0, 32f\n"
        "la t1, ai_arch_ialign_resume_pc\n"
        "sd t0, 0(t1)\n"
        "lla t0, 30f\n"
        "la t1, ai_arch_ialign_source\n"
        "sd t0, 16(t1)\n"
        "lla t0, 33f\n"
        "la t1, ai_arch_ialign_target\n"
        "sd t0, 16(t1)\n"
        "30:\n"
        "jal zero, 33f\n"
        "32:\n"
        "j 34f\n"
        ".2byte 0x0001\n"
        "33:\n"
        "li t0, 0x103\n"
        "la t1, ai_arch_ialign_status\n"
        "sd t0, 0(t1)\n"
        "j 34f\n"
        ".2byte 0x0001\n"
        "34:\n"

        "90:\n"
        "la t1, ai_arch_ialign_old_misa\n"
        "ld t0, 0(t1)\n"
        "csrw misa, t0\n"
        "fence.i\n"
        "la t1, ai_arch_ialign_old_mtvec\n"
        "ld t0, 0(t1)\n"
        "csrw mtvec, t0\n"
        "la t1, ai_arch_ialign_status\n"
        "ld a0, 0(t1)\n"
        "ret\n"
        ".option pop\n");
}

static inline void ai_arch_ialign_reset_capture(void)
{
    ai_arch_ialign_hits = 0;
    ai_arch_ialign_status = 0;
    ai_arch_ialign_resume_pc = 0;
    ai_arch_ialign_old_misa = 0;
    ai_arch_ialign_old_mtvec = 0;
    ai_arch_ialign16_hits = 0;

    for (unsigned i = 0; i < AI_ARCH_IALIGN_PROBES; i++) {
        ai_arch_ialign_cause[i] = 0;
        ai_arch_ialign_epc[i] = 0;
        ai_arch_ialign_tval[i] = 0;
        ai_arch_ialign_source[i] = 0;
        ai_arch_ialign_target[i] = 0;
        ai_arch_ialign16_source[i] = 0;
        ai_arch_ialign16_target[i] = 0;
    }
}

static inline void ai_arch_ialign_log_capture(void)
{
    INFO("ialign_status = 0x%llx", ai_arch_ialign_status);
    INFO("ialign_hits = 0x%llx", ai_arch_ialign_hits);
    for (unsigned i = 0; i < AI_ARCH_IALIGN_PROBES; i++) {
        INFO("ialign[%u] cause=0x%llx epc=0x%llx tval=0x%llx source=0x%llx target=0x%llx",
            i,
            ai_arch_ialign_cause[i],
            ai_arch_ialign_epc[i],
            ai_arch_ialign_tval[i],
            ai_arch_ialign_source[i],
            ai_arch_ialign_target[i]);
    }
}

__attribute__((naked, noinline, aligned(4))) static uint64_t
ai_arch_ialign16_run_all(void)
{
    asm volatile(
        ".option push\n"
        ".option norelax\n"
        ".option norvc\n"
        "li a0, 0\n"
        "lla t0, 10f\n"
        "la t1, ai_arch_ialign16_source\n"
        "sd t0, 0(t1)\n"
        "lla t2, 11f\n"
        "la t1, ai_arch_ialign16_target\n"
        "sd t2, 0(t1)\n"
        ".2byte 0x0001\n"
        "10:\n"
        "jalr zero, 0(t2)\n"
        "11:\n"
        ".2byte 0x0001\n"
        "addi a0, a0, 1\n"
        "j 12f\n"
        "12:\n"

        "lla t0, 20f\n"
        "la t1, ai_arch_ialign16_source\n"
        "sd t0, 8(t1)\n"
        "lla t0, 21f\n"
        "la t1, ai_arch_ialign16_target\n"
        "sd t0, 8(t1)\n"
        "20:\n"
        "beq zero, zero, 21f + 2\n"
        ".2byte 0x0001\n"
        "21:\n"
        ".2byte 0x0001\n"
        "addi a0, a0, 1\n"
        "j 22f\n"
        "22:\n"

        "lla t0, 30f\n"
        "la t1, ai_arch_ialign16_source\n"
        "sd t0, 16(t1)\n"
        "lla t0, 31f + 2\n"
        "la t1, ai_arch_ialign16_target\n"
        "sd t0, 16(t1)\n"
        "30:\n"
        "jal zero, 31f + 2\n"
        "31:\n"
        ".2byte 0x0001\n"
        "addi a0, a0, 1\n"
        "la t1, ai_arch_ialign16_hits\n"
        "sd a0, 0(t1)\n"
        "ret\n"
        ".option pop\n"
        ::: "memory");
}

bool ai_arch_ialign16_rvc_jal_jalr_branch_halfword_targets_legal_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_ialign_reset_capture();
    uint64_t hits = ai_arch_ialign16_run_all();

    TEST_ASSERT("with RVC enabled and IALIGN=16, JALR/branch/JAL may legally transfer control to halfword-aligned targets without trapping",
        hits == AI_ARCH_IALIGN_PROBES &&
        ai_arch_ialign16_hits == AI_ARCH_IALIGN_PROBES &&
        excpt.triggered == false
    );

    for (unsigned i = 0; i < AI_ARCH_IALIGN_PROBES; i++) {
        TEST_ASSERT("each IALIGN=16 legal control-transfer source is halfword-aligned and its target is halfword-aligned but not 4-byte aligned",
            ai_arch_ialign16_source[i] != 0 &&
            ai_arch_ialign16_target[i] != 0 &&
            (ai_arch_ialign16_source[i] & 0x1ULL) == 0 &&
            (ai_arch_ialign16_target[i] & 0x3ULL) == 0x2ULL
        );
    }

    TEST_END();
}

bool ai_arch_ialign32_jal_jalr_branch_misaligned_targets_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_ialign_reset_capture();
    uint64_t status = ai_arch_ialign32_run_all();

    if (status == AI_ARCH_IALIGN_STATUS_C_STUCK) {
        ai_arch_ialign_log_capture();
        TEST_ASSERT("on the current NHV5.1AP profile, runtime misa.C clearing is unsupported, so the IALIGN=32 dynamic probe is skipped before any control-transfer capture",
            ai_arch_ialign_hits == 0
        );
    } else {
        if (!(status == 0 && ai_arch_ialign_hits == AI_ARCH_IALIGN_PROBES)) {
            ai_arch_ialign_log_capture();
        }
        TEST_ASSERT("with misa.C temporarily cleared in a 32-bit M-mode probe island, JALR/branch/JAL halfword targets each trap and execution resumes at aligned recovery PCs",
            status == 0 &&
            ai_arch_ialign_hits == AI_ARCH_IALIGN_PROBES
        );

        for (unsigned i = 0; i < AI_ARCH_IALIGN_PROBES; i++) {
            bool shape_ok =
                ai_arch_ialign_source[i] != 0 &&
                ai_arch_ialign_target[i] != 0 &&
                (ai_arch_ialign_source[i] & 0x3ULL) == 0 &&
                (ai_arch_ialign_target[i] & 0x3ULL) == 0x2ULL;

            if (!(shape_ok &&
                ai_arch_ialign_cause[i] == CAUSE_IAM &&
                ai_arch_ialign_epc[i] == ai_arch_ialign_source[i] &&
                ai_arch_ialign_tval[i] == ai_arch_ialign_target[i])) {
                ai_arch_ialign_log_capture();
            }
            TEST_ASSERT("each captured IALIGN=32 control-transfer fault reports instruction-address-misaligned, points epc at the source instruction, and reports the halfword target in mtval",
                shape_ok &&
                ai_arch_ialign_cause[i] == CAUSE_IAM &&
                ai_arch_ialign_epc[i] == ai_arch_ialign_source[i] &&
                ai_arch_ialign_tval[i] == ai_arch_ialign_target[i]
            );
        }
    }

    TEST_END();
}

bool ai_arch_load_pbmt_nc_misaligned_baseline()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t vaddr = ai_arch_prepare_pbmt_nc_alias() + 7U;

    goto_priv(PRIV_M);
    write64(paddr, 0x1122334455667788ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr)) {
        ai_arch_log_unexpected_priority_result("unexpected PBMT=NC load misalign baseline result");
    }
    TEST_ASSERT("a scalar HS load to a PBMT=NC non-Device page reports load misaligned before any PF/AF cross",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PBMT=NC load misalign baseline leaves backing memory unchanged",
        read64(paddr) == 0x1122334455667788ULL
    );
    ai_arch_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_pbmt_nc_misaligned_baseline()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    const uint64_t initial_value = 0x0123456789abcdefULL;
    uintptr_t vaddr = ai_arch_prepare_pbmt_nc_alias() + 7U;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr)) {
        ai_arch_log_unexpected_priority_result("unexpected PBMT=NC store misalign baseline result");
    }
    TEST_ASSERT("a scalar HS store to a PBMT=NC non-Device page reports store misaligned before any PF/AF cross",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PBMT=NC store misalign baseline leaves backing memory unchanged",
        read64(paddr) == initial_value
    );
    ai_arch_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_pbmt_nc_over_io_misaligned_priority()
{
    TEST_START();

    uintptr_t vaddr = ai_arch_prepare_nc_page_over_io_region() + 7U;
    uintptr_t recovery_vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t recovery_paddr = phys_page_base(VSRWX_GRWX) + 0x180U;
    const uint64_t load_sentinel = 0x5a6b7c8d9eafb001ULL;
    const uint64_t recovery_value = 0x1122334455667788ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_fault =
        ai_arch_faulting_ld_rd_probe(vaddr, load_sentinel);
    TEST_ASSERT("a scalar HS PBMT=NC load reports load misaligned even when the PA also matches a PMA IO/MMIO region and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr &&
        observed_fault == load_sentinel
    );

    goto_priv(PRIV_M);
    ai_arch_reset_svpbmt_pma_env();
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    write64(recovery_paddr, recovery_value);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(recovery_vaddr);
    TEST_ASSERT("after removing the PBMT/PMA-over-IO misalign setup and using an aligned address, a fresh HS load writes back the current memory value",
        excpt.triggered == false &&
        observed_recovery == recovery_value
    );

    TEST_END();
}

bool ai_arch_load_pf_af_priority()
{
    TEST_START();

    enum test_page page = VSRWX_GRWX;
    uintptr_t vaddr = ai_arch_prepare_pf_af_priority_page(page);
    const uint64_t load_sentinel = 0x9a8b7c6d5e4f3021ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_load =
        ai_arch_faulting_ld_rd_probe(vaddr, load_sentinel);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        observed_load == load_sentinel)) {
        ai_arch_log_unexpected_priority_result("unexpected load PF+AF priority result");
    }
    TEST_ASSERT("a load that sees both an invalid leaf-page encoding and an overflowing leaf PPN reports the first encountered translation exception as load page fault and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        observed_load == load_sentinel
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_leaf_overflow_access_fault_rd_suppressed_corner()
{
    TEST_START();

    enum test_page page = VSRWX_GRWX;
    uintptr_t vaddr = hs_page_base(page) + 0x128U;
    uintptr_t paddr = phys_page_base(page) + 0x128U;
    const uint64_t seed = 0x0123456789abcdefULL;
    const uint64_t load_sentinel = 0xfeedface00112233ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_priority_hs_leaf_entry(page) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_fault =
        ai_arch_faulting_ld_rd_probe(vaddr, load_sentinel);
    TEST_ASSERT("a load to an HS leaf with an overflowing PPN reports load access fault with the original VA and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr &&
        observed_fault == load_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the leaf-overflow load access fault leaves the backing word unchanged",
        read64(paddr) == seed
    );

    ai_arch_set_hs_leaf(page, phys_page_base(page), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_repair = ld(vaddr);
    TEST_ASSERT("after repairing the HS leaf PPN, the same load writes back the seeded value without inheriting the earlier access-fault metadata",
        excpt.triggered == false &&
        observed_repair == seed
    );

    TEST_END();
}

bool ai_arch_store_pbmt_nc_over_io_misaligned_priority()
{
    TEST_START();

    uintptr_t vaddr = ai_arch_prepare_nc_page_over_io_region() + 7U;
    uintptr_t recovery_vaddr = hs_page_base(VSRWX_GRWX) + 0x1c0U;
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    uintptr_t recovery_paddr = phys_page_base(VSRWX_GRWX) + 0x1c0U;
    uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    const uint64_t recovery_seed = 0x0102030405060708ULL;
    const uint64_t recovery_adjacent = 0xaabbccddeeff0011ULL;
    const uint64_t recovery_store = 0x8877665544332211ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("a scalar HS PBMT=NC store reports store misaligned even when the PA also matches a PMA IO/MMIO region",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_arch_reset_svpbmt_pma_env();
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    write64(recovery_paddr, recovery_seed);
    write64(recovery_adjacent_paddr, recovery_adjacent);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store);
    TEST_ASSERT("after removing the PBMT/PMA-over-IO misalign setup and using an aligned address, a fresh HS store retires normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(recovery_adjacent_vaddr);
    TEST_ASSERT("the repaired aligned store updates only its target doubleword and leaves the adjacent guard word readable",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repaired aligned store, the target backing word matches the store payload while the adjacent guard remains unchanged",
        read64(recovery_paddr) == recovery_store &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    TEST_END();
}

bool ai_arch_store_pf_af_priority()
{
    TEST_START();

    enum test_page page = VSRW_GRWX;
    uintptr_t paddr = phys_page_base(page);
    uintptr_t vaddr = ai_arch_prepare_pf_af_priority_page(page);
    const uint64_t initial_value = 0x1021324354657687ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr)) {
        ai_arch_log_unexpected_priority_result("unexpected store PF+AF priority result");
    }
    TEST_ASSERT("a store that sees both an invalid leaf-page encoding and an overflowing leaf PPN reports the first encountered translation exception, which is store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PF-selected store priority path does not modify the target backing word",
        read64(paddr) == initial_value
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_amo_pf_af_priority_no_rmw_side_effect_corner()
{
    TEST_START();

    enum test_page page = VSRW_GRWX;
    uintptr_t paddr = phys_page_base(page);
    uintptr_t vaddr = ai_arch_prepare_pf_af_priority_page(page);
    const uint64_t initial_value = 0x3141592653589793ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x0102030405060708ULL);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr)) {
        ai_arch_log_unexpected_priority_result("unexpected AMO PF+AF priority result");
    }
    TEST_ASSERT("an AMO that sees both an invalid leaf-page encoding and an overflowing leaf PPN reports the first translation exception as store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PF-selected AMO priority path does not perform a partial read-modify-write on the backing word",
        read64(paddr) == initial_value
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_all_ones_virtual_address_faults()
{
    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    uintptr_t vaddr = (uintptr_t)-1;
    const uint64_t load_sentinel = 0xfeedfacecafebeefULL;

    TEST_SETUP_EXCEPT();
    uint64_t observed_load = ai_arch_faulting_ld_rd_probe(vaddr, load_sentinel);
    TEST_ASSERT("loading from an all-ones virtual address reports a load page fault and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        observed_load == load_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("storing to an all-ones virtual address reports a store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_arch_all_ones_store_page_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t probe_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(probe_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("storing to an all-ones virtual address reports store page fault with the raw all-ones VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the all-ones store page-fault corner leaves a valid backing page unchanged",
        read64(probe_paddr) == seed
    );

    TEST_END();
}

bool ai_arch_all_ones_faults_then_valid_access_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t stored = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("loading from an all-ones virtual address first reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("storing to an all-ones virtual address next reports store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(valid_vaddr);
    TEST_ASSERT("after the all-ones load/store faults, a subsequent canonical HS load still returns the seeded payload",
        excpt.triggered == false &&
        loaded == seed
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after the all-ones load/store faults, a subsequent canonical HS store still retires successfully",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovery access sequence, backing memory matches the canonical store payload",
        read64(valid_paddr) == stored
    );

    TEST_END();
}

bool ai_arch_all_ones_faults_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX) + 0x40U;
    uintptr_t adjacent_vaddr = valid_vaddr + sizeof(uint64_t);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX) + 0x40U;
    uintptr_t adjacent_paddr = valid_paddr + sizeof(uint64_t);
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t stored = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("loading from an all-ones virtual address first reports load page fault before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("storing to an all-ones virtual address next reports store page fault before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the all-ones load/store fault pair leaves both the recovery target and its adjacent backing word unchanged",
        read64(valid_paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(valid_vaddr);
    TEST_ASSERT("after the all-ones load/store faults, a subsequent canonical HS load still returns the seeded payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        loaded == seed
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after the all-ones load/store faults, a subsequent canonical HS store still retires successfully in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered canonical store, the adjacent HS word still reads back its original payload in the all-ones adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the all-ones recovery sequence, only the target backing word changes while the adjacent word remains untouched",
        read64(valid_paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_all_ones_repeated_faults_stable_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t probe_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x1122334455667788ULL;
    const uint64_t first_sentinel = 0xfeedfacecafebeefULL;
    const uint64_t second_sentinel = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(probe_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t first_probe =
        ai_arch_faulting_ld_rd_probe(bad_vaddr, first_sentinel);
    TEST_ASSERT("the first all-ones load probe reports load page fault with the raw all-ones VA in tval and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr &&
        first_probe == first_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_probe =
        ai_arch_faulting_ld_rd_probe(bad_vaddr, second_sentinel);
    TEST_ASSERT("the second all-ones load probe still reports the same load page fault without changing tval semantics or rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr &&
        second_probe == second_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after repeated all-ones load faults, an all-ones store probe still reports store page fault with the raw all-ones VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated all-ones fault probes leave a valid backing page unchanged",
        read64(probe_paddr) == seed
    );

    TEST_END();
}

bool ai_arch_all_ones_repeated_faults_then_valid_access_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t stored = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first all-ones load probe reports load page fault with the raw all-ones VA in tval before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first all-ones store probe reports store page fault with the raw all-ones VA in tval before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the second all-ones load probe still reports load page fault with the same raw VA in tval before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xfeedfacecafebeefULL);
    TEST_ASSERT("the second all-ones store probe still reports store page fault with the same raw VA in tval before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(valid_vaddr);
    TEST_ASSERT("after repeated all-ones load/store faults, a subsequent canonical HS load still returns the seeded payload",
        excpt.triggered == false &&
        loaded == seed
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after repeated all-ones load/store faults, a subsequent canonical HS store still retires successfully",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated all-ones faults and the recovery access sequence, backing memory matches the canonical store payload",
        read64(valid_paddr) == stored
    );

    TEST_END();
}

bool ai_arch_all_ones_repeated_faults_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX) + 0x80U;
    uintptr_t adjacent_vaddr = valid_vaddr + sizeof(uint64_t);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX) + 0x80U;
    uintptr_t adjacent_paddr = valid_paddr + sizeof(uint64_t);
    const uint64_t seed = 0x7766554433221100ULL;
    const uint64_t stored = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t adjacent = 0xa5b4c3d2e1f00112ULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first all-ones load probe reports load page fault with the raw all-ones VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first all-ones store probe reports store page fault with the raw all-ones VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the second all-ones load probe still reports the same raw all-ones load page fault before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xfeedfacecafebeefULL);
    TEST_ASSERT("the second all-ones store probe still reports the same raw all-ones store page fault before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated all-ones load/store faults leave both the recovery target and its adjacent backing word unchanged",
        read64(valid_paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(valid_vaddr);
    TEST_ASSERT("after repeated all-ones load/store faults, a subsequent canonical HS load still returns the seeded payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        loaded == seed
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after repeated all-ones load/store faults, a subsequent canonical HS store still retires successfully in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the repeated all-ones recovery sequence, the adjacent HS word still reads back its original payload",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated all-ones faults and the recovery access sequence, only the target backing word changes while the adjacent word remains untouched",
        read64(valid_paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_noncanonical_virtual_address_faults()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(valid_vaddr);
    const uint64_t load_sentinel = 0x0badf00d12345678ULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed_load =
        ai_arch_faulting_ld_rd_probe(bad_vaddr, load_sentinel);
    TEST_ASSERT("loading from a non-canonical virtual address reports a load page fault with the raw non-canonical VA in tval and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr &&
        observed_load == load_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("storing to a non-canonical virtual address reports a store page fault with the raw non-canonical VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_END();
}

bool ai_arch_noncanonical_store_page_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(valid_vaddr);
    uintptr_t probe_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(probe_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("storing to a non-canonical virtual address reports store page fault with the raw non-canonical VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the non-canonical store page-fault corner leaves a valid backing page unchanged",
        read64(probe_paddr) == seed
    );

    TEST_END();
}

bool ai_arch_noncanonical_faults_then_valid_access_recovers_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(valid_vaddr);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t stored = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("loading from a non-canonical virtual address first reports load page fault with the raw non-canonical VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("storing to a non-canonical virtual address next reports store page fault with the raw non-canonical VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(valid_vaddr);
    TEST_ASSERT("after the non-canonical load/store faults, a subsequent canonical HS load still returns the seeded payload",
        excpt.triggered == false &&
        loaded == seed
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after the non-canonical load/store faults, a subsequent canonical HS store still retires successfully",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the non-canonical recovery access sequence, backing memory matches the canonical store payload",
        read64(valid_paddr) == stored
    );

    TEST_END();
}

bool ai_arch_noncanonical_faults_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX) + 0xc0U;
    uintptr_t adjacent_vaddr = valid_vaddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(valid_vaddr);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX) + 0xc0U;
    uintptr_t adjacent_paddr = valid_paddr + sizeof(uint64_t);
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t stored = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent = 0x0badc0de55aa1234ULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("loading from a non-canonical virtual address first reports load page fault with the raw non-canonical VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("storing to a non-canonical virtual address next reports store page fault with the raw non-canonical VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the non-canonical load/store fault pair leaves both the recovery target and its adjacent backing word unchanged",
        read64(valid_paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(valid_vaddr);
    TEST_ASSERT("after the non-canonical load/store faults, a subsequent canonical HS load still returns the seeded payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        loaded == seed
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after the non-canonical load/store faults, a subsequent canonical HS store still retires successfully in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered canonical store, the adjacent HS word still reads back its original payload in the non-canonical adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the non-canonical recovery sequence, only the target backing word changes while the adjacent word remains untouched",
        read64(valid_paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_noncanonical_repeated_faults_stable_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(valid_vaddr);
    uintptr_t probe_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x1122334455667788ULL;
    const uint64_t first_sentinel = 0x0badf00d12345678ULL;
    const uint64_t second_sentinel = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(probe_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t first_probe =
        ai_arch_faulting_ld_rd_probe(bad_vaddr, first_sentinel);
    TEST_ASSERT("the first non-canonical load probe reports load page fault with the raw non-canonical VA in tval and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr &&
        first_probe == first_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_probe =
        ai_arch_faulting_ld_rd_probe(bad_vaddr, second_sentinel);
    TEST_ASSERT("the second non-canonical load probe still reports the same load page fault without changing tval semantics or rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr &&
        second_probe == second_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after repeated non-canonical load faults, a non-canonical store probe still reports store page fault with the raw non-canonical VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated non-canonical fault probes leave a valid backing page unchanged",
        read64(probe_paddr) == seed
    );

    TEST_END();
}

bool ai_arch_noncanonical_repeated_faults_then_valid_access_recovers_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(valid_vaddr);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t stored = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first non-canonical load probe reports load page fault with the raw non-canonical VA in tval before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first non-canonical store probe reports store page fault with the raw non-canonical VA in tval before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the second non-canonical load probe still reports load page fault with the same raw VA in tval before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xfeedfacecafebeefULL);
    TEST_ASSERT("the second non-canonical store probe still reports store page fault with the same raw VA in tval before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(valid_vaddr);
    TEST_ASSERT("after repeated non-canonical load/store faults, a subsequent canonical HS load still returns the seeded payload",
        excpt.triggered == false &&
        loaded == seed
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after repeated non-canonical load/store faults, a subsequent canonical HS store still retires successfully",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated non-canonical faults and the recovery access sequence, backing memory matches the canonical store payload",
        read64(valid_paddr) == stored
    );

    TEST_END();
}

bool ai_arch_noncanonical_repeated_faults_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX) + 0x100U;
    uintptr_t adjacent_vaddr = valid_vaddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(valid_vaddr);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX) + 0x100U;
    uintptr_t adjacent_paddr = valid_paddr + sizeof(uint64_t);
    const uint64_t seed = 0x66554433221100ffULL;
    const uint64_t stored = 0x1021324354657687ULL;
    const uint64_t adjacent = 0xcc33cc3355aa55aaULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first non-canonical load probe reports load page fault with the raw non-canonical VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first non-canonical store probe reports store page fault with the raw non-canonical VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the second non-canonical load probe still reports load page fault with the same raw VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xfeedfacecafebeefULL);
    TEST_ASSERT("the second non-canonical store probe still reports store page fault with the same raw VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated non-canonical load/store faults leave both the recovery target and its adjacent backing word unchanged",
        read64(valid_paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(valid_vaddr);
    TEST_ASSERT("after repeated non-canonical load/store faults, a subsequent canonical HS load still returns the seeded payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        loaded == seed
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after repeated non-canonical load/store faults, a subsequent canonical HS store still retires successfully in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the repeated non-canonical recovery sequence, the adjacent HS word still reads back its original payload",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated non-canonical faults and the recovery access sequence, only the target backing word changes while the adjacent word remains untouched",
        read64(valid_paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_fetch_all_ones_page_fault_tval_aligned_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = bad_vaddr & ~1ULL;

    goto_priv(PRIV_M);
    write32(phys_page_base(VSRWX_GRWX), 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("a canonical executable HS address still fetches successfully before probing the all-ones fetch corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("an HS fetch to an all-ones virtual address reports instruction page fault, with tval reflecting the JALR-cleared fetch target",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    TEST_END();
}

bool ai_arch_fetch_all_ones_fault_then_canonical_executes_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = bad_vaddr & ~1ULL;
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("an HS fetch to an all-ones virtual address first reports instruction page fault, with tval reflecting the JALR-cleared fetch target",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the all-ones fetch fault leaves the backing instruction word unchanged before canonical recovery",
        read32(exec_paddr) == instruction
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after the all-ones fetch fault, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs with stale metadata",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    TEST_END();
}

bool ai_arch_fetch_all_ones_fault_then_canonical_executes_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = bad_vaddr & ~1ULL;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("an HS fetch to an all-ones virtual address first reports instruction page fault, with tval reflecting the JALR-cleared fetch target before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the all-ones fetch fault leaves both the target and adjacent backing instruction words unchanged before canonical recovery",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after the all-ones fetch fault, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs in the adjacent-preservation corner",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the all-ones fetch recovery, both the target and adjacent backing instruction words remain untouched",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_fetch_all_ones_repeated_faults_stable_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = bad_vaddr & ~1ULL;
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the first all-ones HS fetch probe reports instruction page fault, with tval reflecting the JALR-cleared fetch target",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the second all-ones HS fetch probe still reports instruction page fault with the same JALR-cleared fetch target in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated all-ones fetch faults leave the backing instruction word unchanged",
        read32(exec_paddr) == instruction
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated all-ones fetch faults, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    TEST_END();
}

bool ai_arch_fetch_all_ones_repeated_faults_then_canonical_executes_backing_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = bad_vaddr & ~1ULL;
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the first all-ones HS fetch probe reports instruction page fault with tval reflecting the JALR-cleared fetch target before canonical recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the second all-ones HS fetch probe still reports instruction page fault with the same JALR-cleared fetch target before canonical recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated all-ones fetch faults still leave the backing instruction word unchanged before canonical recovery",
        read32(exec_paddr) == instruction
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated all-ones fetch faults, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated all-ones fetch faults and canonical recovery, the backing instruction word remains unchanged",
        read32(exec_paddr) == instruction
    );

    TEST_END();
}

bool ai_arch_fetch_all_ones_repeated_faults_then_canonical_executes_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = bad_vaddr & ~1ULL;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the first all-ones HS fetch probe reports instruction page fault with tval reflecting the JALR-cleared fetch target before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the second all-ones HS fetch probe still reports instruction page fault with the same JALR-cleared fetch target before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated all-ones fetch faults leave both the target and adjacent backing instruction words unchanged before canonical recovery",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated all-ones fetch faults, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs in the adjacent-preservation corner",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated all-ones fetch faults and canonical recovery, both the target and adjacent backing instruction words remain untouched",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_fetch_noncanonical_page_fault_tval_raw()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(exec_vaddr);

    goto_priv(PRIV_M);
    write32(phys_page_base(VSRWX_GRWX), 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("a canonical executable HS address still fetches successfully before probing the non-canonical corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("an HS fetch to a non-canonical virtual address reports instruction page fault and preserves the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    TEST_END();
}

bool ai_arch_fetch_noncanonical_fault_then_canonical_executes_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(exec_vaddr);

    goto_priv(PRIV_M);
    write32(phys_page_base(VSRWX_GRWX), 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("an HS fetch to a non-canonical virtual address first reports instruction page fault with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after the non-canonical fetch fault, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs with stale metadata",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    TEST_END();
}

bool ai_arch_fetch_noncanonical_fault_then_canonical_executes_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(exec_vaddr);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("an HS fetch to a non-canonical virtual address first reports instruction page fault with the raw VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the non-canonical fetch fault leaves both the target and adjacent backing instruction words unchanged before canonical recovery",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after the non-canonical fetch fault, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs in the adjacent-preservation corner",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the non-canonical fetch recovery, both the target and adjacent backing instruction words remain untouched",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_fetch_noncanonical_repeated_faults_stable_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(exec_vaddr);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the first non-canonical HS fetch probe reports instruction page fault with the raw non-canonical VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the second non-canonical HS fetch probe still reports instruction page fault with the same raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated non-canonical fetch faults leave the backing instruction word unchanged",
        read32(exec_paddr) == instruction
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated non-canonical fetch faults, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    TEST_END();
}

bool ai_arch_fetch_noncanonical_repeated_faults_then_canonical_executes_backing_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(exec_vaddr);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the first non-canonical HS fetch probe reports instruction page fault with the raw non-canonical VA in tval before canonical recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the second non-canonical HS fetch probe still reports instruction page fault with the same raw non-canonical VA in tval before canonical recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated non-canonical fetch faults still leave the backing instruction word unchanged before canonical recovery",
        read32(exec_paddr) == instruction
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated non-canonical fetch faults, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated non-canonical fetch faults and canonical recovery, the backing instruction word remains unchanged",
        read32(exec_paddr) == instruction
    );

    TEST_END();
}

bool ai_arch_fetch_noncanonical_repeated_faults_then_canonical_executes_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t bad_vaddr = ai_arch_make_noncanonical_vaddr(exec_vaddr);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the first non-canonical HS fetch probe reports instruction page fault with the raw non-canonical VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the second non-canonical HS fetch probe still reports instruction page fault with the same raw non-canonical VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated non-canonical fetch faults leave both the target and adjacent backing instruction words unchanged before canonical recovery",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_fetch_recovery_csrs(exec_vaddr);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated non-canonical fetch faults, a subsequent canonical fetch executes successfully without rewriting HS trap CSRs in the adjacent-preservation corner",
        excpt.triggered == false &&
        ai_arch_hs_fetch_recovery_csrs_unchanged(exec_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated non-canonical fetch faults and canonical recovery, both the target and adjacent backing instruction words remain untouched",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_fetch_page_fault_masks_illegal_instruction_decode_corner()
{
    TEST_START();

    enum test_page exec_page = VSRWX_GRWX;
    uintptr_t exec_vaddr = hs_page_base(exec_page);
    uintptr_t exec_paddr = phys_page_base(exec_page);
    const uint32_t illegal_instruction = 0xffffffffU;
    const uint32_t legal_return = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, illegal_instruction);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_set_hs_leaf(exec_page, exec_paddr, PTE_V | PTE_AD | PTE_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(exec_vaddr);
    TEST_ASSERT("an HS fetch from a non-executable leaf that contains an illegal instruction reports instruction page fault before decode can surface IllegalInstruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == exec_vaddr
    );

    goto_priv(PRIV_M);
    write32(exec_paddr, legal_return);
    ai_arch_set_hs_leaf(exec_page, exec_paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repairing execute permission and replacing the illegal encoding with a return instruction, the same HS fetch path executes normally",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_fetch_page_fault_blocks_embedded_store_side_effect_corner()
{
    TEST_START();

    enum test_page exec_page = VSRWX_GRWX;
    enum test_page data_page = VSRW_GRWX;
    uintptr_t exec_vaddr = hs_page_base(exec_page) + 0x100U;
    uintptr_t exec_paddr = phys_page_base(exec_page) + 0x100U;
    uintptr_t data_vaddr = hs_page_base(data_page) + 0x240U;
    uintptr_t data_paddr = phys_page_base(data_page) + 0x240U;
    uintptr_t guard_paddr = data_paddr + sizeof(uint64_t);
    const uint32_t sd_a1_0_a0 = 0x00b53023U;
    const uint32_t ret_inst = 0x00008067U;
    const uint64_t data_seed = 0x1021324354657687ULL;
    const uint64_t guard_seed = 0xfedcba9876543210ULL;
    const uint64_t store_payload = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write32(exec_paddr, sd_a1_0_a0);
    write32(exec_paddr + sizeof(uint32_t), ret_inst);
    write64(data_paddr, data_seed);
    write64(guard_paddr, guard_seed);
    asm volatile("fence.i" ::: "memory");

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_set_hs_leaf(exec_page, phys_page_base(exec_page),
        PTE_V | PTE_AD | PTE_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_exec_store_func_expect_fetch_fault(exec_vaddr, data_vaddr,
        store_payload);
    TEST_ASSERT("fetching an embedded store from a non-executable HS leaf reports instruction page fault before the younger data store can issue",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == exec_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fetch-page-faulted embedded-store path leaves both target and guard backing words unchanged",
        read64(data_paddr) == data_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_arch_set_hs_leaf(exec_page, phys_page_base(exec_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(uintptr_t, uint64_t))exec_vaddr)(data_vaddr, store_payload);
    TEST_ASSERT("after execute permission is repaired, the same embedded store function retires normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repaired fetch path executes, only the target backing word reflects the embedded store payload",
        read64(data_paddr) == store_payload &&
        read64(guard_paddr) == guard_seed
    );

    TEST_END();
}

bool ai_arch_fetch_data_fault_metadata_isolation_corner()
{
    TEST_START();

    enum test_page exec_page = VSRWX_GRWX;
    enum test_page data_page = VSRW_GRWX;
    uintptr_t exec_vaddr = hs_page_base(exec_page);
    uintptr_t exec_paddr = phys_page_base(exec_page);
    uintptr_t data_vaddr = hs_page_base(data_page) + 0x80U;
    uintptr_t data_paddr = phys_page_base(data_page) + 0x80U;
    uintptr_t bad_fetch = ai_arch_make_noncanonical_vaddr(exec_vaddr);
    uintptr_t bad_data = ai_arch_make_noncanonical_vaddr(data_vaddr);
    const uint32_t legal_return = 0x00008067U;
    const uint64_t data_seed = 0x0123456789abcdefULL;
    const uint64_t data_store = 0xfedcba9876543210ULL;
    const uint64_t load_sentinel = 0x55aa66bb77cc88ddULL;

    goto_priv(PRIV_M);
    write32(exec_paddr, legal_return);
    write64(data_paddr, data_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_fetch);
    TEST_ASSERT("the metadata-isolation sequence starts with a non-canonical HS fetch reporting instruction page fault and raw fetch VA in stval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_fetch
    );

    TEST_SETUP_EXCEPT();
    uint64_t load_value =
        ai_arch_faulting_ld_rd_probe(bad_data, load_sentinel);
    TEST_ASSERT("after the fetch fault, a non-canonical HS load reports load page fault with its own data VA in stval and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_data &&
        load_value == load_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(bad_data, data_store);
    TEST_ASSERT("after fetch and load faults, a non-canonical HS store reports store page fault with store VA metadata rather than stale fetch/load metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_data
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fault metadata isolation sequence leaves the canonical data backing word untouched before recovery accesses",
        read64(data_paddr) == data_seed
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_recovery_csrs(exec_vaddr + sizeof(uint32_t),
        CAUSE_SPF, bad_data);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after IPF/LPF/SPF metadata switching, a legal HS fetch executes normally and does not rewrite HS trap CSRs with stale fault metadata",
        excpt.triggered == false &&
        ai_arch_hs_recovery_csrs_match(exec_vaddr + sizeof(uint32_t),
            CAUSE_SPF, bad_data)
    );

    ai_arch_seed_hs_recovery_csrs(data_vaddr, CAUSE_IPF, bad_fetch);
    TEST_SETUP_EXCEPT();
    uint64_t recovered_load = ld(data_vaddr);
    TEST_ASSERT("after legal fetch recovery, a legal HS load returns the seeded data and still does not inherit stale fetch-fault metadata",
        excpt.triggered == false &&
        recovered_load == data_seed &&
        ai_arch_hs_recovery_csrs_match(data_vaddr, CAUSE_IPF, bad_fetch)
    );

    ai_arch_seed_hs_recovery_csrs(data_vaddr + sizeof(uint32_t),
        CAUSE_LPF, bad_data);
    TEST_SETUP_EXCEPT();
    sd(data_vaddr, data_store);
    TEST_ASSERT("after fetch/data fault metadata switching, a legal HS store retires normally without stale load/store fault CSR rewrite",
        excpt.triggered == false &&
        ai_arch_hs_recovery_csrs_match(data_vaddr + sizeof(uint32_t),
            CAUSE_LPF, bad_data)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the final legal store, only the canonical data backing word is updated to the store payload",
        read64(data_paddr) == data_store
    );

    goto_priv(PRIV_HS);
    ai_arch_seed_hs_recovery_csrs(data_vaddr, CAUSE_LPF, bad_data);
    TEST_SETUP_EXCEPT();
    uint64_t second_load =
        ai_arch_faulting_ld_rd_probe(bad_data, load_sentinel);
    TEST_ASSERT("the reverse metadata-isolation leg starts with a fresh data-side load page fault and does not inherit the earlier legal-store recovery CSR seed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_data &&
        second_load == load_sentinel
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_fetch);
    TEST_ASSERT("after a data-side load fault, a following fetch fault refreshes cause and stval back to the fetch VA instead of reusing data metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_fetch
    );

    ai_arch_seed_hs_recovery_csrs(exec_vaddr + sizeof(uint32_t),
        CAUSE_LPF, bad_data);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after the reverse data-to-fetch fault switch, the canonical fetch path still executes and leaves the seeded HS trap CSRs unchanged",
        excpt.triggered == false &&
        ai_arch_hs_recovery_csrs_match(exec_vaddr + sizeof(uint32_t),
            CAUSE_LPF, bad_data)
    );

    TEST_END();
}

bool ai_arch_fetch_pf_af_priority_cross_page_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    uintptr_t code_vaddr = hs_page_base(low_page) + PAGE_SIZE - 2U;
    uintptr_t high_half_vaddr = hs_page_base(high_page);
    uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 2U;
    uintptr_t high_phys = phys_page_base(high_page);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    write8(low_phys, 0x13U);
    write8(low_phys + 1U, 0x05U);
    write8(high_phys, 0x70U);
    write8(high_phys + 1U, 0x00U);
    write32(high_phys + 2U, 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_R);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX | (1ULL << 46));
    sfence_vma();

    goto_priv(PRIV_HS);
    asm volatile("fence.i" ::: "memory");
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("when a split 32-bit HS instruction starts on a no-X low half and its upper half also has a leaf-overflow candidate, instruction page fault on the first half is reported first",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.epc == code_vaddr &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    asm volatile("fence.i" ::: "memory");
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after repairing the low half execute permission, the same split instruction exposes the upper-half fetch access fault with sepc still at the instruction start",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.epc == code_vaddr &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    asm volatile("fence.i" ::: "memory");
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after both fetch halves are repaired, the split 32-bit instruction and following return execute normally",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_amo_reserved_encoding_illegal_no_memory_side_effects_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page guard_page = VSX_GRWX;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x180U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x180U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x180U;
    const uint64_t target_seed = 0x0123456789abcdefULL;
    const uint64_t guard_seed = 0xfedcba9876543210ULL;
    const uint64_t legal_leaf_no_ad = PTE_V | PTE_RWX;
    const uint64_t legal_addend = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = target_seed + legal_addend;

    goto_priv(PRIV_M);
    write64(target_paddr, target_seed);
    write64(guard_paddr, guard_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_set_hs_leaf(target_page, phys_page_base(target_page),
        legal_leaf_no_ad);
    ai_arch_set_hs_leaf(guard_page, phys_page_base(guard_page),
        legal_leaf_no_ad);
    const uint64_t target_leaf = *ai_arch_priority_hs_leaf_entry(target_page);
    const uint64_t guard_leaf = *ai_arch_priority_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_reserved_amoadd_w_probe(target_vaddr, 0x13579bdfU);
    TEST_ASSERT("a reserved AMO encoding reports illegal instruction before data translation or AMO side effects and records the trapped instruction in stval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        (uint32_t)excpt.tval == 0xf8b5362fU &&
        excpt.tval != target_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the reserved-AMO illegal path does not set PTE A/D bits or modify target/guard backing memory",
        *ai_arch_priority_hs_leaf_entry(target_page) == target_leaf &&
        *ai_arch_priority_hs_leaf_entry(guard_page) == guard_leaf &&
        read64(target_paddr) == target_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_arch_set_hs_leaf(target_page, phys_page_base(target_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(target_vaddr, legal_addend);
    TEST_ASSERT("after the illegal reserved-AMO probe, enabling the target leaf lets a legal AMO retire normally and return the old doubleword",
        excpt.triggered == false &&
        old_value == target_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the later legal AMO updates only the target doubleword while the guard page remains unchanged",
        read64(target_paddr) == expected_after_amo &&
        read64(guard_paddr) == guard_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_misalign_pf_priority()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    const uint64_t load_sentinel = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_load =
        ai_arch_faulting_ld_rd_probe(vaddr, load_sentinel);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr &&
        observed_load == load_sentinel)) {
        ai_arch_log_unexpected_priority_result("unexpected load misalign+PF priority result");
    }
    TEST_ASSERT("for the project RTL policy, once the low half of a scalar HS cross-page load already hits the PBMT=NC misalign condition, the access stops before probing the high half, reports load misaligned with the low-half start VA in tval, and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr &&
        observed_load == load_sentinel
    );

    ai_arch_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_misalign_pf_priority()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1021324354657687ULL;
    const uint64_t high_pattern = 0x98badcfe10325476ULL;
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr)) {
        ai_arch_log_unexpected_priority_result("unexpected store misalign+PF priority result");
    }
    TEST_ASSERT("for the project RTL policy, once the low half of a scalar HS cross-page store already hits the PBMT=NC misalign condition, the access stops before probing the high half and reports store misaligned with the low-half start VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the low-half PBMT-NC store-misalign priority path leaves both cross-page boundary fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_arch_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_pbmt_nc_misaligned_store_fault_no_side_effect_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1021324354657687ULL;
    const uint64_t high_pattern = 0x98badcfe10325476ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr)) {
        ai_arch_log_unexpected_priority_result("unexpected PBMT-NC misaligned store priority result");
    }
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    if (!(read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    )) {
        ai_arch_log_cross_page_store_state(low_phys, high_phys);
    }
    TEST_ASSERT("PBMT-NC low-half misaligned store takes priority before probing the high-half page-fault path, reports the original VA in tval, and leaves both page fragments unchanged",
        trap_triggered == true &&
        trap_cause == CAUSE_SAM &&
        trap_tval == vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );

    ai_arch_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_misalign_pf_repeated_then_fixed_cross_page_load_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    const uint64_t first_sentinel = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t second_sentinel = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_probe =
        ai_arch_faulting_ld_rd_probe(vaddr, first_sentinel);
    TEST_ASSERT("the first cross-page load probe on the project-RTL PBMT-NC low-half plus high-half page-fault setup stops at low-half load misaligned and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr &&
        first_probe == first_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_probe =
        ai_arch_faulting_ld_rd_probe(vaddr, second_sentinel);
    TEST_ASSERT("the second probe of the same PBMT-NC low-half plus high-half page-fault setup still stops at the same low-half load misaligned point and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr &&
        second_probe == second_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated low-half load-misalign probes leave both boundary backing doublewords unchanged before the cross-page leaves are repaired",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_arch_reset_pbmt_only_env();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page), PTE_V | PTE_AD | PTE_RWX);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after repeated project-RTL low-half misalign stops, fixing both leaves lets the same split load recover and return the merged payload",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    ai_arch_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_misalign_pf_repeated_then_fixed_cross_page_load_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    const uint64_t low_adjacent = 0x3141592653589793ULL;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t high_adjacent = 0x2718281828459045ULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    const uint64_t first_sentinel = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t second_sentinel = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_probe =
        ai_arch_faulting_ld_rd_probe(vaddr, first_sentinel);
    TEST_ASSERT("the first cross-page load probe on the project-RTL PBMT-NC low-half plus high-half page-fault setup still stops at low-half load misaligned and keeps rd unchanged in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr &&
        first_probe == first_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_probe =
        ai_arch_faulting_ld_rd_probe(vaddr, second_sentinel);
    TEST_ASSERT("the second probe of that same PBMT-NC low-half plus high-half page-fault setup still stops at the same low-half load misaligned point and keeps rd unchanged in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr &&
        second_probe == second_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated low-half load-misalign probes leave both boundary backing doublewords and their adjacent neighbors unchanged before the cross-page leaves are repaired",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_arch_reset_pbmt_only_env();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page), PTE_V | PTE_AD | PTE_RWX);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after repeated project-RTL low-half misalign stops, fixing both leaves lets the same split load recover while keeping both adjacent boundary words untouched",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-load path reads the merged payload without dirtying either adjacent boundary word",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_arch_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_misalign_pf_repeated_then_fixed_cross_page_store_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first cross-page store probe on the project-RTL PBMT-NC low-half plus high-half page-fault setup stops at low-half store misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("the second probe of the same PBMT-NC low-half plus high-half page-fault setup still stops at the same low-half store misaligned point",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated low-half store-misalign probes leave both boundary backing doublewords unchanged before the cross-page leaves are repaired",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_arch_reset_pbmt_only_env();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page), PTE_V | PTE_AD | PTE_RWX);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store);
    TEST_ASSERT("after repeated project-RTL low-half misalign stops, fixing both leaves lets the same split store recover and retire normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the untouched low-page bytes",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the untouched high-page bytes",
        read64(high_phys) == expected_high_after_store
    );

    ai_arch_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_misalign_pf_repeated_then_fixed_cross_page_store_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    const uint64_t low_adjacent = 0x0badc0de55aa1234ULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0x1234fedc5678ba90ULL;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first cross-page store probe on the project-RTL PBMT-NC low-half plus high-half page-fault setup still stops at low-half store misaligned in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("the second probe of that same PBMT-NC low-half plus high-half page-fault setup still stops at the same low-half store misaligned point in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated low-half store-misalign probes leave both boundary backing doublewords and their adjacent neighbors unchanged before the cross-page leaves are repaired",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_arch_reset_pbmt_only_env();
    ai_arch_set_hs_leaf(low_page, phys_page_base(low_page), PTE_V | PTE_AD | PTE_RWX);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store);
    TEST_ASSERT("after repeated project-RTL low-half misalign stops, fixing both leaves lets the same split store recover while keeping both adjacent boundary words untouched",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-store path updates the low boundary fragment as expected while preserving the adjacent low-page word",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("the recovered split-store path updates the high boundary fragment as expected while preserving the adjacent high-page word",
        read64(high_phys) == expected_high_after_store &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_arch_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_cross_page_high_half_page_fault_after_low_half_cacheable_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t load_sentinel = 0x1122aabb3344ccddULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed_load =
        ai_arch_faulting_ld_rd_probe(vaddr, load_sentinel);
    TEST_ASSERT("once the low half is an ordinary cacheable half-page again, the same split-load shape surfaces the true high-half load page fault with the second-half start VA in tval and no partial rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr &&
        observed_load == load_sentinel
    );

    TEST_END();
}

bool ai_arch_store_cross_page_high_half_page_fault_after_low_half_cacheable_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x02468ace13579bdfULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("once the low half is no longer blocked first, the same split-store shape surfaces the true high-half store page fault with the second-half start VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the strict high-half store page-fault no-partial-write probe keeps both low and high boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    TEST_END();
}

bool ai_arch_load_cross_page_high_half_page_fault_repeated_then_fix_high_half_load_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    const uint64_t first_sentinel = 0x1122aabb3344ccddULL;
    const uint64_t second_sentinel = 0x5566eeff77889900ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t first_probe =
        ai_arch_faulting_ld_rd_probe(vaddr, first_sentinel);
    TEST_ASSERT("the first cross-page load probe whose low half is cacheable but high half has no read permission reports load page fault at the true second-half start address and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr &&
        first_probe == first_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_probe =
        ai_arch_faulting_ld_rd_probe(vaddr, second_sentinel);
    TEST_ASSERT("the second probe of the same cacheable-low-half plus high-half page-fault setup still reports the same second-half load page fault and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr &&
        second_probe == second_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated high-half load-page-fault probes leave both boundary backing doublewords unchanged before the high-half leaf is repaired",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    goto_priv(PRIV_HS);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after repeated high-half page faults, fixing the upper leaf lets the same split load recover and return the merged payload",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_cross_page_high_half_page_fault_repeated_then_fix_high_half_load_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first cross-page load probe whose low half is cacheable but high half has no read permission still reports the same second-half load page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the second probe of that same cacheable-low-half plus high-half page-fault setup still reports the same second-half load page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated high-half load-page-fault probes leave both boundary backing doublewords and their adjacent neighbors unchanged before the high-half leaf is repaired",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    goto_priv(PRIV_HS);
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after repeated high-half page faults, fixing the upper leaf lets the same split load recover while keeping both adjacent boundary words untouched",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-load path reads the merged payload without dirtying either adjacent boundary word on the repaired cross-page path",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_cross_page_high_half_page_fault_repeated_then_other_aligned_load_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSX_GRWX;
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x100U;
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x100U;
    const uint64_t low_pattern = 0x6a7b8c9daebfc0d1ULL;
    const uint64_t high_pattern = 0x1234fedc5678ba90ULL;
    const uint64_t recovery_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t first_sentinel = 0x1122aabb3344ccddULL;
    const uint64_t second_sentinel = 0x5566eeff77889900ULL;
    const uint64_t third_sentinel = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(recovery_phys, recovery_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t first_probe =
        ai_arch_faulting_ld_rd_probe(vaddr, first_sentinel);
    TEST_ASSERT("the first cacheable-low-half plus high-half load-page-fault probe reports load page fault at the second-half start address before the unrelated aligned-load recovery probe and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr &&
        first_probe == first_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_probe =
        ai_arch_faulting_ld_rd_probe(vaddr, second_sentinel);
    TEST_ASSERT("the second cacheable-low-half plus high-half load-page-fault probe still reports the same second-half load page fault before the unrelated aligned-load recovery probe and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr &&
        second_probe == second_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(recovery_vaddr);
    TEST_ASSERT("even without fixing the faulting high half, another aligned load in the still-cacheable low page recovers normally after the repeated second-half load page faults",
        excpt.triggered == false &&
        observed == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t third_probe =
        ai_arch_faulting_ld_rd_probe(vaddr, third_sentinel);
    TEST_ASSERT("after the unrelated aligned-load recovery probe, the original split-load shape still reports the same second-half load page fault until the high-half leaf is actually repaired and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr &&
        third_probe == third_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated second-half load page faults followed by an unrelated aligned low-page load leave the boundary backing doublewords untouched while preserving the aligned recovery payload",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_cross_page_high_half_page_fault_repeated_then_other_aligned_load_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSX_GRWX;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x100U;
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x100U;
    uintptr_t recovery_adjacent_phys = recovery_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0x1021324354657687ULL;
    const uint64_t low_pattern = 0x6a7b8c9daebfc0d1ULL;
    const uint64_t high_pattern = 0x1234fedc5678ba90ULL;
    const uint64_t high_adjacent = 0x8796a5b4c3d2e1f0ULL;
    const uint64_t recovery_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t recovery_adjacent = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    write64(recovery_phys, recovery_seed);
    write64(recovery_adjacent_phys, recovery_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first cacheable-low-half plus high-half load-page-fault probe reports load page fault at the second-half start address before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the second cacheable-low-half plus high-half load-page-fault probe still reports the same second-half load page fault before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the unrelated aligned-load recovery probe, repeated second-half load page faults leave both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_seed &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(recovery_vaddr);
    TEST_ASSERT("even without fixing the faulting high half, another aligned load in the still-cacheable low page recovers normally in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(recovery_adjacent_vaddr);
    TEST_ASSERT("the aligned recovery probe does not disturb the adjacent low-page word, which still reads back its original payload",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after the unrelated aligned-load recovery probe, the original split-load shape still reports the same second-half load page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated second-half load page faults followed by an unrelated aligned low-page load leave the boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_seed &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_cross_page_high_half_page_fault_repeated_then_fix_high_half_store_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t second_fault_store = 0x1122334455667788ULL;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first cross-page store probe whose low half is writable but high half has no write permission reports store page fault at the true second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_fault_store);
    TEST_ASSERT("the second probe of the same cacheable-low-half plus high-half page-fault setup still reports the same second-half store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated high-half store-page-fault probes preserve the faulting upper-page word before the repaired retry",
        read64(high_phys) == high_pattern
    );
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store);
    TEST_ASSERT("after repeated high-half page faults, fixing the upper leaf lets the same split store recover and retire normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the untouched low-page bytes",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the untouched high-page bytes",
        read64(high_phys) == expected_high_after_store
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_cross_page_high_half_page_fault_repeated_then_fix_high_half_store_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t low_adjacent = 0xfaceb00c12344321ULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0xabcdef0123456789ULL;
    const uint64_t second_fault_store = 0x1122334455667788ULL;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first cross-page store probe whose low half is writable but high half has no write permission still reports the same second-half store page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_fault_store);
    TEST_ASSERT("the second probe of that same cacheable-low-half plus high-half page-fault setup still reports the same second-half store page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated high-half store-page-fault probes preserve both adjacent neighbors and the faulting upper-page word before the repaired retry",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_arch_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store);
    TEST_ASSERT("after repeated high-half page faults, fixing the upper leaf lets the same split store recover while keeping both adjacent boundary words untouched",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-store path updates the low boundary fragment as expected while preserving the adjacent low-page word on the repaired cross-page path",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("the recovered split-store path updates the high boundary fragment as expected while preserving the adjacent high-page word on the repaired cross-page path",
        read64(high_phys) == expected_high_after_store &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_cross_page_high_half_page_fault_repeated_then_other_aligned_store_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x100U;
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x100U;
    const uint64_t low_pattern = 0x89abcdef01234567ULL;
    const uint64_t high_pattern = 0x76543210fedcba98ULL;
    const uint64_t recovery_seed = 0x1111222233334444ULL;
    const uint64_t recovery_store = 0xdeadbeefcafef00dULL;
    const uint64_t final_fault_store = 0x0badf00d12345678ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(recovery_phys, recovery_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("the first cacheable-low-half plus high-half store-page-fault probe reports store page fault at the second-half start address before the unrelated aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("the second cacheable-low-half plus high-half store-page-fault probe still reports the same second-half store page fault before the unrelated aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store);
    TEST_ASSERT("even without fixing the faulting high half, another aligned store in the still-cacheable low page retires normally after the repeated second-half store page faults",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, final_fault_store);
    TEST_ASSERT("after the unrelated aligned-store recovery probe, the original split-store shape still reports the same second-half store page fault until the high-half leaf is actually repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated second-half store page faults followed by an unrelated aligned low-page store preserve the faulting upper-page word and commit the independent aligned store",
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_store
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_cross_page_high_half_page_fault_repeated_then_other_aligned_store_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t vaddr = ai_arch_cross_page_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x100U;
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x100U;
    uintptr_t recovery_adjacent_phys = recovery_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0x2233445566778899ULL;
    const uint64_t low_pattern = 0x89abcdef01234567ULL;
    const uint64_t high_pattern = 0x76543210fedcba98ULL;
    const uint64_t high_adjacent = 0x9988776655443322ULL;
    const uint64_t recovery_seed = 0x1111222233334444ULL;
    const uint64_t recovery_adjacent = 0x5555666677778888ULL;
    const uint64_t recovery_store = 0xdeadbeefcafef00dULL;
    const uint64_t second_fault_store = 0xfedcba9876543210ULL;
    const uint64_t final_fault_store = 0x0badf00d12345678ULL;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    write64(recovery_phys, recovery_seed);
    write64(recovery_adjacent_phys, recovery_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("the first cacheable-low-half plus high-half store-page-fault probe reports store page fault at the second-half start address before the adjacent-preservation aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_fault_store);
    TEST_ASSERT("the second cacheable-low-half plus high-half store-page-fault probe still reports the same second-half store page fault before the adjacent-preservation aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the unrelated aligned-store recovery probe, repeated second-half store page faults preserve adjacent words, the faulting upper-page word, and the aligned recovery region",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_seed &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store);
    TEST_ASSERT("even without fixing the faulting high half, another aligned store in the still-cacheable low page retires normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(recovery_adjacent_vaddr);
    TEST_ASSERT("the aligned recovery store does not disturb the adjacent low-page word, which still reads back its original payload",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, final_fault_store);
    TEST_ASSERT("after the unrelated aligned-store recovery probe, the original split-store shape still reports the same second-half store page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated second-half store page faults followed by an unrelated aligned low-page store preserve adjacent words and the faulting upper-page word, and commit only the aligned recovery store",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_store &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
