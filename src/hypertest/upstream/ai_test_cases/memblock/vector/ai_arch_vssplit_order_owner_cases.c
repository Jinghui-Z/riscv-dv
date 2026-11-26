#include <rvh_test.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_ARCH_VSSPLIT_ORDER_BYTES 96U
#define AI_ARCH_VSSPLIT_ORDER_VL 16U
#define AI_ARCH_VSSPLIT_ORDER_OBS 6U
#define AI_ARCH_VSSPLIT_ORDER_SCALAR_ATTR \
    __attribute__((noinline, optimize("no-tree-vectorize", \
                                      "no-tree-loop-distribute-patterns")))

static uint8_t ai_arch_vssplit_unit_mem[AI_ARCH_VSSPLIT_ORDER_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vssplit_strided_mem[AI_ARCH_VSSPLIT_ORDER_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vssplit_masked_mem[AI_ARCH_VSSPLIT_ORDER_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vssplit_src[AI_ARCH_VSSPLIT_ORDER_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vssplit_mask[2] __attribute__((aligned(8)));
static uint64_t ai_arch_vssplit_marker[4] __attribute__((aligned(64)));

static AI_ARCH_VSSPLIT_ORDER_SCALAR_ATTR uint8_t
ai_arch_vssplit_seed_byte(unsigned index, uint8_t salt)
{
    return (uint8_t)(salt + (uint8_t)(index * 19U) ^
        (uint8_t)(index << 1));
}

static AI_ARCH_VSSPLIT_ORDER_SCALAR_ATTR void
ai_arch_vssplit_fill_pair(uint8_t *actual, uint8_t *expected,
    unsigned bytes, uint8_t salt)
{
    volatile uint8_t *actual_v = (volatile uint8_t *)actual;
    volatile uint8_t *expected_v = (volatile uint8_t *)expected;

    for (unsigned i = 0; i < bytes; ++i) {
        const uint8_t value = ai_arch_vssplit_seed_byte(i, salt);

        actual_v[i] = value;
        expected_v[i] = value;
    }
}

static AI_ARCH_VSSPLIT_ORDER_SCALAR_ATTR void
ai_arch_vssplit_fill_u64(uint64_t *dst, unsigned count, uint64_t value)
{
    volatile uint64_t *dst_v = (volatile uint64_t *)dst;

    for (unsigned i = 0; i < count; ++i) {
        dst_v[i] = value ^ ((uint64_t)i << 52);
    }
}

static AI_ARCH_VSSPLIT_ORDER_SCALAR_ATTR bool
ai_arch_vssplit_bytes_equal(const uint8_t *actual, const uint8_t *expected,
    unsigned bytes, unsigned *bad_index)
{
    volatile const uint8_t *actual_v = (volatile const uint8_t *)actual;
    volatile const uint8_t *expected_v = (volatile const uint8_t *)expected;

    for (unsigned i = 0; i < bytes; ++i) {
        if (actual_v[i] != expected_v[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = 0U;
    return true;
}

static inline void ai_arch_vssplit_apply_u64(uint8_t *expected,
    unsigned offset, uint64_t value)
{
    volatile uint8_t *expected_v = (volatile uint8_t *)expected;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        expected_v[offset + i] = (uint8_t)(value >> (8U * i));
    }
}

static inline uint64_t ai_arch_vssplit_load_le64(const uint8_t *bytes,
    unsigned offset)
{
    volatile const uint8_t *bytes_v = (volatile const uint8_t *)bytes;
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < 8U; ++i) {
        value |= ((uint64_t)bytes_v[offset + i]) << (8U * i);
    }

    return value;
}

static inline void ai_arch_vssplit_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_arch_vssplit_unit_episode(uintptr_t base,
    const uint8_t *src, uintptr_t marker, uint64_t first_payload,
    uint64_t second_payload, uint64_t marker_value, uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v8, (%[src])\n\t"
        "sd %[first_payload], 13(%[base])\n\t"
        "sd %[second_payload], 29(%[base])\n\t"
        "addi t1, %[base], 15\n\t"
        "vse8.v v8, (t1)\n\t"
        "lbu t2, 15(%[base])\n\t"
        "sd t2, 0(%[obs])\n\t"
        "lbu t2, 20(%[base])\n\t"
        "sd t2, 8(%[obs])\n\t"
        "lbu t2, 29(%[base])\n\t"
        "sd t2, 16(%[obs])\n\t"
        "lbu t2, 30(%[base])\n\t"
        "sd t2, 24(%[obs])\n\t"
        "csrr t2, vstart\n\t"
        "sd t2, 32(%[obs])\n\t"
        "sd %[marker_value], 0(%[marker])\n\t"
        "ld t2, 0(%[marker])\n\t"
        "sd t2, 40(%[obs])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [src] "r"(src),
          [marker] "r"(marker),
          [first_payload] "r"(first_payload),
          [second_payload] "r"(second_payload),
          [marker_value] "r"(marker_value),
          [obs] "r"(obs),
          [vl] "r"((unsigned)AI_ARCH_VSSPLIT_ORDER_VL)
        : "t0", "t1", "t2", "memory");
}

static inline void ai_arch_vssplit_strided_episode(uintptr_t base,
    const uint8_t *src, uintptr_t marker, uint64_t scalar_payload,
    uint64_t marker_value, uint64_t *obs)
{
    const uintptr_t stride = 2U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v8, (%[src])\n\t"
        "sd %[scalar_payload], 13(%[base])\n\t"
        "addi t1, %[base], 14\n\t"
        "vsse8.v v8, (t1), %[stride]\n\t"
        "lbu t2, 14(%[base])\n\t"
        "sd t2, 0(%[obs])\n\t"
        "lbu t2, 16(%[base])\n\t"
        "sd t2, 8(%[obs])\n\t"
        "lbu t2, 18(%[base])\n\t"
        "sd t2, 16(%[obs])\n\t"
        "lbu t2, 20(%[base])\n\t"
        "sd t2, 24(%[obs])\n\t"
        "csrr t2, vstart\n\t"
        "sd t2, 32(%[obs])\n\t"
        "sd %[marker_value], 0(%[marker])\n\t"
        "ld t2, 0(%[marker])\n\t"
        "sd t2, 40(%[obs])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [src] "r"(src),
          [marker] "r"(marker),
          [scalar_payload] "r"(scalar_payload),
          [marker_value] "r"(marker_value),
          [obs] "r"(obs),
          [stride] "r"(stride),
          [vl] "r"((unsigned)AI_ARCH_VSSPLIT_ORDER_VL)
        : "t0", "t1", "t2", "memory");
}

static inline void ai_arch_vssplit_masked_episode(uintptr_t base,
    const uint8_t *src, const uint8_t *mask, uintptr_t marker,
    uint64_t first_payload, uint64_t second_payload, uint64_t marker_value,
    uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v8, (%[src])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "sd %[first_payload], 13(%[base])\n\t"
        "sd %[second_payload], 29(%[base])\n\t"
        "addi t1, %[base], 15\n\t"
        "vse8.v v8, (t1), v0.t\n\t"
        "lbu t2, 15(%[base])\n\t"
        "sd t2, 0(%[obs])\n\t"
        "lbu t2, 16(%[base])\n\t"
        "sd t2, 8(%[obs])\n\t"
        "lbu t2, 17(%[base])\n\t"
        "sd t2, 16(%[obs])\n\t"
        "lbu t2, 20(%[base])\n\t"
        "sd t2, 24(%[obs])\n\t"
        "csrr t2, vstart\n\t"
        "sd t2, 32(%[obs])\n\t"
        "sd %[marker_value], 0(%[marker])\n\t"
        "ld t2, 0(%[marker])\n\t"
        "sd t2, 40(%[obs])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [src] "r"(src),
          [mask] "r"(mask),
          [marker] "r"(marker),
          [first_payload] "r"(first_payload),
          [second_payload] "r"(second_payload),
          [marker_value] "r"(marker_value),
          [obs] "r"(obs),
          [vl] "r"((unsigned)AI_ARCH_VSSPLIT_ORDER_VL)
        : "t0", "t1", "t2", "memory");
}

#pragma GCC push_options
#pragma GCC optimize ("no-tree-vectorize", "no-tree-loop-distribute-patterns")
bool ai_arch_vssplit_older_cross16_younger_vector_order_corner()
{
    TEST_START();

    enum {
        WINDOW_OFF = 16U,
        SCALAR_A_OFF = 13U,
        SCALAR_B_OFF = 29U,
        UNIT_VECTOR_OFF = 15U,
        STRIDED_VECTOR_OFF = 14U,
    };
    const uint64_t scalar_a = 0x1021324354657687ULL;
    const uint64_t scalar_b = 0x8877665544332211ULL;
    const uint64_t scalar_c = 0xcafed00d13579bdfULL;
    const uint64_t marker_seed = 0x0badfeed12345678ULL;
    const uint64_t unit_marker = 0xa0a1a2a3a4a5a6a7ULL;
    const uint64_t strided_marker = 0xb0b1b2b3b4b5b6b7ULL;
    const uint64_t masked_marker = 0xc0c1c2c3c4c5c6c7ULL;
    uint8_t expected_unit[AI_ARCH_VSSPLIT_ORDER_BYTES];
    uint8_t expected_strided[AI_ARCH_VSSPLIT_ORDER_BYTES];
    uint8_t expected_masked[AI_ARCH_VSSPLIT_ORDER_BYTES];
    uint64_t unit_obs[AI_ARCH_VSSPLIT_ORDER_OBS];
    uint64_t strided_obs[AI_ARCH_VSSPLIT_ORDER_OBS];
    uint64_t masked_obs[AI_ARCH_VSSPLIT_ORDER_OBS];
    unsigned bad_index = 0U;

    ai_arch_vssplit_enable_vector_state();
    for (unsigned i = 0; i < AI_ARCH_VSSPLIT_ORDER_VL; ++i) {
        ai_arch_vssplit_src[i] = (uint8_t)(0x31U + (uint8_t)(i * 11U));
    }
    ai_arch_vssplit_mask[0] = 0xadU;
    ai_arch_vssplit_mask[1] = 0x53U;

    ai_arch_vssplit_fill_pair(ai_arch_vssplit_unit_mem, expected_unit,
        AI_ARCH_VSSPLIT_ORDER_BYTES, 0x21U);
    ai_arch_vssplit_fill_pair(ai_arch_vssplit_strided_mem, expected_strided,
        AI_ARCH_VSSPLIT_ORDER_BYTES, 0x43U);
    ai_arch_vssplit_fill_pair(ai_arch_vssplit_masked_mem, expected_masked,
        AI_ARCH_VSSPLIT_ORDER_BYTES, 0x65U);
    ai_arch_vssplit_fill_u64(unit_obs, AI_ARCH_VSSPLIT_ORDER_OBS,
        0xbad1000000000000ULL);
    ai_arch_vssplit_fill_u64(strided_obs, AI_ARCH_VSSPLIT_ORDER_OBS,
        0xbad2000000000000ULL);
    ai_arch_vssplit_fill_u64(masked_obs, AI_ARCH_VSSPLIT_ORDER_OBS,
        0xbad3000000000000ULL);
    ai_arch_vssplit_marker[0] = marker_seed;
    ai_arch_vssplit_marker[1] = marker_seed ^ 0x1111111111111111ULL;
    ai_arch_vssplit_marker[2] = marker_seed ^ 0x2222222222222222ULL;
    ai_arch_vssplit_marker[3] = 0xfeedfacecafebeefULL;

    ai_arch_vssplit_apply_u64(expected_unit, WINDOW_OFF + SCALAR_A_OFF,
        scalar_a);
    ai_arch_vssplit_apply_u64(expected_unit, WINDOW_OFF + SCALAR_B_OFF,
        scalar_b);
    for (unsigned i = 0; i < AI_ARCH_VSSPLIT_ORDER_VL; ++i) {
        expected_unit[WINDOW_OFF + UNIT_VECTOR_OFF + i] =
            ai_arch_vssplit_src[i];
    }

    ai_arch_vssplit_apply_u64(expected_strided, WINDOW_OFF + SCALAR_A_OFF,
        scalar_c);
    for (unsigned i = 0; i < AI_ARCH_VSSPLIT_ORDER_VL; ++i) {
        expected_strided[WINDOW_OFF + STRIDED_VECTOR_OFF + 2U * i] =
            ai_arch_vssplit_src[i];
    }

    ai_arch_vssplit_apply_u64(expected_masked, WINDOW_OFF + SCALAR_A_OFF,
        scalar_a);
    ai_arch_vssplit_apply_u64(expected_masked, WINDOW_OFF + SCALAR_B_OFF,
        scalar_b);
    for (unsigned i = 0; i < AI_ARCH_VSSPLIT_ORDER_VL; ++i) {
        const bool active =
            ((ai_arch_vssplit_mask[i >> 3] >> (i & 7U)) & 1U) != 0U;

        if (active) {
            expected_masked[WINDOW_OFF + UNIT_VECTOR_OFF + i] =
                ai_arch_vssplit_src[i];
        }
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vssplit_unit_episode(
        (uintptr_t)&ai_arch_vssplit_unit_mem[WINDOW_OFF],
        ai_arch_vssplit_src, (uintptr_t)&ai_arch_vssplit_marker[0],
        scalar_a, scalar_b, unit_marker, unit_obs);
    TEST_ASSERT("VSSplit ordering: older cross16 scalar stores followed by younger misaligned unit-stride vector store leave every overlapping byte owned by the younger vector episode",
        excpt.triggered == false &&
        unit_obs[0] == ai_arch_vssplit_src[0] &&
        unit_obs[1] == ai_arch_vssplit_src[5] &&
        unit_obs[2] == ai_arch_vssplit_src[14] &&
        unit_obs[3] == ai_arch_vssplit_src[15] &&
        unit_obs[4] == 0ULL &&
        unit_obs[5] == unit_marker &&
        ai_arch_vssplit_bytes_equal(ai_arch_vssplit_unit_mem,
            expected_unit, AI_ARCH_VSSPLIT_ORDER_BYTES, &bad_index),
        "bad=%u actual=0x%02x expected=0x%02x word16=0x%lx word24=0x%lx",
        bad_index, (unsigned)ai_arch_vssplit_unit_mem[bad_index],
        (unsigned)expected_unit[bad_index],
        ai_arch_vssplit_load_le64(ai_arch_vssplit_unit_mem, WINDOW_OFF + 16U),
        ai_arch_vssplit_load_le64(expected_unit, WINDOW_OFF + 16U)
    );

    TEST_SETUP_EXCEPT();
    ai_arch_vssplit_strided_episode(
        (uintptr_t)&ai_arch_vssplit_strided_mem[WINDOW_OFF],
        ai_arch_vssplit_src, (uintptr_t)&ai_arch_vssplit_marker[1],
        scalar_c, strided_marker, strided_obs);
    TEST_ASSERT("VSSplit ordering: older cross16 scalar store cannot cover bytes from a younger misaligned strided vector store that should retire later in program order",
        excpt.triggered == false &&
        strided_obs[0] == ai_arch_vssplit_src[0] &&
        strided_obs[1] == ai_arch_vssplit_src[1] &&
        strided_obs[2] == ai_arch_vssplit_src[2] &&
        strided_obs[3] == ai_arch_vssplit_src[3] &&
        strided_obs[4] == 0ULL &&
        strided_obs[5] == strided_marker &&
        ai_arch_vssplit_bytes_equal(ai_arch_vssplit_strided_mem,
            expected_strided, AI_ARCH_VSSPLIT_ORDER_BYTES, &bad_index),
        "bad=%u actual=0x%02x expected=0x%02x",
        bad_index, (unsigned)ai_arch_vssplit_strided_mem[bad_index],
        (unsigned)expected_strided[bad_index]
    );

    TEST_SETUP_EXCEPT();
    ai_arch_vssplit_masked_episode(
        (uintptr_t)&ai_arch_vssplit_masked_mem[WINDOW_OFF],
        ai_arch_vssplit_src, ai_arch_vssplit_mask,
        (uintptr_t)&ai_arch_vssplit_marker[2], scalar_a, scalar_b,
        masked_marker, masked_obs);
    TEST_ASSERT("VSSplit ordering: masked younger vector store updates active overlapping lanes after older cross16 scalar stores while inactive lanes retain the older scalar or seed image",
        excpt.triggered == false &&
        masked_obs[0] == ai_arch_vssplit_src[0] &&
        masked_obs[1] == ((scalar_a >> 24U) & 0xffU) &&
        masked_obs[2] == ai_arch_vssplit_src[2] &&
        masked_obs[3] == ai_arch_vssplit_src[5] &&
        masked_obs[4] == 0ULL &&
        masked_obs[5] == masked_marker &&
        ai_arch_vssplit_bytes_equal(ai_arch_vssplit_masked_mem,
            expected_masked, AI_ARCH_VSSPLIT_ORDER_BYTES, &bad_index),
        "bad=%u actual=0x%02x expected=0x%02x",
        bad_index, (unsigned)ai_arch_vssplit_masked_mem[bad_index],
        (unsigned)expected_masked[bad_index]
    );

    TEST_ASSERT("VSSplit ordering: marker guard survives all ordering episodes and proves forward progress is independent of the data target lines",
        ai_arch_vssplit_marker[0] == unit_marker &&
        ai_arch_vssplit_marker[1] == strided_marker &&
        ai_arch_vssplit_marker[2] == masked_marker &&
        ai_arch_vssplit_marker[3] == 0xfeedfacecafebeefULL
    );

    TEST_END();
}
#pragma GCC pop_options
