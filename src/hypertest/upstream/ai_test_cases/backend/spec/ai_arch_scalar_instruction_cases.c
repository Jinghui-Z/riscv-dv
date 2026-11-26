#include <rvh_test.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define AI_SCALAR_ATTR \
    __attribute__((noinline, optimize("no-tree-vectorize", \
                                      "no-tree-loop-distribute-patterns")))

#define AI_SCALAR_FFLAGS_NX 0x01ULL
#define AI_SCALAR_FFLAGS_NV 0x10ULL
#define AI_SCALAR_MSTATUS_TSR (1ULL << 22)

#ifndef AI_SCALAR_REG_RANDOM_SEED
#define AI_SCALAR_REG_RANDOM_SEED 0
#endif

#define AI_SCALAR_REG_PROFILE (AI_SCALAR_REG_RANDOM_SEED & 3)
#define AI_SCALAR_STR_(x) #x
#define AI_SCALAR_STR(x) AI_SCALAR_STR_(x)

#if AI_SCALAR_REG_PROFILE == 0
#define AI_SCALAR_GPR_SRC0 t0
#define AI_SCALAR_GPR_SRC1 t1
#define AI_SCALAR_GPR_TMP0 t2
#define AI_SCALAR_GPR_DST  t3
#define AI_SCALAR_GPR_TMP1 t4
#define AI_SCALAR_GPR_TMP2 t5
#define AI_SCALAR_GPR_TMP3 t6
#define AI_SCALAR_CGPR_BASE a1
#define AI_SCALAR_CGPR_TMP0 a2
#define AI_SCALAR_CGPR_TMP1 a3
#define AI_SCALAR_CGPR_TMP2 a4
#define AI_SCALAR_CGPR_ACC  a0
#elif AI_SCALAR_REG_PROFILE == 1
#define AI_SCALAR_GPR_SRC0 a2
#define AI_SCALAR_GPR_SRC1 a3
#define AI_SCALAR_GPR_TMP0 a4
#define AI_SCALAR_GPR_DST  a5
#define AI_SCALAR_GPR_TMP1 a6
#define AI_SCALAR_GPR_TMP2 a7
#define AI_SCALAR_GPR_TMP3 t0
#define AI_SCALAR_CGPR_BASE a2
#define AI_SCALAR_CGPR_TMP0 a3
#define AI_SCALAR_CGPR_TMP1 a4
#define AI_SCALAR_CGPR_TMP2 a5
#define AI_SCALAR_CGPR_ACC  a1
#elif AI_SCALAR_REG_PROFILE == 2
#define AI_SCALAR_GPR_SRC0 s1
#define AI_SCALAR_GPR_SRC1 s2
#define AI_SCALAR_GPR_TMP0 s3
#define AI_SCALAR_GPR_DST  s4
#define AI_SCALAR_GPR_TMP1 s5
#define AI_SCALAR_GPR_TMP2 s6
#define AI_SCALAR_GPR_TMP3 s7
#define AI_SCALAR_CGPR_BASE a3
#define AI_SCALAR_CGPR_TMP0 a4
#define AI_SCALAR_CGPR_TMP1 a5
#define AI_SCALAR_CGPR_TMP2 a2
#define AI_SCALAR_CGPR_ACC  a0
#else
#define AI_SCALAR_GPR_SRC0 s8
#define AI_SCALAR_GPR_SRC1 s9
#define AI_SCALAR_GPR_TMP0 s10
#define AI_SCALAR_GPR_DST  s11
#define AI_SCALAR_GPR_TMP1 a2
#define AI_SCALAR_GPR_TMP2 a3
#define AI_SCALAR_GPR_TMP3 a4
#define AI_SCALAR_CGPR_BASE a4
#define AI_SCALAR_CGPR_TMP0 a5
#define AI_SCALAR_CGPR_TMP1 a2
#define AI_SCALAR_CGPR_TMP2 a3
#define AI_SCALAR_CGPR_ACC  a1
#endif

#define AI_SCALAR_GPR_SRC0_S AI_SCALAR_STR(AI_SCALAR_GPR_SRC0)
#define AI_SCALAR_GPR_SRC1_S AI_SCALAR_STR(AI_SCALAR_GPR_SRC1)
#define AI_SCALAR_GPR_TMP0_S AI_SCALAR_STR(AI_SCALAR_GPR_TMP0)
#define AI_SCALAR_GPR_DST_S  AI_SCALAR_STR(AI_SCALAR_GPR_DST)
#define AI_SCALAR_GPR_TMP1_S AI_SCALAR_STR(AI_SCALAR_GPR_TMP1)
#define AI_SCALAR_GPR_TMP2_S AI_SCALAR_STR(AI_SCALAR_GPR_TMP2)
#define AI_SCALAR_GPR_TMP3_S AI_SCALAR_STR(AI_SCALAR_GPR_TMP3)
#define AI_SCALAR_CGPR_BASE_S AI_SCALAR_STR(AI_SCALAR_CGPR_BASE)
#define AI_SCALAR_CGPR_TMP0_S AI_SCALAR_STR(AI_SCALAR_CGPR_TMP0)
#define AI_SCALAR_CGPR_TMP1_S AI_SCALAR_STR(AI_SCALAR_CGPR_TMP1)
#define AI_SCALAR_CGPR_TMP2_S AI_SCALAR_STR(AI_SCALAR_CGPR_TMP2)
#define AI_SCALAR_CGPR_ACC_S  AI_SCALAR_STR(AI_SCALAR_CGPR_ACC)

#if AI_SCALAR_REG_PROFILE == 0
#define AI_SCALAR_FPR_SRC0 ft0
#define AI_SCALAR_FPR_SRC1 ft1
#define AI_SCALAR_FPR_DST  ft2
#define AI_SCALAR_FPR_TMP  ft3
#elif AI_SCALAR_REG_PROFILE == 1
#define AI_SCALAR_FPR_SRC0 ft4
#define AI_SCALAR_FPR_SRC1 ft5
#define AI_SCALAR_FPR_DST  ft6
#define AI_SCALAR_FPR_TMP  ft7
#elif AI_SCALAR_REG_PROFILE == 2
#define AI_SCALAR_FPR_SRC0 ft8
#define AI_SCALAR_FPR_SRC1 ft9
#define AI_SCALAR_FPR_DST  ft10
#define AI_SCALAR_FPR_TMP  ft11
#else
#define AI_SCALAR_FPR_SRC0 fa0
#define AI_SCALAR_FPR_SRC1 fa1
#define AI_SCALAR_FPR_DST  fa2
#define AI_SCALAR_FPR_TMP  fa3
#endif

