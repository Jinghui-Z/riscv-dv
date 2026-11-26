#include "ai_directed_random_stress_common.h"

#include <stddef.h>
#include <stdint.h>

#ifndef AI_IT_STRESS_SEED_COUNT
#define AI_IT_STRESS_SEED_COUNT 4U
#endif

#ifndef AI_IT_STRESS_SHARD_ID
#define AI_IT_STRESS_SHARD_ID 0ULL
#endif

#ifndef AI_IT_STRESS_MIN_OPS
#define AI_IT_STRESS_MIN_OPS 160U
#endif

#ifndef AI_IT_STRESS_MAX_OPS
#define AI_IT_STRESS_MAX_OPS 192U
#endif

#define AI_IT_STRESS_REQUIRED_SLOTS 18U

#if AI_IT_STRESS_SEED_COUNT == 0
#error "AI_IT_STRESS_SEED_COUNT must be greater than zero"
#endif

#if AI_IT_STRESS_MIN_OPS < AI_IT_STRESS_REQUIRED_SLOTS || \
    AI_IT_STRESS_MIN_OPS > AI_IT_STRESS_MAX_OPS
#error "AI_IT_STRESS_MIN_OPS must cover the 18 mandatory slots"
#endif

#define AI_IT_STREAM_PROFILE_ID       5U
#define AI_IT_STREAM_CACHE_LINE_BYTES 64U
#define AI_IT_STREAM_LINE_WORDS       8U
#define AI_IT_STREAM_MEM_LINES        128U
#define AI_IT_STREAM_VECTOR_BYTES     128U
#define AI_IT_STREAM_VECTOR_STORAGE   192U
#define AI_IT_STREAM_MSTATUS_VS_DIRTY (3ULL << 9U)

/*
 * The test body is a runtime-generated instruction stream.  The generator
 * emits legal words into executable memory and the CPU fetches those words
 * after fence.i.  The opcode classes below are only a legality/coverage
 * grammar; there is no opcode-to-asm execution table.
 */
#define AI_IT_RAW_MEM_WORDS           512U
#define AI_IT_RAW_MEM_LINES           (AI_IT_RAW_MEM_WORDS / 8U)
#define AI_IT_RAW_BASE_WORD           (AI_IT_RAW_MEM_WORDS / 2U)
#define AI_IT_RAW_MAX_WORDS           (AI_IT_STRESS_MAX_OPS * 3U + 128U)
#define AI_IT_RAW_A0                  10U
#define AI_IT_RAW_BASE                11U
#define AI_IT_RAW_AMO_BASE            12U
#define AI_IT_RAW_VECTOR_SRC         13U
#define AI_IT_RAW_VECTOR_DST         14U
#define AI_IT_RAW_AVL                15U
#define AI_IT_RAW_VECTOR_GROUPS       4U
#define AI_IT_RAW_VECTOR_GROUP_BYTES  AI_IT_STREAM_VECTOR_STORAGE

enum ai_it_raw_class {
    AI_IT_RAW_INIT = 0,
    AI_IT_RAW_VSTART,
    AI_IT_RAW_LUI,
    AI_IT_RAW_R_ALU,
    AI_IT_RAW_I_ALU,
    AI_IT_RAW_MUL,
    AI_IT_RAW_DIV,
    AI_IT_RAW_REM,
    AI_IT_RAW_LOAD,
    AI_IT_RAW_STORE,
    AI_IT_RAW_AMO_RMW,
    AI_IT_RAW_BRANCH,
    AI_IT_RAW_JAL,
    AI_IT_RAW_FENCE,
    AI_IT_RAW_FENCE_I,
    AI_IT_RAW_CBO_FLUSH,
    AI_IT_RAW_VSETVLI,
    AI_IT_RAW_VLE8,
    AI_IT_RAW_VSE8,
    AI_IT_RAW_VRGATHER_VX,
    AI_IT_RAW_VALU_VX,
    AI_IT_RAW_NOP,
    AI_IT_RAW_RET,
    AI_IT_RAW_CLASS_COUNT,
};

struct ai_it_raw_stream {
    unsigned word_count;
    unsigned body_start;
    unsigned body_slots;
    unsigned body_words;
    unsigned unique_word_count;
    uint64_t word_hash;
    uint64_t field_hash;
    uint64_t opcode_mask[2];
    uint64_t upper7_mask[2];
    unsigned funct3_mask;
    unsigned opcode_variants;
    unsigned funct3_variants;
    unsigned upper7_variants;
    uint64_t semantic_variant_mask;
    unsigned semantic_variants;
    uint32_t sample_words[4];
    unsigned sample_pcs[4];
    unsigned sample_count;
    unsigned generated_hist[AI_IT_RAW_CLASS_COUNT];
};

struct ai_it_raw_reference {
    bool valid;
    uint64_t result;
    unsigned stop_pc;
    uint32_t stop_word;
    unsigned executed_words;
    unsigned taken_branches;
    unsigned not_taken_branches;
    unsigned executed_hist[AI_IT_RAW_CLASS_COUNT];
};

enum ai_it_address_mode {
    AI_IT_ADDR_STREAM = 0,
    AI_IT_ADDR_RANDOM,
    AI_IT_ADDR_SET_CONFLICT_CANDIDATE,
    AI_IT_ADDR_HOTSPOT,
    AI_IT_ADDR_MODE_COUNT,
};

enum ai_it_dependency_mode {
    AI_IT_DEP_LONG_CHAIN = 0,
    AI_IT_DEP_ROUND_ROBIN,
    AI_IT_DEP_MIXED,
    AI_IT_DEP_MODE_COUNT,
};

enum ai_it_branch_mode {
    AI_IT_BRANCH_RANDOM = 0,
    AI_IT_BRANCH_ALTERNATING,
    AI_IT_BRANCH_LONG_RUN,
    AI_IT_BRANCH_BIASED,
    AI_IT_BRANCH_MODE_COUNT,
};

enum ai_it_cleanup_mode {
    AI_IT_CLEANUP_FENCE = 0,
    AI_IT_CLEANUP_CMO,
    AI_IT_CLEANUP_FENCE_AND_CMO,
    AI_IT_CLEANUP_MODE_COUNT,
};

static unsigned ai_it_branch_direction(unsigned branch_mode, unsigned slot,
    uint64_t random_value);

static const uint8_t ai_it_raw_register_pool[] = {
    5U, 6U, 7U, AI_IT_RAW_A0, 16U, 17U,
};

static volatile uint32_t ai_it_raw_code[AI_IT_RAW_MAX_WORDS]
    __attribute__((used, aligned(64), section(".text.ai_it_raw")));
static volatile uint64_t ai_it_raw_memory[AI_IT_RAW_MEM_WORDS]
    __attribute__((aligned(4096)));
static uint64_t ai_it_raw_expected_memory[AI_IT_RAW_MEM_WORDS];
static unsigned ai_it_raw_expected_last_write_pc[AI_IT_RAW_MEM_WORDS];
static uint32_t ai_it_raw_expected_last_write_word[AI_IT_RAW_MEM_WORDS];
static volatile uint8_t ai_it_raw_vector_src[AI_IT_STREAM_VECTOR_BYTES]
    __attribute__((aligned(64)));
static volatile uint8_t ai_it_raw_vector_dst[AI_IT_STREAM_VECTOR_STORAGE]
    __attribute__((aligned(64)));
static uint8_t ai_it_raw_expected_vector_src[AI_IT_STREAM_VECTOR_BYTES];
static uint8_t ai_it_raw_expected_vector_dst[AI_IT_STREAM_VECTOR_STORAGE];
static uint8_t ai_it_raw_expected_vregs[AI_IT_RAW_VECTOR_GROUPS]
    [AI_IT_RAW_VECTOR_GROUP_BYTES];

typedef uint64_t (*ai_it_raw_exec_fn)(uint64_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t);

static inline uint64_t ai_it_raw_sign_extend(uint64_t value, unsigned bits)
{
    const uint64_t mask = (1ULL << bits) - 1ULL;
    const uint64_t sign = 1ULL << (bits - 1U);

    value &= mask;
    return (value & sign) != 0ULL ? value | ~mask : value;
}

static inline uint32_t ai_it_raw_encode_r(unsigned funct7, unsigned rs2,
    unsigned rs1, unsigned funct3, unsigned rd)
{
    return ((uint32_t)funct7 << 25U) | ((uint32_t)rs2 << 20U) |
        ((uint32_t)rs1 << 15U) | ((uint32_t)funct3 << 12U) |
        ((uint32_t)rd << 7U) | 0x33U;
}

static inline uint32_t ai_it_raw_encode_i(int32_t immediate, unsigned rs1,
    unsigned funct3, unsigned rd, unsigned opcode)
{
    return ((uint32_t)immediate & 0xfffU) << 20U |
        ((uint32_t)rs1 << 15U) | ((uint32_t)funct3 << 12U) |
        ((uint32_t)rd << 7U) | opcode;
}

static inline uint32_t ai_it_raw_encode_s(unsigned immediate, unsigned rs2,
    unsigned rs1, unsigned funct3)
{
    return (((uint32_t)immediate & 0xfe0U) << 20U) |
        ((uint32_t)rs2 << 20U) | ((uint32_t)rs1 << 15U) |
        ((uint32_t)funct3 << 12U) |
        (((uint32_t)immediate & 0x1fU) << 7U) | 0x23U;
}

static inline uint32_t ai_it_raw_encode_b(int32_t immediate, unsigned rs2,
    unsigned rs1, unsigned funct3)
{
    const uint32_t value = (uint32_t)immediate;

    return (((value >> 12U) & 1U) << 31U) |
        (((value >> 5U) & 0x3fU) << 25U) |
        ((uint32_t)rs2 << 20U) | ((uint32_t)rs1 << 15U) |
        ((uint32_t)funct3 << 12U) |
        (((value >> 1U) & 0xfU) << 8U) |
        (((value >> 11U) & 1U) << 7U) | 0x63U;
}

static inline uint32_t ai_it_raw_encode_j(int32_t immediate, unsigned rd)
{
    const uint32_t value = (uint32_t)immediate;

    return (((value >> 20U) & 1U) << 31U) |
        (((value >> 1U) & 0x3ffU) << 21U) |
        (((value >> 11U) & 1U) << 20U) |
        (((value >> 12U) & 0xffU) << 12U) |
        ((uint32_t)rd << 7U) | 0x6fU;
}

static inline uint32_t ai_it_raw_encode_amo(unsigned funct5, unsigned rd,
    unsigned rs1, unsigned rs2, unsigned aqrl)
{
    return ((uint32_t)funct5 << 27U) | ((uint32_t)aqrl << 25U) |
        ((uint32_t)rs2 << 20U) |
        ((uint32_t)rs1 << 15U) | (3U << 12U) |
        ((uint32_t)rd << 7U) | 0x2fU;
}

static inline uint32_t ai_it_raw_encode_u(unsigned immediate, unsigned rd,
    unsigned opcode)
{
    return ((uint32_t)(immediate & 0xfffffU) << 12U) |
        ((uint32_t)rd << 7U) | opcode;
}

static inline uint32_t ai_it_raw_encode_vsetvli(unsigned rd, unsigned rs1)
{
    /* e8,m8,tu,mu: a legal vtypei that keeps the reference model byte-wide. */
    return (3U << 20U) | ((uint32_t)rs1 << 15U) | (7U << 12U) |
        ((uint32_t)rd << 7U) | 0x57U;
}

