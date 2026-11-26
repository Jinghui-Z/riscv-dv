#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

static uint8_t ai_arch_fp_mem[16] __attribute__((aligned(16)));
static uint8_t ai_arch_fp_dirtyfs_mem[64] __attribute__((aligned(16)));
static uint8_t ai_arch_amo_mem[16] __attribute__((aligned(16)));

#define AI_ARCH_AP78_MMIO_MEM_BASE      0x30000000000ULL
#define AI_ARCH_AP78_RESP_MMIO64_BASE   0x40070000ULL
#define AI_ARCH_AP78_RESP_MMIO64_LIMIT  0x40080000ULL
#define AI_ARCH_AP78_BAD_MMIO_PADDR \
    (phys_page_base_offset(VSRWXPbmt2_GURWXPbmt3, \
        AI_ARCH_AP78_MMIO_MEM_BASE) + 0x7e0ULL)

#define AI_ASSERT_NO_EXCEPT(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline uintptr_t ai_arch_fp_addr(size_t offset)
{
    return (uintptr_t)(ai_arch_fp_mem + offset);
}

static inline uintptr_t ai_arch_fp_dirtyfs_addr(size_t offset)
{
    return (uintptr_t)(ai_arch_fp_dirtyfs_mem + offset);
}

static inline uintptr_t ai_arch_amo_addr(size_t offset)
{
    return (uintptr_t)(ai_arch_amo_mem + offset);
}

static inline void ai_arch_enable_fp(void)
{
    CSRS(CSR_MSTATUS, SSTATUS_FS_INITIAL);
    CSRC(CSR_MSTATUS, 1ULL << 14);
}

static inline uintptr_t ai_arch_ap78_responsive_mmio64_addr(uintptr_t offset)
{
    return AI_ARCH_AP78_RESP_MMIO64_BASE +
        (offset & (AI_ARCH_AP78_RESP_MMIO64_LIMIT -
            AI_ARCH_AP78_RESP_MMIO64_BASE - sizeof(uint64_t)));
}

static inline void ai_arch_ap78_configure_pma_io_page(uintptr_t paddr)
{
    const uintptr_t page_start = paddr & PAGE_ADDR_MSK;

    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFFULL);
    CSRW(CSR_PMACFG0, 0x6FULL);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000ULL);
    CSRW(CSR_PMAADDR12, 0x20000000ULL);
    CSRW(CSR_PMAADDR13, 0x120000000ULL);
    CSRW(CSR_PMAADDR14, 0xC000000000ULL);
    CSRW(CSR_PMAADDR15, 0x10000000000ULL);

    CSRW(CSR_PMACFG0, 0ULL);
    CSRW(CSR_PMAADDR0, page_start >> 2);
    CSRW(CSR_PMAADDR1, (page_start + PAGE_SIZE) >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    fence_iorw_iorw();
}

static inline void ai_arch_ap78_restore_pma_env(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFFULL);
    CSRW(CSR_PMACFG0, 0x6FULL);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000ULL);
    CSRW(CSR_PMAADDR12, 0x20000000ULL);
    CSRW(CSR_PMAADDR13, 0x120000000ULL);
    CSRW(CSR_PMAADDR14, 0xC000000000ULL);
    CSRW(CSR_PMAADDR15, 0x10000000000ULL);

    CSRW(CSR_PMACFG0, 0ULL);
    fence_iorw_iorw();
}

static inline pte_t *ai_arch_fp_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_fp_prepare_hs(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    sfence_vma();

    goto_priv(PRIV_M);
}

static inline void ai_arch_disable_fp(void)
{
    CSRC(CSR_SSTATUS, SSTATUS_FS_MSK);
}

static inline void ai_arch_fp_write_fcsr(uint64_t fflags, uint64_t frm)
{
    CSRW(CSR_FCSR, ((frm & 0x7ULL) << 5) | (fflags & 0x1fULL));
}

static inline bool ai_arch_fp_fcsr_matches(uint64_t fflags, uint64_t frm)
{
    return (CSRR(CSR_FFLAGS) & 0x1fULL) == (fflags & 0x1fULL) &&
        (CSRR(CSR_FRM) & 0x7ULL) == (frm & 0x7ULL);
}

static inline void ai_arch_fp_seed_ft0_ft1(uint64_t target,
    uint64_t non_target)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft0, %[target]\n\t"
        "fmv.d.x ft1, %[non_target]\n\t"
        ".option pop\n\t"
        : : [target] "r"(target), [non_target] "r"(non_target)
        : "ft0", "ft1");
}

static inline void ai_arch_fp_read_ft0_ft1(uint64_t *target,
    uint64_t *non_target)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.x.d %[target], ft0\n\t"
        "fmv.x.d %[non_target], ft1\n\t"
        ".option pop\n\t"
        : [target] "=r"(*target), [non_target] "=r"(*non_target)
        : : "memory");
}

static inline void ai_arch_fp_illegal_flw_probe(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "flw ft0, 0(%0)\n\t"
        ".option pop\n\t"
        : : "r"(addr) : "ft0", "memory");
}

static inline void ai_arch_fp_illegal_fld_probe(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fld ft0, 0(%0)\n\t"
        ".option pop\n\t"
        : : "r"(addr) : "ft0", "memory");
}

static inline void ai_arch_fp_illegal_fsw_probe(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fsw ft0, 0(%0)\n\t"
        ".option pop\n\t"
        : : "r"(addr) : "memory");
}

static inline void ai_arch_fp_illegal_fsd_probe(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fsd ft0, 0(%0)\n\t"
        ".option pop\n\t"
        : : "r"(addr) : "memory");
}

static inline uint64_t ai_flw_raw_bits(uintptr_t addr)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "flw ft0, 0(%1)\n\t"
        "fmv.x.w %0, ft0\n\t"
        ".option pop\n\t"
        : "=r"(value) : "r"(addr) : "ft0", "memory");
    return value;
}

static inline uint64_t ai_flw_boxed_bits(uintptr_t addr)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "flw ft0, 0(%1)\n\t"
        "fmv.x.d %0, ft0\n\t"
        ".option pop\n\t"
        : "=r"(value) : "r"(addr) : "ft0", "memory");
    return value;
}

static inline uint64_t ai_fld_raw_bits(uintptr_t addr)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fld ft0, 0(%1)\n\t"
        "fmv.x.d %0, ft0\n\t"
        ".option pop\n\t"
        : "=r"(value) : "r"(addr) : "ft0", "memory");
    return value;
}

static inline void ai_fsw_raw_bits(uintptr_t addr, uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.w.x ft0, %1\n\t"
        "fsw ft0, 0(%0)\n\t"
        ".option pop\n\t"
        : : "r"(addr), "r"(bits) : "ft0", "memory");
}

static inline void ai_fsd_raw_bits(uintptr_t addr, uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft0, %1\n\t"
        "fsd ft0, 0(%0)\n\t"
        ".option pop\n\t"
        : : "r"(addr), "r"(bits) : "ft0", "memory");
}

static inline uint64_t ai_flw_non_target_fpr_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft1, %[sentinel]\n\t"
        "flw ft0, 0(%[addr])\n\t"
        "fmv.x.d %[value], ft1\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "ft0", "ft1", "memory");
    return value;
}

static inline uint64_t ai_fld_non_target_fpr_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft1, %[sentinel]\n\t"
        "fld ft0, 0(%[addr])\n\t"
        "fmv.x.d %[value], ft1\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "ft0", "ft1", "memory");
    return value;
}

static inline uint64_t ai_dirtyfs_wrong_path_fp_memory_probe(
    uintptr_t control_addr, uintptr_t flw_addr, uintptr_t fld_addr,
    uintptr_t fsw_addr, uintptr_t fsd_addr, uint64_t store_poison,
    uint64_t *mstatus_before_fpr_read)
{
    uint64_t ft1_value;
    uint64_t mstatus_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld t0, 0(%[control_addr])\n\t"
        "bnez t0, 1f\n\t"
        "flw ft1, 0(%[flw_addr])\n\t"
        "fld ft1, 0(%[fld_addr])\n\t"
        "fmv.d.x ft0, %[store_poison]\n\t"
        "fsw ft0, 0(%[fsw_addr])\n\t"
        "fsd ft0, 0(%[fsd_addr])\n\t"
        "1:\n\t"
        "csrr %[mstatus_value], mstatus\n\t"
        "fmv.x.d %[ft1_value], ft1\n\t"
        ".option pop\n\t"
        : [ft1_value] "=r"(ft1_value),
          [mstatus_value] "=r"(mstatus_value)
        : [control_addr] "r"(control_addr), [flw_addr] "r"(flw_addr),
          [fld_addr] "r"(fld_addr), [fsw_addr] "r"(fsw_addr),
          [fsd_addr] "r"(fsd_addr), [store_poison] "r"(store_poison)
        : "t0", "ft0", "ft1", "memory");

    *mstatus_before_fpr_read = mstatus_value;
    return ft1_value;
}

static inline uint64_t ai_fsw_non_target_fpr_probe(uintptr_t addr,
    uint64_t bits, uint64_t sentinel)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft1, %[sentinel]\n\t"
        "fmv.w.x ft0, %[bits]\n\t"
        "fsw ft0, 0(%[addr])\n\t"
        "fmv.x.d %[value], ft1\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [addr] "r"(addr), [bits] "r"(bits),
          [sentinel] "r"(sentinel)
        : "ft0", "ft1", "memory");
    return value;
}

static inline uint64_t ai_fsd_non_target_fpr_probe(uintptr_t addr,
    uint64_t bits, uint64_t sentinel)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft1, %[sentinel]\n\t"
        "fmv.d.x ft0, %[bits]\n\t"
        "fsd ft0, 0(%[addr])\n\t"
        "fmv.x.d %[value], ft1\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [addr] "r"(addr), [bits] "r"(bits),
          [sentinel] "r"(sentinel)
        : "ft0", "ft1", "memory");
    return value;
}

static inline void ai_flw_fsw_round_trip(uintptr_t load_addr,
    uintptr_t store_addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "flw ft0, 0(%0)\n\t"
        "fsw ft0, 0(%1)\n\t"
        ".option pop\n\t"
        : : "r"(load_addr), "r"(store_addr) : "ft0", "memory");
}

static inline void ai_fld_fsd_round_trip(uintptr_t load_addr,
    uintptr_t store_addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fld ft0, 0(%0)\n\t"
        "fsd ft0, 0(%1)\n\t"
        ".option pop\n\t"
        : : "r"(load_addr), "r"(store_addr) : "ft0", "memory");
}

static inline void ai_arch_amo_write64(uint64_t value)
{
    *(volatile uint64_t *)ai_arch_amo_mem = value;
}

static inline uint64_t ai_arch_amo_read64(void)
{
    return *(volatile uint64_t *)ai_arch_amo_mem;
}

static inline void ai_arch_amo_write64_at(size_t offset, uint64_t value)
{
    *(volatile uint64_t *)(ai_arch_amo_mem + offset) = value;
}

static inline uint64_t ai_arch_amo_read64_at(size_t offset)
{
    return *(volatile uint64_t *)(ai_arch_amo_mem + offset);
}

static inline uint64_t ai_amoswap_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoswap.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amoadd_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amoadd_w_aq_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.w.aq %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amoadd_w_rl_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.w.rl %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amoadd_w_aqrl_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.w.aqrl %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amoadd_d_aq_raw64(uintptr_t addr, uint64_t value)
{
    uint64_t old_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d.aq %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(value) : "memory");

    return old_value;
}

static inline uint64_t ai_amoadd_d_rl_raw64(uintptr_t addr, uint64_t value)
{
    uint64_t old_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d.rl %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(value) : "memory");

    return old_value;
}

static inline uint64_t ai_amoadd_d_aqrl_raw64(uintptr_t addr, uint64_t value)
{
    uint64_t old_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d.aqrl %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(value) : "memory");

    return old_value;
}

static inline uint64_t ai_amoand_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoand.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amoor_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoor.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amoxor_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoxor.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amomaxu_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amomaxu.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amomin_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amomin.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amominu_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amominu.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_amomax_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amomax.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

