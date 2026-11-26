#include <rvh_test.h>
#include <stdint.h>
#include <stddef.h>

#define AI_ARCH_VSEG_CHURN_VL 4U
#define AI_ARCH_VSEG_CHURN_E16_NF 4U
#define AI_ARCH_VSEG_CHURN_STRIDE_WORDS 6U
#define AI_ARCH_VSEG_CHURN_INDEX_BYTES 80U
#define AI_ARCH_VSEG_NF8_E8_FIELDS 8U
#define AI_ARCH_VSEG_NF8_E8_VL 2U
#define AI_ARCH_VSEG_MASK_E8_LANES 8U
#define AI_ARCH_VSEG_MASK_E8_VL 6U
#define AI_ARCH_VSEG_CROSS16_E32_VL 2U
#define AI_ARCH_VSEG_CROSS16_E32_OFFSET 12U
#define AI_ARCH_VSEG_CROSS16_E32_BYTES \
    (AI_ARCH_VSEG_CROSS16_E32_VL * 2U * sizeof(uint32_t))
#define AI_ARCH_VSEG_INDEX_BOUNDARY_VL 8U
#define AI_ARCH_VSEG_INDEX_BOUNDARY_BYTES 256U
#define AI_ARCH_VSEG_INDEX_BOUNDARY_MASK \
    ((uint8_t)((1U << 0U) | (1U << 3U) | (1U << 4U) | (1U << 7U)))

static uint16_t ai_arch_vseg_e16_src[AI_ARCH_VSEG_CHURN_E16_NF]
    [AI_ARCH_VSEG_CHURN_VL] __attribute__((aligned(64)));
