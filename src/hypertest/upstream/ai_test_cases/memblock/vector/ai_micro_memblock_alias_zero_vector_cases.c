#include <rvh_test.h>
#include <page_tables.h>
#include <stddef.h>
#include <stdint.h>

#define AI_P14AV_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_P14AV_BLOCK_BYTES 64U
#define AI_P14AV_BLOCK_WORDS 8U
#define AI_P14AV_VECTOR_BYTES 16U
#define AI_P14AV_VSEG_VL 8U
#define AI_P14AV_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static inline pte_t *ai_p14av_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_p14av_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
}

static inline void ai_p14av_prepare_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_p14av_prepare_same_paddr_alias(
    enum test_page alias_page,
    enum test_page base_page)
{
    goto_priv(PRIV_M);
    *ai_p14av_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline void ai_p14av_reset_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_p14av_seed_word(uint64_t seed, uintptr_t word_idx)
{
    return seed + (word_idx * 0x1111111111111111ULL);
}

static inline void ai_p14av_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_P14AV_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)),
            ai_p14av_seed_word(seed, i));
    }
}

static inline bool ai_p14av_block_all_zero(uintptr_t paddr)
{
    for (uintptr_t i = 0; i < AI_P14AV_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) != 0ULL) {
            return false;
        }
    }

    return true;
}

static inline bool ai_p14av_block_tail_all_zero(uintptr_t paddr,
    unsigned first_zero_word)
{
    for (uintptr_t i = first_zero_word; i < AI_P14AV_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) != 0ULL) {
            return false;
        }
    }

    return true;
}

static inline bool ai_p14av_block_matches_seed(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_P14AV_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) !=
            ai_p14av_seed_word(seed, i)) {
            return false;
        }
    }

    return true;
}

