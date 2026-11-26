#include <rvh_test.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#define AI_BOUNDARY_ATTR \
    __attribute__((noinline, optimize("no-tree-vectorize", \
                                      "no-tree-loop-distribute-patterns")))

#define AI_FFLAGS_NX 0x01ULL
#define AI_FFLAGS_UF 0x02ULL
#define AI_FFLAGS_OF 0x04ULL
#define AI_FFLAGS_DZ 0x08ULL
#define AI_FFLAGS_NV 0x10ULL

static inline uint64_t ai_sext32(uint32_t value)
{
    return (uint64_t)(int64_t)(int32_t)value;
}

static inline void ai_enable_fp_boundary(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    CSRW(CSR_FCSR, 0);
}

#define AI_R_TYPE_OP(name, insn) \
static inline uint64_t name(uint64_t lhs, uint64_t rhs) \
{ \
    uint64_t rd; \
    asm volatile(insn " %0, %1, %2" \
        : "=r"(rd) : "r"(lhs), "r"(rhs)); \
    return rd; \
}

#define AI_R_TYPE_OP_ARCH(name, arch, insn) \
static inline uint64_t name(uint64_t lhs, uint64_t rhs) \
{ \
    uint64_t rd; \
    asm volatile( \
        ".option push\n\t" \
        ".option arch," arch "\n\t" \
        insn " %0, %1, %2\n\t" \
        ".option pop\n\t" \
        : "=r"(rd) : "r"(lhs), "r"(rhs)); \
    return rd; \
}

#define AI_R1_OP_ARCH(name, arch, insn) \
static inline uint64_t name(uint64_t rs) \
{ \
    uint64_t rd; \
    asm volatile( \
        ".option push\n\t" \
        ".option arch," arch "\n\t" \
        insn " %0, %1\n\t" \
        ".option pop\n\t" \
        : "=r"(rd) : "r"(rs)); \
    return rd; \
}

AI_R_TYPE_OP(ai_add, "add")
AI_R_TYPE_OP(ai_sub, "sub")
AI_R_TYPE_OP(ai_slt, "slt")
AI_R_TYPE_OP(ai_sltu, "sltu")
AI_R_TYPE_OP(ai_xor, "xor")
AI_R_TYPE_OP(ai_or, "or")
AI_R_TYPE_OP(ai_and, "and")
AI_R_TYPE_OP(ai_sll, "sll")
AI_R_TYPE_OP(ai_srl, "srl")
AI_R_TYPE_OP(ai_sra, "sra")
AI_R_TYPE_OP(ai_addw, "addw")
AI_R_TYPE_OP(ai_subw, "subw")
AI_R_TYPE_OP(ai_sllw, "sllw")
AI_R_TYPE_OP(ai_srlw, "srlw")
AI_R_TYPE_OP(ai_sraw, "sraw")
AI_R_TYPE_OP(ai_mul, "mul")
AI_R_TYPE_OP(ai_mulh, "mulh")
AI_R_TYPE_OP(ai_mulhsu, "mulhsu")
AI_R_TYPE_OP(ai_mulhu, "mulhu")
AI_R_TYPE_OP(ai_mulw, "mulw")
AI_R_TYPE_OP(ai_div, "div")
AI_R_TYPE_OP(ai_divu, "divu")
AI_R_TYPE_OP(ai_rem, "rem")
AI_R_TYPE_OP(ai_remu, "remu")
AI_R_TYPE_OP(ai_divw, "divw")
AI_R_TYPE_OP(ai_divuw, "divuw")
AI_R_TYPE_OP(ai_remw, "remw")
AI_R_TYPE_OP(ai_remuw, "remuw")

AI_R1_OP_ARCH(ai_clz, "+zbb", "clz")
AI_R1_OP_ARCH(ai_ctz, "+zbb", "ctz")
AI_R1_OP_ARCH(ai_cpop, "+zbb", "cpop")
AI_R1_OP_ARCH(ai_clzw, "+zbb", "clzw")
AI_R1_OP_ARCH(ai_ctzw, "+zbb", "ctzw")
AI_R1_OP_ARCH(ai_cpopw, "+zbb", "cpopw")
AI_R1_OP_ARCH(ai_sext_b, "+zbb", "sext.b")
AI_R1_OP_ARCH(ai_sext_h, "+zbb", "sext.h")
AI_R1_OP_ARCH(ai_zext_h, "+zbb", "zext.h")
AI_R1_OP_ARCH(ai_rev8, "+zbb", "rev8")
AI_R1_OP_ARCH(ai_orc_b, "+zbb", "orc.b")
AI_R_TYPE_OP_ARCH(ai_min, "+zbb", "min")
AI_R_TYPE_OP_ARCH(ai_max, "+zbb", "max")
AI_R_TYPE_OP_ARCH(ai_minu, "+zbb", "minu")
AI_R_TYPE_OP_ARCH(ai_maxu, "+zbb", "maxu")
AI_R_TYPE_OP_ARCH(ai_andn, "+zbb", "andn")
AI_R_TYPE_OP_ARCH(ai_orn, "+zbb", "orn")
AI_R_TYPE_OP_ARCH(ai_xnor, "+zbb", "xnor")
AI_R_TYPE_OP_ARCH(ai_rol, "+zbb", "rol")
AI_R_TYPE_OP_ARCH(ai_ror, "+zbb", "ror")
AI_R_TYPE_OP_ARCH(ai_rolw, "+zbb", "rolw")
AI_R_TYPE_OP_ARCH(ai_rorw, "+zbb", "rorw")
AI_R_TYPE_OP_ARCH(ai_bset, "+zbs", "bset")
AI_R_TYPE_OP_ARCH(ai_bclr, "+zbs", "bclr")
AI_R_TYPE_OP_ARCH(ai_binv, "+zbs", "binv")
AI_R_TYPE_OP_ARCH(ai_bext, "+zbs", "bext")
AI_R_TYPE_OP_ARCH(ai_sh1add, "+zba", "sh1add")
AI_R_TYPE_OP_ARCH(ai_sh2add, "+zba", "sh2add")
AI_R_TYPE_OP_ARCH(ai_sh3add, "+zba", "sh3add")
AI_R_TYPE_OP_ARCH(ai_add_uw, "+zba", "add.uw")
AI_R_TYPE_OP_ARCH(ai_sh1add_uw, "+zba", "sh1add.uw")
AI_R_TYPE_OP_ARCH(ai_clmul, "+zbc", "clmul")
AI_R_TYPE_OP_ARCH(ai_clmulh, "+zbc", "clmulh")
AI_R_TYPE_OP_ARCH(ai_clmulr, "+zbc", "clmulr")

