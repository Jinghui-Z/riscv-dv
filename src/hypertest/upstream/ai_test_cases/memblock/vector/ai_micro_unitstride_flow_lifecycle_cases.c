#include <rvh_test.h>
#include <stdint.h>
#include <stddef.h>

#define AI_MICRO_US_FLOW_BYTES 16U
#define AI_MICRO_US_FLOW_WINDOW 64U

static uint8_t ai_micro_us_flow_src_a[AI_MICRO_US_FLOW_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_micro_us_flow_src_b[AI_MICRO_US_FLOW_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_micro_us_flow_dst[AI_MICRO_US_FLOW_WINDOW]
    __attribute__((aligned(64)));
static uint8_t ai_micro_us_flow_probe_dump[AI_MICRO_US_FLOW_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_micro_us_flow_mask_high[2]
    __attribute__((aligned(8)));
static uint8_t ai_micro_us_flow_mask_sparse[2]
    __attribute__((aligned(8)));

static inline void ai_micro_us_flow_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_micro_us_flow_fill(uint8_t *dst, size_t bytes,
    uint8_t seed)
{
    for (size_t i = 0; i < bytes; ++i) {
        dst[i] = (uint8_t)(seed + (uint8_t)(i * 7U));
    }
}

static inline bool ai_micro_us_flow_match(const uint8_t *actual,
    const uint8_t *expected, size_t bytes, size_t *bad_index,
    uint8_t *bad_actual, uint8_t *bad_expected)
{
    for (size_t i = 0; i < bytes; ++i) {
        if (actual[i] != expected[i]) {
            *bad_index = i;
            *bad_actual = actual[i];
            *bad_expected = expected[i];
            return false;
        }
    }
    *bad_index = bytes;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static inline void ai_micro_us_flow_masked_high_vse8(const uint8_t *src,
    uint8_t *dst, const uint8_t *mask)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v6, (%[src])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vse8.v v6, (%[dst]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dst] "r"(dst),
          [mask] "r"(mask),
          [vl] "r"((unsigned)AI_MICRO_US_FLOW_BYTES)
        : "t0", "memory");
}

static inline void ai_micro_us_flow_cross16_vse16(const uint8_t *src,
    uint8_t *dst)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "vle16.v v6, (%[src])\n\t"
        "vse16.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"((unsigned)(AI_MICRO_US_FLOW_BYTES / 2U))
        : "t0", "memory");
}

static inline void ai_micro_us_flow_sparse_vse16_then_vle16_probe(
    const uint8_t *src, uint8_t *dst, uint8_t *dump, const uint8_t *mask)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "vle16.v v6, (%[src])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vse16.v v6, (%[dst]), v0.t\n\t"
        "vle16.v v8, (%[dst])\n\t"
        "vse16.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dst] "r"(dst),
          [dump] "r"(dump),
          [mask] "r"(mask),
          [vl] "r"((unsigned)(AI_MICRO_US_FLOW_BYTES / 2U))
        : "t0", "memory");
}

bool ai_micro_unitstride_no_split_mask_then_cross16_split_store_byte_image_corner()
{
    TEST_START();

    const size_t aligned_off = 16U;
    const size_t cross_off = 31U;
    uint8_t expected[AI_MICRO_US_FLOW_WINDOW];
    size_t bad_index = 0;
    uint8_t bad_actual = 0;
    uint8_t bad_expected = 0;

    ai_micro_us_flow_enable_vector_state();

    ai_micro_us_flow_fill(ai_micro_us_flow_src_a, AI_MICRO_US_FLOW_BYTES,
        0x21U);
    ai_micro_us_flow_fill(ai_micro_us_flow_src_b, AI_MICRO_US_FLOW_BYTES,
        0xa1U);
    ai_micro_us_flow_fill(ai_micro_us_flow_dst, AI_MICRO_US_FLOW_WINDOW,
        0x60U);
    for (size_t i = 0; i < AI_MICRO_US_FLOW_WINDOW; ++i) {
        expected[i] = ai_micro_us_flow_dst[i];
    }

    ai_micro_us_flow_mask_high[0] = 0x00U;
    ai_micro_us_flow_mask_high[1] = 0xffU;

    for (size_t i = 8U; i < AI_MICRO_US_FLOW_BYTES; ++i) {
        expected[aligned_off + i] = ai_micro_us_flow_src_a[i];
    }

    TEST_SETUP_EXCEPT();
    ai_micro_us_flow_masked_high_vse8(ai_micro_us_flow_src_a,
        ai_micro_us_flow_dst + aligned_off, ai_micro_us_flow_mask_high);
    TEST_ASSERT("aligned no-split unit-stride store with only high-half active bytes must not wait for or drop a phantom second flow",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_US_FLOW_BYTES &&
        ai_micro_us_flow_match(ai_micro_us_flow_dst, expected,
            AI_MICRO_US_FLOW_WINDOW, &bad_index, &bad_actual,
            &bad_expected),
        "vl=%lu bad_index=%lu actual=0x%x expected=0x%x",
        CSRR(CSR_VL), bad_index, bad_actual, bad_expected
    );

    for (size_t i = 0; i < AI_MICRO_US_FLOW_BYTES; ++i) {
        expected[cross_off + i] = ai_micro_us_flow_src_b[i];
    }

    TEST_SETUP_EXCEPT();
    ai_micro_us_flow_cross16_vse16(ai_micro_us_flow_src_b,
        ai_micro_us_flow_dst + cross_off);
    TEST_ASSERT("following e16 unit-stride store starts at byte15 of a 16B window and must update the split element and high-half bytes exactly once",
        excpt.triggered == false &&
        CSRR(CSR_VL) == (AI_MICRO_US_FLOW_BYTES / 2U) &&
        ai_micro_us_flow_match(ai_micro_us_flow_dst, expected,
            AI_MICRO_US_FLOW_WINDOW, &bad_index, &bad_actual,
            &bad_expected),
        "vl=%lu bad_index=%lu actual=0x%x expected=0x%x cross_off=%lu",
        CSRR(CSR_VL), bad_index, bad_actual, bad_expected, cross_off
    );

    TEST_ASSERT("no-split high-half mask and later cross16 split store preserve both outer guard regions",
        ai_micro_us_flow_match(ai_micro_us_flow_dst, expected, cross_off,
            &bad_index, &bad_actual, &bad_expected) &&
        ai_micro_us_flow_match(ai_micro_us_flow_dst + cross_off +
            AI_MICRO_US_FLOW_BYTES, expected + cross_off +
            AI_MICRO_US_FLOW_BYTES,
            AI_MICRO_US_FLOW_WINDOW - cross_off - AI_MICRO_US_FLOW_BYTES,
            &bad_index, &bad_actual, &bad_expected)
    );

    TEST_END();
}

