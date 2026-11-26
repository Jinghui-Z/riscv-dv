#include <rvh_test.h>
#include <stdint.h>
#include <stddef.h>

#define AI_ARCH_VEC_UNIT_BYTES 16U
#define AI_ARCH_VEC_E8_VL 16U
#define AI_ARCH_VEC_E16_VL 8U
#define AI_ARCH_VEC_E32_VL 4U
#define AI_ARCH_VEC_E64_VL 2U
#define AI_ARCH_VEC_STRIDE_VL 4U
#define AI_ARCH_VEC_STRIDE_GAP 3U
#define AI_ARCH_VEC_UNIT_DST_OFFSET 8U
#define AI_ARCH_VEC_INDEX_MEM_BYTES 320U
#define AI_ARCH_VEC_INDEX_BASE_OFFSET 128U
#define AI_ARCH_VEC_CTRL_VL 4U
#define AI_ARCH_VEC_SEG_VL 4U
#define AI_ARCH_VEC_SEG_NF 3U
#define AI_ARCH_VEC_SEG_NF_SWEEP_VL 2U
#define AI_ARCH_VEC_SEG_WORDS (AI_ARCH_VEC_SEG_VL * AI_ARCH_VEC_SEG_NF)
#define AI_ARCH_VEC_SEG_DST_OFFSET 4U
#define AI_ARCH_VEC_WHOLE_MAX_BYTES 4096U
#define AI_ARCH_VEC_WHOLE_DST_OFFSET 8U
#define AI_ARCH_VEC_GUARD_LO 0xa5U
#define AI_ARCH_VEC_GUARD_HI 0x5aU

