#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_VSEG_STORE_PRESSURE_PRE_STORES 64U
#define AI_VSEG_STORE_PRESSURE_POST_STORES 8U
#define AI_VSEG_STORE_PRESSURE_LINE_STRIDE 64U
#define AI_VSEG_STORE_PRESSURE_SEG_VL 4U
#define AI_VSEG_STORE_PRESSURE_SEG_WORDS \
    (AI_VSEG_STORE_PRESSURE_SEG_VL * 2U)
#define AI_VSEG_STORE_PRESSURE_PROBE_VL 16U
#define AI_VSEG_STORE_PRESSURE_PROBE_WORDS \
    (AI_VSEG_STORE_PRESSURE_PROBE_VL * 2U)
#define AI_VSEG_STORE_PRESSURE_INDEXED_VL 9U
#define AI_VSEG_STORE_PRESSURE_INDEXED_PREFIX 8U
#define AI_VSEG_STORE_PRESSURE_INDEXED_WORDS \
    (AI_VSEG_STORE_PRESSURE_INDEXED_VL * 2U)
#define AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_VL 9U
#define AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FAULT_IDX \
    (AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_VL - 1U)
#define AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS 2U
#define AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_WORDS \
    (AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_VL * \
     AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS)
#define AI_VSEG_STORE_PRESSURE_PENDING_VL 10U
#define AI_VSEG_STORE_PRESSURE_PENDING_PREFIX 9U
#define AI_VSEG_STORE_PRESSURE_PENDING_WORDS \
    (AI_VSEG_STORE_PRESSURE_PENDING_VL * 2U)
#define AI_VSEG_STORE_PRESSURE_INDEXED_LATE_VL 16U
#define AI_VSEG_STORE_PRESSURE_INDEXED_LATE_FAULT_IDX \
    (AI_VSEG_STORE_PRESSURE_INDEXED_LATE_VL - 1U)
#define AI_VSEG_STORE_PRESSURE_INDEXED_LATE_WORDS \
    (AI_VSEG_STORE_PRESSURE_INDEXED_LATE_VL * 2U)
#define AI_VSEG_STORE_PRESSURE_LONG_E8_VL 64U
#define AI_VSEG_STORE_PRESSURE_LONG_E8_BYTES \
    (AI_VSEG_STORE_PRESSURE_LONG_E8_VL * 2U)
#define AI_VSEG_STORE_PRESSURE_INDEXED_E8_VL 32U
#define AI_VSEG_STORE_PRESSURE_INDEXED_E8_FAULT_IDX \
    (AI_VSEG_STORE_PRESSURE_INDEXED_E8_VL - 1U)
#define AI_VSEG_STORE_PRESSURE_INDEXED_E8_SEG_FIELDS 4U
#define AI_VSEG_STORE_PRESSURE_INDEXED_E8_BYTES \
    (AI_VSEG_STORE_PRESSURE_INDEXED_E8_VL * \
     AI_VSEG_STORE_PRESSURE_INDEXED_E8_SEG_FIELDS)
#define AI_VSEG_STORE_PRESSURE_INDEXED_E8_SAME_SET_STRIDE 0x2000U
#define AI_VSEG_STORE_PRESSURE_FAST_SD_COUNT 64U
#define AI_VSEG_STORE_PRESSURE_FAST_SD_VALUE_BASE 0x5d00cafeULL
#define AI_VSEG_STORE_PRESSURE_FAST_SD_VALUE_STEP 0x14ULL
#define AI_VSEG_STORE_PRESSURE_LATE_FAULT_IDX 15U
#define AI_VSEG_STORE_PRESSURE_LONG_E8_FAULT_IDX 46U
#define AI_VSEG_STORE_PRESSURE_LONG_E8_FINAL_FAULT_IDX \
    (AI_VSEG_STORE_PRESSURE_LONG_E8_VL - 1U)
#define AI_VSEG_STORE_PRESSURE_TAIL_SD_INDEX \
    (AI_VSEG_STORE_PRESSURE_PRE_STORES - 1U)
#define AI_VSEG_STORE_PRESSURE_COMMIT_MISMATCH_STRIDE 0x58U
#define AI_VSEG_STORE_PRESSURE_CROSS16_SLOT_OFFSET 12U

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static volatile uintptr_t ai_vseg_store_pressure_handler_sd_addr = 0;
static volatile uintptr_t ai_vseg_store_pressure_handler_sd_stride = 0;
static volatile uint64_t ai_vseg_store_pressure_handler_sd_value = 0;
static volatile uint64_t ai_vseg_store_pressure_handler_sd_step = 0;
static volatile uint64_t ai_vseg_store_pressure_handler_sd_count = 0;
static volatile uint64_t ai_vseg_store_pressure_handler_sd_fired = 0;
static volatile uint64_t ai_vseg_store_pressure_handler_cause = 0;
static volatile uint64_t ai_vseg_store_pressure_handler_tval = 0;
static volatile uint64_t ai_vseg_store_pressure_handler_epc = 0;
static volatile uint64_t ai_vseg_store_pressure_handler_vstart = 0;
static uint64_t ai_vseg_store_pressure_trap_scratch[8]
    __attribute__((aligned(16), used));

extern void ai_vseg_store_pressure_fault_head_sd_handler(void);
extern void ai_vseg_store_pressure_fault_head_sd_loop_handler(void);
extern void ai_vseg_store_pressure_fault_head_first_sd_loop_handler(void);

asm(
".balign 0x4\n"
".global ai_vseg_store_pressure_fault_head_sd_handler\n"
"ai_vseg_store_pressure_fault_head_sd_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -56\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    sd t5, 40(sp)\n"
"    sd t6, 48(sp)\n"
"    la t0, ai_vseg_store_pressure_handler_sd_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_vseg_store_pressure_handler_sd_value\n"
"    ld t2, 0(t0)\n"
"    sd t2, 0(t1)\n"
"    la t0, ai_vseg_store_pressure_handler_sd_fired\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 1\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_vseg_store_pressure_handler_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_vseg_store_pressure_handler_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, vstart\n"
"    la t0, ai_vseg_store_pressure_handler_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_vseg_store_pressure_handler_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    ld t5, 40(sp)\n"
"    ld t6, 48(sp)\n"
"    addi sp, sp, 56\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_vseg_store_pressure_fault_head_sd_loop_handler\n"
"ai_vseg_store_pressure_fault_head_sd_loop_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -56\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    sd t5, 40(sp)\n"
"    sd t6, 48(sp)\n"
"    la t0, ai_vseg_store_pressure_handler_sd_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_vseg_store_pressure_handler_sd_value\n"
"    ld t2, 0(t0)\n"
"    la t0, ai_vseg_store_pressure_handler_sd_count\n"
"    ld t3, 0(t0)\n"
"    la t0, ai_vseg_store_pressure_handler_sd_stride\n"
"    ld t4, 0(t0)\n"
"    la t0, ai_vseg_store_pressure_handler_sd_step\n"
"    ld t5, 0(t0)\n"
"    beqz t3, 2f\n"
"1:\n"
"    sd t2, 0(t1)\n"
"    add t1, t1, t4\n"
"    add t2, t2, t5\n"
"    addi t3, t3, -1\n"
"    bnez t3, 1b\n"
"    fence rw, rw\n"
"2:\n"
"    la t0, ai_vseg_store_pressure_handler_sd_fired\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 1\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_vseg_store_pressure_handler_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_vseg_store_pressure_handler_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, vstart\n"
"    la t0, ai_vseg_store_pressure_handler_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_vseg_store_pressure_handler_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    ld t5, 40(sp)\n"
"    ld t6, 48(sp)\n"
"    addi sp, sp, 56\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_vseg_store_pressure_fault_head_first_sd_loop_handler\n"
"ai_vseg_store_pressure_fault_head_first_sd_loop_handler:\n"
"    csrr t1, sscratch\n"
"    li t2, 0x5d00cafe\n"
"    sd t2, 0(t1)\n"
"    addi t1, t1, 88\n"
"    addi t2, t2, 20\n"
"    li t3, 63\n"
"1:\n"
"    sd t2, 0(t1)\n"
"    addi t1, t1, 88\n"
"    addi t2, t2, 20\n"
"    addi t3, t3, -1\n"
"    bnez t3, 1b\n"
"    la t0, ai_vseg_store_pressure_handler_sd_fired\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 1\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_vseg_store_pressure_handler_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_vseg_store_pressure_handler_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, vstart\n"
"    la t0, ai_vseg_store_pressure_handler_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_vseg_store_pressure_handler_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    sret\n"
);

