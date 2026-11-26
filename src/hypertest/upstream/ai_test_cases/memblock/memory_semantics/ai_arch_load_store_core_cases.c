#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

static uint8_t ai_arch_mem[16] __attribute__((aligned(16)));
static uint8_t ai_arch_cacheline_mem[128] __attribute__((aligned(64)));

#define AI_LOAD_RAW64(name, instruction)                                            \
    static inline uint64_t name(uintptr_t addr)                                     \
    {                                                                                \
        uint64_t value;                                                              \
        asm volatile(                                                                \
            ".option push\n\t"                                                    \
            ".option norvc\n\t"                                                   \
            instruction " %0, 0(%1)\n\t"                                          \
            ".option pop\n\t"                                                     \
            : "=r"(value) : "r"(addr) : "memory");                              \
        return value;                                                                \
    }

AI_LOAD_RAW64(ai_lb_raw64, "lb");
AI_LOAD_RAW64(ai_lbu_raw64, "lbu");
AI_LOAD_RAW64(ai_lh_raw64, "lh");
AI_LOAD_RAW64(ai_lhu_raw64, "lhu");
AI_LOAD_RAW64(ai_lw_raw64, "lw");
AI_LOAD_RAW64(ai_lwu_raw64, "lwu");
AI_LOAD_RAW64(ai_ld_raw64, "ld");

static inline uint64_t ai_arch_ld_rd_probe(uintptr_t addr, uint64_t sentinel)
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

static inline uintptr_t ai_arch_addr(size_t offset)
{
    return (uintptr_t)(ai_arch_mem + offset);
}

static inline void ai_arch_write64(uint64_t value)
{
    *(volatile uint64_t *)ai_arch_mem = value;
}

static inline uint64_t ai_arch_read64(void)
{
    return *(volatile uint64_t *)ai_arch_mem;
}

static inline void ai_arch_write64_at(size_t offset, uint64_t value)
{
    *(volatile uint64_t *)(ai_arch_mem + offset) = value;
}

static inline uint64_t ai_arch_read64_at(size_t offset)
{
    return *(volatile uint64_t *)(ai_arch_mem + offset);
}

static inline pte_t *ai_arch_load_store_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_load_store_set_hs_leaf(
    enum test_page page, uint64_t flags)
{
    *ai_arch_load_store_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline uintptr_t ai_arch_load_store_cross_page_addr(
    enum test_page low_page, uintptr_t low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - low_bytes;
}

static inline uintptr_t ai_arch_load_store_page_tail_phys(
    enum test_page page)
{
    return phys_page_base(page) + PAGE_SIZE - sizeof(uint64_t);
}

#define AI_LOAD_IMM120_RAW64(name, instruction)                                    \
    static inline uint64_t name(uintptr_t base)                                    \
    {                                                                              \
        uint64_t value;                                                            \
        asm volatile(                                                              \
            ".option push\n\t"                                                     \
            ".option norvc\n\t"                                                    \
            instruction " %0, 0x120(%1)\n\t"                                       \
            ".option pop\n\t"                                                      \
            : "=r"(value) : "r"(base) : "memory");                              \
        return value;                                                              \
    }

AI_LOAD_IMM120_RAW64(ai_arch_lb_imm120_raw64, "lb");
AI_LOAD_IMM120_RAW64(ai_arch_lbu_imm120_raw64, "lbu");
AI_LOAD_IMM120_RAW64(ai_arch_lh_imm120_raw64, "lh");
AI_LOAD_IMM120_RAW64(ai_arch_lhu_imm120_raw64, "lhu");
AI_LOAD_IMM120_RAW64(ai_arch_lw_imm120_raw64, "lw");
AI_LOAD_IMM120_RAW64(ai_arch_lwu_imm120_raw64, "lwu");
AI_LOAD_IMM120_RAW64(ai_arch_ld_imm120_raw64, "ld");

static inline void ai_arch_ld_x0_imm120(uintptr_t base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld x0, 0x120(%0)\n\t"
        ".option pop\n\t"
        :: "r"(base) : "memory");
}

static inline void ai_arch_ld_x0_zero(uintptr_t base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld x0, 0(%0)\n\t"
        ".option pop\n\t"
        :: "r"(base) : "memory");
}

static inline uint64_t ai_arch_read_x0_value(void)
{
    uint64_t value;
    asm volatile("mv %0, x0" : "=r"(value));
    return value;
}

#define AI_STORE_RAW64(name, instruction)                                          \
    static inline void name(uintptr_t addr, uint64_t value)                        \
    {                                                                              \
        asm volatile(                                                              \
            ".option push\n\t"                                                     \
            ".option norvc\n\t"                                                    \
            instruction " %0, 0(%1)\n\t"                                           \
            ".option pop\n\t"                                                      \
            :: "r"(value), "r"(addr) : "memory");                               \
    }

AI_STORE_RAW64(ai_arch_sb_from_raw64, "sb");
AI_STORE_RAW64(ai_arch_sh_from_raw64, "sh");
AI_STORE_RAW64(ai_arch_sw_from_raw64, "sw");
AI_STORE_RAW64(ai_arch_sd_from_raw64, "sd");

#define AI_STORE_X0(name, instruction)                                             \
    static inline void name(uintptr_t addr)                                        \
    {                                                                              \
        asm volatile(                                                              \
            ".option push\n\t"                                                     \
            ".option norvc\n\t"                                                    \
            instruction " x0, 0(%0)\n\t"                                           \
            ".option pop\n\t"                                                      \
            :: "r"(addr) : "memory");                                            \
    }

AI_STORE_X0(ai_arch_sb_x0, "sb");
AI_STORE_X0(ai_arch_sh_x0, "sh");
AI_STORE_X0(ai_arch_sw_x0, "sw");
AI_STORE_X0(ai_arch_sd_x0, "sd");

static inline void ai_arch_sh_x5_zero(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li x5, 0\n\t"
        "sh x5, 0(%0)\n\t"
        ".option pop\n\t"
        :: "r"(addr) : "x5", "memory");
}

static inline void ai_arch_enable_fp_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_INITIAL);
    CSRC(CSR_MSTATUS, 1ULL << 14);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
}

static inline uint64_t ai_arch_fld_raw_bits(uintptr_t addr)
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

static inline void ai_arch_fsd_raw_bits(uintptr_t addr, uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft0, %1\n\t"
        "fsd ft0, 0(%0)\n\t"
        ".option pop\n\t"
        :: "r"(addr), "r"(bits) : "ft0", "memory");
}

static inline uint64_t ai_arch_vector_vle64_element0_raw(uintptr_t load_addr)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m1, ta, ma\n\t"
        "vle64.v v6, (%[load_addr])\n\t"
        "vmv.x.s %[value], v6\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [load_addr] "r"(load_addr),
          [vl] "r"(1)
        : "t0", "memory");
    return value;
}

static inline void ai_arch_vector_vle64_vse64_once(uintptr_t load_addr,
    uintptr_t store_addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m1, ta, ma\n\t"
        "vle64.v v6, (%[load_addr])\n\t"
        "vse64.v v6, (%[store_addr])\n\t"
        ".option pop\n\t"
        :
        : [load_addr] "r"(load_addr),
          [store_addr] "r"(store_addr),
          [vl] "r"(1)
        : "t0", "memory");
}

bool ai_arch_lb_sign_extension()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_mem[0] = 0x80;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lb_raw64(ai_arch_addr(0));
    TEST_ASSERT("lb sign-extends 0x80 into XLEN",
        excpt.triggered == false &&
        value == 0xffffffffffffff80ULL
    );

    TEST_END();
}

bool ai_arch_lbu_zero_extension()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_mem[0] = 0x80;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lbu_raw64(ai_arch_addr(0));
    TEST_ASSERT("lbu zero-extends 0x80 into XLEN",
        excpt.triggered == false &&
        value == 0x80ULL
    );

    TEST_END();
}

bool ai_arch_lh_sign_extension()
{
    TEST_START();

    goto_priv(PRIV_M);
    *(volatile uint16_t *)ai_arch_mem = 0x8001U;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lh_raw64(ai_arch_addr(0));
    TEST_ASSERT("lh sign-extends 0x8001 into XLEN",
        excpt.triggered == false &&
        value == 0xffffffffffff8001ULL
    );

    TEST_END();
}