static uint8_t ai_arch_vec_unit_src[AI_ARCH_VEC_UNIT_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vec_unit_dst[
    AI_ARCH_VEC_UNIT_DST_OFFSET + AI_ARCH_VEC_UNIT_BYTES + 8U]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_stride_src[AI_ARCH_VEC_STRIDE_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_stride_dump[AI_ARCH_VEC_STRIDE_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_stride_mem[AI_ARCH_VEC_STRIDE_VL *
    AI_ARCH_VEC_STRIDE_GAP + 2U] __attribute__((aligned(64)));
static uint32_t ai_arch_vec_index_offsets[AI_ARCH_VEC_STRIDE_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_index_target[3] __attribute__((aligned(64)));
static uint8_t ai_arch_vec_index_mem[AI_ARCH_VEC_INDEX_MEM_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vec_index_offsets8[AI_ARCH_VEC_STRIDE_VL]
    __attribute__((aligned(64)));
static uint16_t ai_arch_vec_index_offsets16[AI_ARCH_VEC_STRIDE_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_index_offsets32[AI_ARCH_VEC_STRIDE_VL]
    __attribute__((aligned(64)));
static uint64_t ai_arch_vec_index_offsets64[AI_ARCH_VEC_STRIDE_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_index_dump[AI_ARCH_VEC_STRIDE_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_ctrl_src[AI_ARCH_VEC_CTRL_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_ctrl_old[AI_ARCH_VEC_CTRL_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_ctrl_dump[AI_ARCH_VEC_CTRL_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_ctrl_target[AI_ARCH_VEC_CTRL_VL]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_segment_src[AI_ARCH_VEC_SEG_WORDS]
    __attribute__((aligned(64)));
static uint32_t ai_arch_vec_segment_dump[AI_ARCH_VEC_SEG_WORDS]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vec_segment_dst[
    AI_ARCH_VEC_SEG_DST_OFFSET + AI_ARCH_VEC_SEG_WORDS * sizeof(uint32_t) + 4U]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vec_whole_src[AI_ARCH_VEC_WHOLE_MAX_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_vec_whole_dst[
    AI_ARCH_VEC_WHOLE_DST_OFFSET + AI_ARCH_VEC_WHOLE_MAX_BYTES + 8U]
    __attribute__((aligned(64)));

static inline void ai_arch_vec_enable_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint64_t ai_arch_vec_read_vlenb(void)
{
    uint64_t vlenb;

    asm volatile("csrr %0, vlenb" : "=r"(vlenb));
    return vlenb;
}

static inline void ai_arch_vec_unit_reset_dst(uint8_t sentinel)
{
    for (size_t i = 0; i < sizeof(ai_arch_vec_unit_dst); ++i) {
        ai_arch_vec_unit_dst[i] = sentinel;
    }
    ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET - 1U] =
        AI_ARCH_VEC_GUARD_LO;
    ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET +
        AI_ARCH_VEC_UNIT_BYTES] =
        AI_ARCH_VEC_GUARD_HI;
}

static inline bool ai_arch_vec_bytes_equal(const uint8_t *actual,
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

static inline bool ai_arch_vec_stride_slots_match(const uint32_t *mem,
    const uint32_t *expected, const uint32_t sentinel, const unsigned gap,
    const unsigned vl, int *bad_slot, uint32_t *bad_actual,
    uint32_t *bad_expected)
{
    const unsigned slots = vl * gap + 1U;

    for (unsigned slot = 0; slot < slots; ++slot) {
        const bool active = (slot % gap) == 0U && (slot / gap) < vl;
        const uint32_t exp = active ? expected[slot / gap] : sentinel;
        if (mem[slot] != exp) {
            *bad_slot = (int)slot;
            *bad_actual = mem[slot];
            *bad_expected = exp;
            return false;
        }
    }
    *bad_slot = -1;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static inline void ai_arch_vec_index_write32(size_t offset, uint32_t value)
{
    *(volatile uint32_t *)(void *)(ai_arch_vec_index_mem + offset) = value;
}

static inline uint32_t ai_arch_vec_index_read32(size_t offset)
{
    return *(volatile uint32_t *)(void *)(ai_arch_vec_index_mem + offset);
}

static inline bool ai_arch_vec_words_equal(const uint32_t *actual,
    const uint32_t *expected, unsigned words, int *bad_slot,
    uint32_t *bad_actual, uint32_t *bad_expected)
{
    for (unsigned i = 0; i < words; ++i) {
        if (actual[i] != expected[i]) {
            *bad_slot = (int)i;
            *bad_actual = actual[i];
            *bad_expected = expected[i];
            return false;
        }
    }
    *bad_slot = -1;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static inline bool ai_arch_vec_word_is_one_of(uint32_t value,
    const uint32_t *allowed, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        if (value == allowed[i]) {
            return true;
        }
    }
    return false;
}

static inline void ai_arch_vec_extract_vle8_store(const uint8_t *src,
    uint8_t *dst, uint64_t *elem0, uint64_t *elem1)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v6, (%[src])\n\t"
        "vmv.x.s %[elem0], v6\n\t"
        "vslidedown.vi v8, v6, 1\n\t"
        "vmv.x.s %[elem1], v8\n\t"
        "vse8.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [elem0] "=&r"(*elem0),
          [elem1] "=&r"(*elem1)
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_VEC_E8_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_extract_vle16_store(const uint16_t *src,
    uint16_t *dst, uint64_t *elem0, uint64_t *elem1)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, ta, ma\n\t"
        "vle16.v v6, (%[src])\n\t"
        "vmv.x.s %[elem0], v6\n\t"
        "vslidedown.vi v8, v6, 1\n\t"
        "vmv.x.s %[elem1], v8\n\t"
        "vse16.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [elem0] "=&r"(*elem0),
          [elem1] "=&r"(*elem1)
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_VEC_E16_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_extract_vle32_store(const uint32_t *src,
    uint32_t *dst, uint64_t *elem0, uint64_t *elem1)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vmv.x.s %[elem0], v6\n\t"
        "vslidedown.vi v8, v6, 1\n\t"
        "vmv.x.s %[elem1], v8\n\t"
        "vse32.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [elem0] "=&r"(*elem0),
          [elem1] "=&r"(*elem1)
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_VEC_E32_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_extract_vle64_store(const uint64_t *src,
    uint64_t *dst, uint64_t *elem0, uint64_t *elem1)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m1, ta, ma\n\t"
        "vle64.v v6, (%[src])\n\t"
        "vmv.x.s %[elem0], v6\n\t"
        "vslidedown.vi v8, v6, 1\n\t"
        "vmv.x.s %[elem1], v8\n\t"
        "vse64.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [elem0] "=&r"(*elem0),
          [elem1] "=&r"(*elem1)
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_VEC_E64_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vlse32_dump(const uint32_t *base,
    intptr_t stride_bytes, uint32_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlse32.v v6, (%[base]), %[stride]\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [stride] "r"(stride_bytes),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_VEC_STRIDE_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vlse32_zero_stride_dump(const uint32_t *base,
    uint32_t *dump)
{
    register intptr_t zero_stride_reg asm("a2") = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlse32.v v6, (%[base]), %[stride]\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [stride] "r"(zero_stride_reg),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_VEC_STRIDE_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vsse32_from_contiguous(const uint32_t *src,
    uint32_t *base, intptr_t stride_bytes)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vsse32.v v6, (%[base]), %[stride]\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [base] "r"(base),
          [stride] "r"(stride_bytes),
          [vl] "r"((unsigned)AI_ARCH_VEC_STRIDE_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vsoxei32_repeated_ordered_store(
    const uint32_t *src, const uint32_t *offsets, uint32_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsoxei32.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_ARCH_VEC_STRIDE_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vluxei8_dump(const uint8_t *base,
    const uint8_t *offsets, uint32_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vluxei8.v v6, (%[base]), v8\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_VEC_STRIDE_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vluxei16_dump(const uint8_t *base,
    const uint16_t *offsets, uint32_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, ta, ma\n\t"
        "vle16.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vluxei16.v v6, (%[base]), v8\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_VEC_STRIDE_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vluxei32_dump(const uint8_t *base,
    const uint32_t *offsets, uint32_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vluxei32.v v6, (%[base]), v8\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_VEC_STRIDE_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vsuxei32_repeated_unordered_store(
    const uint32_t *src, const uint32_t *offsets, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsuxei32.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_ARCH_VEC_STRIDE_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vlseg3e32_dump(const uint32_t *src,
    uint32_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlseg3e32.v v6, (%[src])\n\t"
        "vsseg3e32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_VEC_SEG_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vlseg3e32_field_dump(const uint32_t *src,
    uint32_t *dump0, uint32_t *dump1, uint32_t *dump2)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlseg3e32.v v6, (%[src])\n\t"
        "vse32.v v6, (%[dump0])\n\t"
        "vse32.v v7, (%[dump1])\n\t"
        "vse32.v v8, (%[dump2])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dump0] "r"(dump0),
          [dump1] "r"(dump1),
          [dump2] "r"(dump2),
          [vl] "r"((unsigned)AI_ARCH_VEC_SEG_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vlseg_e8_roundtrip(const uint8_t *src,
    uint8_t *dst, unsigned nf)
{
    switch (nf) {
    case 1:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
            "vle8.v v16, (%[src])\n\t"
            "vse8.v v16, (%[dst])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src] "r"(src),
              [dst] "r"(dst),
              [vl] "r"((unsigned)AI_ARCH_VEC_SEG_NF_SWEEP_VL)
            : "t0", "memory");
        break;
    case 2:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
            "vlseg2e8.v v16, (%[src])\n\t"
            "vsseg2e8.v v16, (%[dst])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src] "r"(src),
              [dst] "r"(dst),
              [vl] "r"((unsigned)AI_ARCH_VEC_SEG_NF_SWEEP_VL)
            : "t0", "memory");
        break;
    case 3:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
            "vlseg3e8.v v16, (%[src])\n\t"
            "vsseg3e8.v v16, (%[dst])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src] "r"(src),
              [dst] "r"(dst),
              [vl] "r"((unsigned)AI_ARCH_VEC_SEG_NF_SWEEP_VL)
            : "t0", "memory");
        break;
    case 4:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
            "vlseg4e8.v v16, (%[src])\n\t"
            "vsseg4e8.v v16, (%[dst])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src] "r"(src),
              [dst] "r"(dst),
              [vl] "r"((unsigned)AI_ARCH_VEC_SEG_NF_SWEEP_VL)
            : "t0", "memory");
        break;
    case 5:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
            "vlseg5e8.v v16, (%[src])\n\t"
            "vsseg5e8.v v16, (%[dst])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src] "r"(src),
              [dst] "r"(dst),
              [vl] "r"((unsigned)AI_ARCH_VEC_SEG_NF_SWEEP_VL)
            : "t0", "memory");
        break;
    case 6:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
            "vlseg6e8.v v16, (%[src])\n\t"
            "vsseg6e8.v v16, (%[dst])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src] "r"(src),
              [dst] "r"(dst),
              [vl] "r"((unsigned)AI_ARCH_VEC_SEG_NF_SWEEP_VL)
            : "t0", "memory");
        break;
    case 7:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
            "vlseg7e8.v v16, (%[src])\n\t"
            "vsseg7e8.v v16, (%[dst])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src] "r"(src),
              [dst] "r"(dst),
              [vl] "r"((unsigned)AI_ARCH_VEC_SEG_NF_SWEEP_VL)
            : "t0", "memory");
        break;
    default:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
            "vlseg8e8.v v16, (%[src])\n\t"
            "vsseg8e8.v v16, (%[dst])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [src] "r"(src),
              [dst] "r"(dst),
              [vl] "r"((unsigned)AI_ARCH_VEC_SEG_NF_SWEEP_VL)
            : "t0", "memory");
        break;
    }
}

static inline void ai_arch_vec_whole_1r_copy(const uint8_t *src, uint8_t *dst,
    uint64_t *vl_before_store)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vl1re8.v v6, (%[src])\n\t"
        "csrr %[vl_before_store], vl\n\t"
        "vs1r.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [vl_before_store] "=&r"(*vl_before_store)
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"(1U)
        : "t0", "memory");
}

static inline void ai_arch_vec_whole_2r_copy(const uint8_t *src, uint8_t *dst,
    uint64_t *vl_before_store)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m1, tu, mu\n\t"
        "vl2re8.v v6, (%[src])\n\t"
        "csrr %[vl_before_store], vl\n\t"
        "vs2r.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [vl_before_store] "=&r"(*vl_before_store)
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"(2U)
        : "t0", "memory");
}

static inline void ai_arch_vec_masked_vluxei64_even_dump(const uint8_t *base,
    const uint64_t *offsets, const uint32_t *old_src, uint32_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[old_src])\n\t"
        "vid.v v4\n\t"
        "vand.vi v4, v4, 1\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vluxei64.v v6, (%[base]), v8, v0.t\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [old_src] "r"(old_src),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_masked_vloxei64_even_dump(const uint8_t *base,
    const uint64_t *offsets, const uint32_t *old_src, uint32_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[old_src])\n\t"
        "vid.v v4\n\t"
        "vand.vi v4, v4, 1\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vloxei64.v v6, (%[base]), v8, v0.t\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [old_src] "r"(old_src),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_masked_vsuxei64_even_store(const uint32_t *src,
    const uint64_t *offsets, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vid.v v4\n\t"
        "vand.vi v4, v4, 1\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vsuxei64.v v6, (%[base]), v8, v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_tail_load_dump(const uint32_t *old_src,
    const uint32_t *new_src, uint32_t *dump, int agnostic)
{
    const char *policy = agnostic ? "ta" : "tu";
    (void)policy;

    if (agnostic) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
            "vle32.v v6, (%[old_src])\n\t"
            "vsetvli t0, %[short_vl], e32, m1, ta, mu\n\t"
            "vle32.v v6, (%[new_src])\n\t"
            "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
            "vse32.v v6, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [old_src] "r"(old_src),
              [new_src] "r"(new_src),
              [dump] "r"(dump),
              [full_vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL),
              [short_vl] "r"(2U)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
            "vle32.v v6, (%[old_src])\n\t"
            "vsetvli t0, %[short_vl], e32, m1, tu, mu\n\t"
            "vle32.v v6, (%[new_src])\n\t"
            "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
            "vse32.v v6, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [old_src] "r"(old_src),
              [new_src] "r"(new_src),
              [dump] "r"(dump),
              [full_vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL),
              [short_vl] "r"(2U)
            : "t0", "memory");
    }
}

static inline void ai_arch_vec_masked_load_dump(const uint32_t *old_src,
    const uint32_t *new_src, uint32_t *dump, int agnostic)
{
    if (agnostic) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e32, m1, tu, ma\n\t"
            "vle32.v v6, (%[old_src])\n\t"
            "vid.v v4\n\t"
            "vand.vi v4, v4, 1\n\t"
            "vmseq.vi v0, v4, 0\n\t"
            "vle32.v v6, (%[new_src]), v0.t\n\t"
            "vse32.v v6, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [old_src] "r"(old_src),
              [new_src] "r"(new_src),
              [dump] "r"(dump),
              [vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
            "vle32.v v6, (%[old_src])\n\t"
            "vid.v v4\n\t"
            "vand.vi v4, v4, 1\n\t"
            "vmseq.vi v0, v4, 0\n\t"
            "vle32.v v6, (%[new_src]), v0.t\n\t"
            "vse32.v v6, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [old_src] "r"(old_src),
              [new_src] "r"(new_src),
              [dump] "r"(dump),
              [vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
            : "t0", "memory");
    }
}

static inline void ai_arch_vec_masked_store_even(const uint32_t *src,
    uint32_t *target)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vid.v v4\n\t"
        "vand.vi v4, v4, 1\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vse32.v v6, (%[target]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [target] "r"(target),
          [vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
        : "t0", "memory");
}

static inline void ai_arch_vec_vl0_load_keeps_old(const uint32_t *old_src,
    const uint32_t *new_src, uint32_t *dump, uint64_t *zero_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[old_src])\n\t"
        "li t1, 0\n\t"
        "vsetvli t0, t1, e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[new_src])\n\t"
        "csrr %[zero_vl], vl\n\t"
        "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [zero_vl] "=&r"(*zero_vl)
        : [old_src] "r"(old_src),
          [new_src] "r"(new_src),
          [dump] "r"(dump),
          [full_vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
        : "t0", "t1", "memory");
}

static inline void ai_arch_vec_vl0_store_keeps_memory(const uint32_t *src,
    uint32_t *target, uint64_t *zero_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[src])\n\t"
        "li t1, 0\n\t"
        "vsetvli t0, t1, e32, m1, tu, mu\n\t"
        "vse32.v v6, (%[target])\n\t"
        "csrr %[zero_vl], vl\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [zero_vl] "=&r"(*zero_vl)
        : [src] "r"(src),
          [target] "r"(target),
          [full_vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
        : "t0", "t1", "memory");
}

static inline void ai_arch_vec_vstart2_load_dump(const uint32_t *old_src,
    const uint32_t *new_src, uint32_t *dump, uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[old_src])\n\t"
        "li t1, 2\n\t"
        "csrw vstart, t1\n\t"
        "vle32.v v6, (%[new_src])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [old_src] "r"(old_src),
          [new_src] "r"(new_src),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
        : "t0", "t1", "memory");
}