static inline uint32_t ai_it_raw_encode_vload(unsigned vd, unsigned rs1)
{
    return (1U << 25U) | ((uint32_t)rs1 << 15U) |
        ((uint32_t)vd << 7U) | 0x07U;
}

static inline uint32_t ai_it_raw_encode_vstore(unsigned vs3, unsigned rs1)
{
    return (1U << 25U) | ((uint32_t)rs1 << 15U) |
        ((uint32_t)vs3 << 7U) | 0x27U;
}

static inline uint32_t ai_it_raw_encode_vvx(unsigned funct6, unsigned vd,
    unsigned vs2, unsigned rs1)
{
    return ((uint32_t)funct6 << 26U) | (1U << 25U) |
        ((uint32_t)vs2 << 20U) | ((uint32_t)rs1 << 15U) |
        (4U << 12U) | ((uint32_t)vd << 7U) | 0x57U;
}

static inline uint32_t ai_it_raw_encode_cbo_flush(unsigned rs1)
{
    /* Zicbom encodes cbo.flush as imm=2, funct3=2, rd=0. */
    return (2U << 20U) | ((uint32_t)rs1 << 15U) | (2U << 12U) | 0x0fU;
}

static inline int32_t ai_it_raw_decode_i(uint32_t word)
{
    return (int32_t)(int64_t)ai_it_raw_sign_extend(word >> 20U, 12U);
}

static inline int32_t ai_it_raw_decode_s(uint32_t word)
{
    const uint32_t immediate = ((word >> 25U) << 5U) |
        ((word >> 7U) & 0x1fU);

    return (int32_t)(int64_t)ai_it_raw_sign_extend(immediate, 12U);
}

static inline int32_t ai_it_raw_decode_b(uint32_t word)
{
    const uint32_t immediate = (((word >> 31U) & 1U) << 12U) |
        (((word >> 7U) & 1U) << 11U) |
        (((word >> 25U) & 0x3fU) << 5U) |
        (((word >> 8U) & 0xfU) << 1U);

    return (int32_t)(int64_t)ai_it_raw_sign_extend(immediate, 13U);
}

static inline int32_t ai_it_raw_decode_j(uint32_t word)
{
    const uint32_t immediate = (((word >> 31U) & 1U) << 20U) |
        (((word >> 12U) & 0xffU) << 12U) |
        (((word >> 20U) & 1U) << 11U) |
        (((word >> 21U) & 0x3ffU) << 1U);

    return (int32_t)(int64_t)ai_it_raw_sign_extend(immediate, 21U);
}

static inline uint64_t ai_it_raw_sra(uint64_t value, unsigned shift)
{
    if (shift == 0U) {
        return value;
    }
    uint64_t result = value >> shift;

    if ((value & (1ULL << 63U)) != 0ULL) {
        result |= ~0ULL << (64U - shift);
    }
    return result;
}

static inline uint64_t ai_it_raw_div_signed(uint64_t lhs, uint64_t rhs)
{
    const int64_t dividend = (int64_t)lhs;
    const int64_t divisor = (int64_t)rhs;
    const int64_t minimum = (int64_t)(1ULL << 63U);

    if (divisor == 0) {
        return ~0ULL;
    }
    if (dividend == minimum && divisor == -1) {
        return (uint64_t)minimum;
    }
    return (uint64_t)(dividend / divisor);
}

static inline uint64_t ai_it_raw_rem_signed(uint64_t lhs, uint64_t rhs)
{
    const int64_t dividend = (int64_t)lhs;
    const int64_t divisor = (int64_t)rhs;
    const int64_t minimum = (int64_t)(1ULL << 63U);

    if (divisor == 0) {
        return lhs;
    }
    if (dividend == minimum && divisor == -1) {
        return 0ULL;
    }
    return (uint64_t)(dividend % divisor);
}

static inline void ai_it_raw_write_reg(uint64_t regs[32], unsigned rd,
    uint64_t value)
{
    if (rd != 0U) {
        regs[rd] = value;
    }
}

static uint64_t ai_it_raw_semantic_variant(uint32_t word,
    enum ai_it_raw_class class)
{
    const unsigned funct3 = (word >> 12U) & 7U;
    const unsigned funct7 = (word >> 25U) & 0x7fU;

    switch (class) {
    case AI_IT_RAW_R_ALU:
        if (funct3 == 0U) return 1ULL << (funct7 == 0x20U ? 1U : 0U);
        if (funct3 == 1U) return 1ULL << 2U;
        if (funct3 == 2U) return 1ULL << 3U;
        if (funct3 == 3U) return 1ULL << 4U;
        if (funct3 == 4U) return 1ULL << 5U;
        if (funct3 == 5U) return 1ULL << (funct7 == 0x20U ? 7U : 6U);
        if (funct3 == 6U) return 1ULL << 8U;
        if (funct3 == 7U) return 1ULL << 9U;
        break;
    case AI_IT_RAW_MUL:
        return 1ULL << 10U;
    case AI_IT_RAW_DIV:
        return 1ULL << (funct3 == 4U ? 11U : 12U);
    case AI_IT_RAW_REM:
        return 1ULL << (funct3 == 6U ? 13U : 14U);
    case AI_IT_RAW_I_ALU:
        if (funct3 == 0U) return 1ULL << 15U;
        if (funct3 == 4U) return 1ULL << 16U;
        if (funct3 == 6U) return 1ULL << 17U;
        if (funct3 == 7U) return 1ULL << 18U;
        if (funct3 == 2U) return 1ULL << 19U;
        if (funct3 == 3U) return 1ULL << 20U;
        if (funct3 == 1U) return 1ULL << 21U;
        if (funct3 == 5U) {
            return 1ULL << (((word >> 30U) & 1U) != 0U ? 23U : 22U);
        }
        break;
    case AI_IT_RAW_AMO_RMW: {
        const unsigned funct5 = (word >> 27U) & 0x1fU;

        if (funct5 == 0U) return 1ULL << 24U;
        if (funct5 == 1U) return 1ULL << 25U;
        if (funct5 == 4U) return 1ULL << 26U;
        if (funct5 == 12U) return 1ULL << 27U;
        if (funct5 == 8U) return 1ULL << 28U;
        break;
    }
    case AI_IT_RAW_BRANCH:
        if (funct3 <= 1U) return 1ULL << (29U + funct3);
        if (funct3 >= 4U) return 1ULL << (27U + funct3);
        break;
    case AI_IT_RAW_VRGATHER_VX:
        return 1ULL << 35U;
    case AI_IT_RAW_VALU_VX: {
        const unsigned funct6 = (word >> 26U) & 0x3fU;

        if (funct6 == 0x00U) return 1ULL << 36U;
        if (funct6 == 0x02U) return 1ULL << 37U;
        if (funct6 == 0x09U) return 1ULL << 38U;
        if (funct6 == 0x0aU) return 1ULL << 39U;
        if (funct6 == 0x0bU) return 1ULL << 40U;
        break;
    }
    default:
        break;
    }
    return 0ULL;
}

static bool ai_it_raw_emit(struct ai_it_raw_stream *stream, uint32_t word,
    enum ai_it_raw_class class)
{
    const unsigned opcode = word & 0x7fU;
    const unsigned funct3 = (word >> 12U) & 7U;
    const unsigned funct7 = (word >> 25U) & 0x7fU;
    if (stream->word_count >= AI_IT_RAW_MAX_WORDS) {
        return false;
    }
    const unsigned index = stream->word_count++;
    bool unique = true;

    ai_it_raw_code[index] = word;
    for (unsigned previous = 0U; previous < index; ++previous) {
        if (ai_it_raw_code[previous] == word) {
            unique = false;
            break;
        }
    }
    if (unique) {
        ++stream->unique_word_count;
    }
    if (opcode < 64U) {
        const uint64_t bit = 1ULL << opcode;

        if ((stream->opcode_mask[0] & bit) == 0ULL) {
            stream->opcode_mask[0] |= bit;
            ++stream->opcode_variants;
        }
    } else {
        const uint64_t bit = 1ULL << (opcode - 64U);

        if ((stream->opcode_mask[1] & bit) == 0ULL) {
            stream->opcode_mask[1] |= bit;
            ++stream->opcode_variants;
        }
    }
    {
        const unsigned bit = 1U << funct3;

        if ((stream->funct3_mask & bit) == 0U) {
            stream->funct3_mask |= bit;
            ++stream->funct3_variants;
        }
    }
    if (funct7 < 64U) {
        const uint64_t bit = 1ULL << funct7;

        if ((stream->upper7_mask[0] & bit) == 0ULL) {
            stream->upper7_mask[0] |= bit;
            ++stream->upper7_variants;
        }
    } else {
        const uint64_t bit = 1ULL << (funct7 - 64U);

        if ((stream->upper7_mask[1] & bit) == 0ULL) {
            stream->upper7_mask[1] |= bit;
            ++stream->upper7_variants;
        }
    }
    {
        const uint64_t variant = ai_it_raw_semantic_variant(word, class);

        if (variant != 0ULL &&
            (stream->semantic_variant_mask & variant) == 0ULL) {
            stream->semantic_variant_mask |= variant;
            ++stream->semantic_variants;
        }
    }
    ++stream->generated_hist[class];
    stream->word_hash = ai_stress_signature_fold(stream->word_hash,
        ((uint64_t)index << 32U) | word);
    stream->field_hash = ai_stress_signature_fold(stream->field_hash,
        ((uint64_t)(word & 0x7fU) << 56U) |
        ((uint64_t)((word >> 7U) & 0x1fU) << 48U) |
        ((uint64_t)((word >> 12U) & 0x7U) << 40U) |
        ((uint64_t)((word >> 15U) & 0x1fU) << 32U) |
        ((uint64_t)((word >> 20U) & 0x1fU) << 24U) |
        ((uint64_t)((word >> 25U) & 0x7fU) << 16U) |
        (uint64_t)(word & 0xffffU));
    return true;
}

static unsigned ai_it_raw_register_count(void)
{
    return (unsigned)(sizeof(ai_it_raw_register_pool) /
        sizeof(ai_it_raw_register_pool[0]));
}

static unsigned ai_it_raw_pick_dest(struct ai_stress_rng *rng,
    unsigned dependency_mode, unsigned slot)
{
    const unsigned count = ai_it_raw_register_count();
    unsigned index;

    if (dependency_mode == AI_IT_DEP_LONG_CHAIN &&
        (slot & 3U) != 0U) {
        return AI_IT_RAW_A0;
    }
    if (dependency_mode == AI_IT_DEP_ROUND_ROBIN) {
        index = slot % count;
    } else {
        index = (unsigned)(ai_stress_rng_next(rng) % count);
    }
    return ai_it_raw_register_pool[index];
}

static unsigned ai_it_raw_pick_source(struct ai_stress_rng *rng,
    unsigned dependency_mode, unsigned slot)
{
    if ((ai_stress_rng_next(rng) & 7ULL) == 0ULL) {
        return 0U;
    }
    if (dependency_mode == AI_IT_DEP_LONG_CHAIN &&
        (slot & 1U) == 0U) {
        return AI_IT_RAW_A0;
    }
    return ai_it_raw_pick_dest(rng, dependency_mode, slot + 1U);
}

static int32_t ai_it_raw_random_i12(struct ai_stress_rng *rng)
{
    return (int32_t)ai_it_raw_sign_extend(ai_stress_rng_next(rng), 12U);
}