bool ai_arch_lhu_zero_extension()
{
    TEST_START();

    goto_priv(PRIV_M);
    *(volatile uint16_t *)ai_arch_mem = 0x8001U;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lhu_raw64(ai_arch_addr(0));
    TEST_ASSERT("lhu zero-extends 0x8001 into XLEN",
        excpt.triggered == false &&
        value == 0x8001ULL
    );

    TEST_END();
}

bool ai_arch_lw_sign_extension()
{
    TEST_START();

    goto_priv(PRIV_M);
    *(volatile uint32_t *)ai_arch_mem = 0x80000001U;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lw_raw64(ai_arch_addr(0));
    TEST_ASSERT("lw sign-extends 0x80000001 into XLEN",
        excpt.triggered == false &&
        value == 0xffffffff80000001ULL
    );

    TEST_END();
}

bool ai_arch_lwu_zero_extension()
{
    TEST_START();

    goto_priv(PRIV_M);
    *(volatile uint32_t *)ai_arch_mem = 0x80000001U;

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lwu_raw64(ai_arch_addr(0));
    TEST_ASSERT("lwu zero-extends 0x80000001 into XLEN",
        excpt.triggered == false &&
        value == 0x0000000080000001ULL
    );

    TEST_END();
}

bool ai_arch_ld_full_width_read()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x8877665544332211ULL);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_ld_raw64(ai_arch_addr(0));
    TEST_ASSERT("ld reads a full 64-bit payload without corruption",
        excpt.triggered == false &&
        value == 0x8877665544332211ULL
    );

    TEST_END();
}

bool ai_arch_load_x0_ea_and_fault_metadata_corner()
{
    TEST_START();

    const enum test_page legal_page = VSRW_GRW;
    const enum test_page fault_page = VSX_GX;
    const uintptr_t imm12_offset = 0x120U;
    const uintptr_t byte_vaddr = hs_page_base(legal_page) + 0x120U;
    const uintptr_t half_vaddr = hs_page_base(legal_page) + 0x130U;
    const uintptr_t word_vaddr = hs_page_base(legal_page) + 0x140U;
    const uintptr_t double_vaddr = hs_page_base(legal_page) + 0x150U;
    const uintptr_t byte_base = byte_vaddr - imm12_offset;
    const uintptr_t half_base = half_vaddr - imm12_offset;
    const uintptr_t word_base = word_vaddr - imm12_offset;
    const uintptr_t double_base = double_vaddr - imm12_offset;
    const uintptr_t byte_paddr = phys_page_base(legal_page) + 0x120U;
    const uintptr_t half_paddr = phys_page_base(legal_page) + 0x130U;
    const uintptr_t word_paddr = phys_page_base(legal_page) + 0x140U;
    const uintptr_t double_paddr = phys_page_base(legal_page) + 0x150U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + imm12_offset;
    const uintptr_t fault_base = fault_vaddr - imm12_offset;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + imm12_offset;
    const uint64_t legal_value = 0x6a5b4c3d2e1f9081ULL;
    const uint64_t fault_seed = 0xcafef00d13579bdfULL;
    uint64_t saved_medeleg;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    saved_medeleg = CSRR(CSR_MEDELEG);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_LPF);
    write8(byte_paddr, 0x7fU);
    write8(byte_paddr + 1U, 0x80U);
    write8(byte_paddr + 2U, 0xffU);
    write16(half_paddr, 0x8001U);
    write16(half_paddr + 2U, 0xffffU);
    write32(word_paddr, 0x80000001U);
    write32(word_paddr + 4U, 0xffffffffU);
    write64(double_paddr, legal_value);
    write64(fault_paddr, fault_seed);
    fence_iorw_iorw();
    hspt_init();
    ai_arch_load_store_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    uint64_t lb_pos_value = ai_arch_lb_imm120_raw64(byte_base);
    uint64_t lb_neg_value = ai_arch_lb_imm120_raw64(byte_base + 1U);
    uint64_t lbu_value = ai_arch_lbu_imm120_raw64(byte_base);
    uint64_t lbu_ones_value = ai_arch_lbu_imm120_raw64(byte_base + 2U);
    uint64_t lh_value = ai_arch_lh_imm120_raw64(half_base);
    uint64_t lhu_value = ai_arch_lhu_imm120_raw64(half_base);
    uint64_t lhu_ones_value = ai_arch_lhu_imm120_raw64(half_base + 2U);
    uint64_t lw_value = ai_arch_lw_imm120_raw64(word_base);
    uint64_t lwu_value = ai_arch_lwu_imm120_raw64(word_base);
    uint64_t lwu_ones_value = ai_arch_lwu_imm120_raw64(word_base + 4U);
    uint64_t ld_value = ai_arch_ld_imm120_raw64(double_base);
    TEST_ASSERT("lb/lbu/lh/lhu/lw/lwu/ld with a nonzero signed imm12 use rs1+signext(imm12) and apply boundary XLEN extension rules",
        excpt.triggered == false &&
        lb_pos_value == 0x7fULL &&
        lb_neg_value == 0xffffffffffffff80ULL &&
        lbu_value == 0x7fULL &&
        lbu_ones_value == 0xffULL &&
        lh_value == 0xffffffffffff8001ULL &&
        lhu_value == 0x8001ULL &&
        lhu_ones_value == 0xffffULL &&
        lw_value == 0xffffffff80000001ULL &&
        lwu_value == 0x0000000080000001ULL &&
        lwu_ones_value == 0x00000000ffffffffULL &&
        ld_value == legal_value
    );

    TEST_SETUP_EXCEPT();
    ai_arch_ld_x0_imm120(double_base);
    TEST_ASSERT("ld x0 on a legal readable leaf completes without writeback and keeps x0 hardwired to zero",
        excpt.triggered == false &&
        ai_arch_read_x0_value() == 0 &&
        ld(double_vaddr) == legal_value
    );

    TEST_SETUP_EXCEPT();
    ai_arch_ld_x0_zero(double_vaddr);
    TEST_ASSERT("ld x0, 0(a0) is legal, still performs the readable memory access, and leaves x0 hardwired to zero",
        excpt.triggered == false &&
        ai_arch_read_x0_value() == 0 &&
        ld(double_vaddr) == legal_value
    );

    TEST_SETUP_EXCEPT();
    ai_arch_ld_x0_imm120(fault_base);
    TEST_ASSERT("ld x0 still performs the access: a no-read leaf reports load page fault metadata for the same effective address",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        ai_arch_read_x0_value() == 0 &&
        read64(fault_paddr) == fault_seed
    );

    goto_priv(PRIV_M);
    ai_arch_load_store_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    CSRW(CSR_MEDELEG, saved_medeleg);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovered = ai_arch_ld_imm120_raw64(fault_base);
    TEST_ASSERT("after restoring read permission, the same rs1+imm12 effective address loads the seeded value",
        excpt.triggered == false &&
        recovered == fault_seed
    );

    TEST_END();
}