bool ai_arch_flw_register_writeback()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint32_t *)ai_arch_fp_mem = 0x3fc00001U;
    TEST_SETUP_EXCEPT();
    uint64_t positive_bits = ai_flw_raw_bits(ai_arch_fp_addr(0));
    AI_ASSERT_NO_EXCEPT("flw writes a positive single-precision payload into the FP register",
        positive_bits == 0x000000003fc00001ULL
    );

    *(volatile uint32_t *)ai_arch_fp_mem = 0xbf800123U;
    TEST_SETUP_EXCEPT();
    uint64_t negative_bits = ai_flw_raw_bits(ai_arch_fp_addr(0));
    AI_ASSERT_NO_EXCEPT("fmv.x.w preserves the loaded single-precision payload and sign fill",
        negative_bits == 0xffffffffbf800123ULL
    );

    TEST_END();
}

bool ai_arch_fld_register_writeback()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0xfff8123456789abcULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_fld_raw_bits(ai_arch_fp_addr(0));
    AI_ASSERT_NO_EXCEPT("fld writes the complete 64-bit payload into the FP register",
        value == 0xfff8123456789abcULL
    );

    TEST_END();
}

bool ai_arch_ap78_fp_load_fs_dirty_control_corner()
{
    TEST_START();

    const uint32_t flw_payload = 0x3f801234U;
    const uint32_t flw_repeat_payload = 0x40405678U;
    const uint64_t fld_payload = 0x400921fb54442d18ULL;
    const uint32_t integer_payload = 0x55667788U;
    const uint64_t clean_fs = SSTATUS_FS_CLEAN;
    uint64_t flw_bits = 0ULL;
    uint64_t flw_repeat_bits = 0ULL;
    uint64_t fld_bits = 0ULL;
    uint64_t integer_value = 0ULL;

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint32_t *)ai_arch_fp_mem = flw_payload;
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, clean_fs);
    TEST_SETUP_EXCEPT();
    flw_bits = ai_flw_raw_bits(ai_arch_fp_addr(0));
    TEST_ASSERT("AP-78 control: a committed normal-memory flw updates the destination FPR and dirties mstatus.FS in the same instruction",
        excpt.triggered == false &&
        flw_bits == flw_payload &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == SSTATUS_FS_DIRTY
    );

    *(volatile uint32_t *)ai_arch_fp_mem = flw_repeat_payload;
    TEST_SETUP_EXCEPT();
    flw_repeat_bits = ai_flw_raw_bits(ai_arch_fp_addr(0));
    TEST_ASSERT("AP-78 control: a repeated committed flw keeps mstatus.FS Dirty while updating the destination FPR again",
        excpt.triggered == false &&
        flw_repeat_bits == flw_repeat_payload &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == SSTATUS_FS_DIRTY
    );

    *(volatile uint32_t *)ai_arch_fp_mem = integer_payload;
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, clean_fs);
    TEST_SETUP_EXCEPT();
    integer_value = lw(ai_arch_fp_addr(0));
    TEST_ASSERT("AP-78 control: a committed integer load from the same memory path does not dirty mstatus.FS",
        excpt.triggered == false &&
        (uint32_t)integer_value == integer_payload &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == clean_fs
    );

    *(volatile uint64_t *)ai_arch_fp_mem = fld_payload;
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, clean_fs);
    TEST_SETUP_EXCEPT();
    fld_bits = ai_fld_raw_bits(ai_arch_fp_addr(0));
    TEST_ASSERT("AP-78 control: fld shares the FP-load rule that a committed FPR write dirties mstatus.FS",
        excpt.triggered == false &&
        fld_bits == fld_payload &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == SSTATUS_FS_DIRTY
    );

    TEST_END();
}

bool ai_arch_dirtyfs_wrong_path_fp_memory_squash_corner()
{
    TEST_START();

    const uint64_t clean_fs = SSTATUS_FS_CLEAN;
    const uint64_t sentinel = 0x7ff81234deadbeefULL;
    const uint64_t flw_poison = 0x111122223f801234ULL;
    const uint64_t fld_poison = 0x400921fb54442d18ULL;
    const uint64_t store_guard0 = 0x8877665544332211ULL;
    const uint64_t store_guard1 = 0x0123456789abcdefULL;
    const uint64_t store_poison = 0xa5a55a5afedcba98ULL;
    const uint32_t flw_good = 0x40405678U;
    const uint64_t fld_good = 0xc00921fb54442d18ULL;
    uint64_t wrong_path_ft1 = 0ULL;
    uint64_t wrong_path_mstatus = 0ULL;
    uint64_t flw_bits = 0ULL;
    uint64_t fld_bits = 0ULL;

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_dirtyfs_addr(0) = 1ULL;
    *(volatile uint64_t *)ai_arch_fp_dirtyfs_addr(8) = flw_poison;
    *(volatile uint64_t *)ai_arch_fp_dirtyfs_addr(16) = fld_poison;
    *(volatile uint64_t *)ai_arch_fp_dirtyfs_addr(24) = store_guard0;
    *(volatile uint64_t *)ai_arch_fp_dirtyfs_addr(32) = store_guard1;
    ai_arch_fp_seed_ft0_ft1(store_poison, sentinel);

    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, clean_fs);
    TEST_SETUP_EXCEPT();
    wrong_path_ft1 = ai_dirtyfs_wrong_path_fp_memory_probe(
        ai_arch_fp_dirtyfs_addr(0), ai_arch_fp_dirtyfs_addr(8),
        ai_arch_fp_dirtyfs_addr(16), ai_arch_fp_dirtyfs_addr(24),
        ai_arch_fp_dirtyfs_addr(32), store_poison, &wrong_path_mstatus);
    TEST_ASSERT("dirtyFs wrong-path: taken branch squashes poison flw/fld/fsw/fsd without dirtying FS or changing architectural state",
        excpt.triggered == false &&
        wrong_path_ft1 == sentinel &&
        *(volatile uint64_t *)ai_arch_fp_dirtyfs_addr(24) == store_guard0 &&
        *(volatile uint64_t *)ai_arch_fp_dirtyfs_addr(32) == store_guard1 &&
        (wrong_path_mstatus & SSTATUS_FS_MSK) == clean_fs
    );

    *(volatile uint32_t *)ai_arch_fp_dirtyfs_addr(40) = flw_good;
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, clean_fs);
    TEST_SETUP_EXCEPT();
    flw_bits = ai_flw_raw_bits(ai_arch_fp_dirtyfs_addr(40));
    TEST_ASSERT("dirtyFs wrong-path control: a real committed flw from the same case dirties FS and writes FPR",
        excpt.triggered == false &&
        flw_bits == flw_good &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == SSTATUS_FS_DIRTY
    );

    *(volatile uint64_t *)ai_arch_fp_dirtyfs_addr(48) = fld_good;
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, clean_fs);
    TEST_SETUP_EXCEPT();
    fld_bits = ai_fld_raw_bits(ai_arch_fp_dirtyfs_addr(48));
    TEST_ASSERT("dirtyFs wrong-path control: a real committed fld dirties FS with the expected raw payload",
        excpt.triggered == false &&
        fld_bits == fld_good &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == SSTATUS_FS_DIRTY
    );

    TEST_END();
}

bool ai_arch_ap78_mmio_fp_load_fs_dirty_manual_smoke_corner()
{
    TEST_START();

    const uintptr_t mmio_addr = ai_arch_ap78_responsive_mmio64_addr(0x120U);
    const uintptr_t bad_mmio_addr = AI_ARCH_AP78_BAD_MMIO_PADDR;
    const uint64_t mmio_payload = 0x400921fb3f801234ULL;
    const uint32_t flw_payload = (uint32_t)mmio_payload;
    const uint32_t flw_repeat_payload = 0x40405678U;
    const uint32_t integer_payload = 0x11223344U;
    const uint64_t fld_payload = 0x400921fb54442d18ULL;
    const uint64_t clean_fs = SSTATUS_FS_CLEAN;
    uint64_t flw_bits = 0ULL;
    uint64_t flw_repeat_bits = 0ULL;
    uint64_t fld_bits = 0ULL;
    uint64_t integer_bits = 0ULL;

    goto_priv(PRIV_M);
    ai_arch_enable_fp();
    ai_arch_ap78_configure_pma_io_page(mmio_addr);

    write64(mmio_addr, mmio_payload);
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, clean_fs);
    TEST_SETUP_EXCEPT();
    flw_bits = ai_flw_raw_bits(mmio_addr);
    TEST_ASSERT("AP-78 manual MMIO: a responsive MMIO flw commits, writes its FPR payload, and dirties mstatus.FS",
        excpt.triggered == false &&
        flw_bits == flw_payload &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == SSTATUS_FS_DIRTY
    );

    write64(mmio_addr, flw_repeat_payload);
    TEST_SETUP_EXCEPT();
    flw_repeat_bits = ai_flw_raw_bits(mmio_addr);
    TEST_ASSERT("AP-78 manual MMIO: a repeated responsive MMIO flw keeps FS Dirty and refreshes the FPR payload",
        excpt.triggered == false &&
        flw_repeat_bits == flw_repeat_payload &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == SSTATUS_FS_DIRTY
    );

    write64(mmio_addr, integer_payload);
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, clean_fs);
    TEST_SETUP_EXCEPT();
    integer_bits = (uint64_t)lwu(mmio_addr);
    TEST_ASSERT("AP-78 manual MMIO: an integer MMIO load from the same skip-capable responder does not dirty mstatus.FS",
        excpt.triggered == false &&
        integer_bits == integer_payload &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == clean_fs
    );

    write64(mmio_addr, fld_payload);
    TEST_SETUP_EXCEPT();
    fld_bits = ai_fld_raw_bits(mmio_addr);
    TEST_ASSERT("AP-78 manual MMIO: a later responsive MMIO fld also follows the FP-load rule and leaves FS Dirty",
        excpt.triggered == false &&
        fld_bits == fld_payload &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == SSTATUS_FS_DIRTY
    );

    ai_arch_ap78_configure_pma_io_page(bad_mmio_addr);
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, clean_fs);
    TEST_SETUP_EXCEPT();
    (void)ai_flw_raw_bits(bad_mmio_addr);
    TEST_ASSERT("AP-78 manual MMIO: a faulting MMIO flw does not commit and must not be treated as a skipped normal writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_mmio_addr &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == clean_fs
    );

    ai_arch_ap78_restore_pma_env();
    TEST_END();
}

bool ai_arch_flw_nan_payload_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint32_t *)ai_arch_fp_mem = 0x7fc12345U;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_flw_raw_bits(ai_arch_fp_addr(0));
    AI_ASSERT_NO_EXCEPT("flw preserves a quiet-NaN payload bit pattern exactly when moved back with fmv.x.w",
        value == 0x000000007fc12345ULL
    );

    TEST_END();
}

bool ai_arch_flw_negative_zero_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint32_t *)ai_arch_fp_mem = 0x80000000U;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_flw_raw_bits(ai_arch_fp_addr(0));
    AI_ASSERT_NO_EXCEPT("flw preserves the exact -0.0 single-precision bit pattern when moved back with fmv.x.w",
        value == 0xffffffff80000000ULL
    );

    TEST_END();
}

bool ai_arch_flw_upper_word_negative_zero_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x800000003fc00001ULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_flw_raw_bits(ai_arch_fp_addr(4));
    AI_ASSERT_NO_EXCEPT("flw preserves the exact -0.0 single-precision bit pattern when loading from the high 32-bit lane of a doubleword",
        value == 0xffffffff80000000ULL
    );

    TEST_END();
}

bool ai_arch_flw_upper_word_nan_payload_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x7fc123453fc00001ULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_flw_raw_bits(ai_arch_fp_addr(4));
    AI_ASSERT_NO_EXCEPT("flw preserves a quiet-NaN payload exactly when loading from the high 32-bit lane of a doubleword",
        value == 0x000000007fc12345ULL
    );

    TEST_END();
}

bool ai_arch_fld_negative_zero_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x8000000000000000ULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_fld_raw_bits(ai_arch_fp_addr(0));
    AI_ASSERT_NO_EXCEPT("fld preserves the exact -0.0 double-precision bit pattern when moved back with fmv.x.d",
        value == 0x8000000000000000ULL
    );

    TEST_END();
}

bool ai_arch_fld_nan_payload_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x7ff8123456789abcULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_fld_raw_bits(ai_arch_fp_addr(0));
    AI_ASSERT_NO_EXCEPT("fld preserves a quiet-NaN payload bit pattern exactly when moved back with fmv.x.d",
        value == 0x7ff8123456789abcULL
    );

    TEST_END();
}