static AI_BOUNDARY_ATTR uint64_t ai_ref_rol64(uint64_t value, unsigned shamt)
{
    shamt &= 63U;
    return (value << shamt) | (value >> ((64U - shamt) & 63U));
}

static AI_BOUNDARY_ATTR uint64_t ai_ref_ror64(uint64_t value, unsigned shamt)
{
    shamt &= 63U;
    return (value >> shamt) | (value << ((64U - shamt) & 63U));
}

static AI_BOUNDARY_ATTR uint32_t ai_ref_rol32(uint32_t value, unsigned shamt)
{
    shamt &= 31U;
    return (value << shamt) | (value >> ((32U - shamt) & 31U));
}

static AI_BOUNDARY_ATTR uint32_t ai_ref_ror32(uint32_t value, unsigned shamt)
{
    shamt &= 31U;
    return (value >> shamt) | (value << ((32U - shamt) & 31U));
}

static AI_BOUNDARY_ATTR uint64_t ai_ref_clmul(uint64_t lhs, uint64_t rhs)
{
    uint64_t result = 0;

    for (unsigned i = 0; i < 64U; ++i) {
        if ((rhs >> i) & 1ULL) {
            result ^= lhs << i;
        }
    }

    return result;
}

static AI_BOUNDARY_ATTR uint64_t ai_ref_clmulh(uint64_t lhs, uint64_t rhs)
{
    uint64_t result = 0;

    for (unsigned i = 1; i < 64U; ++i) {
        if ((rhs >> i) & 1ULL) {
            result ^= lhs >> (64U - i);
        }
    }

    return result;
}

static AI_BOUNDARY_ATTR uint64_t ai_ref_clmulr(uint64_t lhs, uint64_t rhs)
{
    uint64_t result = 0;

    for (unsigned i = 0; i < 64U; ++i) {
        if ((rhs >> i) & 1ULL) {
            result ^= (i == 63U) ? lhs : (lhs >> (63U - i));
        }
    }

    return result;
}

#define AI_FP_S_BIN(name, insn) \
static inline uint32_t name(uint32_t lhs, uint32_t rhs) \
{ \
    uint64_t rd; \
    asm volatile( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        "fmv.w.x ft0, %1\n\t" \
        "fmv.w.x ft1, %2\n\t" \
        insn " ft2, ft0, ft1\n\t" \
        "fmv.x.w %0, ft2\n\t" \
        ".option pop\n\t" \
        : "=r"(rd) : "r"((uint64_t)lhs), "r"((uint64_t)rhs) \
        : "ft0", "ft1", "ft2"); \
    return (uint32_t)rd; \
}

#define AI_FP_D_BIN(name, insn) \
static inline uint64_t name(uint64_t lhs, uint64_t rhs) \
{ \
    uint64_t rd; \
    asm volatile( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        "fmv.d.x ft0, %1\n\t" \
        "fmv.d.x ft1, %2\n\t" \
        insn " ft2, ft0, ft1\n\t" \
        "fmv.x.d %0, ft2\n\t" \
        ".option pop\n\t" \
        : "=r"(rd) : "r"(lhs), "r"(rhs) \
        : "ft0", "ft1", "ft2"); \
    return rd; \
}

AI_FP_S_BIN(ai_fadd_s, "fadd.s")
AI_FP_S_BIN(ai_fsub_s, "fsub.s")
AI_FP_S_BIN(ai_fmul_s, "fmul.s")
AI_FP_S_BIN(ai_fdiv_s, "fdiv.s")
AI_FP_S_BIN(ai_fmin_s, "fmin.s")
AI_FP_S_BIN(ai_fmax_s, "fmax.s")
AI_FP_S_BIN(ai_fsgnj_s, "fsgnj.s")
AI_FP_S_BIN(ai_fsgnjn_s, "fsgnjn.s")
AI_FP_S_BIN(ai_fsgnjx_s, "fsgnjx.s")
AI_FP_D_BIN(ai_fadd_d, "fadd.d")
AI_FP_D_BIN(ai_fsub_d, "fsub.d")
AI_FP_D_BIN(ai_fmul_d, "fmul.d")
AI_FP_D_BIN(ai_fdiv_d, "fdiv.d")
AI_FP_D_BIN(ai_fmin_d, "fmin.d")
AI_FP_D_BIN(ai_fmax_d, "fmax.d")
AI_FP_D_BIN(ai_fsgnj_d, "fsgnj.d")
AI_FP_D_BIN(ai_fsgnjn_d, "fsgnjn.d")
AI_FP_D_BIN(ai_fsgnjx_d, "fsgnjx.d")