static inline pte_t *ai_vseg_store_pressure_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_vseg_store_pressure_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_vseg_store_pressure_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline bool ai_vseg_store_pressure_blocks_match(uintptr_t paddr,
    unsigned count, uint64_t expected)
{
    for (unsigned i = 0; i < count; ++i) {
        if (read64(paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE) !=
            expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_vseg_store_pressure_loop_values_match(uintptr_t paddr,
    unsigned count, uint64_t expected_base, uint64_t expected_step)
{
    uint64_t expected = expected_base;

    for (unsigned i = 0; i < count; ++i) {
        if (read64(paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE) !=
            expected) {
            return false;
        }
        expected += expected_step;
    }

    return true;
}

static inline bool ai_vseg_store_pressure_loop_values_match_stride(
    uintptr_t paddr, unsigned count, unsigned stride, uint64_t expected_base,
    uint64_t expected_step)
{
    uint64_t expected = expected_base;

    for (unsigned i = 0; i < count; ++i) {
        if (read64(paddr + i * stride) != expected) {
            return false;
        }
        expected += expected_step;
    }

    return true;
}

static inline bool ai_vseg_store_pressure_loop_values_match_stride_offset(
    uintptr_t paddr, unsigned count, unsigned stride, unsigned offset,
    uint64_t expected_base, uint64_t expected_step)
{
    uint64_t expected = expected_base;

    for (unsigned i = 0; i < count; ++i) {
        if (read64(paddr + i * stride + offset) != expected) {
            return false;
        }
        expected += expected_step;
    }

    return true;
}

static inline bool ai_vseg_store_pressure_seg_blocks_match(uintptr_t paddr,
    unsigned count, const uint32_t *expected_words)
{
    for (unsigned i = 0; i < count; ++i) {
        const uintptr_t slot =
            paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE;
        for (unsigned j = 0; j < AI_VSEG_STORE_PRESSURE_SEG_WORDS; ++j) {
            if (read32(slot + j * sizeof(uint32_t)) != expected_words[j]) {
                return false;
            }
        }
    }

    return true;
}

static inline bool ai_vseg_store_pressure_seg_blocks_match_stride(
    uintptr_t paddr, unsigned count, unsigned stride,
    const uint32_t *expected_words)
{
    for (unsigned i = 0; i < count; ++i) {
        const uintptr_t slot = paddr + i * stride;
        for (unsigned j = 0; j < AI_VSEG_STORE_PRESSURE_SEG_WORDS; ++j) {
            if (read32(slot + j * sizeof(uint32_t)) != expected_words[j]) {
                return false;
            }
        }
    }

    return true;
}

static inline bool ai_vseg_store_pressure_words_match(uintptr_t paddr,
    unsigned count, const uint32_t *expected_words)
{
    for (unsigned i = 0; i < count; ++i) {
        if (read32(paddr + i * sizeof(uint32_t)) != expected_words[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_vseg_store_pressure_vssseg2_stride_match(
    uintptr_t paddr, unsigned vl, unsigned stride,
    const uint32_t *expected_words)
{
    for (unsigned i = 0; i < vl; ++i) {
        const uintptr_t slot = paddr + i * stride;

        if (read32(slot) != expected_words[i * 2U] ||
            read32(slot + sizeof(uint32_t)) !=
                expected_words[i * 2U + 1U]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_vseg_store_pressure_vsuxseg2ei32_match(
    uintptr_t base_paddr, const uint32_t *offsets, unsigned count,
    const uint32_t *expected_words)
{
    for (unsigned i = 0; i < count; ++i) {
        const uintptr_t slot = base_paddr + offsets[i];

        if (read32(slot) != expected_words[i * 2U] ||
            read32(slot + sizeof(uint32_t)) !=
                expected_words[i * 2U + 1U]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_vseg_store_pressure_vsuxseg2ei32_field_count_match(
    uintptr_t base_paddr, const uint32_t *offsets, unsigned count,
    const uint32_t *expected_words)
{
    for (unsigned i = 0; i < count; ++i) {
        const uintptr_t slot = base_paddr + offsets[i];

        if (read32(slot) !=
                expected_words[i *
                AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS] ||
            read32(slot + sizeof(uint32_t)) !=
                expected_words[i *
                AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS + 1U]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_vseg_store_pressure_vsuxseg4ei32_e8_match(
    uintptr_t base_paddr, const uint32_t *offsets, unsigned count,
    const uint8_t *expected_bytes)
{
    for (unsigned i = 0; i < count; ++i) {
        const uintptr_t slot = base_paddr + offsets[i];

        for (unsigned j = 0; j < AI_VSEG_STORE_PRESSURE_INDEXED_E8_SEG_FIELDS;
             ++j) {
            if (read8(slot + j * sizeof(uint8_t)) !=
                expected_bytes[i *
                AI_VSEG_STORE_PRESSURE_INDEXED_E8_SEG_FIELDS + j]) {
                return false;
            }
        }
    }

    return true;
}

static inline bool ai_vseg_store_pressure_vssseg2e8_stride_match(
    uintptr_t paddr, unsigned vl, unsigned stride,
    const uint8_t *expected_bytes)
{
    for (unsigned i = 0; i < vl; ++i) {
        const uintptr_t slot = paddr + i * stride;

        if (read8(slot) != expected_bytes[i * 2U] ||
            read8(slot + sizeof(uint8_t)) !=
                expected_bytes[i * 2U + 1U]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_vseg_store_pressure_cross16_sd_values_match(
    uintptr_t paddr, unsigned count, uint64_t expected_base,
    uint64_t expected_step)
{
    uint64_t expected = expected_base;

    for (unsigned i = 0; i < count; ++i) {
        const uintptr_t slot = paddr +
            i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE + 12U;
        if (read32(slot) != (uint32_t)expected ||
            read32(slot + sizeof(uint32_t)) !=
                (uint32_t)(expected >> 32)) {
            return false;
        }
        expected += expected_step;
    }

    return true;
}

static inline void ai_vseg_store_pressure_fence_rw(void)
{
    asm volatile("fence rw, rw" ::: "memory");
}

static inline void ai_vseg_store_pressure_cbo_flush_blocks(uintptr_t base,
    unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        cbo_flush(base + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    }
}

static inline void ai_vseg_store_pressure_cbo_flush_stride(uintptr_t base,
    unsigned count, unsigned stride)
{
    for (unsigned i = 0; i < count; ++i) {
        cbo_flush(base + i * stride);
    }
}

static inline void ai_vseg_store_pressure_vlseg2e32_to_v8(
    const uint32_t *src)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlseg2e32.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(1U)
        : "t0", "memory");
}

static inline void ai_vseg_store_pressure_vlseg2e32_to_v8_vl(
    const uint32_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlseg2e32.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
    const uint32_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vlseg2e32.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_vseg_store_pressure_vlseg2e8_to_v8_m4(
    const uint8_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m4, ta, ma\n\t"
        "vlseg2e8.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_vseg_store_pressure_vlseg4e8_to_v8_m2(
    const uint8_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m2, ta, ma\n\t"
        "vlseg4e8.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_vseg_store_pressure_pre63_tail_sd_then_vsseg2e32(
    uintptr_t pre_base0, uintptr_t pre_base1, uint64_t value,
    uint64_t tail_value, uint32_t *dst, uintptr_t post_base,
    uint64_t post_value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "sd %[value], 0(%[base0])\n\t"
        "sd %[value], 64(%[base0])\n\t"
        "sd %[value], 128(%[base0])\n\t"
        "sd %[value], 192(%[base0])\n\t"
        "sd %[value], 256(%[base0])\n\t"
        "sd %[value], 320(%[base0])\n\t"
        "sd %[value], 384(%[base0])\n\t"
        "sd %[value], 448(%[base0])\n\t"
        "sd %[value], 512(%[base0])\n\t"
        "sd %[value], 576(%[base0])\n\t"
        "sd %[value], 640(%[base0])\n\t"
        "sd %[value], 704(%[base0])\n\t"
        "sd %[value], 768(%[base0])\n\t"
        "sd %[value], 832(%[base0])\n\t"
        "sd %[value], 896(%[base0])\n\t"
        "sd %[value], 960(%[base0])\n\t"
        "sd %[value], 1024(%[base0])\n\t"
        "sd %[value], 1088(%[base0])\n\t"
        "sd %[value], 1152(%[base0])\n\t"
        "sd %[value], 1216(%[base0])\n\t"
        "sd %[value], 1280(%[base0])\n\t"
        "sd %[value], 1344(%[base0])\n\t"
        "sd %[value], 1408(%[base0])\n\t"
        "sd %[value], 1472(%[base0])\n\t"
        "sd %[value], 1536(%[base0])\n\t"
        "sd %[value], 1600(%[base0])\n\t"
        "sd %[value], 1664(%[base0])\n\t"
        "sd %[value], 1728(%[base0])\n\t"
        "sd %[value], 1792(%[base0])\n\t"
        "sd %[value], 1856(%[base0])\n\t"
        "sd %[value], 1920(%[base0])\n\t"
        "sd %[value], 1984(%[base0])\n\t"
        "sd %[value], 0(%[base1])\n\t"
        "sd %[value], 64(%[base1])\n\t"
        "sd %[value], 128(%[base1])\n\t"
        "sd %[value], 192(%[base1])\n\t"
        "sd %[value], 256(%[base1])\n\t"
        "sd %[value], 320(%[base1])\n\t"
        "sd %[value], 384(%[base1])\n\t"
        "sd %[value], 448(%[base1])\n\t"
        "sd %[value], 512(%[base1])\n\t"
        "sd %[value], 576(%[base1])\n\t"
        "sd %[value], 640(%[base1])\n\t"
        "sd %[value], 704(%[base1])\n\t"
        "sd %[value], 768(%[base1])\n\t"
        "sd %[value], 832(%[base1])\n\t"
        "sd %[value], 896(%[base1])\n\t"
        "sd %[value], 960(%[base1])\n\t"
        "sd %[value], 1024(%[base1])\n\t"
        "sd %[value], 1088(%[base1])\n\t"
        "sd %[value], 1152(%[base1])\n\t"
        "sd %[value], 1216(%[base1])\n\t"
        "sd %[value], 1280(%[base1])\n\t"
        "sd %[value], 1344(%[base1])\n\t"
        "sd %[value], 1408(%[base1])\n\t"
        "sd %[value], 1472(%[base1])\n\t"
        "sd %[value], 1536(%[base1])\n\t"
        "sd %[value], 1600(%[base1])\n\t"
        "sd %[value], 1664(%[base1])\n\t"
        "sd %[value], 1728(%[base1])\n\t"
        "sd %[value], 1792(%[base1])\n\t"
        "sd %[value], 1856(%[base1])\n\t"
        "sd %[value], 1920(%[base1])\n\t"
        "sd %[tail_value], 1984(%[base1])\n\t"
        "vsseg2e32.v v8, (%[dst])\n\t"
        "sd %[post_value], 0(%[post_base])\n\t"
        "sd %[post_value], 64(%[post_base])\n\t"
        "sd %[post_value], 128(%[post_base])\n\t"
        "sd %[post_value], 192(%[post_base])\n\t"
        "sd %[post_value], 256(%[post_base])\n\t"
        "sd %[post_value], 320(%[post_base])\n\t"
        "sd %[post_value], 384(%[post_base])\n\t"
        "sd %[post_value], 448(%[post_base])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base0] "r"(pre_base0),
          [base1] "r"(pre_base1),
          [value] "r"(value),
          [tail_value] "r"(tail_value),
          [dst] "r"(dst),
          [post_base] "r"(post_base),
          [post_value] "r"(post_value),
          [vl] "r"(1U)
        : "t0", "memory");
}

static inline void ai_vseg_store_pressure_loop_sd_vsseg2e32(
    uintptr_t scalar_base, uintptr_t seg_base, uint64_t value_base,
    uint64_t value_step, unsigned count)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "mv t1, %[scalar_base]\n\t"
        "mv t2, %[seg_base]\n\t"
        "mv t3, %[value_base]\n\t"
        "mv t4, %[count]\n\t"
        "mv t5, %[value_step]\n\t"
        "1:\n\t"
        "sd t3, 0(t1)\n\t"
        "vsseg2e32.v v8, (t2)\n\t"
        "addi t1, t1, 64\n\t"
        "addi t2, t2, 64\n\t"
        "add t3, t3, t5\n\t"
        "addi t4, t4, -1\n\t"
        "bnez t4, 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [scalar_base] "r"(scalar_base),
          [seg_base] "r"(seg_base),
          [value_base] "r"(value_base),
          [value_step] "r"(value_step),
          [count] "r"(count),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_SEG_VL)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

static inline void ai_vseg_store_pressure_loop_vsseg2e32_then_sd(
    uintptr_t scalar_base, uintptr_t seg_base, uint64_t value_base,
    uint64_t value_step, unsigned count)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "mv t1, %[scalar_base]\n\t"
        "mv t2, %[seg_base]\n\t"
        "mv t3, %[value_base]\n\t"
        "mv t4, %[count]\n\t"
        "mv t5, %[value_step]\n\t"
        "1:\n\t"
        "vsseg2e32.v v8, (t2)\n\t"
        "sd t3, 0(t1)\n\t"
        "addi t1, t1, 64\n\t"
        "addi t2, t2, 64\n\t"
        "add t3, t3, t5\n\t"
        "addi t4, t4, -1\n\t"
        "bnez t4, 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [scalar_base] "r"(scalar_base),
          [seg_base] "r"(seg_base),
          [value_base] "r"(value_base),
          [value_step] "r"(value_step),
          [count] "r"(count),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_SEG_VL)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

static inline void ai_vseg_store_pressure_loop_vsseg2e32_then_sd_stride(
    uintptr_t scalar_base, uintptr_t seg_base, uint64_t value_base,
    uint64_t value_step, unsigned count, unsigned stride)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "mv t1, %[scalar_base]\n\t"
        "mv t2, %[seg_base]\n\t"
        "mv t3, %[value_base]\n\t"
        "mv t4, %[count]\n\t"
        "mv t5, %[value_step]\n\t"
        "mv t6, %[stride]\n\t"
        "1:\n\t"
        "vsseg2e32.v v8, (t2)\n\t"
        "sd t3, 0(t1)\n\t"
        "add t1, t1, t6\n\t"
        "add t2, t2, t6\n\t"
        "add t3, t3, t5\n\t"
        "addi t4, t4, -1\n\t"
        "bnez t4, 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [scalar_base] "r"(scalar_base),
          [seg_base] "r"(seg_base),
          [value_base] "r"(value_base),
          [value_step] "r"(value_step),
          [count] "r"(count),
          [stride] "r"((uintptr_t)stride),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_SEG_VL)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");
}

static inline void ai_vseg_store_pressure_probe_64sd_vsseg2e32m4_post64(
    uintptr_t pre_base, uintptr_t seg_base, uintptr_t post_base,
    uint64_t pre_value_base, uint64_t post_value_base, uint64_t value_step)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[pre_base]\n\t"
        "mv t2, %[pre_value_base]\n\t"
        "li t3, 64\n\t"
        "1:\n\t"
        "sd t2, 0(t1)\n\t"
        "addi t1, t1, 64\n\t"
        "add t2, t2, %[value_step]\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vsseg2e32.v v8, (%[seg_base])\n\t"
        "mv t1, %[post_base]\n\t"
        "mv t2, %[post_value_base]\n\t"
        "li t3, 64\n\t"
        "2:\n\t"
        "sd t2, 0(t1)\n\t"
        "addi t1, t1, 64\n\t"
        "add t2, t2, %[value_step]\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 2b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [pre_base] "r"(pre_base),
          [seg_base] "r"(seg_base),
          [post_base] "r"(post_base),
          [pre_value_base] "r"(pre_value_base),
          [post_value_base] "r"(post_value_base),
          [value_step] "r"(value_step),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_PROBE_VL)
        : "t0", "t1", "t2", "t3", "memory");
}

static inline void ai_vseg_store_pressure_probe_64cross16sd_vsseg2e32m4_post64(
    uintptr_t pre_base, uintptr_t seg_base, uintptr_t post_base,
    uint64_t pre_value_base, uint64_t post_value_base, uint64_t value_step)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[pre_base]\n\t"
        "mv t2, %[pre_value_base]\n\t"
        "li t3, 64\n\t"
        "1:\n\t"
        "sd t2, 12(t1)\n\t"
        "addi t1, t1, 64\n\t"
        "add t2, t2, %[value_step]\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vsseg2e32.v v8, (%[seg_base])\n\t"
        "mv t1, %[post_base]\n\t"
        "mv t2, %[post_value_base]\n\t"
        "li t3, 64\n\t"
        "2:\n\t"
        "sd t2, 12(t1)\n\t"
        "addi t1, t1, 64\n\t"
        "add t2, t2, %[value_step]\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 2b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [pre_base] "r"(pre_base),
          [seg_base] "r"(seg_base),
          [post_base] "r"(post_base),
          [pre_value_base] "r"(pre_value_base),
          [post_value_base] "r"(post_value_base),
          [value_step] "r"(value_step),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_PROBE_VL)
        : "t0", "t1", "t2", "t3", "memory");
}

static inline void ai_vseg_store_pressure_probe_64sd_vssseg2e32m4_post64(
    uintptr_t pre_base, uintptr_t seg_base, uintptr_t post_base,
    uintptr_t stride, uint64_t pre_value_base, uint64_t post_value_base,
    uint64_t value_step)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[pre_base]\n\t"
        "mv t2, %[pre_value_base]\n\t"
        "li t3, 64\n\t"
        "1:\n\t"
        "sd t2, 0(t1)\n\t"
        "addi t1, t1, 64\n\t"
        "add t2, t2, %[value_step]\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vssseg2e32.v v8, (%[seg_base]), %[stride]\n\t"
        "mv t1, %[post_base]\n\t"
        "mv t2, %[post_value_base]\n\t"
        "li t3, 64\n\t"
        "2:\n\t"
        "sd t2, 0(t1)\n\t"
        "addi t1, t1, 64\n\t"
        "add t2, t2, %[value_step]\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 2b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [pre_base] "r"(pre_base),
          [seg_base] "r"(seg_base),
          [post_base] "r"(post_base),
          [stride] "r"(stride),
          [pre_value_base] "r"(pre_value_base),
          [post_value_base] "r"(post_value_base),
          [value_step] "r"(value_step),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_PROBE_VL)
        : "t0", "t1", "t2", "t3", "memory");
}

static inline void ai_vseg_store_pressure_late_fault_vssseg2_then_sd_stride(
    uintptr_t seg_base, uintptr_t stride, uintptr_t scalar_base,
    uint64_t value_base, uint64_t value_step, unsigned count)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vssseg2e32.v v8, (%[seg_base]), %[stride]\n\t"
        "mv t1, %[scalar_base]\n\t"
        "mv t2, %[value_base]\n\t"
        "mv t3, %[count]\n\t"
        "mv t4, %[value_step]\n\t"
        "1:\n\t"
        "sd t2, 0(t1)\n\t"
        "add t1, t1, %[stride]\n\t"
        "add t2, t2, t4\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [seg_base] "r"(seg_base),
          [stride] "r"(stride),
          [scalar_base] "r"(scalar_base),
          [value_base] "r"(value_base),
          [value_step] "r"(value_step),
          [count] "r"(count),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_PROBE_VL)
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

static inline void ai_vseg_store_pressure_vsuxseg2ei32_then_sd_stride(
    uintptr_t seg_base, uintptr_t offsets, uintptr_t scalar_base,
    uint64_t value_base, uint64_t value_step, unsigned count)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v16, (%[offsets])\n\t"
        "vsuxseg2ei32.v v8, (%[seg_base]), v16\n\t"
        "mv t1, %[scalar_base]\n\t"
        "mv t2, %[value_base]\n\t"
        "mv t3, %[count]\n\t"
        "mv t4, %[value_step]\n\t"
        "1:\n\t"
        "sd t2, 0(t1)\n\t"
        "addi t1, t1, 64\n\t"
        "add t2, t2, t4\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [seg_base] "r"(seg_base),
          [offsets] "r"(offsets),
          [scalar_base] "r"(scalar_base),
          [value_base] "r"(value_base),
          [value_step] "r"(value_step),
          [count] "r"(count),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_INDEXED_VL)
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

static inline void ai_vseg_store_pressure_vsuxseg2ei32_vl_then_sd_stride(
    uintptr_t seg_base, uintptr_t offsets, uintptr_t scalar_base,
    uint64_t value_base, uint64_t value_step, unsigned count,
    unsigned scalar_stride, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v16, (%[offsets])\n\t"
        "vsuxseg2ei32.v v8, (%[seg_base]), v16\n\t"
        "mv t1, %[scalar_base]\n\t"
        "mv t2, %[value_base]\n\t"
        "mv t3, %[count]\n\t"
        "mv t4, %[value_step]\n\t"
        "mv t5, %[scalar_stride]\n\t"
        "1:\n\t"
        "sd t2, 0(t1)\n\t"
        "add t1, t1, t5\n\t"
        "add t2, t2, t4\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [seg_base] "r"(seg_base),
          [offsets] "r"(offsets),
          [scalar_base] "r"(scalar_base),
          [value_base] "r"(value_base),
          [value_step] "r"(value_step),
          [count] "r"(count),
          [scalar_stride] "r"((uintptr_t)scalar_stride),
          [vl] "r"(vl)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

static inline void ai_vseg_store_pressure_vsuxseg2ei32_vl_only(
    uintptr_t seg_base, uintptr_t offsets, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v16, (%[offsets])\n\t"
        "vsuxseg2ei32.v v8, (%[seg_base]), v16\n\t"
        ".option pop\n\t"
        :
        : [seg_base] "r"(seg_base),
          [offsets] "r"(offsets),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_vseg_store_pressure_vsuxseg2ei32_vl_only_trap_clobber(
    uintptr_t seg_base, uintptr_t offsets, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v16, (%[offsets])\n\t"
        "vsuxseg2ei32.v v8, (%[seg_base]), v16\n\t"
        ".option pop\n\t"
        :
        : [seg_base] "r"(seg_base),
          [offsets] "r"(offsets),
          [vl] "r"(vl)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");
}

static inline void ai_vseg_store_pressure_vsuxseg4ei32_e8_m2_vl_only(
    uintptr_t seg_base, uintptr_t offsets, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m2, ta, ma\n\t"
        "vle32.v v16, (%[offsets])\n\t"
        "vsuxseg4ei32.v v8, (%[seg_base]), v16\n\t"
        ".option pop\n\t"
        :
        : [seg_base] "r"(seg_base),
          [offsets] "r"(offsets),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_vseg_store_pressure_vsuxseg4ei32_e8_m2_vl_only_trap_clobber(
    uintptr_t seg_base, uintptr_t offsets, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m2, ta, ma\n\t"
        "vle32.v v16, (%[offsets])\n\t"
        "vsuxseg4ei32.v v8, (%[seg_base]), v16\n\t"
        ".option pop\n\t"
        :
        : [seg_base] "r"(seg_base),
          [offsets] "r"(offsets),
          [vl] "r"(vl)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");
}

static inline void ai_vseg_store_pressure_sd_stride_window(
    uintptr_t scalar_base, uint64_t value_base, uint64_t value_step,
    unsigned count, unsigned scalar_stride)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[scalar_base]\n\t"
        "mv t2, %[value_base]\n\t"
        "mv t3, %[count]\n\t"
        "mv t4, %[value_step]\n\t"
        "mv t5, %[scalar_stride]\n\t"
        "1:\n\t"
        "sd t2, 0(t1)\n\t"
        "add t1, t1, t5\n\t"
        "add t2, t2, t4\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [scalar_base] "r"(scalar_base),
          [value_base] "r"(value_base),
          [value_step] "r"(value_step),
          [count] "r"(count),
          [scalar_stride] "r"((uintptr_t)scalar_stride)
        : "t1", "t2", "t3", "t4", "t5", "memory");
}

static inline void ai_vseg_store_pressure_64sd_vssseg2e8_then_64sd_stride(
    uintptr_t pre_base, uintptr_t seg_base, uintptr_t post_base,
    uintptr_t stride, uint64_t pre_value_base, uint64_t post_value_base,
    uint64_t value_step, unsigned count)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[pre_base]\n\t"
        "mv t2, %[pre_value_base]\n\t"
        "mv t3, %[count]\n\t"
        "mv t4, %[value_step]\n\t"
        "mv t5, %[stride]\n\t"
        "1:\n\t"
        "sd t2, 0(t1)\n\t"
        "add t1, t1, t5\n\t"
        "add t2, t2, t4\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        "vsetvli t0, %[vl], e8, m4, ta, ma\n\t"
        "vssseg2e8.v v8, (%[seg_base]), %[stride]\n\t"
        "mv t1, %[post_base]\n\t"
        "mv t2, %[post_value_base]\n\t"
        "mv t3, %[count]\n\t"
        "2:\n\t"
        "sd t2, 0(t1)\n\t"
        "add t1, t1, t5\n\t"
        "add t2, t2, t4\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 2b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [pre_base] "r"(pre_base),
          [seg_base] "r"(seg_base),
          [post_base] "r"(post_base),
          [stride] "r"(stride),
          [pre_value_base] "r"(pre_value_base),
          [post_value_base] "r"(post_value_base),
          [value_step] "r"(value_step),
          [count] "r"(count),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_LONG_E8_VL)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

static inline void ai_vseg_store_pressure_64slot_sd_vssseg2e8_then_64slot_sd_stride(
    uintptr_t pre_slot_base, uintptr_t seg_base, uintptr_t post_slot_base,
    uintptr_t stride, uint64_t pre_value_base, uint64_t post_value_base,
    uint64_t value_step, unsigned count)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[pre_slot_base]\n\t"
        "mv t2, %[pre_value_base]\n\t"
        "mv t3, %[count]\n\t"
        "mv t4, %[value_step]\n\t"
        "mv t5, %[stride]\n\t"
        "1:\n\t"
        "sd t2, 0(t1)\n\t"
        "add t1, t1, t5\n\t"
        "add t2, t2, t4\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        "vsetvli t0, %[vl], e8, m4, ta, ma\n\t"
        "vssseg2e8.v v8, (%[seg_base]), %[stride]\n\t"
        "mv t1, %[post_slot_base]\n\t"
        "mv t2, %[post_value_base]\n\t"
        "mv t3, %[count]\n\t"
        "2:\n\t"
        "sd t2, 0(t1)\n\t"
        "add t1, t1, t5\n\t"
        "add t2, t2, t4\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 2b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [pre_slot_base] "r"(pre_slot_base),
          [seg_base] "r"(seg_base),
          [post_slot_base] "r"(post_slot_base),
          [stride] "r"(stride),
          [pre_value_base] "r"(pre_value_base),
          [post_value_base] "r"(post_value_base),
          [value_step] "r"(value_step),
          [count] "r"(count),
          [vl] "r"(AI_VSEG_STORE_PRESSURE_LONG_E8_VL)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

static inline void ai_vseg_store_pressure_post8(uintptr_t base,
    uint64_t value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[value], 0(%[base])\n\t"
        "sd %[value], 64(%[base])\n\t"
        "sd %[value], 128(%[base])\n\t"
        "sd %[value], 192(%[base])\n\t"
        "sd %[value], 256(%[base])\n\t"
        "sd %[value], 320(%[base])\n\t"
        "sd %[value], 384(%[base])\n\t"
        "sd %[value], 448(%[base])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [value] "r"(value)
        : "memory");
}

bool ai_micro_hs_sbuffer_backpressure_sd_vsseg2_valid_not_dropped_corner()
{
    TEST_START();

    const enum test_page pre_page = VSRWX_GRWX;
    const enum test_page src_post_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t pre_base_vaddr = hs_page_base(pre_page);
    const uintptr_t pre_base1_vaddr = pre_base_vaddr +
        (AI_VSEG_STORE_PRESSURE_PRE_STORES / 2U) *
        AI_VSEG_STORE_PRESSURE_LINE_STRIDE;
    const uintptr_t pre_base_paddr = phys_page_base(pre_page);
    const uintptr_t post_base_vaddr = hs_page_base(src_post_page);
    const uintptr_t post_base_paddr = phys_page_base(src_post_page);
    const uintptr_t src_vaddr = hs_page_base(src_post_page) + 0x800U;
    const uintptr_t src_paddr = phys_page_base(src_post_page) + 0x800U;
    const uintptr_t dst_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t dst_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t pre_seed = 0x1111222233334444ULL;
    const uint64_t pre_value = 0xa5a5000000006401ULL;
    const uint64_t tail_sd_value = 0xd15c0de000006401ULL;
    const uint64_t post_seed = 0x5555666677778888ULL;
    const uint64_t post_value = 0x5a5a000000000808ULL;
    const uint32_t src_words[2] = {
        0x6401f00dU, 0x6401bad0U,
    };
    const uint32_t old_field0 = 0x10101010U;
    const uint32_t old_field1 = 0x20202020U;
    const uint32_t low_guard = 0x30303030U;
    const uint32_t high_guard = 0x40404040U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(pre_base_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE,
            pre_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_POST_STORES; ++i) {
        write64(post_base_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE,
            post_seed);
    }
    write32(src_paddr, src_words[0]);
    write32(src_paddr + 4U, src_words[1]);
    write32(dst_paddr, old_field0);
    write32(dst_paddr - 4U, low_guard);
    write32(fault_paddr, old_field1);
    write32(fault_paddr + 4U, high_guard);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_blocks(pre_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    ai_vseg_store_pressure_cbo_flush_blocks(post_base_paddr,
        AI_VSEG_STORE_PRESSURE_POST_STORES);
    cbo_flush(src_paddr);
    cbo_flush(dst_paddr);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(pre_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(src_post_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8((const uint32_t *)src_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("setup loads one nf=2 e32 segment payload into v8/v9 before the scalar-store pressure window",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_pre63_tail_sd_then_vsseg2e32(pre_base_vaddr,
        pre_base1_vaddr, pre_value, tail_sd_value, (uint32_t *)dst_vaddr,
        post_base_vaddr, post_value);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("under sbuffer pressure, the faulting vsseg2e32 segment0 field1 reports store page fault after a tail scalar sd competes with the segment-store request",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first 63 scalar stores create sbuffer pressure by updating distinct cache blocks before the scalar-vsseg arbitration point",
        ai_vseg_store_pressure_blocks_match(pre_base_paddr,
            AI_VSEG_STORE_PRESSURE_TAIL_SD_INDEX, pre_value)
    );
    TEST_ASSERT("the tail scalar sd immediately before vsseg2e32 is not dropped when scalar-store valid competes with segment-store valid under sbuffer backpressure",
        read64(pre_base_paddr +
            AI_VSEG_STORE_PRESSURE_TAIL_SD_INDEX *
            AI_VSEG_STORE_PRESSURE_LINE_STRIDE) == tail_sd_value,
        "tail_sd=0x%lx expected=0x%lx",
        read64(pre_base_paddr +
            AI_VSEG_STORE_PRESSURE_TAIL_SD_INDEX *
            AI_VSEG_STORE_PRESSURE_LINE_STRIDE),
        tail_sd_value
    );
    TEST_ASSERT("the faulting segment store makes segment0 field0 visible while segment0 field1 and adjacent guards remain old",
        read32(dst_paddr) == src_words[0] &&
        read32(fault_paddr) == old_field1 &&
        read32(dst_paddr - 4U) == low_guard &&
        read32(fault_paddr + 4U) == high_guard,
        "field0=%x expected=%x field1=%x old=%x",
        read32(dst_paddr), src_words[0], read32(fault_paddr), old_field1
    );
    TEST_ASSERT("the scalar stores placed immediately after the faulting segment instruction update distinct cache blocks without corrupting the vector source payload",
        ai_vseg_store_pressure_blocks_match(post_base_paddr,
            AI_VSEG_STORE_PRESSURE_POST_STORES, post_value) &&
        read32(src_paddr) == src_words[0] &&
        read32(src_paddr + 4U) == src_words[1]
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_backpressure_loop_sd_vsseg2_commit_not_dropped_corner()
{
    TEST_START();

    const enum test_page scalar_page = VSRWX_GRWX;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page seg_dst_page = SWITCH1;
    const uintptr_t scalar_base_vaddr = hs_page_base(scalar_page);
    const uintptr_t scalar_base_paddr = phys_page_base(scalar_page);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page);
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page);
    const uint64_t scalar_seed = 0x3333444455556666ULL;
    const uint64_t scalar_value_base = 0xffffffc000c01080ULL;
    const uint64_t scalar_value_step = 0x0000000001013588ULL;
    const uint32_t seg_words[AI_VSEG_STORE_PRESSURE_SEG_WORDS] = {
        0x1f800001U, 0x0db09001U,
        0x1f800002U, 0x0db09002U,
        0x1f800003U, 0x0db09003U,
        0x1f800004U, 0x0db09004U,
    };
    const uint32_t seg_seed = 0xfeed6401U;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(scalar_base_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE,
            scalar_seed);
        for (unsigned j = 0; j < AI_VSEG_STORE_PRESSURE_SEG_WORDS; ++j) {
            write32(seg_dst_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE +
                j * sizeof(uint32_t), seg_seed);
        }
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_SEG_WORDS; ++i) {
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_blocks(scalar_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    ai_vseg_store_pressure_cbo_flush_blocks(seg_dst_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    cbo_flush(seg_src_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(scalar_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_dst_page,
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_vl(
        (const uint32_t *)seg_src_vaddr, AI_VSEG_STORE_PRESSURE_SEG_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=4 nf=2 e32 segment payload into v8/v9 before the looped scalar-store pressure window",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_loop_sd_vsseg2e32(scalar_base_vaddr,
        seg_dst_vaddr, scalar_value_base, scalar_value_step,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    AI_ASSERT_NO_EXCEPT_AND("one fixed-PC scalar sd loop interleaved with vl=4 vsseg2e32 stores retires without exception under sbuffer backpressure",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the looped scalar sd commits all 64 dynamic stores from the same PC without losing one store-valid request to the segment-store path",
        ai_vseg_store_pressure_loop_values_match(scalar_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_value_base,
            scalar_value_step)
    );
    TEST_ASSERT("the interleaved vl=4 vsseg2e32 stream writes each segment block while preserving scalar-store commit ordering",
        ai_vseg_store_pressure_seg_blocks_match(seg_dst_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, seg_words)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_backpressure_loop_vsseg2_then_sd_commit_not_dropped_corner()
{
    TEST_START();

    const enum test_page scalar_page = VSRWX_GRWX;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page seg_dst_page = SWITCH1;
    const uintptr_t scalar_base_vaddr = hs_page_base(scalar_page);
    const uintptr_t scalar_base_paddr = phys_page_base(scalar_page);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page);
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page);
    const uint64_t scalar_seed = 0x777788889999aaaaULL;
    const uint64_t scalar_value_base = 0xffffffc3400db000ULL;
    const uint64_t scalar_value_step = 0x0000000000000094ULL;
    const uint32_t seg_words[AI_VSEG_STORE_PRESSURE_SEG_WORDS] = {
        0x64010001U, 0xdb090001U,
        0x64010002U, 0xdb090002U,
        0x64010003U, 0xdb090003U,
        0x64010004U, 0xdb090004U,
    };
    const uint32_t seg_seed = 0xfeed9008U;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(scalar_base_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE,
            scalar_seed);
        for (unsigned j = 0; j < AI_VSEG_STORE_PRESSURE_SEG_WORDS; ++j) {
            write32(seg_dst_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE +
                j * sizeof(uint32_t), seg_seed);
        }
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_SEG_WORDS; ++i) {
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_blocks(scalar_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    ai_vseg_store_pressure_cbo_flush_blocks(seg_dst_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    cbo_flush(seg_src_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(scalar_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_dst_page,
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_vl(
        (const uint32_t *)seg_src_vaddr, AI_VSEG_STORE_PRESSURE_SEG_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=4 nf=2 e32 segment payload before the segment-store then scalar-sd pressure window",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_loop_vsseg2e32_then_sd(scalar_base_vaddr,
        seg_dst_vaddr, scalar_value_base, scalar_value_step,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    AI_ASSERT_NO_EXCEPT_AND("one fixed-PC scalar sd immediately after each vl=4 vsseg2e32 retires without exception under sbuffer backpressure",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-segment fixed-PC scalar sd commits all 64 dynamic stores and is not dropped behind segment-store valid arbitration",
        ai_vseg_store_pressure_loop_values_match(scalar_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_value_base,
            scalar_value_step)
    );
    TEST_ASSERT("the leading vl=4 vsseg2e32 stream writes each segment block while the following scalar sd stream stays ordered",
        ai_vseg_store_pressure_seg_blocks_match(seg_dst_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, seg_words)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_backpressure_loop_vsseg2_then_sd_stride58_commit_not_dropped_corner()
{
    TEST_START();

    const enum test_page scalar_page = VSRWX_GRWX;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page seg_dst_page = SWITCH1;
    const uintptr_t scalar_base_vaddr = hs_page_base(scalar_page);
    const uintptr_t scalar_base_paddr = phys_page_base(scalar_page);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page);
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page);
    const unsigned stride = AI_VSEG_STORE_PRESSURE_COMMIT_MISMATCH_STRIDE;
    const uint64_t scalar_seed = 0x2222333344445555ULL;
    const uint64_t scalar_value_base = 0xffffffc000c01f80ULL;
    const uint64_t scalar_value_step = 0x000000004300bb14ULL;
    const uint32_t seg_words[AI_VSEG_STORE_PRESSURE_SEG_WORDS] = {
        0x90080001U, 0xc0010001U,
        0x90080002U, 0xc0010002U,
        0x90080003U, 0xc0010003U,
        0x90080004U, 0xc0010004U,
    };
    const uint32_t seg_seed = 0xfeed5808U;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(scalar_base_paddr + i * stride, scalar_seed);
        for (unsigned j = 0; j < AI_VSEG_STORE_PRESSURE_SEG_WORDS; ++j) {
            write32(seg_dst_paddr + i * stride + j * sizeof(uint32_t),
                seg_seed);
        }
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_SEG_WORDS; ++i) {
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    ai_vseg_store_pressure_fence_rw();
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        cbo_flush(scalar_base_paddr + i * stride);
        cbo_flush(seg_dst_paddr + i * stride);
    }
    cbo_flush(seg_src_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(scalar_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_dst_page,
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_vl(
        (const uint32_t *)seg_src_vaddr, AI_VSEG_STORE_PRESSURE_SEG_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=4 nf=2 e32 segment payload before the stride-0x58 segment-store then scalar-sd pressure window",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_loop_vsseg2e32_then_sd_stride(scalar_base_vaddr,
        seg_dst_vaddr, scalar_value_base, scalar_value_step,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, stride);
    AI_ASSERT_NO_EXCEPT_AND("one fixed-PC scalar sd after each vl=4 vsseg2e32 uses 0x58 stride and retires without exception under sbuffer backpressure",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the stride-0x58 fixed-PC scalar sd stream commits all dynamic stores without the one-entry store-commit shift seen in lost-valid bugs",
        ai_vseg_store_pressure_loop_values_match_stride(scalar_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, stride, scalar_value_base,
            scalar_value_step)
    );
    TEST_ASSERT("the stride-0x58 leading vsseg2e32 stream writes every segment block while the following scalar sd stream stays aligned",
        ai_vseg_store_pressure_seg_blocks_match_stride(seg_dst_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, stride, seg_words)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_late_fault_vssseg2_then_handler_sd_stride58_repro_corner()
{
    TEST_START();

    const enum test_page scalar_page0 = VSRWX_GRWX;
    const enum test_page scalar_page1 = VSRWX_GRW;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned stride = AI_VSEG_STORE_PRESSURE_COMMIT_MISMATCH_STRIDE;
    const unsigned fault_idx = AI_VSEG_STORE_PRESSURE_LATE_FAULT_IDX;
    const uintptr_t scalar_base_vaddr = hs_page_base(scalar_page0);
    const uintptr_t scalar_base_paddr = phys_page_base(scalar_page0);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_offset =
        PAGE_SIZE - sizeof(uint32_t) - fault_idx * stride;
    const uintptr_t seg_dst_vaddr = hs_page_base(low_page) + seg_dst_offset;
    const uintptr_t seg_dst_paddr = phys_page_base(low_page) + seg_dst_offset;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t scalar_seed = 0x13579bdf2468ace0ULL;
    const uint64_t scalar_value_base = 0xffffffc000c01f80ULL;
    const uint64_t scalar_value_step = 0x000000004300bb14ULL;
    const uint32_t low_seed = 0x51515151U;
    const uint32_t high_seed = 0xa7a7a7a7U;
    uint32_t seg_words[AI_VSEG_STORE_PRESSURE_PROBE_WORDS];
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(scalar_base_paddr + i * stride, scalar_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PROBE_WORDS; ++i) {
        seg_words[i] = 0x58080000U + i;
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    for (unsigned i = 0; i < fault_idx; ++i) {
        const uintptr_t slot = seg_dst_paddr + i * stride;

        write32(slot, low_seed);
        write32(slot + sizeof(uint32_t), low_seed);
    }
    write32(seg_dst_paddr + fault_idx * stride, low_seed);
    write32(fault_paddr, high_seed);
    write32(fault_paddr + sizeof(uint32_t), high_seed);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_stride(scalar_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, stride);
    ai_vseg_store_pressure_cbo_flush_stride(seg_dst_paddr, fault_idx + 1U,
        stride);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(scalar_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(scalar_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
        (const uint32_t *)seg_src_vaddr, AI_VSEG_STORE_PRESSURE_PROBE_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=16 nf=2 e32 m4 payload before the late-field1 vssseg2e32 fault reproducer",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_PROBE_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_late_fault_vssseg2_then_sd_stride(seg_dst_vaddr,
        stride, scalar_base_vaddr, scalar_value_base, scalar_value_step,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("late field1 vssseg2e32 raises store page fault at element15 and then the fixed-PC stride-0x58 scalar sd loop runs after handler return",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr,
        observed_vstart, fault_idx
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("late-fault vssseg2e32 commits all prefix segment fields and the faulting element field0 while field1 remains old",
        ai_vseg_store_pressure_vssseg2_stride_match(seg_dst_paddr, fault_idx,
            stride, seg_words) &&
        read32(seg_dst_paddr + fault_idx * stride) ==
            seg_words[fault_idx * 2U] &&
        read32(fault_paddr) == high_seed,
        "prefix0=0x%x fault_field0=0x%x expected=0x%x fault_field1=0x%x old=0x%x",
        read32(seg_dst_paddr), read32(seg_dst_paddr + fault_idx * stride),
        seg_words[fault_idx * 2U], read32(fault_paddr), high_seed
    );
    TEST_ASSERT("the post-handler fixed-PC scalar sd stream commits 64 dynamic stride-0x58 stores with screenshot-shaped addr/data spacing",
        ai_vseg_store_pressure_loop_values_match_stride(scalar_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, stride, scalar_value_base,
            scalar_value_step)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_indexed_vsegment_fill_then_sd_drop_repro_corner()
{
    TEST_START();

    const enum test_page scalar_page0 = VSRWX_GRWX;
    const enum test_page scalar_page1 = VSRWX_GRW;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t scalar_base_vaddr = hs_page_base(scalar_page0);
    const uintptr_t scalar_base_paddr = phys_page_base(scalar_page0);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t offsets_vaddr = hs_page_base(seg_src_page) + 0xa00U;
    const uintptr_t offsets_paddr = phys_page_base(seg_src_page) + 0xa00U;
    const uintptr_t seg_base_vaddr = hs_page_base(low_page);
    const uintptr_t seg_base_paddr = phys_page_base(low_page);
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t scalar_seed = 0x4242424264646464ULL;
    const uint64_t scalar_value_base = 0xffffffc000c0a000ULL;
    const uint64_t scalar_value_step = 0x0000000000000188ULL;
    const uint32_t low_seed = 0x51515151U;
    const uint32_t high_seed = 0xa7a7a7a7U;
    const unsigned fault_idx = AI_VSEG_STORE_PRESSURE_INDEXED_PREFIX;
    uint32_t seg_words[AI_VSEG_STORE_PRESSURE_INDEXED_WORDS];
    uint32_t offsets[AI_VSEG_STORE_PRESSURE_INDEXED_VL];
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(scalar_base_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE,
            scalar_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_INDEXED_WORDS; ++i) {
        seg_words[i] = 0x1d500000U + i;
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_INDEXED_PREFIX; ++i) {
        offsets[i] = i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE;
        write32(offsets_paddr + i * sizeof(uint32_t), offsets[i]);
        write32(seg_base_paddr + offsets[i], low_seed);
        write32(seg_base_paddr + offsets[i] + sizeof(uint32_t), low_seed);
    }
    offsets[fault_idx] = PAGE_SIZE - sizeof(uint32_t);
    write32(offsets_paddr + fault_idx * sizeof(uint32_t),
        offsets[fault_idx]);
    write32(seg_base_paddr + offsets[fault_idx], low_seed);
    write32(fault_paddr, high_seed);
    write32(fault_paddr + sizeof(uint32_t), high_seed);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_blocks(scalar_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    ai_vseg_store_pressure_cbo_flush_stride(seg_base_paddr,
        AI_VSEG_STORE_PRESSURE_INDEXED_PREFIX,
        AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(seg_base_paddr + offsets[fault_idx]);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(offsets_paddr);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(scalar_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(scalar_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
        (const uint32_t *)seg_src_vaddr,
        AI_VSEG_STORE_PRESSURE_INDEXED_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=9 nf=2 e32 payload before the indexed segment sbuffer-fill pressure window",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_INDEXED_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vsuxseg2ei32_then_sd_stride(seg_base_vaddr,
        offsets_vaddr, scalar_base_vaddr, scalar_value_base,
        scalar_value_step, AI_VSEG_STORE_PRESSURE_PRE_STORES);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("vl=9 vsuxseg2ei32 sends eight prefix elements to distinct cache blocks, keeps element8 field0 visible, and reports field1 store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr,
        observed_vstart, fault_idx
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("indexed segment-store prefix fills eight distinct sbuffer cache-block slots before the faulting element",
        ai_vseg_store_pressure_vsuxseg2ei32_match(seg_base_paddr,
            offsets, AI_VSEG_STORE_PRESSURE_INDEXED_PREFIX, seg_words)
    );
    TEST_ASSERT("faulting indexed element keeps field0 side effect visible while field1 remains old on the X-only high page",
        read32(seg_base_paddr + offsets[fault_idx]) ==
            seg_words[fault_idx * 2U] &&
        read32(fault_paddr) == high_seed &&
        read32(fault_paddr + sizeof(uint32_t)) == high_seed,
        "field0=0x%x expected=0x%x field1=0x%x old=0x%x",
        read32(seg_base_paddr + offsets[fault_idx]),
        seg_words[fault_idx * 2U], read32(fault_paddr), high_seed
    );
    TEST_ASSERT("the 64 younger scalar sd stores after the indexed segment fault handler commit to different cache blocks without a dropped valid request",
        ai_vseg_store_pressure_loop_values_match(scalar_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_value_base,
            scalar_value_step)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_indexed_vsegment_pending_then_sd_drop_difftest_repro_corner()
{
    TEST_START();

    const enum test_page scalar_page0 = VSRWX_GRWX;
    const enum test_page scalar_page1 = VSRWX_GRW;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned scalar_stride =
        AI_VSEG_STORE_PRESSURE_COMMIT_MISMATCH_STRIDE;
    const uintptr_t scalar_base_vaddr = hs_page_base(scalar_page0);
    const uintptr_t scalar_base_paddr = phys_page_base(scalar_page0);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t offsets_vaddr = hs_page_base(seg_src_page) + 0xa00U;
    const uintptr_t offsets_paddr = phys_page_base(seg_src_page) + 0xa00U;
    const uintptr_t seg_base_vaddr = hs_page_base(low_page);
    const uintptr_t seg_base_paddr = phys_page_base(low_page);
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t scalar_seed = 0x5353535362626262ULL;
    const uint64_t scalar_value_base = 0xffffffc000c01080ULL;
    const uint64_t scalar_value_step = 0x000000004300bb14ULL;
    const uint32_t low_seed = 0x62626262U;
    const uint32_t high_seed = 0xb6b6b6b6U;
    const unsigned fault_idx = AI_VSEG_STORE_PRESSURE_PENDING_PREFIX;
    uint32_t seg_words[AI_VSEG_STORE_PRESSURE_PENDING_WORDS];
    uint32_t offsets[AI_VSEG_STORE_PRESSURE_PENDING_VL];
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(scalar_base_paddr + i * scalar_stride, scalar_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PENDING_WORDS; ++i) {
        seg_words[i] = 0x5b000000U + i;
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PENDING_PREFIX; ++i) {
        offsets[i] = i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE;
        write32(offsets_paddr + i * sizeof(uint32_t), offsets[i]);
        write32(seg_base_paddr + offsets[i], low_seed);
        write32(seg_base_paddr + offsets[i] + sizeof(uint32_t), low_seed);
    }
    offsets[fault_idx] = PAGE_SIZE - sizeof(uint32_t);
    write32(offsets_paddr + fault_idx * sizeof(uint32_t),
        offsets[fault_idx]);
    write32(seg_base_paddr + offsets[fault_idx], low_seed);
    write32(fault_paddr, high_seed);
    write32(fault_paddr + sizeof(uint32_t), high_seed);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_stride(scalar_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_stride);
    ai_vseg_store_pressure_cbo_flush_stride(seg_base_paddr,
        AI_VSEG_STORE_PRESSURE_PENDING_PREFIX,
        AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(seg_base_paddr + offsets[fault_idx]);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(offsets_paddr);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(scalar_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(scalar_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
        (const uint32_t *)seg_src_vaddr,
        AI_VSEG_STORE_PRESSURE_PENDING_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=10 nf=2 e32 payload before the indexed vsegment pending-sbuffer repro",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_PENDING_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vsuxseg2ei32_vl_then_sd_stride(seg_base_vaddr,
        offsets_vaddr, scalar_base_vaddr, scalar_value_base,
        scalar_value_step, AI_VSEG_STORE_PRESSURE_PRE_STORES,
        scalar_stride, AI_VSEG_STORE_PRESSURE_PENDING_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("vl=10 vsuxseg2ei32 writes nine prefix elements, leaves the last prefix request in the vsegment pipeline under sbuffer pressure, then reports element9 field1 store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr,
        observed_vstart, fault_idx
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("indexed segment-store prefix covers nine distinct cache-block slots before the following scalar sd stream",
        ai_vseg_store_pressure_vsuxseg2ei32_match(seg_base_paddr,
            offsets, AI_VSEG_STORE_PRESSURE_PENDING_PREFIX, seg_words)
    );
    TEST_ASSERT("faulting indexed element keeps field0 visible while field1 remains old on the X-only high page",
        read32(seg_base_paddr + offsets[fault_idx]) ==
            seg_words[fault_idx * 2U] &&
        read32(fault_paddr) == high_seed &&
        read32(fault_paddr + sizeof(uint32_t)) == high_seed,
        "field0=0x%x expected=0x%x field1=0x%x old=0x%x",
        read32(seg_base_paddr + offsets[fault_idx]),
        seg_words[fault_idx * 2U], read32(fault_paddr), high_seed
    );
    TEST_ASSERT("the post-fault stride-0x58 scalar sd stream commits every request; a missing port0 backpressure priority shows up as a LinkNan store-commit shift or memory gap",
        ai_vseg_store_pressure_loop_values_match_stride(scalar_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_stride,
            scalar_value_base, scalar_value_step)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_indexed_vsegment_fault_handler_head_sd_drop_difftest_repro_corner()
{
    TEST_START();

    const enum test_page scalar_page0 = VSRWX_GRWX;
    const enum test_page scalar_page1 = VSRWX_GRW;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned scalar_stride =
        AI_VSEG_STORE_PRESSURE_COMMIT_MISMATCH_STRIDE;
    const uintptr_t scalar_base_vaddr = hs_page_base(scalar_page0);
    const uintptr_t scalar_base_paddr = phys_page_base(scalar_page0);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t offsets_vaddr = hs_page_base(seg_src_page) + 0xa00U;
    const uintptr_t offsets_paddr = phys_page_base(seg_src_page) + 0xa00U;
    const uintptr_t seg_base_vaddr = hs_page_base(low_page);
    const uintptr_t seg_base_paddr = phys_page_base(low_page);
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t scalar_seed = 0x5454545478787878ULL;
    const uint64_t scalar_value_base = 0xffffffc000c01080ULL;
    const uint64_t scalar_value_step = 0x000000004300bb14ULL;
    const uint32_t low_seed = 0x78787878U;
    const uint32_t high_seed = 0xd4d4d4d4U;
    const unsigned fault_idx = AI_VSEG_STORE_PRESSURE_PENDING_PREFIX;
    uint32_t seg_words[AI_VSEG_STORE_PRESSURE_PENDING_WORDS];
    uint32_t offsets[AI_VSEG_STORE_PRESSURE_PENDING_VL];
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(scalar_base_paddr + i * scalar_stride, scalar_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PENDING_WORDS; ++i) {
        seg_words[i] = 0x6d000000U + i;
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PENDING_PREFIX; ++i) {
        offsets[i] = i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE;
        write32(offsets_paddr + i * sizeof(uint32_t), offsets[i]);
        write32(seg_base_paddr + offsets[i], low_seed);
        write32(seg_base_paddr + offsets[i] + sizeof(uint32_t), low_seed);
    }
    offsets[fault_idx] = PAGE_SIZE - sizeof(uint32_t);
    write32(offsets_paddr + fault_idx * sizeof(uint32_t),
        offsets[fault_idx]);
    write32(seg_base_paddr + offsets[fault_idx], low_seed);
    write32(fault_paddr, high_seed);
    write32(fault_paddr + sizeof(uint32_t), high_seed);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_stride(scalar_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_stride);
    ai_vseg_store_pressure_cbo_flush_stride(seg_base_paddr,
        AI_VSEG_STORE_PRESSURE_PENDING_PREFIX,
        AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(seg_base_paddr + offsets[fault_idx]);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(offsets_paddr);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF));
    ai_vseg_store_pressure_set_hs_leaf(scalar_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(scalar_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
        (const uint32_t *)seg_src_vaddr,
        AI_VSEG_STORE_PRESSURE_PENDING_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=10 nf=2 e32 payload before the handler-head scalar-sd indexed-vsegment repro",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_PENDING_VL
    );

    ai_vseg_store_pressure_handler_sd_addr = scalar_base_vaddr;
    ai_vseg_store_pressure_handler_sd_value = scalar_value_base;
    ai_vseg_store_pressure_handler_sd_fired = 0ULL;
    ai_vseg_store_pressure_handler_cause = 0ULL;
    ai_vseg_store_pressure_handler_tval = 0ULL;
    ai_vseg_store_pressure_handler_epc = 0ULL;
    ai_vseg_store_pressure_handler_vstart = 0ULL;

    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_vseg_store_pressure_trap_scratch[
        sizeof(ai_vseg_store_pressure_trap_scratch) /
        sizeof(ai_vseg_store_pressure_trap_scratch[0])]);
    CSRW(stvec, ai_vseg_store_pressure_fault_head_sd_handler);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vsuxseg2ei32_vl_only(seg_base_vaddr,
        offsets_vaddr, AI_VSEG_STORE_PRESSURE_PENDING_VL);
    ai_vseg_store_pressure_sd_stride_window(scalar_base_vaddr + scalar_stride,
        scalar_value_base + scalar_value_step, scalar_value_step,
        AI_VSEG_STORE_PRESSURE_PRE_STORES - 1U, scalar_stride);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    TEST_ASSERT("vl=10 indexed segment store reports element9 field1 store page fault, and the local trap-entry handler records vstart before issuing following scalar stores",
        ai_vseg_store_pressure_handler_sd_fired == 1ULL &&
        ai_vseg_store_pressure_handler_cause == CAUSE_SPF &&
        ai_vseg_store_pressure_handler_tval == fault_vaddr &&
        ai_vseg_store_pressure_handler_vstart == fault_idx,
        "handler_fired=%lu cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%u epc=0x%lx",
        ai_vseg_store_pressure_handler_sd_fired,
        ai_vseg_store_pressure_handler_cause,
        ai_vseg_store_pressure_handler_tval, fault_vaddr,
        ai_vseg_store_pressure_handler_vstart, fault_idx,
        ai_vseg_store_pressure_handler_epc
    );
    TEST_ASSERT("indexed segment-store prefix writes nine distinct cache-block slots before the handler-head scalar sd competes with the pending vsegment sbuffer request",
        ai_vseg_store_pressure_vsuxseg2ei32_match(seg_base_paddr,
            offsets, AI_VSEG_STORE_PRESSURE_PENDING_PREFIX, seg_words)
    );
    TEST_ASSERT("faulting indexed element keeps field0 visible while field1 remains old on the X-only high page",
        read32(seg_base_paddr + offsets[fault_idx]) ==
            seg_words[fault_idx * 2U] &&
        read32(fault_paddr) == high_seed &&
        read32(fault_paddr + sizeof(uint32_t)) == high_seed,
        "field0=0x%x expected=0x%x field1=0x%x old=0x%x",
        read32(seg_base_paddr + offsets[fault_idx]),
        seg_words[fault_idx * 2U], read32(fault_paddr), high_seed
    );
    TEST_ASSERT("the scalar stream includes the handler-head sd at slot0 plus 63 following stride-0x58 sd stores; missing scalar ready backpressure should surface as a LinkNan store-commit mismatch before this selfcheck",
        ai_vseg_store_pressure_loop_values_match_stride(scalar_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_stride,
            scalar_value_base, scalar_value_step)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_indexed_vseg_pending_fault_head_sd_drop_repro_corner()
{
    TEST_START();

    const enum test_page scalar_page0 = VSRWX_GRWX;
    const enum test_page scalar_page1 = VSRWX_GRW;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page alias_page = VSRWXPbmt0_GURWXPbmt0;
    const unsigned scalar_stride =
        AI_VSEG_STORE_PRESSURE_COMMIT_MISMATCH_STRIDE;
    const unsigned fault_idx =
        AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FAULT_IDX;
    const unsigned alias_idx = fault_idx - 1U;
    const unsigned seg_stride = AI_VSEG_STORE_PRESSURE_LINE_STRIDE;
    const uintptr_t scalar_base_vaddr = hs_page_base(scalar_page0);
    const uintptr_t scalar_base_paddr = phys_page_base(scalar_page0);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t offsets_vaddr = hs_page_base(seg_src_page) + 0xa00U;
    const uintptr_t offsets_paddr = phys_page_base(seg_src_page) + 0xa00U;
    const uintptr_t seg_base_vaddr = hs_page_base(low_page);
    const uintptr_t seg_base_paddr = phys_page_base(low_page);
    const uintptr_t alias_base_vaddr = hs_page_base(alias_page);
    const uintptr_t pending_offset = PAGE_SIZE - sizeof(uint32_t);
    const uintptr_t fault_offset = pending_offset + sizeof(uint32_t);
    const uintptr_t alias_vaddr = alias_base_vaddr;
    const uintptr_t alias_paddr = seg_base_paddr;
    const uintptr_t pending_vaddr = seg_base_vaddr + pending_offset;
    const uintptr_t fault_vaddr = seg_base_vaddr + fault_offset;
    const uintptr_t pending_paddr = seg_base_paddr + pending_offset;
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t scalar_seed = 0x5858585847474747ULL;
    const uint64_t scalar_value_base =
        AI_VSEG_STORE_PRESSURE_FAST_SD_VALUE_BASE;
    const uint64_t scalar_value_step =
        AI_VSEG_STORE_PRESSURE_FAST_SD_VALUE_STEP;
    const uint32_t low_seed = 0x47474747U;
    const uint32_t high_seed = 0x8a8a8a8aU;
    uint32_t seg_words[AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_WORDS];
    uint32_t offsets[AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_VL];
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(scalar_base_paddr + i * scalar_stride, scalar_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_WORDS; ++i) {
        seg_words[i] = 0x7d000000U + i;
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    for (unsigned i = 0; i < fault_idx; ++i) {
        offsets[i] = i * seg_stride;
        write32(offsets_paddr + i * sizeof(uint32_t), offsets[i]);
        write32(seg_base_paddr + offsets[i], low_seed);
        write32(seg_base_paddr + offsets[i] + sizeof(uint32_t), low_seed);
    }
    offsets[alias_idx] = alias_vaddr - seg_base_vaddr;
    write32(offsets_paddr + alias_idx * sizeof(uint32_t),
        offsets[alias_idx]);
    offsets[fault_idx] = pending_offset;
    write32(offsets_paddr + fault_idx * sizeof(uint32_t),
        offsets[fault_idx]);
    write32(alias_paddr, low_seed);
    write32(alias_paddr + sizeof(uint32_t), low_seed);
    write32(pending_paddr, low_seed);
    write32(fault_paddr, high_seed);
    write32(fault_paddr + sizeof(uint32_t), high_seed);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_stride(scalar_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_stride);
    ai_vseg_store_pressure_cbo_flush_stride(seg_base_paddr, fault_idx,
        seg_stride);
    cbo_flush(alias_paddr);
    cbo_flush(pending_paddr);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(offsets_paddr);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF));
    ai_vseg_store_pressure_set_hs_leaf(scalar_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(scalar_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    *ai_vseg_store_pressure_hs_leaf_entry(alias_page) =
        (alias_paddr >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
        (const uint32_t *)seg_src_vaddr,
        AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=9 nf=2 e32 payload before the alias-drain indexed-vsegment pending repro",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_VL
    );

    ai_vseg_store_pressure_handler_sd_addr = scalar_base_vaddr;
    ai_vseg_store_pressure_handler_sd_stride = scalar_stride;
    ai_vseg_store_pressure_handler_sd_value = scalar_value_base;
    ai_vseg_store_pressure_handler_sd_step = scalar_value_step;
    ai_vseg_store_pressure_handler_sd_count =
        AI_VSEG_STORE_PRESSURE_PRE_STORES;
    ai_vseg_store_pressure_handler_sd_fired = 0ULL;
    ai_vseg_store_pressure_handler_cause = 0ULL;
    ai_vseg_store_pressure_handler_tval = 0ULL;
    ai_vseg_store_pressure_handler_epc = 0ULL;
    ai_vseg_store_pressure_handler_vstart = 0ULL;

    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, scalar_base_vaddr);
    CSRW(stvec, ai_vseg_store_pressure_fault_head_first_sd_loop_handler);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vsuxseg2ei32_vl_only_trap_clobber(
        seg_base_vaddr,
        offsets_vaddr, AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_VL);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    TEST_ASSERT("vl=9 indexed nf=2 e32 segment store uses a prefix alias-vtag merge to force sbuffer drain, leaves element8 field0 pending, then reports field1 store page fault before the trap-head scalar sd loop",
        ai_vseg_store_pressure_handler_sd_fired == 1ULL &&
        ai_vseg_store_pressure_handler_cause == CAUSE_SPF &&
        ai_vseg_store_pressure_handler_tval == fault_vaddr &&
        ai_vseg_store_pressure_handler_vstart == fault_idx,
        "handler_fired=%lu cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%u epc=0x%lx",
        ai_vseg_store_pressure_handler_sd_fired,
        ai_vseg_store_pressure_handler_cause,
        ai_vseg_store_pressure_handler_tval, fault_vaddr,
        ai_vseg_store_pressure_handler_vstart, fault_idx,
        ai_vseg_store_pressure_handler_epc
    );
    TEST_ASSERT("indexed nf=2 e32 segment-store prefix writes six non-alias cache-block slots before an alias-vtag merge on element7 forces sbuffer drain",
        ai_vseg_store_pressure_vsuxseg2ei32_field_count_match(
            seg_base_paddr, offsets + 1U, alias_idx - 1U,
            seg_words + AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS) &&
        read32(alias_paddr) ==
            seg_words[alias_idx *
            AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS] &&
        read32(alias_paddr + sizeof(uint32_t)) ==
            seg_words[alias_idx *
            AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS + 1U]
    );
    TEST_ASSERT("the fault element field0 reaches the low-page tail while field1 remains old on the X-only high page",
        read32(pending_paddr) ==
            seg_words[fault_idx *
            AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS] &&
        read32(fault_paddr) == high_seed &&
        read32(fault_paddr + sizeof(uint32_t)) == high_seed,
        "pending_field0=0x%x expected0=0x%x field1=0x%x guard=0x%x old=0x%x",
        read32(pending_paddr),
        seg_words[fault_idx *
            AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS],
        read32(fault_paddr), read32(fault_paddr + sizeof(uint32_t)),
        high_seed
    );
    TEST_ASSERT("the prefix alias element is visible through the aliased physical cache line before the handler-head scalar sd collision window",
        read32(alias_paddr) ==
            seg_words[fault_idx *
            AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS - 2U] &&
        read32(alias_paddr + sizeof(uint32_t)) ==
            seg_words[fault_idx *
            AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS - 1U],
        "alias0=0x%x expected0=0x%x alias1=0x%x expected1=0x%x",
        read32(alias_paddr),
        seg_words[fault_idx *
            AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS - 2U],
        read32(alias_paddr + sizeof(uint32_t)),
        seg_words[fault_idx *
            AI_VSEG_STORE_PRESSURE_ALIAS_DRAIN_FIELDS - 1U]
    );
    TEST_ASSERT("the trap-head scalar sd loop commits all 64 stride-0x58 stores; buggy LinkNan port0 arbitration should show a store-commit mismatch or visible scalar gap before this selfcheck can pass",
        ai_vseg_store_pressure_loop_values_match_stride(scalar_base_paddr,
            AI_VSEG_STORE_PRESSURE_FAST_SD_COUNT, scalar_stride,
            scalar_value_base, scalar_value_step)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_indexed_vsegment_late_pending_then_sd_drop_difftest_repro_corner()
{
    TEST_START();

    const enum test_page scalar_page0 = VSRWX_GRWX;
    const enum test_page scalar_page1 = VSRWX_GRW;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned scalar_stride =
        AI_VSEG_STORE_PRESSURE_COMMIT_MISMATCH_STRIDE;
    const uintptr_t scalar_base_vaddr = hs_page_base(scalar_page0);
    const uintptr_t scalar_base_paddr = phys_page_base(scalar_page0);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t offsets_vaddr = hs_page_base(seg_src_page) + 0xa00U;
    const uintptr_t offsets_paddr = phys_page_base(seg_src_page) + 0xa00U;
    const uintptr_t seg_base_vaddr = hs_page_base(low_page);
    const uintptr_t seg_base_paddr = phys_page_base(low_page);
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t scalar_seed = 0x4c4c4c4c73737373ULL;
    const uint64_t scalar_value_base = 0xffffffc000c01080ULL;
    const uint64_t scalar_value_step = 0x000000004300bb14ULL;
    const uint32_t low_seed = 0x73737373U;
    const uint32_t high_seed = 0xc4c4c4c4U;
    const unsigned fault_idx =
        AI_VSEG_STORE_PRESSURE_INDEXED_LATE_FAULT_IDX;
    uint32_t seg_words[AI_VSEG_STORE_PRESSURE_INDEXED_LATE_WORDS];
    uint32_t offsets[AI_VSEG_STORE_PRESSURE_INDEXED_LATE_VL];
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(scalar_base_paddr + i * scalar_stride, scalar_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_INDEXED_LATE_WORDS; ++i) {
        seg_words[i] = 0x1a700000U + i;
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    for (unsigned i = 0; i < fault_idx; ++i) {
        offsets[i] = i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE;
        write32(offsets_paddr + i * sizeof(uint32_t), offsets[i]);
        write32(seg_base_paddr + offsets[i], low_seed);
        write32(seg_base_paddr + offsets[i] + sizeof(uint32_t), low_seed);
    }
    offsets[fault_idx] = PAGE_SIZE - sizeof(uint32_t);
    write32(offsets_paddr + fault_idx * sizeof(uint32_t),
        offsets[fault_idx]);
    write32(seg_base_paddr + offsets[fault_idx], low_seed);
    write32(fault_paddr, high_seed);
    write32(fault_paddr + sizeof(uint32_t), high_seed);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_stride(scalar_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_stride);
    ai_vseg_store_pressure_cbo_flush_stride(seg_base_paddr, fault_idx,
        AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(seg_base_paddr + offsets[fault_idx]);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(offsets_paddr);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(scalar_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(scalar_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
        (const uint32_t *)seg_src_vaddr,
        AI_VSEG_STORE_PRESSURE_INDEXED_LATE_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=16 nf=2 e32 payload before the late indexed-vsegment sbuffer pressure repro",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_INDEXED_LATE_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vsuxseg2ei32_vl_then_sd_stride(seg_base_vaddr,
        offsets_vaddr, scalar_base_vaddr, scalar_value_base,
        scalar_value_step, AI_VSEG_STORE_PRESSURE_PRE_STORES,
        scalar_stride, AI_VSEG_STORE_PRESSURE_INDEXED_LATE_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("vl=16 indexed segment store fills the 8-entry sbuffer, keeps later prefix requests pending, and reports element15 field1 store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr,
        observed_vstart, fault_idx
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the late indexed segment-store prefix writes fifteen distinct cache-block slots before scalar sd pressure drains",
        ai_vseg_store_pressure_vsuxseg2ei32_match(seg_base_paddr,
            offsets, fault_idx, seg_words)
    );
    TEST_ASSERT("the faulting late indexed element keeps field0 visible while field1 remains old on the X-only high page",
        read32(seg_base_paddr + offsets[fault_idx]) ==
            seg_words[fault_idx * 2U] &&
        read32(fault_paddr) == high_seed &&
        read32(fault_paddr + sizeof(uint32_t)) == high_seed,
        "field0=0x%x expected=0x%x field1=0x%x old=0x%x",
        read32(seg_base_paddr + offsets[fault_idx]),
        seg_words[fault_idx * 2U], read32(fault_paddr), high_seed
    );
    TEST_ASSERT("the post-fault stride-0x58 scalar sd stream commits every request; LinkNan missing scalar ready backpressure should produce a store-commit mismatch before this passes",
        ai_vseg_store_pressure_loop_values_match_stride(scalar_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, scalar_stride,
            scalar_value_base, scalar_value_step)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_probe_long_vsseg2_scalar_overlap_corner()
{
    TEST_START();

    const enum test_page pre_page = VSRWX_GRWX;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page seg_dst_page = SWITCH1;
    const enum test_page post_page = SWITCH2;
    const uintptr_t pre_base_vaddr = hs_page_base(pre_page);
    const uintptr_t pre_base_paddr = phys_page_base(pre_page);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page);
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page);
    const uintptr_t post_base_vaddr = hs_page_base(post_page);
    const uintptr_t post_base_paddr = phys_page_base(post_page);
    const uint64_t pre_seed = 0x1f1f2f2f3f3f4f4fULL;
    const uint64_t post_seed = 0x5151616171718181ULL;
    const uint64_t pre_value_base = 0xffffffc000640000ULL;
    const uint64_t post_value_base = 0xffffffc000650000ULL;
    const uint64_t value_step = 0x0000000000000101ULL;
    const uint32_t seg_seed = 0xfeed1600U;
    uint32_t seg_words[AI_VSEG_STORE_PRESSURE_PROBE_WORDS];

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(pre_base_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE,
            pre_seed);
        write64(post_base_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE,
            post_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PROBE_WORDS; ++i) {
        seg_words[i] = 0x16000000U + i;
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
        write32(seg_dst_paddr + i * sizeof(uint32_t), seg_seed);
    }
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_blocks(pre_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    ai_vseg_store_pressure_cbo_flush_blocks(post_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_dst_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(pre_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_dst_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(post_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
        (const uint32_t *)seg_src_vaddr, AI_VSEG_STORE_PRESSURE_PROBE_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=16 nf=2 e32 m4 segment payload to lengthen the later vsegment sbuffer-valid window",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_PROBE_VL
    );

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_probe_64sd_vsseg2e32m4_post64(pre_base_vaddr,
        seg_dst_vaddr, post_base_vaddr, pre_value_base, post_value_base,
        value_step);
    AI_ASSERT_NO_EXCEPT_AND("64 scalar sd stores, one long vl=16 vsseg2e32, and 64 younger scalar sd stores retire without architectural exception",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older scalar sd stream updates all 64 distinct cache blocks before the long vsegment probe",
        ai_vseg_store_pressure_loop_values_match(pre_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, pre_value_base, value_step)
    );
    TEST_ASSERT("the long m4 vsseg2e32 writes the complete contiguous nf=2 segment image",
        ai_vseg_store_pressure_words_match(seg_dst_paddr,
            AI_VSEG_STORE_PRESSURE_PROBE_WORDS, seg_words)
    );
    TEST_ASSERT("the younger scalar sd stream updates all 64 distinct cache blocks after the long vsegment probe",
        ai_vseg_store_pressure_loop_values_match(post_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, post_value_base, value_step)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_probe_cross16_sd_vsseg2_scalar_overlap_corner()
{
    TEST_START();

    const enum test_page pre_page = VSRWX_GRWX;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page seg_dst_page = SWITCH1;
    const enum test_page post_page = SWITCH2;
    const uintptr_t pre_base_vaddr = hs_page_base(pre_page);
    const uintptr_t pre_base_paddr = phys_page_base(pre_page);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page);
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page);
    const uintptr_t post_base_vaddr = hs_page_base(post_page);
    const uintptr_t post_base_paddr = phys_page_base(post_page);
    const uint64_t pre_value_base = 0xffffffc016160000ULL;
    const uint64_t post_value_base = 0xffffffc017170000ULL;
    const uint64_t value_step = 0x0000000000010001ULL;
    const uint32_t cross_seed_low = 0x31313131U;
    const uint32_t cross_seed_high = 0x42424242U;
    const uint32_t seg_seed = 0xfeed1616U;
    uint32_t seg_words[AI_VSEG_STORE_PRESSURE_PROBE_WORDS];

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        const uintptr_t pre_slot = pre_base_paddr +
            i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE + 12U;
        const uintptr_t post_slot = post_base_paddr +
            i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE + 12U;
        write32(pre_slot, cross_seed_low);
        write32(pre_slot + sizeof(uint32_t), cross_seed_high);
        write32(post_slot, cross_seed_low);
        write32(post_slot + sizeof(uint32_t), cross_seed_high);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PROBE_WORDS; ++i) {
        seg_words[i] = 0x16160000U + i;
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
        write32(seg_dst_paddr + i * sizeof(uint32_t), seg_seed);
    }
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_blocks(pre_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    ai_vseg_store_pressure_cbo_flush_blocks(post_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_dst_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(pre_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_dst_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(post_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
        (const uint32_t *)seg_src_vaddr, AI_VSEG_STORE_PRESSURE_PROBE_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=16 nf=2 e32 m4 segment payload before the cross16 scalar-store probe window",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_PROBE_VL
    );

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_probe_64cross16sd_vsseg2e32m4_post64(
        pre_base_vaddr, seg_dst_vaddr, post_base_vaddr, pre_value_base,
        post_value_base, value_step);
    AI_ASSERT_NO_EXCEPT_AND("64 cross16 scalar sd stores, one long vl=16 vsseg2e32, and 64 younger cross16 scalar sd stores retire without architectural exception",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older cross16 scalar sd stream updates the low and high halves of all 64 distinct cache blocks",
        ai_vseg_store_pressure_cross16_sd_values_match(pre_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, pre_value_base, value_step)
    );
    TEST_ASSERT("the long m4 vsseg2e32 writes the complete segment image between the older and younger cross16 scalar streams",
        ai_vseg_store_pressure_words_match(seg_dst_paddr,
            AI_VSEG_STORE_PRESSURE_PROBE_WORDS, seg_words)
    );
    TEST_ASSERT("the younger cross16 scalar sd stream updates the low and high halves of all 64 distinct cache blocks",
        ai_vseg_store_pressure_cross16_sd_values_match(post_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, post_value_base, value_step)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_probe_strided_vssseg2_scalar_overlap_corner()
{
    TEST_START();

    const enum test_page pre_page = VSRWX_GRWX;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page seg_dst_page = SWITCH1;
    const enum test_page post_page = SWITCH2;
    const uintptr_t pre_base_vaddr = hs_page_base(pre_page);
    const uintptr_t pre_base_paddr = phys_page_base(pre_page);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page);
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page);
    const uintptr_t post_base_vaddr = hs_page_base(post_page);
    const uintptr_t post_base_paddr = phys_page_base(post_page);
    const uintptr_t stride = AI_VSEG_STORE_PRESSURE_LINE_STRIDE;
    const uint64_t pre_seed = 0x2323232345454545ULL;
    const uint64_t post_seed = 0x6767676789898989ULL;
    const uint64_t pre_value_base = 0xffffffc018180000ULL;
    const uint64_t post_value_base = 0xffffffc019190000ULL;
    const uint64_t value_step = 0x0000000000000111ULL;
    const uint32_t seg_seed = 0x5eed1800U;
    uint32_t seg_words[AI_VSEG_STORE_PRESSURE_PROBE_WORDS];

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(pre_base_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE,
            pre_seed);
        write64(post_base_paddr + i * AI_VSEG_STORE_PRESSURE_LINE_STRIDE,
            post_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PROBE_WORDS; ++i) {
        seg_words[i] = 0x18180000U + i;
        write32(seg_src_paddr + i * sizeof(uint32_t), seg_words[i]);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PROBE_VL; ++i) {
        const uintptr_t slot = seg_dst_paddr + i * stride;

        write32(slot, seg_seed);
        write32(slot + sizeof(uint32_t), seg_seed);
    }
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_blocks(pre_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    ai_vseg_store_pressure_cbo_flush_blocks(post_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES);
    ai_vseg_store_pressure_cbo_flush_blocks(seg_dst_paddr,
        AI_VSEG_STORE_PRESSURE_PROBE_VL);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(pre_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_dst_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(post_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e32_to_v8_m4(
        (const uint32_t *)seg_src_vaddr, AI_VSEG_STORE_PRESSURE_PROBE_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=16 nf=2 e32 m4 segment payload before the strided vsegment sbuffer-valid probe",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_PROBE_VL
    );

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_probe_64sd_vssseg2e32m4_post64(
        pre_base_vaddr, seg_dst_vaddr, post_base_vaddr, stride,
        pre_value_base, post_value_base, value_step);
    AI_ASSERT_NO_EXCEPT_AND("64 scalar sd stores, one stride-64B vl=16 vssseg2e32, and 64 younger scalar sd stores retire without architectural exception",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older scalar sd stream updates all 64 distinct cache blocks before the strided vsegment probe",
        ai_vseg_store_pressure_loop_values_match(pre_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, pre_value_base, value_step)
    );
    TEST_ASSERT("the strided m4 vssseg2e32 writes each nf=2 element into its own cache block",
        ai_vseg_store_pressure_vssseg2_stride_match(seg_dst_paddr,
            AI_VSEG_STORE_PRESSURE_PROBE_VL,
            AI_VSEG_STORE_PRESSURE_LINE_STRIDE, seg_words)
    );
    TEST_ASSERT("the younger scalar sd stream updates all 64 distinct cache blocks after the strided vsegment probe",
        ai_vseg_store_pressure_loop_values_match(post_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, post_value_base, value_step)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_probe_long_vssseg2e8_stride58_scalar_overlap_corner()
{
    TEST_START();

    const enum test_page pre_page0 = VSRWX_GRWX;
    const enum test_page pre_page1 = VSRWX_GRW;
    const enum test_page post_page0 = VSRW_GRWX;
    const enum test_page post_page1 = VSRW_GRW;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned stride = AI_VSEG_STORE_PRESSURE_COMMIT_MISMATCH_STRIDE;
    const unsigned fault_idx = AI_VSEG_STORE_PRESSURE_LONG_E8_FAULT_IDX;
    const uintptr_t pre_base_vaddr = hs_page_base(pre_page0);
    const uintptr_t pre_base_paddr = phys_page_base(pre_page0);
    const uintptr_t post_base_vaddr = hs_page_base(post_page0);
    const uintptr_t post_base_paddr = phys_page_base(post_page0);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_offset =
        PAGE_SIZE - sizeof(uint8_t) - fault_idx * stride;
    const uintptr_t seg_dst_vaddr = hs_page_base(low_page) + seg_dst_offset;
    const uintptr_t seg_dst_paddr = phys_page_base(low_page) + seg_dst_offset;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t pre_seed = 0x2424242458585858ULL;
    const uint64_t post_seed = 0x3535353569696969ULL;
    const uint64_t pre_value_base = 0xffffffc000c01f80ULL;
    const uint64_t post_value_base = 0xffffffc3400db094ULL;
    const uint64_t value_step = 0x000000004300bb14ULL;
    const uint8_t low_seed = 0x5aU;
    const uint8_t high_seed = 0xa5U;
    uint8_t seg_bytes[AI_VSEG_STORE_PRESSURE_LONG_E8_BYTES];
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(pre_base_paddr + i * stride, pre_seed);
        write64(post_base_paddr + i * stride, post_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_LONG_E8_BYTES; ++i) {
        seg_bytes[i] = (uint8_t)(0x20U + i);
        write8(seg_src_paddr + i * sizeof(uint8_t), seg_bytes[i]);
    }
    for (unsigned i = 0; i < fault_idx; ++i) {
        const uintptr_t slot = seg_dst_paddr + i * stride;

        write8(slot, low_seed);
        write8(slot + sizeof(uint8_t), low_seed);
    }
    write8(seg_dst_paddr + fault_idx * stride, low_seed);
    write8(fault_paddr, high_seed);
    write8(fault_paddr + sizeof(uint8_t), high_seed);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_stride(pre_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, stride);
    ai_vseg_store_pressure_cbo_flush_stride(post_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, stride);
    ai_vseg_store_pressure_cbo_flush_stride(seg_dst_paddr, fault_idx + 1U,
        stride);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(pre_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(pre_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(post_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(post_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e8_to_v8_m4(
        (const uint8_t *)seg_src_vaddr,
        AI_VSEG_STORE_PRESSURE_LONG_E8_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=64 nf=2 e8 m4 payload to maximize the later vsegment sbuffer-valid window",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_LONG_E8_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_64sd_vssseg2e8_then_64sd_stride(pre_base_vaddr,
        seg_dst_vaddr, post_base_vaddr, stride, pre_value_base,
        post_value_base, value_step, AI_VSEG_STORE_PRESSURE_PRE_STORES);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("long vl=64 vssseg2e8 raises store page fault when element46 field1 crosses into the X-only high page after field0 has been sent",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr,
        observed_vstart, fault_idx
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older fixed-PC scalar sd stream commits all 64 stride-0x58 stores before the long vsegment window",
        ai_vseg_store_pressure_loop_values_match_stride(pre_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, stride, pre_value_base,
            value_step)
    );
    TEST_ASSERT("the long vssseg2e8 prefix writes every completed nf=2 element and also writes the faulting element field0 before field1 faults",
        ai_vseg_store_pressure_vssseg2e8_stride_match(seg_dst_paddr,
            fault_idx, stride, seg_bytes) &&
        read8(seg_dst_paddr + fault_idx * stride) ==
            seg_bytes[fault_idx * 2U] &&
        read8(fault_paddr) == high_seed,
        "prefix0=0x%x fault_field0=0x%x expected=0x%x fault_field1=0x%x old=0x%x",
        read8(seg_dst_paddr), read8(seg_dst_paddr + fault_idx * stride),
        seg_bytes[fault_idx * 2U], read8(fault_paddr), high_seed
    );
    TEST_ASSERT("the younger fixed-PC scalar sd stream commits all 64 stride-0x58 stores after handler return, exposing any dropped scalar request as a data or difftest shift",
        ai_vseg_store_pressure_loop_values_match_stride(post_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, stride, post_value_base,
            value_step)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_probe_final_vssseg2e8_stride64_scalar_overlap_corner()
{
    TEST_START();

    const enum test_page pre_page = VSRWX_GRWX;
    const enum test_page post_page = VSRW_GRWX;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned stride = AI_VSEG_STORE_PRESSURE_LINE_STRIDE;
    const unsigned fault_idx = AI_VSEG_STORE_PRESSURE_LONG_E8_FINAL_FAULT_IDX;
    const uintptr_t pre_base_vaddr = hs_page_base(pre_page);
    const uintptr_t pre_base_paddr = phys_page_base(pre_page);
    const uintptr_t post_base_vaddr = hs_page_base(post_page);
    const uintptr_t post_base_paddr = phys_page_base(post_page);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_offset =
        PAGE_SIZE - sizeof(uint8_t) - fault_idx * stride;
    const uintptr_t seg_dst_vaddr = hs_page_base(low_page) + seg_dst_offset;
    const uintptr_t seg_dst_paddr = phys_page_base(low_page) + seg_dst_offset;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t pre_seed = 0x4646464664646464ULL;
    const uint64_t post_seed = 0x5757575775757575ULL;
    const uint64_t pre_value_base = 0xffffffc000640000ULL;
    const uint64_t post_value_base = 0xffffffc001640000ULL;
    const uint64_t value_step = 0x0000000000000101ULL;
    const uint8_t low_seed = 0x6cU;
    const uint8_t high_seed = 0xc6U;
    uint8_t seg_bytes[AI_VSEG_STORE_PRESSURE_LONG_E8_BYTES];
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(pre_base_paddr + i * stride, pre_seed);
        write64(post_base_paddr + i * stride, post_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_LONG_E8_BYTES; ++i) {
        seg_bytes[i] = (uint8_t)(0x40U + i);
        write8(seg_src_paddr + i * sizeof(uint8_t), seg_bytes[i]);
    }
    for (unsigned i = 0; i < fault_idx; ++i) {
        const uintptr_t slot = seg_dst_paddr + i * stride;

        write8(slot, low_seed);
        write8(slot + sizeof(uint8_t), low_seed);
    }
    write8(seg_dst_paddr + fault_idx * stride, low_seed);
    write8(fault_paddr, high_seed);
    write8(fault_paddr + sizeof(uint8_t), high_seed);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_stride(pre_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, stride);
    ai_vseg_store_pressure_cbo_flush_stride(post_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, stride);
    ai_vseg_store_pressure_cbo_flush_stride(seg_dst_paddr, fault_idx + 1U,
        stride);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(pre_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(post_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e8_to_v8_m4(
        (const uint8_t *)seg_src_vaddr,
        AI_VSEG_STORE_PRESSURE_LONG_E8_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=64 nf=2 e8 m4 payload before the maximum-length vssseg2e8 store-fault window",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_LONG_E8_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_64sd_vssseg2e8_then_64sd_stride(pre_base_vaddr,
        seg_dst_vaddr, post_base_vaddr, stride, pre_value_base,
        post_value_base, value_step, AI_VSEG_STORE_PRESSURE_PRE_STORES);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("vl=64 vssseg2e8 reaches the final element: field0 writes the last low-page byte and field1 raises store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr,
        observed_vstart, fault_idx
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older fixed-PC scalar sd stream commits all 64 stride-64B stores before the maximum-length vsegment window",
        ai_vseg_store_pressure_loop_values_match_stride(pre_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, stride, pre_value_base,
            value_step)
    );
    TEST_ASSERT("the maximum-length vssseg2e8 prefix writes completed elements and the final element field0 before field1 faults",
        ai_vseg_store_pressure_vssseg2e8_stride_match(seg_dst_paddr,
            fault_idx, stride, seg_bytes) &&
        read8(seg_dst_paddr + fault_idx * stride) ==
            seg_bytes[fault_idx * 2U] &&
        read8(fault_paddr) == high_seed,
        "prefix0=0x%x final_field0=0x%x expected=0x%x field1=0x%x old=0x%x",
        read8(seg_dst_paddr), read8(seg_dst_paddr + fault_idx * stride),
        seg_bytes[fault_idx * 2U], read8(fault_paddr), high_seed
    );
    TEST_ASSERT("the younger fixed-PC scalar sd stream commits all 64 stride-64B stores after handler return, exposing any lost scalar request as a visible gap or difftest shift",
        ai_vseg_store_pressure_loop_values_match_stride(post_base_paddr,
            AI_VSEG_STORE_PRESSURE_PRE_STORES, stride, post_value_base,
            value_step)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_sbuffer_probe_stride58_tail_vssseg2e8_scalar_difftest_overlap_corner()
{
    TEST_START();

    const enum test_page pre_page0 = VSRWX_GRWX;
    const enum test_page pre_page1 = VSRWX_GRW;
    const enum test_page post_page0 = VSRW_GRWX;
    const enum test_page post_page1 = VSRW_GRW;
    const enum test_page seg_src_page = SCRATCHPAD;
    const enum test_page low_page0 = SWITCH1;
    const enum test_page low_page1 = SWITCH2;
    const enum test_page high_page = VSRWXPbmt0_GURWXPbmt0;
    const unsigned stride = AI_VSEG_STORE_PRESSURE_COMMIT_MISMATCH_STRIDE;
    const unsigned slot_offset = AI_VSEG_STORE_PRESSURE_CROSS16_SLOT_OFFSET;
    const unsigned fault_idx = AI_VSEG_STORE_PRESSURE_LONG_E8_FINAL_FAULT_IDX;
    const uintptr_t pre_base_vaddr = hs_page_base(pre_page0);
    const uintptr_t pre_base_paddr = phys_page_base(pre_page0);
    const uintptr_t post_base_vaddr = hs_page_base(post_page0);
    const uintptr_t post_base_paddr = phys_page_base(post_page0);
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x800U;
    const uintptr_t seg_dst_offset =
        2U * PAGE_SIZE - sizeof(uint8_t) - fault_idx * stride;
    const uintptr_t seg_dst_vaddr = hs_page_base(low_page0) + seg_dst_offset;
    const uintptr_t seg_dst_paddr = phys_page_base(low_page0) +
        seg_dst_offset;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint64_t pre_seed = 0x5858585816161616ULL;
    const uint64_t post_seed = 0x6969696927272727ULL;
    const uint64_t pre_value_base = 0xffffffc000c01080ULL;
    const uint64_t post_value_base = 0xffffffc000c01080ULL +
        AI_VSEG_STORE_PRESSURE_PRE_STORES *
        0x000000004300bb14ULL;
    const uint64_t value_step = 0x000000004300bb14ULL;
    const uint8_t low_seed = 0x58U;
    const uint8_t high_seed = 0x85U;
    uint8_t seg_bytes[AI_VSEG_STORE_PRESSURE_LONG_E8_BYTES];
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_PRE_STORES; ++i) {
        write64(pre_base_paddr + i * stride + slot_offset, pre_seed);
        write64(post_base_paddr + i * stride + slot_offset, post_seed);
    }
    for (unsigned i = 0; i < AI_VSEG_STORE_PRESSURE_LONG_E8_BYTES; ++i) {
        seg_bytes[i] = (uint8_t)(0x80U + i);
        write8(seg_src_paddr + i * sizeof(uint8_t), seg_bytes[i]);
    }
    for (unsigned i = 0; i < fault_idx; ++i) {
        const uintptr_t slot = seg_dst_paddr + i * stride;

        write8(slot, low_seed);
        write8(slot + sizeof(uint8_t), low_seed);
    }
    write8(seg_dst_paddr + fault_idx * stride, low_seed);
    write8(fault_paddr, high_seed);
    write8(fault_paddr + sizeof(uint8_t), high_seed);
    ai_vseg_store_pressure_fence_rw();
    ai_vseg_store_pressure_cbo_flush_stride(pre_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, stride);
    ai_vseg_store_pressure_cbo_flush_stride(post_base_paddr,
        AI_VSEG_STORE_PRESSURE_PRE_STORES, stride);
    ai_vseg_store_pressure_cbo_flush_stride(seg_dst_paddr, fault_idx + 1U,
        stride);
    cbo_flush(seg_src_paddr);
    cbo_flush(seg_src_paddr + AI_VSEG_STORE_PRESSURE_LINE_STRIDE);
    cbo_flush(fault_paddr);
    ai_vseg_store_pressure_fence_rw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vseg_store_pressure_set_hs_leaf(pre_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(pre_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(post_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(post_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(seg_src_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page0,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(low_page1,
        PTE_V | PTE_AD | PTE_RWX);
    ai_vseg_store_pressure_set_hs_leaf(high_page,
        PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_vlseg2e8_to_v8_m4(
        (const uint8_t *)seg_src_vaddr,
        AI_VSEG_STORE_PRESSURE_LONG_E8_VL);
    AI_ASSERT_NO_EXCEPT_AND("setup loads vl=64 nf=2 e8 m4 payload before the stride-0x58 difftest-overlap probe",
        CSRR(CSR_VL) == AI_VSEG_STORE_PRESSURE_LONG_E8_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vseg_store_pressure_64slot_sd_vssseg2e8_then_64slot_sd_stride(
        pre_base_vaddr + slot_offset, seg_dst_vaddr,
        post_base_vaddr + slot_offset, stride, pre_value_base,
        post_value_base, value_step, AI_VSEG_STORE_PRESSURE_PRE_STORES);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("stride-0x58 vl=64 vssseg2e8 reaches the final element: field0 writes the last low-page byte and field1 raises store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr,
        observed_vstart, fault_idx
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older stride-0x58 scalar sd stream commits all 64 dynamic stores before the faulting vsegment tail window",
        ai_vseg_store_pressure_loop_values_match_stride_offset(
            pre_base_paddr, AI_VSEG_STORE_PRESSURE_PRE_STORES, stride,
            slot_offset, pre_value_base, value_step)
    );
    TEST_ASSERT("the stride-0x58 vssseg2e8 prefix writes completed elements and the final element field0 before field1 faults",
        ai_vseg_store_pressure_vssseg2e8_stride_match(seg_dst_paddr,
            fault_idx, stride, seg_bytes) &&
        read8(seg_dst_paddr + fault_idx * stride) ==
            seg_bytes[fault_idx * 2U] &&
        read8(fault_paddr) == high_seed,
        "prefix0=0x%x final_field0=0x%x expected=0x%x field1=0x%x old=0x%x",
        read8(seg_dst_paddr), read8(seg_dst_paddr + fault_idx * stride),
        seg_bytes[fault_idx * 2U], read8(fault_paddr), high_seed
    );
    TEST_ASSERT("the younger stride-0x58 scalar sd stream commits all 64 dynamic stores, exposing a dropped scalar valid as a data gap or difftest shift",
        ai_vseg_store_pressure_loop_values_match_stride_offset(
            post_base_paddr, AI_VSEG_STORE_PRESSURE_PRE_STORES, stride,
            slot_offset, post_value_base, value_step)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}