bool ai_arch_flw_second_doubleword_negative_zero_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x0123456789abcdefULL;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = 0x55aa33cc80000000ULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_flw_raw_bits(ai_arch_fp_addr(8));
    AI_ASSERT_NO_EXCEPT("flw preserves the exact -0.0 single-precision bit pattern when loading from the low 32-bit lane of the second doubleword",
        value == 0xffffffff80000000ULL
    );

    TEST_END();
}

bool ai_arch_flw_second_doubleword_nan_payload_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x0123456789abcdefULL;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = 0x55aa33cc7fc12345ULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_flw_raw_bits(ai_arch_fp_addr(8));
    AI_ASSERT_NO_EXCEPT("flw preserves a quiet-NaN payload exactly when loading from the low 32-bit lane of the second doubleword",
        value == 0x000000007fc12345ULL
    );

    TEST_END();
}

bool ai_arch_flw_second_doubleword_upper_word_nan_payload_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = 0x7fc12345deadbeefULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_flw_raw_bits(ai_arch_fp_addr(12));
    AI_ASSERT_NO_EXCEPT("flw preserves a quiet-NaN payload exactly when loading from the upper 32-bit lane of the second doubleword",
        value == 0x000000007fc12345ULL
    );

    TEST_END();
}

bool ai_arch_flw_second_doubleword_upper_word_negative_zero_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x0123456789abcdefULL;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = 0x80000000deadbeefULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_flw_raw_bits(ai_arch_fp_addr(12));
    AI_ASSERT_NO_EXCEPT("flw preserves the exact -0.0 single-precision bit pattern when loading from the upper 32-bit lane of the second doubleword",
        value == 0xffffffff80000000ULL
    );

    TEST_END();
}

bool ai_arch_fld_second_doubleword_register_writeback_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x0123456789abcdefULL;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = 0xfff8123456789abcULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_fld_raw_bits(ai_arch_fp_addr(8));
    AI_ASSERT_NO_EXCEPT("fld writes the complete 64-bit payload into the FP register when loading from the second doubleword",
        value == 0xfff8123456789abcULL
    );

    TEST_END();
}

bool ai_arch_fld_second_doubleword_nan_payload_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x0123456789abcdefULL;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = 0x7ff8123456789abcULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_fld_raw_bits(ai_arch_fp_addr(8));
    AI_ASSERT_NO_EXCEPT("fld preserves a quiet-NaN payload bit pattern exactly when loading from the second doubleword",
        value == 0x7ff8123456789abcULL
    );

    TEST_END();
}

bool ai_arch_fld_second_doubleword_negative_zero_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_enable_fp();

    *(volatile uint64_t *)ai_arch_fp_mem = 0x0123456789abcdefULL;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = 0x8000000000000000ULL;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_fld_raw_bits(ai_arch_fp_addr(8));
    AI_ASSERT_NO_EXCEPT("fld preserves the exact -0.0 double-precision bit pattern when loading from the second doubleword",
        value == 0x8000000000000000ULL
    );

    TEST_END();
}

bool ai_arch_fp_load_store_raw_bits_fflags_matrix_corner()
{
    TEST_START();

    const uint64_t fflags_sentinel = 0x15ULL;
    const uint64_t frm_sentinel = 0x2ULL;
    const uint32_t flw_qnan = 0x7fc12345U;
    const uint32_t flw_snan = 0x7fa54321U;
    const uint32_t flw_normal = 0x3fc00001U;
    const uint32_t flw_neg_zero = 0x80000000U;
    const uint64_t fld_qnan = 0x7ff8123456789abcULL;
    const uint64_t fld_snan = 0x7ff0123456789abcULL;
    const uint64_t fld_neg_zero = 0x8000000000000000ULL;
    const uint64_t fld_subnormal = 0x0000000000000001ULL;
    const uint64_t fld_positive_inf = 0x7ff0000000000000ULL;
    const uint64_t fld_normal = 0x400921fb54442d18ULL;
    const uint64_t guard = 0x1122334455667788ULL;
    const uint64_t non_target_fpr = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_arch_enable_fp();
    ai_arch_fp_write_fcsr(fflags_sentinel, frm_sentinel);

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) =
        ((uint64_t)flw_snan << 32) | flw_qnan;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) =
        ((uint64_t)flw_neg_zero << 32) | flw_normal;

    TEST_SETUP_EXCEPT();
    uint64_t boxed_qnan = ai_flw_boxed_bits(ai_arch_fp_addr(0));
    uint64_t boxed_snan = ai_flw_boxed_bits(ai_arch_fp_addr(4));
    uint64_t boxed_normal = ai_flw_boxed_bits(ai_arch_fp_addr(8));
    uint64_t boxed_neg_zero = ai_flw_boxed_bits(ai_arch_fp_addr(12));
    TEST_ASSERT("flw preserves qNaN, sNaN, ordinary, and -0.0 low 32-bit payloads with NaN-boxed upper bits and stable fcsr",
        boxed_qnan == (0xffffffff00000000ULL | flw_qnan) &&
        boxed_snan == (0xffffffff00000000ULL | flw_snan) &&
        boxed_normal == (0xffffffff00000000ULL | flw_normal) &&
        boxed_neg_zero == (0xffffffff00000000ULL | flw_neg_zero) &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) ==
            (((uint64_t)flw_snan << 32) | flw_qnan) &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) ==
            (((uint64_t)flw_neg_zero << 32) | flw_normal) &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    for (size_t i = 0; i < sizeof(ai_arch_fp_mem); i++) {
        ai_arch_fp_mem[i] = 0U;
    }
    ai_arch_fp_mem[4] = 0x80U;
    ai_arch_fp_mem[9] = 0x80U;
    ai_arch_fp_mem[15] = 0x80U;
    TEST_SETUP_EXCEPT();
    uint64_t boxed_bit_clear = ai_flw_boxed_bits(ai_arch_fp_addr(0));
    uint64_t boxed_bit7 = ai_flw_boxed_bits(ai_arch_fp_addr(4));
    uint64_t boxed_bit15 = ai_flw_boxed_bits(ai_arch_fp_addr(8));
    uint64_t boxed_bit31 = ai_flw_boxed_bits(ai_arch_fp_addr(12));
    TEST_ASSERT("flw byte patterns with bit7, bit15, and bit31 toggled expose exact lower 32 raw bits and NaN-box the upper bits",
        excpt.triggered == false &&
        boxed_bit_clear == 0xffffffff00000000ULL &&
        boxed_bit7 == 0xffffffff00000080ULL &&
        boxed_bit15 == 0xffffffff00008000ULL &&
        boxed_bit31 == 0xffffffff80000000ULL &&
        *(volatile uint32_t *)(ai_arch_fp_mem + 0) == 0x00000000U &&
        *(volatile uint32_t *)(ai_arch_fp_mem + 4) == 0x00000080U &&
        *(volatile uint32_t *)(ai_arch_fp_mem + 8) == 0x00008000U &&
        *(volatile uint32_t *)(ai_arch_fp_mem + 12) == 0x80000000U &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    for (size_t i = 0; i < sizeof(ai_arch_fp_mem); i++) {
        ai_arch_fp_mem[i] = 0U;
    }
    ai_arch_fp_mem[0] = 0x80U;
    ai_arch_fp_mem[1] = 0x80U;
    ai_arch_fp_mem[3] = 0x80U;
    ai_arch_fp_mem[8] = 0x01U;
    ai_arch_fp_mem[9] = 0x02U;
    ai_arch_fp_mem[10] = 0x03U;
    ai_arch_fp_mem[11] = 0x04U;
    ai_arch_fp_mem[12] = 0x05U;
    ai_arch_fp_mem[13] = 0x06U;
    ai_arch_fp_mem[14] = 0x07U;
    ai_arch_fp_mem[15] = 0x08U;
    TEST_SETUP_EXCEPT();
    uint64_t fld_lane_bits_set = ai_fld_raw_bits(ai_arch_fp_addr(0));
    uint64_t fld_lane_bits_clear = ai_fld_raw_bits(ai_arch_fp_addr(8));
    TEST_ASSERT("fld byte patterns stitch exact 64-bit raw images while preserving bit7, bit15, and bit31 values without touching fcsr",
        excpt.triggered == false &&
        fld_lane_bits_set == 0x0000000080008080ULL &&
        fld_lane_bits_clear == 0x0807060504030201ULL &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) ==
            0x0000000080008080ULL &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) ==
            0x0807060504030201ULL &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    TEST_SETUP_EXCEPT();
    uint64_t flw_other_fpr = ai_flw_non_target_fpr_probe(
        ai_arch_fp_addr(0), non_target_fpr);
    uint64_t fld_other_fpr = ai_fld_non_target_fpr_probe(
        ai_arch_fp_addr(0), non_target_fpr);
    TEST_ASSERT("flw/fld update only the target FPR and preserve an independent non-target FPR sentinel",
        excpt.triggered == false &&
        flw_other_fpr == non_target_fpr &&
        fld_other_fpr == non_target_fpr &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_qnan;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    uint64_t fld_qnan_value = ai_fld_raw_bits(ai_arch_fp_addr(0));
    TEST_ASSERT("fld preserves a double-precision qNaN payload exactly and does not update fcsr or adjacent memory",
        fld_qnan_value == fld_qnan &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) == fld_qnan &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == guard &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_snan;
    TEST_SETUP_EXCEPT();
    uint64_t fld_snan_value = ai_fld_raw_bits(ai_arch_fp_addr(0));
    TEST_ASSERT("fld preserves a double-precision sNaN payload exactly without canonicalization or fcsr updates",
        fld_snan_value == fld_snan &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == guard &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_neg_zero;
    TEST_SETUP_EXCEPT();
    uint64_t fld_neg_zero_value = ai_fld_raw_bits(ai_arch_fp_addr(0));
    TEST_ASSERT("fld preserves the double-precision -0.0 sign bit and keeps fcsr stable",
        fld_neg_zero_value == fld_neg_zero &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == guard &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_subnormal;
    TEST_SETUP_EXCEPT();
    uint64_t fld_subnormal_value = ai_fld_raw_bits(ai_arch_fp_addr(0));
    TEST_ASSERT("fld preserves a non-canonical subnormal bit pattern and leaves fcsr unchanged",
        fld_subnormal_value == fld_subnormal &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == guard &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = guard;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = 0xaabbccddeeff0011ULL;
    TEST_SETUP_EXCEPT();
    ai_fsw_raw_bits(ai_arch_fp_addr(4), 0xdeadbeef7fc12345ULL);
    TEST_ASSERT("fsw writes only the low 32 raw bits from the source FPR into the addressed word lane and keeps fcsr stable",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) ==
            ((uint64_t)flw_qnan << 32 | (uint32_t)guard) &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == 0xaabbccddeeff0011ULL &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    TEST_SETUP_EXCEPT();
    ai_fsd_raw_bits(ai_arch_fp_addr(8), fld_snan);
    TEST_ASSERT("fsd writes the complete 64-bit raw payload into the target doubleword and keeps the first doubleword plus fcsr stable",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) ==
            ((uint64_t)flw_qnan << 32 | (uint32_t)guard) &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == fld_snan &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = guard;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = 0xaabbccddeeff0011ULL;
    TEST_SETUP_EXCEPT();
    uint64_t fsw_other_fpr = ai_fsw_non_target_fpr_probe(
        ai_arch_fp_addr(4), 0xdeadbeef7fc12345ULL, non_target_fpr);
    uint64_t fsd_other_fpr = ai_fsd_non_target_fpr_probe(
        ai_arch_fp_addr(8), fld_snan, non_target_fpr);
    TEST_ASSERT("fsw/fsd consume only the source FPR, update memory, and preserve an independent non-target FPR sentinel",
        excpt.triggered == false &&
        fsw_other_fpr == non_target_fpr &&
        fsd_other_fpr == non_target_fpr &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) ==
            ((uint64_t)flw_qnan << 32 | (uint32_t)guard) &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == fld_snan &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = guard;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = fld_normal;
    TEST_SETUP_EXCEPT();
    uint64_t fld_second_value = ai_fld_raw_bits(ai_arch_fp_addr(8));
    uint64_t fld_second_other_fpr =
        ai_fld_non_target_fpr_probe(ai_arch_fp_addr(8), non_target_fpr);
    TEST_ASSERT("fld from the second doubleword returns the exact 64-bit payload and leaves the first doubleword guard plus non-target FPR and fcsr stable",
        excpt.triggered == false &&
        fld_second_value == fld_normal &&
        fld_second_other_fpr == non_target_fpr &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) == guard &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == fld_normal &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) =
        ((uint64_t)flw_neg_zero << 32) | flw_snan;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    ai_flw_fsw_round_trip(ai_arch_fp_addr(0), ai_arch_fp_addr(4));
    TEST_ASSERT("flw followed by fsw round-trips an sNaN payload without canonicalization and does not disturb fcsr",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) ==
            ((uint64_t)flw_snan << 32 | flw_snan) &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == guard &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) =
        ((uint64_t)flw_neg_zero << 32) | flw_qnan;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    ai_flw_fsw_round_trip(ai_arch_fp_addr(0), ai_arch_fp_addr(4));
    TEST_ASSERT("flw followed by fsw round-trips a qNaN payload without canonicalization and keeps fcsr stable",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) ==
            ((uint64_t)flw_qnan << 32 | flw_qnan) &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == guard &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) =
        ((uint64_t)flw_qnan << 32) | flw_neg_zero;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    ai_flw_fsw_round_trip(ai_arch_fp_addr(0), ai_arch_fp_addr(4));
    TEST_ASSERT("flw followed by fsw round-trips a -0.0 sign bit without canonicalization and keeps fcsr stable",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) ==
            ((uint64_t)flw_neg_zero << 32 | flw_neg_zero) &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == guard &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_qnan;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    ai_fld_fsd_round_trip(ai_arch_fp_addr(0), ai_arch_fp_addr(8));
    TEST_ASSERT("fld followed by fsd round-trips a qNaN payload exactly and leaves fcsr unchanged",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) == fld_qnan &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == fld_qnan &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_snan;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    ai_fld_fsd_round_trip(ai_arch_fp_addr(0), ai_arch_fp_addr(8));
    TEST_ASSERT("fld followed by fsd round-trips an sNaN payload exactly and leaves fcsr unchanged",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) == fld_snan &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == fld_snan &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_neg_zero;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    ai_fld_fsd_round_trip(ai_arch_fp_addr(0), ai_arch_fp_addr(8));
    TEST_ASSERT("fld followed by fsd round-trips the -0.0 sign bit exactly and leaves fcsr unchanged",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) == fld_neg_zero &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == fld_neg_zero &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_subnormal;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    ai_fld_fsd_round_trip(ai_arch_fp_addr(0), ai_arch_fp_addr(8));
    TEST_ASSERT("fld followed by fsd round-trips a subnormal payload exactly and leaves fcsr unchanged",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) == fld_subnormal &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == fld_subnormal &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_positive_inf;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    ai_fld_fsd_round_trip(ai_arch_fp_addr(0), ai_arch_fp_addr(8));
    TEST_ASSERT("fld followed by fsd round-trips an infinity bit pattern exactly and leaves fcsr unchanged",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) == fld_positive_inf &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == fld_positive_inf &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    *(volatile uint64_t *)(ai_arch_fp_mem + 0) = fld_normal;
    *(volatile uint64_t *)(ai_arch_fp_mem + 8) = guard;
    TEST_SETUP_EXCEPT();
    ai_fld_fsd_round_trip(ai_arch_fp_addr(0), ai_arch_fp_addr(8));
    TEST_ASSERT("fld followed by fsd round-trips an ordinary non-canonical-check payload exactly and leaves fcsr unchanged",
        *(volatile uint64_t *)(ai_arch_fp_mem + 0) == fld_normal &&
        *(volatile uint64_t *)(ai_arch_fp_mem + 8) == fld_normal &&
        ai_arch_fp_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    TEST_END();
}

bool ai_arch_fp_fs_off_illegal_no_fpr_memory_or_pte_side_effects_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x120U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x120U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x220U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x220U;
    const uint64_t target_initial = 0x8877665544332211ULL;
    const uint64_t guard_initial = 0xaabbccddeeff0011ULL;
    const uint64_t target_fpr_sentinel = 0x7ff80000deadbabeULL;
    const uint64_t non_target_fpr_sentinel = 0x55aa33cc77ee11ffULL;
    const uint64_t fflags_sentinel = 0x12ULL;
    const uint64_t legal_leaf_no_ad = PTE_V | PTE_RWX;
    uint64_t observed_target_fpr = 0ULL;
    uint64_t observed_non_target_fpr = 0ULL;

    goto_priv(PRIV_M);
    ai_arch_enable_fp();
    write64(target_paddr, target_initial);
    write64(guard_paddr, guard_initial);

    ai_arch_fp_prepare_hs();

    goto_priv(PRIV_M);
    *ai_arch_fp_hs_leaf_entry(target_page) =
        (phys_page_base(target_page) >> 2) | legal_leaf_no_ad;
    *ai_arch_fp_hs_leaf_entry(guard_page) =
        (phys_page_base(guard_page) >> 2) | legal_leaf_no_ad;
    const uint64_t target_leaf_word =
        *ai_arch_fp_hs_leaf_entry(target_page);
    const uint64_t guard_leaf_word =
        *ai_arch_fp_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the FS-off FP illegal probe starts from valid RWX HS leaves with A/D clear and stable backing memory",
        (target_leaf_word & (PTE_V | PTE_RWX)) ==
            (PTE_V | PTE_RWX) &&
        (target_leaf_word & PTE_AD) == 0 &&
        (guard_leaf_word & (PTE_V | PTE_RWX)) ==
            (PTE_V | PTE_RWX) &&
        (guard_leaf_word & PTE_AD) == 0 &&
        read64(target_paddr) == target_initial &&
        read64(guard_paddr) == guard_initial
    );

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    ai_arch_fp_write_fcsr(fflags_sentinel, 0ULL);
    ai_arch_fp_seed_ft0_ft1(target_fpr_sentinel, non_target_fpr_sentinel);
    ai_arch_disable_fp();

    TEST_SETUP_EXCEPT();
    ai_arch_fp_illegal_flw_probe(target_vaddr);
    TEST_ASSERT("FS=Off flw raises illegal instruction before data translation and does not report a load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fp_illegal_fld_probe(target_vaddr);
    TEST_ASSERT("FS=Off fld raises illegal instruction before data translation and does not report a load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    ai_arch_fp_read_ft0_ft1(&observed_target_fpr, &observed_non_target_fpr);
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("FS=Off FP loads do not clobber the target FPR, non-target FPR, fflags, target data, guard data, or PTE A/D bits",
        observed_target_fpr == target_fpr_sentinel &&
        observed_non_target_fpr == non_target_fpr_sentinel &&
        (CSRR(CSR_FFLAGS) & 0x1fULL) == fflags_sentinel &&
        read64(target_paddr) == target_initial &&
        read64(guard_paddr) == guard_initial &&
        *ai_arch_fp_hs_leaf_entry(target_page) == target_leaf_word &&
        *ai_arch_fp_hs_leaf_entry(guard_page) == guard_leaf_word
    );

    goto_priv(PRIV_HS);
    ai_arch_disable_fp();

    TEST_SETUP_EXCEPT();
    ai_arch_fp_illegal_fsw_probe(target_vaddr + sizeof(uint32_t));
    TEST_ASSERT("FS=Off fsw raises illegal instruction before issuing a data store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fp_illegal_fsd_probe(target_vaddr);
    TEST_ASSERT("FS=Off fsd raises illegal instruction before issuing a data store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    ai_arch_fp_read_ft0_ft1(&observed_target_fpr, &observed_non_target_fpr);
    TEST_ASSERT("FS=Off FP stores do not update target memory, byte lanes, guard memory, PTE A/D bits, fflags, or FPR sentinels",
        observed_target_fpr == target_fpr_sentinel &&
        observed_non_target_fpr == non_target_fpr_sentinel &&
        (CSRR(CSR_FFLAGS) & 0x1fULL) == fflags_sentinel &&
        read64(target_paddr) == target_initial &&
        read64(guard_paddr) == guard_initial &&
        *ai_arch_fp_hs_leaf_entry(target_page) == target_leaf_word &&
        *ai_arch_fp_hs_leaf_entry(guard_page) == guard_leaf_word
    );

    TEST_END();
}

bool ai_arch_amoswap_w_exchange()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoswap_w(ai_arch_amo_addr(4), 0xa1b2c3d4U);
    AI_ASSERT_NO_EXCEPT("amoswap.w returns the old word and writes rs2 into memory",
        old_value == 0x11223344U &&
        ai_arch_amo_read64() == 0xa1b2c3d455667788ULL
    );

    TEST_END();
}

bool ai_arch_amoadd_w_accumulate()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x0000001055667788ULL);

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoadd_w(ai_arch_amo_addr(4), 0x22U);
    AI_ASSERT_NO_EXCEPT("amoadd.w returns the old word and stores A+B",
        old_value == 0x00000010U &&
        ai_arch_amo_read64() == 0x0000003255667788ULL
    );

    TEST_END();
}