bool ai_arch_ld_second_doubleword_full_width_corner()
{
    TEST_START();

    const uint64_t first_value = 0x1122334455667788ULL;
    const uint64_t second_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_ld_raw64(ai_arch_addr(8));
    TEST_ASSERT("ld reads the second doubleword exactly without corrupting the neighboring first doubleword",
        excpt.triggered == false &&
        value == second_value &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lb_second_doubleword_sign_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;
    const uint64_t second_value = 0x1122334455667780ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lb_raw64(ai_arch_addr(8));
    TEST_ASSERT("lb sign-extends the low byte of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0xffffffffffffff80ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lbu_second_doubleword_zero_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;
    const uint64_t second_value = 0x1122334455667780ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lbu_raw64(ai_arch_addr(8));
    TEST_ASSERT("lbu zero-extends the low byte of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0x80ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lh_second_doubleword_sign_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x55aa33cc77ee11ffULL;
    const uint64_t second_value = 0x1122334455668001ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lh_raw64(ai_arch_addr(8));
    TEST_ASSERT("lh sign-extends the low halfword of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0xffffffffffff8001ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lhu_second_doubleword_zero_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0badc0de55aa33ccULL;
    const uint64_t second_value = 0x1122334455668001ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lhu_raw64(ai_arch_addr(8));
    TEST_ASSERT("lhu zero-extends the low halfword of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0x8001ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lw_second_doubleword_sign_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x8899aabbccddeeffULL;
    const uint64_t second_value = 0x5566778880000001ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lw_raw64(ai_arch_addr(8));
    TEST_ASSERT("lw sign-extends the low 32-bit word of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0xffffffff80000001ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lwu_second_doubleword_zero_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;
    const uint64_t second_value = 0x5566778880000001ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lwu_raw64(ai_arch_addr(8));
    TEST_ASSERT("lwu zero-extends the low 32-bit word of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0x0000000080000001ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lb_second_doubleword_tail_byte_sign_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;
    const uint64_t second_value = 0x8011223344556677ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lb_raw64(ai_arch_addr(15));
    TEST_ASSERT("lb sign-extends the tail byte of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0xffffffffffffff80ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lbu_second_doubleword_tail_byte_zero_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;
    const uint64_t second_value = 0x8011223344556677ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lbu_raw64(ai_arch_addr(15));
    TEST_ASSERT("lbu zero-extends the tail byte of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0x80ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lh_second_doubleword_upper_halfword_sign_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x55aa33cc77ee11ffULL;
    const uint64_t second_value = 0x8001223344556677ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lh_raw64(ai_arch_addr(14));
    TEST_ASSERT("lh sign-extends the upper halfword of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0xffffffffffff8001ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lhu_second_doubleword_upper_halfword_zero_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0badc0de55aa33ccULL;
    const uint64_t second_value = 0x8001223344556677ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lhu_raw64(ai_arch_addr(14));
    TEST_ASSERT("lhu zero-extends the upper halfword of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0x8001ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lw_second_doubleword_upper_word_sign_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x8899aabbccddeeffULL;
    const uint64_t second_value = 0x8000000144556677ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lw_raw64(ai_arch_addr(12));
    TEST_ASSERT("lw sign-extends the upper 32-bit word of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0xffffffff80000001ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_lwu_second_doubleword_upper_word_zero_extension_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;
    const uint64_t second_value = 0x8000000144556677ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lwu_raw64(ai_arch_addr(12));
    TEST_ASSERT("lwu zero-extends the upper 32-bit word of the second doubleword correctly while preserving both doublewords in memory",
        excpt.triggered == false &&
        value == 0x0000000080000001ULL &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_sb_byte_writeback()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    sb(ai_arch_addr(1), 0xaaU);
    TEST_ASSERT("sb updates only the targeted byte",
        excpt.triggered == false &&
        ai_arch_read64() == 0x112233445566aa88ULL
    );

    TEST_END();
}

bool ai_arch_sh_halfword_writeback()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    sh(ai_arch_addr(2), 0xbeefU);
    TEST_ASSERT("sh updates only the targeted halfword",
        excpt.triggered == false &&
        ai_arch_read64() == 0x11223344beef7788ULL
    );

    TEST_END();
}

bool ai_arch_sw_word_writeback()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    sw(ai_arch_addr(4), 0xa1b2c3d4U);
    TEST_ASSERT("sw updates only the targeted word",
        excpt.triggered == false &&
        ai_arch_read64() == 0xa1b2c3d455667788ULL
    );

    TEST_END();
}

bool ai_arch_sd_doubleword_writeback()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    sd(ai_arch_addr(0), 0xdeadbeefcafef00dULL);
    TEST_ASSERT("sd overwrites the complete doubleword",
        excpt.triggered == false &&
        ai_arch_read64() == 0xdeadbeefcafef00dULL
    );

    TEST_END();
}

bool ai_arch_sb_low_byte_preserves_second_doubleword_corner()
{
    TEST_START();

    const uint64_t second_value = 0xdeadbeefcafebabeULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, 0x1122334455667788ULL);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    sb(ai_arch_addr(0), 0xaaU);
    TEST_ASSERT("sb on the low byte of the first doubleword updates only that byte and preserves the second doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0x11223344556677aaULL &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_sh_low_halfword_preserves_second_doubleword_corner()
{
    TEST_START();

    const uint64_t second_value = 0xdeadbeefcafebabeULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, 0x1122334455667788ULL);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    sh(ai_arch_addr(0), 0xbeefU);
    TEST_ASSERT("sh on the low halfword of the first doubleword updates only that halfword and preserves the second doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0x112233445566beefULL &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_sw_low_word_preserves_second_doubleword_corner()
{
    TEST_START();

    const uint64_t second_value = 0xdeadbeefcafebabeULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, 0x1122334455667788ULL);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    sw(ai_arch_addr(0), 0xa1b2c3d4U);
    TEST_ASSERT("sw on the low 32-bit word of the first doubleword updates only that word and preserves the second doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0x11223344a1b2c3d4ULL &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_sb_tail_byte_preserves_second_doubleword_corner()
{
    TEST_START();

    const uint64_t second_value = 0xdeadbeefcafebabeULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, 0x1122334455667788ULL);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    sb(ai_arch_addr(7), 0xaaU);
    TEST_ASSERT("sb on the tail byte of the first doubleword updates only that byte and preserves the second doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0xaa22334455667788ULL &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_sh_upper_halfword_preserves_second_doubleword_corner()
{
    TEST_START();

    const uint64_t second_value = 0xdeadbeefcafebabeULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, 0x1122334455667788ULL);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    sh(ai_arch_addr(6), 0xbeefU);
    TEST_ASSERT("sh on the uppermost halfword of the first doubleword updates only that halfword and preserves the second doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0xbeef334455667788ULL &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_sw_upper_word_preserves_second_doubleword_corner()
{
    TEST_START();

    const uint64_t second_value = 0xdeadbeefcafebabeULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, 0x1122334455667788ULL);
    ai_arch_write64_at(8, second_value);

    TEST_SETUP_EXCEPT();
    sw(ai_arch_addr(4), 0xa1b2c3d4U);
    TEST_ASSERT("sw on the upper 32-bit word of the first doubleword updates only that word and preserves the second doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0xa1b2c3d455667788ULL &&
        ai_arch_read64_at(8) == second_value
    );

    TEST_END();
}

bool ai_arch_sd_second_doubleword_writeback_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x1122334455667788ULL;
    const uint64_t second_initial = 0xaabbccddeeff0011ULL;
    const uint64_t second_written = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_initial);

    TEST_SETUP_EXCEPT();
    sd(ai_arch_addr(8), second_written);
    TEST_ASSERT("sd on the second doubleword updates only that 64-bit lane and preserves the first doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == second_written
    );

    TEST_END();
}

bool ai_arch_sb_second_doubleword_tail_byte_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;
    const uint64_t second_initial = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_initial);

    TEST_SETUP_EXCEPT();
    sb(ai_arch_addr(15), 0xaaU);
    TEST_ASSERT("sb on the tail byte of the second doubleword updates only that byte and preserves the first doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == 0xaa22334455667788ULL
    );

    TEST_END();
}

bool ai_arch_sh_second_doubleword_upper_halfword_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;
    const uint64_t second_initial = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_initial);

    TEST_SETUP_EXCEPT();
    sh(ai_arch_addr(14), 0xbeefU);
    TEST_ASSERT("sh on the uppermost halfword of the second doubleword updates only that halfword and preserves the first doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == 0xbeef334455667788ULL
    );

    TEST_END();
}

bool ai_arch_sw_second_doubleword_upper_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x8899aabbccddeeffULL;
    const uint64_t second_initial = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_initial);

    TEST_SETUP_EXCEPT();
    sw(ai_arch_addr(12), 0xa1b2c3d4U);
    TEST_ASSERT("sw on the upper 32-bit word of the second doubleword updates only that word and preserves the first doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == 0xa1b2c3d455667788ULL
    );

    TEST_END();
}

bool ai_arch_sb_second_doubleword_low_byte_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x0123456789abcdefULL;
    const uint64_t second_initial = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_initial);

    TEST_SETUP_EXCEPT();
    sb(ai_arch_addr(8), 0xaaU);
    TEST_ASSERT("sb on the low byte of the second doubleword updates only that byte and preserves the first doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == 0x11223344556677aaULL
    );

    TEST_END();
}

bool ai_arch_sh_second_doubleword_low_halfword_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0xfedcba9876543210ULL;
    const uint64_t second_initial = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_initial);

    TEST_SETUP_EXCEPT();
    sh(ai_arch_addr(8), 0xbeefU);
    TEST_ASSERT("sh on the low halfword of the second doubleword updates only that halfword and preserves the first doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == 0x112233445566beefULL
    );

    TEST_END();
}

bool ai_arch_sw_second_doubleword_low_word_preserves_first_doubleword_corner()
{
    TEST_START();

    const uint64_t first_value = 0x8899aabbccddeeffULL;
    const uint64_t second_initial = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, first_value);
    ai_arch_write64_at(8, second_initial);

    TEST_SETUP_EXCEPT();
    sw(ai_arch_addr(8), 0xa1b2c3d4U);
    TEST_ASSERT("sw on the low 32-bit word of the second doubleword updates only that word and preserves the first doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_value &&
        ai_arch_read64_at(8) == 0x11223344a1b2c3d4ULL
    );

    TEST_END();
}

bool ai_arch_lb_sign_extension_tail_byte_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x8011223344556677ULL);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lb_raw64(ai_arch_addr(7));
    TEST_ASSERT("lb sign-extends a negative tail byte correctly when reading the highest byte of the doubleword",
        excpt.triggered == false &&
        value == 0xffffffffffffff80ULL
    );

    TEST_END();
}