static inline uint32_t ai_fsqrt_s(uint32_t value)
{
    uint64_t rd;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.w.x ft0, %1\n\t"
        "fsqrt.s ft1, ft0\n\t"
        "fmv.x.w %0, ft1\n\t"
        ".option pop\n\t"
        : "=r"(rd) : "r"((uint64_t)value) : "ft0", "ft1");

    return (uint32_t)rd;
}

static inline uint64_t ai_fsqrt_d(uint64_t value)
{
    uint64_t rd;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft0, %1\n\t"
        "fsqrt.d ft1, ft0\n\t"
        "fmv.x.d %0, ft1\n\t"
        ".option pop\n\t"
        : "=r"(rd) : "r"(value) : "ft0", "ft1");

    return rd;
}

static inline uint64_t ai_fclass_s(uint32_t value)
{
    uint64_t rd;

    asm volatile(
        "fmv.w.x ft0, %1\n\t"
        "fclass.s %0, ft0\n\t"
        : "=r"(rd) : "r"((uint64_t)value) : "ft0");

    return rd;
}

static inline uint64_t ai_fclass_d(uint64_t value)
{
    uint64_t rd;

    asm volatile(
        "fmv.d.x ft0, %1\n\t"
        "fclass.d %0, ft0\n\t"
        : "=r"(rd) : "r"(value) : "ft0");

    return rd;
}

static inline uint64_t ai_feq_s(uint32_t lhs, uint32_t rhs)
{
    uint64_t rd;

    asm volatile(
        "fmv.w.x ft0, %1\n\t"
        "fmv.w.x ft1, %2\n\t"
        "feq.s %0, ft0, ft1\n\t"
        : "=r"(rd) : "r"((uint64_t)lhs), "r"((uint64_t)rhs)
        : "ft0", "ft1");

    return rd;
}

static inline uint64_t ai_flt_s(uint32_t lhs, uint32_t rhs)
{
    uint64_t rd;

    asm volatile(
        "fmv.w.x ft0, %1\n\t"
        "fmv.w.x ft1, %2\n\t"
        "flt.s %0, ft0, ft1\n\t"
        : "=r"(rd) : "r"((uint64_t)lhs), "r"((uint64_t)rhs)
        : "ft0", "ft1");

    return rd;
}

static inline uint64_t ai_fle_s(uint32_t lhs, uint32_t rhs)
{
    uint64_t rd;

    asm volatile(
        "fmv.w.x ft0, %1\n\t"
        "fmv.w.x ft1, %2\n\t"
        "fle.s %0, ft0, ft1\n\t"
        : "=r"(rd) : "r"((uint64_t)lhs), "r"((uint64_t)rhs)
        : "ft0", "ft1");

    return rd;
}

static inline uint64_t ai_feq_d(uint64_t lhs, uint64_t rhs)
{
    uint64_t rd;

    asm volatile(
        "fmv.d.x ft0, %1\n\t"
        "fmv.d.x ft1, %2\n\t"
        "feq.d %0, ft0, ft1\n\t"
        : "=r"(rd) : "r"(lhs), "r"(rhs) : "ft0", "ft1");

    return rd;
}

static inline uint64_t ai_flt_d(uint64_t lhs, uint64_t rhs)
{
    uint64_t rd;

    asm volatile(
        "fmv.d.x ft0, %1\n\t"
        "fmv.d.x ft1, %2\n\t"
        "flt.d %0, ft0, ft1\n\t"
        : "=r"(rd) : "r"(lhs), "r"(rhs) : "ft0", "ft1");

    return rd;
}

static inline uint64_t ai_fle_d(uint64_t lhs, uint64_t rhs)
{
    uint64_t rd;

    asm volatile(
        "fmv.d.x ft0, %1\n\t"
        "fmv.d.x ft1, %2\n\t"
        "fle.d %0, ft0, ft1\n\t"
        : "=r"(rd) : "r"(lhs), "r"(rhs) : "ft0", "ft1");

    return rd;
}

static inline uint32_t ai_fmadd_s(uint32_t a, uint32_t b, uint32_t c)
{
    uint64_t rd;

    asm volatile(
        "fmv.w.x ft0, %1\n\t"
        "fmv.w.x ft1, %2\n\t"
        "fmv.w.x ft2, %3\n\t"
        "fmadd.s ft3, ft0, ft1, ft2\n\t"
        "fmv.x.w %0, ft3\n\t"
        : "=r"(rd) : "r"((uint64_t)a), "r"((uint64_t)b),
          "r"((uint64_t)c) : "ft0", "ft1", "ft2", "ft3");

    return (uint32_t)rd;
}

static inline uint64_t ai_fmadd_d(uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t rd;

    asm volatile(
        "fmv.d.x ft0, %1\n\t"
        "fmv.d.x ft1, %2\n\t"
        "fmv.d.x ft2, %3\n\t"
        "fmadd.d ft3, ft0, ft1, ft2\n\t"
        "fmv.x.d %0, ft3\n\t"
        : "=r"(rd) : "r"(a), "r"(b), "r"(c)
        : "ft0", "ft1", "ft2", "ft3");

    return rd;
}