static inline void ai_arch_vec_vstart2_store_suffix(const uint32_t *src,
    uint32_t *target, uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[src])\n\t"
        "li t1, 2\n\t"
        "csrw vstart, t1\n\t"
        "vse32.v v6, (%[target])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [target] "r"(target),
          [vl] "r"((unsigned)AI_ARCH_VEC_CTRL_VL)
        : "t0", "t1", "memory");
}

bool ai_arch_vector_unit_strided_lane_visible_matrix_corner()
{
    TEST_START();

    size_t bad_byte = 0;
    uint8_t bad_actual_byte = 0;
    uint8_t bad_expected_byte = 0;
    int bad_slot = -1;
    uint32_t bad_actual_word = 0;
    uint32_t bad_expected_word = 0;
    uint64_t elem0 = 0;
    uint64_t elem1 = 0;
    const intptr_t stride_bytes =
        (intptr_t)(AI_ARCH_VEC_STRIDE_GAP * sizeof(uint32_t));
    const uint32_t stride_sentinel = 0x6a6b6c6dU;
    const uint32_t ordered_final = 0xd0d0d004U;
    const uint8_t unit_pattern[AI_ARCH_VEC_UNIT_BYTES] = {
        0x12U, 0x34U, 0x34U, 0x12U,
        0x67U, 0x45U, 0x78U, 0x56U,
        0x34U, 0x12U, 0x08U, 0x07U,
        0x06U, 0x05U, 0x04U, 0x03U
    };
    const uint32_t stride_payload[AI_ARCH_VEC_STRIDE_VL] = {
        0x10203040U, 0x21314151U, 0x32425262U, 0x43536373U
    };
    const uint32_t pos_expected[AI_ARCH_VEC_STRIDE_VL] = {
        0x10203040U, 0x21314151U, 0x32425262U, 0x43536373U
    };
    const uint32_t neg_expected[AI_ARCH_VEC_STRIDE_VL] = {
        0x43536373U, 0x32425262U, 0x21314151U, 0x10203040U
    };
    const uint32_t index_offsets[AI_ARCH_VEC_STRIDE_VL] = {
        0U, 4U, 12U, 28U
    };
    const uint32_t index_expected[AI_ARCH_VEC_STRIDE_VL] = {
        0x81020304U, 0x92030405U, 0xa3040506U, 0xb4050607U
    };
    const size_t repeated_offset = AI_ARCH_VEC_INDEX_BASE_OFFSET + 64U;

    ai_arch_vec_enable_state();

    for (size_t i = 0; i < AI_ARCH_VEC_UNIT_BYTES; ++i) {
        ai_arch_vec_unit_src[i] = unit_pattern[i];
    }

    ai_arch_vec_unit_reset_dst(0xeeU);
    TEST_SETUP_EXCEPT();
    ai_arch_vec_extract_vle8_store(ai_arch_vec_unit_src,
        &ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET], &elem0, &elem1);
    TEST_ASSERT("vle8/vse8 unit-stride maps 16 active byte elements in increasing address order while preserving destination guards",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_E8_VL &&
        elem0 == 0x12ULL &&
        elem1 == 0x34ULL &&
        ai_arch_vec_bytes_equal(
            &ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET],
            ai_arch_vec_unit_src, AI_ARCH_VEC_UNIT_BYTES, &bad_byte,
            &bad_actual_byte, &bad_expected_byte) &&
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET - 1U] ==
            AI_ARCH_VEC_GUARD_LO &&
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET +
            AI_ARCH_VEC_UNIT_BYTES] == AI_ARCH_VEC_GUARD_HI,
        "vl=%lu elem0=0x%lx elem1=0x%lx bad_byte=%lu actual=0x%x expected=0x%x guard_lo=0x%x guard_hi=0x%x",
        CSRR(CSR_VL), elem0, elem1, bad_byte, bad_actual_byte,
        bad_expected_byte,
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET - 1U],
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET +
            AI_ARCH_VEC_UNIT_BYTES]
    );

    ai_arch_vec_unit_reset_dst(0xddU);
    TEST_SETUP_EXCEPT();
    ai_arch_vec_extract_vle16_store((const uint16_t *)ai_arch_vec_unit_src,
        (uint16_t *)&ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET],
        &elem0, &elem1);
    TEST_ASSERT("vle16/vse16 stitches little-endian halfword elements and writes only the active 16-byte contiguous image",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_E16_VL &&
        elem0 == 0x3412ULL &&
        elem1 == 0x1234ULL &&
        ai_arch_vec_bytes_equal(
            &ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET],
            ai_arch_vec_unit_src, AI_ARCH_VEC_UNIT_BYTES, &bad_byte,
            &bad_actual_byte, &bad_expected_byte) &&
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET - 1U] ==
            AI_ARCH_VEC_GUARD_LO &&
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET +
            AI_ARCH_VEC_UNIT_BYTES] == AI_ARCH_VEC_GUARD_HI,
        "vl=%lu elem0=0x%lx elem1=0x%lx bad_byte=%lu actual=0x%x expected=0x%x",
        CSRR(CSR_VL), elem0, elem1, bad_byte, bad_actual_byte,
        bad_expected_byte
    );

    ai_arch_vec_unit_reset_dst(0xccU);
    TEST_SETUP_EXCEPT();
    ai_arch_vec_extract_vle32_store((const uint32_t *)ai_arch_vec_unit_src,
        (uint32_t *)&ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET],
        &elem0, &elem1);
    TEST_ASSERT("vle32/vse32 scales consecutive element addresses by four bytes and preserves the little-endian word lane image",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_E32_VL &&
        elem0 == 0x12343412ULL &&
        elem1 == 0x56784567ULL &&
        ai_arch_vec_bytes_equal(
            &ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET],
            ai_arch_vec_unit_src, AI_ARCH_VEC_UNIT_BYTES, &bad_byte,
            &bad_actual_byte, &bad_expected_byte) &&
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET - 1U] ==
            AI_ARCH_VEC_GUARD_LO &&
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET +
            AI_ARCH_VEC_UNIT_BYTES] == AI_ARCH_VEC_GUARD_HI,
        "vl=%lu elem0=0x%lx elem1=0x%lx bad_byte=%lu actual=0x%x expected=0x%x",
        CSRR(CSR_VL), elem0, elem1, bad_byte, bad_actual_byte,
        bad_expected_byte
    );

    ai_arch_vec_unit_reset_dst(0xbbU);
    TEST_SETUP_EXCEPT();
    ai_arch_vec_extract_vle64_store((const uint64_t *)ai_arch_vec_unit_src,
        (uint64_t *)&ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET],
        &elem0, &elem1);
    TEST_ASSERT("vle64/vse64 scales consecutive element addresses by eight bytes and exposes the exact 64-bit little-endian element image",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_E64_VL &&
        elem0 == 0x5678456712343412ULL &&
        elem1 == 0x0304050607081234ULL &&
        ai_arch_vec_bytes_equal(
            &ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET],
            ai_arch_vec_unit_src, AI_ARCH_VEC_UNIT_BYTES, &bad_byte,
            &bad_actual_byte, &bad_expected_byte) &&
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET - 1U] ==
            AI_ARCH_VEC_GUARD_LO &&
        ai_arch_vec_unit_dst[AI_ARCH_VEC_UNIT_DST_OFFSET +
            AI_ARCH_VEC_UNIT_BYTES] == AI_ARCH_VEC_GUARD_HI,
        "vl=%lu elem0=0x%lx elem1=0x%lx bad_byte=%lu actual=0x%x expected=0x%x",
        CSRR(CSR_VL), elem0, elem1, bad_byte, bad_actual_byte,
        bad_expected_byte
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL; ++i) {
        ai_arch_vec_stride_src[i] = stride_payload[i];
        ai_arch_vec_stride_dump[i] = 0x99999999U;
    }
    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL * AI_ARCH_VEC_STRIDE_GAP + 2U;
        ++i) {
        ai_arch_vec_stride_mem[i] = stride_sentinel;
    }
    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL; ++i) {
        ai_arch_vec_stride_mem[i * AI_ARCH_VEC_STRIDE_GAP] =
            stride_payload[i];
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vec_vlse32_dump(ai_arch_vec_stride_mem, stride_bytes,
        ai_arch_vec_stride_dump);
    TEST_ASSERT("vlse32 with positive stride binds element i to base plus i times byte stride and writes the loaded vector to a contiguous observer",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_stride_dump[0] == stride_payload[0] &&
        ai_arch_vec_stride_dump[1] == stride_payload[1] &&
        ai_arch_vec_stride_dump[2] == stride_payload[2] &&
        ai_arch_vec_stride_dump[3] == stride_payload[3],
        "vl=%lu dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), ai_arch_vec_stride_dump[0],
        ai_arch_vec_stride_dump[1], ai_arch_vec_stride_dump[2],
        ai_arch_vec_stride_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL * AI_ARCH_VEC_STRIDE_GAP + 2U;
        ++i) {
        ai_arch_vec_stride_mem[i] = stride_sentinel;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_vsse32_from_contiguous(ai_arch_vec_stride_src,
        ai_arch_vec_stride_mem, stride_bytes);
    TEST_ASSERT("vsse32 with positive stride writes only the element-addressed words and preserves intervening stride slots",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_stride_slots_match(ai_arch_vec_stride_mem,
            pos_expected, stride_sentinel, AI_ARCH_VEC_STRIDE_GAP,
            AI_ARCH_VEC_STRIDE_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL * AI_ARCH_VEC_STRIDE_GAP + 2U;
        ++i) {
        ai_arch_vec_stride_mem[i] = stride_sentinel;
    }
    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL; ++i) {
        ai_arch_vec_stride_mem[i * AI_ARCH_VEC_STRIDE_GAP] =
            stride_payload[i];
        ai_arch_vec_stride_dump[i] = 0x88888888U;
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vec_vlse32_dump(&ai_arch_vec_stride_mem[
        (AI_ARCH_VEC_STRIDE_VL - 1U) * AI_ARCH_VEC_STRIDE_GAP],
        -stride_bytes, ai_arch_vec_stride_dump);
    TEST_ASSERT("vlse32 with negative stride binds element order to descending addresses without changing the element numbering",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_stride_dump[0] == stride_payload[3] &&
        ai_arch_vec_stride_dump[1] == stride_payload[2] &&
        ai_arch_vec_stride_dump[2] == stride_payload[1] &&
        ai_arch_vec_stride_dump[3] == stride_payload[0],
        "vl=%lu dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), ai_arch_vec_stride_dump[0],
        ai_arch_vec_stride_dump[1], ai_arch_vec_stride_dump[2],
        ai_arch_vec_stride_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL * AI_ARCH_VEC_STRIDE_GAP + 2U;
        ++i) {
        ai_arch_vec_stride_mem[i] = stride_sentinel;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_vsse32_from_contiguous(ai_arch_vec_stride_src,
        &ai_arch_vec_stride_mem[(AI_ARCH_VEC_STRIDE_VL - 1U) *
            AI_ARCH_VEC_STRIDE_GAP], -stride_bytes);
    TEST_ASSERT("vsse32 with negative stride stores element0 at the high base address and later elements at descending stride slots",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_stride_slots_match(ai_arch_vec_stride_mem,
            neg_expected, stride_sentinel, AI_ARCH_VEC_STRIDE_GAP,
            AI_ARCH_VEC_STRIDE_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL; ++i) {
        ai_arch_vec_stride_dump[i] = 0x77777777U;
    }
    ai_arch_vec_stride_mem[1] = 0x4badc0deU;
    TEST_SETUP_EXCEPT();
    ai_arch_vec_vlse32_zero_stride_dump(&ai_arch_vec_stride_mem[1],
        ai_arch_vec_stride_dump);
    TEST_ASSERT("vlse32 with a non-x0 zero stride returns the repeated-address word in every active destination element",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_stride_dump[0] == 0x4badc0deU &&
        ai_arch_vec_stride_dump[1] == 0x4badc0deU &&
        ai_arch_vec_stride_dump[2] == 0x4badc0deU &&
        ai_arch_vec_stride_dump[3] == 0x4badc0deU,
        "vl=%lu dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), ai_arch_vec_stride_dump[0],
        ai_arch_vec_stride_dump[1], ai_arch_vec_stride_dump[2],
        ai_arch_vec_stride_dump[3]
    );

    ai_arch_vec_stride_src[0] = 0xd0d0d001U;
    ai_arch_vec_stride_src[1] = 0xd0d0d002U;
    ai_arch_vec_stride_src[2] = 0xd0d0d003U;
    ai_arch_vec_stride_src[3] = ordered_final;
    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL; ++i) {
        ai_arch_vec_index_offsets[i] = 0U;
    }
    ai_arch_vec_index_target[0] = 0x11223344U;
    ai_arch_vec_index_target[1] = 0xa0a0a0a0U;
    ai_arch_vec_index_target[2] = 0x55667788U;

    TEST_SETUP_EXCEPT();
    ai_arch_vec_vsoxei32_repeated_ordered_store(ai_arch_vec_stride_src,
        ai_arch_vec_index_offsets, &ai_arch_vec_index_target[1]);
    TEST_ASSERT("vsoxei32 ordered indexed repeated-offset stores commit element writes in element order so the final memory word is the last active element",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_index_target[0] == 0x11223344U &&
        ai_arch_vec_index_target[1] == ordered_final &&
        ai_arch_vec_index_target[2] == 0x55667788U,
        "vl=%lu guard_lo=0x%x target=0x%x guard_hi=0x%x",
        CSRR(CSR_VL), ai_arch_vec_index_target[0],
        ai_arch_vec_index_target[1], ai_arch_vec_index_target[2]
    );

    for (size_t i = 0; i < AI_ARCH_VEC_INDEX_MEM_BYTES; ++i) {
        ai_arch_vec_index_mem[i] = 0x5cU;
    }
    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL; ++i) {
        ai_arch_vec_index_offsets8[i] = (uint8_t)index_offsets[i];
        ai_arch_vec_index_offsets16[i] = (uint16_t)index_offsets[i];
        ai_arch_vec_index_offsets32[i] = index_offsets[i];
        ai_arch_vec_index_dump[i] = 0xababababU;
        ai_arch_vec_index_write32(AI_ARCH_VEC_INDEX_BASE_OFFSET +
            index_offsets[i], index_expected[i]);
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vec_vluxei8_dump(
        &ai_arch_vec_index_mem[AI_ARCH_VEC_INDEX_BASE_OFFSET],
        ai_arch_vec_index_offsets8, ai_arch_vec_index_dump);
    TEST_ASSERT("vluxei8 uses zero-extended 8-bit byte offsets added to the base while loading e32 data elements into element order",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_index_dump, index_expected,
            AI_ARCH_VEC_STRIDE_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word,
        ai_arch_vec_index_dump[0], ai_arch_vec_index_dump[1],
        ai_arch_vec_index_dump[2], ai_arch_vec_index_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL; ++i) {
        ai_arch_vec_index_dump[i] = 0xcdcdcdcdU;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_vluxei16_dump(
        &ai_arch_vec_index_mem[AI_ARCH_VEC_INDEX_BASE_OFFSET],
        ai_arch_vec_index_offsets16, ai_arch_vec_index_dump);
    TEST_ASSERT("vluxei16 uses zero-extended 16-bit byte offsets added to the base while preserving the same indexed e32 load image",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_index_dump, index_expected,
            AI_ARCH_VEC_STRIDE_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word,
        ai_arch_vec_index_dump[0], ai_arch_vec_index_dump[1],
        ai_arch_vec_index_dump[2], ai_arch_vec_index_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL; ++i) {
        ai_arch_vec_index_dump[i] = 0xefefefefU;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_vluxei32_dump(
        &ai_arch_vec_index_mem[AI_ARCH_VEC_INDEX_BASE_OFFSET],
        ai_arch_vec_index_offsets32, ai_arch_vec_index_dump);
    TEST_ASSERT("vluxei32 uses 32-bit byte offsets added to the base and returns the same indexed e32 element sequence",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_index_dump, index_expected,
            AI_ARCH_VEC_STRIDE_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word,
        ai_arch_vec_index_dump[0], ai_arch_vec_index_dump[1],
        ai_arch_vec_index_dump[2], ai_arch_vec_index_dump[3]
    );

    ai_arch_vec_stride_src[0] = 0xc0c0c001U;
    ai_arch_vec_stride_src[1] = 0xc0c0c002U;
    ai_arch_vec_stride_src[2] = 0xc0c0c003U;
    ai_arch_vec_stride_src[3] = 0xc0c0c004U;
    for (unsigned i = 0; i < AI_ARCH_VEC_STRIDE_VL; ++i) {
        ai_arch_vec_index_offsets32[i] = 0U;
    }
    ai_arch_vec_index_write32(repeated_offset - sizeof(uint32_t),
        0x01234567U);
    ai_arch_vec_index_write32(repeated_offset, 0x89abcdefU);
    ai_arch_vec_index_write32(repeated_offset + sizeof(uint32_t),
        0x76543210U);

    TEST_SETUP_EXCEPT();
    ai_arch_vec_vsuxei32_repeated_unordered_store(ai_arch_vec_stride_src,
        ai_arch_vec_index_offsets32, &ai_arch_vec_index_mem[repeated_offset]);
    TEST_ASSERT("vsuxei32 unordered repeated-offset stores leave the aliased word equal to one active element value and do not touch neighboring guards",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_STRIDE_VL &&
        ai_arch_vec_index_read32(repeated_offset - sizeof(uint32_t)) ==
            0x01234567U &&
        ai_arch_vec_word_is_one_of(ai_arch_vec_index_read32(repeated_offset),
            ai_arch_vec_stride_src, AI_ARCH_VEC_STRIDE_VL) &&
        ai_arch_vec_index_read32(repeated_offset + sizeof(uint32_t)) ==
            0x76543210U,
        "vl=%lu guard_lo=0x%x target=0x%x guard_hi=0x%x src=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL),
        ai_arch_vec_index_read32(repeated_offset - sizeof(uint32_t)),
        ai_arch_vec_index_read32(repeated_offset),
        ai_arch_vec_index_read32(repeated_offset + sizeof(uint32_t)),
        ai_arch_vec_stride_src[0], ai_arch_vec_stride_src[1],
        ai_arch_vec_stride_src[2], ai_arch_vec_stride_src[3]
    );

    TEST_END();
}

bool ai_arch_vector_segment_whole_register_visible_layout_corner()
{
    TEST_START();

    size_t bad_byte = 0;
    uint8_t bad_actual_byte = 0;
    uint8_t bad_expected_byte = 0;
    int bad_slot = -1;
    uint32_t bad_actual_word = 0;
    uint32_t bad_expected_word = 0;
    uint64_t vlenb = 0;
    uint64_t vl_whole = 0;
    bool field_dump_ok = true;
    bool segment_store_ok = true;
    bool whole1_ok = true;
    bool whole2_ok = true;
    const uint32_t segment_pattern[AI_ARCH_VEC_SEG_WORDS] = {
        0xa0000000U, 0xb0000000U, 0xc0000000U,
        0xa1111111U, 0xb1111111U, 0xc1111111U,
        0xa2222222U, 0xb2222222U, 0xc2222222U,
        0xa3333333U, 0xb3333333U, 0xc3333333U
    };

    ai_arch_vec_enable_state();
    vlenb = ai_arch_vec_read_vlenb();

    TEST_ASSERT("vector whole-register layout probe uses a VLENB that fits the static observer buffers",
        vlenb > 0U &&
        (2U * vlenb) <= AI_ARCH_VEC_WHOLE_MAX_BYTES,
        "vlenb=%lu max_bytes=%u", vlenb, AI_ARCH_VEC_WHOLE_MAX_BYTES
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_SEG_WORDS; ++i) {
        ai_arch_vec_segment_src[i] = segment_pattern[i];
        ai_arch_vec_segment_dump[i] = 0xededededU;
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vec_vlseg3e32_dump(ai_arch_vec_segment_src,
        ai_arch_vec_segment_dump);
    TEST_ASSERT("vlseg3e32 plus vsseg3e32 preserves the element-major memory layout for nf=3 and vl=4",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_SEG_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_segment_dump, segment_pattern,
            AI_ARCH_VEC_SEG_WORDS, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_SEG_WORDS; ++i) {
        ai_arch_vec_segment_dump[i] = 0xdedededeU;
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vec_vlseg3e32_field_dump(ai_arch_vec_segment_src,
        &ai_arch_vec_segment_dump[0],
        &ai_arch_vec_segment_dump[AI_ARCH_VEC_SEG_VL],
        &ai_arch_vec_segment_dump[2U * AI_ARCH_VEC_SEG_VL]);
    for (unsigned i = 0; i < AI_ARCH_VEC_SEG_VL; ++i) {
        const uint32_t expected0 = segment_pattern[i * AI_ARCH_VEC_SEG_NF];
        const uint32_t expected1 =
            segment_pattern[i * AI_ARCH_VEC_SEG_NF + 1U];
        const uint32_t expected2 =
            segment_pattern[i * AI_ARCH_VEC_SEG_NF + 2U];

        if (ai_arch_vec_segment_dump[i] != expected0 ||
            ai_arch_vec_segment_dump[AI_ARCH_VEC_SEG_VL + i] != expected1 ||
            ai_arch_vec_segment_dump[2U * AI_ARCH_VEC_SEG_VL + i] !=
                expected2) {
            field_dump_ok = false;
        }
    }
    TEST_ASSERT("vlseg3e32 maps fields to consecutive destination registers vd, vd+1, and vd+2 in element order",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_SEG_VL &&
        field_dump_ok,
        "vl=%lu dump=%08x,%08x,%08x,%08x / %08x,%08x,%08x,%08x / %08x,%08x,%08x,%08x",
        CSRR(CSR_VL),
        ai_arch_vec_segment_dump[0], ai_arch_vec_segment_dump[1],
        ai_arch_vec_segment_dump[2], ai_arch_vec_segment_dump[3],
        ai_arch_vec_segment_dump[4], ai_arch_vec_segment_dump[5],
        ai_arch_vec_segment_dump[6], ai_arch_vec_segment_dump[7],
        ai_arch_vec_segment_dump[8], ai_arch_vec_segment_dump[9],
        ai_arch_vec_segment_dump[10], ai_arch_vec_segment_dump[11]
    );

    for (size_t i = 0; i < sizeof(ai_arch_vec_segment_dst); ++i) {
        ai_arch_vec_segment_dst[i] = 0x7eU;
    }
    ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET - 1U] =
        AI_ARCH_VEC_GUARD_LO;
    ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET +
        AI_ARCH_VEC_SEG_WORDS * sizeof(uint32_t)] = AI_ARCH_VEC_GUARD_HI;

    TEST_SETUP_EXCEPT();
    ai_arch_vec_vlseg3e32_dump(ai_arch_vec_segment_src,
        (uint32_t *)(void *)&ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET]);
    segment_store_ok = ai_arch_vec_bytes_equal(
        &ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET],
        (const uint8_t *)(const void *)segment_pattern,
        AI_ARCH_VEC_SEG_WORDS * sizeof(uint32_t), &bad_byte,
        &bad_actual_byte, &bad_expected_byte);
    TEST_ASSERT("vsseg3e32 writes exactly the nf=3 element-major byte image and preserves adjacent guard bytes",
        excpt.triggered == false &&
        segment_store_ok &&
        ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET - 1U] ==
            AI_ARCH_VEC_GUARD_LO &&
        ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET +
            AI_ARCH_VEC_SEG_WORDS * sizeof(uint32_t)] == AI_ARCH_VEC_GUARD_HI,
        "bad_byte=%lu actual=0x%x expected=0x%x guard_lo=0x%x guard_hi=0x%x",
        bad_byte, bad_actual_byte, bad_expected_byte,
        ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET - 1U],
        ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET +
            AI_ARCH_VEC_SEG_WORDS * sizeof(uint32_t)]
    );

    for (unsigned nf = 1U; nf <= 8U; ++nf) {
        const size_t active_bytes = nf * AI_ARCH_VEC_SEG_NF_SWEEP_VL;

        for (size_t i = 0; i < sizeof(ai_arch_vec_segment_dst); ++i) {
            ai_arch_vec_segment_dst[i] = 0x6dU;
        }
        for (size_t i = 0; i < active_bytes; ++i) {
            ai_arch_vec_segment_src[i] =
                (uint32_t)(0x00000010U + (nf << 4U) + i);
        }
        ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET - 1U] =
            AI_ARCH_VEC_GUARD_LO;
        ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET + active_bytes] =
            AI_ARCH_VEC_GUARD_HI;

        TEST_SETUP_EXCEPT();
        ai_arch_vec_vlseg_e8_roundtrip(
            (const uint8_t *)(const void *)ai_arch_vec_segment_src,
            &ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET], nf);
        segment_store_ok = ai_arch_vec_bytes_equal(
            &ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET],
            (const uint8_t *)(const void *)ai_arch_vec_segment_src,
            active_bytes, &bad_byte, &bad_actual_byte, &bad_expected_byte);
        TEST_ASSERT("vlsegNf/vssegNf e8 sweep preserves element-major byte layout for nf=1..8 and keeps neighboring guards",
            excpt.triggered == false &&
            CSRR(CSR_VL) == AI_ARCH_VEC_SEG_NF_SWEEP_VL &&
            segment_store_ok &&
            ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET - 1U] ==
                AI_ARCH_VEC_GUARD_LO &&
            ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET +
                active_bytes] == AI_ARCH_VEC_GUARD_HI,
            "nf=%u vl=%lu active_bytes=%lu bad_byte=%lu actual=0x%x expected=0x%x guard_lo=0x%x guard_hi=0x%x",
            nf, CSRR(CSR_VL), active_bytes, bad_byte, bad_actual_byte,
            bad_expected_byte,
            ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET - 1U],
            ai_arch_vec_segment_dst[AI_ARCH_VEC_SEG_DST_OFFSET +
                active_bytes]
        );
    }

    for (size_t i = 0; i < AI_ARCH_VEC_WHOLE_MAX_BYTES; ++i) {
        ai_arch_vec_whole_src[i] = (uint8_t)(0x30U + ((i * 17U) & 0x3fU));
    }
    for (size_t i = 0; i < sizeof(ai_arch_vec_whole_dst); ++i) {
        ai_arch_vec_whole_dst[i] = 0x91U;
    }
    ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET - 1U] =
        AI_ARCH_VEC_GUARD_LO;
    ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET + vlenb] =
        AI_ARCH_VEC_GUARD_HI;

    TEST_SETUP_EXCEPT();
    ai_arch_vec_whole_1r_copy(ai_arch_vec_whole_src,
        &ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET], &vl_whole);
    whole1_ok = ai_arch_vec_bytes_equal(
        &ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET],
        ai_arch_vec_whole_src, (size_t)vlenb, &bad_byte, &bad_actual_byte,
        &bad_expected_byte);
    TEST_ASSERT("vl1re8/vs1r copies exactly one whole vector register worth of bytes independent of the current element interpretation",
        excpt.triggered == false &&
        vl_whole == 1U &&
        whole1_ok &&
        ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET - 1U] ==
            AI_ARCH_VEC_GUARD_LO &&
        ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET + vlenb] ==
            AI_ARCH_VEC_GUARD_HI,
        "vlenb=%lu vl_before_store=%lu bad_byte=%lu actual=0x%x expected=0x%x guard_lo=0x%x guard_hi=0x%x",
        vlenb, vl_whole, bad_byte, bad_actual_byte, bad_expected_byte,
        ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET - 1U],
        ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET + vlenb]
    );

    for (size_t i = 0; i < sizeof(ai_arch_vec_whole_dst); ++i) {
        ai_arch_vec_whole_dst[i] = 0x82U;
    }
    ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET - 1U] =
        AI_ARCH_VEC_GUARD_LO;
    ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET + 2U * vlenb] =
        AI_ARCH_VEC_GUARD_HI;

    TEST_SETUP_EXCEPT();
    ai_arch_vec_whole_2r_copy(ai_arch_vec_whole_src,
        &ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET], &vl_whole);
    whole2_ok = ai_arch_vec_bytes_equal(
        &ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET],
        ai_arch_vec_whole_src, (size_t)(2U * vlenb), &bad_byte,
        &bad_actual_byte, &bad_expected_byte);
    TEST_ASSERT("vl2re8/vs2r copies a consecutive two-register byte image and preserves the surrounding guards",
        excpt.triggered == false &&
        vl_whole == 2U &&
        whole2_ok &&
        ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET - 1U] ==
            AI_ARCH_VEC_GUARD_LO &&
        ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET + 2U * vlenb] ==
            AI_ARCH_VEC_GUARD_HI,
        "vlenb=%lu vl_before_store=%lu bad_byte=%lu actual=0x%x expected=0x%x guard_lo=0x%x guard_hi=0x%x",
        vlenb, vl_whole, bad_byte, bad_actual_byte, bad_expected_byte,
        ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET - 1U],
        ai_arch_vec_whole_dst[AI_ARCH_VEC_WHOLE_DST_OFFSET + 2U * vlenb]
    );

    TEST_END();
}

