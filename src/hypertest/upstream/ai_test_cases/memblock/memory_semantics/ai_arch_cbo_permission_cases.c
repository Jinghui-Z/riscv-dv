#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT(message) \
    TEST_ASSERT(message, excpt.triggered == false)

#define AI_ASSERT_NO_EXCEPT_AND(message, condition) \
    TEST_ASSERT(message, excpt.triggered == false && (condition))

#define AI_ASSERT_EXCEPT(message, expected_cause, expected_tval) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == (expected_cause) && \
        excpt.tval == (expected_tval))

#define AI_ARCH_CBO_CBOM_PAGE SWITCH1
#define AI_ARCH_CBO_ZERO_PAGE SWITCH2
#define AI_ARCH_CBO_BLOCK_OFFSET 64U
#define AI_ARCH_CBO_BLOCK_WORDS 8U
#define AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET \
    ((AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t)) - 1U)
#define AI_ARCH_CBO_PMP_TOR_RWX 0x0fULL
#define AI_ARCH_CBO_PMP_TOR_NO_R 0x0eULL
#define AI_ARCH_CBO_PMP_TOR_NO_W 0x0dULL
#define AI_ARCH_CBO_ENVCFG_CBO_MASK \
    (MENVCFG_CBZE | MENVCFG_CBZFE | MENVCFG_CBIE11)
#define AI_ARCH_CBO_ENVCFG_ILLEGAL_PATTERN (1ULL << 61)
#ifndef CSR_SPFCTL
#define CSR_SPFCTL 0x5c1
#endif
#ifndef CSR_SMBLOCKCTL
#define CSR_SMBLOCKCTL 0x5c3
#endif
#define AI_ARCH_CBO_SPFCTL_SAFE_PATTERN \
    ((1ULL << 0) | (1ULL << 1) | (1ULL << 2) | (1ULL << 4) | \
     (1ULL << 5) | (7ULL << 6) | (0x15ULL << 10) | (1ULL << 16))
#define AI_ARCH_CBO_SMBLOCKCTL_SAFE_PATTERN \
    ((1ULL << 0) | (5ULL << 1))

static inline pte_t *ai_arch_cbo_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uintptr_t ai_arch_cbo_target(enum test_page page)
{
    return hs_page_base(page) + AI_ARCH_CBO_BLOCK_OFFSET;
}

static inline uintptr_t ai_arch_cbo_paddr(enum test_page page)
{
    return phys_page_base(page) + AI_ARCH_CBO_BLOCK_OFFSET;
}

static inline void ai_arch_log_unexpected_cbo_result(const char *message)
{
    INFO("%s", message);
    INFO_PRINT_XCEPT();
}

static inline void ai_arch_prepare_hs_cbo_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_HS);
}

static inline void ai_arch_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_ARCH_CBO_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_arch_reset_hs_cbo_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, menvcfg_bits);
    ai_arch_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline void ai_arch_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_ARCH_CBO_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_ARCH_CBO_PMP_TOR_RWX << 16));
}

static inline void ai_arch_seed_block(enum test_page page, uint64_t seed)
{
    uintptr_t paddr = ai_arch_cbo_paddr(page);

    goto_priv(PRIV_M);
    for (size_t i = 0; i < AI_ARCH_CBO_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)),
            seed + (i * 0x1111111111111111ULL));
    }
}

static inline void ai_arch_seed_block_at(uintptr_t paddr, uint64_t seed)
{
    goto_priv(PRIV_M);
    for (size_t i = 0; i < AI_ARCH_CBO_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)),
            seed + (i * 0x1111111111111111ULL));
    }
}

static inline bool ai_arch_block_matches_seed(enum test_page page, uint64_t seed)
{
    uintptr_t paddr = ai_arch_cbo_paddr(page);

    goto_priv(PRIV_M);
    for (size_t i = 0; i < AI_ARCH_CBO_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) !=
            (seed + (i * 0x1111111111111111ULL))) {
            return false;
        }
    }

    return true;
}

static inline bool ai_arch_block_at_matches_seed(uintptr_t paddr, uint64_t seed)
{
    goto_priv(PRIV_M);
    for (size_t i = 0; i < AI_ARCH_CBO_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) !=
            (seed + (i * 0x1111111111111111ULL))) {
            return false;
        }
    }

    return true;
}

static inline bool ai_arch_block_at_all_zero(uintptr_t paddr)
{
    goto_priv(PRIV_M);
    for (size_t i = 0; i < AI_ARCH_CBO_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) != 0ULL) {
            return false;
        }
    }

    return true;
}

#define AI_ARCH_CBO_MPRV_MPP_MASK (3ULL << 11)

#define AI_ARCH_CBO_MPRV_INSN(name, imm) \
static inline void name(uintptr_t addr, uint64_t mpp_bits) \
{ \
    const uint64_t clear_mprv_mpp = MSTATUS_MPRV | \
        AI_ARCH_CBO_MPRV_MPP_MASK; \
    const uint64_t set_mprv_mpp = MSTATUS_MPRV | \
        (mpp_bits & AI_ARCH_CBO_MPRV_MPP_MASK); \
    asm volatile( \
        "csrc mstatus, %0\n\t" \
        "csrs mstatus, %1\n\t" \
        ".insn i 0x0f, 0x2, x0, %2, " #imm "\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %0\n\t" \
        : \
        : "r"(clear_mprv_mpp), "r"(set_mprv_mpp), "r"(addr) \
        : "memory"); \
}

AI_ARCH_CBO_MPRV_INSN(ai_arch_cbo_mprv_clean, 1)
AI_ARCH_CBO_MPRV_INSN(ai_arch_cbo_mprv_flush, 2)
AI_ARCH_CBO_MPRV_INSN(ai_arch_cbo_mprv_inval, 0)
AI_ARCH_CBO_MPRV_INSN(ai_arch_cbo_mprv_zero, 4)

#define AI_ARCH_CBO_CSRR_INST(csr) \
    (((uint32_t)(csr) << 20) | (2U << 12) | (5U << 7) | 0x73U)
#define AI_ARCH_CBO_CSRW_INST(csr) \
    (((uint32_t)(csr) << 20) | (5U << 15) | (1U << 12) | 0x73U)
#define AI_ARCH_CBO_CLEAN_T0_INST \
    (((uint32_t)1U << 20) | (5U << 15) | (2U << 12) | 0x0fU)
#define AI_ARCH_CBO_FLUSH_T0_INST \
    (((uint32_t)2U << 20) | (5U << 15) | (2U << 12) | 0x0fU)
#define AI_ARCH_CBO_INVAL_T0_INST \
    (((uint32_t)0U << 20) | (5U << 15) | (2U << 12) | 0x0fU)
#define AI_ARCH_CBO_ZERO_T0_INST \
    (((uint32_t)4U << 20) | (5U << 15) | (2U << 12) | 0x0fU)

static inline uint64_t ai_arch_cbo_csrr_menvcfg(void)
{
    uint64_t value;

    asm volatile("csrr %0, menvcfg" : "=r"(value) :: "memory");
    return value;
}

static inline uint64_t ai_arch_cbo_csrr_senvcfg(void)
{
    uint64_t value;

    asm volatile("csrr %0, senvcfg" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_arch_cbo_csrw_menvcfg(uint64_t value)
{
    asm volatile("csrw menvcfg, %0" :: "rK"(value) : "memory");
}

static inline void ai_arch_cbo_csrw_senvcfg(uint64_t value)
{
    asm volatile("csrw senvcfg, %0" :: "rK"(value) : "memory");
}

static inline void ai_arch_cbo_fixed_csrr_menvcfg(void)
{
    asm volatile("csrr t0, menvcfg" ::: "t0", "memory");
}

static inline void ai_arch_cbo_fixed_csrw_menvcfg(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw menvcfg, t0\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline void ai_arch_cbo_fixed_csrr_senvcfg(void)
{
    asm volatile("csrr t0, senvcfg" ::: "t0", "memory");
}

static inline void ai_arch_cbo_fixed_csrw_senvcfg(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw senvcfg, t0\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline bool ai_arch_cbo_check_illegal_with_inst(uint32_t expected_inst)
{
    return excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        (uint32_t)excpt.tval == expected_inst &&
        excpt.epc != 0;
}

static inline void ai_arch_cbo_fixed_clean_t0(uintptr_t addr)
{
    asm volatile(
        "mv t0, %0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        :: "r"(addr)
        : "t0", "memory");
}

static inline void ai_arch_cbo_fixed_flush_t0(uintptr_t addr)
{
    asm volatile(
        "mv t0, %0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        :: "r"(addr)
        : "t0", "memory");
}

static inline void ai_arch_cbo_fixed_inval_t0(uintptr_t addr)
{
    asm volatile(
        "mv t0, %0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        :: "r"(addr)
        : "t0", "memory");
}

static inline void ai_arch_cbo_fixed_zero_t0(uintptr_t addr)
{
    asm volatile(
        "mv t0, %0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        :: "r"(addr)
        : "t0", "memory");
}

static inline uint64_t ai_arch_cbo_ld_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value)
        : "r"(addr)
        : "memory");
    return value;
}

static inline uint64_t ai_arch_cbo_clean_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        "mv t1, %[sentinel]\n\t"
        ".insn i 0x0f, 0x2, x0, %[addr], 1\n\t"
        "mv %[value], t1\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "t1", "memory");
    return value;
}

static inline uint64_t ai_arch_cbo_flush_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        "mv t1, %[sentinel]\n\t"
        ".insn i 0x0f, 0x2, x0, %[addr], 2\n\t"
        "mv %[value], t1\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "t1", "memory");
    return value;
}

static inline uint64_t ai_arch_cbo_inval_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        "mv t1, %[sentinel]\n\t"
        ".insn i 0x0f, 0x2, x0, %[addr], 0\n\t"
        "mv %[value], t1\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "t1", "memory");
    return value;
}

static inline uint64_t ai_arch_cbo_repeat_load_pair_probe(uintptr_t addr,
    uint64_t *second_out)
{
    uint64_t first;
    uint64_t second;

    asm volatile(
        "ld %[first], 0(%[addr])\n\t"
        "ld %[second], 0(%[addr])\n\t"
        : [first] "=&r"(first), [second] "=&r"(second)
        : [addr] "r"(addr)
        : "memory");
    *second_out = second;
    return first;
}