#define AI_FP_S_FUSED(name, insn) \
static inline uint32_t name(uint32_t a, uint32_t b, uint32_t c) \
{ \
    uint64_t rd; \
    asm volatile( \
        "fmv.w.x ft0, %1\n\t" \
        "fmv.w.x ft1, %2\n\t" \
        "fmv.w.x ft2, %3\n\t" \
        insn " ft3, ft0, ft1, ft2\n\t" \
        "fmv.x.w %0, ft3\n\t" \
        : "=r"(rd) : "r"((uint64_t)a), "r"((uint64_t)b), \
          "r"((uint64_t)c) : "ft0", "ft1", "ft2", "ft3"); \
    return (uint32_t)rd; \
}

#define AI_FP_D_FUSED(name, insn) \
static inline uint64_t name(uint64_t a, uint64_t b, uint64_t c) \
{ \
    uint64_t rd; \
    asm volatile( \
        "fmv.d.x ft0, %1\n\t" \
        "fmv.d.x ft1, %2\n\t" \
        "fmv.d.x ft2, %3\n\t" \
        insn " ft3, ft0, ft1, ft2\n\t" \
        "fmv.x.d %0, ft3\n\t" \
        : "=r"(rd) : "r"(a), "r"(b), "r"(c) \
        : "ft0", "ft1", "ft2", "ft3"); \
    return rd; \
}

AI_FP_S_FUSED(ai_fmsub_s, "fmsub.s")
AI_FP_S_FUSED(ai_fnmsub_s, "fnmsub.s")
AI_FP_S_FUSED(ai_fnmadd_s, "fnmadd.s")
AI_FP_D_FUSED(ai_fmsub_d, "fmsub.d")
AI_FP_D_FUSED(ai_fnmsub_d, "fnmsub.d")
AI_FP_D_FUSED(ai_fnmadd_d, "fnmadd.d")

bool ai_arch_scalar_integer_boundary_operand_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint64_t imm_pos;
    uint64_t imm_neg;
    uint64_t slti_neg;
    uint64_t sltiu_allones;
    uint64_t xori_inv;
    uint64_t ori_signext;
    uint64_t andi_low;
    uint64_t slli_top;
    uint64_t srli_top;
    uint64_t srai_top;
    uint64_t addiw_wrap;
    uint64_t slliw_top;
    uint64_t srliw_top;
    uint64_t sraiw_top;

    asm volatile(
        "li t0, 1\n\t"
        "addi %0, t0, 2047\n\t"
        "addi %1, t0, -2048\n\t"
        "li t0, -1\n\t"
        "slti %2, t0, 0\n\t"
        "sltiu %3, t0, 1\n\t"
        "li t0, 0x123456789abcdef0\n\t"
        "xori %4, t0, -1\n\t"
        "ori %5, zero, -2048\n\t"
        "andi %6, t0, 0xff\n\t"
        "li t0, 1\n\t"
        "slli %7, t0, 63\n\t"
        "srli %8, %7, 63\n\t"
        "srai %9, %7, 63\n\t"
        "li t0, 0xffffffff\n\t"
        "addiw %10, t0, 1\n\t"
        "li t0, 1\n\t"
        "slliw %11, t0, 31\n\t"
        "srliw %12, %11, 31\n\t"
        "sraiw %13, %11, 31\n\t"
        : "=r"(imm_pos), "=r"(imm_neg), "=r"(slti_neg),
          "=r"(sltiu_allones), "=r"(xori_inv),
          "=r"(ori_signext), "=r"(andi_low), "=r"(slli_top),
          "=r"(srli_top), "=r"(srai_top), "=r"(addiw_wrap),
          "=r"(slliw_top), "=r"(srliw_top), "=r"(sraiw_top)
        :
        : "t0");

    TEST_ASSERT("RV64I immediate boundary operands cover addi +/- imm12 edge, signed/unsigned compare, logical-immediate sign extension, and shift-immediate masks",
        imm_pos == 2048ULL &&
        imm_neg == UINT64_MAX - 2046ULL &&
        slti_neg == 1ULL &&
        sltiu_allones == 0ULL &&
        xori_inv == ~0x123456789abcdef0ULL &&
        ori_signext == 0xfffffffffffff800ULL &&
        andi_low == 0xf0ULL &&
        slli_top == 0x8000000000000000ULL &&
        srli_top == 1ULL &&
        srai_top == UINT64_MAX &&
        addiw_wrap == 0ULL &&
        slliw_top == 0xffffffff80000000ULL &&
        srliw_top == 1ULL &&
        sraiw_top == UINT64_MAX);

    TEST_ASSERT("RV64I register ALU boundary operands cover wraparound add/sub, logical ops, signed/unsigned compare extremes, and shamt masking",
        ai_add(UINT64_MAX, 1) == 0ULL &&
        ai_add(INT64_MAX, 1) == 0x8000000000000000ULL &&
        ai_sub(0, 1) == UINT64_MAX &&
        ai_xor(0x123456789abcdef0ULL, UINT64_MAX) ==
            ~0x123456789abcdef0ULL &&
        ai_or(0x123456789abcdef0ULL, 0x0f0f0f0f0f0f0f0fULL) ==
            0x1f3f5f7f9fbfdfffULL &&
        ai_and(0x123456789abcdef0ULL, 0xffULL) == 0xf0ULL &&
        ai_slt(UINT64_MAX, 0) == 1ULL &&
        ai_sltu(UINT64_MAX, 0) == 0ULL &&
        ai_slt(0x8000000000000000ULL, 0x7fffffffffffffffULL) == 1ULL &&
        ai_sltu(0x8000000000000000ULL, 0x7fffffffffffffffULL) == 0ULL &&
        ai_sll(1, 63) == 0x8000000000000000ULL &&
        ai_sll(1, 64) == 1ULL &&
        ai_srl(0x8000000000000000ULL, 63) == 1ULL &&
        ai_sra(0x8000000000000000ULL, 63) == UINT64_MAX);

    TEST_ASSERT("RV64I W-form compute instructions sign-extend low 32-bit boundary results after overflow and shifts",
        ai_addw(0x7fffffffULL, 1) == 0xffffffff80000000ULL &&
        ai_subw(0x80000000ULL, 1) == 0x7fffffffULL &&
        ai_sllw(1, 31) == 0xffffffff80000000ULL &&
        ai_srlw(0x80000000ULL, 31) == 1ULL &&
        ai_sraw(0x80000000ULL, 31) == UINT64_MAX &&
        ai_sllw(1, 32) == 1ULL);

    TEST_END();
}

