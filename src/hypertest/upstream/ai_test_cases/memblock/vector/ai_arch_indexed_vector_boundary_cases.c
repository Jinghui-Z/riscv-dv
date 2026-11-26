#include <rvh_test.h>
#include <stdint.h>
#include <stddef.h>

#define AI_ARCH_IDXVEC_BOUNDARY_VL 16U
#define AI_ARCH_IDXVEC_MEM_BYTES 512U
#define AI_ARCH_IDXVEC_SENTINEL 0x5cU

static uint8_t ai_arch_idxvec_mem[AI_ARCH_IDXVEC_MEM_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_idxvec_dump[AI_ARCH_IDXVEC_BOUNDARY_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_idxvec_src[AI_ARCH_IDXVEC_BOUNDARY_VL]
    __attribute__((aligned(64)));
static uint16_t ai_arch_idxvec_offsets16[AI_ARCH_IDXVEC_BOUNDARY_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_idxvec_offsets32[AI_ARCH_IDXVEC_BOUNDARY_VL]
    __attribute__((aligned(64)));

static const uint16_t ai_arch_idxvec_boundary_offsets[AI_ARCH_IDXVEC_BOUNDARY_VL] = {
    0x0f0U, 0x010U, 0x1c0U, 0x084U,
    0x130U, 0x048U, 0x1a8U, 0x070U,
    0x1e0U, 0x028U, 0x160U, 0x0c4U,
    0x100U, 0x058U, 0x198U, 0x0a8U,
};

static const uint8_t ai_arch_idxvec_load_signature[AI_ARCH_IDXVEC_BOUNDARY_VL] = {
    0x81U, 0x42U, 0xd3U, 0x24U,
    0xb5U, 0x66U, 0xf7U, 0x38U,
    0xc9U, 0x1aU, 0xabU, 0x7cU,
    0xedU, 0x0eU, 0x9fU, 0x50U,
};

static const uint8_t ai_arch_idxvec_store_signature[AI_ARCH_IDXVEC_BOUNDARY_VL] = {
    0x13U, 0xa4U, 0x35U, 0xc6U,
    0x57U, 0xe8U, 0x79U, 0x0aU,
    0x9bU, 0x2cU, 0xbdU, 0x4eU,
    0xdfU, 0x60U, 0xf1U, 0x82U,
};

static inline void ai_arch_idxvec_enable_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_arch_idxvec_prepare_offsets(void)
{
    for (unsigned lane = 0; lane < AI_ARCH_IDXVEC_BOUNDARY_VL; ++lane) {
        ai_arch_idxvec_offsets16[lane] =
            ai_arch_idxvec_boundary_offsets[lane];
        ai_arch_idxvec_offsets32[lane] =
            ai_arch_idxvec_boundary_offsets[lane];
    }
}

static inline void ai_arch_idxvec_fill_mem(uint8_t value)
{
    for (size_t i = 0; i < AI_ARCH_IDXVEC_MEM_BYTES; ++i) {
        ai_arch_idxvec_mem[i] = value;
    }
}

static inline void ai_arch_idxvec_prepare_load_image(void)
{
    ai_arch_idxvec_fill_mem(AI_ARCH_IDXVEC_SENTINEL);
    for (unsigned lane = 0; lane < AI_ARCH_IDXVEC_BOUNDARY_VL; ++lane) {
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[lane]] =
            ai_arch_idxvec_load_signature[lane];
        ai_arch_idxvec_dump[lane] = 0xa5U;
    }
}

static inline void ai_arch_idxvec_prepare_store_image(void)
{
    ai_arch_idxvec_fill_mem(AI_ARCH_IDXVEC_SENTINEL);
    for (unsigned lane = 0; lane < AI_ARCH_IDXVEC_BOUNDARY_VL; ++lane) {
        ai_arch_idxvec_src[lane] = ai_arch_idxvec_store_signature[lane];
    }
}

static inline bool ai_arch_idxvec_load_dump_ok(unsigned *bad_lane,
    uint8_t *bad_actual, uint8_t *bad_expected)
{
    for (unsigned lane = 0; lane < AI_ARCH_IDXVEC_BOUNDARY_VL; ++lane) {
        if (ai_arch_idxvec_dump[lane] !=
            ai_arch_idxvec_load_signature[lane]) {
            *bad_lane = lane;
            *bad_actual = ai_arch_idxvec_dump[lane];
            *bad_expected = ai_arch_idxvec_load_signature[lane];
            return false;
        }
    }
    *bad_lane = AI_ARCH_IDXVEC_BOUNDARY_VL;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static inline bool ai_arch_idxvec_store_image_ok(unsigned *bad_offset,
    uint8_t *bad_actual, uint8_t *bad_expected)
{
    for (unsigned offset = 0; offset < AI_ARCH_IDXVEC_MEM_BYTES; ++offset) {
        uint8_t expected = AI_ARCH_IDXVEC_SENTINEL;
        for (unsigned lane = 0; lane < AI_ARCH_IDXVEC_BOUNDARY_VL; ++lane) {
            if (offset == ai_arch_idxvec_boundary_offsets[lane]) {
                expected = ai_arch_idxvec_store_signature[lane];
                break;
            }
        }
        if (ai_arch_idxvec_mem[offset] != expected) {
            *bad_offset = offset;
            *bad_actual = ai_arch_idxvec_mem[offset];
            *bad_expected = expected;
            return false;
        }
    }
    *bad_offset = AI_ARCH_IDXVEC_MEM_BYTES;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static inline void ai_arch_idxvec_vluxei16_e8_m1_dump(const uint8_t *base,
    const uint16_t *offsets, uint8_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e16, m2, ta, ma\n\t"
        "vle16.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vluxei16.v v16, (%[base]), v8\n\t"
        "vse8.v v16, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_IDXVEC_BOUNDARY_VL)
        : "t0", "memory");
}

static inline void ai_arch_idxvec_vloxei16_e8_m1_dump(const uint8_t *base,
    const uint16_t *offsets, uint8_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e16, m2, ta, ma\n\t"
        "vle16.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vloxei16.v v16, (%[base]), v8\n\t"
        "vse8.v v16, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_IDXVEC_BOUNDARY_VL)
        : "t0", "memory");
}

static inline void ai_arch_idxvec_vluxei32_e8_m1_dump(const uint8_t *base,
    const uint32_t *offsets, uint8_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vluxei32.v v16, (%[base]), v8\n\t"
        "vse8.v v16, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_IDXVEC_BOUNDARY_VL)
        : "t0", "memory");
}

static inline void ai_arch_idxvec_vloxei32_e8_m1_dump(const uint8_t *base,
    const uint32_t *offsets, uint8_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vloxei32.v v16, (%[base]), v8\n\t"
        "vse8.v v16, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_IDXVEC_BOUNDARY_VL)
        : "t0", "memory");
}