static uint16_t ai_arch_vseg_e16_dst[
    2U + AI_ARCH_VSEG_CHURN_E16_NF * AI_ARCH_VSEG_CHURN_VL + 2U]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_e32_src0[AI_ARCH_VSEG_CHURN_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_e32_src1[AI_ARCH_VSEG_CHURN_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_stride_dst[
    AI_ARCH_VSEG_CHURN_STRIDE_WORDS * AI_ARCH_VSEG_CHURN_VL + 2U]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_index_offsets[AI_ARCH_VSEG_CHURN_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_index_dst[AI_ARCH_VSEG_CHURN_INDEX_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_nf8_e8_src[AI_ARCH_VSEG_NF8_E8_FIELDS]
    [AI_ARCH_VSEG_NF8_E8_VL] __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_nf8_e8_dst[
    2U + AI_ARCH_VSEG_NF8_E8_FIELDS * AI_ARCH_VSEG_NF8_E8_VL + 2U]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_mask_e8_src0[AI_ARCH_VSEG_MASK_E8_LANES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_mask_e8_src1[AI_ARCH_VSEG_MASK_E8_LANES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_mask_e8_dst[
    2U + 2U * AI_ARCH_VSEG_MASK_E8_LANES + 2U]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_mask_e8_bits[1] __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_cross32_src0[AI_ARCH_VSEG_CROSS16_E32_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_cross32_src1[AI_ARCH_VSEG_CROSS16_E32_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_cross32_dst[
    AI_ARCH_VSEG_CROSS16_E32_OFFSET + AI_ARCH_VSEG_CROSS16_E32_BYTES + 8U]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_idx_boundary_src0[AI_ARCH_VSEG_INDEX_BOUNDARY_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_idx_boundary_src1[AI_ARCH_VSEG_INDEX_BOUNDARY_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_idx_boundary_offsets[
    AI_ARCH_VSEG_INDEX_BOUNDARY_VL] __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_idx_boundary_seed0[AI_ARCH_VSEG_INDEX_BOUNDARY_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_idx_boundary_seed1[AI_ARCH_VSEG_INDEX_BOUNDARY_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_idx_boundary_dump0[AI_ARCH_VSEG_INDEX_BOUNDARY_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vseg_idx_boundary_dump1[AI_ARCH_VSEG_INDEX_BOUNDARY_VL]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_idx_boundary_dst[
    AI_ARCH_VSEG_INDEX_BOUNDARY_BYTES] __attribute__((aligned(64)));
static uint8_t ai_arch_vseg_idx_boundary_mask[1] __attribute__((aligned(64)));

static inline void ai_arch_vseg_enable_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_arch_vseg_vsseg4e16(uint16_t *dst)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "vle16.v v8, (%[src0])\n\t"
        "vle16.v v9, (%[src1])\n\t"
        "vle16.v v10, (%[src2])\n\t"
        "vle16.v v11, (%[src3])\n\t"
        "vsseg4e16.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src0] "r"(ai_arch_vseg_e16_src[0]),
          [src1] "r"(ai_arch_vseg_e16_src[1]),
          [src2] "r"(ai_arch_vseg_e16_src[2]),
          [src3] "r"(ai_arch_vseg_e16_src[3]),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_VSEG_CHURN_VL)
        : "t0", "memory");
}

static inline void ai_arch_vseg_vssseg2e32(uint32_t *dst,
    intptr_t stride_bytes)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v8, (%[src0])\n\t"
        "vle32.v v9, (%[src1])\n\t"
        "vssseg2e32.v v8, (%[dst]), %[stride]\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src0] "r"(ai_arch_vseg_e32_src0),
          [src1] "r"(ai_arch_vseg_e32_src1),
          [dst] "r"(dst),
          [stride] "r"(stride_bytes),
          [vl] "r"((unsigned)AI_ARCH_VSEG_CHURN_VL)
        : "t0", "memory");
}

static inline void ai_arch_vseg_vsuxseg2ei32(uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v8, (%[src0])\n\t"
        "vle32.v v9, (%[src1])\n\t"
        "vle32.v v10, (%[offsets])\n\t"
        "vsuxseg2ei32.v v8, (%[base]), v10\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src0] "r"(ai_arch_vseg_e32_src0),
          [src1] "r"(ai_arch_vseg_e32_src1),
          [offsets] "r"(ai_arch_vseg_index_offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_ARCH_VSEG_CHURN_VL)
        : "t0", "memory");
}

static inline void ai_arch_vseg_vsuxseg2ei32_boundary(uint8_t *base,
    bool masked)
{
    if (masked) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle32.v v8, (%[src0])\n\t"
            "vle32.v v10, (%[src1])\n\t"
            "vle32.v v12, (%[offsets])\n\t"
            "vlm.v v0, (%[mask])\n\t"
            "vsuxseg2ei32.v v8, (%[base]), v12, v0.t\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src0] "r"(ai_arch_vseg_idx_boundary_src0),
              [src1] "r"(ai_arch_vseg_idx_boundary_src1),
              [offsets] "r"(ai_arch_vseg_idx_boundary_offsets),
              [mask] "r"(ai_arch_vseg_idx_boundary_mask),
              [base] "r"(base),
              [vl] "r"((unsigned)AI_ARCH_VSEG_INDEX_BOUNDARY_VL)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle32.v v8, (%[src0])\n\t"
            "vle32.v v10, (%[src1])\n\t"
            "vle32.v v12, (%[offsets])\n\t"
            "vsuxseg2ei32.v v8, (%[base]), v12\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src0] "r"(ai_arch_vseg_idx_boundary_src0),
              [src1] "r"(ai_arch_vseg_idx_boundary_src1),
              [offsets] "r"(ai_arch_vseg_idx_boundary_offsets),
              [base] "r"(base),
              [vl] "r"((unsigned)AI_ARCH_VSEG_INDEX_BOUNDARY_VL)
            : "t0", "memory");
    }
}

static inline void ai_arch_vseg_vluxseg2ei32_boundary(uint8_t *base,
    bool masked)
{
    if (masked) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle32.v v16, (%[seed0])\n\t"
            "vle32.v v18, (%[seed1])\n\t"
            "vle32.v v12, (%[offsets])\n\t"
            "vlm.v v0, (%[mask])\n\t"
            "vluxseg2ei32.v v16, (%[base]), v12, v0.t\n\t"
            "csrw vstart, zero\n\t"
            "vse32.v v16, (%[dump0])\n\t"
            "vse32.v v18, (%[dump1])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [seed0] "r"(ai_arch_vseg_idx_boundary_seed0),
              [seed1] "r"(ai_arch_vseg_idx_boundary_seed1),
              [offsets] "r"(ai_arch_vseg_idx_boundary_offsets),
              [mask] "r"(ai_arch_vseg_idx_boundary_mask),
              [base] "r"(base),
              [dump0] "r"(ai_arch_vseg_idx_boundary_dump0),
              [dump1] "r"(ai_arch_vseg_idx_boundary_dump1),
              [vl] "r"((unsigned)AI_ARCH_VSEG_INDEX_BOUNDARY_VL)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle32.v v16, (%[seed0])\n\t"
            "vle32.v v18, (%[seed1])\n\t"
            "vle32.v v12, (%[offsets])\n\t"
            "vluxseg2ei32.v v16, (%[base]), v12\n\t"
            "csrw vstart, zero\n\t"
            "vse32.v v16, (%[dump0])\n\t"
            "vse32.v v18, (%[dump1])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [seed0] "r"(ai_arch_vseg_idx_boundary_seed0),
              [seed1] "r"(ai_arch_vseg_idx_boundary_seed1),
              [offsets] "r"(ai_arch_vseg_idx_boundary_offsets),
              [base] "r"(base),
              [dump0] "r"(ai_arch_vseg_idx_boundary_dump0),
              [dump1] "r"(ai_arch_vseg_idx_boundary_dump1),
              [vl] "r"((unsigned)AI_ARCH_VSEG_INDEX_BOUNDARY_VL)
            : "t0", "memory");
    }
}

static inline void ai_arch_vseg_vsseg8e8_direct(uint8_t *dst)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v8, (%[src0])\n\t"
        "vle8.v v9, (%[src1])\n\t"
        "vle8.v v10, (%[src2])\n\t"
        "vle8.v v11, (%[src3])\n\t"
        "vle8.v v12, (%[src4])\n\t"
        "vle8.v v13, (%[src5])\n\t"
        "vle8.v v14, (%[src6])\n\t"
        "vle8.v v15, (%[src7])\n\t"
        "vsseg8e8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src0] "r"(ai_arch_vseg_nf8_e8_src[0]),
          [src1] "r"(ai_arch_vseg_nf8_e8_src[1]),
          [src2] "r"(ai_arch_vseg_nf8_e8_src[2]),
          [src3] "r"(ai_arch_vseg_nf8_e8_src[3]),
          [src4] "r"(ai_arch_vseg_nf8_e8_src[4]),
          [src5] "r"(ai_arch_vseg_nf8_e8_src[5]),
          [src6] "r"(ai_arch_vseg_nf8_e8_src[6]),
          [src7] "r"(ai_arch_vseg_nf8_e8_src[7]),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_VSEG_NF8_E8_VL)
        : "t0", "memory");
}