static int32_t ai_it_raw_memory_offset(unsigned address_mode, unsigned slot,
    unsigned stream_stride, unsigned address_anchor, uint64_t random_value)
{
    const unsigned base = address_anchor % AI_IT_RAW_MEM_LINES;
    unsigned line;
    unsigned word = (unsigned)((random_value >> 8U) %
        AI_IT_STREAM_LINE_WORDS);

    switch (address_mode) {
    case AI_IT_ADDR_STREAM:
        line = (base + slot * stream_stride) % AI_IT_RAW_MEM_LINES;
        break;
    case AI_IT_ADDR_SET_CONFLICT_CANDIDATE:
        line = (base % (AI_IT_RAW_MEM_LINES / 2U)) +
            (slot & 1U) * (AI_IT_RAW_MEM_LINES / 2U);
        break;
    case AI_IT_ADDR_HOTSPOT:
        line = (base + (unsigned)(random_value & 3ULL)) %
            AI_IT_RAW_MEM_LINES;
        break;
    default:
        line = (unsigned)(random_value % AI_IT_RAW_MEM_LINES);
        break;
    }
    return (int32_t)(line * AI_IT_STREAM_CACHE_LINE_BYTES +
        word * sizeof(uint64_t)) -
        (int32_t)(AI_IT_RAW_BASE_WORD * sizeof(uint64_t));
}

static enum ai_it_raw_class ai_it_raw_weighted_class(
    struct ai_stress_rng *rng)
{
    const unsigned selector = (unsigned)(ai_stress_rng_next(rng) % 100ULL);

    if (selector < 16U) return AI_IT_RAW_R_ALU;
    if (selector < 31U) return AI_IT_RAW_I_ALU;
    if (selector < 35U) return AI_IT_RAW_LUI;
    if (selector < 41U) return AI_IT_RAW_MUL;
    if (selector < 47U) return AI_IT_RAW_DIV;
    if (selector < 53U) return AI_IT_RAW_REM;
    if (selector < 64U) return AI_IT_RAW_LOAD;
    if (selector < 74U) return AI_IT_RAW_STORE;
    if (selector < 77U) return AI_IT_RAW_AMO_RMW;
    if (selector < 84U) return AI_IT_RAW_BRANCH;
    if (selector < 87U) return AI_IT_RAW_JAL;
    if (selector < 89U) return AI_IT_RAW_FENCE;
    if (selector < 90U) return AI_IT_RAW_FENCE_I;
    if (selector < 92U) return AI_IT_RAW_CBO_FLUSH;
    if (selector < 93U) return AI_IT_RAW_VSETVLI;
    if (selector < 95U) return AI_IT_RAW_VLE8;
    if (selector < 97U) return AI_IT_RAW_VSE8;
    if (selector < 99U) return AI_IT_RAW_VRGATHER_VX;
    return AI_IT_RAW_VALU_VX;
}

static void ai_it_raw_shuffle_u32(struct ai_stress_rng *rng,
    unsigned positions[AI_IT_RAW_MAX_WORDS], unsigned count)
{
    for (unsigned index = count; index > 1U; --index) {
        const unsigned other = (unsigned)(ai_stress_rng_next(rng) % index);
        const unsigned temp = positions[index - 1U];

        positions[index - 1U] = positions[other];
        positions[other] = temp;
    }
}

static void ai_it_raw_shuffle_classes(struct ai_stress_rng *rng,
    enum ai_it_raw_class classes[], unsigned count)
{
    for (unsigned index = count; index > 1U; --index) {
        const unsigned other = (unsigned)(ai_stress_rng_next(rng) % index);
        const enum ai_it_raw_class temp = classes[index - 1U];

        classes[index - 1U] = classes[other];
        classes[other] = temp;
    }
}

/*
 * Generate the whole body first, then repair coverage at random positions.
 * The repair is a constraint on the distribution, not a fixed instruction
 * sequence: the position and the order of the required classes both vary
 * with the seed.
 */
static void ai_it_raw_plan_classes(struct ai_stress_rng *rng,
    unsigned body_count, unsigned cleanup_mode,
    enum ai_it_raw_class planned[AI_IT_RAW_MAX_WORDS])
{
    static const enum ai_it_raw_class common_required[] = {
        AI_IT_RAW_LUI, AI_IT_RAW_R_ALU, AI_IT_RAW_I_ALU,
        AI_IT_RAW_MUL, AI_IT_RAW_DIV, AI_IT_RAW_REM,
        AI_IT_RAW_LOAD, AI_IT_RAW_STORE, AI_IT_RAW_AMO_RMW,
        AI_IT_RAW_BRANCH, AI_IT_RAW_BRANCH, AI_IT_RAW_VLE8, AI_IT_RAW_VSE8,
        AI_IT_RAW_VRGATHER_VX, AI_IT_RAW_VALU_VX, AI_IT_RAW_VSETVLI,
    };
    enum ai_it_raw_class required[sizeof(common_required) /
        sizeof(common_required[0]) + 2U];
    enum ai_it_raw_class shuffled_required[sizeof(required) /
        sizeof(required[0])];
    unsigned required_count = (unsigned)(sizeof(common_required) /
        sizeof(common_required[0]));
    unsigned positions[AI_IT_RAW_MAX_WORDS];

    for (unsigned index = 0U; index < required_count; ++index) {
        required[index] = common_required[index];
    }
    if (cleanup_mode != AI_IT_CLEANUP_CMO) {
        required[required_count++] = AI_IT_RAW_FENCE;
    }
    if (cleanup_mode != AI_IT_CLEANUP_FENCE) {
        required[required_count++] = AI_IT_RAW_CBO_FLUSH;
    }
    for (unsigned slot = 0U; slot < body_count; ++slot) {
        planned[slot] = ai_it_raw_weighted_class(rng);
        if (cleanup_mode == AI_IT_CLEANUP_FENCE &&
            planned[slot] == AI_IT_RAW_CBO_FLUSH) {
            planned[slot] = AI_IT_RAW_FENCE;
        } else if (cleanup_mode == AI_IT_CLEANUP_CMO &&
                   planned[slot] == AI_IT_RAW_FENCE) {
            planned[slot] = AI_IT_RAW_CBO_FLUSH;
        }
        positions[slot] = slot;
    }
    for (unsigned index = 0U; index < required_count; ++index) {
        shuffled_required[index] = required[index];
    }
    ai_it_raw_shuffle_u32(rng, positions, body_count);
    ai_it_raw_shuffle_classes(rng, shuffled_required, required_count);
    for (unsigned index = 0U; index < required_count; ++index) {
        planned[positions[index]] = shuffled_required[index];
    }
}

static bool ai_it_raw_emit_control_filler(struct ai_stress_rng *rng,
    struct ai_it_raw_stream *stream, unsigned dependency_mode, unsigned slot)
{
    static const unsigned funct3s[] = { 0U, 4U, 6U, 7U, 2U, 3U };
    const unsigned rs1 = ai_it_raw_pick_source(rng, dependency_mode, slot);
    const unsigned funct3 = funct3s[ai_stress_rng_next(rng) %
        (sizeof(funct3s) / sizeof(funct3s[0]))];
    const int32_t immediate = ai_it_raw_random_i12(rng);

    return ai_it_raw_emit(stream,
        ai_it_raw_encode_i(immediate, rs1, funct3, 0U, 0x13U),
        AI_IT_RAW_I_ALU);
}