bool ai_arch_amoand_w_mask()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xf0f0aa5555667788ULL);

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoand_w(ai_arch_amo_addr(4), 0x0ff00f0fU);
    AI_ASSERT_NO_EXCEPT("amoand.w returns the old word and stores A&B",
        old_value == 0xf0f0aa55U &&
        ai_arch_amo_read64() == 0x00f00a0555667788ULL
    );

    TEST_END();
}

bool ai_arch_amoor_w_merge()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x1200340055667788ULL);

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoor_w(ai_arch_amo_addr(4), 0x00f0004fU);
    AI_ASSERT_NO_EXCEPT("amoor.w returns the old word and stores A|B",
        old_value == 0x12003400U &&
        ai_arch_amo_read64() == 0x12f0344f55667788ULL
    );

    TEST_END();
}

bool ai_arch_amoxor_w_toggle()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xaa55cc3355667788ULL);

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoxor_w(ai_arch_amo_addr(4), 0x0f0f00ffU);
    AI_ASSERT_NO_EXCEPT("amoxor.w returns the old word and stores A^B",
        old_value == 0xaa55cc33U &&
        ai_arch_amo_read64() == 0xa55acccc55667788ULL
    );

    TEST_END();
}

bool ai_arch_amomin_w_signed()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x0000001055667788ULL);

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amomin_w(ai_arch_amo_addr(4), 0xfffffff0U);
    AI_ASSERT_NO_EXCEPT("amomin.w compares operands as signed values",
        old_value == 0x00000010U &&
        ai_arch_amo_read64() == 0xfffffff055667788ULL
    );

    TEST_END();
}

bool ai_arch_amomax_w_signed()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xfffffff055667788ULL);

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amomax_w(ai_arch_amo_addr(4), 0x00000020U);
    AI_ASSERT_NO_EXCEPT("amomax.w compares operands as signed values",
        old_value == 0xfffffff0U &&
        ai_arch_amo_read64() == 0x0000002055667788ULL
    );

    TEST_END();
}

bool ai_arch_amominu_w_unsigned()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xffffff0055667788ULL);

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amominu_w(ai_arch_amo_addr(4), 0x00000020U);
    AI_ASSERT_NO_EXCEPT("amominu.w compares operands as unsigned values",
        old_value == 0xffffff00U &&
        ai_arch_amo_read64() == 0x0000002055667788ULL
    );

    TEST_END();
}

bool ai_arch_amomaxu_w_unsigned()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x0000002055667788ULL);

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amomaxu_w(ai_arch_amo_addr(4), 0xffffff00U);
    AI_ASSERT_NO_EXCEPT("amomaxu.w compares operands as unsigned values",
        old_value == 0x00000020U &&
        ai_arch_amo_read64() == 0xffffff0055667788ULL
    );

    TEST_END();
}