#define AI_SCALAR_FPR_SRC0_S AI_SCALAR_STR(AI_SCALAR_FPR_SRC0)
#define AI_SCALAR_FPR_SRC1_S AI_SCALAR_STR(AI_SCALAR_FPR_SRC1)
#define AI_SCALAR_FPR_DST_S  AI_SCALAR_STR(AI_SCALAR_FPR_DST)
#define AI_SCALAR_FPR_TMP_S  AI_SCALAR_STR(AI_SCALAR_FPR_TMP)

extern uint64_t ecall_args[2];

static uint8_t ai_scalar_mem[128] __attribute__((aligned(16)));
static uint64_t ai_scalar_obs[96] __attribute__((aligned(16)));

static inline uint64_t ai_scalar_sext32(uint32_t value)
{
    return (uint64_t)(int64_t)(int32_t)value;
}

static inline uint64_t ai_scalar_sext16(uint16_t value)
{
    return (uint64_t)(int64_t)(int16_t)value;
}

static inline uint64_t ai_scalar_sext8(uint8_t value)
{
    return (uint64_t)(int64_t)(int8_t)value;
}

static AI_SCALAR_ATTR uint64_t ai_scalar_rotl64(uint64_t value,
    unsigned shamt)
{
    shamt &= 63U;
    return (value << shamt) | (value >> ((64U - shamt) & 63U));
}

static AI_SCALAR_ATTR uint64_t ai_scalar_rotr64(uint64_t value,
    unsigned shamt)
{
    shamt &= 63U;
    return (value >> shamt) | (value << ((64U - shamt) & 63U));
}

static AI_SCALAR_ATTR uint32_t ai_scalar_rotl32(uint32_t value,
    unsigned shamt)
{
    shamt &= 31U;
    return (value << shamt) | (value >> ((32U - shamt) & 31U));
}

static AI_SCALAR_ATTR uint32_t ai_scalar_rotr32(uint32_t value,
    unsigned shamt)
{
    shamt &= 31U;
    return (value >> shamt) | (value << ((32U - shamt) & 31U));
}

static AI_SCALAR_ATTR uint64_t ai_scalar_rev8(uint64_t value)
{
    uint64_t result = 0;

    for (unsigned i = 0; i < 8U; ++i) {
        result = (result << 8) | ((value >> (8U * i)) & 0xffULL);
    }

    return result;
}

static AI_SCALAR_ATTR uint64_t ai_scalar_orc_b(uint64_t value)
{
    uint64_t result = 0;

    for (unsigned i = 0; i < 8U; ++i) {
        if (((value >> (8U * i)) & 0xffULL) != 0) {
            result |= 0xffULL << (8U * i);
        }
    }

    return result;
}

static AI_SCALAR_ATTR uint64_t ai_scalar_clmul(uint64_t lhs, uint64_t rhs)
{
    uint64_t result = 0;

    for (unsigned i = 0; i < 64U; ++i) {
        if ((rhs >> i) & 1ULL) {
            result ^= lhs << i;
        }
    }

    return result;
}

static AI_SCALAR_ATTR uint64_t ai_scalar_clmulh(uint64_t lhs, uint64_t rhs)
{
    uint64_t result = 0;

    for (unsigned i = 1; i < 64U; ++i) {
        if ((rhs >> i) & 1ULL) {
            result ^= lhs >> (64U - i);
        }
    }

    return result;
}

static AI_SCALAR_ATTR uint64_t ai_scalar_clmulr(uint64_t lhs, uint64_t rhs)
{
    uint64_t result = 0;

    for (unsigned i = 0; i < 64U; ++i) {
        if ((rhs >> i) & 1ULL) {
            if (i == 63U) {
                result ^= lhs;
            } else {
                result ^= lhs >> (63U - i);
            }
        }
    }

    return result;
}

static inline uint64_t ai_scalar_mulh_signed(int64_t lhs, int64_t rhs)
{
    return (uint64_t)(((__int128)lhs * (__int128)rhs) >> 64);
}

static inline uint64_t ai_scalar_mulh_su(int64_t lhs, uint64_t rhs)
{
    return (uint64_t)(((__int128)lhs * (unsigned __int128)rhs) >> 64);
}

static inline uint64_t ai_scalar_mulh_unsigned(uint64_t lhs, uint64_t rhs)
{
    return (uint64_t)(((unsigned __int128)lhs *
        (unsigned __int128)rhs) >> 64);
}

static inline void ai_scalar_enable_fp(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    CSRW(CSR_FCSR, 0);
}

static inline uintptr_t ai_scalar_fire_ebreak(void)
{
    uintptr_t pc;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "la %[pc], 1f\n\t"
        "1:\n\t"
        "ebreak\n\t"
        ".option pop\n\t"
        : [pc] "=r"(pc)
        :
        : "memory");

    return pc;
}

#define AI_SCALAR_EXEC_WORD(word_) \
    asm volatile( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        ".word " #word_ "\n\t" \
        ".option pop\n\t" \
        ::: "memory")

#define AI_SCALAR_EXEC_HALF(half_) \
    asm volatile( \
        ".option push\n\t" \
        ".option rvc\n\t" \
        ".2byte " #half_ "\n\t" \
        ".option pop\n\t" \
        ::: "memory")