static bool ai_it_raw_build_stream(struct ai_stress_rng *rng,
    unsigned body_count, unsigned address_mode, unsigned stream_stride,
    unsigned address_anchor, unsigned dependency_mode, unsigned branch_mode,
    unsigned cleanup_mode, size_t vl, struct ai_it_raw_stream *stream)
{
    uint8_t init_regs[sizeof(ai_it_raw_register_pool)];
    unsigned init_count = 0U;
    enum ai_it_raw_class planned[AI_IT_RAW_MAX_WORDS];
    unsigned branch_ordinal = 0U;

    *stream = (struct ai_it_raw_stream){
        .word_hash = 0x243f6a8885a308d3ULL,
        .field_hash = 0x13198a2e03707344ULL,
        .body_slots = body_count,
    };
    for (unsigned index = 0U; index < AI_IT_RAW_MAX_WORDS; ++index) {
        ai_it_raw_code[index] = 0x00000013U;
    }
    for (unsigned index = 0U;
         index < sizeof(ai_it_raw_register_pool) /
             sizeof(ai_it_raw_register_pool[0]); ++index) {
        if (ai_it_raw_register_pool[index] != AI_IT_RAW_A0) {
            init_regs[init_count++] = ai_it_raw_register_pool[index];
        }
    }
    for (unsigned index = init_count; index > 1U; --index) {
        const unsigned other = (unsigned)(ai_stress_rng_next(rng) % index);
        const uint8_t temp = init_regs[index - 1U];

        init_regs[index - 1U] = init_regs[other];
        init_regs[other] = temp;
    }
    for (unsigned index = 0U; index < init_count; ++index) {
        if (!ai_it_raw_emit(stream,
                ai_it_raw_encode_i(ai_it_raw_random_i12(rng), 0U, 0U,
                    init_regs[index], 0x13U), AI_IT_RAW_INIT)) {
            return false;
        }
    }

    /* Establish deterministic vector state using raw words as well. */
    if (!ai_it_raw_emit(stream,
            ai_it_raw_encode_vsetvli(ai_it_raw_pick_dest(rng,
                AI_IT_DEP_ROUND_ROBIN, 0U), AI_IT_RAW_AVL),
            AI_IT_RAW_INIT) ||
        !ai_it_raw_emit(stream, 0x00801073U, AI_IT_RAW_INIT)) {
        return false;
    }
    unsigned vector_groups[AI_IT_RAW_VECTOR_GROUPS] = { 0U, 1U, 2U, 3U };
    ai_it_raw_shuffle_u32(rng, vector_groups, AI_IT_RAW_VECTOR_GROUPS);
    for (unsigned index = 0U; index < AI_IT_RAW_VECTOR_GROUPS; ++index) {
        const unsigned base = vector_groups[index] * 8U;
        const unsigned address_reg = AI_IT_RAW_VECTOR_SRC;

        if (!ai_it_raw_emit(stream, ai_it_raw_encode_vload(base, address_reg),
                AI_IT_RAW_INIT)) {
            return false;
        }
    }

    stream->body_start = stream->word_count;
    ai_it_raw_plan_classes(rng, body_count, cleanup_mode, planned);
    for (unsigned slot = 0U; slot < body_count; ++slot) {
        const enum ai_it_raw_class class = planned[slot];
        uint32_t word = 0U;

        switch (class) {
        case AI_IT_RAW_LUI: {
            const unsigned immediate = (unsigned)ai_stress_rng_next(rng);
            const unsigned rd = ai_it_raw_pick_dest(rng, dependency_mode,
                slot);

            word = ai_it_raw_encode_u(immediate, rd, 0x37U);
            break;
        }
        case AI_IT_RAW_R_ALU: {
            const unsigned rd = ai_it_raw_pick_dest(rng, dependency_mode, slot);
            const unsigned rs1 = ai_it_raw_pick_source(rng, dependency_mode,
                slot);
            const unsigned rs2 = ai_it_raw_pick_source(rng, dependency_mode,
                slot + 1U);
            const unsigned variant = (unsigned)(ai_stress_rng_next(rng) %
                10ULL);
            unsigned funct3 = 0U;
            unsigned funct7 = 0U;

            switch (variant) {
            case 0U: funct3 = 0U; break; /* add */
            case 1U: funct3 = 0U; funct7 = 0x20U; break; /* sub */
            case 2U: funct3 = 1U; break; /* sll */
            case 3U: funct3 = 2U; break; /* slt */
            case 4U: funct3 = 3U; break; /* sltu */
            case 5U: funct3 = 4U; break; /* xor */
            case 6U: funct3 = 5U; break; /* srl */
            case 7U: funct3 = 5U; funct7 = 0x20U; break; /* sra */
            case 8U: funct3 = 6U; break; /* or */
            default: funct3 = 7U; break; /* and */
            }
            word = ai_it_raw_encode_r(funct7, rs2, rs1, funct3, rd);
            break;
        }
        case AI_IT_RAW_I_ALU: {
            const unsigned rd = ai_it_raw_pick_dest(rng, dependency_mode, slot);
            const unsigned rs1 = ai_it_raw_pick_source(rng, dependency_mode,
                slot);
            const unsigned variant = (unsigned)(ai_stress_rng_next(rng) %
                9ULL);
            unsigned funct3;
            int32_t immediate;

            if (variant < 6U) {
                static const unsigned funct3s[] = {
                    0U, 4U, 6U, 7U, 2U, 3U
                };

                funct3 = funct3s[variant];
                immediate = ai_it_raw_random_i12(rng);
            } else {
                const unsigned shift = (unsigned)(ai_stress_rng_next(rng) &
                    63ULL);

                funct3 = variant == 6U ? 1U : 5U;
                immediate = variant == 8U ? (int32_t)(0x400U | shift) :
                    (int32_t)shift;
            }
            word = ai_it_raw_encode_i(immediate, rs1, funct3, rd, 0x13U);
            break;
        }
        case AI_IT_RAW_MUL: {
            const unsigned rd = ai_it_raw_pick_dest(rng, dependency_mode,
                slot);
            const unsigned rs1 = ai_it_raw_pick_source(rng, dependency_mode,
                slot);
            const unsigned rs2 = ai_it_raw_pick_source(rng, dependency_mode,
                slot + 1U);

            word = ai_it_raw_encode_r(1U, rs2, rs1, 0U, rd);
            break;
        }
        case AI_IT_RAW_DIV: {
            const unsigned rd = ai_it_raw_pick_dest(rng, dependency_mode,
                slot);
            const unsigned rs1 = ai_it_raw_pick_source(rng, dependency_mode,
                slot);
            const unsigned rs2 = ai_it_raw_pick_source(rng, dependency_mode,
                slot + 1U);
            const unsigned funct3 = 4U +
                (unsigned)(ai_stress_rng_next(rng) & 1ULL);

            word = ai_it_raw_encode_r(1U, rs2, rs1, funct3, rd);
            break;
        }
        case AI_IT_RAW_REM: {
            const unsigned rd = ai_it_raw_pick_dest(rng, dependency_mode,
                slot);
            const unsigned rs1 = ai_it_raw_pick_source(rng, dependency_mode,
                slot);
            const unsigned rs2 = ai_it_raw_pick_source(rng, dependency_mode,
                slot + 1U);
            const unsigned funct3 = 6U +
                (unsigned)(ai_stress_rng_next(rng) & 1ULL);

            word = ai_it_raw_encode_r(1U, rs2, rs1, funct3, rd);
            break;
        }
        case AI_IT_RAW_LOAD: {
            const int32_t offset = ai_it_raw_memory_offset(address_mode,
                slot, stream_stride, address_anchor,
                ai_stress_rng_next(rng));

            word = ai_it_raw_encode_i(offset, AI_IT_RAW_BASE, 3U,
                ai_it_raw_pick_dest(rng, dependency_mode, slot), 0x03U);
            break;
        }
        case AI_IT_RAW_STORE: {
            const int32_t offset = ai_it_raw_memory_offset(address_mode,
                slot, stream_stride, address_anchor,
                ai_stress_rng_next(rng));

            word = ai_it_raw_encode_s((unsigned)offset,
                ai_it_raw_pick_source(rng, dependency_mode, slot),
                AI_IT_RAW_BASE, 3U);
            break;
        }
        case AI_IT_RAW_AMO_RMW: {
            static const unsigned funct5s[] = { 0U, 1U, 4U, 12U, 8U };
            const unsigned address_reg = (ai_stress_rng_next(rng) & 1ULL) !=
                0ULL ? AI_IT_RAW_AMO_BASE : AI_IT_RAW_BASE;
            const unsigned funct5 = funct5s[ai_stress_rng_next(rng) %
                (sizeof(funct5s) / sizeof(funct5s[0]))];
            const unsigned rd = ai_it_raw_pick_dest(rng, dependency_mode,
                slot);
            const unsigned rs2 = ai_it_raw_pick_source(rng, dependency_mode,
                slot);
            const unsigned aqrl =
                (unsigned)(ai_stress_rng_next(rng) & 3ULL);

            word = ai_it_raw_encode_amo(funct5, rd, address_reg, rs2, aqrl);
            break;
        }
        case AI_IT_RAW_BRANCH: {
            const bool forced = branch_ordinal < 2U;
            unsigned rs1;
            unsigned rs2;
            unsigned funct3;

            if (forced) {
                rs1 = 0U;
                rs2 = 0U;
                funct3 = branch_ordinal == 0U ? 0U : 1U;
            } else if ((ai_stress_rng_next(rng) & 3ULL) == 0ULL) {
                const unsigned direction = ai_it_branch_direction(branch_mode,
                    slot, ai_stress_rng_next(rng));

                rs1 = 0U;
                rs2 = 0U;
                funct3 = direction != 0U ? 0U : 1U;
            } else {
                static const unsigned funct3s[] = { 0U, 1U, 4U, 5U, 6U, 7U };

                rs1 = ai_it_raw_pick_source(rng, dependency_mode, slot);
                rs2 = ai_it_raw_pick_source(rng, dependency_mode, slot + 1U);
                funct3 = funct3s[ai_stress_rng_next(rng) %
                    (sizeof(funct3s) / sizeof(funct3s[0]))];
            }
            /* Side-effect-free random I-type fillers make the skipped words
             * vary while both outcomes still converge on the next operation. */
            const unsigned span_words = 1U +
                (unsigned)(ai_stress_rng_next(rng) % 3ULL);

            word = ai_it_raw_encode_b((int32_t)(span_words * 4U), rs2, rs1,
                funct3);
            ++branch_ordinal;
            if (!ai_it_raw_emit(stream, word, AI_IT_RAW_BRANCH)) {
                return false;
            }
            for (unsigned filler = 1U; filler < span_words; ++filler) {
                if (!ai_it_raw_emit_control_filler(rng, stream,
                        dependency_mode, slot + filler)) {
                    return false;
                }
            }
            continue;
        }
        case AI_IT_RAW_JAL: {
            const unsigned span_words = 1U +
                (unsigned)(ai_stress_rng_next(rng) % 3ULL);

            word = ai_it_raw_encode_j((int32_t)(span_words * 4U), 0U);
            if (!ai_it_raw_emit(stream, word, AI_IT_RAW_JAL)) {
                return false;
            }
            for (unsigned filler = 1U; filler < span_words; ++filler) {
                if (!ai_it_raw_emit_control_filler(rng, stream,
                        dependency_mode, slot + filler)) {
                    return false;
                }
            }
            continue;
        }
        case AI_IT_RAW_FENCE: {
            const unsigned pred = 1U +
                (unsigned)(ai_stress_rng_next(rng) % 15ULL);
            const unsigned succ = 1U +
                (unsigned)(ai_stress_rng_next(rng) % 15ULL);

            word = ai_it_raw_encode_i((int32_t)((pred << 4U) | succ),
                0U, 0U, 0U, 0x0fU);
            break;
        }
        case AI_IT_RAW_FENCE_I:
            word = 0x0000100fU;
            break;
        case AI_IT_RAW_CBO_FLUSH:
            word = ai_it_raw_encode_cbo_flush(
                (ai_stress_rng_next(rng) & 1ULL) != 0ULL ?
                    AI_IT_RAW_BASE : AI_IT_RAW_AMO_BASE);
            break;
        case AI_IT_RAW_VSETVLI:
            word = ai_it_raw_encode_vsetvli(
                ai_it_raw_pick_dest(rng, dependency_mode, slot),
                AI_IT_RAW_AVL);
            break;
        case AI_IT_RAW_VLE8: {
            const unsigned base = (unsigned)(ai_stress_rng_next(rng) %
                AI_IT_RAW_VECTOR_GROUPS) * 8U;
            const unsigned address_reg = AI_IT_RAW_VECTOR_SRC;

            word = ai_it_raw_encode_vload(base, address_reg);
            break;
        }
        case AI_IT_RAW_VSE8: {
            const unsigned base = (unsigned)(ai_stress_rng_next(rng) %
                AI_IT_RAW_VECTOR_GROUPS) * 8U;
            const unsigned address_reg = AI_IT_RAW_VECTOR_DST;

            word = ai_it_raw_encode_vstore(base, address_reg);
            break;
        }
        case AI_IT_RAW_VRGATHER_VX: {
            const unsigned index_reg = ai_it_raw_pick_dest(rng,
                dependency_mode, slot);
            const unsigned source_group = (unsigned)(ai_stress_rng_next(rng) %
                AI_IT_RAW_VECTOR_GROUPS);
            const unsigned base = source_group * 8U;
            const unsigned destination =
                ((source_group + 1U + (unsigned)(ai_stress_rng_next(rng) %
                    (AI_IT_RAW_VECTOR_GROUPS - 1U))) %
                    AI_IT_RAW_VECTOR_GROUPS) * 8U;
            const int32_t index = (int32_t)(ai_stress_rng_next(rng) %
                (vl != 0U ? vl : 1U));

            if (!ai_it_raw_emit(stream,
                    ai_it_raw_encode_i(index, 0U, 0U, index_reg, 0x13U),
                    AI_IT_RAW_I_ALU) ||
                !ai_it_raw_emit(stream, ai_it_raw_encode_vvx(0x0cU,
                    destination, base, index_reg), AI_IT_RAW_VRGATHER_VX)) {
                return false;
            }
            continue;
        }
        case AI_IT_RAW_VALU_VX: {
            static const unsigned funct6s[] = {
                0x00U, 0x02U, 0x09U, 0x0aU, 0x0bU
            };
            const unsigned source_group = (unsigned)(ai_stress_rng_next(rng) %
                AI_IT_RAW_VECTOR_GROUPS);
            const unsigned base = source_group * 8U;
            const unsigned destination =
                ((source_group + 1U + (unsigned)(ai_stress_rng_next(rng) %
                    (AI_IT_RAW_VECTOR_GROUPS - 1U))) %
                    AI_IT_RAW_VECTOR_GROUPS) * 8U;
            const unsigned scalar = ai_it_raw_pick_source(rng,
                dependency_mode, slot);
            const unsigned funct6 = funct6s[ai_stress_rng_next(rng) %
                (sizeof(funct6s) / sizeof(funct6s[0]))];

            word = ai_it_raw_encode_vvx(funct6, destination, base, scalar);
            break;
        }
        default:
            return false;
        }
        if (!ai_it_raw_emit(stream, word, class)) {
            return false;
        }
    }
    /* Gives the largest forward branch/JAL a legal landing point before ret. */
    if (!ai_it_raw_emit(stream, 0x00000013U, AI_IT_RAW_NOP) ||
        !ai_it_raw_emit(stream, 0x00008067U, AI_IT_RAW_RET)) {
        return false;
    }
    stream->body_words = stream->word_count - stream->body_start;
    stream->sample_count = sizeof(stream->sample_words) /
        sizeof(stream->sample_words[0]);
    for (unsigned sample = 0U; sample < stream->sample_count; ++sample) {
        const unsigned begin = stream->body_start +
            stream->body_words * sample / stream->sample_count;
        const unsigned end = stream->body_start +
            stream->body_words * (sample + 1U) / stream->sample_count;
        const unsigned pc = begin + (unsigned)(ai_stress_rng_next(rng) %
            (end - begin));

        stream->sample_pcs[sample] = pc;
        stream->sample_words[sample] = ai_it_raw_code[pc];
    }
    return stream->body_words > 0U;
}

