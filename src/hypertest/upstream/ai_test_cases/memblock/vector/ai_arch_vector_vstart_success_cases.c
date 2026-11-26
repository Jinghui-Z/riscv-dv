#include <rvh_test.h>
#include <stdint.h>
#include <stddef.h>

#define AI_ARCH_VSTART_E8_VL 16U
#define AI_ARCH_VSTART_E32_VL 8U
#define AI_ARCH_VSTART_E8_LOAD_START 7U
#define AI_ARCH_VSTART_E8_STORE_START 6U
#define AI_ARCH_VSTART_E32_STORE_START 3U

static uint8_t ai_arch_vstart_load_seed[AI_ARCH_VSTART_E8_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vstart_load_src[AI_ARCH_VSTART_E8_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vstart_load_dump[AI_ARCH_VSTART_E8_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vstart_store_src[AI_ARCH_VSTART_E8_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vstart_store_dst[AI_ARCH_VSTART_E8_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vstart_store_carry_dst[AI_ARCH_VSTART_E8_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vstart_strided_src[AI_ARCH_VSTART_E32_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vstart_strided_dst[AI_ARCH_VSTART_E32_VL]
    __attribute__((aligned(64)));

static inline void ai_arch_vstart_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline bool ai_arch_vstart_bytes_equal(const uint8_t *actual,
    const uint8_t *expected, unsigned bytes, unsigned *bad_index)
{
    for (unsigned i = 0; i < bytes; ++i) {
        if (actual[i] != expected[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = 0U;
    return true;
}

static inline bool ai_arch_vstart_words_equal(const uint32_t *actual,
    const uint32_t *expected, unsigned words, unsigned *bad_index)
{
    for (unsigned i = 0; i < words; ++i) {
        if (actual[i] != expected[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = 0U;
    return true;
}

static inline void ai_arch_vstart_vle8_dump(const uint8_t *seed,
    const uint8_t *src, uint8_t *dump, unsigned start,
    unsigned *observed_vstart)
{
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, %[start]\n\t"
        "vle8.v v8, (%[src])\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed),
          [src] "r"(src),
          [dump] "r"(dump),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_VSTART_E8_VL)
        : "t0", "memory");

    *observed_vstart = captured_vstart;
}

static inline void ai_arch_vstart_vse8_then_carry_store(const uint8_t *src,
    uint8_t *dst, uint8_t *carry_dst, unsigned start,
    unsigned *observed_vstart)
{
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[src])\n\t"
        "csrw vstart, %[start]\n\t"
        "vse8.v v8, (%[dst])\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "vse8.v v8, (%[carry_dst])\n\t"
        "csrw vstart, zero\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(captured_vstart)
        : [src] "r"(src),
          [dst] "r"(dst),
          [carry_dst] "r"(carry_dst),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_VSTART_E8_VL)
        : "t0", "memory");

    *observed_vstart = captured_vstart;
}

static inline void ai_arch_vstart_vsse32_negative_stride(const uint32_t *src,
    uint32_t *base, int64_t stride, unsigned start, unsigned *observed_vstart)
{
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
        "vle32.v v8, (%[src])\n\t"
        "csrw vstart, %[start]\n\t"
        "vsse32.v v8, (%[base]), %[stride]\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(captured_vstart)
        : [src] "r"(src),
          [base] "r"(base),
          [stride] "r"(stride),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_VSTART_E32_VL)
        : "t0", "memory");

    *observed_vstart = captured_vstart;
}

bool ai_arch_vector_vstart_success_path_owner_corner()
{
    TEST_START();

    uint8_t expected_load[AI_ARCH_VSTART_E8_VL];
    uint8_t expected_store[AI_ARCH_VSTART_E8_VL];
    uint32_t expected_strided[AI_ARCH_VSTART_E32_VL];
    unsigned observed_vstart = 0U;
    unsigned bad_index = 0U;

    ai_arch_vstart_enable_vector_state();

    for (unsigned i = 0; i < AI_ARCH_VSTART_E8_VL; ++i) {
        ai_arch_vstart_load_seed[i] = (uint8_t)(0xa0U + i);
        ai_arch_vstart_load_src[i] = (uint8_t)(0x30U + (3U * i));
        ai_arch_vstart_load_dump[i] = 0x5aU;
        ai_arch_vstart_store_src[i] = (uint8_t)(0xc0U + (5U * i));
        ai_arch_vstart_store_dst[i] = 0x6bU;
        ai_arch_vstart_store_carry_dst[i] = 0x7cU;
        expected_load[i] = (i < AI_ARCH_VSTART_E8_LOAD_START) ?
            ai_arch_vstart_load_seed[i] : ai_arch_vstart_load_src[i];
        expected_store[i] = (i < AI_ARCH_VSTART_E8_STORE_START) ?
            0x6bU : ai_arch_vstart_store_src[i];
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vstart_vle8_dump(ai_arch_vstart_load_seed,
        ai_arch_vstart_load_src, ai_arch_vstart_load_dump,
        AI_ARCH_VSTART_E8_LOAD_START, &observed_vstart);
    TEST_ASSERT("nonzero vstart unit-stride load keeps prefix lanes old, loads only suffix lanes, and clears CSR vstart",
        excpt.triggered == false &&
        observed_vstart == 0U &&
        ai_arch_vstart_bytes_equal(ai_arch_vstart_load_dump,
            expected_load, AI_ARCH_VSTART_E8_VL, &bad_index),
        "vstart=%u bad=%u actual=0x%x expected=0x%x",
        observed_vstart, bad_index, ai_arch_vstart_load_dump[bad_index],
        expected_load[bad_index]
    );

    TEST_SETUP_EXCEPT();
    ai_arch_vstart_vse8_then_carry_store(ai_arch_vstart_store_src,
        ai_arch_vstart_store_dst, ai_arch_vstart_store_carry_dst,
        AI_ARCH_VSTART_E8_STORE_START, &observed_vstart);
    TEST_ASSERT("nonzero vstart unit-stride store preserves prefix bytes, writes suffix bytes, and the following store starts from element zero",
        excpt.triggered == false &&
        observed_vstart == 0U &&
        ai_arch_vstart_bytes_equal(ai_arch_vstart_store_dst,
            expected_store, AI_ARCH_VSTART_E8_VL, &bad_index) &&
        ai_arch_vstart_bytes_equal(ai_arch_vstart_store_carry_dst,
            ai_arch_vstart_store_src, AI_ARCH_VSTART_E8_VL, &bad_index),
        "vstart=%u bad=%u dst=0x%x exp=0x%x carry=0x%x src=0x%x",
        observed_vstart, bad_index, ai_arch_vstart_store_dst[bad_index],
        expected_store[bad_index], ai_arch_vstart_store_carry_dst[bad_index],
        ai_arch_vstart_store_src[bad_index]
    );

    for (unsigned i = 0; i < AI_ARCH_VSTART_E32_VL; ++i) {
        ai_arch_vstart_strided_src[i] =
            0x51000000U | (AI_ARCH_VSTART_E32_STORE_START << 16) |
            (i << 8) | (0x41U + i);
        ai_arch_vstart_strided_dst[i] = 0xdead7000U | i;
        expected_strided[i] = 0xdead7000U | i;
    }
    for (unsigned i = AI_ARCH_VSTART_E32_STORE_START;
         i < AI_ARCH_VSTART_E32_VL; ++i) {
        expected_strided[AI_ARCH_VSTART_E32_VL - 1U - i] =
            ai_arch_vstart_strided_src[i];
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vstart_vsse32_negative_stride(ai_arch_vstart_strided_src,
        &ai_arch_vstart_strided_dst[AI_ARCH_VSTART_E32_VL - 1U],
        -(int64_t)sizeof(uint32_t), AI_ARCH_VSTART_E32_STORE_START,
        &observed_vstart);
    TEST_ASSERT("nonzero vstart negative-stride store skips prefix addresses, writes only suffix element episodes, and clears CSR vstart",
        excpt.triggered == false &&
        observed_vstart == 0U &&
        ai_arch_vstart_words_equal(ai_arch_vstart_strided_dst,
            expected_strided, AI_ARCH_VSTART_E32_VL, &bad_index),
        "vstart=%u bad=%u actual=0x%x expected=0x%x",
        observed_vstart, bad_index, ai_arch_vstart_strided_dst[bad_index],
        expected_strided[bad_index]
    );

    TEST_END();
}