bool ai_arch_scalar_integer_alu_immediate_register_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint64_t *obs = ai_scalar_obs;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x123456789abcdef0\n\t"
        "addi " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S ", 0\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 0(%[obs])\n\t"
        "addi " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S ", -17\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 8(%[obs])\n\t"
        "slti " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S ", 0\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 16(%[obs])\n\t"
        "sltiu " AI_SCALAR_GPR_SRC1_S ", zero, 1\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 24(%[obs])\n\t"
        "li " AI_SCALAR_GPR_TMP0_S ", 5\n\t"
        "sltiu " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S ", 1\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 32(%[obs])\n\t"
        "xori " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S ", -1\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 40(%[obs])\n\t"
        "ori x0, " AI_SCALAR_GPR_SRC0_S ", 0x20\n\t"
        "addi " AI_SCALAR_GPR_SRC1_S ", x0, 0\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 48(%[obs])\n\t"
        "andi " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S ", 0x0ff\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 56(%[obs])\n\t"
        "slli " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S ", 4\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 64(%[obs])\n\t"
        "srli " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S ", 60\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 72(%[obs])\n\t"
        "li " AI_SCALAR_GPR_DST_S ", -16\n\t"
        "srai " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_DST_S ", 2\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 80(%[obs])\n\t"
        "lui " AI_SCALAR_GPR_SRC1_S ", 0x12345\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 88(%[obs])\n\t"
        "1:\n\t"
        "auipc " AI_SCALAR_GPR_SRC1_S ", 0\n\t"
        "la " AI_SCALAR_GPR_TMP0_S ", 1b\n\t"
        "sub " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 96(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 7\n\t"
        "li " AI_SCALAR_GPR_TMP0_S ", -3\n\t"
        "add " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 104(%[obs])\n\t"
        "sub " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 112(%[obs])\n\t"
        "slt " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 120(%[obs])\n\t"
        "sltu " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 128(%[obs])\n\t"
        "and " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 136(%[obs])\n\t"
        "or " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 144(%[obs])\n\t"
        "xor " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 152(%[obs])\n\t"
        "sll " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 160(%[obs])\n\t"
        "srl " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 168(%[obs])\n\t"
        "sra " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 176(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 0x7fffffffffffffff\n\t"
        "addi " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC1_S ", 1\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 184(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 0xffffffff\n\t"
        "addiw " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC1_S ", 1\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 192(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 1\n\t"
        "slliw " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC1_S ", 31\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 200(%[obs])\n\t"
        "srliw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_TMP0_S ", 31\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 208(%[obs])\n\t"
        "sraiw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_TMP0_S ", 31\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 216(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 0xffffffff\n\t"
        "li " AI_SCALAR_GPR_TMP0_S ", 2\n\t"
        "addw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 224(%[obs])\n\t"
        "subw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 232(%[obs])\n\t"
        "sllw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 240(%[obs])\n\t"
        "srlw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 248(%[obs])\n\t"
        "sraw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 256(%[obs])\n\t"
        "add x0, " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sub x0, " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sll x0, " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "addi x0, x0, 0\n\t"
        "addi " AI_SCALAR_GPR_DST_S ", x0, 0\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 264(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [obs] "r"(obs)
        : AI_SCALAR_GPR_SRC0_S, AI_SCALAR_GPR_SRC1_S,
          AI_SCALAR_GPR_TMP0_S, AI_SCALAR_GPR_DST_S, "memory");

    TEST_ASSERT("RV64I integer immediate/register matrix covers ADDI/SLTI/SLTIU/logical/shift/LUI/AUIPC/base-register operations, overflow wrap, W sign-extension, and rd=x0 HINT/NOP behavior",
        obs[0] == 0x123456789abcdef0ULL &&
        obs[1] == 0x123456789abcdedfULL &&
        obs[2] == 0ULL &&
        obs[3] == 1ULL &&
        obs[4] == 0ULL &&
        obs[5] == ~0x123456789abcdef0ULL &&
        obs[6] == 0ULL &&
        obs[7] == 0xf0ULL &&
        obs[8] == 80ULL &&
        obs[9] == 1ULL &&
        obs[10] == (uint64_t)-4LL &&
        obs[11] == 0x12345000ULL &&
        obs[12] == 0ULL &&
        obs[13] == 4ULL &&
        obs[14] == 10ULL &&
        obs[15] == 1ULL &&
        obs[16] == 0ULL &&
        obs[17] == (0x123456789abcdef0ULL & (uint64_t)-3LL) &&
        obs[18] == (0x123456789abcdef0ULL | (uint64_t)-3LL) &&
        obs[19] == (0x123456789abcdef0ULL ^ (uint64_t)-3LL) &&
        obs[20] == (7ULL << 61) &&
        obs[21] == (0x123456789abcdef0ULL >> 7) &&
        obs[22] == (uint64_t)-1LL &&
        obs[23] == 0x8000000000000000ULL &&
        obs[24] == 0ULL &&
        obs[25] == 0xffffffff80000000ULL &&
        obs[26] == 1ULL &&
        obs[27] == 0xffffffffffffffffULL &&
        obs[28] == 1ULL &&
        obs[29] == 3ULL &&
        obs[30] == 8ULL &&
        obs[31] == 0x3fffffffULL &&
        obs[32] == 0xffffffffffffffffULL &&
        obs[33] == 0ULL
    );

    TEST_END();
}

bool ai_arch_scalar_branch_jal_jalr_condition_and_link_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint64_t *obs = ai_scalar_obs;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0\n\t"
        "jal " AI_SCALAR_GPR_SRC1_S ", 1f\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 1\n\t"
        "1:\n\t"
        "la " AI_SCALAR_GPR_TMP0_S ", 1b\n\t"
        "sub " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 0(%[obs])\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 8(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0\n\t"
        "jal x0, 2f\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 1\n\t"
        "2:\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 16(%[obs])\n\t"
        "la " AI_SCALAR_GPR_SRC0_S ", 3f\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 1\n\t"
        "jalr " AI_SCALAR_GPR_SRC1_S ", 0(" AI_SCALAR_GPR_SRC0_S ")\n\t"
        "ori " AI_SCALAR_GPR_TMP1_S ", zero, 0x7f\n\t"
        "3:\n\t"
        "la " AI_SCALAR_GPR_TMP0_S ", 3b\n\t"
        "sub " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 24(%[obs])\n\t"
        "sd " AI_SCALAR_GPR_TMP1_S ", 32(%[obs])\n\t"
        "li " AI_SCALAR_GPR_TMP2_S ", 0x55\n\t"
        "la " AI_SCALAR_GPR_SRC0_S ", 4f\n\t"
        "jalr x0, 0(" AI_SCALAR_GPR_SRC0_S ")\n\t"
        "li " AI_SCALAR_GPR_TMP2_S ", 0xaa\n\t"
        "4:\n\t"
        "sd " AI_SCALAR_GPR_TMP2_S ", 40(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 5\n\t"
        "li " AI_SCALAR_GPR_TMP0_S ", 5\n\t"
        "beq " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S ", 5f\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 1\n\t"
        "5:\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 2\n\t"
        "bne " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S ", 6f\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 4\n\t"
        "j 7f\n\t"
        "6:\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 8\n\t"
        "7:\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", -1\n\t"
        "li " AI_SCALAR_GPR_TMP0_S ", 1\n\t"
        "blt " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S ", 8f\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 16\n\t"
        "8:\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 32\n\t"
        "bge " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S ", 9f\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 64\n\t"
        "j 10f\n\t"
        "9:\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 128\n\t"
        "10:\n\t"
        "bltu " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S ", 11f\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 256\n\t"
        "11:\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 512\n\t"
        "bgeu " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_GPR_TMP0_S ", 12f\n\t"
        "ori " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", 1024\n\t"
        "j 13f\n\t"
        "12:\n\t"
        "li " AI_SCALAR_GPR_TMP3_S ", 2048\n\t"
        "or " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_TMP3_S "\n\t"
        "13:\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 48(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [obs] "r"(obs)
        : AI_SCALAR_GPR_SRC0_S, AI_SCALAR_GPR_SRC1_S,
          AI_SCALAR_GPR_TMP0_S, AI_SCALAR_GPR_DST_S,
          AI_SCALAR_GPR_TMP1_S, AI_SCALAR_GPR_TMP2_S,
          AI_SCALAR_GPR_TMP3_S, "memory");

    TEST_ASSERT("JAL/JALR links point at the next instruction, JALR clears target bit0, rd=x0 drops the link, and all signed/unsigned branch true/false paths resolve architecturally",
        obs[0] == -4ULL &&
        obs[1] == 0ULL &&
        obs[2] == 0ULL &&
        obs[3] == -4ULL &&
        obs[4] != 0x7fULL &&
        obs[5] == 0x55ULL &&
        obs[6] == (2ULL | 4ULL | 32ULL | 64ULL | 256ULL | 512ULL | 2048ULL)
    );

    TEST_END();
}