static void ai_it_raw_initialize_memory(struct ai_stress_rng *rng)
{
    for (unsigned index = 0U; index < AI_IT_RAW_MEM_WORDS; ++index) {
        const uint64_t value = ai_stress_rng_next(rng) ^
            ((uint64_t)index << 32U) ^ 0xfeed000000000000ULL;

        ai_it_raw_expected_memory[index] = value;
        ai_it_raw_memory[index] = value;
        ai_it_raw_expected_last_write_pc[index] = AI_IT_RAW_MAX_WORDS;
        ai_it_raw_expected_last_write_word[index] = 0U;
    }
}

static void ai_it_raw_initialize_vectors(struct ai_stress_rng *rng)
{
    for (unsigned index = 0U; index < AI_IT_STREAM_VECTOR_BYTES; ++index) {
        const uint8_t value = (uint8_t)ai_stress_rng_next(rng);

        ai_it_raw_expected_vector_src[index] = value;
        ai_it_raw_vector_src[index] = value;
    }
    for (unsigned index = 0U; index < AI_IT_STREAM_VECTOR_STORAGE; ++index) {
        const uint8_t value = (uint8_t)ai_stress_rng_next(rng);

        ai_it_raw_expected_vector_dst[index] = value;
        ai_it_raw_vector_dst[index] = value;
    }
    for (unsigned group = 0U; group < AI_IT_RAW_VECTOR_GROUPS; ++group) {
        for (unsigned index = 0U; index < AI_IT_RAW_VECTOR_GROUP_BYTES; ++index) {
            ai_it_raw_expected_vregs[group][index] = 0U;
        }
    }
}

static void ai_it_raw_flush_memory(void)
{
    for (unsigned line = 0U; line < AI_IT_RAW_MEM_LINES; ++line) {
        cbo_flush((uintptr_t)&ai_it_raw_memory[line * 8U]);
    }
    fence_iorw_iorw();
}

static void ai_it_raw_flush_vectors(void)
{
    for (unsigned index = 0U; index < AI_IT_STREAM_VECTOR_BYTES;
         index += AI_IT_STREAM_CACHE_LINE_BYTES) {
        cbo_flush((uintptr_t)&ai_it_raw_vector_src[index]);
    }
    for (unsigned index = 0U; index < AI_IT_STREAM_VECTOR_STORAGE;
         index += AI_IT_STREAM_CACHE_LINE_BYTES) {
        cbo_flush((uintptr_t)&ai_it_raw_vector_dst[index]);
    }
    fence_iorw_iorw();
}

__attribute__((noinline)) static uint64_t ai_it_raw_execute(uint64_t input,
    unsigned amo_word, size_t vl)
{
    const ai_it_raw_exec_fn execute = (ai_it_raw_exec_fn)(uintptr_t)
        &ai_it_raw_code[0];

    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");
    return execute(input, (uintptr_t)&ai_it_raw_memory[AI_IT_RAW_BASE_WORD],
        (uintptr_t)&ai_it_raw_memory[amo_word],
        (uintptr_t)&ai_it_raw_vector_src[0],
        (uintptr_t)&ai_it_raw_vector_dst[0], (uintptr_t)vl);
}

static uint64_t ai_it_raw_memory_signature(
    const volatile uint64_t *memory)
{
    uint64_t signature = 0x510e527fade682d1ULL;

    for (unsigned index = 0U; index < AI_IT_RAW_MEM_WORDS; ++index) {
        signature = ai_stress_signature_fold(signature,
            memory[index] ^ index);
    }
    return signature;
}

static enum ai_it_raw_class ai_it_raw_classify_word(uint32_t word)
{
    const unsigned opcode = word & 0x7fU;
    const unsigned funct3 = (word >> 12U) & 7U;
    const unsigned funct7 = (word >> 25U) & 0x7fU;
    const unsigned funct6 = (word >> 26U) & 0x3fU;

    if (word == 0x00801073U) return AI_IT_RAW_VSTART;
    if (word == 0x00000013U) return AI_IT_RAW_NOP;
    if (opcode == 0x37U) return AI_IT_RAW_LUI;
    if (opcode == 0x33U) {
        if (funct7 == 1U && funct3 == 0U) return AI_IT_RAW_MUL;
        if (funct7 == 1U && (funct3 == 4U || funct3 == 5U)) {
            return AI_IT_RAW_DIV;
        }
        if (funct7 == 1U && (funct3 == 6U || funct3 == 7U)) {
            return AI_IT_RAW_REM;
        }
        return AI_IT_RAW_R_ALU;
    }
    if (opcode == 0x13U) return AI_IT_RAW_I_ALU;
    if (opcode == 0x03U && funct3 == 3U) return AI_IT_RAW_LOAD;
    if (opcode == 0x23U && funct3 == 3U) return AI_IT_RAW_STORE;
    if (opcode == 0x2fU) return AI_IT_RAW_AMO_RMW;
    if (opcode == 0x63U) return AI_IT_RAW_BRANCH;
    if (opcode == 0x6fU) return AI_IT_RAW_JAL;
    if (opcode == 0x0fU && word == 0x0000100fU) {
        return AI_IT_RAW_FENCE_I;
    }
    if (opcode == 0x0fU && funct3 == 0U) return AI_IT_RAW_FENCE;
    if (opcode == 0x0fU && funct3 == 2U &&
        ((word >> 20U) & 0xfffU) == 2U && ((word >> 7U) & 0x1fU) == 0U) {
        return AI_IT_RAW_CBO_FLUSH;
    }
    if (opcode == 0x57U && funct3 == 7U &&
        ((word >> 20U) & 0x7ffU) == 3U) {
        return AI_IT_RAW_VSETVLI;
    }
    if (opcode == 0x07U && funct3 == 0U &&
        ((word >> 25U) & 1U) != 0U && ((word >> 20U) & 0x1fU) == 0U) {
        return AI_IT_RAW_VLE8;
    }
    if (opcode == 0x27U && funct3 == 0U &&
        ((word >> 25U) & 1U) != 0U && ((word >> 20U) & 0x1fU) == 0U) {
        return AI_IT_RAW_VSE8;
    }
    if (opcode == 0x57U && funct3 == 4U && ((word >> 25U) & 1U) != 0U) {
        if (funct6 == 0x0cU) return AI_IT_RAW_VRGATHER_VX;
        if (funct6 == 0x00U || funct6 == 0x02U || funct6 == 0x09U ||
            funct6 == 0x0aU || funct6 == 0x0bU) {
            return AI_IT_RAW_VALU_VX;
        }
    }
    if (opcode == 0x67U && word == 0x00008067U) return AI_IT_RAW_RET;
    return AI_IT_RAW_CLASS_COUNT;
}

static bool ai_it_raw_address_index(const uint64_t regs[32], unsigned rs1,
    int32_t immediate, unsigned amo_word, unsigned *index)
{
    int64_t byte_offset;

    if (rs1 == AI_IT_RAW_BASE) {
        byte_offset = (int64_t)regs[rs1] + immediate;
    } else if (rs1 == AI_IT_RAW_AMO_BASE) {
        byte_offset = (int64_t)regs[rs1] + immediate;
    } else {
        return false;
    }
    if (byte_offset < 0 || byte_offset % (int64_t)sizeof(uint64_t) != 0 ||
        byte_offset >= (int64_t)AI_IT_RAW_MEM_WORDS *
            (int64_t)sizeof(uint64_t)) {
        return false;
    }
    *index = (unsigned)(byte_offset / (int64_t)sizeof(uint64_t));
    return true;
}

static int ai_it_raw_vector_group_index(unsigned base)
{
    if (base > 24U || (base & 7U) != 0U) {
        return -1;
    }
    return (int)(base / 8U);
}

static bool ai_it_raw_is_vector_address(unsigned rs1)
{
    return rs1 == AI_IT_RAW_VECTOR_SRC || rs1 == AI_IT_RAW_VECTOR_DST;
}