bool ai_arch_scalar_mul_div_boundary_operand_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    const uint64_t smin = 0x8000000000000000ULL;

    TEST_ASSERT("M extension multiply boundary operands cover low/high signed, signed-unsigned, unsigned, and W-form products",
        ai_mul(UINT64_MAX, 2) == UINT64_MAX - 1ULL &&
        ai_mulh(smin, 2) == UINT64_MAX &&
        ai_mulhu(UINT64_MAX, UINT64_MAX) == UINT64_MAX - 1ULL &&
        ai_mulhsu(UINT64_MAX, UINT64_MAX) == UINT64_MAX &&
        ai_mulw(0x80000000ULL, 2) == 0ULL);

    TEST_ASSERT("M extension divide/remainder boundary operands cover truncation, divide-by-zero, signed overflow, unsigned, and W-form sign extension",
        ai_div(7, UINT64_MAX - 2ULL) == UINT64_MAX - 1ULL &&
        ai_rem(7, UINT64_MAX - 2ULL) == 1ULL &&
        ai_div(UINT64_MAX - 6ULL, 3) == UINT64_MAX - 1ULL &&
        ai_rem(UINT64_MAX - 6ULL, 3) == UINT64_MAX &&
        ai_div(123, 0) == UINT64_MAX &&
        ai_rem(123, 0) == 123ULL &&
        ai_divu(0x123456789abcdef0ULL, 0) == UINT64_MAX &&
        ai_remu(0x123456789abcdef0ULL, 0) == 0x123456789abcdef0ULL &&
        ai_div(smin, UINT64_MAX) == smin &&
        ai_rem(smin, UINT64_MAX) == 0ULL &&
        ai_divuw(UINT32_MAX, 2) == 0x7fffffffULL &&
        ai_remuw(UINT32_MAX, 2) == 1ULL &&
        ai_divw(0x80000000ULL, UINT64_MAX) == 0xffffffff80000000ULL &&
        ai_remw(0x80000000ULL, UINT64_MAX) == 0ULL);

    TEST_END();
}

bool ai_arch_scalar_bitmanip_boundary_operand_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t pattern = 0x0123456789abcdefULL;

    TEST_ASSERT("Zbb count/min/max/logical boundary operands cover zero, all-ones, sign-bit, signed and unsigned extremes",
        ai_clz(0) == 64ULL &&
        ai_ctz(0) == 64ULL &&
        ai_cpop(0) == 0ULL &&
        ai_clz(1) == 63ULL &&
        ai_ctz(0x8000000000000000ULL) == 63ULL &&
        ai_cpop(UINT64_MAX) == 64ULL &&
        ai_clzw(0) == 32ULL &&
        ai_ctzw(0) == 32ULL &&
        ai_cpopw(UINT64_MAX) == 32ULL &&
        ai_min(0x8000000000000000ULL, 0x7fffffffffffffffULL) == 0x8000000000000000ULL &&
        ai_max(0x8000000000000000ULL, 0x7fffffffffffffffULL) == 0x7fffffffffffffffULL &&
        ai_minu(0x8000000000000000ULL, 0x7fffffffffffffffULL) == 0x7fffffffffffffffULL &&
        ai_maxu(0x8000000000000000ULL, 0x7fffffffffffffffULL) == 0x8000000000000000ULL &&
        ai_andn(pattern, UINT64_MAX) == 0ULL &&
        ai_orn(0, 0) == UINT64_MAX &&
        ai_xnor(pattern, pattern) == UINT64_MAX);

    TEST_ASSERT("Zbb/Zbs shift-like boundary operands cover rotate masks, sign/zero extension, byte reverse/or-combine, and bit-index masking",
        ai_rol(pattern, 63) == ai_ref_rol64(pattern, 63) &&
        ai_ror(pattern, 63) == ai_ref_ror64(pattern, 63) &&
        ai_rolw(0x80000001ULL, 1) == ai_sext32(ai_ref_rol32(0x80000001U, 1)) &&
        ai_rorw(0x80000001ULL, 1) == ai_sext32(ai_ref_ror32(0x80000001U, 1)) &&
        ai_sext_b(0x80) == UINT64_MAX - 127ULL &&
        ai_sext_h(0x8000) == UINT64_MAX - 32767ULL &&
        ai_zext_h(UINT64_MAX) == 0xffffULL &&
        ai_rev8(pattern) == 0xefcdab8967452301ULL &&
        ai_orc_b(0x000100800000ff00ULL) == 0x00ff00ff0000ff00ULL &&
        ai_bset(0, 63) == 0x8000000000000000ULL &&
        ai_bclr(UINT64_MAX, 63) == 0x7fffffffffffffffULL &&
        ai_binv(0, 63) == 0x8000000000000000ULL &&
        ai_bext(0x8000000000000000ULL, 63) == 1ULL &&
        ai_bset(0, 64) == 1ULL);

    TEST_ASSERT("Zba/Zbc boundary operands cover word-zero extension add paths and carry-less multiply low/high/reversed projections",
        ai_add_uw(UINT64_MAX, 1) == 0x100000000ULL &&
        ai_sh1add(0x8000000000000000ULL, 1) == 1ULL &&
        ai_sh2add(0x4000000000000000ULL, 3) == 3ULL &&
        ai_sh3add(0x2000000000000000ULL, 5) == 5ULL &&
        ai_sh1add_uw(UINT64_MAX, 1) == 0x1ffffffffULL &&
        ai_clmul(3, 5) == 0xfULL &&
        ai_clmul(pattern, UINT64_MAX) == ai_ref_clmul(pattern, UINT64_MAX) &&
        ai_clmulh(pattern, UINT64_MAX) == ai_ref_clmulh(pattern, UINT64_MAX) &&
        ai_clmulr(pattern, UINT64_MAX) == ai_ref_clmulr(pattern, UINT64_MAX));

    TEST_END();
}