bool ai_micro_unitstride_cross16_sparse_store_then_vector_load_query_mask_corner()
{
    TEST_START();

    const size_t cross_off = 31U;
    const size_t elem_bytes = 2U;
    const size_t active_elem0 = 0U;
    const size_t active_elem1 = 3U;
    const size_t active_elem2 = 7U;
    uint8_t expected[AI_MICRO_US_FLOW_WINDOW];
    size_t bad_index = 0;
    uint8_t bad_actual = 0;
    uint8_t bad_expected = 0;
    bool active_bytes_ok = false;
    bool inactive_bytes_ok = false;

    ai_micro_us_flow_enable_vector_state();

    ai_micro_us_flow_fill(ai_micro_us_flow_src_a, AI_MICRO_US_FLOW_BYTES,
        0xc3U);
    ai_micro_us_flow_fill(ai_micro_us_flow_dst, AI_MICRO_US_FLOW_WINDOW,
        0x18U);
    ai_micro_us_flow_fill(ai_micro_us_flow_probe_dump,
        AI_MICRO_US_FLOW_BYTES, 0xe1U);
    for (size_t i = 0; i < AI_MICRO_US_FLOW_WINDOW; ++i) {
        expected[i] = ai_micro_us_flow_dst[i];
    }

    ai_micro_us_flow_mask_sparse[0] = 0x89U;
    ai_micro_us_flow_mask_sparse[1] = 0x00U;

    for (size_t b = 0; b < elem_bytes; ++b) {
        expected[cross_off + active_elem0 * elem_bytes + b] =
            ai_micro_us_flow_src_a[active_elem0 * elem_bytes + b];
        expected[cross_off + active_elem1 * elem_bytes + b] =
            ai_micro_us_flow_src_a[active_elem1 * elem_bytes + b];
        expected[cross_off + active_elem2 * elem_bytes + b] =
            ai_micro_us_flow_src_a[active_elem2 * elem_bytes + b];
    }

    TEST_SETUP_EXCEPT();
    ai_micro_us_flow_sparse_vse16_then_vle16_probe(ai_micro_us_flow_src_a,
        ai_micro_us_flow_dst + cross_off, ai_micro_us_flow_probe_dump,
        ai_micro_us_flow_mask_sparse);

    active_bytes_ok =
        ai_micro_us_flow_probe_dump[0] == expected[cross_off] &&
        ai_micro_us_flow_probe_dump[1] == expected[cross_off + 1U] &&
        ai_micro_us_flow_probe_dump[6] == expected[cross_off + 6U] &&
        ai_micro_us_flow_probe_dump[7] == expected[cross_off + 7U] &&
        ai_micro_us_flow_probe_dump[14] == expected[cross_off + 14U] &&
        ai_micro_us_flow_probe_dump[15] == expected[cross_off + 15U];
    inactive_bytes_ok =
        ai_micro_us_flow_probe_dump[2] == expected[cross_off + 2U] &&
        ai_micro_us_flow_probe_dump[3] == expected[cross_off + 3U] &&
        ai_micro_us_flow_probe_dump[8] == expected[cross_off + 8U] &&
        ai_micro_us_flow_probe_dump[9] == expected[cross_off + 9U] &&
        ai_micro_us_flow_probe_dump[12] == expected[cross_off + 12U] &&
        ai_micro_us_flow_probe_dump[13] == expected[cross_off + 13U];

    TEST_ASSERT("masked e16 cross16 unit-stride store followed immediately by a full vector load must forward only true active bytes, not over-query-only inactive bytes",
        excpt.triggered == false &&
        CSRR(CSR_VL) == (AI_MICRO_US_FLOW_BYTES / 2U) &&
        active_bytes_ok &&
        inactive_bytes_ok &&
        ai_micro_us_flow_match(ai_micro_us_flow_probe_dump,
            expected + cross_off, AI_MICRO_US_FLOW_BYTES, &bad_index,
            &bad_actual, &bad_expected),
        "vl=%lu active_ok=%d inactive_ok=%d dump_bad=%lu actual=0x%x expected=0x%x",
        CSRR(CSR_VL), active_bytes_ok, inactive_bytes_ok, bad_index,
        bad_actual, bad_expected
    );

    TEST_ASSERT("sparse cross16 store leaves inactive destination bytes and outer guards at their old image after the query-mask stress window drains",
        ai_micro_us_flow_match(ai_micro_us_flow_dst, expected,
            AI_MICRO_US_FLOW_WINDOW, &bad_index, &bad_actual,
            &bad_expected),
        "bad_index=%lu actual=0x%x expected=0x%x cross_off=%lu",
        bad_index, bad_actual, bad_expected, cross_off
    );

    TEST_END();
}