bool ai_arch_amo_d_variants()
{
    TEST_START();

    goto_priv(PRIV_M);

    ai_arch_amo_write64(0x1122334455667788ULL);
    TEST_SETUP_EXCEPT();
    uint64_t old_swap = amoswap_d(ai_arch_amo_addr(0), 0xa1b2c3d4e5f60718ULL);
    AI_ASSERT_NO_EXCEPT("amoswap.d returns the old doubleword and writes rs2",
        old_swap == 0x1122334455667788ULL &&
        ai_arch_amo_read64() == 0xa1b2c3d4e5f60718ULL
    );

    ai_arch_amo_write64(0x0000000000000010ULL);
    TEST_SETUP_EXCEPT();
    uint64_t old_add = amoadd_d(ai_arch_amo_addr(0), 0x22ULL);
    AI_ASSERT_NO_EXCEPT("amoadd.d returns the old doubleword and stores A+B",
        old_add == 0x0000000000000010ULL &&
        ai_arch_amo_read64() == 0x0000000000000032ULL
    );

    ai_arch_amo_write64(0xff00ff00ff00ff00ULL);
    TEST_SETUP_EXCEPT();
    uint64_t old_and = amoand_d(ai_arch_amo_addr(0), 0x0f0f0f0f0f0f0f0fULL);
    AI_ASSERT_NO_EXCEPT("amoand.d returns the old doubleword and stores A&B",
        old_and == 0xff00ff00ff00ff00ULL &&
        ai_arch_amo_read64() == 0x0f000f000f000f00ULL
    );

    ai_arch_amo_write64(0x1200340000003400ULL);
    TEST_SETUP_EXCEPT();
    uint64_t old_or = amoor_d(ai_arch_amo_addr(0), 0x00f0004f0000004fULL);
    AI_ASSERT_NO_EXCEPT("amoor.d returns the old doubleword and stores A|B",
        old_or == 0x1200340000003400ULL &&
        ai_arch_amo_read64() == 0x12f0344f0000344fULL
    );

    ai_arch_amo_write64(0xaaaaaaaa55555555ULL);
    TEST_SETUP_EXCEPT();
    uint64_t old_xor = amoxor_d(ai_arch_amo_addr(0), 0xffffffff0000ffffULL);
    AI_ASSERT_NO_EXCEPT("amoxor.d returns the old doubleword and stores A^B",
        old_xor == 0xaaaaaaaa55555555ULL &&
        ai_arch_amo_read64() == 0x555555555555aaaaULL
    );

    ai_arch_amo_write64(0x0000000000000010ULL);
    TEST_SETUP_EXCEPT();
    uint64_t old_min = amomin_d(ai_arch_amo_addr(0), 0xfffffffffffffff0ULL);
    AI_ASSERT_NO_EXCEPT("amomin.d compares operands as signed values",
        old_min == 0x0000000000000010ULL &&
        ai_arch_amo_read64() == 0xfffffffffffffff0ULL
    );

    ai_arch_amo_write64(0xfffffffffffffff0ULL);
    TEST_SETUP_EXCEPT();
    uint64_t old_max = amomax_d(ai_arch_amo_addr(0), 0x0000000000000020ULL);
    AI_ASSERT_NO_EXCEPT("amomax.d compares operands as signed values",
        old_max == 0xfffffffffffffff0ULL &&
        ai_arch_amo_read64() == 0x0000000000000020ULL
    );

    ai_arch_amo_write64(0xffffffffffffff00ULL);
    TEST_SETUP_EXCEPT();
    uint64_t old_minu = amominu_d(ai_arch_amo_addr(0), 0x0000000000000020ULL);
    AI_ASSERT_NO_EXCEPT("amominu.d compares operands as unsigned values",
        old_minu == 0xffffffffffffff00ULL &&
        ai_arch_amo_read64() == 0x0000000000000020ULL
    );

    ai_arch_amo_write64(0x0000000000000020ULL);
    TEST_SETUP_EXCEPT();
    uint64_t old_maxu = amomaxu_d(ai_arch_amo_addr(0), 0xffffffffffffff00ULL);
    AI_ASSERT_NO_EXCEPT("amomaxu.d compares operands as unsigned values",
        old_maxu == 0x0000000000000020ULL &&
        ai_arch_amo_read64() == 0xffffffffffffff00ULL
    );

    TEST_END();
}