static inline void ai_arch_vseg_vsseg2e8_masked_direct(uint8_t *dst)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v8, (%[src0])\n\t"
        "vle8.v v9, (%[src1])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vsseg2e8.v v8, (%[dst]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src0] "r"(ai_arch_vseg_mask_e8_src0),
          [src1] "r"(ai_arch_vseg_mask_e8_src1),
          [mask] "r"(ai_arch_vseg_mask_e8_bits),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_VSEG_MASK_E8_VL)
        : "t0", "memory");
}

static inline void ai_arch_vseg_vsseg2e32_cross16_direct(uint8_t *dst)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v8, (%[src0])\n\t"
        "vle32.v v9, (%[src1])\n\t"
        "vsseg2e32.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src0] "r"(ai_arch_vseg_cross32_src0),
          [src1] "r"(ai_arch_vseg_cross32_src1),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_VSEG_CROSS16_E32_VL)
        : "t0", "memory");
}

static inline uint32_t ai_arch_vseg_read32(const uint8_t *base,
    size_t offset)
{
    return *(const volatile uint32_t *)(const void *)(base + offset);
}

static inline void ai_arch_vseg_write8(uint8_t *base, size_t offset,
    uint8_t value)
{
    volatile uint8_t *ptr = (volatile uint8_t *)(void *)(base + offset);

    *ptr = value;
}

static inline void ai_arch_vseg_write32(uint32_t *base, size_t offset,
    uint32_t value)
{
    volatile uint32_t *ptr = (volatile uint32_t *)(void *)(base + offset);

    *ptr = value;
}

static inline void ai_arch_vseg_fill_bytes(uint8_t *base, size_t bytes,
    uint8_t value)
{
    for (size_t i = 0; i < bytes; ++i) {
        ai_arch_vseg_write8(base, i, value);
    }
}

static bool ai_arch_vseg_e16_image_ok(void)
{
    if (ai_arch_vseg_e16_dst[1] != 0xa55aU ||
        ai_arch_vseg_e16_dst[
            2U + AI_ARCH_VSEG_CHURN_E16_NF *
            AI_ARCH_VSEG_CHURN_VL] != 0x5aa5U) {
        return false;
    }

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_CHURN_VL; ++lane) {
        for (unsigned field = 0; field < AI_ARCH_VSEG_CHURN_E16_NF;
             ++field) {
            const size_t slot =
                2U + lane * AI_ARCH_VSEG_CHURN_E16_NF + field;

            if (ai_arch_vseg_e16_dst[slot] !=
                ai_arch_vseg_e16_src[field][lane]) {
                return false;
            }
        }
    }

    return true;
}