static inline void ai_arch_idxvec_vsuxei16_e8_m1_store(const uint8_t *src,
    const uint16_t *offsets, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v16, (%[src])\n\t"
        "vsetvli t0, %[vl], e16, m2, ta, ma\n\t"
        "vle16.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vsuxei16.v v16, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_ARCH_IDXVEC_BOUNDARY_VL)
        : "t0", "memory");
}

static inline void ai_arch_idxvec_vsoxei16_e8_m1_store(const uint8_t *src,
    const uint16_t *offsets, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v16, (%[src])\n\t"
        "vsetvli t0, %[vl], e16, m2, ta, ma\n\t"
        "vle16.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vsoxei16.v v16, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_ARCH_IDXVEC_BOUNDARY_VL)
        : "t0", "memory");
}

static inline void ai_arch_idxvec_vsuxei32_e8_m1_store(const uint8_t *src,
    const uint32_t *offsets, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v16, (%[src])\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vsuxei32.v v16, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_ARCH_IDXVEC_BOUNDARY_VL)
        : "t0", "memory");
}

static inline void ai_arch_idxvec_vsoxei32_e8_m1_store(const uint8_t *src,
    const uint32_t *offsets, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v16, (%[src])\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vsoxei32.v v16, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_ARCH_IDXVEC_BOUNDARY_VL)
        : "t0", "memory");
}