bool ai_arch_vector_mask_tail_vl_visible_matrix_corner()
{
    TEST_START();

    int bad_slot = -1;
    uint32_t bad_actual_word = 0;
    uint32_t bad_expected_word = 0;
    uint64_t zero_vl = 0xffffffffffffffffULL;
    uint64_t final_vstart = 0xffffffffffffffffULL;
    const uint32_t ctrl_old[AI_ARCH_VEC_CTRL_VL] = {
        0x10101010U, 0x20202020U, 0x30303030U, 0x40404040U
    };
    const uint32_t ctrl_new[AI_ARCH_VEC_CTRL_VL] = {
        0xa1a1a1a1U, 0xb2b2b2b2U, 0xc3c3c3c3U, 0xd4d4d4d4U
    };
    const uint32_t tail_tu_expected[AI_ARCH_VEC_CTRL_VL] = {
        0xa1a1a1a1U, 0xb2b2b2b2U, 0x30303030U, 0x40404040U
    };
    const uint32_t mask_mu_expected[AI_ARCH_VEC_CTRL_VL] = {
        0xa1a1a1a1U, 0x20202020U, 0xc3c3c3c3U, 0x40404040U
    };
    const uint32_t vstart2_expected[AI_ARCH_VEC_CTRL_VL] = {
        0x10101010U, 0x20202020U, 0xc3c3c3c3U, 0xd4d4d4d4U
    };
    const uint32_t tail_allowed2[2] = { 0x30303030U, 0xffffffffU };
    const uint32_t tail_allowed3[2] = { 0x40404040U, 0xffffffffU };
    const uint32_t mask_allowed1[2] = { 0x20202020U, 0xffffffffU };
    const uint32_t mask_allowed3[2] = { 0x40404040U, 0xffffffffU };
    uint8_t *const indexed_base =
        &ai_arch_vec_index_mem[AI_ARCH_VEC_INDEX_BASE_OFFSET];
    const uint64_t invalid_offset =
        (uint64_t)(1ULL - (uint64_t)(uintptr_t)indexed_base);

    ai_arch_vec_enable_state();

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_old[i] = ctrl_old[i];
        ai_arch_vec_ctrl_src[i] = ctrl_new[i];
        ai_arch_vec_ctrl_dump[i] = 0xeeeeeeeeU;
        ai_arch_vec_ctrl_target[i] = ctrl_old[i];
    }

    TEST_SETUP_EXCEPT();
    ai_arch_vec_tail_load_dump(ai_arch_vec_ctrl_old, ai_arch_vec_ctrl_src,
        ai_arch_vec_ctrl_dump, 0);
    TEST_ASSERT("tail-undisturbed vle32 with vl=2 updates only body elements and preserves old tail elements when later observed with vl=4",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_CTRL_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_dump, tail_tu_expected,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word,
        ai_arch_vec_ctrl_dump[0], ai_arch_vec_ctrl_dump[1],
        ai_arch_vec_ctrl_dump[2], ai_arch_vec_ctrl_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_dump[i] = 0xddddddddU;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_tail_load_dump(ai_arch_vec_ctrl_old, ai_arch_vec_ctrl_src,
        ai_arch_vec_ctrl_dump, 1);
    TEST_ASSERT("tail-agnostic vle32 with vl=2 updates body elements and leaves tail elements as either old values or all-ones",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_CTRL_VL &&
        ai_arch_vec_ctrl_dump[0] == ctrl_new[0] &&
        ai_arch_vec_ctrl_dump[1] == ctrl_new[1] &&
        ai_arch_vec_word_is_one_of(ai_arch_vec_ctrl_dump[2],
            tail_allowed2, 2U) &&
        ai_arch_vec_word_is_one_of(ai_arch_vec_ctrl_dump[3],
            tail_allowed3, 2U),
        "vl=%lu dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), ai_arch_vec_ctrl_dump[0],
        ai_arch_vec_ctrl_dump[1], ai_arch_vec_ctrl_dump[2],
        ai_arch_vec_ctrl_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_dump[i] = 0xccccccccU;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_masked_load_dump(ai_arch_vec_ctrl_old, ai_arch_vec_ctrl_src,
        ai_arch_vec_ctrl_dump, 0);
    TEST_ASSERT("mask-undisturbed vle32 updates only active even elements and preserves masked-off destination elements",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_CTRL_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_dump, mask_mu_expected,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word,
        ai_arch_vec_ctrl_dump[0], ai_arch_vec_ctrl_dump[1],
        ai_arch_vec_ctrl_dump[2], ai_arch_vec_ctrl_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_dump[i] = 0xbbbbbbbbU;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_masked_load_dump(ai_arch_vec_ctrl_old, ai_arch_vec_ctrl_src,
        ai_arch_vec_ctrl_dump, 1);
    TEST_ASSERT("mask-agnostic vle32 updates active even elements and leaves inactive elements as either old values or all-ones",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_CTRL_VL &&
        ai_arch_vec_ctrl_dump[0] == ctrl_new[0] &&
        ai_arch_vec_ctrl_dump[2] == ctrl_new[2] &&
        ai_arch_vec_word_is_one_of(ai_arch_vec_ctrl_dump[1],
            mask_allowed1, 2U) &&
        ai_arch_vec_word_is_one_of(ai_arch_vec_ctrl_dump[3],
            mask_allowed3, 2U),
        "vl=%lu dump=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), ai_arch_vec_ctrl_dump[0],
        ai_arch_vec_ctrl_dump[1], ai_arch_vec_ctrl_dump[2],
        ai_arch_vec_ctrl_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_target[i] = ctrl_old[i];
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_masked_store_even(ai_arch_vec_ctrl_src,
        ai_arch_vec_ctrl_target);
    TEST_ASSERT("masked vse32 writes only active even memory elements and leaves masked-off memory words unchanged",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_CTRL_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_target, mask_mu_expected,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x target=%08x,%08x,%08x,%08x",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word,
        ai_arch_vec_ctrl_target[0], ai_arch_vec_ctrl_target[1],
        ai_arch_vec_ctrl_target[2], ai_arch_vec_ctrl_target[3]
    );

    for (size_t i = 0; i < AI_ARCH_VEC_INDEX_MEM_BYTES; ++i) {
        ai_arch_vec_index_mem[i] = 0x6dU;
    }
    ai_arch_vec_index_offsets64[0] = 0U;
    ai_arch_vec_index_offsets64[1] = invalid_offset;
    ai_arch_vec_index_offsets64[2] = 8U;
    ai_arch_vec_index_offsets64[3] = invalid_offset;
    ai_arch_vec_index_write32(AI_ARCH_VEC_INDEX_BASE_OFFSET, ctrl_new[0]);
    ai_arch_vec_index_write32(AI_ARCH_VEC_INDEX_BASE_OFFSET + 8U,
        ctrl_new[2]);
    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_dump[i] = 0x88888888U;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_masked_vluxei64_even_dump(indexed_base,
        ai_arch_vec_index_offsets64, ai_arch_vec_ctrl_old,
        ai_arch_vec_ctrl_dump);
    TEST_ASSERT("masked indexed load suppresses inactive odd-lane invalid addresses while active even lanes load normally",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_CTRL_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_dump, mask_mu_expected,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x dump=%08x,%08x,%08x,%08x invalid_offset=0x%lx",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word,
        ai_arch_vec_ctrl_dump[0], ai_arch_vec_ctrl_dump[1],
        ai_arch_vec_ctrl_dump[2], ai_arch_vec_ctrl_dump[3],
        invalid_offset
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_dump[i] = 0x87878787U;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_masked_vloxei64_even_dump(indexed_base,
        ai_arch_vec_index_offsets64, ai_arch_vec_ctrl_old,
        ai_arch_vec_ctrl_dump);
    TEST_ASSERT("masked ordered indexed load skips inactive odd-lane invalid holes and still lets the later active lane2 retire",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_CTRL_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_dump, mask_mu_expected,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x dump=%08x,%08x,%08x,%08x invalid_offset=0x%lx",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word,
        ai_arch_vec_ctrl_dump[0], ai_arch_vec_ctrl_dump[1],
        ai_arch_vec_ctrl_dump[2], ai_arch_vec_ctrl_dump[3],
        invalid_offset
    );

    for (size_t i = 0; i < AI_ARCH_VEC_INDEX_MEM_BYTES; ++i) {
        ai_arch_vec_index_mem[i] = 0x7eU;
    }
    ai_arch_vec_index_write32(AI_ARCH_VEC_INDEX_BASE_OFFSET, ctrl_old[0]);
    ai_arch_vec_index_write32(AI_ARCH_VEC_INDEX_BASE_OFFSET + 4U,
        ctrl_old[1]);
    ai_arch_vec_index_write32(AI_ARCH_VEC_INDEX_BASE_OFFSET + 8U,
        ctrl_old[2]);
    ai_arch_vec_index_write32(AI_ARCH_VEC_INDEX_BASE_OFFSET + 12U,
        ctrl_old[3]);
    TEST_SETUP_EXCEPT();
    ai_arch_vec_masked_vsuxei64_even_store(ai_arch_vec_ctrl_src,
        ai_arch_vec_index_offsets64, indexed_base);
    ai_arch_vec_ctrl_target[0] =
        ai_arch_vec_index_read32(AI_ARCH_VEC_INDEX_BASE_OFFSET);
    ai_arch_vec_ctrl_target[1] =
        ai_arch_vec_index_read32(AI_ARCH_VEC_INDEX_BASE_OFFSET + 4U);
    ai_arch_vec_ctrl_target[2] =
        ai_arch_vec_index_read32(AI_ARCH_VEC_INDEX_BASE_OFFSET + 8U);
    ai_arch_vec_ctrl_target[3] =
        ai_arch_vec_index_read32(AI_ARCH_VEC_INDEX_BASE_OFFSET + 12U);
    TEST_ASSERT("masked indexed store suppresses inactive odd-lane invalid addresses and writes only active even memory words",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_ARCH_VEC_CTRL_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_target, mask_mu_expected,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "vl=%lu bad_slot=%d actual=0x%x expected=0x%x target=%08x,%08x,%08x,%08x invalid_offset=0x%lx",
        CSRR(CSR_VL), bad_slot, bad_actual_word, bad_expected_word,
        ai_arch_vec_ctrl_target[0], ai_arch_vec_ctrl_target[1],
        ai_arch_vec_ctrl_target[2], ai_arch_vec_ctrl_target[3],
        invalid_offset
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_dump[i] = 0xaaaaaaaaU;
    }
    TEST_SETUP_EXCEPT();
    ai_arch_vec_vl0_load_keeps_old(ai_arch_vec_ctrl_old,
        ai_arch_vec_ctrl_src, ai_arch_vec_ctrl_dump, &zero_vl);
    TEST_ASSERT("vle32 with vl=0 performs no element update so the previous vector register image is preserved",
        excpt.triggered == false &&
        zero_vl == 0ULL &&
        CSRR(CSR_VL) == AI_ARCH_VEC_CTRL_VL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_dump, ctrl_old,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "zero_vl=%lu final_vl=%lu bad_slot=%d actual=0x%x expected=0x%x dump=%08x,%08x,%08x,%08x",
        zero_vl, CSRR(CSR_VL), bad_slot, bad_actual_word,
        bad_expected_word, ai_arch_vec_ctrl_dump[0],
        ai_arch_vec_ctrl_dump[1], ai_arch_vec_ctrl_dump[2],
        ai_arch_vec_ctrl_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_target[i] = ctrl_old[i];
    }
    zero_vl = 0xffffffffffffffffULL;
    TEST_SETUP_EXCEPT();
    ai_arch_vec_vl0_store_keeps_memory(ai_arch_vec_ctrl_src,
        ai_arch_vec_ctrl_target, &zero_vl);
    TEST_ASSERT("vse32 with vl=0 performs no memory write and leaves all target words unchanged",
        excpt.triggered == false &&
        zero_vl == 0ULL &&
        CSRR(CSR_VL) == 0ULL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_target, ctrl_old,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "zero_vl=%lu csr_vl=%lu bad_slot=%d actual=0x%x expected=0x%x target=%08x,%08x,%08x,%08x",
        zero_vl, CSRR(CSR_VL), bad_slot, bad_actual_word,
        bad_expected_word, ai_arch_vec_ctrl_target[0],
        ai_arch_vec_ctrl_target[1], ai_arch_vec_ctrl_target[2],
        ai_arch_vec_ctrl_target[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_dump[i] = 0x99999999U;
    }
    final_vstart = 0xffffffffffffffffULL;
    TEST_SETUP_EXCEPT();
    ai_arch_vec_vstart2_load_dump(ai_arch_vec_ctrl_old,
        ai_arch_vec_ctrl_src, ai_arch_vec_ctrl_dump, &final_vstart);
    TEST_ASSERT("vle32 resumed with vstart=2 preserves prestart elements, updates the suffix, and clears vstart after success",
        excpt.triggered == false &&
        final_vstart == 0ULL &&
        CSRR(CSR_VSTART) == 0ULL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_dump, vstart2_expected,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "final_vstart=%lu csr_vstart=%lu bad_slot=%d actual=0x%x expected=0x%x dump=%08x,%08x,%08x,%08x",
        final_vstart, CSRR(CSR_VSTART), bad_slot, bad_actual_word,
        bad_expected_word, ai_arch_vec_ctrl_dump[0],
        ai_arch_vec_ctrl_dump[1], ai_arch_vec_ctrl_dump[2],
        ai_arch_vec_ctrl_dump[3]
    );

    for (unsigned i = 0; i < AI_ARCH_VEC_CTRL_VL; ++i) {
        ai_arch_vec_ctrl_target[i] = ctrl_old[i];
    }
    final_vstart = 0xffffffffffffffffULL;
    TEST_SETUP_EXCEPT();
    ai_arch_vec_vstart2_store_suffix(ai_arch_vec_ctrl_src,
        ai_arch_vec_ctrl_target, &final_vstart);
    TEST_ASSERT("vse32 resumed with vstart=2 skips prestart memory words, stores only the suffix, and clears vstart after success",
        excpt.triggered == false &&
        final_vstart == 0ULL &&
        CSRR(CSR_VSTART) == 0ULL &&
        ai_arch_vec_words_equal(ai_arch_vec_ctrl_target, vstart2_expected,
            AI_ARCH_VEC_CTRL_VL, &bad_slot, &bad_actual_word,
            &bad_expected_word),
        "final_vstart=%lu csr_vstart=%lu bad_slot=%d actual=0x%x expected=0x%x target=%08x,%08x,%08x,%08x",
        final_vstart, CSRR(CSR_VSTART), bad_slot, bad_actual_word,
        bad_expected_word, ai_arch_vec_ctrl_target[0],
        ai_arch_vec_ctrl_target[1], ai_arch_vec_ctrl_target[2],
        ai_arch_vec_ctrl_target[3]
    );

    TEST_END();
}