static inline void ai_p14av_write_bytes(uintptr_t paddr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_p14av_bytes_match(uintptr_t paddr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_p14av_zero_active_seed_inactive_capture_matches(
    uintptr_t paddr,
    const uint8_t *inactive_seed,
    const uint8_t *mask_bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        const bool active = ((mask_bytes[i / 8U] >> (i % 8U)) & 1U) != 0U;
        const uint8_t expected = active ? 0U : inactive_seed[i];

        if (read8(paddr + i) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_p14av_active_source_inactive_zero_matches(
    uintptr_t paddr,
    const uint8_t *active_source,
    const uint8_t *mask_bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        const bool active = ((mask_bytes[i / 8U] >> (i % 8U)) & 1U) != 0U;
        const uint8_t expected = active ? active_source[i] : 0U;

        if (read8(paddr + i) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_p14av_cbo_inval_prefetch_zero_vector_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *dst0_observed,
    uint64_t *dst1_observed,
    uint64_t *marker_observed)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t dst0_readback = 0xffffffffffffffffULL;
    uint64_t dst1_readback = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[dst0_readback], 0(%[dst_addr])\n\t"
        "ld %[dst1_readback], 8(%[dst_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [dst0_readback] "=&r"(dst0_readback),
          [dst1_readback] "=&r"(dst1_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_P14AV_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *dst0_observed = dst0_readback;
    *dst1_observed = dst1_readback;
    *marker_observed = marker_readback;
}

static inline void ai_p14av_cbo_inval_prefetch_zero_vsegment_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *dst0_observed,
    uint64_t *dst1_observed,
    uint64_t *marker_observed)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t dst0_readback = 0xffffffffffffffffULL;
    uint64_t dst1_readback = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[src_addr])\n\t"
        "vsseg2e8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[dst0_readback], 0(%[dst_addr])\n\t"
        "ld %[dst1_readback], 8(%[dst_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [dst0_readback] "=&r"(dst0_readback),
          [dst1_readback] "=&r"(dst1_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_P14AV_VSEG_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *dst0_observed = dst0_readback;
    *dst1_observed = dst1_readback;
    *marker_observed = marker_readback;
}

static inline void ai_p14av_cbo_inval_prefetch_zero_vfof_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    unsigned *vl_observed,
    uint64_t *dst0_observed,
    uint64_t *dst1_observed,
    uint64_t *marker_observed)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned captured_vl = 0U;
    uint64_t dst0_readback = 0xffffffffffffffffULL;
    uint64_t dst1_readback = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[src_addr])\n\t"
        "csrr %[captured_vl], vl\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[dst0_readback], 0(%[dst_addr])\n\t"
        "ld %[dst1_readback], 8(%[dst_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [captured_vl] "=&r"(captured_vl),
          [dst0_readback] "=&r"(dst0_readback),
          [dst1_readback] "=&r"(dst1_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_P14AV_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *vl_observed = captured_vl;
    *dst0_observed = dst0_readback;
    *dst1_observed = dst1_readback;
    *marker_observed = marker_readback;
}

static inline void ai_p14av_cbo_inval_prefetch_zero_all_inactive_vstore_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t dst_tail_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *dst0_observed,
    uint64_t *dst1_observed,
    uint64_t *dst_tail_observed,
    uint64_t *marker_observed)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t dst0_readback = 0xffffffffffffffffULL;
    uint64_t dst1_readback = 0xffffffffffffffffULL;
    uint64_t dst_tail_readback = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vse8.v v8, (%[dst_addr]), v0.t\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[dst0_readback], 0(%[dst_addr])\n\t"
        "ld %[dst1_readback], 8(%[dst_addr])\n\t"
        "ld %[dst_tail_readback], 0(%[dst_tail_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [dst0_readback] "=&r"(dst0_readback),
          [dst1_readback] "=&r"(dst1_readback),
          [dst_tail_readback] "=&r"(dst_tail_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [dst_tail_addr] "r"(dst_tail_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_P14AV_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *dst0_observed = dst0_readback;
    *dst1_observed = dst1_readback;
    *dst_tail_observed = dst_tail_readback;
    *marker_observed = marker_readback;
}

static inline void ai_p14av_cbo_inval_prefetch_zero_all_inactive_vload_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t seed_src_vaddr,
    uintptr_t masked_src_vaddr,
    uintptr_t masked_tail_vaddr,
    uintptr_t capture_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *capture0_observed,
    uint64_t *capture1_observed,
    uint64_t *masked0_observed,
    uint64_t *masked1_observed,
    uint64_t *masked_tail_observed,
    uint64_t *marker_observed)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t capture0_readback = 0xffffffffffffffffULL;
    uint64_t capture1_readback = 0xffffffffffffffffULL;
    uint64_t masked0_readback = 0xffffffffffffffffULL;
    uint64_t masked1_readback = 0xffffffffffffffffULL;
    uint64_t masked_tail_readback = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[seed_src_addr])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vle8.v v8, (%[masked_src_addr]), v0.t\n\t"
        "vse8.v v8, (%[capture_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[capture0_readback], 0(%[capture_addr])\n\t"
        "ld %[capture1_readback], 8(%[capture_addr])\n\t"
        "ld %[masked0_readback], 0(%[masked_src_addr])\n\t"
        "ld %[masked1_readback], 8(%[masked_src_addr])\n\t"
        "ld %[masked_tail_readback], 0(%[masked_tail_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [capture0_readback] "=&r"(capture0_readback),
          [capture1_readback] "=&r"(capture1_readback),
          [masked0_readback] "=&r"(masked0_readback),
          [masked1_readback] "=&r"(masked1_readback),
          [masked_tail_readback] "=&r"(masked_tail_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [masked_tail_addr] "r"(masked_tail_vaddr),
          [capture_addr] "r"(capture_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_P14AV_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *capture0_observed = capture0_readback;
    *capture1_observed = capture1_readback;
    *masked0_observed = masked0_readback;
    *masked1_observed = masked1_readback;
    *masked_tail_observed = masked_tail_readback;
    *marker_observed = marker_readback;
}

static inline void ai_p14av_cbo_inval_prefetch_zero_partial_active_vload_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t seed_src_vaddr,
    uintptr_t masked_src_vaddr,
    uintptr_t masked_tail_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t capture_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *capture0_observed,
    uint64_t *capture1_observed,
    uint64_t *masked0_observed,
    uint64_t *masked1_observed,
    uint64_t *masked_tail_observed,
    uint64_t *marker_observed)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t capture0_readback = 0xffffffffffffffffULL;
    uint64_t capture1_readback = 0xffffffffffffffffULL;
    uint64_t masked0_readback = 0xffffffffffffffffULL;
    uint64_t masked1_readback = 0xffffffffffffffffULL;
    uint64_t masked_tail_readback = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[seed_src_addr])\n\t"
        "vlm.v v0, (%[mask_addr])\n\t"
        "vle8.v v8, (%[masked_src_addr]), v0.t\n\t"
        "vse8.v v8, (%[capture_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[capture0_readback], 0(%[capture_addr])\n\t"
        "ld %[capture1_readback], 8(%[capture_addr])\n\t"
        "ld %[masked0_readback], 0(%[masked_src_addr])\n\t"
        "ld %[masked1_readback], 8(%[masked_src_addr])\n\t"
        "ld %[masked_tail_readback], 0(%[masked_tail_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [capture0_readback] "=&r"(capture0_readback),
          [capture1_readback] "=&r"(capture1_readback),
          [masked0_readback] "=&r"(masked0_readback),
          [masked1_readback] "=&r"(masked1_readback),
          [masked_tail_readback] "=&r"(masked_tail_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [masked_tail_addr] "r"(masked_tail_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [capture_addr] "r"(capture_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_P14AV_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *capture0_observed = capture0_readback;
    *capture1_observed = capture1_readback;
    *masked0_observed = masked0_readback;
    *masked1_observed = masked1_readback;
    *masked_tail_observed = masked_tail_readback;
    *marker_observed = marker_readback;
}

static inline void ai_p14av_cbo_inval_prefetch_zero_partial_active_vstore_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t dst_tail_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *dst0_observed,
    uint64_t *dst1_observed,
    uint64_t *dst_tail_observed,
    uint64_t *marker_observed)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t dst0_readback = 0xffffffffffffffffULL;
    uint64_t dst1_readback = 0xffffffffffffffffULL;
    uint64_t dst_tail_readback = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vlm.v v0, (%[mask_addr])\n\t"
        "vse8.v v8, (%[dst_addr]), v0.t\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[dst0_readback], 0(%[dst_addr])\n\t"
        "ld %[dst1_readback], 8(%[dst_addr])\n\t"
        "ld %[dst_tail_readback], 0(%[dst_tail_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [dst0_readback] "=&r"(dst0_readback),
          [dst1_readback] "=&r"(dst1_readback),
          [dst_tail_readback] "=&r"(dst_tail_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [dst_tail_addr] "r"(dst_tail_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_P14AV_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *dst0_observed = dst0_readback;
    *dst1_observed = dst1_readback;
    *dst_tail_observed = dst_tail_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_normal_vector_no_stale_zero_vector_payload_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x900U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t tail_base_vaddr =
        target_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t adjacent_tail_base_vaddr =
        adjacent_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_base_vaddr =
        target_base_vaddr + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t dst_after_guard_base_vaddr =
        dst_base_vaddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t dst_base_paddr =
        target_base_paddr + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t dst_after_guard_paddr =
        dst_base_paddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef76543210ULL,
        0x3141592653589793ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t dst_fills[] = {
        0xc3c3c3c3c3c3c3c3ULL,
        0x3c3c3c3c3c3c3c3cULL,
        0xa55aa55ac33cc33cULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_after_guards[] = {
        0x0102030405060708ULL,
        0x8070605040302010ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t marker_seeds[] = {
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
        0xddddeeeeffff0000ULL,
    };
    const uint64_t marker_updates[] = {
        0x456789abcdef0123ULL,
        0x3210fedcba987654ULL,
        0x5a5aa5a50badf00dULL,
    };
    const uint64_t marker_guards[] = {
        0x123456789abcdef0ULL,
        0x0badf00d5a5aa5a5ULL,
        0x7766554433221100ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t adjacent_tail_alias_vaddr = 0;
    uintptr_t dst_alias_vaddr = 0;
    uintptr_t dst_after_guard_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p14av_enable_vector_state();
    ai_p14av_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p14av_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr = target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    adjacent_alias_vaddr = target_alias_vaddr + AI_P14AV_BLOCK_BYTES;
    adjacent_tail_alias_vaddr =
        adjacent_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    dst_alias_vaddr = target_alias_vaddr + 2U * AI_P14AV_BLOCK_BYTES;
    dst_after_guard_alias_vaddr =
        dst_alias_vaddr + AI_P14AV_VECTOR_BYTES;
    marker_alias_vaddr = target_alias_vaddr + 3U * AI_P14AV_BLOCK_BYTES;
    marker_guard_alias_vaddr = marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < AI_P14AV_ARRAY_SIZE(target_seeds);
         ++round) {
        uint64_t dst0_observed = 0xffffffffffffffffULL;
        uint64_t dst1_observed = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p14av_seed_block(target_base_paddr, target_seeds[round]);
        ai_p14av_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(dst_base_paddr, dst_fills[round]);
        write64(dst_base_paddr + sizeof(uint64_t), ~dst_fills[round]);
        write64(dst_after_guard_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + normal-vector window starts, base and alias views expose the seeded target, adjacent, destination, and marker values",
            ld(target_base_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(target_alias_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(tail_base_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(tail_alias_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(dst_base_vaddr) == dst_fills[round] &&
            ld(dst_alias_vaddr) == dst_fills[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        CSRW(CSR_VSTART, 0ULL);
        ai_p14av_cbo_inval_prefetch_zero_vector_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            target_alias_vaddr, dst_alias_vaddr, marker_alias_vaddr,
            marker_updates[round], &dst0_observed, &dst1_observed,
            &marker_observed);
        AI_P14AV_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero clear the block before alias-view normal vector load/store copies only zero bytes and marker still progresses",
            dst0_observed == 0ULL &&
            dst1_observed == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(dst_base_vaddr) == 0ULL &&
            ld(dst_alias_vaddr) == 0ULL &&
            ld(dst_base_vaddr + sizeof(uint64_t)) == 0ULL &&
            ld(dst_alias_vaddr + sizeof(uint64_t)) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("the alias zero-to-normal-vector handoff preserves the pre-target guard, adjacent block, destination tail guard, and marker guard through both address views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_tail_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_tail_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(dst_after_guard_base_vaddr) == dst_after_guards[round] &&
            ld(dst_after_guard_alias_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + normal-vector handoff contains a fully zeroed target block, zero vector destination, marker update, and unchanged guards",
            ai_p14av_block_all_zero(target_base_paddr) &&
            ai_p14av_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(dst_base_paddr) == 0ULL &&
            read64(dst_base_paddr + sizeof(uint64_t)) == 0ULL &&
            read64(dst_after_guard_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p14av_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_partial_active_masked_vstore_no_lane_mix_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xe00U;
    const uintptr_t marker_offset =
        target_offset + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t seed_offset =
        target_offset + 3U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t mask_offset =
        target_offset + 4U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t tail_base_vaddr =
        target_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t adjacent_tail_base_vaddr =
        adjacent_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t marker_base_vaddr =
        hs_page_base(base_page) + marker_offset;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t seed_base_vaddr =
        hs_page_base(base_page) + seed_offset;
    const uintptr_t seed_base_paddr =
        phys_page_base(base_page) + seed_offset;
    const uintptr_t seed_guard_base_vaddr =
        seed_base_vaddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t mask_base_vaddr =
        hs_page_base(base_page) + mask_offset;
    const uintptr_t mask_base_paddr =
        phys_page_base(base_page) + mask_offset;
    const uintptr_t mask_guard_base_vaddr =
        mask_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        phys_page_base(base_page) + marker_offset;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uintptr_t seed_guard_paddr =
        seed_base_paddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t mask_guard_paddr =
        mask_base_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x314256789abcdef0ULL,
        0x42536789abcdef01ULL,
        0x5364789abcdef012ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x778899aabbccddeeULL,
        0x8899aabbccddeeffULL,
        0x99aabbccddeeff00ULL,
    };
    static const uint8_t source_patterns[][AI_P14AV_VECTOR_BYTES] = {
        { 0x81, 0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8,
          0x09, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70 },
        { 0x7e, 0x6d, 0x5c, 0x4b, 0x3a, 0x29, 0x18, 0x07,
          0xf6, 0xe5, 0xd4, 0xc3, 0xb2, 0xa1, 0x90, 0x8f },
        { 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe,
          0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01 },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0x5a, 0xa5 },
        { 0xa5, 0x5a },
        { 0x3c, 0xc3 },
    };
    const uint64_t before_guards[] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t marker_seeds[] = {
        0x1111111122222222ULL,
        0x3333333344444444ULL,
        0x5555555566666666ULL,
    };
    const uint64_t marker_updates[] = {
        0xca57000000000001ULL,
        0xca57000000000002ULL,
        0xca57000000000003ULL,
    };
    const uint64_t marker_guards[] = {
        0xaabbccdd00112233ULL,
        0xbbccdd0011223344ULL,
        0xccdd001122334455ULL,
    };
    const uint64_t seed_guards[] = {
        0x1234567890abcdefULL,
        0x234567890abcdef1ULL,
        0x34567890abcdef12ULL,
    };
    const uint64_t mask_guards[] = {
        0x0badf00dcafebeefULL,
        0x1badf00dcafebeefULL,
        0x2badf00dcafebeefULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t adjacent_tail_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;
    uintptr_t seed_alias_vaddr = 0;
    uintptr_t seed_guard_alias_vaddr = 0;
    uintptr_t mask_alias_vaddr = 0;
    uintptr_t mask_guard_alias_vaddr = 0;

    ai_p14av_enable_vector_state();
    ai_p14av_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p14av_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr = target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    adjacent_alias_vaddr = target_alias_vaddr + AI_P14AV_BLOCK_BYTES;
    adjacent_tail_alias_vaddr =
        adjacent_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    marker_alias_vaddr = alias_page_base_vaddr + marker_offset;
    marker_guard_alias_vaddr = marker_alias_vaddr + sizeof(uint64_t);
    seed_alias_vaddr = alias_page_base_vaddr + seed_offset;
    seed_guard_alias_vaddr = seed_alias_vaddr + AI_P14AV_VECTOR_BYTES;
    mask_alias_vaddr = alias_page_base_vaddr + mask_offset;
    mask_guard_alias_vaddr = mask_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < AI_P14AV_ARRAY_SIZE(target_seeds);
         ++round) {
        uint64_t dst0_observed = 0xffffffffffffffffULL;
        uint64_t dst1_observed = 0xffffffffffffffffULL;
        uint64_t dst_tail_observed = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p14av_seed_block(target_base_paddr, target_seeds[round]);
        ai_p14av_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);
        ai_p14av_write_bytes(seed_base_paddr, source_patterns[round],
            AI_P14AV_VECTOR_BYTES);
        write64(seed_guard_paddr, seed_guards[round]);
        write8(mask_base_paddr, mask_patterns[round][0]);
        write8(mask_base_paddr + 1U, mask_patterns[round][1]);
        write64(mask_guard_paddr, mask_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + partial-active masked vector-store window starts, base and alias views expose the seeded destination, source, mask, adjacent, and marker values",
            ld(target_base_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(target_alias_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(tail_base_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(tail_alias_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ai_p14av_bytes_match(seed_base_vaddr, source_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            ai_p14av_bytes_match(seed_alias_vaddr, source_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            read8(mask_base_vaddr) == mask_patterns[round][0] &&
            read8(mask_alias_vaddr) == mask_patterns[round][0] &&
            read8(mask_base_vaddr + 1U) == mask_patterns[round][1] &&
            read8(mask_alias_vaddr + 1U) == mask_patterns[round][1] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        CSRW(CSR_VSTART, 0ULL);
        ai_p14av_cbo_inval_prefetch_zero_partial_active_vstore_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            seed_alias_vaddr, target_alias_vaddr, tail_alias_vaddr,
            mask_alias_vaddr, marker_alias_vaddr, marker_updates[round],
            &dst0_observed, &dst1_observed, &dst_tail_observed,
            &marker_observed);
        AI_P14AV_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero clear the destination before an alias-view partial-active masked vector store writes only active source lanes and marker still progresses",
            ai_p14av_active_source_inactive_zero_matches(
                target_base_vaddr,
                source_patterns[round],
                mask_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            ai_p14av_active_source_inactive_zero_matches(
                target_alias_vaddr,
                source_patterns[round],
                mask_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            dst0_observed == ld(target_alias_vaddr) &&
            dst1_observed == ld(target_alias_vaddr + sizeof(uint64_t)) &&
            dst_tail_observed == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round] &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("the alias zero-to-partial-active-vector-store handoff preserves the pre-target guard, adjacent block, source payload, source guard, mask bytes, mask guard, and marker guard through both address views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_tail_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_tail_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ai_p14av_bytes_match(seed_base_vaddr, source_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            ai_p14av_bytes_match(seed_alias_vaddr, source_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            ld(seed_guard_base_vaddr) == seed_guards[round] &&
            ld(seed_guard_alias_vaddr) == seed_guards[round] &&
            read8(mask_base_vaddr) == mask_patterns[round][0] &&
            read8(mask_alias_vaddr) == mask_patterns[round][0] &&
            read8(mask_base_vaddr + 1U) == mask_patterns[round][1] &&
            read8(mask_alias_vaddr + 1U) == mask_patterns[round][1] &&
            ld(mask_guard_base_vaddr) == mask_guards[round] &&
            ld(mask_guard_alias_vaddr) == mask_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + partial-active masked vector-store handoff contains active source bytes, zero inactive and tail bytes, unchanged source/mask payloads, marker update, and intact guards",
            ai_p14av_active_source_inactive_zero_matches(
                target_base_paddr,
                source_patterns[round],
                mask_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            ai_p14av_block_tail_all_zero(target_base_paddr,
                AI_P14AV_VECTOR_BYTES / sizeof(uint64_t)) &&
            ai_p14av_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            ai_p14av_bytes_match(seed_base_paddr, source_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            read8(mask_base_paddr) == mask_patterns[round][0] &&
            read8(mask_base_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p14av_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_vsegment_no_stale_zero_segment_payload_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xa40U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t tail_base_vaddr =
        target_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t adjacent_tail_base_vaddr =
        adjacent_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_base_vaddr =
        target_base_vaddr + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t dst_after_guard_base_vaddr =
        dst_base_vaddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t dst_base_paddr =
        target_base_paddr + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t dst_after_guard_paddr =
        dst_base_paddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x4d5e6f708192a3b4ULL,
        0x5e6f708192a3b4c5ULL,
        0x6f708192a3b4c5d6ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
        0x7766554433221100ULL,
    };
    const uint64_t dst_fills[] = {
        0x5a5a5a5a5a5a5a5aULL,
        0xa5a5a5a5a5a5a5a5ULL,
        0x3c3c3c3c3c3c3c3cULL,
    };
    const uint64_t before_guards[] = {
        0x1357246813572468ULL,
        0x2468135724681357ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t dst_after_guards[] = {
        0xf0e1d2c3b4a59687ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa2a3a4a5a6a7a8a9ULL,
        0xb2b3b4b5b6b7b8b9ULL,
        0xc2c3c4c5c6c7c8c9ULL,
    };
    const uint64_t marker_updates[] = {
        0x5a5aa5a5c3c33c3cULL,
        0x69966996d2d22d2dULL,
        0x78877887e1e11e1eULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t adjacent_tail_alias_vaddr = 0;
    uintptr_t dst_alias_vaddr = 0;
    uintptr_t dst_after_guard_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p14av_enable_vector_state();
    ai_p14av_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p14av_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr = target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    adjacent_alias_vaddr = target_alias_vaddr + AI_P14AV_BLOCK_BYTES;
    adjacent_tail_alias_vaddr =
        adjacent_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    dst_alias_vaddr = target_alias_vaddr + 2U * AI_P14AV_BLOCK_BYTES;
    dst_after_guard_alias_vaddr =
        dst_alias_vaddr + AI_P14AV_VECTOR_BYTES;
    marker_alias_vaddr = target_alias_vaddr + 3U * AI_P14AV_BLOCK_BYTES;
    marker_guard_alias_vaddr = marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < AI_P14AV_ARRAY_SIZE(target_seeds);
         ++round) {
        uint64_t dst0_observed = 0xffffffffffffffffULL;
        uint64_t dst1_observed = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p14av_seed_block(target_base_paddr, target_seeds[round]);
        ai_p14av_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(dst_base_paddr, dst_fills[round]);
        write64(dst_base_paddr + sizeof(uint64_t), ~dst_fills[round]);
        write64(dst_after_guard_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + VSegment window starts, base and alias views expose the seeded target, adjacent, destination, and marker values",
            ld(target_base_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(target_alias_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(tail_base_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(tail_alias_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(dst_base_vaddr) == dst_fills[round] &&
            ld(dst_alias_vaddr) == dst_fills[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        CSRW(CSR_VSTART, 0ULL);
        ai_p14av_cbo_inval_prefetch_zero_vsegment_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            target_alias_vaddr, dst_alias_vaddr, marker_alias_vaddr,
            marker_updates[round], &dst0_observed, &dst1_observed,
            &marker_observed);
        AI_P14AV_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero clear the block before alias-view VSegment load/store copies only zero segment payload and marker still progresses",
            dst0_observed == 0ULL &&
            dst1_observed == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(dst_base_vaddr) == 0ULL &&
            ld(dst_alias_vaddr) == 0ULL &&
            ld(dst_base_vaddr + sizeof(uint64_t)) == 0ULL &&
            ld(dst_alias_vaddr + sizeof(uint64_t)) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("the alias zero-to-VSegment handoff preserves the pre-target guard, adjacent block, destination tail guard, and marker guard through both address views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_tail_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_tail_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(dst_after_guard_base_vaddr) == dst_after_guards[round] &&
            ld(dst_after_guard_alias_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + VSegment handoff contains a fully zeroed target block, zero segment destination, marker update, and unchanged guards",
            ai_p14av_block_all_zero(target_base_paddr) &&
            ai_p14av_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(dst_base_paddr) == 0ULL &&
            read64(dst_base_paddr + sizeof(uint64_t)) == 0ULL &&
            read64(dst_after_guard_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p14av_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_vfof_no_stale_zero_fof_payload_or_vl_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xb80U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t tail_base_vaddr =
        target_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t adjacent_tail_base_vaddr =
        adjacent_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_base_vaddr =
        target_base_vaddr + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t dst_after_guard_base_vaddr =
        dst_base_vaddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t dst_base_paddr =
        target_base_paddr + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t dst_after_guard_paddr =
        dst_base_paddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x8192a3b4c5d6e7f0ULL,
        0x708192a3b4c5d6e7ULL,
        0x5f6e7d8c9ba0b1c2ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x1122334455667788ULL,
    };
    const uint64_t dst_fills[] = {
        0x9696969696969696ULL,
        0x6969696969696969ULL,
        0xc3c33c3ca5a55a5aULL,
    };
    const uint64_t before_guards[] = {
        0x1029384756a5b4c3ULL,
        0xa5b4c31029384756ULL,
        0x55aa33cc7799bbddULL,
    };
    const uint64_t dst_after_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0xf0e1d2c3b4a59687ULL,
        0x3141592653589793ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0xabcdef0123456789ULL,
        0x9876543210fedcbaULL,
        0x1123581321345589ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t adjacent_tail_alias_vaddr = 0;
    uintptr_t dst_alias_vaddr = 0;
    uintptr_t dst_after_guard_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p14av_enable_vector_state();
    ai_p14av_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p14av_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr = target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    adjacent_alias_vaddr = target_alias_vaddr + AI_P14AV_BLOCK_BYTES;
    adjacent_tail_alias_vaddr =
        adjacent_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    dst_alias_vaddr = target_alias_vaddr + 2U * AI_P14AV_BLOCK_BYTES;
    dst_after_guard_alias_vaddr =
        dst_alias_vaddr + AI_P14AV_VECTOR_BYTES;
    marker_alias_vaddr = target_alias_vaddr + 3U * AI_P14AV_BLOCK_BYTES;
    marker_guard_alias_vaddr = marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < AI_P14AV_ARRAY_SIZE(target_seeds);
         ++round) {
        unsigned observed_vl = 0U;
        uint64_t dst0_observed = 0xffffffffffffffffULL;
        uint64_t dst1_observed = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p14av_seed_block(target_base_paddr, target_seeds[round]);
        ai_p14av_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(dst_base_paddr, dst_fills[round]);
        write64(dst_base_paddr + sizeof(uint64_t), ~dst_fills[round]);
        write64(dst_after_guard_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + VFOF window starts, base and alias views expose the seeded target, adjacent, destination, and marker values",
            ld(target_base_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(target_alias_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(tail_base_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(tail_alias_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(dst_base_vaddr) == dst_fills[round] &&
            ld(dst_alias_vaddr) == dst_fills[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        CSRW(CSR_VSTART, 0ULL);
        ai_p14av_cbo_inval_prefetch_zero_vfof_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            target_alias_vaddr, dst_alias_vaddr, marker_alias_vaddr,
            marker_updates[round], &observed_vl, &dst0_observed,
            &dst1_observed, &marker_observed);
        AI_P14AV_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero clear the block before alias-view VFOF reports full vl, stores only zero payload, and marker still progresses",
            observed_vl == AI_P14AV_VECTOR_BYTES &&
            dst0_observed == 0ULL &&
            dst1_observed == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(dst_base_vaddr) == 0ULL &&
            ld(dst_alias_vaddr) == 0ULL &&
            ld(dst_base_vaddr + sizeof(uint64_t)) == 0ULL &&
            ld(dst_alias_vaddr + sizeof(uint64_t)) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("the alias zero-to-VFOF handoff preserves the pre-target guard, adjacent block, destination tail guard, and marker guard through both address views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_tail_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_tail_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(dst_after_guard_base_vaddr) == dst_after_guards[round] &&
            ld(dst_after_guard_alias_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + VFOF handoff contains a fully zeroed target block, zero VFOF destination, marker update, and unchanged guards",
            ai_p14av_block_all_zero(target_base_paddr) &&
            ai_p14av_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(dst_base_paddr) == 0ULL &&
            read64(dst_base_paddr + sizeof(uint64_t)) == 0ULL &&
            read64(dst_after_guard_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p14av_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_all_inactive_masked_vstore_no_source_payload_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xcc0U;
    const uintptr_t src_offset = target_offset + 4U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t tail_base_vaddr =
        target_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t adjacent_tail_base_vaddr =
        adjacent_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_base_vaddr = hs_page_base(base_page) + src_offset;
    const uintptr_t src_base_paddr = phys_page_base(base_page) + src_offset;
    const uintptr_t src_guard_base_vaddr =
        src_base_vaddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uintptr_t src_guard_paddr =
        src_base_paddr + AI_P14AV_VECTOR_BYTES;
    const uint64_t target_seeds[] = {
        0x6a7b8c9daeafb0c1ULL,
        0x7b8c9daeafb0c1d2ULL,
        0x8c9daeafb0c1d2e3ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xcafebabedeadbeefULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t src_word0_values[] = {
        0x0011223344556677ULL,
        0x8899aabbccddeeffULL,
        0x1123581321345589ULL,
    };
    const uint64_t src_word1_values[] = {
        0x7766554433221100ULL,
        0xffeeddccbbaa9988ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x4567012300000001ULL,
        0x4567012300000002ULL,
        0x4567012300000003ULL,
    };
    const uint64_t marker_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t src_guards[] = {
        0xf0e1d2c3b4a59687ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x3141592653589793ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t adjacent_tail_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;
    uintptr_t src_alias_vaddr = 0;
    uintptr_t src_guard_alias_vaddr = 0;

    ai_p14av_enable_vector_state();
    ai_p14av_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p14av_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr = target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    adjacent_alias_vaddr = target_alias_vaddr + AI_P14AV_BLOCK_BYTES;
    adjacent_tail_alias_vaddr =
        adjacent_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    marker_alias_vaddr = target_alias_vaddr + 2U * AI_P14AV_BLOCK_BYTES;
    marker_guard_alias_vaddr = marker_alias_vaddr + sizeof(uint64_t);
    src_alias_vaddr = alias_page_base_vaddr + src_offset;
    src_guard_alias_vaddr = src_alias_vaddr + AI_P14AV_VECTOR_BYTES;

    for (size_t round = 0; round < AI_P14AV_ARRAY_SIZE(target_seeds);
         ++round) {
        uint64_t dst0_observed = 0xffffffffffffffffULL;
        uint64_t dst1_observed = 0xffffffffffffffffULL;
        uint64_t dst_tail_observed = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p14av_seed_block(target_base_paddr, target_seeds[round]);
        ai_p14av_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);
        write64(src_base_paddr, src_word0_values[round]);
        write64(src_base_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + all-inactive masked vector-store window starts, base and alias views expose the seeded destination, source, adjacent, and marker values",
            ld(target_base_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(target_alias_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(tail_base_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(tail_alias_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(src_base_vaddr) == src_word0_values[round] &&
            ld(src_alias_vaddr) == src_word0_values[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        CSRW(CSR_VSTART, 0ULL);
        ai_p14av_cbo_inval_prefetch_zero_all_inactive_vstore_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            src_alias_vaddr, target_alias_vaddr, tail_alias_vaddr,
            marker_alias_vaddr, marker_updates[round], &dst0_observed,
            &dst1_observed, &dst_tail_observed, &marker_observed);
        AI_P14AV_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero clear the destination block before an alias-view all-inactive masked vector store retires without source payload side effect and marker still progresses",
            dst0_observed == 0ULL &&
            dst1_observed == 0ULL &&
            dst_tail_observed == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round] &&
            ld(src_base_vaddr) == src_word0_values[round] &&
            ld(src_alias_vaddr) == src_word0_values[round] &&
            ld(src_base_vaddr + sizeof(uint64_t)) == src_word1_values[round] &&
            ld(src_alias_vaddr + sizeof(uint64_t)) == src_word1_values[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("the alias zero-to-all-inactive-vector-store handoff preserves the pre-target guard, adjacent block, source guard, and marker guard through both address views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_tail_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_tail_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(src_guard_base_vaddr) == src_guards[round] &&
            ld(src_guard_alias_vaddr) == src_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + all-inactive masked vector-store handoff contains a fully zeroed destination block, unchanged source payload, marker update, and intact guards",
            ai_p14av_block_all_zero(target_base_paddr) &&
            ai_p14av_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(src_base_paddr) == src_word0_values[round] &&
            read64(src_base_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p14av_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_all_inactive_masked_vload_no_dest_clobber_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xd80U;
    const uintptr_t capture_offset =
        target_offset + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_offset =
        target_offset + 3U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t seed_offset =
        target_offset + 4U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t tail_base_vaddr =
        target_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t adjacent_tail_base_vaddr =
        adjacent_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t capture_base_vaddr =
        hs_page_base(base_page) + capture_offset;
    const uintptr_t capture_base_paddr =
        phys_page_base(base_page) + capture_offset;
    const uintptr_t capture_guard_base_vaddr =
        capture_base_vaddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_base_vaddr =
        hs_page_base(base_page) + marker_offset;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t seed_base_vaddr =
        hs_page_base(base_page) + seed_offset;
    const uintptr_t seed_base_paddr =
        phys_page_base(base_page) + seed_offset;
    const uintptr_t seed_guard_base_vaddr =
        seed_base_vaddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t capture_guard_paddr =
        capture_base_paddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_paddr =
        phys_page_base(base_page) + marker_offset;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uintptr_t seed_guard_paddr =
        seed_base_paddr + AI_P14AV_VECTOR_BYTES;
    const uint64_t target_seeds[] = {
        0x9daeafb0c1d2e3f4ULL,
        0xaebfc0d1e2f30415ULL,
        0xbfc0d1e2f3041526ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x1029384756a5b4c3ULL,
        0x2233445566778899ULL,
        0x8877665544332211ULL,
    };
    const uint64_t seed_word0_values[] = {
        0x0011223344556677ULL,
        0x8899aabbccddeeffULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t seed_word1_values[] = {
        0x7766554433221100ULL,
        0xffeeddccbbaa9988ULL,
        0x0eca8642fdb97531ULL,
    };
    const uint64_t capture_fills[] = {
        0xc3c3c3c3c3c3c3c3ULL,
        0x3c3c3c3c3c3c3c3cULL,
        0xa55aa55ac33cc33cULL,
    };
    const uint64_t before_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t capture_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0x1029384756a5b4c3ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t marker_seeds[] = {
        0xd0d1d2d3d4d5d6d7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xf0f1f2f3f4f5f6f7ULL,
    };
    const uint64_t marker_updates[] = {
        0x4567012400000001ULL,
        0x4567012400000002ULL,
        0x4567012400000003ULL,
    };
    const uint64_t marker_guards[] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t seed_guards[] = {
        0xf0e1d2c3b4a59687ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x3141592653589793ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t adjacent_tail_alias_vaddr = 0;
    uintptr_t capture_alias_vaddr = 0;
    uintptr_t capture_guard_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;
    uintptr_t seed_alias_vaddr = 0;
    uintptr_t seed_guard_alias_vaddr = 0;

    ai_p14av_enable_vector_state();
    ai_p14av_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p14av_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr = target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    adjacent_alias_vaddr = target_alias_vaddr + AI_P14AV_BLOCK_BYTES;
    adjacent_tail_alias_vaddr =
        adjacent_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    capture_alias_vaddr = alias_page_base_vaddr + capture_offset;
    capture_guard_alias_vaddr =
        capture_alias_vaddr + AI_P14AV_VECTOR_BYTES;
    marker_alias_vaddr = alias_page_base_vaddr + marker_offset;
    marker_guard_alias_vaddr = marker_alias_vaddr + sizeof(uint64_t);
    seed_alias_vaddr = alias_page_base_vaddr + seed_offset;
    seed_guard_alias_vaddr = seed_alias_vaddr + AI_P14AV_VECTOR_BYTES;

    for (size_t round = 0; round < AI_P14AV_ARRAY_SIZE(target_seeds);
         ++round) {
        uint64_t capture0_observed = 0xffffffffffffffffULL;
        uint64_t capture1_observed = 0xffffffffffffffffULL;
        uint64_t masked0_observed = 0xffffffffffffffffULL;
        uint64_t masked1_observed = 0xffffffffffffffffULL;
        uint64_t masked_tail_observed = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p14av_seed_block(target_base_paddr, target_seeds[round]);
        ai_p14av_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(capture_base_paddr, capture_fills[round]);
        write64(capture_base_paddr + sizeof(uint64_t),
            ~capture_fills[round]);
        write64(capture_guard_paddr, capture_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);
        write64(seed_base_paddr, seed_word0_values[round]);
        write64(seed_base_paddr + sizeof(uint64_t),
            seed_word1_values[round]);
        write64(seed_guard_paddr, seed_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + all-inactive masked vector-load window starts, base and alias views expose the seeded masked source, capture destination, seed source, adjacent, and marker values",
            ld(target_base_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(target_alias_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(tail_base_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(tail_alias_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(capture_base_vaddr) == capture_fills[round] &&
            ld(capture_alias_vaddr) == capture_fills[round] &&
            ld(seed_base_vaddr) == seed_word0_values[round] &&
            ld(seed_alias_vaddr) == seed_word0_values[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        CSRW(CSR_VSTART, 0ULL);
        ai_p14av_cbo_inval_prefetch_zero_all_inactive_vload_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            seed_alias_vaddr, target_alias_vaddr, tail_alias_vaddr,
            capture_alias_vaddr, marker_alias_vaddr,
            marker_updates[round], &capture0_observed,
            &capture1_observed, &masked0_observed, &masked1_observed,
            &masked_tail_observed, &marker_observed);
        AI_P14AV_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero clear the masked source block before an alias-view all-inactive masked vector load retires without clobbering the preloaded vector payload and marker still progresses",
            capture0_observed == seed_word0_values[round] &&
            capture1_observed == seed_word1_values[round] &&
            masked0_observed == 0ULL &&
            masked1_observed == 0ULL &&
            masked_tail_observed == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(capture_base_vaddr) == seed_word0_values[round] &&
            ld(capture_alias_vaddr) == seed_word0_values[round] &&
            ld(capture_base_vaddr + sizeof(uint64_t)) ==
                seed_word1_values[round] &&
            ld(capture_alias_vaddr + sizeof(uint64_t)) ==
                seed_word1_values[round] &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("the alias zero-to-all-inactive-vector-load handoff preserves the pre-target guard, adjacent block, capture guard, seed source, seed guard, and marker guard through both address views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_tail_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_tail_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(capture_guard_base_vaddr) == capture_guards[round] &&
            ld(capture_guard_alias_vaddr) == capture_guards[round] &&
            ld(seed_base_vaddr) == seed_word0_values[round] &&
            ld(seed_alias_vaddr) == seed_word0_values[round] &&
            ld(seed_base_vaddr + sizeof(uint64_t)) ==
                seed_word1_values[round] &&
            ld(seed_alias_vaddr + sizeof(uint64_t)) ==
                seed_word1_values[round] &&
            ld(seed_guard_base_vaddr) == seed_guards[round] &&
            ld(seed_guard_alias_vaddr) == seed_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + all-inactive masked vector-load handoff contains a fully zeroed masked source block, captured preloaded vector payload, unchanged seed source, marker update, and intact guards",
            ai_p14av_block_all_zero(target_base_paddr) &&
            ai_p14av_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(capture_base_paddr) == seed_word0_values[round] &&
            read64(capture_base_paddr + sizeof(uint64_t)) ==
                seed_word1_values[round] &&
            read64(capture_guard_paddr) == capture_guards[round] &&
            read64(seed_base_paddr) == seed_word0_values[round] &&
            read64(seed_base_paddr + sizeof(uint64_t)) ==
                seed_word1_values[round] &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p14av_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_partial_active_masked_vload_no_lane_mix_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xe40U;
    const uintptr_t capture_offset =
        target_offset + 2U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t marker_offset =
        target_offset + 3U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t seed_offset =
        target_offset + 4U * AI_P14AV_BLOCK_BYTES;
    const uintptr_t mask_offset =
        target_offset + 4U * AI_P14AV_BLOCK_BYTES + 0x40U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t tail_base_vaddr =
        target_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t adjacent_tail_base_vaddr =
        adjacent_base_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t capture_base_vaddr =
        hs_page_base(base_page) + capture_offset;
    const uintptr_t capture_base_paddr =
        phys_page_base(base_page) + capture_offset;
    const uintptr_t capture_guard_base_vaddr =
        capture_base_vaddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_base_vaddr =
        hs_page_base(base_page) + marker_offset;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t seed_base_vaddr =
        hs_page_base(base_page) + seed_offset;
    const uintptr_t seed_base_paddr =
        phys_page_base(base_page) + seed_offset;
    const uintptr_t seed_guard_base_vaddr =
        seed_base_vaddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t mask_base_vaddr =
        hs_page_base(base_page) + mask_offset;
    const uintptr_t mask_base_paddr =
        phys_page_base(base_page) + mask_offset;
    const uintptr_t mask_guard_base_vaddr =
        mask_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P14AV_BLOCK_BYTES;
    const uintptr_t capture_guard_paddr =
        capture_base_paddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t marker_paddr =
        phys_page_base(base_page) + marker_offset;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uintptr_t seed_guard_paddr =
        seed_base_paddr + AI_P14AV_VECTOR_BYTES;
    const uintptr_t mask_guard_paddr =
        mask_base_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0xc0d1e2f304152637ULL,
        0xd1e2f30415263748ULL,
        0xe2f3041526374859ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x4567765445677654ULL,
        0x5678876556788765ULL,
        0x6789987667899876ULL,
    };
    static const uint8_t seed_patterns[][AI_P14AV_VECTOR_BYTES] = {
        { 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0, 0xc1,
          0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38, 0x49 },
        { 0xa5, 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e,
          0x2d, 0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6 },
        { 0x09, 0x18, 0x27, 0x36, 0x45, 0x54, 0x63, 0x72,
          0x81, 0x90, 0xaf, 0xbe, 0xcd, 0xdc, 0xeb, 0xf0 },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0x55, 0xaa },
        { 0xaa, 0x55 },
        { 0xc3, 0x3c },
    };
    const uint64_t capture_fills[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t before_guards[] = {
        0x13572468ace0bdf9ULL,
        0x24681357bdf9ace0ULL,
        0x35792468ce0abdf1ULL,
    };
    const uint64_t capture_guards[] = {
        0xc001c0de55aa33ccULL,
        0xd112d1ef66bb44ddULL,
        0xe223e2f077cc55eeULL,
    };
    const uint64_t marker_seeds[] = {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
        0x0303030303030303ULL,
    };
    const uint64_t marker_updates[] = {
        0xca56000000000001ULL,
        0xca56000000000002ULL,
        0xca56000000000003ULL,
    };
    const uint64_t marker_guards[] = {
        0xfeeddadac0020001ULL,
        0xfeeddadac0020002ULL,
        0xfeeddadac0020003ULL,
    };
    const uint64_t seed_guards[] = {
        0x445566778899aabbULL,
        0x5566778899aabbccULL,
        0x66778899aabbccddULL,
    };
    const uint64_t mask_guards[] = {
        0xabcdef9876543210ULL,
        0xbcdefa8765432101ULL,
        0xcdefab7654321012ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t adjacent_tail_alias_vaddr = 0;
    uintptr_t capture_alias_vaddr = 0;
    uintptr_t capture_guard_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;
    uintptr_t seed_alias_vaddr = 0;
    uintptr_t seed_guard_alias_vaddr = 0;
    uintptr_t mask_alias_vaddr = 0;
    uintptr_t mask_guard_alias_vaddr = 0;

    ai_p14av_enable_vector_state();
    ai_p14av_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p14av_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr = target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    adjacent_alias_vaddr = target_alias_vaddr + AI_P14AV_BLOCK_BYTES;
    adjacent_tail_alias_vaddr =
        adjacent_alias_vaddr +
        (AI_P14AV_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    capture_alias_vaddr = alias_page_base_vaddr + capture_offset;
    capture_guard_alias_vaddr =
        capture_alias_vaddr + AI_P14AV_VECTOR_BYTES;
    marker_alias_vaddr = alias_page_base_vaddr + marker_offset;
    marker_guard_alias_vaddr = marker_alias_vaddr + sizeof(uint64_t);
    seed_alias_vaddr = alias_page_base_vaddr + seed_offset;
    seed_guard_alias_vaddr = seed_alias_vaddr + AI_P14AV_VECTOR_BYTES;
    mask_alias_vaddr = alias_page_base_vaddr + mask_offset;
    mask_guard_alias_vaddr = mask_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < AI_P14AV_ARRAY_SIZE(target_seeds);
         ++round) {
        uint64_t capture0_observed = 0xffffffffffffffffULL;
        uint64_t capture1_observed = 0xffffffffffffffffULL;
        uint64_t masked0_observed = 0xffffffffffffffffULL;
        uint64_t masked1_observed = 0xffffffffffffffffULL;
        uint64_t masked_tail_observed = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p14av_seed_block(target_base_paddr, target_seeds[round]);
        ai_p14av_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(capture_base_paddr, capture_fills[round]);
        write64(capture_base_paddr + sizeof(uint64_t),
            ~capture_fills[round]);
        write64(capture_guard_paddr, capture_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);
        ai_p14av_write_bytes(seed_base_paddr, seed_patterns[round],
            AI_P14AV_VECTOR_BYTES);
        write64(seed_guard_paddr, seed_guards[round]);
        write8(mask_base_paddr, mask_patterns[round][0]);
        write8(mask_base_paddr + 1U, mask_patterns[round][1]);
        write64(mask_guard_paddr, mask_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + partial-active masked vector-load window starts, base and alias views expose the seeded masked source, capture destination, seed source, mask, adjacent, and marker values",
            ld(target_base_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(target_alias_vaddr) == ai_p14av_seed_word(target_seeds[round], 0U) &&
            ld(tail_base_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(tail_alias_vaddr) == ai_p14av_seed_word(target_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(capture_base_vaddr) == capture_fills[round] &&
            ld(capture_alias_vaddr) == capture_fills[round] &&
            ai_p14av_bytes_match(seed_base_vaddr, seed_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            ai_p14av_bytes_match(seed_alias_vaddr, seed_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            read8(mask_base_vaddr) == mask_patterns[round][0] &&
            read8(mask_alias_vaddr) == mask_patterns[round][0] &&
            read8(mask_base_vaddr + 1U) == mask_patterns[round][1] &&
            read8(mask_alias_vaddr + 1U) == mask_patterns[round][1] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        CSRW(CSR_VSTART, 0ULL);
        ai_p14av_cbo_inval_prefetch_zero_partial_active_vload_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            seed_alias_vaddr, target_alias_vaddr, tail_alias_vaddr,
            mask_alias_vaddr, capture_alias_vaddr, marker_alias_vaddr,
            marker_updates[round], &capture0_observed,
            &capture1_observed, &masked0_observed, &masked1_observed,
            &masked_tail_observed, &marker_observed);
        AI_P14AV_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero clear the masked source block before an alias-view partial-active masked vector load captures zero active lanes, seed inactive lanes, and marker progress",
            ai_p14av_zero_active_seed_inactive_capture_matches(
                capture_base_vaddr,
                seed_patterns[round],
                mask_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            ai_p14av_zero_active_seed_inactive_capture_matches(
                capture_alias_vaddr,
                seed_patterns[round],
                mask_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            capture0_observed == ld(capture_alias_vaddr) &&
            capture1_observed == ld(capture_alias_vaddr + sizeof(uint64_t)) &&
            masked0_observed == 0ULL &&
            masked1_observed == 0ULL &&
            masked_tail_observed == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P14AV_ASSERT_NO_EXCEPT_AND("the alias zero-to-partial-active-vector-load handoff preserves the pre-target guard, adjacent block, capture guard, seed source, seed guard, mask bytes, mask guard, and marker guard through both address views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(adjacent_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round], 0U) &&
            ld(adjacent_tail_base_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(adjacent_tail_alias_vaddr) == ai_p14av_seed_word(adjacent_seeds[round],
                AI_P14AV_BLOCK_WORDS - 1U) &&
            ld(capture_guard_base_vaddr) == capture_guards[round] &&
            ld(capture_guard_alias_vaddr) == capture_guards[round] &&
            ai_p14av_bytes_match(seed_base_vaddr, seed_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            ai_p14av_bytes_match(seed_alias_vaddr, seed_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            ld(seed_guard_base_vaddr) == seed_guards[round] &&
            ld(seed_guard_alias_vaddr) == seed_guards[round] &&
            read8(mask_base_vaddr) == mask_patterns[round][0] &&
            read8(mask_alias_vaddr) == mask_patterns[round][0] &&
            read8(mask_base_vaddr + 1U) == mask_patterns[round][1] &&
            read8(mask_alias_vaddr + 1U) == mask_patterns[round][1] &&
            ld(mask_guard_base_vaddr) == mask_guards[round] &&
            ld(mask_guard_alias_vaddr) == mask_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + base prefetch_r + alias cbo.zero + partial-active masked vector-load handoff contains a fully zeroed masked source block, zero-active/seed-inactive capture bytes, unchanged seed and mask payloads, marker update, and intact guards",
            ai_p14av_block_all_zero(target_base_paddr) &&
            ai_p14av_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            ai_p14av_zero_active_seed_inactive_capture_matches(
                capture_base_paddr,
                seed_patterns[round],
                mask_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            read64(capture_guard_paddr) == capture_guards[round] &&
            ai_p14av_bytes_match(seed_base_paddr, seed_patterns[round],
                AI_P14AV_VECTOR_BYTES) &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            read8(mask_base_paddr) == mask_patterns[round][0] &&
            read8(mask_base_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p14av_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}