bool ai_arch_scalar_mul_div_edge_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint64_t *obs = ai_scalar_obs;
    const int64_t lhs_s = -0x123456789LL;
    const int64_t rhs_s = 0x12345LL;
    const uint64_t lhs_u = 0xfedcba9876543210ULL;
    const uint64_t rhs_u = 0x0000000102030405ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv " AI_SCALAR_GPR_SRC0_S ", %[lhs_s]\n\t"
        "mv " AI_SCALAR_GPR_SRC1_S ", %[rhs_s]\n\t"
        "mul " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 0(%[obs])\n\t"
        "mulh " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 8(%[obs])\n\t"
        "mulhsu " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", %[rhs_u]\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 16(%[obs])\n\t"
        "mulhu " AI_SCALAR_GPR_TMP0_S ", %[lhs_u], %[rhs_u]\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 24(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", -3\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 7\n\t"
        "mulw " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 32(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", -100\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 7\n\t"
        "div " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 40(%[obs])\n\t"
        "rem " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 48(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", -1\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 7\n\t"
        "divu " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 56(%[obs])\n\t"
        "remu " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 64(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 123\n\t"
        "div " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", zero\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 72(%[obs])\n\t"
        "rem " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", zero\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 80(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x8000000000000000\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", -1\n\t"
        "div " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 88(%[obs])\n\t"
        "rem " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 96(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", -100\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 7\n\t"
        "divw " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 104(%[obs])\n\t"
        "remw " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 112(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", -1\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 7\n\t"
        "divuw " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 120(%[obs])\n\t"
        "remuw " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 128(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x80000000\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", -1\n\t"
        "divw " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 136(%[obs])\n\t"
        "remw " AI_SCALAR_GPR_TMP0_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_TMP0_S ", 144(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [obs] "r"(obs),
          [lhs_s] "r"(lhs_s),
          [rhs_s] "r"(rhs_s),
          [lhs_u] "r"(lhs_u),
          [rhs_u] "r"(rhs_u)
        : AI_SCALAR_GPR_SRC0_S, AI_SCALAR_GPR_SRC1_S,
          AI_SCALAR_GPR_TMP0_S, "memory");

    TEST_ASSERT("M extension matrix covers MUL/MULH/MULHSU/MULHU/MULW, fused-order pairs, signed/unsigned divide and remainder, divide-by-zero, signed overflow, and W sign-extension rules",
        obs[0] == (uint64_t)(lhs_s * rhs_s) &&
        obs[1] == ai_scalar_mulh_signed(lhs_s, rhs_s) &&
        obs[2] == ai_scalar_mulh_su(lhs_s, rhs_u) &&
        obs[3] == ai_scalar_mulh_unsigned(lhs_u, rhs_u) &&
        obs[4] == ai_scalar_sext32((uint32_t)((int32_t)-3 * (int32_t)7)) &&
        obs[5] == (uint64_t)(int64_t)(-100 / 7) &&
        obs[6] == (uint64_t)(int64_t)(-100 % 7) &&
        obs[7] == (UINT64_MAX / 7ULL) &&
        obs[8] == (UINT64_MAX % 7ULL) &&
        obs[9] == UINT64_MAX &&
        obs[10] == 123ULL &&
        obs[11] == 0x8000000000000000ULL &&
        obs[12] == 0ULL &&
        obs[13] == (uint64_t)(int64_t)(int32_t)(-100 / 7) &&
        obs[14] == (uint64_t)(int64_t)(int32_t)(-100 % 7) &&
        obs[15] == ai_scalar_sext32(UINT32_MAX / 7U) &&
        obs[16] == ai_scalar_sext32(UINT32_MAX % 7U) &&
        obs[17] == 0xffffffff80000000ULL &&
        obs[18] == 0ULL
    );

    TEST_END();
}

bool ai_arch_scalar_fence_ecall_ebreak_csr_zifencei_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint64_t *obs = ai_scalar_obs;
    const uint64_t old_mscratch = CSRR(CSR_MSCRATCH);
    const uint64_t old_ecall_arg0 = ecall_args[0];
    const uint64_t old_ecall_arg1 = ecall_args[1];
    const uint64_t old_mstatus = CSRR(CSR_MSTATUS);
    uintptr_t ebreak_epc;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fence rw, rw\n\t"
        "fence.tso\n\t"
        ".word 0x0ff0000f\n\t"
        ".word 0x0000000f\n\t"
        "fence.i\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x11\n\t"
        "csrrw x0, mscratch, " AI_SCALAR_GPR_SRC0_S "\n\t"
        "csrr " AI_SCALAR_GPR_SRC1_S ", mscratch\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 0(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x22\n\t"
        "csrrw " AI_SCALAR_GPR_SRC1_S ", mscratch, " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 8(%[obs])\n\t"
        "csrr " AI_SCALAR_GPR_SRC1_S ", mscratch\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 16(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x05\n\t"
        "csrrs " AI_SCALAR_GPR_SRC1_S ", mscratch, " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 24(%[obs])\n\t"
        "csrr " AI_SCALAR_GPR_SRC1_S ", mscratch\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 32(%[obs])\n\t"
        "csrrs " AI_SCALAR_GPR_SRC1_S ", mscratch, x0\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 40(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x04\n\t"
        "csrrc " AI_SCALAR_GPR_SRC1_S ", mscratch, " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 48(%[obs])\n\t"
        "csrr " AI_SCALAR_GPR_SRC1_S ", mscratch\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 56(%[obs])\n\t"
        "csrwi mscratch, 7\n\t"
        "csrr " AI_SCALAR_GPR_SRC1_S ", mscratch\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 64(%[obs])\n\t"
        "csrrwi " AI_SCALAR_GPR_SRC1_S ", mscratch, 8\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 72(%[obs])\n\t"
        "csrr " AI_SCALAR_GPR_SRC1_S ", mscratch\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 80(%[obs])\n\t"
        "csrrsi " AI_SCALAR_GPR_SRC1_S ", mscratch, 3\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 88(%[obs])\n\t"
        "csrr " AI_SCALAR_GPR_SRC1_S ", mscratch\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 96(%[obs])\n\t"
        "csrrci " AI_SCALAR_GPR_SRC1_S ", mscratch, 1\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 104(%[obs])\n\t"
        "csrr " AI_SCALAR_GPR_SRC1_S ", mscratch\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 112(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [obs] "r"(obs)
        : AI_SCALAR_GPR_SRC0_S, AI_SCALAR_GPR_SRC1_S, "memory");

    TEST_ASSERT("FENCE variants and FENCE.I retire without architectural register side effects, while CSR read/write forms honor rd/rs1 zero and immediate semantics on mscratch",
        obs[0] == 0x11ULL &&
        obs[1] == 0x11ULL &&
        obs[2] == 0x22ULL &&
        obs[3] == 0x22ULL &&
        obs[4] == 0x27ULL &&
        obs[5] == 0x27ULL &&
        obs[6] == 0x27ULL &&
        obs[7] == 0x23ULL &&
        obs[8] == 7ULL &&
        obs[9] == 7ULL &&
        obs[10] == 8ULL &&
        obs[11] == 8ULL &&
        obs[12] == 0xbULL &&
        obs[13] == 0xbULL &&
        obs[14] == 0xaULL
    );

    ecall_args[0] = 0;
    ecall_args[1] = 0;
    TEST_SETUP_EXCEPT();
    ecall_code();
    TEST_ASSERT("M-mode ECALL reports machine ecall and leaves mtval zero",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_MACHINE_ECALL &&
        excpt.tval == 0
    );

    TEST_SETUP_EXCEPT();
    ebreak_epc = ai_scalar_fire_ebreak();
    TEST_ASSERT("M-mode EBREAK reports breakpoint at the ebreak PC",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BREAKPOINT &&
        excpt.epc == ebreak_epc &&
        excpt.tval == ebreak_epc
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_MRET();
    TEST_ASSERT("MRET returns to the programmed mepc without raising an exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MSCRATCH, old_mscratch);
    CSRW(CSR_MSTATUS, old_mstatus);
    ecall_args[0] = old_ecall_arg0;
    ecall_args[1] = old_ecall_arg1;

    TEST_END();
}

bool ai_arch_scalar_fp_f_d_compute_convert_compare_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_scalar_enable_fp();

    volatile uint32_t *s_mem = (volatile uint32_t *)ai_scalar_mem;
    volatile uint64_t *d_mem = (volatile uint64_t *)(ai_scalar_mem + 32);
    uint64_t *obs = ai_scalar_obs;

    s_mem[0] = 0x3fc00000U;
    s_mem[1] = 0;
    d_mem[0] = 0x3ff8000000000000ULL;
    d_mem[1] = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "flw " AI_SCALAR_FPR_SRC0_S ", 0(%[smem])\n\t"
        "fmv.x.w " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 0(%[obs])\n\t"
        "fsw " AI_SCALAR_FPR_SRC0_S ", 4(%[smem])\n\t"
        "fld " AI_SCALAR_FPR_SRC1_S ", 0(%[dmem])\n\t"
        "fmv.x.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 8(%[obs])\n\t"
        "fsd " AI_SCALAR_FPR_SRC1_S ", 8(%[dmem])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x3fc00000\n\t"
        "fmv.w.x " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x40100000\n\t"
        "fmv.w.x " AI_SCALAR_FPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "fadd.s " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        "fmv.x.w " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 16(%[obs])\n\t"
        "fsub.s " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC1_S ", " AI_SCALAR_FPR_SRC0_S "\n\t"
        "fmv.x.w " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 24(%[obs])\n\t"
        "fmul.s " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        "fmv.x.w " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 32(%[obs])\n\t"
        "fdiv.s " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC1_S ", " AI_SCALAR_FPR_SRC0_S "\n\t"
        "fmv.x.w " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 40(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x40800000\n\t"
        "fmv.w.x " AI_SCALAR_FPR_TMP_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "fsqrt.s " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_TMP_S "\n\t"
        "fmv.x.w " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 48(%[obs])\n\t"
        "fsgnjn.s " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        "fmv.x.w " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 56(%[obs])\n\t"
        "feq.s " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 64(%[obs])\n\t"
        "flt.s " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 72(%[obs])\n\t"
        "fle.s " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S ", " AI_SCALAR_FPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 80(%[obs])\n\t"
        "fclass.s " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 88(%[obs])\n\t"
        "fcvt.w.s " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S ", rtz\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 96(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 42\n\t"
        "fcvt.s.w " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", rne\n\t"
        "fmv.x.w " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 104(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x80000000\n\t"
        "fmv.w.x " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "fmv.x.w " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC1_S ", 112(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x3ff8000000000000\n\t"
        "fmv.d.x " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x4002000000000000\n\t"
        "fmv.d.x " AI_SCALAR_FPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "fadd.d " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        "fmv.x.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 120(%[obs])\n\t"
        "fmul.d " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        "fmv.x.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 128(%[obs])\n\t"
        "fdiv.d " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC1_S ", " AI_SCALAR_FPR_SRC0_S "\n\t"
        "fmv.x.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 136(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x4010000000000000\n\t"
        "fmv.d.x " AI_SCALAR_FPR_TMP_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "fsqrt.d " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_TMP_S "\n\t"
        "fmv.x.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 144(%[obs])\n\t"
        "fsgnjx.d " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        "fmv.x.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 152(%[obs])\n\t"
        "feq.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 160(%[obs])\n\t"
        "flt.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 168(%[obs])\n\t"
        "fclass.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 176(%[obs])\n\t"
        "fcvt.l.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S ", rtz\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 184(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 42\n\t"
        "fcvt.d.l " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", rne\n\t"
        "fmv.x.d " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        "sd " AI_SCALAR_GPR_SRC0_S ", 192(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [obs] "r"(obs), [smem] "r"(s_mem), [dmem] "r"(d_mem)
        : AI_SCALAR_GPR_SRC0_S, AI_SCALAR_GPR_SRC1_S,
          AI_SCALAR_FPR_SRC0_S, AI_SCALAR_FPR_SRC1_S,
          AI_SCALAR_FPR_DST_S, AI_SCALAR_FPR_TMP_S, "memory");

    TEST_ASSERT("F/D load-store matrix preserves FLW low raw bits, raw FLD/FSD bits, and exact store images",
        obs[0] == 0x3fc00000ULL &&
        s_mem[1] == 0x3fc00000U &&
        obs[1] == 0x3ff8000000000000ULL &&
        d_mem[1] == 0x3ff8000000000000ULL
    );

    TEST_ASSERT("F single-precision arithmetic/compare/classify/convert/move results match architectural raw-bit expectations",
        obs[2] == 0x40700000ULL &&
        obs[3] == 0x3f400000ULL &&
        obs[4] == 0x40580000ULL &&
        obs[5] == 0x3fc00000ULL &&
        obs[6] == 0x40000000ULL &&
        obs[7] == 0xffffffffbfc00000ULL &&
        obs[8] == 1ULL &&
        obs[9] == 1ULL &&
        obs[10] == 0ULL &&
        obs[11] == (1ULL << 6) &&
        obs[12] == 2ULL &&
        obs[13] == 0x42280000ULL &&
        obs[14] == 0xffffffff80000000ULL
    );

    TEST_ASSERT("D double-precision arithmetic/compare/classify/convert/move results match architectural raw-bit expectations",
        obs[15] == 0x400e000000000000ULL &&
        obs[16] == 0x400b000000000000ULL &&
        obs[17] == 0x3ff8000000000000ULL &&
        obs[18] == 0x4000000000000000ULL &&
        obs[19] == 0x3ff8000000000000ULL &&
        obs[20] == 1ULL &&
        obs[21] == 1ULL &&
        obs[22] == (1ULL << 6) &&
        obs[23] == 2ULL &&
        obs[24] == 0x4045000000000000ULL
    );

    CSRW(CSR_FFLAGS, 0);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x7f800001\n\t"
        "fmv.w.x " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x3f800000\n\t"
        "fmv.w.x " AI_SCALAR_FPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "fmin.s " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S "\n\t"
        ".option pop\n\t"
        ::: AI_SCALAR_GPR_SRC0_S, AI_SCALAR_FPR_SRC0_S,
            AI_SCALAR_FPR_SRC1_S, AI_SCALAR_FPR_DST_S, "memory");
    const uint64_t fmin_fflags = CSRR(CSR_FFLAGS) & 0x1fULL;

    CSRW(CSR_FFLAGS, 0);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0\n\t"
        "fmv.w.x " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x7f800000\n\t"
        "fmv.w.x " AI_SCALAR_FPR_SRC1_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x7fc00000\n\t"
        "fmv.w.x " AI_SCALAR_FPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "fmadd.s " AI_SCALAR_FPR_TMP_S ", " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_FPR_SRC1_S ", " AI_SCALAR_FPR_DST_S "\n\t"
        ".option pop\n\t"
        ::: AI_SCALAR_GPR_SRC0_S, AI_SCALAR_FPR_SRC0_S,
            AI_SCALAR_FPR_SRC1_S, AI_SCALAR_FPR_DST_S,
            AI_SCALAR_FPR_TMP_S, "memory");
    const uint64_t fmadd_fflags = CSRR(CSR_FFLAGS) & 0x1fULL;

    CSRW(CSR_FFLAGS, 0);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x3fc00000\n\t"
        "fmv.w.x " AI_SCALAR_FPR_SRC0_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "fcvt.w.s " AI_SCALAR_GPR_SRC1_S ", " AI_SCALAR_FPR_SRC0_S ", rtz\n\t"
        ".option pop\n\t"
        ::: AI_SCALAR_GPR_SRC0_S, AI_SCALAR_GPR_SRC1_S,
            AI_SCALAR_FPR_SRC0_S, "memory");
    const uint64_t fcvt_fflags = CSRR(CSR_FFLAGS) & 0x1fULL;

    TEST_ASSERT("F/D corner flags cover signaling-NaN min/max invalid, fused 0*inf+qNaN invalid, and inexact floating-to-integer conversion",
        (fmin_fflags & AI_SCALAR_FFLAGS_NV) != 0 &&
        (fmadd_fflags & AI_SCALAR_FFLAGS_NV) != 0 &&
        (fcvt_fflags & AI_SCALAR_FFLAGS_NX) != 0
    );

    TEST_END();
}

bool ai_arch_scalar_b_extension_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint64_t *obs = ai_scalar_obs;
    const uint64_t lhs = 0x00ff00ff80000001ULL;
    const uint64_t rhs = 5ULL;

    asm volatile(
        ".option push\n\t"
        ".option arch,+zba,+zbb,+zbc,+zbs\n\t"
        "mv " AI_SCALAR_GPR_SRC0_S ", %[lhs]\n\t"
        "mv " AI_SCALAR_GPR_SRC1_S ", %[rhs]\n\t"
        "clz " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 0(%[obs])\n\t"
        "ctz " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 8(%[obs])\n\t"
        "cpop " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 16(%[obs])\n\t"
        "clzw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 24(%[obs])\n\t"
        "ctzw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 32(%[obs])\n\t"
        "cpopw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 40(%[obs])\n\t"
        "clmul " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 48(%[obs])\n\t"
        "clmulh " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 56(%[obs])\n\t"
        "clmulr " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 64(%[obs])\n\t"
        "add.uw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 72(%[obs])\n\t"
        "andn " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 80(%[obs])\n\t"
        "orn " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 88(%[obs])\n\t"
        "xnor " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 96(%[obs])\n\t"
        "max " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 104(%[obs])\n\t"
        "maxu " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 112(%[obs])\n\t"
        "min " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 120(%[obs])\n\t"
        "minu " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 128(%[obs])\n\t"
        "orc.b " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 136(%[obs])\n\t"
        "rev8 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 144(%[obs])\n\t"
        "rol " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 152(%[obs])\n\t"
        "ror " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 160(%[obs])\n\t"
        "rori " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", 5\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 168(%[obs])\n\t"
        "rolw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 176(%[obs])\n\t"
        "rorw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 184(%[obs])\n\t"
        "roriw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", 5\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 192(%[obs])\n\t"
        "sext.b " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 200(%[obs])\n\t"
        "sext.h " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 208(%[obs])\n\t"
        "zext.h " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 216(%[obs])\n\t"
        "sh1add " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 224(%[obs])\n\t"
        "sh2add " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 232(%[obs])\n\t"
        "sh3add " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 240(%[obs])\n\t"
        "sh1add.uw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 248(%[obs])\n\t"
        "sh2add.uw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 256(%[obs])\n\t"
        "sh3add.uw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 264(%[obs])\n\t"
        "slli.uw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", 4\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 272(%[obs])\n\t"
        "bclr " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 280(%[obs])\n\t"
        "bclri " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", 7\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 288(%[obs])\n\t"
        "bext " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 296(%[obs])\n\t"
        "bexti " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", 31\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 304(%[obs])\n\t"
        "binv " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 312(%[obs])\n\t"
        "binvi " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", 7\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 320(%[obs])\n\t"
        "bset " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 328(%[obs])\n\t"
        "bseti " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", 7\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 336(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [obs] "r"(obs), [lhs] "r"(lhs), [rhs] "r"(rhs)
        : AI_SCALAR_GPR_SRC0_S, AI_SCALAR_GPR_SRC1_S,
          AI_SCALAR_GPR_DST_S, "memory");

    TEST_ASSERT("B zbb count and zbc carry-less multiply operations match scalar golden models",
        obs[0] == (uint64_t)__builtin_clzll(lhs) &&
        obs[1] == (uint64_t)__builtin_ctzll(lhs) &&
        obs[2] == (uint64_t)__builtin_popcountll(lhs) &&
        obs[3] == (uint64_t)__builtin_clz((uint32_t)lhs) &&
        obs[4] == (uint64_t)__builtin_ctz((uint32_t)lhs) &&
        obs[5] == (uint64_t)__builtin_popcount((uint32_t)lhs) &&
        obs[6] == ai_scalar_clmul(lhs, rhs) &&
        obs[7] == ai_scalar_clmulh(lhs, rhs) &&
        obs[8] == ai_scalar_clmulr(lhs, rhs)
    );

    TEST_ASSERT("B zba/zbb logic, min/max, byte-wise OR-combine, reverse, and rotate results match scalar golden models",
        obs[9] == ((uint32_t)lhs + rhs) &&
        obs[10] == (lhs & ~rhs) &&
        obs[11] == (lhs | ~rhs) &&
        obs[12] == ~(lhs ^ rhs) &&
        obs[13] == lhs &&
        obs[14] == lhs &&
        obs[15] == rhs &&
        obs[16] == rhs &&
        obs[17] == ai_scalar_orc_b(lhs) &&
        obs[18] == ai_scalar_rev8(lhs) &&
        obs[19] == ai_scalar_rotl64(lhs, rhs) &&
        obs[20] == ai_scalar_rotr64(lhs, rhs) &&
        obs[21] == ai_scalar_rotr64(lhs, 5) &&
        obs[22] == ai_scalar_sext32(ai_scalar_rotl32((uint32_t)lhs, rhs)) &&
        obs[23] == ai_scalar_sext32(ai_scalar_rotr32((uint32_t)lhs, rhs)) &&
        obs[24] == ai_scalar_sext32(ai_scalar_rotr32((uint32_t)lhs, 5)) &&
        obs[25] == ai_scalar_sext8((uint8_t)lhs) &&
        obs[26] == ai_scalar_sext16((uint16_t)lhs) &&
        obs[27] == ((uint16_t)lhs)
    );

    TEST_ASSERT("B zba shifted-add and zero-extended word shifted-add operations match scalar golden models",
        obs[28] == ((lhs << 1) + rhs) &&
        obs[29] == ((lhs << 2) + rhs) &&
        obs[30] == ((lhs << 3) + rhs) &&
        obs[31] == (((uint64_t)(uint32_t)lhs << 1) + rhs) &&
        obs[32] == (((uint64_t)(uint32_t)lhs << 2) + rhs) &&
        obs[33] == (((uint64_t)(uint32_t)lhs << 3) + rhs) &&
        obs[34] == (((uint64_t)(uint32_t)lhs) << 4)
    );

    TEST_ASSERT("B zbs bit clear/extract/invert/set register and immediate variants match scalar golden models",
        obs[35] == (lhs & ~(1ULL << (rhs & 63U))) &&
        obs[36] == (lhs & ~(1ULL << 7)) &&
        obs[37] == ((lhs >> (rhs & 63U)) & 1ULL) &&
        obs[38] == ((lhs >> 31) & 1ULL) &&
        obs[39] == (lhs ^ (1ULL << (rhs & 63U))) &&
        obs[40] == (lhs ^ (1ULL << 7)) &&
        obs[41] == (lhs | (1ULL << (rhs & 63U))) &&
        obs[42] == (lhs | (1ULL << 7))
    );

    TEST_END();
}

bool ai_arch_scalar_k_extension_opcode_smoke_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint64_t *obs = ai_scalar_obs;
    const uint64_t lhs = 0x0011223344556677ULL;
    const uint64_t rhs = 0x8899aabbccddeeffULL;

    for (unsigned i = 0; i < 24U; ++i) {
        obs[i] = 0xfeedface00000000ULL | i;
    }

    TEST_SETUP_EXCEPT();
    asm volatile(
        ".option push\n\t"
        ".option arch,+zknd,+zkne,+zknh,+zksed,+zksh,+zbkb,+zbkc,+zbkx\n\t"
        "mv " AI_SCALAR_GPR_SRC0_S ", %[lhs]\n\t"
        "mv " AI_SCALAR_GPR_SRC1_S ", %[rhs]\n\t"
        "aes64es " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 0(%[obs])\n\t"
        "aes64esm " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 8(%[obs])\n\t"
        "aes64ds " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 16(%[obs])\n\t"
        "aes64dsm " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 24(%[obs])\n\t"
        "aes64im " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 32(%[obs])\n\t"
        "aes64ks1i " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", 0\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 40(%[obs])\n\t"
        "aes64ks1i " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", 10\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 48(%[obs])\n\t"
        "aes64ks2 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 56(%[obs])\n\t"
        "sha256sum0 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 64(%[obs])\n\t"
        "sha256sum1 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 72(%[obs])\n\t"
        "sha256sig0 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 80(%[obs])\n\t"
        "sha256sig1 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 88(%[obs])\n\t"
        "sha512sum0 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 96(%[obs])\n\t"
        "sha512sum1 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 104(%[obs])\n\t"
        "sha512sig0 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 112(%[obs])\n\t"
        "sha512sig1 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 120(%[obs])\n\t"
        "sm3p0 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 128(%[obs])\n\t"
        "sm3p1 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 136(%[obs])\n\t"
        "sm4ed " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S ", 0\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 144(%[obs])\n\t"
        "sm4ks " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S ", 3\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 152(%[obs])\n\t"
        "xperm4 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 160(%[obs])\n\t"
        "xperm8 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 168(%[obs])\n\t"
        "pack " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 176(%[obs])\n\t"
        "packh " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 184(%[obs])\n\t"
        "packw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 192(%[obs])\n\t"
        "pack " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", zero\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 200(%[obs])\n\t"
        "packw " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", zero\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 208(%[obs])\n\t"
        "brev8 " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 216(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [obs] "r"(obs), [lhs] "r"(lhs), [rhs] "r"(rhs)
        : AI_SCALAR_GPR_SRC0_S, AI_SCALAR_GPR_SRC1_S,
          AI_SCALAR_GPR_DST_S, "memory");

    bool any_changed = false;
    for (unsigned i = 0; i < 24U; ++i) {
        any_changed = any_changed || (obs[i] != (0xfeedface00000000ULL | i));
    }

    TEST_ASSERT("K scalar opcode smoke executes AES/SHA/SM3/SM4/XPERM/PACK/BREV8 legal encodings and produces architectural writeback without exceptions",
        excpt.triggered == false &&
        any_changed
    );

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_WORD(0x31b31293);
    TEST_ASSERT("AES64KS1I rnum 0xB reserved encoding traps as illegal",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_END();
}

bool ai_arch_scalar_zicond_zimop_zcmop_zcb_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint64_t *obs = ai_scalar_obs;
    volatile uint8_t *mem = ai_scalar_mem;

    mem[0] = 0x80U;
    mem[1] = 0x7fU;
    mem[2] = 0x34U;
    mem[3] = 0x12U;

    asm volatile(
        ".option push\n\t"
        ".option arch,+zicond\n\t"
        "li " AI_SCALAR_GPR_SRC0_S ", 0x55aa\n\t"
        "czero.eqz " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", zero\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 0(%[obs])\n\t"
        "li " AI_SCALAR_GPR_SRC1_S ", 1\n\t"
        "czero.eqz " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 8(%[obs])\n\t"
        "czero.nez " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", " AI_SCALAR_GPR_SRC1_S "\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 16(%[obs])\n\t"
        "czero.nez " AI_SCALAR_GPR_DST_S ", " AI_SCALAR_GPR_SRC0_S ", zero\n\t"
        "sd " AI_SCALAR_GPR_DST_S ", 24(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [obs] "r"(obs)
        : AI_SCALAR_GPR_SRC0_S, AI_SCALAR_GPR_SRC1_S,
          AI_SCALAR_GPR_DST_S, "memory");

    TEST_ASSERT("Zicond czero.eqz/nez select zero or rs1 for both zero and nonzero rs2 cases",
        obs[0] == 0ULL &&
        obs[1] == 0x55aaULL &&
        obs[2] == 0ULL &&
        obs[3] == 0x55aaULL
    );

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_WORD(0x81c04073);
    AI_SCALAR_EXEC_WORD(0xcdf04073);
    AI_SCALAR_EXEC_WORD(0x82004073);
    AI_SCALAR_EXEC_WORD(0xce004073);
    TEST_ASSERT("Zimop MOP.R/MOP.RR legal encodings retire without visible exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_HALF(0x6081);
    AI_SCALAR_EXEC_HALF(0x6181);
    AI_SCALAR_EXEC_HALF(0x6281);
    TEST_ASSERT("Zcmop C.MOP odd encodings retire without visible exception",
        excpt.triggered == false
    );

    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb,+zba,+zbb\n\t"
        "mv " AI_SCALAR_CGPR_BASE_S ", %[mem]\n\t"
        "c.lbu " AI_SCALAR_CGPR_TMP0_S ", 0(" AI_SCALAR_CGPR_BASE_S ")\n\t"
        "sd " AI_SCALAR_CGPR_TMP0_S ", 32(%[obs])\n\t"
        "c.lh " AI_SCALAR_CGPR_TMP1_S ", 2(" AI_SCALAR_CGPR_BASE_S ")\n\t"
        "sd " AI_SCALAR_CGPR_TMP1_S ", 40(%[obs])\n\t"
        "c.lhu " AI_SCALAR_CGPR_TMP2_S ", 2(" AI_SCALAR_CGPR_BASE_S ")\n\t"
        "sd " AI_SCALAR_CGPR_TMP2_S ", 48(%[obs])\n\t"
        "li " AI_SCALAR_CGPR_TMP0_S ", 0xff\n\t"
        "c.sb " AI_SCALAR_CGPR_TMP0_S ", 1(" AI_SCALAR_CGPR_BASE_S ")\n\t"
        "li " AI_SCALAR_CGPR_TMP0_S ", 0x8001\n\t"
        "c.sh " AI_SCALAR_CGPR_TMP0_S ", 2(" AI_SCALAR_CGPR_BASE_S ")\n\t"
        "li " AI_SCALAR_CGPR_ACC_S ", 0xff80\n\t"
        "c.sext.b " AI_SCALAR_CGPR_ACC_S "\n\t"
        "sd " AI_SCALAR_CGPR_ACC_S ", 56(%[obs])\n\t"
        "li " AI_SCALAR_CGPR_ACC_S ", 0xff80\n\t"
        "c.zext.b " AI_SCALAR_CGPR_ACC_S "\n\t"
        "sd " AI_SCALAR_CGPR_ACC_S ", 64(%[obs])\n\t"
        "li " AI_SCALAR_CGPR_ACC_S ", 0x8001\n\t"
        "c.sext.h " AI_SCALAR_CGPR_ACC_S "\n\t"
        "sd " AI_SCALAR_CGPR_ACC_S ", 72(%[obs])\n\t"
        "li " AI_SCALAR_CGPR_ACC_S ", 0x8001\n\t"
        "c.zext.h " AI_SCALAR_CGPR_ACC_S "\n\t"
        "sd " AI_SCALAR_CGPR_ACC_S ", 80(%[obs])\n\t"
        "li " AI_SCALAR_CGPR_ACC_S ", 0xffffffff80000001\n\t"
        "c.zext.w " AI_SCALAR_CGPR_ACC_S "\n\t"
        "sd " AI_SCALAR_CGPR_ACC_S ", 88(%[obs])\n\t"
        "li " AI_SCALAR_CGPR_ACC_S ", 0x55\n\t"
        "c.not " AI_SCALAR_CGPR_ACC_S "\n\t"
        "sd " AI_SCALAR_CGPR_ACC_S ", 96(%[obs])\n\t"
        "li " AI_SCALAR_CGPR_ACC_S ", -7\n\t"
        "li " AI_SCALAR_CGPR_TMP0_S ", 6\n\t"
        "c.mul " AI_SCALAR_CGPR_ACC_S ", " AI_SCALAR_CGPR_TMP0_S "\n\t"
        "sd " AI_SCALAR_CGPR_ACC_S ", 104(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [obs] "r"(obs), [mem] "r"(mem)
        : AI_SCALAR_CGPR_BASE_S, AI_SCALAR_CGPR_TMP0_S,
          AI_SCALAR_CGPR_TMP1_S, AI_SCALAR_CGPR_TMP2_S,
          AI_SCALAR_CGPR_ACC_S, "memory");

    TEST_ASSERT("Zcb compressed load/store and compute encodings preserve byte/halfword signedness, zext/sext width, not, and multiply behavior",
        obs[4] == 0x80ULL &&
        obs[5] == 0x1234ULL &&
        obs[6] == 0x1234ULL &&
        mem[1] == 0xffU &&
        mem[2] == 0x01U &&
        mem[3] == 0x80U &&
        obs[7] == (uint64_t)-128LL &&
        obs[8] == 0x80ULL &&
        obs[9] == (uint64_t)-32767LL &&
        obs[10] == 0x8001ULL &&
        obs[11] == 0x80000001ULL &&
        obs[12] == ~0x55ULL &&
        obs[13] == (uint64_t)-42LL
    );

    TEST_END();
}

bool ai_arch_scalar_illegal_field_encoding_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_WORD(0x0203129b);
    TEST_ASSERT("SLLIW with imm[5]=1 is reserved on RV64 and raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_WORD(0x000090e7);
    TEST_ASSERT("JALR funct3 other than 000 raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_WORD(0x0020a063);
    TEST_ASSERT("branch funct3 reserved encoding raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_WORD(0x00007083);
    TEST_ASSERT("load funct3 reserved encoding raises illegal instruction before any load side effect",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_WORD(0x00104023);
    TEST_ASSERT("store funct3 reserved encoding raises illegal instruction before any store side effect",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_WORD(0x042081b3);
    TEST_ASSERT("OP invalid funct7/funct3 combination raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_SETUP_EXCEPT();
    AI_SCALAR_EXEC_WORD(0x0020a1bb);
    TEST_ASSERT("OP-32 invalid funct3 combination raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION
    );

    TEST_END();
}