bool ai_arch_amo_rmw_op_width_visible_matrix_corner()
{
    TEST_START();

    const uint64_t guard = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);

    ai_arch_amo_write64_at(0, 0x8000000155667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_swap_w = ai_amoswap_w_raw64(ai_arch_amo_addr(4), 0x01234567U);
    AI_ASSERT_NO_EXCEPT("amoswap.w returns the sign-extended old word, writes rs2 to the target lane, and preserves guards",
        old_swap_w == 0xffffffff80000001ULL &&
        ai_arch_amo_read64_at(0) == 0x0123456755667788ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0xffffffff55667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_add_w = ai_amoadd_w_raw64(ai_arch_amo_addr(4), 0x2U);
    AI_ASSERT_NO_EXCEPT("amoadd.w wraps modulo 32 bits, returns old, and preserves non-target lanes",
        old_add_w == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x0000000155667788ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0xf0f0aa5555667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_and_w = ai_amoand_w_raw64(ai_arch_amo_addr(4), 0x0ff00f0fU);
    AI_ASSERT_NO_EXCEPT("amoand.w returns old and writes the 32-bit AND result only to the target word",
        old_and_w == 0xfffffffff0f0aa55ULL &&
        ai_arch_amo_read64_at(0) == 0x00f00a0555667788ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x1200340055667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_or_w = ai_amoor_w_raw64(ai_arch_amo_addr(4), 0x00f0004fU);
    AI_ASSERT_NO_EXCEPT("amoor.w returns old and writes the 32-bit OR result only to the target word",
        old_or_w == 0x0000000012003400ULL &&
        ai_arch_amo_read64_at(0) == 0x12f0344f55667788ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0xaa55cc3355667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_xor_w = ai_amoxor_w_raw64(ai_arch_amo_addr(4), 0x0f0f00ffU);
    AI_ASSERT_NO_EXCEPT("amoxor.w returns old and writes the 32-bit XOR result only to the target word",
        old_xor_w == 0xffffffffaa55cc33ULL &&
        ai_arch_amo_read64_at(0) == 0xa55acccc55667788ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x8000000055667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_min_w = ai_amomin_w_raw64(ai_arch_amo_addr(4), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomin.w compares signed 32-bit operands and keeps INT32_MIN in the target lane",
        old_min_w == 0xffffffff80000000ULL &&
        ai_arch_amo_read64_at(0) == 0x8000000055667788ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x8000000055667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_max_w = ai_amomax_w_raw64(ai_arch_amo_addr(4), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomax.w compares signed 32-bit operands and writes INT32_MAX to the target lane",
        old_max_w == 0xffffffff80000000ULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffff55667788ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0xffffffff55667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_minu_w = ai_amominu_w_raw64(ai_arch_amo_addr(4), 0x00000000U);
    AI_ASSERT_NO_EXCEPT("amominu.w compares unsigned 32-bit operands and writes zero to the target lane",
        old_minu_w == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x0000000055667788ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x7fffffff55667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_maxu_w = ai_amomaxu_w_raw64(ai_arch_amo_addr(4), 0x80000000U);
    AI_ASSERT_NO_EXCEPT("amomaxu.w compares unsigned 32-bit operands across the sign boundary and preserves the lower guard word",
        old_maxu_w == 0x000000007fffffffULL &&
        ai_arch_amo_read64_at(0) == 0x8000000055667788ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x112233447fffffffULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_maxu_w_low = ai_amomaxu_w_raw64(ai_arch_amo_addr(0), 0x80000000U);
    AI_ASSERT_NO_EXCEPT("amomaxu.w on the lower word updates only that lane while preserving the upper guard word",
        old_maxu_w_low == 0x000000007fffffffULL &&
        ai_arch_amo_read64_at(0) == 0x1122334480000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x1122334480000001ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_swap_w_low = ai_amoswap_w_raw64(ai_arch_amo_addr(0), 0x01234567U);
    AI_ASSERT_NO_EXCEPT("amoswap.w on the lower word sign-extends old rd and preserves the upper guard word",
        old_swap_w_low == 0xffffffff80000001ULL &&
        ai_arch_amo_read64_at(0) == 0x1122334401234567ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x11223344ffffffffULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_add_w_low = ai_amoadd_w_raw64(ai_arch_amo_addr(0), 0x2U);
    AI_ASSERT_NO_EXCEPT("amoadd.w on the lower word wraps modulo 32 bits and preserves the upper guard word",
        old_add_w_low == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x1122334400000001ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x11223344f0f0aa55ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_and_w_low = ai_amoand_w_raw64(ai_arch_amo_addr(0), 0x0ff00f0fU);
    AI_ASSERT_NO_EXCEPT("amoand.w on the lower word updates only the target lane and preserves the upper guard word",
        old_and_w_low == 0xfffffffff0f0aa55ULL &&
        ai_arch_amo_read64_at(0) == 0x1122334400f00a05ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x1122334412003400ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_or_w_low = ai_amoor_w_raw64(ai_arch_amo_addr(0), 0x00f0004fU);
    AI_ASSERT_NO_EXCEPT("amoor.w on the lower word updates only the target lane and preserves the upper guard word",
        old_or_w_low == 0x0000000012003400ULL &&
        ai_arch_amo_read64_at(0) == 0x1122334412f0344fULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x11223344aa55cc33ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_xor_w_low = ai_amoxor_w_raw64(ai_arch_amo_addr(0), 0x0f0f00ffU);
    AI_ASSERT_NO_EXCEPT("amoxor.w on the lower word updates only the target lane and preserves the upper guard word",
        old_xor_w_low == 0xffffffffaa55cc33ULL &&
        ai_arch_amo_read64_at(0) == 0x11223344a55accccULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x1122334480000000ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_min_w_low = ai_amomin_w_raw64(ai_arch_amo_addr(0), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomin.w on the lower word keeps the signed minimum and preserves the upper guard word",
        old_min_w_low == 0xffffffff80000000ULL &&
        ai_arch_amo_read64_at(0) == 0x1122334480000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x1122334480000000ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_max_w_low = ai_amomax_w_raw64(ai_arch_amo_addr(0), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomax.w on the lower word selects the signed maximum and preserves the upper guard word",
        old_max_w_low == 0xffffffff80000000ULL &&
        ai_arch_amo_read64_at(0) == 0x112233447fffffffULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x11223344ffffffffULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_minu_w_low = ai_amominu_w_raw64(ai_arch_amo_addr(0), 0x00000000U);
    AI_ASSERT_NO_EXCEPT("amominu.w on the lower word selects unsigned zero and preserves the upper guard word",
        old_minu_w_low == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x1122334400000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x1122334455667788ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_swap_d = amoswap_d(ai_arch_amo_addr(0), 0xa1b2c3d4e5f60718ULL);
    AI_ASSERT_NO_EXCEPT("amoswap.d returns the full old doubleword, writes rs2, and preserves the adjacent doubleword",
        old_swap_d == 0x1122334455667788ULL &&
        ai_arch_amo_read64_at(0) == 0xa1b2c3d4e5f60718ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0xffffffffffffffffULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_add_d = amoadd_d(ai_arch_amo_addr(0), 0x2ULL);
    AI_ASSERT_NO_EXCEPT("amoadd.d wraps modulo 64 bits and preserves the adjacent doubleword",
        old_add_d == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x0000000000000001ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0xff00ff00ff00ff00ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_and_d = amoand_d(ai_arch_amo_addr(0), 0x0f0f0f0f0f0f0f0fULL);
    AI_ASSERT_NO_EXCEPT("amoand.d returns old and writes the 64-bit AND result",
        old_and_d == 0xff00ff00ff00ff00ULL &&
        ai_arch_amo_read64_at(0) == 0x0f000f000f000f00ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x1200340000003400ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_or_d = amoor_d(ai_arch_amo_addr(0), 0x00f0004f0000004fULL);
    AI_ASSERT_NO_EXCEPT("amoor.d returns old and writes the 64-bit OR result",
        old_or_d == 0x1200340000003400ULL &&
        ai_arch_amo_read64_at(0) == 0x12f0344f0000344fULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0xaaaaaaaa55555555ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_xor_d = amoxor_d(ai_arch_amo_addr(0), 0xffffffff0000ffffULL);
    AI_ASSERT_NO_EXCEPT("amoxor.d returns old and writes the 64-bit XOR result",
        old_xor_d == 0xaaaaaaaa55555555ULL &&
        ai_arch_amo_read64_at(0) == 0x555555555555aaaaULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x8000000000000000ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_min_d = amomin_d(ai_arch_amo_addr(0), 0x7fffffffffffffffULL);
    AI_ASSERT_NO_EXCEPT("amomin.d compares signed 64-bit operands and keeps INT64_MIN",
        old_min_d == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(0) == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x8000000000000000ULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_max_d = amomax_d(ai_arch_amo_addr(0), 0x7fffffffffffffffULL);
    AI_ASSERT_NO_EXCEPT("amomax.d compares signed 64-bit operands and writes INT64_MAX",
        old_max_d == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffffffffffffULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0xffffffffffffffffULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_minu_d = amominu_d(ai_arch_amo_addr(0), 0x0ULL);
    AI_ASSERT_NO_EXCEPT("amominu.d compares unsigned 64-bit operands and writes zero",
        old_minu_d == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x0000000000000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    ai_arch_amo_write64_at(0, 0x7fffffffffffffffULL);
    ai_arch_amo_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    uint64_t old_maxu_d = amomaxu_d(ai_arch_amo_addr(0), 0x8000000000000000ULL);
    AI_ASSERT_NO_EXCEPT("amomaxu.d compares unsigned 64-bit operands across the sign boundary",
        old_maxu_d == 0x7fffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_END();
}

bool ai_arch_amoalu_cmd_width_signedness_churn_corner()
{
    TEST_START();

    const uint64_t guard = 0x0badc0de55aa33ccULL;

    goto_priv(PRIV_M);

    ai_arch_amo_write64_at(0, 0x7fffffff00000000ULL);
    ai_arch_amo_write64_at(8, guard);

    TEST_SETUP_EXCEPT();
    uint64_t old_max_w_hi =
        ai_amomax_w_raw64(ai_arch_amo_addr(4), 0x80000000U);
    AI_ASSERT_NO_EXCEPT("amomax.w upper lane keeps signed positive max while preserving lower lane and guard",
        old_max_w_hi == 0x000000007fffffffULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffff00000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_minu_w_lo =
        ai_amominu_w_raw64(ai_arch_amo_addr(0), 0xffffffffU);
    AI_ASSERT_NO_EXCEPT("amominu.w lower lane keeps unsigned zero after upper signed max",
        old_minu_w_lo == 0x0000000000000000ULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffff00000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_add_w_hi =
        ai_amoadd_w_raw64(ai_arch_amo_addr(4), 0x00000001U);
    AI_ASSERT_NO_EXCEPT("amoadd.w upper lane wraps INT32_MAX to sign bit without carry into lower lane",
        old_add_w_hi == 0x000000007fffffffULL &&
        ai_arch_amo_read64_at(0) == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_minu_d =
        amominu_d(ai_arch_amo_addr(0), 0x7fffffffffffffffULL);
    AI_ASSERT_NO_EXCEPT("amominu.d after amoadd.w uses full 64-bit unsigned comparator and write mask",
        old_minu_d == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffffffffffffULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_and_w_lo =
        ai_amoand_w_raw64(ai_arch_amo_addr(0), 0x0f0f0f0fU);
    AI_ASSERT_NO_EXCEPT("amoand.w lower lane after .d min keeps upper lane and writes only A&B",
        old_and_w_lo == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffff0f0f0f0fULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_or_w_lo =
        ai_amoor_w_raw64(ai_arch_amo_addr(0), 0x333300f0U);
    AI_ASSERT_NO_EXCEPT("amoor.w lower lane after AND produces OR-specific value distinct from AND and XOR",
        old_or_w_lo == 0x000000000f0f0f0fULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffff3f3f0fffULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_xor_w_lo =
        ai_amoxor_w_raw64(ai_arch_amo_addr(0), 0x00ff00ffU);
    AI_ASSERT_NO_EXCEPT("amoxor.w lower lane after OR toggles only the target lane",
        old_xor_w_lo == 0x000000003f3f0fffULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffff3fc00f00ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_max_d =
        amomax_d(ai_arch_amo_addr(0), 0x8000000000000001ULL);
    AI_ASSERT_NO_EXCEPT("amomax.d uses signed 64-bit compare after logic .w churn",
        old_max_d == 0x7fffffff3fc00f00ULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffff3fc00f00ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_maxu_d =
        amomaxu_d(ai_arch_amo_addr(0), 0x8000000000000001ULL);
    AI_ASSERT_NO_EXCEPT("amomaxu.d immediately after signed max flips to unsigned ordering",
        old_maxu_d == 0x7fffffff3fc00f00ULL &&
        ai_arch_amo_read64_at(0) == 0x8000000000000001ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_max_w_lo =
        ai_amomax_w_raw64(ai_arch_amo_addr(0), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomax.w lower lane after .d maxu does not inherit the 64-bit write mask",
        old_max_w_lo == 0x0000000000000001ULL &&
        ai_arch_amo_read64_at(0) == 0x800000007fffffffULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_minu_w_hi =
        ai_amominu_w_raw64(ai_arch_amo_addr(4), 0x00000000U);
    AI_ASSERT_NO_EXCEPT("amominu.w upper lane switches back to unsigned 32-bit compare and preserves lower lane",
        old_minu_w_hi == 0xffffffff80000000ULL &&
        ai_arch_amo_read64_at(0) == 0x000000007fffffffULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_add_d = amoadd_d(ai_arch_amo_addr(0), 0x1ULL);
    AI_ASSERT_NO_EXCEPT("amoadd.d after upper .w minu restores full-width add and final memory image",
        old_add_d == 0x000000007fffffffULL &&
        ai_arch_amo_read64_at(0) == 0x0000000080000000ULL &&
        ai_arch_amo_read64_at(8) == guard
    );

    TEST_END();
}

bool ai_arch_amo_aq_rl_rmw_semantics_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, 0x0000001055667788ULL);
    ai_arch_amo_write64_at(8, 0x8899aabbccddeeffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_aq = ai_amoadd_w_aq_raw64(ai_arch_amo_addr(4), 0x22U);
    AI_ASSERT_NO_EXCEPT("amoadd.w.aq keeps the normal old-value return and target word RMW semantics",
        old_aq == 0x0000000000000010ULL &&
        ai_arch_amo_read64_at(0) == 0x0000003255667788ULL &&
        ai_arch_amo_read64_at(8) == 0x8899aabbccddeeffULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_rl = ai_amoadd_w_rl_raw64(ai_arch_amo_addr(4), 0x1U);
    AI_ASSERT_NO_EXCEPT("amoadd.w.rl keeps the normal old-value return and target word RMW semantics",
        old_rl == 0x0000000000000032ULL &&
        ai_arch_amo_read64_at(0) == 0x0000003355667788ULL &&
        ai_arch_amo_read64_at(8) == 0x8899aabbccddeeffULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_w_aqrl = ai_amoadd_w_aqrl_raw64(ai_arch_amo_addr(4), 0xffffffcdU);
    AI_ASSERT_NO_EXCEPT("amoadd.w.aqrl keeps target-word RMW sequencing while preserving the unrelated lower word",
        old_w_aqrl == 0x0000000000000033ULL &&
        ai_arch_amo_read64_at(0) == 0x0000000055667788ULL &&
        ai_arch_amo_read64_at(8) == 0x8899aabbccddeeffULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_d_aq = ai_amoadd_d_aq_raw64(ai_arch_amo_addr(0), 0x10ULL);
    AI_ASSERT_NO_EXCEPT("amoadd.d.aq keeps the normal old-value return and full doubleword RMW semantics",
        old_d_aq == 0x0000000055667788ULL &&
        ai_arch_amo_read64_at(0) == 0x0000000055667798ULL &&
        ai_arch_amo_read64_at(8) == 0x8899aabbccddeeffULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_d_rl = ai_amoadd_d_rl_raw64(ai_arch_amo_addr(0), 0x1000000000000000ULL);
    AI_ASSERT_NO_EXCEPT("amoadd.d.rl keeps the normal old-value return and full doubleword RMW semantics",
        old_d_rl == 0x0000000055667798ULL &&
        ai_arch_amo_read64_at(0) == 0x1000000055667798ULL &&
        ai_arch_amo_read64_at(8) == 0x8899aabbccddeeffULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_aqrl = ai_amoadd_d_aqrl_raw64(ai_arch_amo_addr(0), 0x0111111100000001ULL);
    AI_ASSERT_NO_EXCEPT("amoadd.d.aqrl keeps the normal old-value return and full doubleword RMW semantics",
        old_aqrl == 0x1000000055667798ULL &&
        ai_arch_amo_read64_at(0) == 0x1111111155667799ULL &&
        ai_arch_amo_read64_at(8) == 0x8899aabbccddeeffULL
    );

    TEST_END();
}

bool ai_arch_amoswap_w_sign_extended_old_value_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x8000000155667788ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoswap_w_raw64(ai_arch_amo_addr(4), 0x01234567U);
    AI_ASSERT_NO_EXCEPT("amoswap.w sign-extends the old 32-bit word into rd on RV64 while updating only the targeted word",
        old_value == 0xffffffff80000001ULL &&
        ai_arch_amo_read64() == 0x0123456755667788ULL
    );

    TEST_END();
}

bool ai_arch_amoadd_w_wraparound_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xffffffff55667788ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoadd_w_raw64(ai_arch_amo_addr(4), 0x2U);
    AI_ASSERT_NO_EXCEPT("amoadd.w wraps modulo 2^32, sign-extends the old word in rd, and leaves the lower unrelated word untouched",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64() == 0x0000000155667788ULL
    );

    TEST_END();
}

bool ai_arch_amomaxu_w_sign_boundary_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x7fffffff55667788ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomaxu_w_raw64(ai_arch_amo_addr(4), 0x80000000U);
    AI_ASSERT_NO_EXCEPT("amomaxu.w compares operands as unsigned across the sign boundary and selects 0x80000000 over 0x7fffffff",
        old_value == 0x000000007fffffffULL &&
        ai_arch_amo_read64() == 0x8000000055667788ULL
    );

    TEST_END();
}

bool ai_arch_amomin_w_signed_boundary_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x8000000055667788ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomin_w_raw64(ai_arch_amo_addr(4), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomin.w compares operands as signed values across the INT32_MIN boundary and keeps 0x80000000 while sign-extending rd",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64() == 0x8000000055667788ULL
    );

    TEST_END();
}

bool ai_arch_amominu_w_zero_boundary_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xffffffff55667788ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amominu_w_raw64(ai_arch_amo_addr(4), 0x00000000U);
    AI_ASSERT_NO_EXCEPT("amominu.w compares operands as unsigned at the zero boundary, writes 0 into the target word, and sign-extends the old word in rd",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64() == 0x0000000055667788ULL
    );

    TEST_END();
}

bool ai_arch_amomax_w_signed_boundary_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x8000000055667788ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomax_w_raw64(ai_arch_amo_addr(4), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomax.w compares operands as signed values across the INT32_MIN boundary and selects 0x7fffffff while sign-extending rd",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64() == 0x7fffffff55667788ULL
    );

    TEST_END();
}

bool ai_arch_amoadd_d_wraparound_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xffffffffffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(ai_arch_amo_addr(0), 0x2ULL);
    AI_ASSERT_NO_EXCEPT("amoadd.d wraps modulo 2^64 and still returns the full old doubleword",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64() == 0x0000000000000001ULL
    );

    TEST_END();
}

bool ai_arch_amomin_d_signed_boundary_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x8000000000000000ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomin_d(ai_arch_amo_addr(0), 0x7fffffffffffffffULL);
    AI_ASSERT_NO_EXCEPT("amomin.d compares operands as signed values across the INT64_MIN boundary and keeps 0x8000000000000000",
        old_value == 0x8000000000000000ULL &&
        ai_arch_amo_read64() == 0x8000000000000000ULL
    );

    TEST_END();
}

bool ai_arch_amominu_d_zero_boundary_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xffffffffffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amominu_d(ai_arch_amo_addr(0), 0x0ULL);
    AI_ASSERT_NO_EXCEPT("amominu.d compares operands as unsigned at the zero boundary and stores 0 when it is the minimum",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64() == 0x0000000000000000ULL
    );

    TEST_END();
}

bool ai_arch_amomaxu_d_sign_boundary_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x7fffffffffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomaxu_d(ai_arch_amo_addr(0), 0x8000000000000000ULL);
    AI_ASSERT_NO_EXCEPT("amomaxu.d compares operands as unsigned across the sign boundary and selects 0x8000000000000000 over 0x7fffffffffffffff",
        old_value == 0x7fffffffffffffffULL &&
        ai_arch_amo_read64() == 0x8000000000000000ULL
    );

    TEST_END();
}

bool ai_arch_amomax_d_signed_boundary_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x8000000000000000ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomax_d(ai_arch_amo_addr(0), 0x7fffffffffffffffULL);
    AI_ASSERT_NO_EXCEPT("amomax.d compares operands as signed values across the INT64_MIN boundary and selects 0x7fffffffffffffff",
        old_value == 0x8000000000000000ULL &&
        ai_arch_amo_read64() == 0x7fffffffffffffffULL
    );

    TEST_END();
}

bool ai_arch_amoadd_d_wraparound_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const uint64_t adjacent_value = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, 0xffffffffffffffffULL);
    ai_arch_amo_write64_at(8, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(ai_arch_amo_addr(0), 0x2ULL);
    AI_ASSERT_NO_EXCEPT("amoadd.d wraps modulo 2^64 while preserving the adjacent doubleword outside the target 64-bit lane",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x0000000000000001ULL &&
        ai_arch_amo_read64_at(8) == adjacent_value
    );

    TEST_END();
}

bool ai_arch_amomin_d_signed_boundary_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const uint64_t adjacent_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, 0x8000000000000000ULL);
    ai_arch_amo_write64_at(8, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomin_d(ai_arch_amo_addr(0), 0x7fffffffffffffffULL);
    AI_ASSERT_NO_EXCEPT("amomin.d keeps INT64_MIN at the signed boundary and also leaves the adjacent doubleword untouched when the compare result does not change memory",
        old_value == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(0) == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(8) == adjacent_value
    );

    TEST_END();
}

bool ai_arch_amomaxu_d_sign_boundary_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const uint64_t adjacent_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, 0x7fffffffffffffffULL);
    ai_arch_amo_write64_at(8, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomaxu_d(ai_arch_amo_addr(0), 0x8000000000000000ULL);
    AI_ASSERT_NO_EXCEPT("amomaxu.d selects the larger unsigned operand across the sign boundary while preserving the adjacent doubleword outside the target lane",
        old_value == 0x7fffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(8) == adjacent_value
    );

    TEST_END();
}

bool ai_arch_amominu_d_zero_boundary_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const uint64_t adjacent_value = 0x0badc0de55aa33ccULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, 0xffffffffffffffffULL);
    ai_arch_amo_write64_at(8, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amominu_d(ai_arch_amo_addr(0), 0x0ULL);
    AI_ASSERT_NO_EXCEPT("amominu.d selects zero at the unsigned boundary while preserving the adjacent doubleword outside the target lane",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == 0x0000000000000000ULL &&
        ai_arch_amo_read64_at(8) == adjacent_value
    );

    TEST_END();
}

bool ai_arch_amomax_d_signed_boundary_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const uint64_t adjacent_value = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, 0x8000000000000000ULL);
    ai_arch_amo_write64_at(8, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomax_d(ai_arch_amo_addr(0), 0x7fffffffffffffffULL);
    AI_ASSERT_NO_EXCEPT("amomax.d selects INT64_MAX at the signed boundary while preserving the adjacent doubleword outside the target lane",
        old_value == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(0) == 0x7fffffffffffffffULL &&
        ai_arch_amo_read64_at(8) == adjacent_value
    );

    TEST_END();
}

bool ai_arch_amoand_d_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const uint64_t adjacent_value = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, 0xff00ff00ff00ff00ULL);
    ai_arch_amo_write64_at(8, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoand_d(ai_arch_amo_addr(0), 0x0f0f0f0f0f0f0f0fULL);
    AI_ASSERT_NO_EXCEPT("amoand.d updates only the targeted doubleword and preserves the adjacent doubleword outside the 64-bit lane",
        old_value == 0xff00ff00ff00ff00ULL &&
        ai_arch_amo_read64_at(0) == 0x0f000f000f000f00ULL &&
        ai_arch_amo_read64_at(8) == adjacent_value
    );

    TEST_END();
}

bool ai_arch_amoor_d_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const uint64_t adjacent_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, 0x1200340000003400ULL);
    ai_arch_amo_write64_at(8, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoor_d(ai_arch_amo_addr(0), 0x00f0004f0000004fULL);
    AI_ASSERT_NO_EXCEPT("amoor.d updates only the targeted doubleword and preserves the adjacent doubleword outside the 64-bit lane",
        old_value == 0x1200340000003400ULL &&
        ai_arch_amo_read64_at(0) == 0x12f0344f0000344fULL &&
        ai_arch_amo_read64_at(8) == adjacent_value
    );

    TEST_END();
}

bool ai_arch_amoxor_d_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const uint64_t adjacent_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, 0xaaaaaaaa55555555ULL);
    ai_arch_amo_write64_at(8, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoxor_d(ai_arch_amo_addr(0), 0xffffffff0000ffffULL);
    AI_ASSERT_NO_EXCEPT("amoxor.d updates only the targeted doubleword and preserves the adjacent doubleword outside the 64-bit lane",
        old_value == 0xaaaaaaaa55555555ULL &&
        ai_arch_amo_read64_at(0) == 0x555555555555aaaaULL &&
        ai_arch_amo_read64_at(8) == adjacent_value
    );

    TEST_END();
}

bool ai_arch_amoswap_w_lower_word_preserves_upper_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x55aa33cc80000001ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoswap_w_raw64(ai_arch_amo_addr(0), 0x01234567U);
    AI_ASSERT_NO_EXCEPT("amoswap.w on the lower 32-bit lane sign-extends rd correctly while preserving the upper unrelated word",
        old_value == 0xffffffff80000001ULL &&
        ai_arch_amo_read64() == 0x55aa33cc01234567ULL
    );

    TEST_END();
}

bool ai_arch_amoadd_w_wraparound_upper_word_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x11223344ffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoadd_w_raw64(ai_arch_amo_addr(0), 0x2U);
    AI_ASSERT_NO_EXCEPT("amoadd.w on the lower 32-bit lane wraps modulo 2^32 and leaves the upper unrelated word untouched",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64() == 0x1122334400000001ULL
    );

    TEST_END();
}

bool ai_arch_amomin_w_signed_boundary_upper_word_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x89abcdef80000000ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomin_w_raw64(ai_arch_amo_addr(0), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomin.w on the lower 32-bit lane keeps INT32_MIN at the signed boundary while preserving the upper unrelated word",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64() == 0x89abcdef80000000ULL
    );

    TEST_END();
}

bool ai_arch_amomaxu_w_sign_boundary_upper_word_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x13579bdf7fffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomaxu_w_raw64(ai_arch_amo_addr(0), 0x80000000U);
    AI_ASSERT_NO_EXCEPT("amomaxu.w on the lower 32-bit lane selects 0x80000000 across the unsigned sign boundary while preserving the upper unrelated word",
        old_value == 0x000000007fffffffULL &&
        ai_arch_amo_read64() == 0x13579bdf80000000ULL
    );

    TEST_END();
}

bool ai_arch_amominu_w_zero_boundary_upper_word_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x2468ace0ffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amominu_w_raw64(ai_arch_amo_addr(0), 0x00000000U);
    AI_ASSERT_NO_EXCEPT("amominu.w on the lower 32-bit lane selects zero at the unsigned boundary while preserving the upper unrelated word",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64() == 0x2468ace000000000ULL
    );

    TEST_END();
}

bool ai_arch_amomax_w_signed_boundary_upper_word_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xa55a33cc80000000ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomax_w_raw64(ai_arch_amo_addr(0), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomax.w on the lower 32-bit lane selects INT32_MAX across the signed boundary while preserving the upper unrelated word",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64() == 0xa55a33cc7fffffffULL
    );

    TEST_END();
}

bool ai_arch_amoand_w_lower_word_preserves_upper_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xcafebabef0f0aa55ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoand_w_raw64(ai_arch_amo_addr(0), 0x0ff00f0fU);
    AI_ASSERT_NO_EXCEPT("amoand.w on the lower 32-bit lane sign-extends rd correctly while preserving the upper unrelated word",
        old_value == 0xfffffffff0f0aa55ULL &&
        ai_arch_amo_read64() == 0xcafebabe00f00a05ULL
    );

    TEST_END();
}

bool ai_arch_amoor_w_lower_word_preserves_upper_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x1357246812003400ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoor_w_raw64(ai_arch_amo_addr(0), 0x00f0004fU);
    AI_ASSERT_NO_EXCEPT("amoor.w on the lower 32-bit lane updates only the target word and preserves the upper unrelated word",
        old_value == 0x0000000012003400ULL &&
        ai_arch_amo_read64() == 0x1357246812f0344fULL
    );

    TEST_END();
}

bool ai_arch_amoxor_w_lower_word_preserves_upper_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xdeadbeefaa55cc33ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoxor_w_raw64(ai_arch_amo_addr(0), 0x0f0f00ffU);
    AI_ASSERT_NO_EXCEPT("amoxor.w on the lower 32-bit lane sign-extends rd correctly while preserving the upper unrelated word",
        old_value == 0xffffffffaa55cc33ULL &&
        ai_arch_amo_read64() == 0xdeadbeefa55accccULL
    );

    TEST_END();
}

bool ai_arch_amoswap_w_upper_word_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x80000001cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoswap_w_raw64(ai_arch_amo_addr(4), 0x01234567U);
    AI_ASSERT_NO_EXCEPT("amoswap.w on the upper 32-bit lane sign-extends rd correctly while preserving the lower unrelated word",
        old_value == 0xffffffff80000001ULL &&
        ai_arch_amo_read64() == 0x01234567cafebabeULL
    );

    TEST_END();
}

bool ai_arch_amoadd_w_upper_word_wraparound_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xffffffff2468ace0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoadd_w_raw64(ai_arch_amo_addr(4), 0x2U);
    AI_ASSERT_NO_EXCEPT("amoadd.w on the upper 32-bit lane wraps modulo 2^32 and leaves the lower unrelated word untouched",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64() == 0x000000012468ace0ULL
    );

    TEST_END();
}

bool ai_arch_amomin_w_upper_word_signed_boundary_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x80000000a55a33ccULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomin_w_raw64(ai_arch_amo_addr(4), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomin.w on the upper 32-bit lane keeps INT32_MIN at the signed boundary while preserving the lower unrelated word",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64() == 0x80000000a55a33ccULL
    );

    TEST_END();
}

bool ai_arch_amomaxu_w_upper_word_sign_boundary_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x7fffffff13579bdfULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomaxu_w_raw64(ai_arch_amo_addr(4), 0x80000000U);
    AI_ASSERT_NO_EXCEPT("amomaxu.w on the upper 32-bit lane selects 0x80000000 across the unsigned sign boundary while preserving the lower unrelated word",
        old_value == 0x000000007fffffffULL &&
        ai_arch_amo_read64() == 0x8000000013579bdfULL
    );

    TEST_END();
}

bool ai_arch_amoand_w_upper_word_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xf0f0aa55cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoand_w_raw64(ai_arch_amo_addr(4), 0x0ff00f0fU);
    AI_ASSERT_NO_EXCEPT("amoand.w on the upper 32-bit lane sign-extends rd correctly while preserving the lower unrelated word",
        old_value == 0xfffffffff0f0aa55ULL &&
        ai_arch_amo_read64() == 0x00f00a05cafebabeULL
    );

    TEST_END();
}

bool ai_arch_amominu_w_upper_word_zero_boundary_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xffffffff2468ace0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amominu_w_raw64(ai_arch_amo_addr(4), 0x00000000U);
    AI_ASSERT_NO_EXCEPT("amominu.w on the upper 32-bit lane selects zero at the unsigned boundary while preserving the lower unrelated word",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64() == 0x000000002468ace0ULL
    );

    TEST_END();
}

bool ai_arch_amomax_w_upper_word_signed_boundary_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x80000000cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomax_w_raw64(ai_arch_amo_addr(4), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomax.w on the upper 32-bit lane selects INT32_MAX across the signed boundary while preserving the lower unrelated word",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64() == 0x7fffffffcafebabeULL
    );

    TEST_END();
}

bool ai_arch_amoor_w_upper_word_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0x12003400cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoor_w_raw64(ai_arch_amo_addr(4), 0x00f0004fU);
    AI_ASSERT_NO_EXCEPT("amoor.w on the upper 32-bit lane updates only the target word and preserves the lower unrelated word",
        old_value == 0x0000000012003400ULL &&
        ai_arch_amo_read64() == 0x12f0344fcafebabeULL
    );

    TEST_END();
}

bool ai_arch_amoxor_w_upper_word_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_amo_write64(0xaa55cc33cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoxor_w_raw64(ai_arch_amo_addr(4), 0x0f0f00ffU);
    AI_ASSERT_NO_EXCEPT("amoxor.w on the upper 32-bit lane sign-extends rd correctly while preserving the lower unrelated word",
        old_value == 0xffffffffaa55cc33ULL &&
        ai_arch_amo_read64() == 0xa55acccccafebabeULL
    );

    TEST_END();
}

bool ai_arch_amoswap_w_second_doubleword_low_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x55aa33cc80000001ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoswap_w_raw64(ai_arch_amo_addr(8), 0x01234567U);
    AI_ASSERT_NO_EXCEPT("amoswap.w on the low 32-bit word of the second doubleword sign-extends rd correctly while preserving the first doubleword",
        old_value == 0xffffffff80000001ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x55aa33cc01234567ULL
    );

    TEST_END();
}

bool ai_arch_amoadd_w_second_doubleword_low_word_wraparound_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x11223344ffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoadd_w_raw64(ai_arch_amo_addr(8), 0x2U);
    AI_ASSERT_NO_EXCEPT("amoadd.w on the low 32-bit word of the second doubleword wraps modulo 2^32 while preserving the first doubleword",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x1122334400000001ULL
    );

    TEST_END();
}

bool ai_arch_amomaxu_w_second_doubleword_low_word_sign_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x13579bdf7fffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomaxu_w_raw64(ai_arch_amo_addr(8), 0x80000000U);
    AI_ASSERT_NO_EXCEPT("amomaxu.w on the low 32-bit word of the second doubleword selects 0x80000000 across the unsigned sign boundary while preserving the first doubleword",
        old_value == 0x000000007fffffffULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x13579bdf80000000ULL
    );

    TEST_END();
}

bool ai_arch_amoand_w_second_doubleword_low_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0badc0de55aa33ccULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xcafebabef0f0aa55ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoand_w_raw64(ai_arch_amo_addr(8), 0x0ff00f0fU);
    AI_ASSERT_NO_EXCEPT("amoand.w on the low 32-bit word of the second doubleword sign-extends rd correctly while preserving the first doubleword",
        old_value == 0xfffffffff0f0aa55ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0xcafebabe00f00a05ULL
    );

    TEST_END();
}

bool ai_arch_amomin_w_second_doubleword_low_word_signed_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x89abcdef80000000ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomin_w_raw64(ai_arch_amo_addr(8), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomin.w on the low 32-bit word of the second doubleword keeps INT32_MIN at the signed boundary while preserving the first doubleword",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x89abcdef80000000ULL
    );

    TEST_END();
}

bool ai_arch_amominu_w_second_doubleword_low_word_zero_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x2468ace0ffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amominu_w_raw64(ai_arch_amo_addr(8), 0x00000000U);
    AI_ASSERT_NO_EXCEPT("amominu.w on the low 32-bit word of the second doubleword selects zero at the unsigned boundary while preserving the first doubleword",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x2468ace000000000ULL
    );

    TEST_END();
}