static inline uint64_t ai_arch_cbo_csrr_spfctl(void)
{
    uint64_t value;

    asm volatile("csrr %0, 0x5c1" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_arch_cbo_csrw_spfctl(uint64_t value)
{
    asm volatile("csrw 0x5c1, %0" :: "rK"(value) : "memory");
}

static inline uint64_t ai_arch_cbo_csrr_smblockctl(void)
{
    uint64_t value;

    asm volatile("csrr %0, 0x5c3" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_arch_cbo_csrw_smblockctl(uint64_t value)
{
    asm volatile("csrw 0x5c3, %0" :: "rK"(value) : "memory");
}

static inline bool ai_arch_cbo_envcfg_cbom_bits_are_legal(uint64_t value)
{
    uint64_t cbie = value & MENVCFG_CBIE11;

    return cbie == 0ULL ||
        cbie == MENVCFG_CBIE01 ||
        cbie == MENVCFG_CBIE11;
}

static inline bool ai_arch_cbo_envcfg_non_cbo_preserved(uint64_t before,
    uint64_t after)
{
    return (before & ~AI_ARCH_CBO_ENVCFG_CBO_MASK) ==
        (after & ~AI_ARCH_CBO_ENVCFG_CBO_MASK);
}

bool ai_arch_cbo_envcfg_cbo_field_warl_privilege_matrix_corner()
{
    TEST_START();

    const uint32_t csrr_menvcfg_inst = AI_ARCH_CBO_CSRR_INST(CSR_MENVCFG);
    const uint32_t csrw_menvcfg_inst = AI_ARCH_CBO_CSRW_INST(CSR_MENVCFG);
    const uint32_t csrr_senvcfg_inst = AI_ARCH_CBO_CSRR_INST(CSR_SENVCFG);
    const uint32_t csrw_senvcfg_inst = AI_ARCH_CBO_CSRW_INST(CSR_SENVCFG);

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = ai_arch_cbo_csrr_menvcfg();
    uint64_t original_senvcfg = ai_arch_cbo_csrr_senvcfg();
    uint64_t original_mstateen0 = CSRR(CSR_MSTATEEN0);
    uint64_t base_menvcfg =
        original_menvcfg & ~AI_ARCH_CBO_ENVCFG_CBO_MASK;
    uint64_t base_senvcfg =
        original_senvcfg & ~AI_ARCH_CBO_ENVCFG_CBO_MASK;
    uint64_t legal_cbo_bits =
        MENVCFG_CBZE | MENVCFG_CBZFE | MENVCFG_CBIE01;
    uint64_t legal_cbo_bits_alt =
        MENVCFG_CBZE | MENVCFG_CBZFE | MENVCFG_CBIE11;

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | legal_cbo_bits);
    uint64_t menvcfg_legal = ai_arch_cbo_csrr_menvcfg();
    AI_ASSERT_NO_EXCEPT("M-mode can write/read legal MENVCFG CBO enable bits");
    TEST_ASSERT("legal MENVCFG CBZE/CBZFE/CBIE=01 bits read back while non-CBO fields stay unchanged",
        (menvcfg_legal & AI_ARCH_CBO_ENVCFG_CBO_MASK) == legal_cbo_bits &&
        ai_arch_cbo_envcfg_non_cbo_preserved(base_menvcfg, menvcfg_legal));

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_csrw_senvcfg(base_senvcfg | legal_cbo_bits);
    uint64_t senvcfg_legal = ai_arch_cbo_csrr_senvcfg();
    AI_ASSERT_NO_EXCEPT("M-mode can write/read legal SENVCFG CBO enable bits");
    TEST_ASSERT("legal SENVCFG CBZE/CBZFE/CBIE=01 bits read back while non-CBO fields stay unchanged",
        (senvcfg_legal & AI_ARCH_CBO_ENVCFG_CBO_MASK) == legal_cbo_bits &&
        ai_arch_cbo_envcfg_non_cbo_preserved(base_senvcfg, senvcfg_legal));

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | legal_cbo_bits_alt);
    uint64_t menvcfg_alt = ai_arch_cbo_csrr_menvcfg();
    AI_ASSERT_NO_EXCEPT("M-mode can write/read MENVCFG CBIE=11 together with CBZE/CBZFE");
    TEST_ASSERT("MENVCFG CBIE=11 does not disturb CBZE/CBZFE or non-CBO fields",
        (menvcfg_alt & AI_ARCH_CBO_ENVCFG_CBO_MASK) == legal_cbo_bits_alt &&
        ai_arch_cbo_envcfg_non_cbo_preserved(base_menvcfg, menvcfg_alt));

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBZE);
    uint64_t menvcfg_cbze_only = ai_arch_cbo_csrr_menvcfg();
    ai_arch_cbo_csrw_menvcfg(menvcfg_cbze_only | MENVCFG_CBZFE);
    uint64_t menvcfg_cbze_cbzfe = ai_arch_cbo_csrr_menvcfg();
    ai_arch_cbo_csrw_menvcfg(menvcfg_cbze_cbzfe | MENVCFG_CBIE01);
    uint64_t menvcfg_all_stepwise = ai_arch_cbo_csrr_menvcfg();
    AI_ASSERT_NO_EXCEPT("stepwise MENVCFG CBO field programming completes without trapping");
    TEST_ASSERT("MENVCFG CBO bits are isolated: setting CBZFE/CBIE does not clear CBZE and final mask is exact",
        (menvcfg_cbze_only & AI_ARCH_CBO_ENVCFG_CBO_MASK) == MENVCFG_CBZE &&
        (menvcfg_cbze_cbzfe & AI_ARCH_CBO_ENVCFG_CBO_MASK) ==
            (MENVCFG_CBZE | MENVCFG_CBZFE) &&
        (menvcfg_all_stepwise & AI_ARCH_CBO_ENVCFG_CBO_MASK) ==
            legal_cbo_bits);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_csrw_senvcfg(base_senvcfg | MENVCFG_CBIE11);
    uint64_t senvcfg_cbie_only = ai_arch_cbo_csrr_senvcfg();
    ai_arch_cbo_csrw_senvcfg(senvcfg_cbie_only | MENVCFG_CBZE);
    uint64_t senvcfg_cbie_cbze = ai_arch_cbo_csrr_senvcfg();
    ai_arch_cbo_csrw_senvcfg(senvcfg_cbie_cbze | MENVCFG_CBZFE);
    uint64_t senvcfg_all_stepwise = ai_arch_cbo_csrr_senvcfg();
    AI_ASSERT_NO_EXCEPT("stepwise SENVCFG CBO field programming completes without trapping");
    TEST_ASSERT("SENVCFG CBO bits are isolated: setting CBZE/CBZFE does not clear CBIE and final mask is exact",
        (senvcfg_cbie_only & AI_ARCH_CBO_ENVCFG_CBO_MASK) == MENVCFG_CBIE11 &&
        (senvcfg_cbie_cbze & AI_ARCH_CBO_ENVCFG_CBO_MASK) ==
            (MENVCFG_CBIE11 | MENVCFG_CBZE) &&
        (senvcfg_all_stepwise & AI_ARCH_CBO_ENVCFG_CBO_MASK) ==
            legal_cbo_bits_alt);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | AI_ARCH_CBO_ENVCFG_ILLEGAL_PATTERN);
    uint64_t menvcfg_reserved = ai_arch_cbo_csrr_menvcfg();
    AI_ASSERT_NO_EXCEPT("writing a non-CBO reserved MENVCFG pattern WARL-normalizes without trapping");
    TEST_ASSERT("reserved MENVCFG pattern does not leak into CBO bits and CBIE remains a legal encoding",
        (menvcfg_reserved & AI_ARCH_CBO_ENVCFG_CBO_MASK) == 0ULL &&
        ai_arch_cbo_envcfg_cbom_bits_are_legal(menvcfg_reserved));

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_csrw_senvcfg(base_senvcfg | AI_ARCH_CBO_ENVCFG_ILLEGAL_PATTERN);
    uint64_t senvcfg_reserved = ai_arch_cbo_csrr_senvcfg();
    AI_ASSERT_NO_EXCEPT("writing a non-CBO reserved SENVCFG pattern WARL-normalizes without trapping");
    TEST_ASSERT("reserved SENVCFG pattern does not leak into CBO bits and CBIE remains a legal encoding",
        (senvcfg_reserved & AI_ARCH_CBO_ENVCFG_CBO_MASK) == 0ULL &&
        ai_arch_cbo_envcfg_cbom_bits_are_legal(senvcfg_reserved));

    CSRS(CSR_MSTATEEN0, MSTATEEN_ENVCFG);
    hspt_init();
    sfence_vma();

    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBZE);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(ai_arch_cbo_target(VSRW_GRWX));
    AI_ASSERT_NO_EXCEPT("with only CBZE enabled, HS cbo.zero succeeds");

    TEST_SETUP_EXCEPT();
    cbo_clean(ai_arch_cbo_target(VSRWX_GRWX));
    TEST_ASSERT("with only CBZE enabled, HS cbo.clean stays disabled and reports IllegalInstruction",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    TEST_SETUP_EXCEPT();
    cbo_flush(ai_arch_cbo_target(VSRWX_GRWX));
    TEST_ASSERT("with only CBZE enabled, HS cbo.flush stays disabled and reports IllegalInstruction",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    TEST_SETUP_EXCEPT();
    cbo_inval(ai_arch_cbo_target(VSRWX_GRWX));
    TEST_ASSERT("with only CBZE enabled, HS cbo.inval stays disabled and reports IllegalInstruction",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBZFE);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(ai_arch_cbo_target(VSRWX_GRWX));
    AI_ASSERT_NO_EXCEPT("with only CBZFE enabled, HS cbo.clean succeeds");

    TEST_SETUP_EXCEPT();
    cbo_flush(ai_arch_cbo_target(VSRWX_GRWX));
    AI_ASSERT_NO_EXCEPT("with only CBZFE enabled, HS cbo.flush succeeds");

    TEST_SETUP_EXCEPT();
    cbo_zero(ai_arch_cbo_target(VSRW_GRWX));
    TEST_ASSERT("with only CBZFE enabled, HS cbo.zero stays disabled and reports IllegalInstruction",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    TEST_SETUP_EXCEPT();
    cbo_inval(ai_arch_cbo_target(VSRWX_GRWX));
    TEST_ASSERT("with only CBZFE enabled, HS cbo.inval stays disabled and reports IllegalInstruction",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBIE01);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(ai_arch_cbo_target(VSRWX_GRWX));
    AI_ASSERT_NO_EXCEPT("with only CBIE enabled, HS cbo.inval succeeds");

    TEST_SETUP_EXCEPT();
    cbo_zero(ai_arch_cbo_target(VSRW_GRWX));
    TEST_ASSERT("with only CBIE enabled, HS cbo.zero stays disabled and reports IllegalInstruction",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    TEST_SETUP_EXCEPT();
    cbo_clean(ai_arch_cbo_target(VSRWX_GRWX));
    TEST_ASSERT("with only CBIE enabled, HS cbo.clean stays disabled and reports IllegalInstruction",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    TEST_SETUP_EXCEPT();
    cbo_flush(ai_arch_cbo_target(VSRWX_GRWX));
    TEST_ASSERT("with only CBIE enabled, HS cbo.flush stays disabled and reports IllegalInstruction",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg);
    goto_priv(PRIV_HS);
    uintptr_t disabled_no_r_target = ai_arch_cbo_target(VSX_GURWX);
    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_clean_t0(disabled_no_r_target);
    TEST_ASSERT("with CBOM disabled, HS cbo.clean reports IllegalInstruction before the no-R leaf can surface a page fault and tval carries the trapped instruction instead of the address",
        ai_arch_cbo_check_illegal_with_inst(AI_ARCH_CBO_CLEAN_T0_INST) &&
        excpt.tval != disabled_no_r_target);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBZFE);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(ai_arch_cbo_target(VSX_GURWX));
    TEST_ASSERT("after enabling CBOM, the same no-R HS cbo.clean target exposes the translated page fault instead of the earlier env-disabled illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == ai_arch_cbo_target(VSX_GURWX));

    goto_priv(PRIV_M);
    uint64_t menvcfg_before_illegal = ai_arch_cbo_csrr_menvcfg();
    uint64_t senvcfg_before_illegal = ai_arch_cbo_csrr_senvcfg();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_csrr_menvcfg();
    TEST_ASSERT("S-mode reading MENVCFG reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_arch_cbo_check_illegal_with_inst(csrr_menvcfg_inst));

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_csrw_menvcfg(base_menvcfg | legal_cbo_bits);
    TEST_ASSERT("S-mode writing MENVCFG reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_arch_cbo_check_illegal_with_inst(csrw_menvcfg_inst));

    TEST_SETUP_EXCEPT();
    uint64_t hs_senvcfg_read = ai_arch_cbo_csrr_senvcfg();
    AI_ASSERT_NO_EXCEPT("S-mode can read SENVCFG after the ENVCFG state is enabled by M-mode");

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_csrw_senvcfg(base_senvcfg | legal_cbo_bits);
    uint64_t hs_senvcfg_after_write = ai_arch_cbo_csrr_senvcfg();
    AI_ASSERT_NO_EXCEPT("S-mode can write legal SENVCFG CBO bits after the ENVCFG state is enabled");
    TEST_ASSERT("S-mode SENVCFG write updates only the CBO enable mask",
        (hs_senvcfg_read & ~AI_ARCH_CBO_ENVCFG_CBO_MASK) ==
            (hs_senvcfg_after_write & ~AI_ARCH_CBO_ENVCFG_CBO_MASK) &&
        (hs_senvcfg_after_write & AI_ARCH_CBO_ENVCFG_CBO_MASK) ==
            legal_cbo_bits);

    goto_priv(PRIV_M);
    TEST_ASSERT("S-mode MENVCFG illegal CSR probes do not commit the attempted MENVCFG update",
        ai_arch_cbo_csrr_menvcfg() == menvcfg_before_illegal);

    uint64_t senvcfg_before_u_illegal = ai_arch_cbo_csrr_senvcfg();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_csrr_menvcfg();
    TEST_ASSERT("U-mode reading MENVCFG reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_arch_cbo_check_illegal_with_inst(csrr_menvcfg_inst));

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_csrw_menvcfg(base_menvcfg | legal_cbo_bits_alt);
    TEST_ASSERT("U-mode writing MENVCFG reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_arch_cbo_check_illegal_with_inst(csrw_menvcfg_inst));

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_csrr_senvcfg();
    TEST_ASSERT("U-mode reading SENVCFG reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_arch_cbo_check_illegal_with_inst(csrr_senvcfg_inst));

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_csrw_senvcfg(base_senvcfg | legal_cbo_bits_alt);
    TEST_ASSERT("U-mode writing SENVCFG reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_arch_cbo_check_illegal_with_inst(csrw_senvcfg_inst));

    goto_priv_u_to_m_direct(PRIV_M);
    TEST_ASSERT("U-mode envcfg illegal CSR probes do not commit MENVCFG or SENVCFG writes",
        ai_arch_cbo_csrr_menvcfg() == menvcfg_before_illegal &&
        ai_arch_cbo_csrr_senvcfg() == senvcfg_before_u_illegal &&
        senvcfg_before_illegal != 0ULL);

    ai_arch_cbo_csrw_menvcfg(original_menvcfg);
    ai_arch_cbo_csrw_senvcfg(original_senvcfg);
    CSRW(CSR_MSTATEEN0, original_mstateen0);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cbo_disabled_illegal_priority_tval_instruction_no_address_side_effect_corner()
{
    TEST_START();

    const enum test_page page = VSX_GURWX;
    const uintptr_t target_vaddr = ai_arch_cbo_target(page);
    const uintptr_t target_paddr = ai_arch_cbo_paddr(page);
    const uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));
    const uint64_t seed = 0x1cc0badd1e9a11c0ULL;
    const uint64_t adjacent_seed = 0x1cc0badd1e9a11c1ULL;

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = ai_arch_cbo_csrr_menvcfg();
    uint64_t base_menvcfg =
        original_menvcfg & ~AI_ARCH_CBO_ENVCFG_CBO_MASK;
    ai_arch_seed_block(page, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_zero_t0(target_vaddr);
    TEST_ASSERT("with CBO.ZERO disabled, cbo.zero reports IllegalInstruction before the no-W leaf can surface a page fault and tval carries the trapped instruction rather than the address",
        ai_arch_cbo_check_illegal_with_inst(AI_ARCH_CBO_ZERO_T0_INST) &&
        excpt.tval != target_vaddr);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_clean_t0(target_vaddr);
    TEST_ASSERT("with CBOM disabled, cbo.clean reports IllegalInstruction before the no-R leaf can surface a page fault and tval carries the trapped instruction rather than the address",
        ai_arch_cbo_check_illegal_with_inst(AI_ARCH_CBO_CLEAN_T0_INST) &&
        excpt.tval != target_vaddr);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_flush_t0(target_vaddr);
    TEST_ASSERT("with CBOM disabled, cbo.flush also reports IllegalInstruction before the no-R leaf can surface a page fault and tval carries the trapped instruction rather than the address",
        ai_arch_cbo_check_illegal_with_inst(AI_ARCH_CBO_FLUSH_T0_INST) &&
        excpt.tval != target_vaddr);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_fixed_inval_t0(target_vaddr);
    TEST_ASSERT("with CBIE disabled, cbo.inval reports IllegalInstruction before the no-R leaf can surface a page fault and tval carries the trapped instruction rather than the address",
        ai_arch_cbo_check_illegal_with_inst(AI_ARCH_CBO_INVAL_T0_INST) &&
        excpt.tval != target_vaddr);

    TEST_ASSERT("the disabled-CBO illegal paths leave the target and adjacent cache blocks unchanged",
        ai_arch_block_matches_seed(page, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed));

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBZE);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(target_vaddr);
    TEST_ASSERT("after enabling CBO.ZERO, the same no-W target exposes fresh Store PF metadata instead of reusing illegal-instruction state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == target_vaddr);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBZFE);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(target_vaddr);
    TEST_ASSERT("after enabling CBOM, the same no-R target exposes Store PF with address-side tval instead of reusing the illegal-instruction metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == target_vaddr);

    TEST_SETUP_EXCEPT();
    cbo_flush(target_vaddr);
    TEST_ASSERT("after enabling CBOM, the same no-R target exposes fresh Store PF metadata for cbo.flush too",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == target_vaddr);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBIE01);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(target_vaddr);
    TEST_ASSERT("after enabling CBIE, the same no-R target exposes fresh Store PF metadata for cbo.inval instead of reusing illegal-instruction state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == target_vaddr);

    TEST_ASSERT("the later CBO page-fault paths also leave the target and adjacent cache blocks unchanged",
        ai_arch_block_matches_seed(page, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed));

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(original_menvcfg);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cbo_mprv_effective_privilege_permission_path_corner()
{
    TEST_START();

    const uintptr_t allowed_vaddr = ai_arch_cbo_target(VSRWX_GRWX);
    const uintptr_t no_r_vaddr = ai_arch_cbo_target(VSX_GURWX);
    const uintptr_t no_a_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE);
    const uintptr_t pmp_vaddr = ai_arch_cbo_target(SWITCH2);
    const uintptr_t u_allowed_vaddr = ai_arch_cbo_target(VSURWX_GURWX);
    const uintptr_t u_no_r_vaddr = ai_arch_cbo_target(VSUX_GURWX);
    const uintptr_t zero_allowed_vaddr = ai_arch_cbo_target(VSRW_GRWX);
    const uintptr_t zero_no_w_vaddr = ai_arch_cbo_target(VSRX_GRWX);
    const uintptr_t zero_allowed_paddr = ai_arch_cbo_paddr(VSRW_GRWX);
    const uintptr_t pmp_paddr = phys_page_base(SWITCH2);
    const uint64_t no_r_seed = 0x4d50525643424f31ULL;
    const uint64_t no_a_seed = 0x4d50525643424f32ULL;
    const uint64_t pmp_seed = 0x4d50525643424f33ULL;
    const uint64_t u_no_r_seed = 0x4d50525643424f34ULL;
    const uint64_t zero_seed = 0x4d50525643424f35ULL;
    const uint64_t zero_no_w_seed = 0x4d50525643424f36ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM | MSTATUS_MPRV | (3ULL << 11));
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE | MENVCFG_CBIE01 | MENVCFG_CBZE);
    hspt_init();
    ai_arch_seed_block(VSX_GURWX, no_r_seed);
    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, no_a_seed);
    ai_arch_seed_block(SWITCH2, pmp_seed);
    ai_arch_seed_block(VSUX_GURWX, u_no_r_seed);
    ai_arch_seed_block(VSRW_GRWX, zero_seed);
    ai_arch_seed_block(VSRX_GRWX, zero_no_w_seed);
    *ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_CBOM_PAGE) &= ~PTE_A;
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_clean(allowed_vaddr, MSTATUS_MPP);
    AI_ASSERT_NO_EXCEPT("M-mode cbo.clean with MPRV=1 and MPP=S uses the effective-S translation path and succeeds on an S-readable leaf");

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_flush(allowed_vaddr, MSTATUS_MPP);
    AI_ASSERT_NO_EXCEPT("M-mode cbo.flush with MPRV=1 and MPP=S also succeeds on the same S-readable leaf");

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_inval(allowed_vaddr, MSTATUS_MPP);
    AI_ASSERT_NO_EXCEPT("M-mode cbo.inval with MPRV=1 and MPP=S also succeeds on the same S-readable leaf when CBIE is enabled");

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_clean(no_r_vaddr, MSTATUS_MPP);
    AI_ASSERT_EXCEPT("M-mode cbo.clean with MPRV=1 and MPP=S reports Store PF when the effective-S leaf has no R bit",
        CAUSE_SPF, no_r_vaddr);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_flush(no_a_vaddr, MSTATUS_MPP);
    AI_ASSERT_EXCEPT("M-mode cbo.flush with MPRV=1 and MPP=S reports Store PF when the effective-S leaf has A=0",
        CAUSE_SPF, no_a_vaddr);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_clean(u_allowed_vaddr, 0ULL);
    AI_ASSERT_NO_EXCEPT("M-mode cbo.clean with MPRV=1 and MPP=U uses the effective-U translation path and succeeds on a PTE.U readable leaf");

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_clean(u_no_r_vaddr, 0ULL);
    AI_ASSERT_EXCEPT("M-mode cbo.clean with MPRV=1 and MPP=U reports Store PF when the effective-U leaf has no R bit",
        CAUSE_SPF, u_no_r_vaddr);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_zero(zero_allowed_vaddr, MSTATUS_MPP);
    AI_ASSERT_NO_EXCEPT("M-mode cbo.zero with MPRV=1 and MPP=S uses the effective-S write-permission path and succeeds on a writable leaf");

    TEST_ASSERT("MPRV effective-S cbo.zero clears the addressed block through the translated S leaf",
        read64(zero_allowed_paddr) == 0ULL);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_zero(zero_no_w_vaddr, MSTATUS_MPP);
    AI_ASSERT_EXCEPT("M-mode cbo.zero with MPRV=1 and MPP=S reports Store PF when the effective-S leaf has no W bit",
        CAUSE_SPF, zero_no_w_vaddr);

    TEST_ASSERT("MPRV effective-S CBO page-fault probes leave the no-R and no-A blocks unchanged",
        ai_arch_block_matches_seed(VSX_GURWX, no_r_seed) &&
        ai_arch_block_matches_seed(AI_ARCH_CBO_CBOM_PAGE, no_a_seed) &&
        ai_arch_block_matches_seed(VSUX_GURWX, u_no_r_seed) &&
        ai_arch_block_matches_seed(VSRX_GRWX, zero_no_w_seed));

    *ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_CBOM_PAGE) |= PTE_A;
    sfence_vma();
    ai_arch_configure_pmp_window(pmp_paddr, pmp_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    TEST_SETUP_EXCEPT();
    ai_arch_cbo_mprv_inval(pmp_vaddr, MSTATUS_MPP);
    AI_ASSERT_EXCEPT("M-mode cbo.inval with MPRV=1 and MPP=S reports Store AF when effective-S translation succeeds but PMP denies read permission",
        CAUSE_SAF, pmp_vaddr);

    TEST_ASSERT("the MPRV effective-S CBO PMP fault leaves the target block unchanged",
        ai_arch_block_matches_seed(SWITCH2, pmp_seed));

    ai_arch_restore_pmp_full_access();
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE | MENVCFG_CBIE01 | MENVCFG_CBZE);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | (3ULL << 11));
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cbo_clean_rx_allowed()
{
    TEST_START();

    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    TEST_SETUP_EXCEPT();
    cbo_clean(ai_arch_cbo_target(VSRWX_GURWX));
    if (excpt.triggered) {
        ai_arch_log_unexpected_cbo_result("unexpected exception while probing HS cbo.clean enablement");
    }
    AI_ASSERT_NO_EXCEPT("cbo.clean executes in HS mode after CBZFE is enabled");

    TEST_SETUP_EXCEPT();
    cbo_clean(ai_arch_cbo_target(VSRX_GURWX));
    if (excpt.triggered) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.clean result on an RX page");
    }
    AI_ASSERT_NO_EXCEPT("cbo.clean remains permitted when the target page keeps R+A but drops W");

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_flush_rx_allowed()
{
    TEST_START();

    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    TEST_SETUP_EXCEPT();
    cbo_flush(ai_arch_cbo_target(VSRWX_GURWX));
    if (excpt.triggered) {
        ai_arch_log_unexpected_cbo_result("unexpected exception while probing HS cbo.flush enablement");
    }
    AI_ASSERT_NO_EXCEPT("cbo.flush executes in HS mode after CBZFE is enabled");

    TEST_SETUP_EXCEPT();
    cbo_flush(ai_arch_cbo_target(VSRX_GURWX));
    if (excpt.triggered) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.flush result on an RX page");
    }
    AI_ASSERT_NO_EXCEPT("cbo.flush remains permitted when the target page keeps R+A but drops W");

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_inval_rx_allowed()
{
    TEST_START();

    ai_arch_prepare_hs_cbo_env(MENVCFG_CBIE01);

    TEST_SETUP_EXCEPT();
    cbo_inval(ai_arch_cbo_target(VSRWX_GURWX));
    if (excpt.triggered) {
        ai_arch_log_unexpected_cbo_result("unexpected exception while probing HS cbo.inval enablement");
    }
    AI_ASSERT_NO_EXCEPT("cbo.inval executes in HS mode after CBIE is enabled");

    TEST_SETUP_EXCEPT();
    cbo_inval(ai_arch_cbo_target(VSRX_GURWX));
    if (excpt.triggered) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.inval result on an RX page");
    }
    AI_ASSERT_NO_EXCEPT("cbo.inval remains permitted when the target page keeps R+A but drops W");

    ai_arch_reset_hs_cbo_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_arch_cbo_success_no_gpr_or_trap_csr_update_corner()
{
    TEST_START();

    const enum test_page cbom_page = VSRX_GURWX;
    const enum test_page zero_page = VSRW_GRWX;
    const uintptr_t cbom_vaddr = ai_arch_cbo_target(cbom_page);
    const uintptr_t cbom_paddr = ai_arch_cbo_paddr(cbom_page);
    const uintptr_t zero_vaddr = ai_arch_cbo_target(zero_page);
    const uintptr_t zero_paddr = ai_arch_cbo_paddr(zero_page);
    const uint64_t cbom_seed = 0x43424f4d4e4f4750ULL;
    const uint64_t zero_seed = 0x5a45524f4e4f4750ULL;
    const uint64_t rd_clean_sentinel = 0x1111222233334444ULL;
    const uint64_t rd_flush_sentinel = 0x5555666677778888ULL;
    const uint64_t rd_inval_sentinel = 0x9999aaaabbbbccccULL;
    const uint64_t rd_zero_sentinel = 0xddddeeeeffff0001ULL;
    const uint64_t scause_seed = 0x345ULL;
    const uint64_t stval_seed = 0x5a5a5a5aULL;
    const uint64_t sepc_seed = (uint64_t)&&cbom_success_after_probes;

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = ai_arch_cbo_csrr_menvcfg();
    ai_arch_seed_block(cbom_page, cbom_seed);
    ai_arch_seed_block(zero_page, zero_seed);
    ai_arch_cbo_csrw_menvcfg(original_menvcfg |
        MENVCFG_CBZFE | MENVCFG_CBIE01 | MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_SCAUSE, scause_seed);
    CSRW(CSR_STVAL, stval_seed);
    CSRW(CSR_SEPC, sepc_seed);

    TEST_SETUP_EXCEPT();
    uint64_t clean_rd = ai_arch_cbo_clean_rd_probe(cbom_vaddr,
        rd_clean_sentinel);
    AI_ASSERT_NO_EXCEPT_AND("successful cbo.clean does not write rd and leaves preseeded S trap CSR metadata untouched",
        clean_rd == rd_clean_sentinel &&
        CSRR(CSR_SCAUSE) == scause_seed &&
        CSRR(CSR_STVAL) == stval_seed &&
        CSRR(CSR_SEPC) == sepc_seed);

    TEST_SETUP_EXCEPT();
    uint64_t flush_rd = ai_arch_cbo_flush_rd_probe(cbom_vaddr,
        rd_flush_sentinel);
    AI_ASSERT_NO_EXCEPT_AND("successful cbo.flush does not write rd and leaves preseeded S trap CSR metadata untouched",
        flush_rd == rd_flush_sentinel &&
        CSRR(CSR_SCAUSE) == scause_seed &&
        CSRR(CSR_STVAL) == stval_seed &&
        CSRR(CSR_SEPC) == sepc_seed);

    TEST_SETUP_EXCEPT();
    uint64_t inval_rd = ai_arch_cbo_inval_rd_probe(cbom_vaddr,
        rd_inval_sentinel);
    AI_ASSERT_NO_EXCEPT_AND("successful cbo.inval does not write rd and leaves preseeded S trap CSR metadata untouched",
        inval_rd == rd_inval_sentinel &&
        CSRR(CSR_SCAUSE) == scause_seed &&
        CSRR(CSR_STVAL) == stval_seed &&
        CSRR(CSR_SEPC) == sepc_seed);

    TEST_SETUP_EXCEPT();
    uint64_t zero_rd = rd_zero_sentinel;
    asm volatile(
        "mv t1, %[sentinel]\n\t"
        ".insn i 0x0f, 0x2, x0, %[addr], 4\n\t"
        "mv %[value], t1\n\t"
        : [value] "=&r"(zero_rd)
        : [addr] "r"(zero_vaddr), [sentinel] "r"(rd_zero_sentinel)
        : "t1", "memory");
    AI_ASSERT_NO_EXCEPT_AND("successful cbo.zero does not write a general register and leaves preseeded S trap CSR metadata untouched",
        zero_rd == rd_zero_sentinel &&
        CSRR(CSR_SCAUSE) == scause_seed &&
        CSRR(CSR_STVAL) == stval_seed &&
        CSRR(CSR_SEPC) == sepc_seed);

cbom_success_after_probes:
    goto_priv(PRIV_M);
    TEST_ASSERT("successful clean/flush/inval preserve their target block while successful cbo.zero clears only its target block",
        ai_arch_block_at_matches_seed(cbom_paddr, cbom_seed) &&
        ai_arch_block_at_all_zero(zero_paddr));

    ai_arch_cbo_csrw_menvcfg(original_menvcfg);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cbo_clean_no_r_page_fault()
{
    TEST_START();

    uintptr_t blocked_vaddr = ai_arch_cbo_target(VSX_GURWX);

    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    TEST_SETUP_EXCEPT();
    cbo_clean(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == blocked_vaddr)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.clean no-R fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.clean reports Store PF when the target page drops R while keeping A",
        CAUSE_SPF, blocked_vaddr);

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_clean_no_a_page_fault()
{
    TEST_START();

    const uint64_t seed = 0x6e6f41436c65616eULL;
    const uint64_t adjacent_seed = 0x6e6f41436c65616fULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t target_paddr = ai_arch_cbo_paddr(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));
    pte_t *leaf = ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_CBOM_PAGE);

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    goto_priv(PRIV_M);
    uint64_t leaf_before = *leaf;
    *leaf = leaf_before & ~PTE_A;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == blocked_vaddr)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.clean no-A fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.clean reports Store PF when the target page keeps R but clears A",
        CAUSE_SPF, blocked_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("failed cbo.clean with A=0 preserves target/adjacent blocks and leaves PTE A/D unchanged",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed) &&
        (*leaf & (PTE_A | PTE_D)) == (leaf_before & ~PTE_A & (PTE_A | PTE_D)));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_flush_no_r_page_fault()
{
    TEST_START();

    uintptr_t blocked_vaddr = ai_arch_cbo_target(VSX_GURWX);

    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    TEST_SETUP_EXCEPT();
    cbo_flush(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == blocked_vaddr)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.flush no-R fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.flush reports Store PF when the target page drops R while keeping A",
        CAUSE_SPF, blocked_vaddr);

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_flush_no_a_page_fault()
{
    TEST_START();

    const uint64_t seed = 0x6e6f41466c757368ULL;
    const uint64_t adjacent_seed = 0x6e6f41466c757369ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t target_paddr = ai_arch_cbo_paddr(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));
    pte_t *leaf = ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_CBOM_PAGE);

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    goto_priv(PRIV_M);
    uint64_t leaf_before = *leaf;
    *leaf = leaf_before & ~PTE_A;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == blocked_vaddr)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.flush no-A fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.flush reports Store PF when the target page keeps R but clears A",
        CAUSE_SPF, blocked_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("failed cbo.flush with A=0 preserves target/adjacent blocks and leaves PTE A/D unchanged",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed) &&
        (*leaf & (PTE_A | PTE_D)) == (leaf_before & ~PTE_A & (PTE_A | PTE_D)));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_inval_no_r_page_fault()
{
    TEST_START();

    uintptr_t blocked_vaddr = ai_arch_cbo_target(VSX_GURWX);

    ai_arch_prepare_hs_cbo_env(MENVCFG_CBIE01);

    TEST_SETUP_EXCEPT();
    cbo_inval(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == blocked_vaddr)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.inval no-R fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.inval reports Store PF when the target page drops R while keeping A",
        CAUSE_SPF, blocked_vaddr);

    ai_arch_reset_hs_cbo_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_arch_cbo_inval_no_a_page_fault()
{
    TEST_START();

    const uint64_t seed = 0x6e6f41496e76616cULL;
    const uint64_t adjacent_seed = 0x6e6f41496e76616dULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t target_paddr = ai_arch_cbo_paddr(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));
    pte_t *leaf = ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_CBOM_PAGE);

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBIE01);

    goto_priv(PRIV_M);
    uint64_t leaf_before = *leaf;
    *leaf = leaf_before & ~PTE_A;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == blocked_vaddr)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.inval no-A fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.inval reports Store PF when the target page keeps R but clears A",
        CAUSE_SPF, blocked_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("failed cbo.inval with A=0 preserves target/adjacent blocks and leaves PTE A/D unchanged",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed) &&
        (*leaf & (PTE_A | PTE_D)) == (leaf_before & ~PTE_A & (PTE_A | PTE_D)));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_arch_cbozero_no_w_page_fault()
{
    TEST_START();

    const uint64_t seed = 0x1021324354657687ULL;
    const uint64_t adjacent_seed = 0x1021324354657688ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(VSRX_GURWX) + 24U;
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t target_paddr = ai_arch_cbo_paddr(VSRX_GURWX);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));

    ai_arch_seed_block(VSRX_GURWX, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.zero no-W fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.zero reports Store PF with the faulting effective address in tval when W is missing",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed cbo.zero leaves target and adjacent blocks unchanged when W is missing",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_arch_cbozero_no_a_page_fault()
{
    TEST_START();

    const uint64_t seed = 0x2233445566778899ULL;
    const uint64_t adjacent_seed = 0x223344556677889aULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_ZERO_PAGE) + 24U;
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t target_paddr = ai_arch_cbo_paddr(AI_ARCH_CBO_ZERO_PAGE);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));
    pte_t *leaf = ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_ZERO_PAGE);

    ai_arch_seed_block(AI_ARCH_CBO_ZERO_PAGE, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZE);

    goto_priv(PRIV_M);
    uint64_t leaf_before = *leaf;
    *leaf = leaf_before & ~PTE_A;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.zero no-A fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.zero reports Store PF with the faulting effective address in tval when A is missing",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed cbo.zero leaves target and adjacent blocks unchanged when A is missing",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed));

    goto_priv(PRIV_M);
    TEST_ASSERT("failed cbo.zero with A=0 does not set PTE A or D",
        (*leaf & (PTE_A | PTE_D)) == (leaf_before & ~PTE_A & (PTE_A | PTE_D)));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_arch_cbozero_no_d_page_fault()
{
    TEST_START();

    const uint64_t seed = 0x33445566778899aaULL;
    const uint64_t adjacent_seed = 0x33445566778899abULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_ZERO_PAGE) + 24U;
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t target_paddr = ai_arch_cbo_paddr(AI_ARCH_CBO_ZERO_PAGE);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));
    pte_t *leaf = ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_ZERO_PAGE);

    ai_arch_seed_block(AI_ARCH_CBO_ZERO_PAGE, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZE);

    goto_priv(PRIV_M);
    uint64_t leaf_before = *leaf;
    *leaf = leaf_before & ~PTE_D;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.zero no-D fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.zero reports Store PF with the faulting effective address in tval when D is missing",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed cbo.zero leaves target and adjacent blocks unchanged when D is missing",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed));

    goto_priv(PRIV_M);
    TEST_ASSERT("failed cbo.zero with D=0 keeps PTE D clear and leaves PTE A unchanged",
        (*leaf & (PTE_A | PTE_D)) == (leaf_before & ~PTE_D & (PTE_A | PTE_D)));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_arch_cbo_clean_pmp_read_access_fault()
{
    TEST_START();

    const uint64_t seed = 0x445566778899aabbULL;
    const uint64_t adjacent_seed = 0x445566778899aabcULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t blocked_paddr = phys_page_base(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t target_paddr = ai_arch_cbo_paddr(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);
    ai_arch_configure_pmp_window(blocked_paddr, blocked_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SAF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.clean PMP-read fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.clean reports Store AF with the faulting effective address in tval when PMP denies reads",
        CAUSE_SAF, expected_tval);

    TEST_ASSERT("the failed cbo.clean leaves target and adjacent blocks unchanged when PMP denies reads",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_flush_pmp_read_access_fault()
{
    TEST_START();

    const uint64_t seed = 0x4f5e6d7c8b9aa1b0ULL;
    const uint64_t adjacent_seed = 0x4f5e6d7c8b9aa1b1ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t blocked_paddr = phys_page_base(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t target_paddr = ai_arch_cbo_paddr(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);
    ai_arch_configure_pmp_window(blocked_paddr, blocked_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SAF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.flush PMP-read fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.flush reports Store AF with the faulting effective address in tval when PMP denies reads",
        CAUSE_SAF, expected_tval);

    TEST_ASSERT("the failed cbo.flush leaves target and adjacent blocks unchanged when PMP denies reads",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_inval_pmp_read_access_fault()
{
    TEST_START();

    const uint64_t seed = 0x5061728394a5b6c7ULL;
    const uint64_t adjacent_seed = 0x5061728394a5b6c8ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t blocked_paddr = phys_page_base(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t target_paddr = ai_arch_cbo_paddr(AI_ARCH_CBO_CBOM_PAGE);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBIE01);
    ai_arch_configure_pmp_window(blocked_paddr, blocked_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SAF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.inval PMP-read fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.inval reports Store AF with the faulting effective address in tval when PMP denies reads",
        CAUSE_SAF, expected_tval);

    TEST_ASSERT("the failed cbo.inval leaves target and adjacent blocks unchanged when PMP denies reads",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_arch_cbozero_pmp_write_access_fault()
{
    TEST_START();

    const uint64_t seed = 0x5566778899aabbccULL;
    const uint64_t adjacent_seed = 0x5566778899aabbcdULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_ZERO_PAGE) + 24U;
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t blocked_paddr = phys_page_base(AI_ARCH_CBO_ZERO_PAGE);
    uintptr_t target_paddr = ai_arch_cbo_paddr(AI_ARCH_CBO_ZERO_PAGE);
    uintptr_t adjacent_paddr =
        target_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));

    ai_arch_seed_block(AI_ARCH_CBO_ZERO_PAGE, seed);
    ai_arch_seed_block_at(adjacent_paddr, adjacent_seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZE);
    ai_arch_configure_pmp_window(blocked_paddr, blocked_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SAF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected cbo.zero PMP-write fault classification");
    }
    AI_ASSERT_EXCEPT("cbo.zero reports Store AF with the faulting effective address in tval when PMP denies writes",
        CAUSE_SAF, expected_tval);

    TEST_ASSERT("the failed cbo.zero leaves target and adjacent blocks unchanged when PMP denies writes",
        ai_arch_block_at_matches_seed(target_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_paddr, adjacent_seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_arch_cbom_access_fault_preserves_custom_csr_state_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_CBO_CBOM_PAGE;
    const uintptr_t blocked_vaddr = ai_arch_cbo_target(page);
    const uintptr_t blocked_paddr = phys_page_base(page);
    const uintptr_t target_block_paddr = ai_arch_cbo_paddr(page);
    const uintptr_t adjacent_block_paddr =
        target_block_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));
    const uint64_t target_seed = 0x637573746f6d4342ULL;
    const uint64_t adjacent_seed = 0x637573746f6d4343ULL;

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = ai_arch_cbo_csrr_menvcfg();
    uint64_t original_spfctl = ai_arch_cbo_csrr_spfctl();
    uint64_t original_smblockctl = ai_arch_cbo_csrr_smblockctl();
    ai_arch_cbo_csrw_spfctl(AI_ARCH_CBO_SPFCTL_SAFE_PATTERN);
    ai_arch_cbo_csrw_smblockctl(AI_ARCH_CBO_SMBLOCKCTL_SAFE_PATTERN);
    uint64_t spfctl_seed = ai_arch_cbo_csrr_spfctl();
    uint64_t smblockctl_seed = ai_arch_cbo_csrr_smblockctl();
    ai_arch_seed_block(page, target_seed);
    ai_arch_seed_block_at(adjacent_block_paddr, adjacent_seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_cbo_csrw_menvcfg(original_menvcfg | MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
    ai_arch_configure_pmp_window(blocked_paddr, blocked_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(blocked_vaddr);
    AI_ASSERT_EXCEPT("cbo.flush reports Store AF with target VA when PMP denies CBOM read permission",
        CAUSE_SAF, blocked_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("CBOM Access Fault preserves target and adjacent blocks plus MemBlock custom CSR state",
        ai_arch_block_at_matches_seed(target_block_paddr, target_seed) &&
        ai_arch_block_at_matches_seed(adjacent_block_paddr, adjacent_seed) &&
        ai_arch_cbo_csrr_spfctl() == spfctl_seed &&
        ai_arch_cbo_csrr_smblockctl() == smblockctl_seed);

    ai_arch_restore_pmp_full_access();
    ai_arch_cbo_csrw_spfctl(original_spfctl);
    ai_arch_cbo_csrw_smblockctl(original_smblockctl);
    ai_arch_cbo_csrw_menvcfg(original_menvcfg);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cbo_clean_tail_offset_no_r_page_fault_tval_raw_rs1_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x66778899aabbccddULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(VSX_GURWX) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;

    ai_arch_seed_block(VSX_GURWX, seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.clean no-R fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.clean reports Store PF with the raw rs1 tail-offset in tval when the target block has no R permission",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.clean leaves the target block unchanged when R is missing",
        ai_arch_block_matches_seed(VSX_GURWX, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_clean_tail_offset_no_a_page_fault_tval_raw_rs1_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x69788796a5b4c3d2ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    goto_priv(PRIV_M);
    *ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_CBOM_PAGE) &= ~PTE_A;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.clean no-A fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.clean reports Store PF with the raw rs1 tail-offset in tval when the target block has A=0",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.clean leaves the target block unchanged when A is missing",
        ai_arch_block_matches_seed(AI_ARCH_CBO_CBOM_PAGE, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_clean_tail_offset_pmp_read_access_fault_tval_raw_rs1_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x6a7b8c9daebfc0d1ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t blocked_paddr = phys_page_base(AI_ARCH_CBO_CBOM_PAGE);

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);
    ai_arch_configure_pmp_window(blocked_paddr, blocked_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SAF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.clean PMP-read fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.clean reports Store AF with the raw rs1 tail-offset in tval when PMP denies reads",
        CAUSE_SAF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.clean leaves the target block unchanged when PMP denies reads",
        ai_arch_block_matches_seed(AI_ARCH_CBO_CBOM_PAGE, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_flush_tail_offset_no_r_page_fault_tval_raw_rs1_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x78899aabbccddeefULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(VSX_GURWX) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;

    ai_arch_seed_block(VSX_GURWX, seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.flush no-R fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.flush reports Store PF with the raw rs1 tail-offset in tval when the target block has no R permission",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.flush leaves the target block unchanged when R is missing",
        ai_arch_block_matches_seed(VSX_GURWX, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_flush_tail_offset_no_a_page_fault_tval_raw_rs1_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x778899aabbccddeeULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    goto_priv(PRIV_M);
    *ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_CBOM_PAGE) &= ~PTE_A;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.flush no-A fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.flush reports Store PF with the raw rs1 tail-offset in tval when the target block has A=0",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.flush leaves the target block unchanged when A is missing",
        ai_arch_block_matches_seed(AI_ARCH_CBO_CBOM_PAGE, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_flush_tail_offset_pmp_read_access_fault_tval_raw_rs1_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x7b8c9daebfc0d1e2ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t blocked_paddr = phys_page_base(AI_ARCH_CBO_CBOM_PAGE);

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);
    ai_arch_configure_pmp_window(blocked_paddr, blocked_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SAF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.flush PMP-read fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.flush reports Store AF with the raw rs1 tail-offset in tval when PMP denies reads",
        CAUSE_SAF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.flush leaves the target block unchanged when PMP denies reads",
        ai_arch_block_matches_seed(AI_ARCH_CBO_CBOM_PAGE, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_inval_tail_offset_no_r_page_fault_tval_raw_rs1_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x899aabbccddeef10ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(VSX_GURWX) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;

    ai_arch_seed_block(VSX_GURWX, seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.inval no-R fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.inval reports Store PF with the raw rs1 tail-offset in tval when the target block has no R permission",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.inval leaves the target block unchanged when R is missing",
        ai_arch_block_matches_seed(VSX_GURWX, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_arch_cbo_inval_tail_offset_no_a_page_fault_tval_raw_rs1_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x9aabbccddeef1021ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBIE01);

    goto_priv(PRIV_M);
    *ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_CBOM_PAGE) &= ~PTE_A;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.inval no-A fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.inval reports Store PF with the raw rs1 tail-offset in tval when the target block has A=0",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.inval leaves the target block unchanged when A is missing",
        ai_arch_block_matches_seed(AI_ARCH_CBO_CBOM_PAGE, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_arch_cbo_inval_tail_offset_pmp_read_access_fault_tval_raw_rs1_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x8899aabbccddeeffULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_CBOM_PAGE) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t blocked_paddr = phys_page_base(AI_ARCH_CBO_CBOM_PAGE);

    ai_arch_seed_block(AI_ARCH_CBO_CBOM_PAGE, seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBIE01);
    ai_arch_configure_pmp_window(blocked_paddr, blocked_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SAF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.inval PMP-read fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.inval reports Store AF with the raw rs1 tail-offset in tval when PMP denies reads",
        CAUSE_SAF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.inval leaves the target block unchanged when PMP denies reads",
        ai_arch_block_matches_seed(AI_ARCH_CBO_CBOM_PAGE, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_arch_cbozero_tail_offset_no_w_page_fault_tval_block_base_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0xaabbccddeeff1021ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(VSRX_GURWX) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;

    ai_arch_seed_block(VSRX_GURWX, seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.zero no-W fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.zero reports Store PF with the raw rs1 tail-offset in tval when the target block has no W permission",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.zero leaves the target block unchanged when W is missing",
        ai_arch_block_matches_seed(VSRX_GURWX, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_arch_cbozero_tail_offset_no_a_page_fault_tval_block_base_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0xbbccddeeff102132ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_ZERO_PAGE) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;

    ai_arch_seed_block(AI_ARCH_CBO_ZERO_PAGE, seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZE);

    goto_priv(PRIV_M);
    *ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_ZERO_PAGE) &= ~PTE_A;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.zero no-A fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.zero reports Store PF with the raw rs1 tail-offset in tval when the target block has A=0",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.zero leaves the target block unchanged when A is missing",
        ai_arch_block_matches_seed(AI_ARCH_CBO_ZERO_PAGE, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_arch_cbozero_tail_offset_no_d_page_fault_tval_block_base_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0xccddeeff10213243ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_ZERO_PAGE) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;

    ai_arch_seed_block(AI_ARCH_CBO_ZERO_PAGE, seed);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZE);

    goto_priv(PRIV_M);
    *ai_arch_cbo_hs_leaf_entry(AI_ARCH_CBO_ZERO_PAGE) &= ~PTE_D;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SPF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.zero no-D fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.zero reports Store PF with the raw rs1 tail-offset in tval when the target block has D=0",
        CAUSE_SPF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.zero leaves the target block unchanged when D is missing",
        ai_arch_block_matches_seed(AI_ARCH_CBO_ZERO_PAGE, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_arch_cbozero_tail_offset_pmp_write_access_fault_tval_block_base_spike_corner()
{
    TEST_START();

    const uint64_t seed = 0x99aabbccddeeff00ULL;
    uintptr_t blocked_vaddr = ai_arch_cbo_target(AI_ARCH_CBO_ZERO_PAGE) +
        AI_ARCH_CBO_TAIL_SUBBLOCK_OFFSET;
    uintptr_t expected_tval = blocked_vaddr;
    uintptr_t blocked_paddr = phys_page_base(AI_ARCH_CBO_ZERO_PAGE);

    ai_arch_seed_block(AI_ARCH_CBO_ZERO_PAGE, seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZE);
    ai_arch_configure_pmp_window(blocked_paddr, blocked_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(blocked_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_SAF &&
        excpt.tval == expected_tval)) {
        ai_arch_log_unexpected_cbo_result("unexpected tail-offset cbo.zero PMP-write fault classification");
    }
    AI_ASSERT_EXCEPT("on Spike, cbo.zero reports Store AF with the raw rs1 tail-offset in tval when PMP denies writes",
        CAUSE_SAF, expected_tval);

    TEST_ASSERT("the failed tail-offset cbo.zero leaves the target block unchanged when PMP denies writes",
        ai_arch_block_matches_seed(AI_ARCH_CBO_ZERO_PAGE, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_arch_cbo_envcfg_illegal_then_scalar_memory_independent_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t scalar_vaddr = hs_page_base(page) + 0x180U;
    const uintptr_t scalar_paddr = phys_page_base(page) + 0x180U;
    const uint64_t initial = 0x1122334455667788ULL;
    const uint64_t store_value = 0x8877665544332211ULL;
    const uint64_t amo_addend = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = ai_arch_cbo_csrr_menvcfg();
    uint64_t base_menvcfg =
        original_menvcfg & ~AI_ARCH_CBO_ENVCFG_CBO_MASK;
    ai_arch_cbo_csrw_menvcfg(base_menvcfg);
    write64(scalar_paddr, initial);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(ai_arch_cbo_target(page));
    TEST_ASSERT("with HS CBO clean disabled by MENVCFG, cbo.clean reports IllegalInstruction before touching the target page",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    TEST_SETUP_EXCEPT();
    uint64_t loaded = read64(scalar_vaddr);
    write64(scalar_vaddr, store_value);
    uint64_t amo_old = amoadd_d(scalar_vaddr, amo_addend);
    TEST_ASSERT("after the CBO enable IllegalInstruction, ordinary HS load/store/AMO make forward progress without inheriting the old CBO exception state",
        excpt.triggered == false &&
        loaded == initial &&
        amo_old == store_value &&
        read64(scalar_vaddr) == store_value + amo_addend);

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory reflects only the later scalar store and AMO, proving the earlier CBO illegal path did not poison non-CBO memory state",
        read64(scalar_paddr) == store_value + amo_addend);

    ai_arch_cbo_csrw_menvcfg(original_menvcfg);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cbo_fault_then_followup_success_metadata_isolated_corner()
{
    TEST_START();

    const uint64_t fault_seed = 0xa0b1c2d3e4f50617ULL;
    const uint64_t good_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_value = 0x76543210fedcba98ULL;
    const uintptr_t fault_vaddr = ai_arch_cbo_target(VSX_GURWX);
    const uintptr_t good_vaddr = ai_arch_cbo_target(VSRWX_GRWX);
    const uintptr_t marker_vaddr = hs_page_base(VSRWX_GRWX) + 0x1c0U;
    const uintptr_t marker_paddr = phys_page_base(VSRWX_GRWX) + 0x1c0U;

    ai_arch_seed_block(VSX_GURWX, fault_seed);
    ai_arch_seed_block(VSRWX_GRWX, good_seed);
    goto_priv(PRIV_M);
    write64(marker_paddr, 0ULL);
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(fault_vaddr);
    AI_ASSERT_EXCEPT("the first HS cbo.clean reports Store PF on a no-R page and captures that faulting VA",
        CAUSE_SPF, fault_vaddr);

    TEST_SETUP_EXCEPT();
    cbo_flush(good_vaddr);
    write64(marker_vaddr, marker_value);
    uint64_t marker_readback = read64(marker_vaddr);
    TEST_ASSERT("a following successful cbo.flush and scalar store/load do not inherit the prior CBO page-fault metadata",
        excpt.triggered == false &&
        marker_readback == marker_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("the failed CBO page-fault block and later successful CBO block both keep their architectural backing contents stable",
        ai_arch_block_matches_seed(VSX_GURWX, fault_seed) &&
        ai_arch_block_matches_seed(VSRWX_GRWX, good_seed) &&
        read64(marker_paddr) == marker_value);

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_fault_repair_reexecute_generates_fresh_metadata_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_CBO_CBOM_PAGE;
    const uint64_t seed = 0x3141592653589793ULL;
    const uint64_t adjacent_seed = 0x3258468135792468ULL;
    const uintptr_t target_vaddr = ai_arch_cbo_target(page);
    const uintptr_t target_paddr = phys_page_base(page);
    const uintptr_t target_block_paddr = ai_arch_cbo_paddr(page);
    const uintptr_t adjacent_block_paddr =
        target_block_paddr + (AI_ARCH_CBO_BLOCK_WORDS * sizeof(uint64_t));

    ai_arch_seed_block(page, seed);
    ai_arch_seed_block_at(adjacent_block_paddr, adjacent_seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);

    goto_priv(PRIV_M);
    pte_t *target_leaf = ai_arch_cbo_hs_leaf_entry(page);
    uint64_t leaf_before_fault = *target_leaf;
    uint64_t menvcfg_before_fault = ai_arch_cbo_csrr_menvcfg();
    *target_leaf &= ~PTE_R;
    uint64_t leaf_pf_candidate = *target_leaf;
    ai_arch_configure_pmp_window(target_paddr, target_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(target_vaddr);
    AI_ASSERT_EXCEPT("when CBO no-R page fault and PMP read-deny are both candidates, the translated page fault is selected with tval at the target VA",
        CAUSE_SPF, target_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the PF-priority CBO fault preserves target and adjacent block contents plus the edited PTE/ENVCFG state",
        ai_arch_block_at_matches_seed(target_block_paddr, seed) &&
        ai_arch_block_at_matches_seed(adjacent_block_paddr, adjacent_seed) &&
        *target_leaf == leaf_pf_candidate &&
        ai_arch_cbo_csrr_menvcfg() == menvcfg_before_fault);

    ai_arch_restore_pmp_full_access();
    *target_leaf = leaf_before_fault;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(target_vaddr);
    AI_ASSERT_NO_EXCEPT("after restoring PTE.R and fencing, the same cbo.clean target is retranslated and succeeds");

    ai_arch_configure_pmp_window(target_paddr, target_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(target_vaddr);
    AI_ASSERT_EXCEPT("after PTE repair succeeds, a new PMP deny on the same CBO target produces fresh Store AF metadata instead of reusing the old Store PF",
        CAUSE_SAF, target_vaddr);

    TEST_ASSERT("fault, repair, success, and later fresh AF leave the target cache block contents unchanged",
        ai_arch_block_matches_seed(page, seed) &&
        ai_arch_block_at_matches_seed(adjacent_block_paddr, adjacent_seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_pmp_access_fault_repair_reexecute_success_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_CBO_CBOM_PAGE;
    const uint64_t seed = 0x2718281828459045ULL;
    const uintptr_t target_vaddr = ai_arch_cbo_target(page);
    const uintptr_t target_paddr = phys_page_base(page);

    ai_arch_seed_block(page, seed);
    ai_arch_restore_pmp_full_access();
    ai_arch_prepare_hs_cbo_env(MENVCFG_CBZFE);
    ai_arch_configure_pmp_window(target_paddr, target_paddr + PAGE_SIZE,
        AI_ARCH_CBO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(target_vaddr);
    AI_ASSERT_EXCEPT("the first HS cbo.flush reports Store AF when PMP denies read permission on the target block",
        CAUSE_SAF, target_vaddr);

    ai_arch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(target_vaddr);
    AI_ASSERT_NO_EXCEPT("after restoring PMP read permission, the same HS cbo.flush target is rechecked and succeeds without stale AF metadata");

    TEST_ASSERT("PMP Access Fault repair and reexecute leave the target block contents unchanged",
        ai_arch_block_matches_seed(page, seed));

    ai_arch_reset_hs_cbo_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_cbo_envcfg_pbmte_rejudges_pbmt_cbo_attr_corner()
{
    TEST_START();

    const enum test_page page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t vaddr = ai_arch_cbo_target(page);
    const uintptr_t paddr = phys_page_base(X) + AI_ARCH_CBO_BLOCK_OFFSET;
    const uint64_t seed = 0x50424d5443424f31ULL;
    const uint64_t updated = 0x314f4243544d4250ULL;

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = ai_arch_cbo_csrr_menvcfg();
    uint64_t base_menvcfg =
        original_menvcfg & ~(AI_ARCH_CBO_ENVCFG_CBO_MASK | MENVCFG_PBMTE);

    write64(paddr, seed);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBZFE | MENVCFG_CBIE01);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(vaddr);
    TEST_ASSERT("with CBO enabled but MENVCFG.PBMTE clear, a PBMT=NC leaf still faults before CBO reaches final memory-attribute routing",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_PBMTE |
        MENVCFG_CBZFE | MENVCFG_CBIE01);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(vaddr);
    AI_ASSERT_EXCEPT("after setting PBMTE, HS cbo.clean through the PBMT=NC leaf is rejudged from PBMT-disabled Store PF into final-attribute Store AF",
        CAUSE_SAF, vaddr);

    TEST_SETUP_EXCEPT();
    cbo_flush(vaddr);
    AI_ASSERT_EXCEPT("PBMTE-enabled PBMT=NC leaf makes HS cbo.flush reach final NC attribute routing and report Store AF",
        CAUSE_SAF, vaddr);

    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr);
    AI_ASSERT_EXCEPT("PBMTE-enabled PBMT=NC leaf makes HS cbo.inval reach final NC attribute routing and report Store AF",
        CAUSE_SAF, vaddr);

    TEST_SETUP_EXCEPT();
    sd(vaddr, updated);
    TEST_ASSERT("PBMTE-enabled CBO probes do not corrupt the same alias data permission path before a real HS store",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("the real PBMT=NC load after CBO probes reads back only the store payload",
        excpt.triggered == false &&
        observed == updated);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBZFE | MENVCFG_CBIE01);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(vaddr);
    TEST_ASSERT("clearing PBMTE again makes the same PBMT=NC CBO target fault instead of reusing the enabled interpretation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(original_menvcfg);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cbo_envcfg_fiom_does_not_gate_cbo_permissions_corner()
{
    TEST_START();

    const uint64_t fiom = 1ULL << 0;
    const enum test_page cbom_page = VSRWX_GRWX;
    const enum test_page zero_page = VSRW_GRWX;
    const uintptr_t cbom_target = ai_arch_cbo_target(cbom_page);
    const uintptr_t zero_target = ai_arch_cbo_target(zero_page);
    const uint64_t cbom_seed = 0x46494f4d43424f31ULL;
    const uint64_t zero_seed = 0x46494f4d43424f32ULL;

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = ai_arch_cbo_csrr_menvcfg();
    uint64_t original_senvcfg = ai_arch_cbo_csrr_senvcfg();
    uint64_t original_mstateen0 = CSRR(CSR_MSTATEEN0);
    uint64_t base_menvcfg =
        original_menvcfg & ~(AI_ARCH_CBO_ENVCFG_CBO_MASK | fiom);
    uint64_t base_senvcfg =
        original_senvcfg & ~(AI_ARCH_CBO_ENVCFG_CBO_MASK | fiom);

    ai_arch_seed_block(cbom_page, cbom_seed);
    ai_arch_seed_block(zero_page, zero_seed);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | fiom);
    ai_arch_cbo_csrw_senvcfg(base_senvcfg | fiom);
    CSRS(CSR_MSTATEEN0, MSTATEEN_ENVCFG);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(cbom_target);
    TEST_ASSERT("HS cbo.clean remains IllegalInstruction when only FIOM is set and CBZFE is still clear",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_target);
    TEST_ASSERT("HS cbo.zero remains IllegalInstruction when only FIOM is set and CBZE is still clear",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | fiom | MENVCFG_CBZFE);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(cbom_target);
    AI_ASSERT_NO_EXCEPT("adding CBZFE while FIOM stays set enables HS cbo.clean");

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_target);
    TEST_ASSERT("FIOM plus CBZFE still does not ghost-enable cbo.zero without CBZE",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | MENVCFG_CBZFE);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(cbom_target);
    AI_ASSERT_NO_EXCEPT("clearing FIOM while keeping CBZFE leaves HS cbo.clean enabled");

    goto_priv(PRIV_M);
    ai_arch_cbo_csrw_menvcfg(base_menvcfg | fiom | MENVCFG_CBZE);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(zero_target);
    AI_ASSERT_NO_EXCEPT("adding CBZE while FIOM stays set enables HS cbo.zero");

    TEST_SETUP_EXCEPT();
    cbo_inval(cbom_target);
    TEST_ASSERT("FIOM plus CBZE still does not ghost-enable cbo.inval without CBIE",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);

    goto_priv(PRIV_M);
    TEST_ASSERT("FIOM toggling around CBO permission checks leaves the clean/inval target block unchanged and lets enabled cbo.zero clear only its own block",
        ai_arch_block_matches_seed(cbom_page, cbom_seed) &&
        read64(phys_page_base(zero_page)) == 0ULL);

    ai_arch_cbo_csrw_menvcfg(original_menvcfg);
    ai_arch_cbo_csrw_senvcfg(original_senvcfg);
    CSRW(CSR_MSTATEEN0, original_mstateen0);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_silent_then_cbo_load_store_trap_csr_independent_corner()
{
    TEST_START();

    const enum test_page no_r_page = VSX_GURWX;
    const enum test_page no_w_page = VSRX_GURWX;
    const uintptr_t cbo_vaddr = ai_arch_cbo_target(no_r_page);
    const uintptr_t load_vaddr = cbo_vaddr + 16U;
    const uintptr_t store_vaddr = ai_arch_cbo_target(no_w_page) + 24U;
    const uint64_t no_r_seed = 0x7072656643624f31ULL;
    const uint64_t no_w_seed = 0x7072656653544f31ULL;
    const uint64_t load_sentinel = 0x5152535455565758ULL;
    const uint64_t denied_store = 0xa5a55a5adeadbeefULL;
    const uint64_t silent_scause = 0x5a5ULL;
    const uint64_t silent_stval = 0ULL;

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = ai_arch_cbo_csrr_menvcfg();
    uint64_t original_medeleg = CSRR(CSR_MEDELEG);
    ai_arch_seed_block(no_r_page, no_r_seed);
    ai_arch_seed_block(no_w_page, no_w_seed);
    ai_arch_cbo_csrw_menvcfg(original_menvcfg | MENVCFG_CBZFE);
    CSRS(CSR_MEDELEG, (1ULL << CAUSE_LPF) | (1ULL << CAUSE_SPF));
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    uint64_t original_sstatus = CSRR(CSR_SSTATUS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    CSRW(CSR_SCAUSE, silent_scause);
    CSRW(CSR_STVAL, silent_stval);
    TEST_SETUP_EXCEPT();
    prefetch_r(load_vaddr);
    TEST_ASSERT("prefetch.r to the no-R HS leaf stays silent and does not refresh S trap CSR metadata before the CBO fault probe",
        excpt.triggered == false &&
        CSRR(CSR_SCAUSE) == silent_scause &&
        CSRR(CSR_STVAL) == silent_stval
    );

    CSRW(CSR_SCAUSE, silent_scause + 1ULL);
    CSRW(CSR_STVAL, silent_stval);
    TEST_SETUP_EXCEPT();
    prefetch_w(store_vaddr);
    TEST_ASSERT("prefetch.w to the no-W HS leaf also stays silent and does not refresh S trap CSR metadata before the store fault probe",
        excpt.triggered == false &&
        CSRR(CSR_SCAUSE) == silent_scause + 1ULL &&
        CSRR(CSR_STVAL) == silent_stval
    );

    CSRW(CSR_SCAUSE, silent_scause + 2ULL);
    CSRW(CSR_STVAL, silent_stval);
    TEST_SETUP_EXCEPT();
    cbo_clean(cbo_vaddr);
    TEST_ASSERT("after silent prefetch probes, HS cbo.clean independently writes Store PF metadata to S trap CSRs for the no-R CBO target",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == cbo_vaddr &&
        CSRR(CSR_SCAUSE) == CAUSE_SPF &&
        CSRR(CSR_STVAL) == cbo_vaddr
    );

    CSRW(CSR_SCAUSE, silent_scause + 3ULL);
    CSRW(CSR_STVAL, silent_stval);
    TEST_SETUP_EXCEPT();
    uint64_t loaded = ai_arch_cbo_ld_rd_probe(load_vaddr, load_sentinel);
    TEST_ASSERT("the following real load independently writes Load PF metadata to S trap CSRs and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        loaded == load_sentinel &&
        CSRR(CSR_SCAUSE) == CAUSE_LPF &&
        CSRR(CSR_STVAL) == load_vaddr
    );

    CSRW(CSR_SCAUSE, silent_scause + 4ULL);
    CSRW(CSR_STVAL, silent_stval);
    TEST_SETUP_EXCEPT();
    sd(store_vaddr, denied_store);
    TEST_ASSERT("the following real store independently writes Store PF metadata to S trap CSRs for its own no-W address",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr &&
        CSRR(CSR_SCAUSE) == CAUSE_SPF &&
        CSRR(CSR_STVAL) == store_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("silent prefetch, CBO fault, load fault, and store fault leave both target cache blocks unchanged",
        ai_arch_block_matches_seed(no_r_page, no_r_seed) &&
        ai_arch_block_matches_seed(no_w_page, no_w_seed)
    );

    CSRW(CSR_SSTATUS, original_sstatus);
    CSRW(CSR_MEDELEG, original_medeleg);
    ai_arch_cbo_csrw_menvcfg(original_menvcfg);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cbo_complete_then_repeated_load_store_amo_no_residual_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x300U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x300U;
    const uint64_t initials[] = {
        0x1122334455667788ULL,
        0x0102030405060708ULL,
        0xa1a2a3a4a5a6a7a8ULL,
    };
    const uint64_t store_values[] = {
        0x8877665544332211ULL,
        0x1112131415161718ULL,
        0xb1b2b3b4b5b6b7b8ULL,
    };
    const uint64_t amo_addend = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = ai_arch_cbo_csrr_menvcfg();
    ai_arch_cbo_csrw_menvcfg(original_menvcfg |
        MENVCFG_CBZFE | MENVCFG_CBZE | MENVCFG_CBIE01);
    write64(base_paddr, initials[0]);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(base_vaddr);
    cbo_flush(base_vaddr);
    AI_ASSERT_NO_EXCEPT("a clean/flush sequence on the cacheable block completes before ordinary accesses");

    TEST_SETUP_EXCEPT();
    uint64_t second_load = 0ULL;
    uint64_t first_load = ai_arch_cbo_repeat_load_pair_probe(base_vaddr,
        &second_load);
    AI_ASSERT_NO_EXCEPT_AND("repeated scalar loads after completed CBOM observe a stable committed value with no intermediate CBO state",
        first_load == initials[0] &&
        second_load == initials[0]);

    TEST_SETUP_EXCEPT();
    write64(base_vaddr, store_values[0]);
    uint64_t after_store = read64(base_vaddr);
    uint64_t amo_old = amoadd_d(base_vaddr, amo_addend);
    uint64_t after_amo = read64(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("ordinary store and AMO after completed CBOM use the current memory image without residual CBO state",
        after_store == store_values[0] &&
        amo_old == store_values[0] &&
        after_amo == store_values[0] + amo_addend);

    goto_priv(PRIV_M);
    write64(base_paddr, initials[1]);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(base_vaddr);
    AI_ASSERT_NO_EXCEPT("completed cbo.zero clears the target block before later ordinary access probes");

    TEST_SETUP_EXCEPT();
    first_load = ai_arch_cbo_repeat_load_pair_probe(base_vaddr, &second_load);
    AI_ASSERT_NO_EXCEPT_AND("repeated scalar loads after completed cbo.zero observe stable zero data with no intermediate state",
        first_load == 0ULL &&
        second_load == 0ULL);

    TEST_SETUP_EXCEPT();
    write64(base_vaddr, store_values[1]);
    after_store = read64(base_vaddr);
    amo_old = amoadd_d(base_vaddr, amo_addend);
    after_amo = read64(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("ordinary store and AMO after completed cbo.zero use the post-zero current memory image",
        after_store == store_values[1] &&
        amo_old == store_values[1] &&
        after_amo == store_values[1] + amo_addend);

    goto_priv(PRIV_M);
    write64(base_paddr, initials[2]);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT("completed cbo.inval on a clean stable block does not leave residual state for later ordinary accesses");

    TEST_SETUP_EXCEPT();
    first_load = ai_arch_cbo_repeat_load_pair_probe(base_vaddr, &second_load);
    AI_ASSERT_NO_EXCEPT_AND("repeated scalar loads after completed cbo.inval observe the committed stable value",
        first_load == initials[2] &&
        second_load == initials[2]);

    TEST_SETUP_EXCEPT();
    write64(base_vaddr, store_values[2]);
    after_store = read64(base_vaddr);
    amo_old = amoadd_d(base_vaddr, amo_addend);
    after_amo = read64(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("ordinary store and AMO after completed cbo.inval use the current memory image without stale invalidate state",
        after_store == store_values[2] &&
        amo_old == store_values[2] &&
        after_amo == store_values[2] + amo_addend);

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after completed CBO plus ordinary load/store/AMO reflects only the later ordinary operations",
        read64(base_paddr) == store_values[2] + amo_addend);

    ai_arch_cbo_csrw_menvcfg(original_menvcfg);
    hspt_init();
    sfence_vma();

    TEST_END();
}