static struct ai_it_raw_reference ai_it_raw_reference_execute(
    const struct ai_it_raw_stream *stream, uint64_t input, unsigned amo_word,
    size_t vl)
{
    struct ai_it_raw_reference reference = { 0 };
    uint64_t regs[32] = { 0 };
    unsigned pc = 0U;
    unsigned steps = 0U;
    bool vector_configured = false;
    const unsigned step_limit = stream->word_count * 4U + 32U;

    regs[AI_IT_RAW_A0] = input;
    regs[AI_IT_RAW_BASE] = (uint64_t)AI_IT_RAW_BASE_WORD * sizeof(uint64_t);
    regs[AI_IT_RAW_AMO_BASE] = (uint64_t)amo_word * sizeof(uint64_t);
    regs[AI_IT_RAW_AVL] = vl;
    while (pc < stream->word_count && steps++ < step_limit) {
        const uint32_t word = ai_it_raw_code[pc];
        const unsigned opcode = word & 0x7fU;
        const unsigned rd = (word >> 7U) & 0x1fU;
        const unsigned funct3 = (word >> 12U) & 7U;
        const unsigned rs1 = (word >> 15U) & 0x1fU;
        const unsigned rs2 = (word >> 20U) & 0x1fU;
        const unsigned funct7 = (word >> 25U) & 0x7fU;
        enum ai_it_raw_class decoded_class = ai_it_raw_classify_word(word);
        enum ai_it_raw_class class = decoded_class;

        reference.stop_pc = pc;
        reference.stop_word = word;

        if (pc < stream->body_start) {
            class = AI_IT_RAW_INIT;
        }
        if (decoded_class == AI_IT_RAW_CLASS_COUNT ||
            (decoded_class == AI_IT_RAW_RET && word != 0x00008067U)) {
            return reference;
        }
        ++reference.executed_words;
        ++reference.executed_hist[class];
        switch (opcode) {
        case 0x13U: {
            const int32_t immediate = ai_it_raw_decode_i(word);
            const unsigned shift = (unsigned)((word >> 20U) & 0x3fU);

            if (decoded_class == AI_IT_RAW_NOP) {
                ++pc;
                break;
            }
            if (funct3 == 0U) {
                ai_it_raw_write_reg(regs, rd, regs[rs1] +
                    (uint64_t)(int64_t)immediate);
            } else if (funct3 == 4U) {
                ai_it_raw_write_reg(regs, rd, regs[rs1] ^
                    (uint64_t)(int64_t)immediate);
            } else if (funct3 == 6U) {
                ai_it_raw_write_reg(regs, rd, regs[rs1] |
                    (uint64_t)(int64_t)immediate);
            } else if (funct3 == 7U) {
                ai_it_raw_write_reg(regs, rd, regs[rs1] &
                    (uint64_t)(int64_t)immediate);
            } else if (funct3 == 2U) {
                ai_it_raw_write_reg(regs, rd,
                    (int64_t)regs[rs1] < (int64_t)immediate);
            } else if (funct3 == 3U) {
                ai_it_raw_write_reg(regs, rd,
                    regs[rs1] < (uint64_t)(int64_t)immediate);
            } else if (funct3 == 1U && ((word >> 26U) & 0x3fU) == 0U) {
                ai_it_raw_write_reg(regs, rd, regs[rs1] << shift);
            } else if (funct3 == 5U && ((word >> 26U) & 0x3fU) == 0U) {
                ai_it_raw_write_reg(regs, rd, regs[rs1] >> shift);
            } else if (funct3 == 5U && ((word >> 26U) & 0x3fU) == 0x10U) {
                ai_it_raw_write_reg(regs, rd,
                    ai_it_raw_sra(regs[rs1], shift));
            } else {
                return reference;
            }
            ++pc;
            break;
        }
        case 0x33U: {
            const uint64_t lhs = regs[rs1];
            const uint64_t rhs = regs[rs2];
            uint64_t result;

            if (funct7 == 1U && funct3 == 0U) {
                result = lhs * rhs;
            } else if (funct7 == 1U && funct3 == 4U) {
                result = ai_it_raw_div_signed(lhs, rhs);
            } else if (funct7 == 1U && funct3 == 5U) {
                result = rhs == 0ULL ? ~0ULL : lhs / rhs;
            } else if (funct7 == 1U && funct3 == 6U) {
                result = ai_it_raw_rem_signed(lhs, rhs);
            } else if (funct7 == 1U && funct3 == 7U) {
                result = rhs == 0ULL ? lhs : lhs % rhs;
            } else if (funct7 == 0U || funct7 == 0x20U) {
                switch (funct3) {
                case 0U: result = funct7 == 0x20U ? lhs - rhs : lhs + rhs; break;
                case 1U: result = lhs << (rhs & 63ULL); break;
                case 2U: result = (int64_t)lhs < (int64_t)rhs; break;
                case 3U: result = lhs < rhs; break;
                case 4U: result = lhs ^ rhs; break;
                case 5U: result = funct7 == 0x20U ?
                    ai_it_raw_sra(lhs, rhs & 63ULL) : lhs >> (rhs & 63ULL); break;
                case 6U: result = lhs | rhs; break;
                case 7U: result = lhs & rhs; break;
                default: return reference;
                }
            } else {
                return reference;
            }
            ai_it_raw_write_reg(regs, rd, result);
            ++pc;
            break;
        }
        case 0x37U:
            ai_it_raw_write_reg(regs, rd,
                (uint64_t)(int64_t)(int32_t)(word & 0xfffff000U));
            ++pc;
            break;
        case 0x03U: {
            unsigned index;

            if (funct3 != 3U || !ai_it_raw_address_index(regs, rs1,
                    ai_it_raw_decode_i(word), amo_word, &index)) {
                return reference;
            }
            ai_it_raw_write_reg(regs, rd, ai_it_raw_expected_memory[index]);
            ++pc;
            break;
        }
        case 0x23U: {
            unsigned index;

            if (funct3 != 3U || !ai_it_raw_address_index(regs, rs1,
                    ai_it_raw_decode_s(word), amo_word, &index)) {
                return reference;
            }
            ai_it_raw_expected_memory[index] = regs[rs2];
            ai_it_raw_expected_last_write_pc[index] = pc;
            ai_it_raw_expected_last_write_word[index] = word;
            ++pc;
            break;
        }
        case 0x2fU: {
            unsigned index;
            const unsigned funct5 = (word >> 27U) & 0x1fU;

            if (funct3 != 3U ||
                !ai_it_raw_address_index(regs, rs1, 0, amo_word, &index)) {
                return reference;
            }
            const uint64_t old_value = ai_it_raw_expected_memory[index];
            uint64_t new_value;

            switch (funct5) {
            case 0U: new_value = old_value + regs[rs2]; break;
            case 1U: new_value = regs[rs2]; break;
            case 4U: new_value = old_value ^ regs[rs2]; break;
            case 12U: new_value = old_value & regs[rs2]; break;
            case 8U: new_value = old_value | regs[rs2]; break;
            default: return reference;
            }

            ai_it_raw_expected_memory[index] = new_value;
            ai_it_raw_expected_last_write_pc[index] = pc;
            ai_it_raw_expected_last_write_word[index] = word;
            ai_it_raw_write_reg(regs, rd, old_value);
            ++pc;
            break;
        }
        case 0x63U: {
            const int32_t immediate = ai_it_raw_decode_b(word);
            const bool equal = regs[rs1] == regs[rs2];
            bool take;

            switch (funct3) {
            case 0U: take = equal; break;
            case 1U: take = !equal; break;
            case 4U: take = (int64_t)regs[rs1] < (int64_t)regs[rs2]; break;
            case 5U: take = (int64_t)regs[rs1] >= (int64_t)regs[rs2]; break;
            case 6U: take = regs[rs1] < regs[rs2]; break;
            case 7U: take = regs[rs1] >= regs[rs2]; break;
            default: return reference;
            }
            if (take) {
                const int64_t target = (int64_t)pc + immediate / 4;

                if (target <= (int64_t)pc || target >=
                    (int64_t)stream->word_count) {
                    return reference;
                }
                pc = (unsigned)target;
                ++reference.taken_branches;
            } else {
                ++pc;
                ++reference.not_taken_branches;
            }
            break;
        }
        case 0x6fU: {
            const int32_t immediate = ai_it_raw_decode_j(word);
            const int64_t target = (int64_t)pc + immediate / 4;

            if (rd != 0U || target <= (int64_t)pc || target >=
                (int64_t)stream->word_count) {
                return reference;
            }
            pc = (unsigned)target;
            break;
        }
        case 0x0fU:
            if (decoded_class == AI_IT_RAW_CBO_FLUSH) {
                if (rs1 != AI_IT_RAW_BASE && rs1 != AI_IT_RAW_AMO_BASE) {
                    return reference;
                }
                ++pc;
                break;
            }
            if (word != 0x0000100fU && funct3 != 0U) {
                return reference;
            }
            ++pc;
            break;
        case 0x57U:
            if (decoded_class == AI_IT_RAW_VSETVLI) {
                if (funct3 != 7U || rs1 != AI_IT_RAW_AVL ||
                    ((word >> 20U) & 0x7ffU) != 3U) {
                    return reference;
                }
                ai_it_raw_write_reg(regs, rd, vl);
                vector_configured = true;
                ++pc;
                break;
            }
            if (!vector_configured || funct3 != 4U ||
                ((word >> 25U) & 1U) == 0U) {
                return reference;
            }
            {
                const int destination = ai_it_raw_vector_group_index(rd);
                const int source = ai_it_raw_vector_group_index(rs2);

                if (destination < 0 || source < 0) {
                    return reference;
                }
                if (decoded_class == AI_IT_RAW_VRGATHER_VX) {
                    const uint64_t index = regs[rs1];
                    const uint8_t value = index < vl ?
                        ai_it_raw_expected_vregs[source][index] : 0U;

                    for (size_t element = 0U; element < vl; ++element) {
                        ai_it_raw_expected_vregs[destination][element] = value;
                    }
                } else if (decoded_class == AI_IT_RAW_VALU_VX) {
                    const unsigned funct6 = (word >> 26U) & 0x3fU;
                    const uint8_t scalar = (uint8_t)regs[rs1];

                    for (size_t element = 0U; element < vl; ++element) {
                        const uint8_t source_value =
                            ai_it_raw_expected_vregs[source][element];
                        uint8_t value;

                        switch (funct6) {
                        case 0x00U: value = (uint8_t)(source_value + scalar);
                            break;
                        case 0x02U: value = (uint8_t)(source_value - scalar);
                            break;
                        case 0x09U: value = source_value & scalar; break;
                        case 0x0aU: value = source_value | scalar; break;
                        case 0x0bU: value = source_value ^ scalar; break;
                        default: return reference;
                        }
                        ai_it_raw_expected_vregs[destination][element] = value;
                    }
                } else {
                    return reference;
                }
                ++pc;
                break;
            }
        case 0x07U:
        case 0x27U:
            if (!vector_configured || !ai_it_raw_is_vector_address(rs1)) {
                return reference;
            }
            {
                const unsigned base = (word >> 7U) & 0x1fU;
                const int group = ai_it_raw_vector_group_index(base);
                const bool source_address = rs1 == AI_IT_RAW_VECTOR_SRC;

                if (group < 0) {
                    return reference;
                }
                if (opcode == 0x07U && decoded_class == AI_IT_RAW_VLE8 &&
                    source_address) {
                    for (size_t element = 0U; element < vl; ++element) {
                        ai_it_raw_expected_vregs[group][element] =
                            ai_it_raw_expected_vector_src[element];
                    }
                } else if (opcode == 0x27U &&
                    decoded_class == AI_IT_RAW_VSE8 && !source_address) {
                    for (size_t element = 0U; element < vl; ++element) {
                        ai_it_raw_expected_vector_dst[element] =
                            ai_it_raw_expected_vregs[group][element];
                    }
                } else {
                    return reference;
                }
                ++pc;
                break;
            }
        case 0x73U:
            if (word != 0x00801073U) {
                return reference;
            }
            ++pc;
            break;
        case 0x67U:
            if (word != 0x00008067U || pc != stream->word_count - 1U) {
                return reference;
            }
            reference.result = regs[AI_IT_RAW_A0];
            reference.valid = true;
            return reference;
        default:
            return reference;
        }
        regs[0] = 0ULL;
    }
    return reference;
}

/* Active raw-only implementation. */
static unsigned ai_it_stratified_mode(unsigned round, unsigned mode_count,
    uint64_t salt)
{
    const uint64_t key = ai_stress_mix64(AI_IT_STRESS_SHARD_ID ^ salt);
    const unsigned offset = (unsigned)(key % mode_count);
    const unsigned step = mode_count == 4U ?
        1U + 2U * (unsigned)((key >> 8U) & 1ULL) :
        1U + (unsigned)((key >> 8U) & 1ULL);

    return (offset + round * step) % mode_count;
}

static unsigned ai_it_branch_direction(unsigned branch_mode, unsigned slot,
    uint64_t random_value)
{
    switch (branch_mode) {
    case AI_IT_BRANCH_ALTERNATING:
        return slot & 1U;
    case AI_IT_BRANCH_LONG_RUN:
        return (slot >> 3U) & 1U;
    case AI_IT_BRANCH_BIASED:
        return slot % 8U != 0U;
    default:
        return (unsigned)(random_value & 1ULL);
    }
}

static const char *ai_it_address_mode_name(unsigned mode)
{
    static const char *const names[AI_IT_ADDR_MODE_COUNT] = {
        "stream", "random", "set-conflict-candidate", "hotspot"
    };

    return names[mode];
}

static const char *ai_it_dependency_mode_name(unsigned mode)
{
    static const char *const names[AI_IT_DEP_MODE_COUNT] = {
        "long-chain", "round-robin", "mixed"
    };

    return names[mode];
}

static const char *ai_it_branch_mode_name(unsigned mode)
{
    static const char *const names[AI_IT_BRANCH_MODE_COUNT] = {
        "random", "alternating", "long-run", "biased"
    };

    return names[mode];
}

static const char *ai_it_cleanup_mode_name(unsigned mode)
{
    static const char *const names[AI_IT_CLEANUP_MODE_COUNT] = {
        "fence", "cmo", "fence+cmo"
    };

    return names[mode];
}