bool ai_arch_indexed_vector_emul_gt_lmul_load_index_register_boundary_corner()
{
    TEST_START();

    unsigned bad_lane = AI_ARCH_IDXVEC_BOUNDARY_VL;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_arch_idxvec_enable_state();
    ai_arch_idxvec_prepare_offsets();

    ai_arch_idxvec_prepare_load_image();
    TEST_SETUP_EXCEPT();
    ai_arch_idxvec_vluxei16_e8_m1_dump(ai_arch_idxvec_mem,
        ai_arch_idxvec_offsets16, ai_arch_idxvec_dump);
    TEST_ASSERT("vluxei16 e8,m1 uses the second index register for lanes8-15 instead of wrapping to the low 128-bit index payload",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_IDXVEC_BOUNDARY_VL &&
        ai_arch_idxvec_load_dump_ok(&bad_lane, &bad_actual, &bad_expected),
        "vl=%lu bad_lane=%u actual=0x%x expected=0x%x dump8=0x%x dump15=0x%x",
        CSRR(CSR_VL), bad_lane, bad_actual, bad_expected,
        ai_arch_idxvec_dump[8], ai_arch_idxvec_dump[15]
    );

    ai_arch_idxvec_prepare_load_image();
    TEST_SETUP_EXCEPT();
    ai_arch_idxvec_vloxei16_e8_m1_dump(ai_arch_idxvec_mem,
        ai_arch_idxvec_offsets16, ai_arch_idxvec_dump);
    TEST_ASSERT("vloxei16 e8,m1 ordered load keeps the same high-index-register boundary binding as unordered indexed load",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_IDXVEC_BOUNDARY_VL &&
        ai_arch_idxvec_load_dump_ok(&bad_lane, &bad_actual, &bad_expected),
        "vl=%lu bad_lane=%u actual=0x%x expected=0x%x dump8=0x%x dump15=0x%x",
        CSRR(CSR_VL), bad_lane, bad_actual, bad_expected,
        ai_arch_idxvec_dump[8], ai_arch_idxvec_dump[15]
    );

    ai_arch_idxvec_prepare_load_image();
    TEST_SETUP_EXCEPT();
    ai_arch_idxvec_vluxei32_e8_m1_dump(ai_arch_idxvec_mem,
        ai_arch_idxvec_offsets32, ai_arch_idxvec_dump);
    TEST_ASSERT("vluxei32 e8,m1 uses all four index registers across lanes4/8/12 and never reuses the first 128-bit index payload",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_IDXVEC_BOUNDARY_VL &&
        ai_arch_idxvec_load_dump_ok(&bad_lane, &bad_actual, &bad_expected),
        "vl=%lu bad_lane=%u actual=0x%x expected=0x%x dump4=0x%x dump8=0x%x dump12=0x%x",
        CSRR(CSR_VL), bad_lane, bad_actual, bad_expected,
        ai_arch_idxvec_dump[4], ai_arch_idxvec_dump[8],
        ai_arch_idxvec_dump[12]
    );

    ai_arch_idxvec_prepare_load_image();
    TEST_SETUP_EXCEPT();
    ai_arch_idxvec_vloxei32_e8_m1_dump(ai_arch_idxvec_mem,
        ai_arch_idxvec_offsets32, ai_arch_idxvec_dump);
    TEST_ASSERT("vloxei32 e8,m1 ordered load keeps all four high-index-register payloads bound to their own data lanes",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_IDXVEC_BOUNDARY_VL &&
        ai_arch_idxvec_load_dump_ok(&bad_lane, &bad_actual, &bad_expected),
        "vl=%lu bad_lane=%u actual=0x%x expected=0x%x dump4=0x%x dump8=0x%x dump12=0x%x",
        CSRR(CSR_VL), bad_lane, bad_actual, bad_expected,
        ai_arch_idxvec_dump[4], ai_arch_idxvec_dump[8],
        ai_arch_idxvec_dump[12]
    );

    TEST_END();
}