bool ai_arch_fp_f_boundary_special_operand_matrix_corner()
{
    TEST_START();

    ai_enable_fp_boundary();

    CSRW(CSR_FFLAGS, 0);
    const uint32_t add_zero = ai_fadd_s(0x00000000U, 0x80000000U);
    const uint32_t sub_zero = ai_fsub_s(0x00000000U, 0x00000000U);
    const uint32_t min_zero = ai_fmin_s(0x80000000U, 0x00000000U);
    const uint32_t max_zero = ai_fmax_s(0x80000000U, 0x00000000U);
    const uint32_t sgnj = ai_fsgnj_s(0x3f800000U, 0x80000000U);
    const uint32_t sgnjn = ai_fsgnjn_s(0x3f800000U, 0x80000000U);
    const uint32_t sgnjx = ai_fsgnjx_s(0x3f800000U, 0x80000000U);

    TEST_ASSERT("F single-precision zero/sign special operands cover +/-zero arithmetic, min/max zero ordering, and sign-injection variants",
        add_zero == 0x00000000U &&
        sub_zero == 0x00000000U &&
        min_zero == 0x80000000U &&
        max_zero == 0x00000000U &&
        sgnj == 0xbf800000U &&
        sgnjn == 0x3f800000U &&
        sgnjx == 0xbf800000U);

    CSRW(CSR_FFLAGS, 0);
    const uint32_t overflow = ai_fmul_s(0x7f7fffffU, 0x40000000U);
    const uint64_t overflow_flags = CSRR(CSR_FFLAGS) & 0x1fULL;
    CSRW(CSR_FFLAGS, 0);
    const uint32_t div_zero = ai_fdiv_s(0x3f800000U, 0x00000000U);
    const uint64_t div_zero_flags = CSRR(CSR_FFLAGS) & 0x1fULL;
    CSRW(CSR_FFLAGS, 0);
    const uint32_t zero_zero = ai_fdiv_s(0x00000000U, 0x00000000U);
    const uint64_t zero_zero_flags = CSRR(CSR_FFLAGS) & 0x1fULL;
    CSRW(CSR_FFLAGS, 0);
    const uint32_t sqrt_neg = ai_fsqrt_s(0xbf800000U);
    const uint64_t sqrt_neg_flags = CSRR(CSR_FFLAGS) & 0x1fULL;

    TEST_ASSERT("F single-precision exceptional special operands cover overflow, divide-by-zero, invalid 0/0, invalid sqrt(-1), and canonical NaN results",
        overflow == 0x7f800000U &&
        (overflow_flags & (AI_FFLAGS_OF | AI_FFLAGS_NX)) ==
            (AI_FFLAGS_OF | AI_FFLAGS_NX) &&
        div_zero == 0x7f800000U &&
        (div_zero_flags & AI_FFLAGS_DZ) != 0 &&
        zero_zero == 0x7fc00000U &&
        (zero_zero_flags & AI_FFLAGS_NV) != 0 &&
        sqrt_neg == 0x7fc00000U &&
        (sqrt_neg_flags & AI_FFLAGS_NV) != 0);

    CSRW(CSR_FFLAGS, 0);
    const uint64_t feq_qnan = ai_feq_s(0x7fc00000U, 0x7fc00000U);
    const uint64_t feq_qnan_flags = CSRR(CSR_FFLAGS) & 0x1fULL;
    CSRW(CSR_FFLAGS, 0);
    const uint64_t flt_qnan = ai_flt_s(0x7fc00000U, 0x3f800000U);
    const uint64_t flt_qnan_flags = CSRR(CSR_FFLAGS) & 0x1fULL;

    TEST_ASSERT("F single-precision classify/compare special operands cover infinities, subnormals, quiet/signaling NaNs, and ordered-vs-unordered compare flags",
        ai_fclass_s(0xff800000U) == (1ULL << 0) &&
        ai_fclass_s(0x80000000U) == (1ULL << 3) &&
        ai_fclass_s(0x00000001U) == (1ULL << 5) &&
        ai_fclass_s(0x7fc00000U) == (1ULL << 9) &&
        ai_fclass_s(0x7f800001U) == (1ULL << 8) &&
        ai_feq_s(0x3f800000U, 0x3f800000U) == 1ULL &&
        ai_fle_s(0xbf800000U, 0x00000000U) == 1ULL &&
        feq_qnan == 0ULL &&
        feq_qnan_flags == 0ULL &&
        flt_qnan == 0ULL &&
        (flt_qnan_flags & AI_FFLAGS_NV) != 0);

    CSRW(CSR_FFLAGS, 0);
    uint64_t fcvt_rtz;
    uint64_t fcvt_wu;
    uint32_t cvt_i32_min;
    uint32_t cvt_u32_mid;
    uint32_t cvt_d_to_s;
    asm volatile(
        "li t0, 0x3fc00000\n\t"
        "fmv.w.x ft0, t0\n\t"
        "fcvt.w.s %0, ft0, rtz\n\t"
        "li t0, 0x4f000000\n\t"
        "fmv.w.x ft1, t0\n\t"
        "fcvt.wu.s %1, ft1, rtz\n\t"
        "li t0, -2147483648\n\t"
        "fcvt.s.w ft2, t0, rne\n\t"
        "fmv.x.w %2, ft2\n\t"
        "li t0, 0x80000000\n\t"
        "fcvt.s.wu ft3, t0, rne\n\t"
        "fmv.x.w %3, ft3\n\t"
        "li t0, 0x3ff8000000000000\n\t"
        "fmv.d.x ft4, t0\n\t"
        "fcvt.s.d ft5, ft4, rne\n\t"
        "fmv.x.w %4, ft5\n\t"
        : "=r"(fcvt_rtz), "=r"(fcvt_wu), "=r"(cvt_i32_min),
          "=r"(cvt_u32_mid), "=r"(cvt_d_to_s)
        :
        : "t0", "ft0", "ft1", "ft2", "ft3", "ft4", "ft5");
    const uint64_t fcvt_flags = CSRR(CSR_FFLAGS) & 0x1fULL;

    TEST_ASSERT("F single-precision conversion/fused compute boundary operands cover signed/unsigned conversions, D-to-S conversion, and all fused add/sub sign forms",
        fcvt_rtz == 1ULL &&
        (fcvt_flags & AI_FFLAGS_NX) != 0 &&
        fcvt_wu == 0xffffffff80000000ULL &&
        cvt_i32_min == 0xcf000000U &&
        cvt_u32_mid == 0x4f000000U &&
        cvt_d_to_s == 0x3fc00000U &&
        ai_fmadd_s(0x40000000U, 0x40400000U, 0x40800000U) ==
            0x41200000U &&
        ai_fmsub_s(0x40000000U, 0x40400000U, 0x40800000U) ==
            0x40000000U &&
        ai_fnmsub_s(0x40000000U, 0x40400000U, 0x40800000U) ==
            0xc0000000U &&
        ai_fnmadd_s(0x40000000U, 0x40400000U, 0x40800000U) ==
            0xc1200000U);

    TEST_END();
}