static void ai_it_raw_add_pressure(const struct ai_it_raw_reference *reference,
    unsigned *pressure_mask, unsigned *cleanup_mask)
{
    for (unsigned class = 0U; class < AI_IT_RAW_CLASS_COUNT; ++class) {
        if (reference->executed_hist[class] == 0U) {
            continue;
        }
        switch (class) {
        case AI_IT_RAW_LUI:
        case AI_IT_RAW_R_ALU:
        case AI_IT_RAW_I_ALU:
        case AI_IT_RAW_MUL:
        case AI_IT_RAW_DIV:
        case AI_IT_RAW_REM:
            *pressure_mask |= AI_STRESS_PRESSURE_BACKEND;
            break;
        case AI_IT_RAW_LOAD:
            *pressure_mask |= AI_STRESS_PRESSURE_SCALAR_LOAD;
            break;
        case AI_IT_RAW_STORE:
            *pressure_mask |= AI_STRESS_PRESSURE_SCALAR_STORE;
            break;
        case AI_IT_RAW_AMO_RMW:
            *pressure_mask |= AI_STRESS_PRESSURE_ATOMIC;
            break;
        case AI_IT_RAW_BRANCH:
        case AI_IT_RAW_JAL:
            *pressure_mask |= AI_STRESS_PRESSURE_BRANCH;
            *cleanup_mask |= AI_STRESS_CLEANUP_REDIRECT;
            break;
        case AI_IT_RAW_VSETVLI:
        case AI_IT_RAW_VLE8:
        case AI_IT_RAW_VSE8:
        case AI_IT_RAW_VRGATHER_VX:
        case AI_IT_RAW_VALU_VX:
            *pressure_mask |= AI_STRESS_PRESSURE_VECTOR;
            break;
        case AI_IT_RAW_FENCE:
        case AI_IT_RAW_FENCE_I:
            *cleanup_mask |= AI_STRESS_CLEANUP_FENCE;
            break;
        case AI_IT_RAW_CBO_FLUSH:
            *cleanup_mask |= AI_STRESS_CLEANUP_CMO;
            break;
        default:
            break;
        }
    }
}

static bool ai_it_raw_required_mix(const unsigned histogram[])
{
    return histogram[AI_IT_RAW_LUI] > 0U &&
        histogram[AI_IT_RAW_R_ALU] > 0U &&
        histogram[AI_IT_RAW_I_ALU] > 0U &&
        histogram[AI_IT_RAW_MUL] > 0U &&
        histogram[AI_IT_RAW_DIV] > 0U &&
        histogram[AI_IT_RAW_REM] > 0U &&
        histogram[AI_IT_RAW_LOAD] > 0U &&
        histogram[AI_IT_RAW_STORE] > 0U &&
        histogram[AI_IT_RAW_AMO_RMW] > 0U &&
        histogram[AI_IT_RAW_BRANCH] > 0U &&
        histogram[AI_IT_RAW_VLE8] > 0U &&
        histogram[AI_IT_RAW_VSE8] > 0U &&
        histogram[AI_IT_RAW_VRGATHER_VX] > 0U &&
        histogram[AI_IT_RAW_VALU_VX] > 0U &&
        histogram[AI_IT_RAW_VSETVLI] > 0U;
}

static bool ai_it_raw_cleanup_policy(unsigned cleanup_mode,
    const unsigned histogram[AI_IT_RAW_CLASS_COUNT])
{
    const bool has_fence = histogram[AI_IT_RAW_FENCE] > 0U;
    const bool has_cbo = histogram[AI_IT_RAW_CBO_FLUSH] > 0U;

    return (cleanup_mode == AI_IT_CLEANUP_FENCE && has_fence && !has_cbo) ||
        (cleanup_mode == AI_IT_CLEANUP_CMO && has_cbo && !has_fence) ||
        (cleanup_mode == AI_IT_CLEANUP_FENCE_AND_CMO && has_fence && has_cbo);
}

static void ai_it_log_raw_stream(uint64_t seed, unsigned round,
    const struct ai_it_raw_stream *stream,
    const struct ai_it_raw_reference *reference, unsigned amo_word,
    uint64_t actual_result, uint64_t expected_result,
    uint64_t actual_memory_signature, uint64_t expected_memory_signature,
    uint64_t actual_vector_signature, uint64_t expected_vector_signature,
    unsigned pressure_mask, unsigned cleanup_mask, unsigned address_mode,
    unsigned dependency_mode, unsigned branch_mode, unsigned cleanup_mode,
    bool hash_unique, bool accepted)
{
    INFO("HYPSTRESS_IT_RAW seed=0x%016llx shard=%llu round=%u "
         "slots=%u words=%u body_words=%u word_hash=0x%016llx "
         "field_hash=0x%016llx unique_words=%u hash_unique=%d "
         "opcode_variants=%u funct3_variants=%u upper7_variants=%u "
         "semantic_variants=%u variant_mask=0x%016llx "
         "samples=%u:0x%08x/%u:0x%08x/%u:0x%08x/%u:0x%08x "
         "executed=%u stop_pc=%u stop_word=0x%08x taken=%u not_taken=%u "
         "amo_word=%u pressure=0x%x cleanup=0x%x address_mode=%s "
         "dependency_mode=%s branch_mode=%s cleanup_mode=%s "
         "raw_result=0x%016llx/0x%016llx raw_mem=0x%016llx/0x%016llx "
         "raw_vector=0x%016llx/0x%016llx raw_valid=%d status=%s "
         "coverage_contract=linknan_high_watermark_required",
        (unsigned long long)seed, (unsigned long long)AI_IT_STRESS_SHARD_ID,
        round, stream->body_slots, stream->word_count, stream->body_words,
        (unsigned long long)stream->word_hash,
        (unsigned long long)stream->field_hash, stream->unique_word_count,
        hash_unique, stream->opcode_variants, stream->funct3_variants,
        stream->upper7_variants, stream->semantic_variants,
        (unsigned long long)stream->semantic_variant_mask,
        stream->sample_count > 0U ? stream->sample_pcs[0] : 0U,
        stream->sample_count > 0U ? stream->sample_words[0] : 0U,
        stream->sample_count > 1U ? stream->sample_pcs[1] : 0U,
        stream->sample_count > 1U ? stream->sample_words[1] : 0U,
        stream->sample_count > 2U ? stream->sample_pcs[2] : 0U,
        stream->sample_count > 2U ? stream->sample_words[2] : 0U,
        stream->sample_count > 3U ? stream->sample_pcs[3] : 0U,
        stream->sample_count > 3U ? stream->sample_words[3] : 0U,
        reference->executed_words, reference->stop_pc,
        reference->stop_word, reference->taken_branches,
        reference->not_taken_branches, amo_word, pressure_mask, cleanup_mask,
        ai_it_address_mode_name(address_mode),
        ai_it_dependency_mode_name(dependency_mode),
        ai_it_branch_mode_name(branch_mode),
        ai_it_cleanup_mode_name(cleanup_mode),
        (unsigned long long)actual_result, (unsigned long long)expected_result,
        (unsigned long long)actual_memory_signature,
        (unsigned long long)expected_memory_signature,
        (unsigned long long)actual_vector_signature,
        (unsigned long long)expected_vector_signature, reference->valid,
        accepted ? "accepted" : "rejected");
    INFO("HYPSTRESS_IT_RAW_HIST generated={init:%u,lui:%u,r:%u,i:%u,mul:%u,"
         "div:%u,rem:%u,ld:%u,st:%u,amo:%u,br:%u,jal:%u,fence:%u,"
         "fencei:%u,cbo:%u,vset:%u,vle:%u,vse:%u,vrg:%u,valu:%u,nop:%u} "
         "executed={init:%u,lui:%u,r:%u,i:%u,mul:%u,div:%u,rem:%u,ld:%u,"
         "st:%u,amo:%u,br:%u,jal:%u,fence:%u,fencei:%u,cbo:%u,vset:%u,"
         "vle:%u,vse:%u,vrg:%u,valu:%u,nop:%u}",
        stream->generated_hist[AI_IT_RAW_INIT],
        stream->generated_hist[AI_IT_RAW_LUI],
        stream->generated_hist[AI_IT_RAW_R_ALU],
        stream->generated_hist[AI_IT_RAW_I_ALU],
        stream->generated_hist[AI_IT_RAW_MUL],
        stream->generated_hist[AI_IT_RAW_DIV],
        stream->generated_hist[AI_IT_RAW_REM],
        stream->generated_hist[AI_IT_RAW_LOAD],
        stream->generated_hist[AI_IT_RAW_STORE],
        stream->generated_hist[AI_IT_RAW_AMO_RMW],
        stream->generated_hist[AI_IT_RAW_BRANCH],
        stream->generated_hist[AI_IT_RAW_JAL],
        stream->generated_hist[AI_IT_RAW_FENCE],
        stream->generated_hist[AI_IT_RAW_FENCE_I],
        stream->generated_hist[AI_IT_RAW_CBO_FLUSH],
        stream->generated_hist[AI_IT_RAW_VSETVLI],
        stream->generated_hist[AI_IT_RAW_VLE8],
        stream->generated_hist[AI_IT_RAW_VSE8],
        stream->generated_hist[AI_IT_RAW_VRGATHER_VX],
        stream->generated_hist[AI_IT_RAW_VALU_VX],
        stream->generated_hist[AI_IT_RAW_NOP],
        reference->executed_hist[AI_IT_RAW_INIT],
        reference->executed_hist[AI_IT_RAW_LUI],
        reference->executed_hist[AI_IT_RAW_R_ALU],
        reference->executed_hist[AI_IT_RAW_I_ALU],
        reference->executed_hist[AI_IT_RAW_MUL],
        reference->executed_hist[AI_IT_RAW_DIV],
        reference->executed_hist[AI_IT_RAW_REM],
        reference->executed_hist[AI_IT_RAW_LOAD],
        reference->executed_hist[AI_IT_RAW_STORE],
        reference->executed_hist[AI_IT_RAW_AMO_RMW],
        reference->executed_hist[AI_IT_RAW_BRANCH],
        reference->executed_hist[AI_IT_RAW_JAL],
        reference->executed_hist[AI_IT_RAW_FENCE],
        reference->executed_hist[AI_IT_RAW_FENCE_I],
        reference->executed_hist[AI_IT_RAW_CBO_FLUSH],
        reference->executed_hist[AI_IT_RAW_VSETVLI],
        reference->executed_hist[AI_IT_RAW_VLE8],
        reference->executed_hist[AI_IT_RAW_VSE8],
        reference->executed_hist[AI_IT_RAW_VRGATHER_VX],
        reference->executed_hist[AI_IT_RAW_VALU_VX],
        reference->executed_hist[AI_IT_RAW_NOP]);
}