bool ai_arch_indexed_vector_emul_gt_lmul_store_index_register_boundary_corner()
{
    TEST_START();

    unsigned bad_offset = AI_ARCH_IDXVEC_MEM_BYTES;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_arch_idxvec_enable_state();
    ai_arch_idxvec_prepare_offsets();

    ai_arch_idxvec_prepare_store_image();
    TEST_SETUP_EXCEPT();
    ai_arch_idxvec_vsuxei16_e8_m1_store(ai_arch_idxvec_src,
        ai_arch_idxvec_offsets16, ai_arch_idxvec_mem);
    TEST_ASSERT("vsuxei16 e8,m1 stores lanes8-15 through the second index register and preserves every untouched byte",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_IDXVEC_BOUNDARY_VL &&
        ai_arch_idxvec_store_image_ok(&bad_offset, &bad_actual,
            &bad_expected),
        "vl=%lu bad_offset=0x%x actual=0x%x expected=0x%x mem_lane8=0x%x mem_lane15=0x%x",
        CSRR(CSR_VL), bad_offset, bad_actual, bad_expected,
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[8]],
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[15]]
    );

    ai_arch_idxvec_prepare_store_image();
    TEST_SETUP_EXCEPT();
    ai_arch_idxvec_vsoxei16_e8_m1_store(ai_arch_idxvec_src,
        ai_arch_idxvec_offsets16, ai_arch_idxvec_mem);
    TEST_ASSERT("vsoxei16 e8,m1 ordered store keeps high-index-register lane binding while producing the same deterministic no-duplicate memory image",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_IDXVEC_BOUNDARY_VL &&
        ai_arch_idxvec_store_image_ok(&bad_offset, &bad_actual,
            &bad_expected),
        "vl=%lu bad_offset=0x%x actual=0x%x expected=0x%x mem_lane8=0x%x mem_lane15=0x%x",
        CSRR(CSR_VL), bad_offset, bad_actual, bad_expected,
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[8]],
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[15]]
    );

    ai_arch_idxvec_prepare_store_image();
    TEST_SETUP_EXCEPT();
    ai_arch_idxvec_vsuxei32_e8_m1_store(ai_arch_idxvec_src,
        ai_arch_idxvec_offsets32, ai_arch_idxvec_mem);
    TEST_ASSERT("vsuxei32 e8,m1 stores through all four index registers across lanes4/8/12 and does not corrupt sentinel bytes",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_IDXVEC_BOUNDARY_VL &&
        ai_arch_idxvec_store_image_ok(&bad_offset, &bad_actual,
            &bad_expected),
        "vl=%lu bad_offset=0x%x actual=0x%x expected=0x%x mem4=0x%x mem8=0x%x mem12=0x%x",
        CSRR(CSR_VL), bad_offset, bad_actual, bad_expected,
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[4]],
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[8]],
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[12]]
    );

    ai_arch_idxvec_prepare_store_image();
    TEST_SETUP_EXCEPT();
    ai_arch_idxvec_vsoxei32_e8_m1_store(ai_arch_idxvec_src,
        ai_arch_idxvec_offsets32, ai_arch_idxvec_mem);
    TEST_ASSERT("vsoxei32 e8,m1 ordered store keeps all four high-index-register payloads bound to their own data lanes",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_IDXVEC_BOUNDARY_VL &&
        ai_arch_idxvec_store_image_ok(&bad_offset, &bad_actual,
            &bad_expected),
        "vl=%lu bad_offset=0x%x actual=0x%x expected=0x%x mem4=0x%x mem8=0x%x mem12=0x%x",
        CSRR(CSR_VL), bad_offset, bad_actual, bad_expected,
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[4]],
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[8]],
        ai_arch_idxvec_mem[ai_arch_idxvec_boundary_offsets[12]]
    );

    TEST_END();
}