bool ai_arch_amomax_w_second_doubleword_low_word_signed_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xa55a33cc80000000ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomax_w_raw64(ai_arch_amo_addr(8), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomax.w on the low 32-bit word of the second doubleword selects INT32_MAX across the signed boundary while preserving the first doubleword",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0xa55a33cc7fffffffULL
    );

    TEST_END();
}

bool ai_arch_amoor_w_second_doubleword_low_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0badc0de55aa33ccULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x1357246812003400ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoor_w_raw64(ai_arch_amo_addr(8), 0x00f0004fU);
    AI_ASSERT_NO_EXCEPT("amoor.w on the low 32-bit word of the second doubleword updates only the target word and preserves the first doubleword",
        old_value == 0x0000000012003400ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x1357246812f0344fULL
    );

    TEST_END();
}

bool ai_arch_amoxor_w_second_doubleword_low_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xdeadbeefaa55cc33ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoxor_w_raw64(ai_arch_amo_addr(8), 0x0f0f00ffU);
    AI_ASSERT_NO_EXCEPT("amoxor.w on the low 32-bit word of the second doubleword sign-extends rd correctly while preserving the first doubleword",
        old_value == 0xffffffffaa55cc33ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0xdeadbeefa55accccULL
    );

    TEST_END();
}

bool ai_arch_amoswap_w_second_doubleword_upper_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x80000001cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoswap_w_raw64(ai_arch_amo_addr(12), 0x01234567U);
    AI_ASSERT_NO_EXCEPT("amoswap.w on the upper 32-bit word of the second doubleword sign-extends rd correctly while preserving the first doubleword",
        old_value == 0xffffffff80000001ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x01234567cafebabeULL
    );

    TEST_END();
}