bool ai_arch_lbu_zero_extension_tail_byte_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x8011223344556677ULL);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lbu_raw64(ai_arch_addr(7));
    TEST_ASSERT("lbu zero-extends the highest byte correctly when reading the tail byte of the doubleword",
        excpt.triggered == false &&
        value == 0x80ULL
    );

    TEST_END();
}

bool ai_arch_lh_sign_extension_upper_halfword_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x8001223344556677ULL);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lh_raw64(ai_arch_addr(6));
    TEST_ASSERT("lh sign-extends the uppermost halfword correctly when reading the high end of the doubleword",
        excpt.triggered == false &&
        value == 0xffffffffffff8001ULL
    );

    TEST_END();
}

bool ai_arch_lhu_zero_extension_upper_halfword_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x8001223344556677ULL);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lhu_raw64(ai_arch_addr(6));
    TEST_ASSERT("lhu zero-extends the uppermost halfword correctly when reading the high end of the doubleword",
        excpt.triggered == false &&
        value == 0x8001ULL
    );

    TEST_END();
}

bool ai_arch_lw_sign_extension_upper_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x8000000144556677ULL);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lw_raw64(ai_arch_addr(4));
    TEST_ASSERT("lw sign-extends the upper 32-bit word correctly when reading the high word of the doubleword",
        excpt.triggered == false &&
        value == 0xffffffff80000001ULL
    );

    TEST_END();
}

bool ai_arch_lwu_zero_extension_upper_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x8000000144556677ULL);

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_lwu_raw64(ai_arch_addr(4));
    TEST_ASSERT("lwu zero-extends the upper 32-bit word correctly when reading the high word of the doubleword",
        excpt.triggered == false &&
        value == 0x0000000080000001ULL
    );

    TEST_END();
}

bool ai_arch_sb_tail_byte_writeback_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    sb(ai_arch_addr(7), 0xaaU);
    TEST_ASSERT("sb updates only the tail byte at the high end of the doubleword",
        excpt.triggered == false &&
        ai_arch_read64() == 0xaa22334455667788ULL
    );

    TEST_END();
}

bool ai_arch_sh_upper_halfword_writeback_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    sh(ai_arch_addr(6), 0xbeefU);
    TEST_ASSERT("sh updates only the uppermost halfword at the high end of the doubleword",
        excpt.triggered == false &&
        ai_arch_read64() == 0xbeef334455667788ULL
    );

    TEST_END();
}

bool ai_arch_sw_lower_word_writeback_preserves_upper_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    sw(ai_arch_addr(0), 0xa1b2c3d4U);
    TEST_ASSERT("sw on the lower 32-bit lane updates only the lower word and preserves the upper word",
        excpt.triggered == false &&
        ai_arch_read64() == 0x11223344a1b2c3d4ULL
    );

    TEST_END();
}

bool ai_arch_sw_upper_word_writeback_preserves_lower_word_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_arch_write64(0x1122334455667788ULL);

    TEST_SETUP_EXCEPT();
    sw(ai_arch_addr(4), 0xa1b2c3d4U);
    TEST_ASSERT("sw on the upper 32-bit lane updates only the upper word and preserves the lower word",
        excpt.triggered == false &&
        ai_arch_read64() == 0xa1b2c3d455667788ULL
    );

    TEST_END();
}