static bool ai_arch_vseg_stride_image_ok(void)
{
    for (unsigned lane = 0; lane < AI_ARCH_VSEG_CHURN_VL; ++lane) {
        const size_t base = lane * AI_ARCH_VSEG_CHURN_STRIDE_WORDS;

        if (ai_arch_vseg_stride_dst[base] != ai_arch_vseg_e32_src0[lane] ||
            ai_arch_vseg_stride_dst[base + 1U] !=
                ai_arch_vseg_e32_src1[lane]) {
            return false;
        }
        for (size_t gap = 2U; gap < AI_ARCH_VSEG_CHURN_STRIDE_WORDS;
             ++gap) {
            if (ai_arch_vseg_stride_dst[base + gap] != 0xdedededeU) {
                return false;
            }
        }
    }

    return ai_arch_vseg_stride_dst[
               AI_ARCH_VSEG_CHURN_STRIDE_WORDS *
               AI_ARCH_VSEG_CHURN_VL] == 0xabcdef01U;
}

static bool ai_arch_vseg_index_image_ok(void)
{
    for (unsigned lane = 0; lane < AI_ARCH_VSEG_CHURN_VL; ++lane) {
        const size_t offset = ai_arch_vseg_index_offsets[lane];

        if (ai_arch_vseg_read32(ai_arch_vseg_index_dst, offset) !=
                ai_arch_vseg_e32_src0[lane] ||
            ai_arch_vseg_read32(ai_arch_vseg_index_dst, offset + 4U) !=
                ai_arch_vseg_e32_src1[lane]) {
            return false;
        }
    }

    for (size_t i = 0; i < AI_ARCH_VSEG_CHURN_INDEX_BYTES; ++i) {
        bool covered = false;

        for (unsigned lane = 0; lane < AI_ARCH_VSEG_CHURN_VL; ++lane) {
            const size_t offset = ai_arch_vseg_index_offsets[lane];

            if (i >= offset && i < offset + 8U) {
                covered = true;
            }
        }
        if (!covered && ai_arch_vseg_index_dst[i] != 0xccU) {
            return false;
        }
    }

    return true;
}

static bool ai_arch_vseg_nf8_e8_image_ok(void)
{
    if (ai_arch_vseg_nf8_e8_dst[1] != 0x6dU ||
        ai_arch_vseg_nf8_e8_dst[
            2U + AI_ARCH_VSEG_NF8_E8_FIELDS * AI_ARCH_VSEG_NF8_E8_VL] !=
            0xd6U) {
        return false;
    }

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_NF8_E8_VL; ++lane) {
        for (unsigned field = 0; field < AI_ARCH_VSEG_NF8_E8_FIELDS;
             ++field) {
            const size_t slot =
                2U + lane * AI_ARCH_VSEG_NF8_E8_FIELDS + field;

            if (ai_arch_vseg_nf8_e8_dst[slot] !=
                ai_arch_vseg_nf8_e8_src[field][lane]) {
                return false;
            }
        }
    }

    return true;
}

static bool ai_arch_vseg_mask_e8_image_ok(void)
{
    if (ai_arch_vseg_mask_e8_dst[1] != 0x71U ||
        ai_arch_vseg_mask_e8_dst[2U + 2U * AI_ARCH_VSEG_MASK_E8_LANES] !=
            0x17U) {
        return false;
    }

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_MASK_E8_LANES; ++lane) {
        const bool active = lane < AI_ARCH_VSEG_MASK_E8_VL &&
            ((ai_arch_vseg_mask_e8_bits[0] & (1U << lane)) != 0U);
        const uint8_t expected0 = active ? ai_arch_vseg_mask_e8_src0[lane] :
            0xeeU;
        const uint8_t expected1 = active ? ai_arch_vseg_mask_e8_src1[lane] :
            0xeeU;
        const size_t slot = 2U + lane * 2U;

        if (ai_arch_vseg_mask_e8_dst[slot] != expected0 ||
            ai_arch_vseg_mask_e8_dst[slot + 1U] != expected1) {
            return false;
        }
    }

    return true;
}