bool ai_stress_it_randomized_instruction_schedule_scoreboard_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, AI_IT_STREAM_MSTATUS_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    size_t vl;
    /* This is setup only. All stress operations below come from raw words. */
    asm volatile(
        "vsetvli %0, %1, e8, m8, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        : "=r"(vl)
        : "r"((size_t)AI_IT_STREAM_VECTOR_BYTES)
        : "memory");
    const size_t safe_vl = vl != 0U ? vl : 1U;

    TEST_ASSERT("the raw IT stream setup obtains a nonzero e8,m8 VL",
        excpt.triggered == false && vl > 0U &&
        vl <= AI_IT_STREAM_VECTOR_BYTES,
        "vl=%lu", (unsigned long)vl);

    uint64_t seen_raw_word_hashes[AI_IT_STRESS_SEED_COUNT];
    unsigned seen_raw_word_hash_count = 0U;
    bool raw_hashes_unique = true;

    for (unsigned round = 0U; round < AI_IT_STRESS_SEED_COUNT; ++round) {
        const uint64_t seed = ai_stress_sharded_seed(
            AI_IT_STREAM_PROFILE_ID, round, AI_IT_STRESS_SHARD_ID);
        struct ai_stress_rng rng;
        struct ai_it_raw_stream raw_stream = { 0 };
        struct ai_it_raw_reference raw_reference = { 0 };
        uint64_t raw_actual_result = 0ULL;
        uint64_t raw_expected_result = 0ULL;
        uint64_t raw_actual_memory_signature = 0ULL;
        uint64_t raw_expected_memory_signature = 0ULL;
        uint64_t raw_actual_vector_signature = 0ULL;
        uint64_t raw_expected_vector_signature = 0ULL;
        unsigned raw_mismatch_index = AI_IT_RAW_MEM_WORDS;
        uint64_t raw_mismatch_actual = 0ULL;
        uint64_t raw_mismatch_expected = 0ULL;
        unsigned raw_mismatch_pc = AI_IT_RAW_MAX_WORDS;
        uint32_t raw_mismatch_word = 0U;
        unsigned raw_vector_mismatch_region = 2U;
        unsigned raw_vector_mismatch_index = AI_IT_STREAM_VECTOR_STORAGE;
        unsigned raw_vector_mismatch_actual = 0U;
        unsigned raw_vector_mismatch_expected = 0U;
        unsigned raw_pressure_mask = 0U;
        unsigned raw_cleanup_mask = 0U;
        bool raw_build_ok = false;
        bool raw_memory_ok = false;
        bool raw_vector_ok = false;
        bool raw_hash_unique = false;

        ai_stress_rng_init(&rng, seed);
        const unsigned body_slots = AI_IT_STRESS_MIN_OPS +
            (unsigned)(ai_stress_rng_next(&rng) %
                (AI_IT_STRESS_MAX_OPS - AI_IT_STRESS_MIN_OPS + 1U));
        const unsigned address_mode = ai_it_stratified_mode(round,
            AI_IT_ADDR_MODE_COUNT, 0x243f6a8885a308d3ULL);
        const unsigned dependency_mode = ai_it_stratified_mode(round,
            AI_IT_DEP_MODE_COUNT, 0x13198a2e03707344ULL);
        const unsigned branch_mode = ai_it_stratified_mode(round,
            AI_IT_BRANCH_MODE_COUNT, 0xa4093822299f31d0ULL);
        const unsigned cleanup_mode = ai_it_stratified_mode(round,
            AI_IT_CLEANUP_MODE_COUNT, 0x082efa98ec4e6c89ULL);
        const unsigned stream_stride =
            1U + 2U * (unsigned)(ai_stress_rng_next(&rng) & 7ULL);
        const unsigned address_anchor = (unsigned)(ai_stress_rng_next(&rng) %
            AI_IT_RAW_MEM_LINES);
        const unsigned raw_amo_word = (unsigned)(ai_stress_rng_next(&rng) %
            AI_IT_RAW_MEM_WORDS);
        const uint64_t initial_input = ai_stress_rng_next(&rng);

        ai_it_raw_initialize_memory(&rng);
        ai_it_raw_initialize_vectors(&rng);
        raw_build_ok = ai_it_raw_build_stream(&rng, body_slots, address_mode,
            stream_stride, address_anchor, dependency_mode, branch_mode,
            cleanup_mode, safe_vl, &raw_stream);
        raw_hash_unique = raw_build_ok;
        for (unsigned index = 0U; index < seen_raw_word_hash_count; ++index) {
            raw_hash_unique = raw_hash_unique &&
                raw_stream.word_hash != seen_raw_word_hashes[index];
        }
        raw_hashes_unique = raw_hashes_unique && raw_hash_unique;
        if (raw_build_ok) {
            seen_raw_word_hashes[seen_raw_word_hash_count++] =
                raw_stream.word_hash;
        }

        if (raw_build_ok) {
            raw_reference = ai_it_raw_reference_execute(&raw_stream,
                initial_input, raw_amo_word, safe_vl);
            raw_expected_result = raw_reference.result;
            raw_expected_memory_signature = ai_it_raw_memory_signature(
                &ai_it_raw_expected_memory[0]);
            raw_expected_vector_signature = ai_stress_signature_fold(
                ai_stress_byte_signature(&ai_it_raw_expected_vector_src[0],
                    AI_IT_STREAM_VECTOR_BYTES),
                ai_stress_byte_signature(&ai_it_raw_expected_vector_dst[0],
                    AI_IT_STREAM_VECTOR_STORAGE));
            ai_it_raw_add_pressure(&raw_reference, &raw_pressure_mask,
                &raw_cleanup_mask);
        }

        TEST_SETUP_EXCEPT();
        if (raw_build_ok && raw_reference.valid) {
            ai_it_raw_flush_memory();
            ai_it_raw_flush_vectors();
            raw_actual_result = ai_it_raw_execute(initial_input,
                raw_amo_word, safe_vl);
            fence_iorw_iorw();
            raw_actual_memory_signature = ai_it_raw_memory_signature(
                &ai_it_raw_memory[0]);
            raw_actual_vector_signature = ai_stress_signature_fold(
                ai_stress_byte_signature(&ai_it_raw_vector_src[0],
                    AI_IT_STREAM_VECTOR_BYTES),
                ai_stress_byte_signature(&ai_it_raw_vector_dst[0],
                    AI_IT_STREAM_VECTOR_STORAGE));
            raw_memory_ok = true;
            for (unsigned index = 0U; index < AI_IT_RAW_MEM_WORDS; ++index) {
                const bool match = ai_it_raw_memory[index] ==
                    ai_it_raw_expected_memory[index];

                raw_memory_ok = raw_memory_ok && match;
                if (!match && raw_mismatch_index == AI_IT_RAW_MEM_WORDS) {
                    raw_mismatch_index = index;
                    raw_mismatch_actual = ai_it_raw_memory[index];
                    raw_mismatch_expected =
                        ai_it_raw_expected_memory[index];
                    raw_mismatch_pc = ai_it_raw_expected_last_write_pc[index];
                    raw_mismatch_word =
                        ai_it_raw_expected_last_write_word[index];
                }
            }
            raw_vector_ok = true;
            for (unsigned index = 0U; index < AI_IT_STREAM_VECTOR_BYTES;
                 ++index) {
                const bool match = ai_it_raw_vector_src[index] ==
                    ai_it_raw_expected_vector_src[index];

                raw_vector_ok = raw_vector_ok && match;
                if (!match && raw_vector_mismatch_region == 2U) {
                    raw_vector_mismatch_region = 0U;
                    raw_vector_mismatch_index = index;
                    raw_vector_mismatch_actual = ai_it_raw_vector_src[index];
                    raw_vector_mismatch_expected =
                        ai_it_raw_expected_vector_src[index];
                }
            }
            for (unsigned index = 0U; index < AI_IT_STREAM_VECTOR_STORAGE;
                 ++index) {
                const bool match = ai_it_raw_vector_dst[index] ==
                    ai_it_raw_expected_vector_dst[index];

                raw_vector_ok = raw_vector_ok && match;
                if (!match && raw_vector_mismatch_region == 2U) {
                    raw_vector_mismatch_region = 1U;
                    raw_vector_mismatch_index = index;
                    raw_vector_mismatch_actual = ai_it_raw_vector_dst[index];
                    raw_vector_mismatch_expected =
                        ai_it_raw_expected_vector_dst[index];
                }
            }
        }

        const bool generated_mix_ok = raw_build_ok &&
            ai_it_raw_required_mix(raw_stream.generated_hist);
        const bool executed_mix_ok = raw_reference.valid &&
            ai_it_raw_required_mix(raw_reference.executed_hist) &&
            raw_reference.taken_branches > 0U &&
            raw_reference.not_taken_branches > 0U;
        const bool cleanup_policy_ok = raw_build_ok &&
            ai_it_raw_cleanup_policy(cleanup_mode, raw_stream.generated_hist);
        const bool pressure_contract_ok =
            ai_stress_popcount(raw_pressure_mask) >= 6U &&
            (raw_cleanup_mask & (AI_STRESS_CLEANUP_FENCE |
                AI_STRESS_CLEANUP_CMO)) != 0U;
        const bool raw_randomized_ok = raw_build_ok &&
            raw_stream.unique_word_count >= raw_stream.body_words / 3U &&
            raw_stream.field_hash != 0ULL &&
            raw_stream.opcode_variants >= 8U &&
            raw_stream.funct3_variants >= 5U &&
            raw_stream.upper7_variants >= 3U &&
            raw_stream.semantic_variants >= 16U;
        const bool raw_result_ok = raw_build_ok && raw_reference.valid &&
            raw_actual_result == raw_expected_result && raw_memory_ok &&
            raw_vector_ok && raw_actual_memory_signature ==
                raw_expected_memory_signature && raw_actual_vector_signature ==
                raw_expected_vector_signature && raw_hash_unique &&
            raw_randomized_ok;
        const bool seed_ok = raw_result_ok && generated_mix_ok &&
            executed_mix_ok && cleanup_policy_ok && pressure_contract_ok &&
            raw_hashes_unique && excpt.triggered == false;
        uint64_t signature = ai_stress_signature_fold(raw_stream.word_hash,
            raw_stream.field_hash);
        signature = ai_stress_signature_fold(signature, raw_actual_result);
        signature = ai_stress_signature_fold(signature,
            raw_actual_memory_signature);
        signature = ai_stress_signature_fold(signature,
            raw_actual_vector_signature);

        TEST_ASSERT("runtime raw-word stream matches register, memory, and vector scoreboards",
            seed_ok,
            "seed=0x%llx shard=%llu slots=%u words=%u body_words=%u build=%d valid=%d generated_mix=%d executed_mix=%d cleanup=%d pressure=0x%x cleanup_mask=0x%x result_ok=%d mem_ok=%d vector_ok=%d vector_mismatch=%u:%u:0x%x/0x%x unique=%u opcode_variants=%u funct3_variants=%u upper7_variants=%u semantic_variants=%u variant_mask=0x%llx field_hash=0x%llx word_hash=0x%llx actual=0x%llx expected=0x%llx first_mismatch=%u:0x%llx/0x%llx producer_pc=%u producer_word=0x%08x",
            (unsigned long long)seed,
            (unsigned long long)AI_IT_STRESS_SHARD_ID, body_slots,
            raw_stream.word_count, raw_stream.body_words, raw_build_ok,
            raw_reference.valid, generated_mix_ok, executed_mix_ok,
            cleanup_policy_ok, raw_pressure_mask, raw_cleanup_mask,
            raw_result_ok, raw_memory_ok, raw_vector_ok,
            raw_vector_mismatch_region, raw_vector_mismatch_index,
            raw_vector_mismatch_actual, raw_vector_mismatch_expected,
            raw_stream.unique_word_count,
            raw_stream.opcode_variants, raw_stream.funct3_variants,
            raw_stream.upper7_variants, raw_stream.semantic_variants,
            (unsigned long long)raw_stream.semantic_variant_mask,
            (unsigned long long)raw_stream.field_hash,
            (unsigned long long)raw_stream.word_hash,
            (unsigned long long)raw_actual_result,
            (unsigned long long)raw_expected_result, raw_mismatch_index,
            (unsigned long long)raw_mismatch_actual,
            (unsigned long long)raw_mismatch_expected, raw_mismatch_pc,
            raw_mismatch_word);
        ai_it_log_raw_stream(seed, round, &raw_stream, &raw_reference,
            raw_amo_word, raw_actual_result, raw_expected_result,
            raw_actual_memory_signature, raw_expected_memory_signature,
            raw_actual_vector_signature, raw_expected_vector_signature,
            raw_pressure_mask, raw_cleanup_mask, address_mode,
            dependency_mode, branch_mode, cleanup_mode, raw_hash_unique,
            seed_ok);
    }

    CSRW(CSR_VSTART, 0ULL);
    TEST_END();
}