bool ai_arch_store_source_lane_and_order_visibility_corner()
{
    TEST_START();

    const uint64_t first_guard = 0x0123456789abcdefULL;
    const uint64_t second_initial = 0x99aabbccddeeff11ULL;
    const uint64_t sd_payload = 0x8877665544332211ULL;

    goto_priv(PRIV_M);

    ai_arch_write64_at(0, 0x1020304050607080ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(ai_arch_addr(0), sd_payload);
    uint64_t same_addr_ld = ai_ld_raw64(ai_arch_addr(0));
    TEST_ASSERT("an sd immediately followed by an ld at the same address exposes the store payload in program order",
        excpt.triggered == false &&
        same_addr_ld == sd_payload &&
        ai_arch_read64_at(0) == sd_payload
    );

    ai_arch_write64_at(0, 0x1122334455667788ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_sw_from_raw64(ai_arch_addr(0), 0x01020304deadbeefULL);
    ai_arch_sw_from_raw64(ai_arch_addr(0), 0x9988776621b2c3d4ULL);
    uint64_t younger_sw_value = ai_lw_raw64(ai_arch_addr(0));
    TEST_ASSERT("back-to-back sw at the same address makes the younger store visible to lw and fully overwrites the older word lane",
        excpt.triggered == false &&
        younger_sw_value == 0x21b2c3d4ULL &&
        ai_arch_read64_at(0) == 0x1122334421b2c3d4ULL
    );

    ai_arch_write64_at(0, first_guard);
    ai_arch_write64_at(8, second_initial);
    TEST_SETUP_EXCEPT();
    ai_arch_sb_x0(ai_arch_addr(8));
    TEST_ASSERT("sb x0 on the second doubleword writes one zero byte and preserves every guard byte",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_guard &&
        ai_arch_read64_at(8) == 0x99aabbccddeeff00ULL
    );

    ai_arch_write64_at(0, first_guard);
    ai_arch_write64_at(8, second_initial);
    TEST_SETUP_EXCEPT();
    ai_arch_sh_x0(ai_arch_addr(8));
    TEST_ASSERT("sh x0 on the second doubleword writes two zero bytes and preserves every guard byte",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_guard &&
        ai_arch_read64_at(8) == 0x99aabbccddee0000ULL
    );

    ai_arch_write64_at(0, first_guard);
    ai_arch_write64_at(8, second_initial);
    TEST_SETUP_EXCEPT();
    ai_arch_sw_x0(ai_arch_addr(8));
    TEST_ASSERT("sw x0 on the second doubleword writes four zero bytes and preserves every guard byte",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_guard &&
        ai_arch_read64_at(8) == 0x99aabbcc00000000ULL
    );

    ai_arch_write64_at(0, first_guard);
    ai_arch_write64_at(8, second_initial);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_x0(ai_arch_addr(8));
    TEST_ASSERT("sd x0 on the second doubleword writes the full zero doubleword and preserves the first doubleword guard",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_guard &&
        ai_arch_read64_at(8) == 0ULL
    );

    ai_arch_write64_at(0, 0x1122334455667788ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_sb_from_raw64(ai_arch_addr(3), 0x0102030405060708ULL);
    TEST_ASSERT("sb consumes only rs2[7:0] from a full XLEN source register and preserves non-target byte lanes",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0x1122334408667788ULL
    );

    ai_arch_write64_at(0, 0x1122334455667788ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_sh_from_raw64(ai_arch_addr(2), 0x010203040506beefULL);
    TEST_ASSERT("sh consumes only rs2[15:0] from a full XLEN source register and preserves non-target byte lanes",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0x11223344beef7788ULL
    );

    ai_arch_write64_at(0, 0x1122334455667788ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_sw_from_raw64(ai_arch_addr(4), 0x01020304a1b2c3d4ULL);
    TEST_ASSERT("sw consumes only rs2[31:0] from a full XLEN source register and preserves non-target byte lanes",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0xa1b2c3d455667788ULL
    );

    ai_arch_write64_at(0, first_guard);
    ai_arch_write64_at(8, second_initial);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(ai_arch_addr(8), 0x0102030405060708ULL);
    TEST_ASSERT("sd consumes the full XLEN source register and leaves the neighboring doubleword unchanged",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == first_guard &&
        ai_arch_read64_at(8) == 0x0102030405060708ULL
    );

    ai_arch_write64_at(0, 0xffffffffffffffffULL);
    TEST_SETUP_EXCEPT();
    for (size_t offset = 0; offset < 8U; offset++) {
        ai_arch_sb_from_raw64(ai_arch_addr(offset),
            0x0102030405060000ULL | (0xaaULL + offset));
    }
    TEST_ASSERT("eight adjacent sb operations produce the exact little-endian byte image with no lane swapped or skipped",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0xb1b0afaeadacabaaULL
    );

    ai_arch_write64_at(0, 0x1122334455667788ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_sh_x5_zero(ai_arch_addr(2));
    TEST_ASSERT("sh from a non-x0 source register holding zero is architecturally visible and clears the target halfword only",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == 0x1122334400007788ULL
    );

    TEST_END();
}

bool ai_arch_cross_page_scalar_stitch_and_store_byte_enable_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const uintptr_t low_tail_paddr =
        ai_arch_load_store_page_tail_phys(low_page);
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t half_vaddr =
        ai_arch_load_store_cross_page_addr(low_page, 1U);
    const uintptr_t word_vaddr =
        ai_arch_load_store_cross_page_addr(low_page, 2U);
    const uintptr_t double_vaddr =
        ai_arch_load_store_cross_page_addr(low_page, 4U);
    const uint64_t half_low_seed = 0x01a6a5a4a3a2a1a0ULL;
    const uint64_t half_high_seed = 0xb7b6b5b4b3b2b180ULL;
    const uint64_t word_low_seed = 0x0001c5c4c3c2c1c0ULL;
    const uint64_t word_high_seed = 0xd7d6d5d4d3d28000ULL;
    const uint64_t double_low_seed = 0x4433221113121110ULL;
    const uint64_t double_high_seed = 0x2726252488776655ULL;
    const uint64_t guard_low_seed = 0x1716151413121110ULL;
    const uint64_t guard_high_seed = 0x2726252423222120ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    hspt_init();
    ai_arch_load_store_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_arch_load_store_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    write64(low_tail_paddr, half_low_seed);
    write64(high_paddr, half_high_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t lh_cross = ai_lh_raw64(half_vaddr);
    uint64_t lhu_cross = ai_lhu_raw64(half_vaddr);
    TEST_ASSERT("legal cacheable cross-page lh/lhu stitch one byte from each page and apply signed versus zero extension",
        excpt.triggered == false &&
        lh_cross == 0xffffffffffff8001ULL &&
        lhu_cross == 0x8001ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page halfword loads do not alter either page-tail fragment or adjacent guard bytes",
        read64(low_tail_paddr) == half_low_seed &&
        read64(high_paddr) == half_high_seed
    );

    write64(low_tail_paddr, word_low_seed);
    write64(high_paddr, word_high_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t lw_cross = ai_lw_raw64(word_vaddr);
    uint64_t lwu_cross = ai_lwu_raw64(word_vaddr);
    TEST_ASSERT("legal cacheable cross-page lw/lwu stitch two bytes from each page and apply signed versus zero extension",
        excpt.triggered == false &&
        lw_cross == 0xffffffff80000001ULL &&
        lwu_cross == 0x0000000080000001ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page word loads preserve the seeded backing image on both pages",
        read64(low_tail_paddr) == word_low_seed &&
        read64(high_paddr) == word_high_seed
    );

    write64(low_tail_paddr, double_low_seed);
    write64(high_paddr, double_high_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t ld_cross = ai_ld_raw64(double_vaddr);
    TEST_ASSERT("legal cacheable cross-page ld returns the full little-endian doubleword stitched from both page fragments",
        excpt.triggered == false &&
        ld_cross == 0x8877665544332211ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page doubleword load preserves both page fragments around the boundary",
        read64(low_tail_paddr) == double_low_seed &&
        read64(high_paddr) == double_high_seed
    );

    write64(low_tail_paddr, guard_low_seed);
    write64(high_paddr, guard_high_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_sh_from_raw64(half_vaddr, 0xbeefULL);
    TEST_ASSERT("legal cacheable cross-page sh retires without exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page sh writes exactly one byte on each page and preserves every surrounding byte lane",
        read64(low_tail_paddr) == 0xef16151413121110ULL &&
        read64(high_paddr) == 0x27262524232221beULL
    );

    write64(low_tail_paddr, guard_low_seed);
    write64(high_paddr, guard_high_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_sw_from_raw64(word_vaddr, 0xa1b2c3d4ULL);
    TEST_ASSERT("legal cacheable cross-page sw retires without exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page sw writes exactly two bytes on each page and preserves pre/post guard byte lanes",
        read64(low_tail_paddr) == 0xc3d4151413121110ULL &&
        read64(high_paddr) == 0x272625242322a1b2ULL
    );

    write64(low_tail_paddr, guard_low_seed);
    write64(high_paddr, guard_high_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(double_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("legal cacheable cross-page sd retires without exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page sd writes four bytes on each page, keeps both outer guards, and exposes one architectural final image",
        read64(low_tail_paddr) == 0x4433221113121110ULL &&
        read64(high_paddr) == 0x2726252488776655ULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_cacheline_cross_scalar_lw_sw_visible_boundary_corner()
{
    TEST_START();

    const size_t cross_offset = 62U;
    const uintptr_t cross_addr =
        (uintptr_t)(ai_arch_cacheline_mem + cross_offset);
    const uint8_t load_image[16] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x01, 0x02, 0x00, 0x80,
        0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    const uint8_t store_image[16] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
        0x66, 0x77, 0x88, 0x99,
        0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_store_image[16] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
        0xd4, 0xc3, 0xb2, 0xa1,
        0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f
    };
    bool load_bytes_unchanged = true;
    bool store_bytes_match = true;

    goto_priv(PRIV_M);
    for (size_t i = 0U; i < 16U; i++) {
        ai_arch_cacheline_mem[56U + i] = load_image[i];
    }
    fence_iorw_iorw();

    TEST_SETUP_EXCEPT();
    uint64_t lw_cross = ai_lw_raw64(cross_addr);
    uint64_t lwu_cross = ai_lwu_raw64(cross_addr);
    TEST_ASSERT("cacheable lw/lwu crossing a 64-byte boundary stitch two bytes from each line with signed versus zero extension",
        excpt.triggered == false &&
        lw_cross == 0xffffffff80000201ULL &&
        lwu_cross == 0x0000000080000201ULL
    );

    for (size_t i = 0U; i < 16U; i++) {
        if (ai_arch_cacheline_mem[56U + i] != load_image[i]) {
            load_bytes_unchanged = false;
        }
    }
    TEST_ASSERT("cache-line boundary scalar loads leave the full surrounding byte image unchanged",
        load_bytes_unchanged
    );

    for (size_t i = 0U; i < 16U; i++) {
        ai_arch_cacheline_mem[56U + i] = store_image[i];
    }
    fence_iorw_iorw();

    TEST_SETUP_EXCEPT();
    ai_arch_sw_from_raw64(cross_addr, 0xa1b2c3d4ULL);
    TEST_ASSERT("cacheable sw crossing a 64-byte boundary retires without an architectural exception",
        excpt.triggered == false
    );

    for (size_t i = 0U; i < 16U; i++) {
        if (ai_arch_cacheline_mem[56U + i] != expected_store_image[i]) {
            store_bytes_match = false;
        }
    }
    TEST_ASSERT("cross-line sw writes exactly two bytes on each line and preserves all pre/post guard byte lanes",
        store_bytes_match
    );

    TEST_SETUP_EXCEPT();
    uint64_t repaired_lwu = ai_lwu_raw64(cross_addr);
    TEST_ASSERT("the final cross-line sw image is immediately visible through an architectural lwu from the same address",
        excpt.triggered == false &&
        repaired_lwu == 0x00000000a1b2c3d4ULL
    );

    TEST_END();
}

bool ai_arch_cross_page_scalar_load_pf_metadata_rd_suppression_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const uintptr_t split_vaddr =
        ai_arch_load_store_cross_page_addr(low_page, sizeof(uint32_t));
    const uintptr_t second_half_vaddr = split_vaddr + sizeof(uint32_t);
    const uintptr_t low_guard_paddr =
        phys_page_base(low_page) + PAGE_SIZE - (2U * sizeof(uint64_t));
    const uintptr_t low_paddr =
        ai_arch_load_store_page_tail_phys(low_page);
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t high_guard_paddr = high_paddr + sizeof(uint64_t);
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t no_read_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t low_guard = 0x0badf00d11223344ULL;
    const uint64_t high_guard = 0x55667788cafebabeULL;
    const uint64_t load_low_seed = 0x1021324354657687ULL;
    const uint64_t load_high_seed = 0x98a9bacbdcedfe0fULL;
    const uint64_t load_sentinel_second = 0x5a6b7c8d9eafb001ULL;
    const uint64_t load_sentinel_first = 0x8877665544332211ULL;
    const uint64_t expected_repaired_load =
        ((load_high_seed & 0xffffffffULL) << 32) |
        (load_low_seed >> 32);
    uint64_t saved_medeleg;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    saved_medeleg = CSRR(CSR_MEDELEG);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_LPF);
    hspt_init();
    ai_arch_load_store_set_hs_leaf(low_page, legal_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, no_read_leaf);
    write64(low_guard_paddr, low_guard);
    write64(low_paddr, load_low_seed);
    write64(high_paddr, load_high_seed);
    write64(high_guard_paddr, high_guard);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t second_rd = ai_arch_ld_rd_probe(split_vaddr,
        load_sentinel_second);
    TEST_ASSERT("cross-page scalar ld with a legal low half and no-read high half reports the second-half load page fault VA and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr &&
        second_rd == load_sentinel_second
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("second-half load page fault preserves both page fragments and adjacent guards",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == load_low_seed &&
        read64(high_paddr) == load_high_seed &&
        read64(high_guard_paddr) == high_guard
    );

    ai_arch_load_store_set_hs_leaf(low_page, no_read_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, legal_leaf);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t first_rd = ai_arch_ld_rd_probe(split_vaddr,
        load_sentinel_first);
    TEST_ASSERT("cross-page scalar ld with a no-read low half reports the original first-half load page fault VA and does not consume high-half metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == split_vaddr &&
        first_rd == load_sentinel_first
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("first-half load page fault also leaves the boundary image unchanged",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == load_low_seed &&
        read64(high_paddr) == load_high_seed &&
        read64(high_guard_paddr) == high_guard
    );

    ai_arch_load_store_set_hs_leaf(low_page, legal_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, legal_leaf);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t repaired_load = ai_ld_raw64(split_vaddr);
    TEST_ASSERT("after restoring both read permissions, the same split ld completes and returns the stitched 4B+4B value",
        excpt.triggered == false &&
        repaired_load == expected_repaired_load
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MEDELEG, saved_medeleg);
    sfence_vma();

    TEST_END();
}

bool ai_arch_cross_page_scalar_store_low_pf_no_partial_write_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const uintptr_t split_vaddr =
        ai_arch_load_store_cross_page_addr(low_page, sizeof(uint32_t));
    const uintptr_t low_guard_paddr =
        phys_page_base(low_page) + PAGE_SIZE - (2U * sizeof(uint64_t));
    const uintptr_t low_paddr =
        ai_arch_load_store_page_tail_phys(low_page);
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t high_guard_paddr = high_paddr + sizeof(uint64_t);
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t no_write_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t low_guard = 0x0badf00d11223344ULL;
    const uint64_t high_guard = 0x55667788cafebabeULL;
    const uint64_t store_low_seed = 0x21436587a9cbed0fULL;
    const uint64_t store_high_seed = 0xfedcba9876543210ULL;
    const uint64_t store_fault_payload = 0x0123456789abcdefULL;
    const uint64_t store_repair_payload = 0xf0e1d2c3b4a59687ULL;
    const uint64_t expected_repaired_store_low =
        (store_low_seed & 0xffffffffULL) |
        ((store_repair_payload & 0xffffffffULL) << 32);
    const uint64_t expected_repaired_store_high =
        (store_high_seed & 0xffffffff00000000ULL) |
        (store_repair_payload >> 32);
    uint64_t saved_medeleg;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    saved_medeleg = CSRR(CSR_MEDELEG);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_SPF);
    hspt_init();
    ai_arch_load_store_set_hs_leaf(low_page, no_write_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, legal_leaf);
    write64(low_guard_paddr, low_guard);
    write64(low_paddr, store_low_seed);
    write64(high_paddr, store_high_seed);
    write64(high_guard_paddr, high_guard);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(split_vaddr, store_fault_payload);
    TEST_ASSERT("cross-page scalar sd with a no-write low half reports the original first-half store page fault VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == split_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("first-half store page fault leaves the whole split target and both guards unchanged",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == store_low_seed &&
        read64(high_paddr) == store_high_seed &&
        read64(high_guard_paddr) == high_guard
    );

    ai_arch_load_store_set_hs_leaf(low_page, legal_leaf);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(split_vaddr, store_repair_payload);
    TEST_ASSERT("after restoring the low write permission, the same split sd retires without stale first-half page-fault metadata",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired low-half split sd writes exactly four bytes on each page and still preserves adjacent guards",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == expected_repaired_store_low &&
        read64(high_paddr) == expected_repaired_store_high &&
        read64(high_guard_paddr) == high_guard
    );

    ai_arch_load_store_set_hs_leaf(low_page, legal_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, legal_leaf);
    CSRW(CSR_MEDELEG, saved_medeleg);
    sfence_vma();

    TEST_END();
}

bool ai_arch_cross_page_scalar_pf_metadata_no_partial_write_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const uintptr_t split_vaddr =
        ai_arch_load_store_cross_page_addr(low_page, sizeof(uint32_t));
    const uintptr_t second_half_vaddr = split_vaddr + sizeof(uint32_t);
    const uintptr_t low_guard_paddr =
        phys_page_base(low_page) + PAGE_SIZE - (2U * sizeof(uint64_t));
    const uintptr_t low_paddr =
        ai_arch_load_store_page_tail_phys(low_page);
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t high_guard_paddr = high_paddr + sizeof(uint64_t);
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t no_read_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t no_write_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t low_guard = 0x0badf00d11223344ULL;
    const uint64_t high_guard = 0x55667788cafebabeULL;
    const uint64_t load_low_seed = 0x1021324354657687ULL;
    const uint64_t load_high_seed = 0x98a9bacbdcedfe0fULL;
    const uint64_t store_low_seed = 0x21436587a9cbed0fULL;
    const uint64_t store_high_seed = 0xfedcba9876543210ULL;
    const uint64_t load_sentinel_second = 0x5a6b7c8d9eafb001ULL;
    const uint64_t load_sentinel_first = 0x8877665544332211ULL;
    const uint64_t store_fault_payload = 0x0123456789abcdefULL;
    const uint64_t store_repair_payload = 0xf0e1d2c3b4a59687ULL;
    const uint64_t expected_repaired_load =
        ((load_high_seed & 0xffffffffULL) << 32) |
        (load_low_seed >> 32);
    const uint64_t expected_repaired_store_low =
        (store_low_seed & 0xffffffffULL) |
        ((store_repair_payload & 0xffffffffULL) << 32);
    const uint64_t expected_repaired_store_high =
        (store_high_seed & 0xffffffff00000000ULL) |
        (store_repair_payload >> 32);
    uint64_t saved_medeleg;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    saved_medeleg = CSRR(CSR_MEDELEG);
    CSRS(CSR_MEDELEG, (1ULL << CAUSE_LPF) | (1ULL << CAUSE_SPF));
    hspt_init();
    ai_arch_load_store_set_hs_leaf(low_page, legal_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, no_read_leaf);
    write64(low_guard_paddr, low_guard);
    write64(low_paddr, load_low_seed);
    write64(high_paddr, load_high_seed);
    write64(high_guard_paddr, high_guard);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t second_rd = ai_arch_ld_rd_probe(split_vaddr,
        load_sentinel_second);
    TEST_ASSERT("cross-page scalar ld with a legal low half and no-read high half reports the second-half load page fault VA and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr &&
        second_rd == load_sentinel_second
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("second-half load page fault preserves both page fragments and adjacent guards",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == load_low_seed &&
        read64(high_paddr) == load_high_seed &&
        read64(high_guard_paddr) == high_guard
    );

    ai_arch_load_store_set_hs_leaf(low_page, no_read_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, legal_leaf);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t first_rd = ai_arch_ld_rd_probe(split_vaddr,
        load_sentinel_first);
    TEST_ASSERT("cross-page scalar ld with a no-read low half reports the original first-half load page fault VA and does not consume high-half metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == split_vaddr &&
        first_rd == load_sentinel_first
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("first-half load page fault also leaves the boundary image unchanged",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == load_low_seed &&
        read64(high_paddr) == load_high_seed &&
        read64(high_guard_paddr) == high_guard
    );

    ai_arch_load_store_set_hs_leaf(low_page, legal_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, no_write_leaf);
    write64(low_guard_paddr, low_guard);
    write64(low_paddr, store_low_seed);
    write64(high_paddr, store_high_seed);
    write64(high_guard_paddr, high_guard);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(split_vaddr, store_fault_payload);
    TEST_ASSERT("cross-page scalar sd with a legal low half and no-write high half reports the second-half store page fault VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("second-half store page fault has no partial side effect on the low fragment, high fragment, or guards",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == store_low_seed &&
        read64(high_paddr) == store_high_seed &&
        read64(high_guard_paddr) == high_guard
    );

    ai_arch_load_store_set_hs_leaf(low_page, no_write_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, legal_leaf);
    write64(low_guard_paddr, low_guard);
    write64(low_paddr, store_low_seed);
    write64(high_paddr, store_high_seed);
    write64(high_guard_paddr, high_guard);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(split_vaddr, store_fault_payload);
    TEST_ASSERT("cross-page scalar sd with a no-write low half reports the original first-half store page fault VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == split_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("first-half store page fault leaves the whole split target and both guards unchanged",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == store_low_seed &&
        read64(high_paddr) == store_high_seed &&
        read64(high_guard_paddr) == high_guard
    );

    ai_arch_load_store_set_hs_leaf(low_page, legal_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, legal_leaf);
    write64(low_paddr, load_low_seed);
    write64(high_paddr, load_high_seed);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t repaired_load = ai_ld_raw64(split_vaddr);
    TEST_ASSERT("after restoring both read permissions, the same split ld completes and returns the stitched 4B+4B value",
        excpt.triggered == false &&
        repaired_load == expected_repaired_load
    );

    goto_priv(PRIV_M);
    write64(low_guard_paddr, low_guard);
    write64(low_paddr, store_low_seed);
    write64(high_paddr, store_high_seed);
    write64(high_guard_paddr, high_guard);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(split_vaddr, store_repair_payload);
    TEST_ASSERT("after restoring both write permissions, the same split sd retires without stale page-fault metadata",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired split sd writes exactly four bytes on each page and still preserves adjacent guards",
        read64(low_guard_paddr) == low_guard &&
        read64(low_paddr) == expected_repaired_store_low &&
        read64(high_paddr) == expected_repaired_store_high &&
        read64(high_guard_paddr) == high_guard
    );

    ai_arch_load_store_set_hs_leaf(low_page, legal_leaf);
    ai_arch_load_store_set_hs_leaf(high_page, legal_leaf);
    CSRW(CSR_MEDELEG, saved_medeleg);
    sfence_vma();

    TEST_END();
}

bool ai_arch_little_endian_scalar_lane_matrix_corner()
{
    TEST_START();

    const uint64_t lane_seed = 0x8877665544332211ULL;
    const uint64_t canonical_seed = 0x0102030405060708ULL;
    const uint64_t guard_seed = 0x1122334455667788ULL;
    const uint64_t adjacent_guard = 0x99aabbccddeeff00ULL;
    const uint64_t expected_lb[8] = {
        0x11ULL, 0x22ULL, 0x33ULL, 0x44ULL,
        0x55ULL, 0x66ULL, 0x77ULL, 0xffffffffffffff88ULL
    };
    const uint64_t expected_lbu[8] = {
        0x11ULL, 0x22ULL, 0x33ULL, 0x44ULL,
        0x55ULL, 0x66ULL, 0x77ULL, 0x88ULL
    };
    const uint64_t expected_lh[4] = {
        0x2211ULL, 0x4433ULL, 0x6655ULL, 0xffffffffffff8877ULL
    };
    const uint64_t expected_lhu[4] = {
        0x2211ULL, 0x4433ULL, 0x6655ULL, 0x8877ULL
    };
    bool lb_matrix_ok = true;
    bool lbu_matrix_ok = true;
    bool sb_matrix_ok = true;
    bool sh_matrix_ok = true;

    goto_priv(PRIV_M);
    ai_arch_write64_at(0, lane_seed);

    TEST_SETUP_EXCEPT();
    for (size_t offset = 0; offset < 8U; offset++) {
        lb_matrix_ok = lb_matrix_ok &&
            ai_lb_raw64(ai_arch_addr(offset)) == expected_lb[offset];
        lbu_matrix_ok = lbu_matrix_ok &&
            ai_lbu_raw64(ai_arch_addr(offset)) == expected_lbu[offset];
    }
    TEST_ASSERT("lb/lbu little-endian byte-lane matrix maps offsets 0..7 to bytes 0x11..0x88 with signed and zero extension",
        excpt.triggered == false &&
        lb_matrix_ok &&
        lbu_matrix_ok &&
        ai_arch_read64_at(0) == lane_seed
    );

    TEST_SETUP_EXCEPT();
    for (size_t index = 0; index < 4U; index++) {
        const size_t offset = index * 2U;
        sh_matrix_ok = sh_matrix_ok &&
            ai_lh_raw64(ai_arch_addr(offset)) == expected_lh[index] &&
            ai_lhu_raw64(ai_arch_addr(offset)) == expected_lhu[index];
    }
    uint64_t lw_low = ai_lw_raw64(ai_arch_addr(0));
    uint64_t lwu_low = ai_lwu_raw64(ai_arch_addr(0));
    uint64_t lw_high = ai_lw_raw64(ai_arch_addr(4));
    uint64_t lwu_high = ai_lwu_raw64(ai_arch_addr(4));
    uint64_t ld_full = ai_ld_raw64(ai_arch_addr(0));
    TEST_ASSERT("lh/lhu/lw/lwu/ld little-endian aligned lane matrix stitches low-address bytes into low-significance bits and applies XLEN extension",
        excpt.triggered == false &&
        sh_matrix_ok &&
        lw_low == 0x44332211ULL &&
        lwu_low == 0x44332211ULL &&
        lw_high == 0xffffffff88776655ULL &&
        lwu_high == 0x88776655ULL &&
        ld_full == lane_seed &&
        ai_arch_read64_at(0) == lane_seed
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write64_at(0, canonical_seed);
    uint64_t canonical_lh_low = ai_lh_raw64(ai_arch_addr(0));
    uint64_t canonical_lhu_low = ai_lhu_raw64(ai_arch_addr(0));
    uint64_t canonical_lh_midlow = ai_lh_raw64(ai_arch_addr(2));
    uint64_t canonical_lhu_midlow = ai_lhu_raw64(ai_arch_addr(2));
    uint64_t canonical_lh_midhigh = ai_lh_raw64(ai_arch_addr(4));
    uint64_t canonical_lhu_midhigh = ai_lhu_raw64(ai_arch_addr(4));
    uint64_t canonical_lh_high = ai_lh_raw64(ai_arch_addr(6));
    uint64_t canonical_lhu_high = ai_lhu_raw64(ai_arch_addr(6));
    uint64_t canonical_lw_low = ai_lw_raw64(ai_arch_addr(0));
    uint64_t canonical_lwu_low = ai_lwu_raw64(ai_arch_addr(0));
    uint64_t canonical_lw_high = ai_lw_raw64(ai_arch_addr(4));
    uint64_t canonical_lwu_high = ai_lwu_raw64(ai_arch_addr(4));
    uint64_t canonical_ld_full = ai_ld_raw64(ai_arch_addr(0));
    TEST_ASSERT("0x0102030405060708 decodes as halfword lanes 0x0708/0x0506/0x0304/0x0102, word lanes 0x05060708/0x01020304, and full ld image",
        excpt.triggered == false &&
        canonical_lh_low == 0x0708ULL &&
        canonical_lhu_low == 0x0708ULL &&
        canonical_lh_midlow == 0x0506ULL &&
        canonical_lhu_midlow == 0x0506ULL &&
        canonical_lh_midhigh == 0x0304ULL &&
        canonical_lhu_midhigh == 0x0304ULL &&
        canonical_lh_high == 0x0102ULL &&
        canonical_lhu_high == 0x0102ULL &&
        canonical_lw_low == 0x05060708ULL &&
        canonical_lwu_low == 0x05060708ULL &&
        canonical_lw_high == 0x01020304ULL &&
        canonical_lwu_high == 0x01020304ULL &&
        canonical_ld_full == canonical_seed &&
        ai_arch_read64_at(0) == canonical_seed
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write64_at(0, 0ULL);
    for (size_t offset = 0; offset < 8U; offset++) {
        const uint64_t byte_value = 0xa0ULL + offset;
        const uint64_t expected = byte_value << (offset * 8U);

        ai_arch_write64_at(0, 0ULL);
        ai_arch_sb_from_raw64(ai_arch_addr(offset), byte_value);
        sb_matrix_ok = sb_matrix_ok && ai_arch_read64_at(0) == expected;
    }
    TEST_ASSERT("sb little-endian byte-enable matrix writes only the selected offset byte 0..7 and keeps the other seven bytes zero",
        excpt.triggered == false &&
        sb_matrix_ok
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write64_at(0, guard_seed);
    ai_arch_sh_from_raw64(ai_arch_addr(0), 0xbeefULL);
    sh_matrix_ok = sh_matrix_ok && ai_arch_read64_at(0) == 0x112233445566beefULL;
    ai_arch_write64_at(0, guard_seed);
    ai_arch_sh_from_raw64(ai_arch_addr(2), 0xbeefULL);
    sh_matrix_ok = sh_matrix_ok && ai_arch_read64_at(0) == 0x11223344beef7788ULL;
    ai_arch_write64_at(0, guard_seed);
    ai_arch_sh_from_raw64(ai_arch_addr(4), 0xbeefULL);
    sh_matrix_ok = sh_matrix_ok && ai_arch_read64_at(0) == 0x1122beef55667788ULL;
    ai_arch_write64_at(0, guard_seed);
    ai_arch_sh_from_raw64(ai_arch_addr(6), 0xbeefULL);
    sh_matrix_ok = sh_matrix_ok && ai_arch_read64_at(0) == 0xbeef334455667788ULL;
    TEST_ASSERT("sh little-endian halfword byte-enable matrix writes exactly two target bytes at offsets 0/2/4/6",
        excpt.triggered == false &&
        sh_matrix_ok
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write64_at(0, guard_seed);
    ai_arch_sw_from_raw64(ai_arch_addr(0), 0xa1b2c3d4ULL);
    uint64_t sw_low = ai_arch_read64_at(0);
    ai_arch_write64_at(0, guard_seed);
    ai_arch_sw_from_raw64(ai_arch_addr(4), 0xa1b2c3d4ULL);
    uint64_t sw_high = ai_arch_read64_at(0);
    ai_arch_write64_at(0, guard_seed);
    ai_arch_write64_at(8, adjacent_guard);
    ai_arch_sd_from_raw64(ai_arch_addr(0), 0x0102030405060708ULL);
    TEST_ASSERT("sw/sd little-endian byte-enable matrix writes low source bytes to low addresses and preserves non-target doubleword guards",
        excpt.triggered == false &&
        sw_low == 0x11223344a1b2c3d4ULL &&
        sw_high == 0xa1b2c3d455667788ULL &&
        ai_arch_read64_at(0) == 0x0102030405060708ULL &&
        ai_arch_read64_at(8) == adjacent_guard
    );

    TEST_END();
}

bool ai_arch_cross_entry_memory_image_consistency_corner()
{
    TEST_START();

    const uint64_t image = 0x0102030405060708ULL;
    const uint64_t alternate_image = 0x8877665544332211ULL;
    const uint64_t cafebabe_image = 0xcafebabedeadbeefULL;
    const uint64_t fp_qnan_bits = 0x7ff4000000000001ULL;
    const uint64_t fp_one_bits = 0x3ff0000000000000ULL;
    const uint64_t amo_addend = 0x1111111111111111ULL;
    const uint64_t amo_expected = image + amo_addend;
    const uint64_t guard = 0x99aabbccddeeff00ULL;
    uint64_t saved_fcsr;

    goto_priv(PRIV_M);
    ai_arch_enable_fp_vector_state();
    saved_fcsr = CSRR(CSR_FCSR);

    ai_arch_write64_at(0, image);
    ai_arch_write64_at(8, guard);

    TEST_SETUP_EXCEPT();
    uint64_t scalar_ld = ai_ld_raw64(ai_arch_addr(0));
    uint64_t fp_fld = ai_arch_fld_raw_bits(ai_arch_addr(0));
    uint64_t rvc_cld = c_ld(ai_arch_addr(0));
    TEST_ASSERT("scalar ld, fld raw bits, and c.ld all interpret the same eight memory bytes as the same little-endian doubleword",
        excpt.triggered == false &&
        scalar_ld == image &&
        fp_fld == image &&
        rvc_cld == image &&
        ai_arch_read64_at(0) == image &&
        ai_arch_read64_at(8) == guard &&
        CSRR(CSR_FCSR) == saved_fcsr
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoadd_d(ai_arch_addr(0), amo_addend);
    uint64_t scalar_after_amo = ai_ld_raw64(ai_arch_addr(0));
    TEST_ASSERT("amoadd.d returns the same little-endian old doubleword image and the later scalar ld observes only the arithmetic update",
        excpt.triggered == false &&
        amo_old == image &&
        scalar_after_amo == amo_expected &&
        ai_arch_read64_at(8) == guard
    );

    ai_arch_write64_at(0, 0ULL);
    ai_arch_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(ai_arch_addr(0), cafebabe_image);
    uint64_t amo_zero_old = amoadd_d(ai_arch_addr(0), 0ULL);
    uint64_t ld_after_amo_zero = ai_ld_raw64(ai_arch_addr(0));
    TEST_ASSERT("sd image 0xcafebabedeadbeef followed by amoadd.d rs2=0 returns the old image and leaves the later ld byte-for-byte identical",
        excpt.triggered == false &&
        amo_zero_old == cafebabe_image &&
        ld_after_amo_zero == cafebabe_image &&
        ai_arch_read64_at(0) == cafebabe_image &&
        ai_arch_read64_at(8) == guard
    );

    ai_arch_write64_at(0, 0ULL);
    ai_arch_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    c_sd(ai_arch_addr(0), alternate_image);
    uint64_t scalar_after_csd = ai_ld_raw64(ai_arch_addr(0));
    uint64_t fp_after_csd = ai_arch_fld_raw_bits(ai_arch_addr(0));
    TEST_ASSERT("c.sd writes the same little-endian memory image that scalar ld and fld read back byte-for-byte",
        excpt.triggered == false &&
        scalar_after_csd == alternate_image &&
        fp_after_csd == alternate_image &&
        ai_arch_read64_at(0) == alternate_image &&
        ai_arch_read64_at(8) == guard
    );

    ai_arch_write64_at(0, image);
    ai_arch_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    ai_arch_vector_vle64_vse64_once(ai_arch_addr(0), ai_arch_addr(8));
    TEST_ASSERT("vle64 element 0 followed by vse64 stores the exact same eight-byte image as scalar ld from the source doubleword",
        excpt.triggered == false &&
        ai_arch_read64_at(0) == image &&
        ai_arch_read64_at(8) == image
    );

    ai_arch_write64_at(0, 0ULL);
    ai_arch_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(ai_arch_addr(0), image);
    uint64_t sd_ld = ai_ld_raw64(ai_arch_addr(0));
    uint64_t sd_fld = ai_arch_fld_raw_bits(ai_arch_addr(0));
    uint64_t sd_cld = c_ld(ai_arch_addr(0));
    uint64_t sd_vle64_element0 =
        ai_arch_vector_vle64_element0_raw(ai_arch_addr(0));
    ai_arch_vector_vle64_vse64_once(ai_arch_addr(0), ai_arch_addr(8));
    TEST_ASSERT("after sd writes 0x0102030405060708, ld, fld, c.ld, and vle64 element 0 all observe the identical byte sequence",
        excpt.triggered == false &&
        sd_ld == image &&
        sd_fld == image &&
        sd_cld == image &&
        sd_vle64_element0 == image &&
        ai_arch_read64_at(0) == image &&
        ai_arch_read64_at(8) == image
    );

    ai_arch_write64_at(0, 0ULL);
    ai_arch_write64_at(8, guard);
    TEST_SETUP_EXCEPT();
    ai_arch_sd_from_raw64(ai_arch_addr(0), fp_qnan_bits);
    uint64_t fp_qnan_after_sd = ai_arch_fld_raw_bits(ai_arch_addr(0));
    ai_arch_fsd_raw_bits(ai_arch_addr(8), fp_one_bits);
    uint64_t scalar_after_fsd = ai_ld_raw64(ai_arch_addr(8));
    TEST_ASSERT("sd qnan raw bits to fld and fsd 1.0 raw bits to integer ld share the same 64-bit byte lane image without fcsr side effects",
        excpt.triggered == false &&
        fp_qnan_after_sd == fp_qnan_bits &&
        scalar_after_fsd == fp_one_bits &&
        ai_arch_read64_at(0) == fp_qnan_bits &&
        ai_arch_read64_at(8) == fp_one_bits &&
        CSRR(CSR_FCSR) == saved_fcsr
    );

    CSRW(CSR_FCSR, saved_fcsr);

    TEST_END();
}