bool ai_arch_fp_d_boundary_special_operand_matrix_corner()
{
    TEST_START();

    ai_enable_fp_boundary();

    CSRW(CSR_FFLAGS, 0);
    const uint64_t add_zero = ai_fadd_d(0x0000000000000000ULL,
        0x8000000000000000ULL);
    const uint64_t sub_zero = ai_fsub_d(0x0000000000000000ULL,
        0x0000000000000000ULL);
    const uint64_t min_zero = ai_fmin_d(0x8000000000000000ULL,
        0x0000000000000000ULL);
    const uint64_t max_zero = ai_fmax_d(0x8000000000000000ULL,
        0x0000000000000000ULL);
    const uint64_t sgnj = ai_fsgnj_d(0x3ff0000000000000ULL,
        0x8000000000000000ULL);
    const uint64_t sgnjn = ai_fsgnjn_d(0x3ff0000000000000ULL,
        0x8000000000000000ULL);
    const uint64_t sgnjx = ai_fsgnjx_d(0x3ff0000000000000ULL,
        0x8000000000000000ULL);

    TEST_ASSERT("D double-precision zero/sign special operands cover +/-zero arithmetic, min/max zero ordering, and sign-injection variants",
        add_zero == 0x0000000000000000ULL &&
        sub_zero == 0x0000000000000000ULL &&
        min_zero == 0x8000000000000000ULL &&
        max_zero == 0x0000000000000000ULL &&
        sgnj == 0xbff0000000000000ULL &&
        sgnjn == 0x3ff0000000000000ULL &&
        sgnjx == 0xbff0000000000000ULL);

    CSRW(CSR_FFLAGS, 0);
    const uint64_t overflow = ai_fmul_d(0x7fefffffffffffffULL,
        0x4000000000000000ULL);
    const uint64_t overflow_flags = CSRR(CSR_FFLAGS) & 0x1fULL;
    CSRW(CSR_FFLAGS, 0);
    const uint64_t div_zero = ai_fdiv_d(0x3ff0000000000000ULL,
        0x0000000000000000ULL);
    const uint64_t div_zero_flags = CSRR(CSR_FFLAGS) & 0x1fULL;
    CSRW(CSR_FFLAGS, 0);
    const uint64_t zero_zero = ai_fdiv_d(0x0000000000000000ULL,
        0x0000000000000000ULL);
    const uint64_t zero_zero_flags = CSRR(CSR_FFLAGS) & 0x1fULL;
    CSRW(CSR_FFLAGS, 0);
    const uint64_t sqrt_neg = ai_fsqrt_d(0xbff0000000000000ULL);
    const uint64_t sqrt_neg_flags = CSRR(CSR_FFLAGS) & 0x1fULL;

    TEST_ASSERT("D double-precision exceptional special operands cover overflow, divide-by-zero, invalid 0/0, invalid sqrt(-1), and canonical NaN results",
        overflow == 0x7ff0000000000000ULL &&
        (overflow_flags & (AI_FFLAGS_OF | AI_FFLAGS_NX)) ==
            (AI_FFLAGS_OF | AI_FFLAGS_NX) &&
        div_zero == 0x7ff0000000000000ULL &&
        (div_zero_flags & AI_FFLAGS_DZ) != 0 &&
        zero_zero == 0x7ff8000000000000ULL &&
        (zero_zero_flags & AI_FFLAGS_NV) != 0 &&
        sqrt_neg == 0x7ff8000000000000ULL &&
        (sqrt_neg_flags & AI_FFLAGS_NV) != 0);

    CSRW(CSR_FFLAGS, 0);
    const uint64_t feq_qnan = ai_feq_d(0x7ff8000000000000ULL,
        0x7ff8000000000000ULL);
    const uint64_t feq_qnan_flags = CSRR(CSR_FFLAGS) & 0x1fULL;
    CSRW(CSR_FFLAGS, 0);
    const uint64_t flt_qnan = ai_flt_d(0x7ff8000000000000ULL,
        0x3ff0000000000000ULL);
    const uint64_t flt_qnan_flags = CSRR(CSR_FFLAGS) & 0x1fULL;

    TEST_ASSERT("D double-precision classify/compare special operands cover infinities, subnormals, quiet/signaling NaNs, and ordered-vs-unordered compare flags",
        ai_fclass_d(0xfff0000000000000ULL) == (1ULL << 0) &&
        ai_fclass_d(0x8000000000000000ULL) == (1ULL << 3) &&
        ai_fclass_d(0x0000000000000001ULL) == (1ULL << 5) &&
        ai_fclass_d(0x7ff8000000000000ULL) == (1ULL << 9) &&
        ai_fclass_d(0x7ff0000000000001ULL) == (1ULL << 8) &&
        ai_feq_d(0x3ff0000000000000ULL, 0x3ff0000000000000ULL) == 1ULL &&
        ai_fle_d(0xbff0000000000000ULL, 0x0000000000000000ULL) == 1ULL &&
        feq_qnan == 0ULL &&
        feq_qnan_flags == 0ULL &&
        flt_qnan == 0ULL &&
        (flt_qnan_flags & AI_FFLAGS_NV) != 0);

    CSRW(CSR_FFLAGS, 0);
    uint64_t fcvt_rtz;
    uint64_t fcvt_lu;
    uint64_t cvt_i64_min;
    uint64_t cvt_u64_mid;
    uint64_t cvt_s_to_d;
    asm volatile(
        "li t0, 0x3ff8000000000000\n\t"
        "fmv.d.x ft0, t0\n\t"
        "fcvt.l.d %0, ft0, rtz\n\t"
        "li t0, 0x43e0000000000000\n\t"
        "fmv.d.x ft1, t0\n\t"
        "fcvt.lu.d %1, ft1, rtz\n\t"
        "li t0, 0x8000000000000000\n\t"
        "fcvt.d.l ft2, t0, rne\n\t"
        "fmv.x.d %2, ft2\n\t"
        "li t0, 0x8000000000000000\n\t"
        "fcvt.d.lu ft3, t0, rne\n\t"
        "fmv.x.d %3, ft3\n\t"
        "li t0, 0x3fc00000\n\t"
        "fmv.w.x ft4, t0\n\t"
        "fcvt.d.s ft5, ft4\n\t"
        "fmv.x.d %4, ft5\n\t"
        : "=r"(fcvt_rtz), "=r"(fcvt_lu), "=r"(cvt_i64_min),
          "=r"(cvt_u64_mid), "=r"(cvt_s_to_d)
        :
        : "t0", "ft0", "ft1", "ft2", "ft3", "ft4", "ft5");
    const uint64_t fcvt_flags = CSRR(CSR_FFLAGS) & 0x1fULL;

    TEST_ASSERT("D double-precision conversion/fused compute boundary operands cover signed/unsigned conversions, S-to-D conversion, and all fused add/sub sign forms",
        fcvt_rtz == 1ULL &&
        (fcvt_flags & AI_FFLAGS_NX) != 0 &&
        fcvt_lu == 0x8000000000000000ULL &&
        cvt_i64_min == 0xc3e0000000000000ULL &&
        cvt_u64_mid == 0x43e0000000000000ULL &&
        cvt_s_to_d == 0x3ff8000000000000ULL &&
        ai_fmadd_d(0x4000000000000000ULL, 0x4008000000000000ULL,
            0x4010000000000000ULL) == 0x4024000000000000ULL &&
        ai_fmsub_d(0x4000000000000000ULL, 0x4008000000000000ULL,
            0x4010000000000000ULL) == 0x4000000000000000ULL &&
        ai_fnmsub_d(0x4000000000000000ULL, 0x4008000000000000ULL,
            0x4010000000000000ULL) == 0xc000000000000000ULL &&
        ai_fnmadd_d(0x4000000000000000ULL, 0x4008000000000000ULL,
            0x4010000000000000ULL) == 0xc024000000000000ULL);

    TEST_END();
}
