#include <rvh_test.h>
#include <stdint.h>

#define AI_ARCH_STRIDED_E16_VL 8U
#define AI_ARCH_STRIDED_E64_VL 4U
#define AI_ARCH_STRIDED_E16_START_LOAD 2U
#define AI_ARCH_STRIDED_E16_START_STORE 3U
#define AI_ARCH_STRIDED_E64_START 1U

static uint16_t ai_arch_strided_e16_seed[AI_ARCH_STRIDED_E16_VL]
    __attribute__((aligned(64)));
static uint16_t ai_arch_strided_e16_src[AI_ARCH_STRIDED_E16_VL]
    __attribute__((aligned(64)));
static uint16_t ai_arch_strided_e16_dump[AI_ARCH_STRIDED_E16_VL]
    __attribute__((aligned(64)));
static uint16_t ai_arch_strided_e16_store_vec[AI_ARCH_STRIDED_E16_VL]
    __attribute__((aligned(64)));
static uint16_t ai_arch_strided_e16_store_mem[AI_ARCH_STRIDED_E16_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_strided_e16_mask[1] __attribute__((aligned(8)));
static uint64_t ai_arch_strided_e64_seed[AI_ARCH_STRIDED_E64_VL]
    __attribute__((aligned(64)));
static uint64_t ai_arch_strided_e64_repeated __attribute__((aligned(64)));
static uint64_t ai_arch_strided_e64_dump[AI_ARCH_STRIDED_E64_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_strided_e64_mask[1] __attribute__((aligned(8)));

static inline void ai_arch_strided_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline bool ai_arch_strided_hwords_equal(const uint16_t *actual,
    const uint16_t *expected, unsigned count, unsigned *bad_index)
{
    for (unsigned i = 0; i < count; ++i) {
        if (actual[i] != expected[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = 0U;
    return true;
}

static inline bool ai_arch_strided_dwords_equal(const uint64_t *actual,
    const uint64_t *expected, unsigned count, unsigned *bad_index)
{
    for (unsigned i = 0; i < count; ++i) {
        if (actual[i] != expected[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = 0U;
    return true;
}

static inline void ai_arch_strided_masked_vlse16_negative_dump(
    const uint16_t *seed, const uint16_t *base, const uint8_t *mask,
    uint16_t *dump, int64_t stride, unsigned start,
    unsigned *observed_vstart)
{
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "vle16.v v8, (%[seed])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, %[start]\n\t"
        "vlse16.v v8, (%[base]), %[stride], v0.t\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vse16.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed),
          [base] "r"(base),
          [mask] "r"(mask),
          [dump] "r"(dump),
          [stride] "r"(stride),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_STRIDED_E16_VL)
        : "t0", "memory");

    *observed_vstart = captured_vstart;
}

static inline void ai_arch_strided_masked_vsse16_negative_store(
    const uint16_t *src, uint16_t *base, const uint8_t *mask,
    int64_t stride, unsigned start, unsigned *observed_vstart)
{
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "vle16.v v8, (%[src])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, %[start]\n\t"
        "vsse16.v v8, (%[base]), %[stride], v0.t\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(captured_vstart)
        : [src] "r"(src),
          [base] "r"(base),
          [mask] "r"(mask),
          [stride] "r"(stride),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_STRIDED_E16_VL)
        : "t0", "memory");

    *observed_vstart = captured_vstart;
}

static inline void ai_arch_strided_masked_vlse64_zero_dump(
    const uint64_t *seed, const uint64_t *base, const uint8_t *mask,
    uint64_t *dump, unsigned start, unsigned *observed_vstart)
{
    unsigned captured_vstart = 0U;
    const int64_t zero_stride = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "vle64.v v16, (%[seed])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, %[start]\n\t"
        "vlse64.v v16, (%[base]), %[stride], v0.t\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vse64.v v16, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed),
          [base] "r"(base),
          [mask] "r"(mask),
          [dump] "r"(dump),
          [stride] "r"(zero_stride),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_STRIDED_E64_VL)
        : "t0", "memory");

    *observed_vstart = captured_vstart;
}

bool ai_arch_strided_mask_vstart_overlap_owner_corner()
{
    TEST_START();

    uint16_t expected_load[AI_ARCH_STRIDED_E16_VL];
    uint16_t expected_store[AI_ARCH_STRIDED_E16_VL];
    uint64_t expected_zero[AI_ARCH_STRIDED_E64_VL];
    unsigned observed_vstart = 0U;
    unsigned bad_index = 0U;

    ai_arch_strided_enable_vector_state();

    for (unsigned i = 0; i < AI_ARCH_STRIDED_E16_VL; ++i) {
        ai_arch_strided_e16_seed[i] = (uint16_t)(0xa100U + i);
        ai_arch_strided_e16_src[i] = (uint16_t)(0x3100U + (0x11U * i));
        ai_arch_strided_e16_dump[i] = 0xeeeeU;
        ai_arch_strided_e16_store_vec[i] =
            (uint16_t)(0xc500U | (i << 4) | i);
        ai_arch_strided_e16_store_mem[i] = (uint16_t)(0x7700U | i);
        expected_load[i] = ai_arch_strided_e16_seed[i];
        expected_store[i] = ai_arch_strided_e16_store_mem[i];
    }

    ai_arch_strided_e16_mask[0] = 0xd4U;
    for (unsigned i = 0; i < AI_ARCH_STRIDED_E16_VL; ++i) {
        const bool active = ((ai_arch_strided_e16_mask[0] >> i) & 1U) != 0U;
        if (i >= AI_ARCH_STRIDED_E16_START_LOAD && active) {
            expected_load[i] =
                ai_arch_strided_e16_src[AI_ARCH_STRIDED_E16_VL - 1U - i];
        }
        if (i >= AI_ARCH_STRIDED_E16_START_STORE && active) {
            expected_store[AI_ARCH_STRIDED_E16_VL - 1U - i] =
                ai_arch_strided_e16_store_vec[i];
        }
    }

    TEST_SETUP_EXCEPT();
    ai_arch_strided_masked_vlse16_negative_dump(ai_arch_strided_e16_seed,
        &ai_arch_strided_e16_src[AI_ARCH_STRIDED_E16_VL - 1U],
        ai_arch_strided_e16_mask, ai_arch_strided_e16_dump,
        -(int64_t)sizeof(uint16_t), AI_ARCH_STRIDED_E16_START_LOAD,
        &observed_vstart);
    TEST_ASSERT("masked vlse16 negative stride binds active suffix elements to descending addresses while vstart prefix and mask-off lanes keep old destination data",
        excpt.triggered == false &&
        observed_vstart == 0U &&
        ai_arch_strided_hwords_equal(ai_arch_strided_e16_dump,
            expected_load, AI_ARCH_STRIDED_E16_VL, &bad_index),
        "vstart=%u bad=%u actual=0x%x expected=0x%x dump=%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x",
        observed_vstart, bad_index, ai_arch_strided_e16_dump[bad_index],
        expected_load[bad_index], ai_arch_strided_e16_dump[0],
        ai_arch_strided_e16_dump[1], ai_arch_strided_e16_dump[2],
        ai_arch_strided_e16_dump[3], ai_arch_strided_e16_dump[4],
        ai_arch_strided_e16_dump[5], ai_arch_strided_e16_dump[6],
        ai_arch_strided_e16_dump[7]
    );

    TEST_SETUP_EXCEPT();
    ai_arch_strided_masked_vsse16_negative_store(
        ai_arch_strided_e16_store_vec,
        &ai_arch_strided_e16_store_mem[AI_ARCH_STRIDED_E16_VL - 1U],
        ai_arch_strided_e16_mask, -(int64_t)sizeof(uint16_t),
        AI_ARCH_STRIDED_E16_START_STORE, &observed_vstart);
    TEST_ASSERT("masked vsse16 negative stride updates only active suffix element addresses and preserves vstart prefix plus mask-off destination slots",
        excpt.triggered == false &&
        observed_vstart == 0U &&
        ai_arch_strided_hwords_equal(ai_arch_strided_e16_store_mem,
            expected_store, AI_ARCH_STRIDED_E16_VL, &bad_index),
        "vstart=%u bad=%u actual=0x%x expected=0x%x mem=%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x",
        observed_vstart, bad_index, ai_arch_strided_e16_store_mem[bad_index],
        expected_store[bad_index], ai_arch_strided_e16_store_mem[0],
        ai_arch_strided_e16_store_mem[1], ai_arch_strided_e16_store_mem[2],
        ai_arch_strided_e16_store_mem[3], ai_arch_strided_e16_store_mem[4],
        ai_arch_strided_e16_store_mem[5], ai_arch_strided_e16_store_mem[6],
        ai_arch_strided_e16_store_mem[7]
    );

    ai_arch_strided_e64_mask[0] = 0x0bU;
    ai_arch_strided_e64_repeated = 0x8877665544332211ULL;
    for (unsigned i = 0; i < AI_ARCH_STRIDED_E64_VL; ++i) {
        ai_arch_strided_e64_seed[i] = 0x9000000000000000ULL | i;
        ai_arch_strided_e64_dump[i] = 0x5555000000000000ULL | i;
        expected_zero[i] = ai_arch_strided_e64_seed[i];
        if (i >= AI_ARCH_STRIDED_E64_START &&
            ((ai_arch_strided_e64_mask[0] >> i) & 1U) != 0U) {
            expected_zero[i] = ai_arch_strided_e64_repeated;
        }
    }

    TEST_SETUP_EXCEPT();
    ai_arch_strided_masked_vlse64_zero_dump(ai_arch_strided_e64_seed,
        &ai_arch_strided_e64_repeated, ai_arch_strided_e64_mask,
        ai_arch_strided_e64_dump, AI_ARCH_STRIDED_E64_START,
        &observed_vstart);
    TEST_ASSERT("masked vlse64 zero stride repeats the same address only for active suffix lanes and leaves prestart or inactive lanes undisturbed",
        excpt.triggered == false &&
        observed_vstart == 0U &&
        ai_arch_strided_dwords_equal(ai_arch_strided_e64_dump,
            expected_zero, AI_ARCH_STRIDED_E64_VL, &bad_index),
        "vstart=%u bad=%u actual=0x%lx expected=0x%lx dump=%lx,%lx,%lx,%lx",
        observed_vstart, bad_index, ai_arch_strided_e64_dump[bad_index],
        expected_zero[bad_index], ai_arch_strided_e64_dump[0],
        ai_arch_strided_e64_dump[1], ai_arch_strided_e64_dump[2],
        ai_arch_strided_e64_dump[3]
    );

    TEST_END();
}
