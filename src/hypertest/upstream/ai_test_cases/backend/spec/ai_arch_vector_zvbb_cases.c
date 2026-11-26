#include <rvh_test.h>
#include <stdbool.h>
#include <stdint.h>

#define AI_ZVBB_VECTOR_BYTES 16U
#define AI_ZVBB_SLOT_BYTES 32U
#define AI_ZVBB_SEW_COUNT 4U

enum ai_zvbb_operation {
    AI_ZVBB_VANDN_VV,
    AI_ZVBB_VANDN_VX,
    AI_ZVBB_VROL_VV,
    AI_ZVBB_VROL_VX,
    AI_ZVBB_VROR_VV,
    AI_ZVBB_VROR_VX,
    AI_ZVBB_VROR_VI,
    AI_ZVBB_VWSLL_VV,
    AI_ZVBB_VWSLL_VX,
    AI_ZVBB_VWSLL_VI,
    AI_ZVBB_VBREV_V,
    AI_ZVBB_VBREV8_V,
    AI_ZVBB_VREV8_V,
    AI_ZVBB_VCLZ_V,
    AI_ZVBB_VCTZ_V,
    AI_ZVBB_VCPOP_V,
    AI_ZVBB_OPERATION_COUNT
};

static uint8_t ai_zvbb_source_a[AI_ZVBB_SEW_COUNT][AI_ZVBB_VECTOR_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_zvbb_source_b[AI_ZVBB_SEW_COUNT][AI_ZVBB_VECTOR_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_zvbb_observed[AI_ZVBB_SEW_COUNT]
    [AI_ZVBB_OPERATION_COUNT][AI_ZVBB_SLOT_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_zvbb_mask_old[AI_ZVBB_VECTOR_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_zvbb_mask_bits[2] __attribute__((aligned(64)));
static uint8_t ai_zvbb_mask_observed[AI_ZVBB_VECTOR_BYTES]
    __attribute__((aligned(64)));

#define AI_ZVBB_RUNNER(name_, sew_, load_, store_, wide_store_) \
static inline void name_(const uint8_t *source_a, const uint8_t *source_b, \
    uint8_t *observed, uint64_t scalar, unsigned vl) \
{ \
    asm volatile( \
        ".option push\n\t" \
        ".option arch,+zvbb\n\t" \
        "vsetvli t0, %[vl], " sew_ ", m1, ta, ma\n\t" \
        load_ " v8, (%[source_a])\n\t" \
        load_ " v10, (%[source_b])\n\t" \
        "mv t1, %[observed]\n\t" \
        "vandn.vv v12, v8, v10\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vandn.vx v12, v8, %[scalar]\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vrol.vv v12, v8, v10\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vrol.vx v12, v8, %[scalar]\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vror.vv v12, v8, v10\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vror.vx v12, v8, %[scalar]\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vror.vi v12, v8, 7\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vwsll.vv v12, v8, v10\n\t" \
        wide_store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vwsll.vx v12, v8, %[scalar]\n\t" \
        wide_store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vwsll.vi v12, v8, 5\n\t" \
        wide_store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vbrev.v v12, v8\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vbrev8.v v12, v8\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vrev8.v v12, v8\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vclz.v v12, v8\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vctz.v v12, v8\n\t" \
        store_ " v12, (t1)\n\t" \
        "addi t1, t1, 32\n\t" \
        "vcpop.v v12, v8\n\t" \
        store_ " v12, (t1)\n\t" \
        ".option pop\n\t" \
        : \
        : [source_a] "r"(source_a), [source_b] "r"(source_b), \
          [observed] "r"(observed), [scalar] "r"(scalar), [vl] "r"(vl) \
        : "t0", "t1", "memory"); \
}

AI_ZVBB_RUNNER(ai_zvbb_run_e8, "e8", "vle8.v", "vse8.v", "vse16.v")
AI_ZVBB_RUNNER(ai_zvbb_run_e16, "e16", "vle16.v", "vse16.v", "vse32.v")
AI_ZVBB_RUNNER(ai_zvbb_run_e32, "e32", "vle32.v", "vse32.v", "vse64.v")

static inline void ai_zvbb_run_e64(const uint8_t *source_a,
    const uint8_t *source_b, uint8_t *observed, uint64_t scalar, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option arch,+zvbb\n\t"
        "vsetvli t0, %[vl], e64, m1, ta, ma\n\t"
        "vle64.v v8, (%[source_a])\n\t"
        "vle64.v v10, (%[source_b])\n\t"
        "mv t1, %[observed]\n\t"
        "vandn.vv v12, v8, v10\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vandn.vx v12, v8, %[scalar]\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vrol.vv v12, v8, v10\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vrol.vx v12, v8, %[scalar]\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vror.vv v12, v8, v10\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vror.vx v12, v8, %[scalar]\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vror.vi v12, v8, 7\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 128\n\t"
        "vbrev.v v12, v8\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vbrev8.v v12, v8\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vrev8.v v12, v8\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vclz.v v12, v8\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vctz.v v12, v8\n\t"
        "vse64.v v12, (t1)\n\t"
        "addi t1, t1, 32\n\t"
        "vcpop.v v12, v8\n\t"
        "vse64.v v12, (t1)\n\t"
        ".option pop\n\t"
        :
        : [source_a] "r"(source_a), [source_b] "r"(source_b),
          [observed] "r"(observed), [scalar] "r"(scalar), [vl] "r"(vl)
        : "t0", "t1", "memory");
}

static inline uint64_t ai_zvbb_mask(unsigned bits)
{
    return bits == 64U ? UINT64_MAX : ((1ULL << bits) - 1ULL);
}

static inline uint64_t ai_zvbb_read_le(const uint8_t *data, unsigned bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < bytes; ++i) {
        value |= (uint64_t)data[i] << (8U * i);
    }
    return value;
}

static inline void ai_zvbb_write_le(uint8_t *data, unsigned bytes,
    uint64_t value)
{
    for (unsigned i = 0; i < bytes; ++i) {
        data[i] = (uint8_t)(value >> (8U * i));
    }
}

static uint64_t ai_zvbb_rol(uint64_t value, uint64_t shamt, unsigned bits)
{
    const uint64_t mask = ai_zvbb_mask(bits);
    const unsigned shift = (unsigned)shamt & (bits - 1U);

    value &= mask;
    if (shift == 0U) {
        return value;
    }
    return ((value << shift) | (value >> (bits - shift))) & mask;
}

static uint64_t ai_zvbb_ror(uint64_t value, uint64_t shamt, unsigned bits)
{
    const uint64_t mask = ai_zvbb_mask(bits);
    const unsigned shift = (unsigned)shamt & (bits - 1U);

    value &= mask;
    if (shift == 0U) {
        return value;
    }
    return ((value >> shift) | (value << (bits - shift))) & mask;
}

static uint64_t ai_zvbb_reverse_bits(uint64_t value, unsigned bits)
{
    uint64_t result = 0ULL;

    for (unsigned i = 0; i < bits; ++i) {
        result |= ((value >> i) & 1ULL) << (bits - 1U - i);
    }
    return result;
}

static uint64_t ai_zvbb_reverse_bits_in_bytes(uint64_t value,
    unsigned bits)
{
    uint64_t result = 0ULL;

    for (unsigned byte = 0; byte < bits / 8U; ++byte) {
        const uint8_t source = (uint8_t)(value >> (8U * byte));
        result |= (ai_zvbb_reverse_bits(source, 8U) & 0xffULL) <<
            (8U * byte);
    }
    return result;
}

static uint64_t ai_zvbb_reverse_bytes(uint64_t value, unsigned bits)
{
    uint64_t result = 0ULL;
    const unsigned bytes = bits / 8U;

    for (unsigned byte = 0; byte < bytes; ++byte) {
        result |= ((value >> (8U * byte)) & 0xffULL) <<
            (8U * (bytes - 1U - byte));
    }
    return result;
}

static uint64_t ai_zvbb_clz(uint64_t value, unsigned bits)
{
    uint64_t count = 0ULL;

    for (unsigned i = bits; i > 0U; --i) {
        if (((value >> (i - 1U)) & 1ULL) != 0ULL) {
            break;
        }
        ++count;
    }
    return count;
}

static uint64_t ai_zvbb_ctz(uint64_t value, unsigned bits)
{
    uint64_t count = 0ULL;

    for (unsigned i = 0; i < bits; ++i) {
        if (((value >> i) & 1ULL) != 0ULL) {
            break;
        }
        ++count;
    }
    return count;
}

static uint64_t ai_zvbb_cpop(uint64_t value, unsigned bits)
{
    uint64_t count = 0ULL;

    for (unsigned i = 0; i < bits; ++i) {
        count += (value >> i) & 1ULL;
    }
    return count;
}

static uint64_t ai_zvbb_expected(enum ai_zvbb_operation operation,
    uint64_t lhs, uint64_t rhs, uint64_t scalar, unsigned bits)
{
    const uint64_t mask = ai_zvbb_mask(bits);

    switch (operation) {
    case AI_ZVBB_VANDN_VV:
        return lhs & ~rhs & mask;
    case AI_ZVBB_VANDN_VX:
        return lhs & ~scalar & mask;
    case AI_ZVBB_VROL_VV:
        return ai_zvbb_rol(lhs, rhs, bits);
    case AI_ZVBB_VROL_VX:
        return ai_zvbb_rol(lhs, scalar, bits);
    case AI_ZVBB_VROR_VV:
        return ai_zvbb_ror(lhs, rhs, bits);
    case AI_ZVBB_VROR_VX:
        return ai_zvbb_ror(lhs, scalar, bits);
    case AI_ZVBB_VROR_VI:
        return ai_zvbb_ror(lhs, 7U, bits);
    case AI_ZVBB_VBREV_V:
        return ai_zvbb_reverse_bits(lhs, bits);
    case AI_ZVBB_VBREV8_V:
        return ai_zvbb_reverse_bits_in_bytes(lhs, bits);
    case AI_ZVBB_VREV8_V:
        return ai_zvbb_reverse_bytes(lhs, bits);
    case AI_ZVBB_VCLZ_V:
        return ai_zvbb_clz(lhs, bits);
    case AI_ZVBB_VCTZ_V:
        return ai_zvbb_ctz(lhs, bits);
    case AI_ZVBB_VCPOP_V:
        return ai_zvbb_cpop(lhs, bits);
    default:
        return 0ULL;
    }
}

static uint64_t ai_zvbb_widen_expected(uint64_t value, uint64_t shamt,
    unsigned bits)
{
    const unsigned wide_bits = bits * 2U;
    const unsigned shift = (unsigned)shamt & (wide_bits - 1U);

    return (value << shift) & ai_zvbb_mask(wide_bits);
}

static void ai_zvbb_prepare_sources(void)
{
    for (unsigned sew_index = 0; sew_index < AI_ZVBB_SEW_COUNT;
         ++sew_index) {
        const unsigned bits = 8U << sew_index;
        const unsigned bytes = bits / 8U;
        const unsigned elements = AI_ZVBB_VECTOR_BYTES / bytes;
        const uint64_t mask = ai_zvbb_mask(bits);

        for (unsigned i = 0; i < elements; ++i) {
            uint64_t lhs = (0x9e3779b97f4a7c15ULL * (i + 1U)) & mask;
            uint64_t rhs = (0xd1b54a32d192ed03ULL ^
                (0x0101010101010101ULL * i)) & mask;

            if (i == 0U) {
                lhs = 0ULL;
            } else if (i == 1U) {
                lhs = ((1ULL << (bits - 1U)) | 1ULL) & mask;
            }
            ai_zvbb_write_le(&ai_zvbb_source_a[sew_index][i * bytes],
                bytes, lhs);
            ai_zvbb_write_le(&ai_zvbb_source_b[sew_index][i * bytes],
                bytes, rhs);
        }
    }

    for (unsigned i = 0; i < AI_ZVBB_VECTOR_BYTES; ++i) {
        ai_zvbb_mask_old[i] = (uint8_t)(0x80U + i);
        ai_zvbb_mask_observed[i] = 0U;
    }
    ai_zvbb_mask_bits[0] = 0xadU;
    ai_zvbb_mask_bits[1] = 0x15U;
}

static unsigned ai_zvbb_normal_mismatches(uint64_t scalar)
{
    unsigned mismatches = 0U;

    for (unsigned sew_index = 0; sew_index < AI_ZVBB_SEW_COUNT;
         ++sew_index) {
        const unsigned bits = 8U << sew_index;
        const unsigned bytes = bits / 8U;
        const unsigned elements = AI_ZVBB_VECTOR_BYTES / bytes;

        for (unsigned operation = AI_ZVBB_VANDN_VV;
             operation <= AI_ZVBB_VROR_VI; ++operation) {
            for (unsigned i = 0; i < elements; ++i) {
                const uint64_t lhs = ai_zvbb_read_le(
                    &ai_zvbb_source_a[sew_index][i * bytes], bytes);
                const uint64_t rhs = ai_zvbb_read_le(
                    &ai_zvbb_source_b[sew_index][i * bytes], bytes);
                const uint64_t actual = ai_zvbb_read_le(
                    &ai_zvbb_observed[sew_index][operation][i * bytes],
                    bytes);
                mismatches += actual != ai_zvbb_expected(
                    (enum ai_zvbb_operation)operation, lhs, rhs, scalar,
                    bits);
            }
        }
    }
    return mismatches;
}

static unsigned ai_zvbb_widen_mismatches(uint64_t scalar)
{
    unsigned mismatches = 0U;

    for (unsigned sew_index = 0; sew_index < 3U; ++sew_index) {
        const unsigned bits = 8U << sew_index;
        const unsigned bytes = bits / 8U;
        const unsigned elements = AI_ZVBB_VECTOR_BYTES / bytes;

        for (unsigned i = 0; i < elements; ++i) {
            const uint64_t lhs = ai_zvbb_read_le(
                &ai_zvbb_source_a[sew_index][i * bytes], bytes);
            const uint64_t rhs = ai_zvbb_read_le(
                &ai_zvbb_source_b[sew_index][i * bytes], bytes);
            const uint64_t vv = ai_zvbb_read_le(
                &ai_zvbb_observed[sew_index][AI_ZVBB_VWSLL_VV]
                    [i * bytes * 2U], bytes * 2U);
            const uint64_t vx = ai_zvbb_read_le(
                &ai_zvbb_observed[sew_index][AI_ZVBB_VWSLL_VX]
                    [i * bytes * 2U], bytes * 2U);
            const uint64_t vi = ai_zvbb_read_le(
                &ai_zvbb_observed[sew_index][AI_ZVBB_VWSLL_VI]
                    [i * bytes * 2U], bytes * 2U);

            mismatches += vv != ai_zvbb_widen_expected(lhs, rhs, bits);
            mismatches += vx != ai_zvbb_widen_expected(lhs, scalar, bits);
            mismatches += vi != ai_zvbb_widen_expected(lhs, 5U, bits);
        }
    }
    return mismatches;
}

static unsigned ai_zvbb_unary_mismatches(uint64_t scalar)
{
    unsigned mismatches = 0U;

    for (unsigned sew_index = 0; sew_index < AI_ZVBB_SEW_COUNT;
         ++sew_index) {
        const unsigned bits = 8U << sew_index;
        const unsigned bytes = bits / 8U;
        const unsigned elements = AI_ZVBB_VECTOR_BYTES / bytes;

        for (unsigned operation = AI_ZVBB_VBREV_V;
             operation <= AI_ZVBB_VCPOP_V; ++operation) {
            for (unsigned i = 0; i < elements; ++i) {
                const uint64_t lhs = ai_zvbb_read_le(
                    &ai_zvbb_source_a[sew_index][i * bytes], bytes);
                const uint64_t actual = ai_zvbb_read_le(
                    &ai_zvbb_observed[sew_index][operation][i * bytes],
                    bytes);
                mismatches += actual != ai_zvbb_expected(
                    (enum ai_zvbb_operation)operation, lhs, 0ULL, scalar,
                    bits);
            }
        }
    }
    return mismatches;
}

static inline void ai_zvbb_run_mask_tail_probe(void)
{
    const unsigned full_vl = AI_ZVBB_VECTOR_BYTES;
    const unsigned active_vl = 13U;

    asm volatile(
        ".option push\n\t"
        ".option arch,+zvbb\n\t"
        "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[source_a])\n\t"
        "vle8.v v10, (%[source_b])\n\t"
        "vle8.v v12, (%[old])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vsetvli t0, %[active_vl], e8, m1, tu, mu\n\t"
        "vandn.vv v12, v8, v10, v0.t\n\t"
        "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
        "vse8.v v12, (%[observed])\n\t"
        ".option pop\n\t"
        :
        : [source_a] "r"(ai_zvbb_source_a[0]),
          [source_b] "r"(ai_zvbb_source_b[0]),
          [old] "r"(ai_zvbb_mask_old), [mask] "r"(ai_zvbb_mask_bits),
          [observed] "r"(ai_zvbb_mask_observed),
          [full_vl] "r"(full_vl), [active_vl] "r"(active_vl)
        : "t0", "memory");
}

static bool ai_zvbb_mask_tail_matches(void)
{
    for (unsigned i = 0; i < AI_ZVBB_VECTOR_BYTES; ++i) {
        const bool active = i < 13U &&
            ((ai_zvbb_mask_bits[i / 8U] >> (i % 8U)) & 1U) != 0U;
        const uint8_t expected = active ?
            (uint8_t)(ai_zvbb_source_a[0][i] &
                ~ai_zvbb_source_b[0][i]) : ai_zvbb_mask_old[i];

        if (ai_zvbb_mask_observed[i] != expected) {
            return false;
        }
    }
    return true;
}

bool ai_arch_vector_zvbb_full_opcode_sew_reference_matrix_corner()
{
    TEST_START();

    const uint64_t scalar = 13ULL;
    uint64_t vlenb;
    unsigned normal_mismatches = 0U;
    unsigned widen_mismatches = 0U;
    unsigned unary_mismatches = 0U;
    bool mask_tail_matches = false;

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
    asm volatile("csrr %0, vlenb" : "=r"(vlenb));

    TEST_ASSERT("Zvbb reference matrix requires at least one 128-bit vector register",
        vlenb >= AI_ZVBB_VECTOR_BYTES,
        "vlenb=%llu", (unsigned long long)vlenb);

    if (vlenb >= AI_ZVBB_VECTOR_BYTES) {
        ai_zvbb_prepare_sources();
        TEST_SETUP_EXCEPT();

        ai_zvbb_run_e8(ai_zvbb_source_a[0], ai_zvbb_source_b[0],
            &ai_zvbb_observed[0][0][0], scalar, 16U);
        ai_zvbb_run_e16(ai_zvbb_source_a[1], ai_zvbb_source_b[1],
            &ai_zvbb_observed[1][0][0], scalar, 8U);
        ai_zvbb_run_e32(ai_zvbb_source_a[2], ai_zvbb_source_b[2],
            &ai_zvbb_observed[2][0][0], scalar, 4U);
        ai_zvbb_run_e64(ai_zvbb_source_a[3], ai_zvbb_source_b[3],
            &ai_zvbb_observed[3][0][0], scalar, 2U);
        ai_zvbb_run_mask_tail_probe();

        normal_mismatches = ai_zvbb_normal_mismatches(scalar);
        widen_mismatches = ai_zvbb_widen_mismatches(scalar);
        unary_mismatches = ai_zvbb_unary_mismatches(scalar);
        mask_tail_matches = ai_zvbb_mask_tail_matches();

        TEST_ASSERT("Zvbb VANDN/VROL/VROR vv/vx/vi forms match independent models at SEW 8/16/32/64",
            normal_mismatches == 0U,
            "mismatches=%u", normal_mismatches);
        TEST_ASSERT("Zvbb VWSLL vv/vx/vi widening forms match independent models at every legal source SEW 8/16/32",
            widen_mismatches == 0U,
            "mismatches=%u", widen_mismatches);
        TEST_ASSERT("Zvbb VBREV/VBREV8/VREV8/VCLZ/VCTZ/VCPOP match independent per-element models at SEW 8/16/32/64",
            unary_mismatches == 0U,
            "mismatches=%u", unary_mismatches);
        TEST_ASSERT("Zvbb masked operation preserves masked-off and tail elements under tu,mu policy",
            mask_tail_matches);
        TEST_ASSERT("all implemented Zvbb opcode forms execute without exception",
            excpt.triggered == false);
    }

    TEST_END();
}