bool ai_arch_amoadd_w_second_doubleword_upper_word_wraparound_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xffffffff2468ace0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoadd_w_raw64(ai_arch_amo_addr(12), 0x2U);
    AI_ASSERT_NO_EXCEPT("amoadd.w on the upper 32-bit word of the second doubleword wraps modulo 2^32 while preserving the first doubleword",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x000000012468ace0ULL
    );

    TEST_END();
}

bool ai_arch_amomaxu_w_second_doubleword_upper_word_sign_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x7fffffff13579bdfULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomaxu_w_raw64(ai_arch_amo_addr(12), 0x80000000U);
    AI_ASSERT_NO_EXCEPT("amomaxu.w on the upper 32-bit word of the second doubleword selects 0x80000000 across the unsigned sign boundary while preserving the first doubleword",
        old_value == 0x000000007fffffffULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x8000000013579bdfULL
    );

    TEST_END();
}

bool ai_arch_amoand_w_second_doubleword_upper_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xf0f0aa55cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoand_w_raw64(ai_arch_amo_addr(12), 0x0ff00f0fU);
    AI_ASSERT_NO_EXCEPT("amoand.w on the upper 32-bit word of the second doubleword sign-extends rd correctly while preserving the first doubleword",
        old_value == 0xfffffffff0f0aa55ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x00f00a05cafebabeULL
    );

    TEST_END();
}

bool ai_arch_amomin_w_second_doubleword_upper_word_signed_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x80000000a55a33ccULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomin_w_raw64(ai_arch_amo_addr(12), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomin.w on the upper 32-bit word of the second doubleword keeps INT32_MIN at the signed boundary while preserving the first doubleword",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x80000000a55a33ccULL
    );

    TEST_END();
}

bool ai_arch_amominu_w_second_doubleword_upper_word_zero_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xffffffff2468ace0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amominu_w_raw64(ai_arch_amo_addr(12), 0x00000000U);
    AI_ASSERT_NO_EXCEPT("amominu.w on the upper 32-bit word of the second doubleword selects zero at the unsigned boundary while preserving the first doubleword",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x000000002468ace0ULL
    );

    TEST_END();
}

bool ai_arch_amomax_w_second_doubleword_upper_word_signed_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x80000000cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amomax_w_raw64(ai_arch_amo_addr(12), 0x7fffffffU);
    AI_ASSERT_NO_EXCEPT("amomax.w on the upper 32-bit word of the second doubleword selects INT32_MAX across the signed boundary while preserving the first doubleword",
        old_value == 0xffffffff80000000ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x7fffffffcafebabeULL
    );

    TEST_END();
}

bool ai_arch_amoor_w_second_doubleword_upper_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0badc0de55aa33ccULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x12003400cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoor_w_raw64(ai_arch_amo_addr(12), 0x00f0004fU);
    AI_ASSERT_NO_EXCEPT("amoor.w on the upper 32-bit word of the second doubleword updates only the target word and preserves the first doubleword",
        old_value == 0x0000000012003400ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x12f0344fcafebabeULL
    );

    TEST_END();
}

bool ai_arch_amoxor_w_second_doubleword_upper_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xaa55cc33cafebabeULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_amoxor_w_raw64(ai_arch_amo_addr(12), 0x0f0f00ffU);
    AI_ASSERT_NO_EXCEPT("amoxor.w on the upper 32-bit word of the second doubleword sign-extends rd correctly while preserving the first doubleword",
        old_value == 0xffffffffaa55cc33ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0xa55acccccafebabeULL
    );

    TEST_END();
}

bool ai_arch_amoswap_d_second_doubleword_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoswap_d(ai_arch_amo_addr(8), 0xa1b2c3d4e5f60718ULL);
    AI_ASSERT_NO_EXCEPT("amoswap.d on the second doubleword returns the old value, updates only that 64-bit lane, and preserves the first doubleword",
        old_value == 0x1122334455667788ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0xa1b2c3d4e5f60718ULL
    );

    TEST_END();
}

bool ai_arch_amoadd_d_second_doubleword_wraparound_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xffffffffffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(ai_arch_amo_addr(8), 0x2ULL);
    AI_ASSERT_NO_EXCEPT("amoadd.d on the second doubleword wraps modulo 2^64 while preserving the first doubleword outside the target lane",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x0000000000000001ULL
    );

    TEST_END();
}

bool ai_arch_amomaxu_d_second_doubleword_sign_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x7fffffffffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomaxu_d(ai_arch_amo_addr(8), 0x8000000000000000ULL);
    AI_ASSERT_NO_EXCEPT("amomaxu.d on the second doubleword selects the larger unsigned operand across the sign boundary while preserving the first doubleword",
        old_value == 0x7fffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x8000000000000000ULL
    );

    TEST_END();
}

bool ai_arch_amoand_d_second_doubleword_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0badc0de55aa33ccULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xff00ff00ff00ff00ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoand_d(ai_arch_amo_addr(8), 0x0f0f0f0f0f0f0f0fULL);
    AI_ASSERT_NO_EXCEPT("amoand.d on the second doubleword updates only that 64-bit lane and preserves the first doubleword outside the target lane",
        old_value == 0xff00ff00ff00ff00ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x0f000f000f000f00ULL
    );

    TEST_END();
}

bool ai_arch_amomin_d_second_doubleword_signed_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x8000000000000000ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomin_d(ai_arch_amo_addr(8), 0x7fffffffffffffffULL);
    AI_ASSERT_NO_EXCEPT("amomin.d on the second doubleword keeps INT64_MIN at the signed boundary while preserving the first doubleword",
        old_value == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x8000000000000000ULL
    );

    TEST_END();
}

bool ai_arch_amominu_d_second_doubleword_zero_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xffffffffffffffffULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amominu_d(ai_arch_amo_addr(8), 0x0ULL);
    AI_ASSERT_NO_EXCEPT("amominu.d on the second doubleword selects zero at the unsigned boundary while preserving the first doubleword",
        old_value == 0xffffffffffffffffULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x0000000000000000ULL
    );

    TEST_END();
}

bool ai_arch_amomax_d_second_doubleword_signed_boundary_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x8000000000000000ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomax_d(ai_arch_amo_addr(8), 0x7fffffffffffffffULL);
    AI_ASSERT_NO_EXCEPT("amomax.d on the second doubleword selects INT64_MAX across the signed boundary while preserving the first doubleword",
        old_value == 0x8000000000000000ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x7fffffffffffffffULL
    );

    TEST_END();
}

bool ai_arch_amoor_d_second_doubleword_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0badc0de55aa33ccULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0x1200340000003400ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoor_d(ai_arch_amo_addr(8), 0x00f0004f0000004fULL);
    AI_ASSERT_NO_EXCEPT("amoor.d on the second doubleword updates only that 64-bit lane and preserves the first doubleword outside the target lane",
        old_value == 0x1200340000003400ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x12f0344f0000344fULL
    );

    TEST_END();
}

bool ai_arch_amoxor_d_second_doubleword_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    ai_arch_amo_write64_at(0, first_value);
    ai_arch_amo_write64_at(8, 0xaaaaaaaa55555555ULL);

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoxor_d(ai_arch_amo_addr(8), 0xffffffff0000ffffULL);
    AI_ASSERT_NO_EXCEPT("amoxor.d on the second doubleword updates only that 64-bit lane and preserves the first doubleword outside the target lane",
        old_value == 0xaaaaaaaa55555555ULL &&
        ai_arch_amo_read64_at(0) == first_value &&
        ai_arch_amo_read64_at(8) == 0x555555555555aaaaULL
    );

    TEST_END();
}