static bool ai_arch_vseg_cross32_image_ok(void)
{
    const size_t base = AI_ARCH_VSEG_CROSS16_E32_OFFSET;

    if (ai_arch_vseg_cross32_dst[base - 1U] != 0x42U ||
        ai_arch_vseg_cross32_dst[
            base + AI_ARCH_VSEG_CROSS16_E32_BYTES] != 0x24U) {
        return false;
    }

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_CROSS16_E32_VL; ++lane) {
        const size_t slot = base + lane * 2U * sizeof(uint32_t);

        if (ai_arch_vseg_read32(ai_arch_vseg_cross32_dst, slot) !=
                ai_arch_vseg_cross32_src0[lane] ||
            ai_arch_vseg_read32(ai_arch_vseg_cross32_dst,
                slot + sizeof(uint32_t)) !=
                ai_arch_vseg_cross32_src1[lane]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_arch_vseg_idx_boundary_lane_active(unsigned lane,
    bool masked)
{
    return !masked ||
        ((AI_ARCH_VSEG_INDEX_BOUNDARY_MASK & (1U << lane)) != 0U);
}

static bool ai_arch_vseg_idx_boundary_store_image_ok(bool masked)
{
    for (unsigned lane = 0; lane < AI_ARCH_VSEG_INDEX_BOUNDARY_VL; ++lane) {
        const size_t offset = ai_arch_vseg_idx_boundary_offsets[lane];
        const bool active = ai_arch_vseg_idx_boundary_lane_active(lane, masked);

        if (active) {
            if (ai_arch_vseg_read32(ai_arch_vseg_idx_boundary_dst, offset) !=
                    ai_arch_vseg_idx_boundary_src0[lane] ||
                ai_arch_vseg_read32(ai_arch_vseg_idx_boundary_dst,
                    offset + sizeof(uint32_t)) !=
                    ai_arch_vseg_idx_boundary_src1[lane]) {
                return false;
            }
        } else {
            if (ai_arch_vseg_read32(ai_arch_vseg_idx_boundary_dst, offset) !=
                    0xccccccccU ||
                ai_arch_vseg_read32(ai_arch_vseg_idx_boundary_dst,
                    offset + sizeof(uint32_t)) != 0xccccccccU) {
                return false;
            }
        }
    }

    for (size_t byte = 0; byte < AI_ARCH_VSEG_INDEX_BOUNDARY_BYTES; ++byte) {
        bool covered = false;

        for (unsigned lane = 0; lane < AI_ARCH_VSEG_INDEX_BOUNDARY_VL; ++lane) {
            const size_t offset = ai_arch_vseg_idx_boundary_offsets[lane];

            if (ai_arch_vseg_idx_boundary_lane_active(lane, masked) &&
                byte >= offset && byte < offset + 2U * sizeof(uint32_t)) {
                covered = true;
            }
        }
        if (!covered && ai_arch_vseg_idx_boundary_dst[byte] != 0xccU) {
            return false;
        }
    }

    return true;
}

static bool ai_arch_vseg_idx_boundary_load_dump_ok(bool masked)
{
    for (unsigned lane = 0; lane < AI_ARCH_VSEG_INDEX_BOUNDARY_VL; ++lane) {
        const bool active = ai_arch_vseg_idx_boundary_lane_active(lane, masked);
        const uint32_t expected0 = active ?
            ai_arch_vseg_read32(ai_arch_vseg_idx_boundary_dst,
                ai_arch_vseg_idx_boundary_offsets[lane]) :
            ai_arch_vseg_idx_boundary_seed0[lane];
        const uint32_t expected1 = active ?
            ai_arch_vseg_read32(ai_arch_vseg_idx_boundary_dst,
                ai_arch_vseg_idx_boundary_offsets[lane] + sizeof(uint32_t)) :
            ai_arch_vseg_idx_boundary_seed1[lane];

        if (ai_arch_vseg_idx_boundary_dump0[lane] != expected0 ||
            ai_arch_vseg_idx_boundary_dump1[lane] != expected1) {
            return false;
        }
    }

    return true;
}

bool ai_arch_vsegment_store_field_data_churn_corner()
{
    TEST_START();

    static const uint16_t e16_patterns[AI_ARCH_VSEG_CHURN_E16_NF]
        [AI_ARCH_VSEG_CHURN_VL] = {
        { 0x1001U, 0x1002U, 0x1003U, 0x1004U },
        { 0x2111U, 0x2112U, 0x2113U, 0x2114U },
        { 0x3221U, 0x3222U, 0x3223U, 0x3224U },
        { 0x4331U, 0x4332U, 0x4333U, 0x4334U },
    };
    static const uint32_t strided_src0[AI_ARCH_VSEG_CHURN_VL] = {
        0xa0000101U, 0xa0000102U, 0xa0000103U, 0xa0000104U,
    };
    static const uint32_t strided_src1[AI_ARCH_VSEG_CHURN_VL] = {
        0xb0000201U, 0xb0000202U, 0xb0000203U, 0xb0000204U,
    };
    static const uint32_t indexed_src0[AI_ARCH_VSEG_CHURN_VL] = {
        0xc1000301U, 0xc1000302U, 0xc1000303U, 0xc1000304U,
    };
    static const uint32_t indexed_src1[AI_ARCH_VSEG_CHURN_VL] = {
        0xd2000401U, 0xd2000402U, 0xd2000403U, 0xd2000404U,
    };
    static const uint32_t index_offsets[AI_ARCH_VSEG_CHURN_VL] = {
        40U, 0U, 24U, 56U,
    };

    ai_arch_vseg_enable_state();

    for (unsigned field = 0; field < AI_ARCH_VSEG_CHURN_E16_NF; ++field) {
        for (unsigned lane = 0; lane < AI_ARCH_VSEG_CHURN_VL; ++lane) {
            ai_arch_vseg_e16_src[field][lane] = e16_patterns[field][lane];
        }
    }
    for (size_t i = 0; i < sizeof(ai_arch_vseg_e16_dst) /
            sizeof(ai_arch_vseg_e16_dst[0]); ++i) {
        ai_arch_vseg_e16_dst[i] = 0xeeeeU;
    }
    ai_arch_vseg_e16_dst[1] = 0xa55aU;
    ai_arch_vseg_e16_dst[
        2U + AI_ARCH_VSEG_CHURN_E16_NF * AI_ARCH_VSEG_CHURN_VL] =
        0x5aa5U;

    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vsseg4e16(&ai_arch_vseg_e16_dst[2]);
    TEST_ASSERT("vsegment data producer: vsseg4e16 writes every field/lane halfword from its matching source register",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_CHURN_VL &&
        ai_arch_vseg_e16_image_ok()
    );

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_CHURN_VL; ++lane) {
        ai_arch_vseg_e32_src0[lane] = strided_src0[lane];
        ai_arch_vseg_e32_src1[lane] = strided_src1[lane];
    }
    for (size_t i = 0; i < sizeof(ai_arch_vseg_stride_dst) /
            sizeof(ai_arch_vseg_stride_dst[0]); ++i) {
        ai_arch_vseg_stride_dst[i] = 0xdedededeU;
    }
    ai_arch_vseg_stride_dst[
        AI_ARCH_VSEG_CHURN_STRIDE_WORDS * AI_ARCH_VSEG_CHURN_VL] =
        0xabcdef01U;

    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vssseg2e32(ai_arch_vseg_stride_dst,
        (intptr_t)(AI_ARCH_VSEG_CHURN_STRIDE_WORDS * sizeof(uint32_t)));
    TEST_ASSERT("vsegment data producer: vssseg2e32 after nf4/e16 churn uses current e32 field data and preserves stride gaps",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_CHURN_VL &&
        ai_arch_vseg_stride_image_ok()
    );

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_CHURN_VL; ++lane) {
        ai_arch_vseg_e32_src0[lane] = indexed_src0[lane];
        ai_arch_vseg_e32_src1[lane] = indexed_src1[lane];
        ai_arch_vseg_index_offsets[lane] = index_offsets[lane];
    }
    for (size_t i = 0; i < sizeof(ai_arch_vseg_index_dst); ++i) {
        ai_arch_vseg_index_dst[i] = 0xccU;
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vsuxseg2ei32(ai_arch_vseg_index_dst);
    TEST_ASSERT("vsegment data producer: vsuxseg2ei32 no-duplicate index store keeps current field/lane payloads despite nonmonotonic addresses",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_CHURN_VL &&
        ai_arch_vseg_index_image_ok()
    );

    TEST_END();
}

bool ai_arch_indexed_vsegment_store_load_boundary_owner_corner()
{
    TEST_START();

    static const uint32_t offsets[AI_ARCH_VSEG_INDEX_BOUNDARY_VL] = {
        0x70U, 0x10U, 0xa0U, 0x38U,
        0xe0U, 0x58U, 0xc0U, 0x90U,
    };
    static const uint32_t store_src0[AI_ARCH_VSEG_INDEX_BOUNDARY_VL] = {
        0x11000101U, 0x11000112U, 0x11000123U, 0x11000134U,
        0x22000445U, 0x22000456U, 0x22000467U, 0x22000478U,
    };
    static const uint32_t store_src1[AI_ARCH_VSEG_INDEX_BOUNDARY_VL] = {
        0x33000201U, 0x33000212U, 0x33000223U, 0x33000234U,
        0x44000545U, 0x44000556U, 0x44000567U, 0x44000578U,
    };
    static const uint32_t load_field0[AI_ARCH_VSEG_INDEX_BOUNDARY_VL] = {
        0x51001000U, 0x51001011U, 0x51001022U, 0x51001033U,
        0x62002044U, 0x62002055U, 0x62002066U, 0x62002077U,
    };
    static const uint32_t load_field1[AI_ARCH_VSEG_INDEX_BOUNDARY_VL] = {
        0x73003000U, 0x73003011U, 0x73003022U, 0x73003033U,
        0x84004044U, 0x84004055U, 0x84004066U, 0x84004077U,
    };

    ai_arch_vseg_enable_state();
    ai_arch_vseg_idx_boundary_mask[0] = AI_ARCH_VSEG_INDEX_BOUNDARY_MASK;

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_INDEX_BOUNDARY_VL; ++lane) {
        ai_arch_vseg_idx_boundary_offsets[lane] = offsets[lane];
        ai_arch_vseg_idx_boundary_src0[lane] = store_src0[lane];
        ai_arch_vseg_idx_boundary_src1[lane] = store_src1[lane];
        ai_arch_vseg_idx_boundary_seed0[lane] = 0xdead0000U + lane;
        ai_arch_vseg_idx_boundary_seed1[lane] = 0xbeef0000U + lane;
    }

    ai_arch_vseg_fill_bytes(ai_arch_vseg_idx_boundary_dst,
        sizeof(ai_arch_vseg_idx_boundary_dst), 0xccU);
    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vsuxseg2ei32_boundary(ai_arch_vseg_idx_boundary_dst, false);
    TEST_ASSERT("indexed VSegment store boundary: unmasked vsuxseg2ei32 e32,m2 uses the high-lane index payload instead of reusing the first index uop",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_INDEX_BOUNDARY_VL &&
        ai_arch_vseg_idx_boundary_store_image_ok(false)
    );

    ai_arch_vseg_fill_bytes(ai_arch_vseg_idx_boundary_dst,
        sizeof(ai_arch_vseg_idx_boundary_dst), 0xccU);
    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vsuxseg2ei32_boundary(ai_arch_vseg_idx_boundary_dst, true);
    TEST_ASSERT("indexed VSegment store boundary: masked vsuxseg2ei32 writes only active lanes across the index-uop boundary and leaves inactive target slots untouched",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_INDEX_BOUNDARY_VL &&
        ai_arch_vseg_idx_boundary_store_image_ok(true)
    );

    ai_arch_vseg_fill_bytes(ai_arch_vseg_idx_boundary_dst,
        sizeof(ai_arch_vseg_idx_boundary_dst), 0xccU);
    for (unsigned lane = 0; lane < AI_ARCH_VSEG_INDEX_BOUNDARY_VL; ++lane) {
        ai_arch_vseg_write32((uint32_t *)(void *)ai_arch_vseg_idx_boundary_dst,
            offsets[lane] / sizeof(uint32_t), load_field0[lane]);
        ai_arch_vseg_write32((uint32_t *)(void *)ai_arch_vseg_idx_boundary_dst,
            offsets[lane] / sizeof(uint32_t) + 1U, load_field1[lane]);
        ai_arch_vseg_idx_boundary_dump0[lane] = 0U;
        ai_arch_vseg_idx_boundary_dump1[lane] = 0U;
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vluxseg2ei32_boundary(ai_arch_vseg_idx_boundary_dst, false);
    TEST_ASSERT("indexed VSegment load boundary: unmasked vluxseg2ei32 e32,m2 dumps every lane from its own nonmonotonic offset across the index-uop boundary",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_INDEX_BOUNDARY_VL &&
        ai_arch_vseg_idx_boundary_load_dump_ok(false)
    );

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_INDEX_BOUNDARY_VL; ++lane) {
        ai_arch_vseg_idx_boundary_dump0[lane] = 0U;
        ai_arch_vseg_idx_boundary_dump1[lane] = 0U;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vluxseg2ei32_boundary(ai_arch_vseg_idx_boundary_dst, true);
    TEST_ASSERT("indexed VSegment load boundary: masked vluxseg2ei32 preserves inactive destination lanes while active high lanes still use their own index payload",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_INDEX_BOUNDARY_VL &&
        ai_arch_vseg_idx_boundary_load_dump_ok(true)
    );

    TEST_END();
}

bool ai_arch_vsegment_store_mask_tail_cross16_payload_owner_corner()
{
    TEST_START();

    static const uint8_t nf8_patterns[AI_ARCH_VSEG_NF8_E8_FIELDS]
        [AI_ARCH_VSEG_NF8_E8_VL] = {
        { 0x10U, 0x18U },
        { 0x21U, 0x29U },
        { 0x32U, 0x3aU },
        { 0x43U, 0x4bU },
        { 0x54U, 0x5cU },
        { 0x65U, 0x6dU },
        { 0x76U, 0x7eU },
        { 0x87U, 0x8fU },
    };
    static const uint8_t mask_src0[AI_ARCH_VSEG_MASK_E8_LANES] = {
        0x91U, 0x92U, 0x93U, 0x94U,
        0x95U, 0x96U, 0x97U, 0x98U,
    };
    static const uint8_t mask_src1[AI_ARCH_VSEG_MASK_E8_LANES] = {
        0xa1U, 0xa2U, 0xa3U, 0xa4U,
        0xa5U, 0xa6U, 0xa7U, 0xa8U,
    };
    static const uint32_t cross_src0[AI_ARCH_VSEG_CROSS16_E32_VL] = {
        0xc3100011U, 0xc3100022U,
    };
    static const uint32_t cross_src1[AI_ARCH_VSEG_CROSS16_E32_VL] = {
        0xd4200033U, 0xd4200044U,
    };

    ai_arch_vseg_enable_state();

    for (unsigned field = 0; field < AI_ARCH_VSEG_NF8_E8_FIELDS; ++field) {
        for (unsigned lane = 0; lane < AI_ARCH_VSEG_NF8_E8_VL; ++lane) {
            ai_arch_vseg_write8(ai_arch_vseg_nf8_e8_src[field], lane,
                nf8_patterns[field][lane]);
        }
    }
    ai_arch_vseg_fill_bytes(ai_arch_vseg_nf8_e8_dst,
        sizeof(ai_arch_vseg_nf8_e8_dst), 0xeeU);
    ai_arch_vseg_write8(ai_arch_vseg_nf8_e8_dst, 1U, 0x6dU);
    ai_arch_vseg_write8(ai_arch_vseg_nf8_e8_dst,
        2U + AI_ARCH_VSEG_NF8_E8_FIELDS * AI_ARCH_VSEG_NF8_E8_VL, 0xd6U);

    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vsseg8e8_direct(&ai_arch_vseg_nf8_e8_dst[2]);
    TEST_ASSERT("vsegment data producer: vsseg8e8 keeps all eight fields bound to the current per-lane payload instead of stale splitPtr data",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_NF8_E8_VL &&
        ai_arch_vseg_nf8_e8_image_ok()
    );

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_MASK_E8_LANES; ++lane) {
        ai_arch_vseg_write8(ai_arch_vseg_mask_e8_src0, lane, mask_src0[lane]);
        ai_arch_vseg_write8(ai_arch_vseg_mask_e8_src1, lane, mask_src1[lane]);
    }
    ai_arch_vseg_fill_bytes(ai_arch_vseg_mask_e8_dst,
        sizeof(ai_arch_vseg_mask_e8_dst), 0xeeU);
    ai_arch_vseg_write8(ai_arch_vseg_mask_e8_dst, 1U, 0x71U);
    ai_arch_vseg_write8(ai_arch_vseg_mask_e8_dst,
        2U + 2U * AI_ARCH_VSEG_MASK_E8_LANES, 0x17U);
    ai_arch_vseg_write8(ai_arch_vseg_mask_e8_bits, 0U,
        (uint8_t)((1U << 0U) | (1U << 2U) | (1U << 5U)));

    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vsseg2e8_masked_direct(&ai_arch_vseg_mask_e8_dst[2]);
    TEST_ASSERT("vsegment data producer: masked vsseg2e8 writes only active lanes and preserves inactive plus tail lanes with their old memory image",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_MASK_E8_VL &&
        ai_arch_vseg_mask_e8_image_ok()
    );

    for (unsigned lane = 0; lane < AI_ARCH_VSEG_CROSS16_E32_VL; ++lane) {
        ai_arch_vseg_write32(ai_arch_vseg_cross32_src0, lane,
            cross_src0[lane]);
        ai_arch_vseg_write32(ai_arch_vseg_cross32_src1, lane,
            cross_src1[lane]);
    }
    ai_arch_vseg_fill_bytes(ai_arch_vseg_cross32_dst,
        sizeof(ai_arch_vseg_cross32_dst), 0x7eU);
    ai_arch_vseg_write8(ai_arch_vseg_cross32_dst,
        AI_ARCH_VSEG_CROSS16_E32_OFFSET - 1U, 0x42U);
    ai_arch_vseg_write8(ai_arch_vseg_cross32_dst,
        AI_ARCH_VSEG_CROSS16_E32_OFFSET + AI_ARCH_VSEG_CROSS16_E32_BYTES,
        0x24U);

    TEST_SETUP_EXCEPT();
    ai_arch_vseg_vsseg2e32_cross16_direct(
        &ai_arch_vseg_cross32_dst[AI_ARCH_VSEG_CROSS16_E32_OFFSET]);
    TEST_ASSERT("vsegment data producer: low4=12 vsseg2e32 crosses the 16B boundary without swapping field0/field1 or reusing an old lane payload",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VSEG_CROSS16_E32_VL &&
        ai_arch_vseg_cross32_image_ok()
    );

    TEST_END();
}
