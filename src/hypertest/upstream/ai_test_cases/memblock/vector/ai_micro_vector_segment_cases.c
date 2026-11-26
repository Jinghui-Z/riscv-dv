#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_MICRO_SEGMENT_FIELDS 2
#define AI_MICRO_SEGMENT_VL     8
#define AI_MICRO_SEGMENT_BYTES  (AI_MICRO_SEGMENT_FIELDS * AI_MICRO_SEGMENT_VL)
#define AI_MICRO_VECTOR_BYTES   16
#define AI_MICRO_SCALAR_BYTES   8U
#define AI_MICRO_SEGMENT_BURST_COUNT 9U
#define AI_MICRO_MIXED_MEMOP_WINDOW_COUNT 6U
#define AI_MICRO_MIXED_VECTOR_STEP        0x20U
#define AI_MICRO_MIXED_SCALAR_STEP        0x10U

static inline void ai_micro_segment_write_bytes(uintptr_t addr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_segment_bytes_match(uintptr_t addr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline bool ai_micro_segment_prefix_and_seed_tail_match(uintptr_t addr,
    const uint8_t *active_bytes,
    unsigned active_vl,
    const uint8_t *seed_bytes)
{
    const unsigned active_len = active_vl * AI_MICRO_SEGMENT_FIELDS;

    for (unsigned i = 0; i < AI_MICRO_SEGMENT_BYTES; ++i) {
        const uint8_t expected = (i < active_len) ? active_bytes[i] :
            seed_bytes[i];
        if (read8(addr + i) != expected) {
            return false;
        }
    }
    return true;
}

static inline uint64_t ai_micro_pack_u64_le(const uint8_t *bytes)
{
    uint64_t value = 0;

    for (unsigned i = 0; i < 8U; ++i) {
        value |= ((uint64_t)bytes[i]) << (i * 8U);
    }

    return value;
}

static inline void ai_micro_vlseg2e8_to_v8(const uint8_t *src, int vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %1, %2\n\t"
        "vlseg2e8.v v8, (%0)\n\t"
        ".option pop\n\t"
        :
        : "r"(src), "r"(vl), "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_vsseg2e8_from_v8(uint8_t *dst, int vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %1, %2\n\t"
        "vsseg2e8.v v8, (%0)\n\t"
        ".option pop\n\t"
        : "+r"(dst)
        : "r"(vl), "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_vsseg2e8_pair_from_v8(uint8_t *dst0, int vl0,
    uint8_t *dst1, int vl1)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl0], %[vtype]\n\t"
        "vsseg2e8.v v8, (%[dst0])\n\t"
        "vsetvl t0, %[vl1], %[vtype]\n\t"
        "vsseg2e8.v v8, (%[dst1])\n\t"
        ".option pop\n\t"
        :
        : [dst0] "r"(dst0), [vl0] "r"(vl0),
          [dst1] "r"(dst1), [vl1] "r"(vl1),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_vsseg2e8_masked_from_v8(uint8_t *dst, int vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %1, %2\n\t"
        "vmv.v.i v0, 0\n\t"
        "vsseg2e8.v v8, (%0), v0.t\n\t"
        ".option pop\n\t"
        : "+r"(dst)
        : "r"(vl), "r"(vtype)
        : "t0", "memory");
}

static bool ai_micro_segment_vsmerge_pair(bool vector_first, bool repeat_rounds)
{
    static const uint8_t seg_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x10, 0x80, 0x11, 0x81, 0x12, 0x82, 0x13, 0x83,
          0x14, 0x84, 0x15, 0x85, 0x16, 0x86, 0x17, 0x87 },
        { 0x21, 0x91, 0x22, 0x92, 0x23, 0x93, 0x24, 0x94,
          0x25, 0x95, 0x26, 0x96, 0x27, 0x97, 0x28, 0x98 },
        { 0x32, 0xa2, 0x33, 0xa3, 0x34, 0xa4, 0x35, 0xa5,
          0x36, 0xa6, 0x37, 0xa7, 0x38, 0xa8, 0x39, 0xa9 },
    };
    static const uint8_t vec_patterns[][AI_MICRO_VECTOR_BYTES] = {
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87,
          0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
        { 0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
          0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0 },
        { 0x55, 0xaa, 0x44, 0xbb, 0x33, 0xcc, 0x22, 0xdd,
          0x11, 0xee, 0x00, 0xff, 0x99, 0x66, 0x88, 0x77 },
    };
    const uintptr_t seg_src_vaddr = hs_page_base(SWITCH1) + 0x100U;
    const uintptr_t seg_dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    const uintptr_t vec_src_vaddr = hs_page_base(SWITCH2) + 0x200U;
    const uintptr_t vec_dst_vaddr = hs_page_base(SCRATCHPAD) + 0x280U;
    const uintptr_t seg_src_paddr = phys_page_base(SWITCH1) + 0x100U;
    const uintptr_t seg_dst_paddr = phys_page_base(VSRWX_GRWX) + 0x180U;
    const uintptr_t vec_src_paddr = phys_page_base(SWITCH2) + 0x200U;
    const uintptr_t vec_dst_paddr = phys_page_base(SCRATCHPAD) + 0x280U;
    const uintptr_t seg_before = seg_dst_paddr - 1U;
    const uintptr_t seg_after = seg_dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uintptr_t vec_before = vec_dst_paddr - 1U;
    const uintptr_t vec_after = vec_dst_paddr + AI_MICRO_VECTOR_BYTES;
    const uint8_t seg_before_sentinel = 0x5aU;
    const uint8_t seg_after_sentinel = 0xa5U;
    const uint8_t vec_before_sentinel = 0x3cU;
    const uint8_t vec_after_sentinel = 0xc3U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(seg_patterns) / sizeof(seg_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(seg_src_paddr, seg_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(vec_src_paddr, vec_patterns[round],
            AI_MICRO_VECTOR_BYTES);
        ai_micro_segment_write_bytes(seg_dst_paddr, vec_patterns[0],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(vec_dst_paddr, seg_patterns[0],
            AI_MICRO_VECTOR_BYTES);
        write8(seg_before, seg_before_sentinel);
        write8(seg_after, seg_after_sentinel);
        write8(vec_before, vec_before_sentinel);
        write8(vec_after, vec_after_sentinel);

        goto_priv(PRIV_HS);
        if (vector_first) {
            vle8_to_v6((const uint8_t *)vec_src_vaddr, AI_MICRO_VECTOR_BYTES);
            vse8_from_v6((uint8_t *)vec_dst_vaddr, AI_MICRO_VECTOR_BYTES);
            ai_micro_vlseg2e8_to_v8((const uint8_t *)seg_src_vaddr,
                AI_MICRO_SEGMENT_VL);
            ai_micro_vsseg2e8_from_v8((uint8_t *)seg_dst_vaddr,
                AI_MICRO_SEGMENT_VL);
        } else {
            ai_micro_vlseg2e8_to_v8((const uint8_t *)seg_src_vaddr,
                AI_MICRO_SEGMENT_VL);
            ai_micro_vsseg2e8_from_v8((uint8_t *)seg_dst_vaddr,
                AI_MICRO_SEGMENT_VL);
            vle8_to_v6((const uint8_t *)vec_src_vaddr, AI_MICRO_VECTOR_BYTES);
            vse8_from_v6((uint8_t *)vec_dst_vaddr, AI_MICRO_VECTOR_BYTES);
        }

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the back-to-back segment-store and normal vector-store issue window retires without exception before integrity checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the segment-store target still receives exactly the intended interleaved payload while the regular vector-store target receives only its own contiguous payload",
            ai_micro_segment_bytes_match(seg_dst_paddr, seg_patterns[round],
                AI_MICRO_SEGMENT_BYTES) &&
            ai_micro_segment_bytes_match(vec_dst_paddr, vec_patterns[round],
                AI_MICRO_VECTOR_BYTES)
        );
        TEST_ASSERT("the segment-store and regular vector-store pair also preserves adjacent sentinel bytes around both destinations",
            read8(seg_before) == seg_before_sentinel &&
            read8(seg_after) == seg_after_sentinel &&
            read8(vec_before) == vec_before_sentinel &&
            read8(vec_after) == vec_after_sentinel
        );

        vector_first = !vector_first;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_and_vsmerge_feedback_port0_same_cycle_no_mux1h_corruption_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_vsmerge_pair(false, false);
    TEST_END();
}

bool ai_micro_hs_vector_store_feedback_priority_segment_vs_merge_is_deterministic_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_vsmerge_pair(true, true);
    TEST_END();
}

static bool ai_micro_segment_scalar_sbuffer_pair(bool vector_first,
    bool same_cache_line,
    bool repeat_rounds)
{
    static const uint8_t seg_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x18, 0x81, 0x19, 0x82, 0x1a, 0x83, 0x1b, 0x84,
          0x1c, 0x85, 0x1d, 0x86, 0x1e, 0x87, 0x1f, 0x88 },
        { 0x28, 0x91, 0x29, 0x92, 0x2a, 0x93, 0x2b, 0x94,
          0x2c, 0x95, 0x2d, 0x96, 0x2e, 0x97, 0x2f, 0x98 },
        { 0x38, 0xa1, 0x39, 0xa2, 0x3a, 0xa3, 0x3b, 0xa4,
          0x3c, 0xa5, 0x3d, 0xa6, 0x3e, 0xa7, 0x3f, 0xa8 },
    };
    static const uint64_t scalar_values[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0xa1b2c3d4e5f60718ULL,
    };
    const uintptr_t seg_src_vaddr = hs_page_base(SWITCH1) + 0x140U;
    const uintptr_t seg_dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    const uintptr_t seg_src_paddr = phys_page_base(SWITCH1) + 0x140U;
    const uintptr_t seg_dst_paddr = phys_page_base(VSRWX_GRWX) + 0x180U;
    const uintptr_t scalar_vaddr = same_cache_line ?
        (seg_dst_vaddr + 0x20U) : (hs_page_base(SCRATCHPAD) + 0x2c0U);
    const uintptr_t scalar_paddr = same_cache_line ?
        (seg_dst_paddr + 0x20U) : (phys_page_base(SCRATCHPAD) + 0x2c0U);
    const uintptr_t seg_before = seg_dst_paddr - 1U;
    const uintptr_t seg_after = seg_dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uintptr_t scalar_before = scalar_paddr - 1U;
    const uintptr_t scalar_after = scalar_paddr + AI_MICRO_SCALAR_BYTES;
    const uint8_t seg_before_sentinel = 0x6cU;
    const uint8_t seg_after_sentinel = 0xc6U;
    const uint8_t scalar_before_sentinel = 0x3aU;
    const uint8_t scalar_after_sentinel = 0xa3U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(seg_patterns) / sizeof(seg_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        const uint64_t scalar_seed = scalar_values[(round + 1U) %
            (sizeof(scalar_values) / sizeof(scalar_values[0]))];

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(seg_src_paddr, seg_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(seg_dst_paddr, seg_patterns[0],
            AI_MICRO_SEGMENT_BYTES);
        write64(scalar_paddr, scalar_seed);
        write8(seg_before, seg_before_sentinel);
        write8(seg_after, seg_after_sentinel);
        write8(scalar_before, scalar_before_sentinel);
        write8(scalar_after, scalar_after_sentinel);

        goto_priv(PRIV_HS);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)seg_src_vaddr,
            AI_MICRO_SEGMENT_VL);

        if (vector_first) {
            ai_micro_vsseg2e8_from_v8((uint8_t *)seg_dst_vaddr,
                AI_MICRO_SEGMENT_VL);
            sd(scalar_vaddr, scalar_values[round]);
        } else {
            sd(scalar_vaddr, scalar_values[round]);
            ai_micro_vsseg2e8_from_v8((uint8_t *)seg_dst_vaddr,
                AI_MICRO_SEGMENT_VL);
        }

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the back-to-back scalar-store plus vsegment-store shared-sbuffer probe retires without exception before integrity checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the vsegment-store destination still receives exactly its intended interleaved payload while the scalar-store destination retains only its own doubleword payload",
            ai_micro_segment_bytes_match(seg_dst_paddr, seg_patterns[round],
                AI_MICRO_SEGMENT_BYTES) &&
            read64(scalar_paddr) == scalar_values[round]
        );
        TEST_ASSERT("the shared-sbuffer scalar-store plus vsegment-store probe also preserves adjacent sentinel bytes around both destinations",
            read8(seg_before) == seg_before_sentinel &&
            read8(seg_after) == seg_after_sentinel &&
            read8(scalar_before) == scalar_before_sentinel &&
            read8(scalar_after) == scalar_after_sentinel
        );

        vector_first = !vector_first;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_sbuffer_port0_scalar_and_vsegment_same_cycle_no_mux1h_drop_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_scalar_sbuffer_pair(false, true, true);
    TEST_END();
}

bool ai_micro_hs_shared_sbuffer_port0_lsqueue_vsegment_no_mixed_addr_data_mask_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_scalar_sbuffer_pair(true, false, true);
    TEST_END();
}

static bool ai_micro_segment_scalar_load_pair(bool vector_first,
    bool same_cache_line,
    bool repeat_rounds)
{
    static const uint8_t seg_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x44, 0xb4, 0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7,
          0x48, 0xb8, 0x49, 0xb9, 0x4a, 0xba, 0x4b, 0xbb },
        { 0x54, 0xc4, 0x55, 0xc5, 0x56, 0xc6, 0x57, 0xc7,
          0x58, 0xc8, 0x59, 0xc9, 0x5a, 0xca, 0x5b, 0xcb },
        { 0x64, 0xd4, 0x65, 0xd5, 0x66, 0xd6, 0x67, 0xd7,
          0x68, 0xd8, 0x69, 0xd9, 0x6a, 0xda, 0x6b, 0xdb },
    };
    static const uint64_t scalar_values[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
    };
    const uintptr_t seg_src_vaddr = hs_page_base(SWITCH1) + 0x180U;
    const uintptr_t seg_src_paddr = phys_page_base(SWITCH1) + 0x180U;
    const uintptr_t seg_dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x240U;
    const uintptr_t seg_dst_paddr = phys_page_base(VSRWX_GRWX) + 0x240U;
    const uintptr_t scalar_src_vaddr = same_cache_line ?
        (seg_src_vaddr + 0x18U) : (hs_page_base(SWITCH2) + 0x2c0U);
    const uintptr_t scalar_src_paddr = same_cache_line ?
        (seg_src_paddr + 0x18U) : (phys_page_base(SWITCH2) + 0x2c0U);
    const uintptr_t scalar_dst_vaddr = hs_page_base(SCRATCHPAD) + 0x300U;
    const uintptr_t scalar_dst_paddr = phys_page_base(SCRATCHPAD) + 0x300U;
    const uintptr_t seg_before = seg_dst_paddr - 1U;
    const uintptr_t seg_after = seg_dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uintptr_t scalar_before = scalar_dst_paddr - 1U;
    const uintptr_t scalar_after = scalar_dst_paddr + AI_MICRO_SCALAR_BYTES;
    const uint8_t seg_before_sentinel = 0x51U;
    const uint8_t seg_after_sentinel = 0x15U;
    const uint8_t scalar_before_sentinel = 0x72U;
    const uint8_t scalar_after_sentinel = 0x27U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(seg_patterns) / sizeof(seg_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        uint64_t scalar_observed = 0;

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(seg_src_paddr, seg_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(seg_dst_paddr, seg_patterns[0],
            AI_MICRO_SEGMENT_BYTES);
        write64(scalar_src_paddr, scalar_values[round]);
        write64(scalar_dst_paddr,
            scalar_values[(round + 1U) %
                (sizeof(scalar_values) / sizeof(scalar_values[0]))]);
        write8(seg_before, seg_before_sentinel);
        write8(seg_after, seg_after_sentinel);
        write8(scalar_before, scalar_before_sentinel);
        write8(scalar_after, scalar_after_sentinel);

        goto_priv(PRIV_HS);
        if (vector_first) {
            ai_micro_vlseg2e8_to_v8((const uint8_t *)seg_src_vaddr,
                AI_MICRO_SEGMENT_VL);
            scalar_observed = ld(scalar_src_vaddr);
        } else {
            scalar_observed = ld(scalar_src_vaddr);
            ai_micro_vlseg2e8_to_v8((const uint8_t *)seg_src_vaddr,
                AI_MICRO_SEGMENT_VL);
        }

        ai_micro_vsseg2e8_from_v8((uint8_t *)seg_dst_vaddr,
            AI_MICRO_SEGMENT_VL);
        sd(scalar_dst_vaddr, scalar_observed);

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the back-to-back scalar-load plus vsegment-load shared-dcache probe retires without exception before integrity checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the vsegment-load result still lands only in the segment destination while the scalar load result lands only in the scalar destination",
            ai_micro_segment_bytes_match(seg_dst_paddr, seg_patterns[round],
                AI_MICRO_SEGMENT_BYTES) &&
            read64(scalar_dst_paddr) == scalar_values[round]
        );
        TEST_ASSERT("the shared-dcache scalar-load plus vsegment-load probe also preserves adjacent sentinel bytes around both destinations",
            read8(seg_before) == seg_before_sentinel &&
            read8(seg_after) == seg_after_sentinel &&
            read8(scalar_before) == scalar_before_sentinel &&
            read8(scalar_after) == scalar_after_sentinel
        );

        vector_first = !vector_first;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_scalar_load0_dcache_handoff_no_response_misroute_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_scalar_load_pair(false, true, true);
    TEST_END();
}

static bool ai_micro_segment_scalar_load_boundary_window(bool repeat_rounds)
{
    static const uint8_t seg_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x74, 0xe4, 0x75, 0xe5, 0x76, 0xe6, 0x77, 0xe7,
          0x78, 0xe8, 0x79, 0xe9, 0x7a, 0xea, 0x7b, 0xeb },
        { 0x84, 0xf4, 0x85, 0xf5, 0x86, 0xf6, 0x87, 0xf7,
          0x88, 0xf8, 0x89, 0xf9, 0x8a, 0xfa, 0x8b, 0xfb },
        { 0x94, 0xa4, 0x95, 0xa5, 0x96, 0xa6, 0x97, 0xa7,
          0x98, 0xa8, 0x99, 0xa9, 0x9a, 0xaa, 0x9b, 0xab },
    };
    static const uint64_t scalar_before_values[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0xa1b2c3d4e5f60718ULL,
    };
    static const uint64_t scalar_after_values[] = {
        0x13579bdf2468ace0ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x55aa33cc77ee11ffULL,
    };
    const uintptr_t seg_src_vaddr = hs_page_base(SWITCH1) + 0x380U;
    const uintptr_t seg_src_paddr = phys_page_base(SWITCH1) + 0x380U;
    const uintptr_t seg_dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x480U;
    const uintptr_t seg_dst_paddr = phys_page_base(VSRWX_GRWX) + 0x480U;
    const uintptr_t scalar_before_src_vaddr = seg_src_vaddr + 0x18U;
    const uintptr_t scalar_before_src_paddr = seg_src_paddr + 0x18U;
    const uintptr_t scalar_after_src_vaddr = hs_page_base(SWITCH2) + 0x3c0U;
    const uintptr_t scalar_after_src_paddr = phys_page_base(SWITCH2) + 0x3c0U;
    const uintptr_t scalar_before_dst_vaddr = hs_page_base(SCRATCHPAD) + 0x500U;
    const uintptr_t scalar_before_dst_paddr = phys_page_base(SCRATCHPAD) + 0x500U;
    const uintptr_t scalar_after_dst_vaddr = hs_page_base(SCRATCHPAD) + 0x508U;
    const uintptr_t scalar_after_dst_paddr = phys_page_base(SCRATCHPAD) + 0x508U;
    const uintptr_t seg_before = seg_dst_paddr - 1U;
    const uintptr_t seg_after = seg_dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uintptr_t scalar_before_guard = scalar_before_dst_paddr - 1U;
    const uintptr_t scalar_after_guard = scalar_after_dst_paddr + AI_MICRO_SCALAR_BYTES;
    const uint8_t seg_before_sentinel = 0x4dU;
    const uint8_t seg_after_sentinel = 0xd4U;
    const uint8_t scalar_before_sentinel = 0x62U;
    const uint8_t scalar_after_sentinel = 0x26U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(seg_patterns) / sizeof(seg_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        uint64_t before_observed = 0;
        uint64_t after_observed = 0;

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(seg_src_paddr, seg_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(seg_dst_paddr, seg_patterns[0],
            AI_MICRO_SEGMENT_BYTES);
        write64(scalar_before_src_paddr, scalar_before_values[round]);
        write64(scalar_after_src_paddr, scalar_after_values[round]);
        write64(scalar_before_dst_paddr,
            scalar_after_values[(round + 1U) %
                (sizeof(scalar_after_values) / sizeof(scalar_after_values[0]))]);
        write64(scalar_after_dst_paddr,
            scalar_before_values[(round + 1U) %
                (sizeof(scalar_before_values) / sizeof(scalar_before_values[0]))]);
        write8(seg_before, seg_before_sentinel);
        write8(seg_after, seg_after_sentinel);
        write8(scalar_before_guard, scalar_before_sentinel);
        write8(scalar_after_guard, scalar_after_sentinel);

        goto_priv(PRIV_HS);
        before_observed = ld(scalar_before_src_vaddr);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)seg_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        after_observed = ld(scalar_after_src_vaddr);
        ai_micro_vsseg2e8_from_v8((uint8_t *)seg_dst_vaddr,
            AI_MICRO_SEGMENT_VL);
        sd(scalar_before_dst_vaddr, before_observed);
        sd(scalar_after_dst_vaddr, after_observed);

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the scalar-load before/after vsegment-load boundary probe retires without exception before integrity checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("scalar loads on both sides of the vsegment-load window still complete with their own payloads while the segment destination keeps only the segment payload",
            ai_micro_segment_bytes_match(seg_dst_paddr, seg_patterns[round],
                AI_MICRO_SEGMENT_BYTES) &&
            read64(scalar_before_dst_paddr) == scalar_before_values[round] &&
            read64(scalar_after_dst_paddr) == scalar_after_values[round]
        );
        TEST_ASSERT("the scalar-load before/after vsegment-load boundary probe also preserves the destination guard bytes",
            read8(seg_before) == seg_before_sentinel &&
            read8(seg_after) == seg_after_sentinel &&
            read8(scalar_before_guard) == scalar_before_sentinel &&
            read8(scalar_after_guard) == scalar_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_start_finish_boundary_no_scalar_load_starvation_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_scalar_load_boundary_window(true);
    TEST_END();
}

static bool ai_micro_segment_dual_store_distinct_vl_pair(bool older_longer,
    bool repeat_rounds)
{
    static const uint8_t src_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x41, 0xa1, 0x42, 0xa2, 0x43, 0xa3, 0x44, 0xa4,
          0x45, 0xa5, 0x46, 0xa6, 0x47, 0xa7, 0x48, 0xa8 },
        { 0x51, 0xb1, 0x52, 0xb2, 0x53, 0xb3, 0x54, 0xb4,
          0x55, 0xb5, 0x56, 0xb6, 0x57, 0xb7, 0x58, 0xb8 },
        { 0x61, 0xc1, 0x62, 0xc2, 0x63, 0xc3, 0x64, 0xc4,
          0x65, 0xc5, 0x66, 0xc6, 0x67, 0xc7, 0x68, 0xc8 },
    };
    static const uint8_t dst0_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd1, 0x11, 0xd2, 0x12, 0xd3, 0x13, 0xd4, 0x14,
          0xd5, 0x15, 0xd6, 0x16, 0xd7, 0x17, 0xd8, 0x18 },
        { 0xe1, 0x21, 0xe2, 0x22, 0xe3, 0x23, 0xe4, 0x24,
          0xe5, 0x25, 0xe6, 0x26, 0xe7, 0x27, 0xe8, 0x28 },
        { 0xf1, 0x31, 0xf2, 0x32, 0xf3, 0x33, 0xf4, 0x34,
          0xf5, 0x35, 0xf6, 0x36, 0xf7, 0x37, 0xf8, 0x38 },
    };
    static const uint8_t dst1_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x19, 0x91, 0x1a, 0x92, 0x1b, 0x93, 0x1c, 0x94,
          0x1d, 0x95, 0x1e, 0x96, 0x1f, 0x97, 0x20, 0x98 },
        { 0x29, 0xa1, 0x2a, 0xa2, 0x2b, 0xa3, 0x2c, 0xa4,
          0x2d, 0xa5, 0x2e, 0xa6, 0x2f, 0xa7, 0x30, 0xa8 },
        { 0x39, 0xb1, 0x3a, 0xb2, 0x3b, 0xb3, 0x3c, 0xb4,
          0x3d, 0xb5, 0x3e, 0xb6, 0x3f, 0xb7, 0x40, 0xb8 },
    };
    const uintptr_t src_vaddr = hs_page_base(SWITCH1) + 0x340U;
    const uintptr_t src_paddr = phys_page_base(SWITCH1) + 0x340U;
    const uintptr_t dst0_vaddr = hs_page_base(VSRWX_GRWX) + 0x380U;
    const uintptr_t dst0_paddr = phys_page_base(VSRWX_GRWX) + 0x380U;
    const uintptr_t dst1_vaddr = hs_page_base(SCRATCHPAD) + 0x3c0U;
    const uintptr_t dst1_paddr = phys_page_base(SCRATCHPAD) + 0x3c0U;
    const uint8_t dst0_before_sentinel = 0x69U;
    const uint8_t dst0_after_sentinel = 0x96U;
    const uint8_t dst1_before_sentinel = 0x4bU;
    const uint8_t dst1_after_sentinel = 0xb4U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(src_patterns) / sizeof(src_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        const unsigned older_vl = older_longer ? AI_MICRO_SEGMENT_VL : 3U;
        const unsigned younger_vl = older_longer ? 3U : AI_MICRO_SEGMENT_VL;

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(src_paddr, src_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst0_paddr, dst0_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst1_paddr, dst1_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(dst0_paddr - 1U, dst0_before_sentinel);
        write8(dst0_paddr + AI_MICRO_SEGMENT_BYTES, dst0_after_sentinel);
        write8(dst1_paddr - 1U, dst1_before_sentinel);
        write8(dst1_paddr + AI_MICRO_SEGMENT_BYTES, dst1_after_sentinel);

        goto_priv(PRIV_HS);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_pair_from_v8((uint8_t *)dst0_vaddr, older_vl,
            (uint8_t *)dst1_vaddr, younger_vl);

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the back-to-back dual-vl vsegment-store pair retires without exception before integrity checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the queued vsegment-store pair keeps each instruction's own vl-bound payload and leaves the unwritten tail bytes at both destinations intact",
            ai_micro_segment_prefix_and_seed_tail_match(dst0_paddr,
                src_patterns[round], older_vl, dst0_seed[round]) &&
            ai_micro_segment_prefix_and_seed_tail_match(dst1_paddr,
                src_patterns[round], younger_vl, dst1_seed[round])
        );
        TEST_ASSERT("the dual-vl vsegment-store pair also preserves adjacent sentinel bytes around both destinations",
            read8(dst0_paddr - 1U) == dst0_before_sentinel &&
            read8(dst0_paddr + AI_MICRO_SEGMENT_BYTES) ==
                dst0_after_sentinel &&
            read8(dst1_paddr - 1U) == dst1_before_sentinel &&
            read8(dst1_paddr + AI_MICRO_SEGMENT_BYTES) ==
                dst1_after_sentinel
        );

        older_longer = !older_longer;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_two_instructions_queued_no_instmicroop_context_bleed_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_dual_store_distinct_vl_pair(true, true);
    TEST_END();
}

static bool ai_micro_segment_store_burst_no_queue_overwrite(bool repeat_rounds)
{
    static const uint8_t src_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x71, 0xd1, 0x72, 0xd2, 0x73, 0xd3, 0x74, 0xd4,
          0x75, 0xd5, 0x76, 0xd6, 0x77, 0xd7, 0x78, 0xd8 },
        { 0x81, 0xe1, 0x82, 0xe2, 0x83, 0xe3, 0x84, 0xe4,
          0x85, 0xe5, 0x86, 0xe6, 0x87, 0xe7, 0x88, 0xe8 },
        { 0x91, 0xf1, 0x92, 0xf2, 0x93, 0xf3, 0x94, 0xf4,
          0x95, 0xf5, 0x96, 0xf6, 0x97, 0xf7, 0x98, 0xf8 },
    };
    static const uint8_t dst_seed[AI_MICRO_SEGMENT_BYTES] = {
        0xca, 0x2a, 0xcb, 0x2b, 0xcc, 0x2c, 0xcd, 0x2d,
        0xce, 0x2e, 0xcf, 0x2f, 0xd0, 0x30, 0xd1, 0x31
    };
    static const unsigned burst_vls[AI_MICRO_SEGMENT_BURST_COUNT] = {
        8U, 1U, 7U, 2U, 6U, 3U, 5U, 4U, 8U
    };
    const uintptr_t src_vaddr = hs_page_base(SWITCH2) + 0x340U;
    const uintptr_t src_paddr = phys_page_base(SWITCH2) + 0x340U;
    const uintptr_t dst_base_vaddr = hs_page_base(VSRWX_GRWX) + 0x440U;
    const uintptr_t dst_base_paddr = phys_page_base(VSRWX_GRWX) + 0x440U;
    const uintptr_t dst_step = 0x20U;
    const uint8_t before_sentinel_base = 0x40U;
    const uint8_t after_sentinel_base = 0xc0U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(src_patterns) / sizeof(src_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(src_paddr, src_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        for (unsigned i = 0; i < AI_MICRO_SEGMENT_BURST_COUNT; ++i) {
            const uintptr_t dst_paddr = dst_base_paddr + (uintptr_t)i * dst_step;

            ai_micro_segment_write_bytes(dst_paddr, dst_seed,
                AI_MICRO_SEGMENT_BYTES);
            write8(dst_paddr - 1U, (uint8_t)(before_sentinel_base + i));
            write8(dst_paddr + AI_MICRO_SEGMENT_BYTES,
                (uint8_t)(after_sentinel_base + i));
        }

        goto_priv(PRIV_HS);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)src_vaddr,
            AI_MICRO_SEGMENT_VL);
        for (unsigned i = 0; i < AI_MICRO_SEGMENT_BURST_COUNT; ++i) {
            ai_micro_vsseg2e8_from_v8((uint8_t *)(dst_base_vaddr +
                (uintptr_t)i * dst_step), (int)burst_vls[i]);
        }

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the back-to-back vsegment-store burst retires without exception before integrity checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SEGMENT_BURST_COUNT; ++i) {
            const uintptr_t dst_paddr = dst_base_paddr + (uintptr_t)i * dst_step;
            TEST_ASSERT("each destination in the queued vsegment-store burst keeps its own vl-bounded payload and untouched tail bytes",
                ai_micro_segment_prefix_and_seed_tail_match(dst_paddr,
                    src_patterns[round], burst_vls[i], dst_seed)
            );
            TEST_ASSERT("each destination in the queued vsegment-store burst also preserves adjacent sentinel bytes",
                read8(dst_paddr - 1U) == (uint8_t)(before_sentinel_base + i) &&
                read8(dst_paddr + AI_MICRO_SEGMENT_BYTES) ==
                    (uint8_t)(after_sentinel_base + i)
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_back_to_back_many_uops_ready_must_not_hide_full_condition_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_store_burst_no_queue_overwrite(true);
    TEST_END();
}

static bool ai_micro_segment_boundary_store_drain_copy_window(bool repeat_rounds)
{
    static const uint8_t source_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xe0, 0x70, 0xe1, 0x71, 0xe2, 0x72, 0xe3, 0x73,
          0xe4, 0x74, 0xe5, 0x75, 0xe6, 0x76, 0xe7, 0x77 },
        { 0xd0, 0x60, 0xd1, 0x61, 0xd2, 0x62, 0xd3, 0x63,
          0xd4, 0x64, 0xd5, 0x65, 0xd6, 0x66, 0xd7, 0x67 },
        { 0xc0, 0x50, 0xc1, 0x51, 0xc2, 0x52, 0xc3, 0x53,
          0xc4, 0x54, 0xc5, 0x55, 0xc6, 0x56, 0xc7, 0x57 },
    };
    static const uint8_t updated_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x10, 0x80, 0x11, 0x81, 0x12, 0x82, 0x13, 0x83,
          0x14, 0x84, 0x15, 0x85, 0x16, 0x86, 0x17, 0x87 },
        { 0x21, 0x91, 0x22, 0x92, 0x23, 0x93, 0x24, 0x94,
          0x25, 0x95, 0x26, 0x96, 0x27, 0x97, 0x28, 0x98 },
        { 0x32, 0xa2, 0x33, 0xa3, 0x34, 0xa4, 0x35, 0xa5,
          0x36, 0xa6, 0x37, 0xa7, 0x38, 0xa8, 0x39, 0xa9 },
    };
    static const uint8_t dst_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x9a, 0x0a, 0x9b, 0x0b, 0x9c, 0x0c, 0x9d, 0x0d,
          0x9e, 0x0e, 0x9f, 0x0f, 0x8a, 0x1a, 0x8b, 0x1b },
        { 0x7c, 0x2c, 0x7d, 0x2d, 0x7e, 0x2e, 0x7f, 0x2f,
          0x6c, 0x3c, 0x6d, 0x3d, 0x6e, 0x3e, 0x6f, 0x3f },
        { 0x5e, 0x4e, 0x5f, 0x4f, 0x4e, 0x5e, 0x4f, 0x5f,
          0x3e, 0x6e, 0x3f, 0x6f, 0x2e, 0x7e, 0x2f, 0x7f },
    };
    const uintptr_t src_vaddr = hs_page_base(SWITCH1) + 0x740U;
    const uintptr_t dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x7a0U;
    const uintptr_t src_paddr = phys_page_base(SWITCH1) + 0x740U;
    const uintptr_t dst_paddr = phys_page_base(VSRWX_GRWX) + 0x7a0U;
    const uintptr_t src_before = src_paddr - 1U;
    const uintptr_t src_after = src_paddr + AI_MICRO_SEGMENT_BYTES;
    const uintptr_t dst_before = dst_paddr - 1U;
    const uintptr_t dst_after = dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t src_before_sentinel = 0x41U;
    const uint8_t src_after_sentinel = 0x14U;
    const uint8_t dst_before_sentinel = 0x52U;
    const uint8_t dst_after_sentinel = 0x25U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(updated_src) / sizeof(updated_src[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(src_paddr, source_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, dst_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(src_before, src_before_sentinel);
        write8(src_after, src_after_sentinel);
        write8(dst_before, dst_before_sentinel);
        write8(dst_after, dst_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(src_vaddr, ai_micro_pack_u64_le(updated_src[round]));
        sd(src_vaddr + 8U, ai_micro_pack_u64_le(updated_src[round] + 8U));
        ai_micro_vlseg2e8_to_v8((const uint8_t *)src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr,
            AI_MICRO_SEGMENT_VL);
        fence_iorw_iorw();
        TEST_ASSERT("the immediate scalar-store then vsegment load/store boundary window retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the vsegment boundary store-drain window copies only the just-stored source payload, showing the segment access does not start from a stale empty indication",
            ai_micro_segment_bytes_match(src_paddr, updated_src[round],
                AI_MICRO_SEGMENT_BYTES) &&
            ai_micro_segment_bytes_match(dst_paddr, updated_src[round],
                AI_MICRO_SEGMENT_BYTES)
        );
        TEST_ASSERT("the vsegment boundary store-drain window also preserves source and destination guard bytes through the same immediate sequence",
            read8(src_before) == src_before_sentinel &&
            read8(src_after) == src_after_sentinel &&
            read8(dst_before) == dst_before_sentinel &&
            read8(dst_after) == dst_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_boundary_store_drain_no_false_empty_or_extra_wait_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_boundary_store_drain_copy_window(true);
    TEST_END();
}

static inline void ai_micro_fill_unit_stride_vector_pattern(uint8_t *bytes,
    unsigned round,
    unsigned slot,
    uint8_t salt)
{
    const uint8_t base = (uint8_t)(salt + (uint8_t)(round * 0x21U) +
        (uint8_t)(slot * 0x0bU));

    for (unsigned i = 0; i < AI_MICRO_VECTOR_BYTES; ++i) {
        bytes[i] = (uint8_t)(base ^
            (uint8_t)(i * 0x13U + slot * 3U + round));
    }
}

static inline uint64_t ai_micro_unit_stride_scalar_pattern(unsigned round,
    unsigned slot,
    uint64_t salt)
{
    return salt ^
        ((uint64_t)(round + 1U) << 56) ^
        ((uint64_t)(slot + 1U) << 48) ^
        ((uint64_t)(0x31U + slot * 7U) << 32) ^
        ((uint64_t)(0x55U + round * 9U) << 16) ^
        (uint64_t)(0x0101U * (slot + round + 1U));
}

static bool ai_micro_unit_stride_vector_scalar_store_window(bool scalar_first,
    bool repeat_rounds)
{
    const uintptr_t vec_src_base_vaddr = hs_page_base(SWITCH1) + 0x500U;
    const uintptr_t vec_dst_base_vaddr = hs_page_base(VSRWX_GRWX) + 0x540U;
    const uintptr_t scalar_dst_base_vaddr = hs_page_base(SCRATCHPAD) + 0x600U;
    const uintptr_t vec_src_base_paddr = phys_page_base(SWITCH1) + 0x500U;
    const uintptr_t vec_dst_base_paddr = phys_page_base(VSRWX_GRWX) + 0x540U;
    const uintptr_t scalar_dst_base_paddr = phys_page_base(SCRATCHPAD) + 0x600U;
    const unsigned rounds = repeat_rounds ? 3U : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_MICRO_MIXED_MEMOP_WINDOW_COUNT;
            ++slot) {
            uint8_t expected_vec[AI_MICRO_VECTOR_BYTES];
            uint8_t seed_vec[AI_MICRO_VECTOR_BYTES];
            const uintptr_t vec_src_paddr = vec_src_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t vec_dst_paddr = vec_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t scalar_dst_paddr = scalar_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_SCALAR_STEP;
            const uint64_t scalar_value =
                ai_micro_unit_stride_scalar_pattern(round, slot,
                    0x1144332255667788ULL);

            ai_micro_fill_unit_stride_vector_pattern(expected_vec, round, slot,
                0x31U);
            ai_micro_fill_unit_stride_vector_pattern(seed_vec, round + 1U,
                slot + 1U, 0xe0U);
            ai_micro_segment_write_bytes(vec_src_paddr, expected_vec,
                AI_MICRO_VECTOR_BYTES);
            ai_micro_segment_write_bytes(vec_dst_paddr, seed_vec,
                AI_MICRO_VECTOR_BYTES);
            write64(scalar_dst_paddr, ~scalar_value);
            write8(vec_dst_paddr - 1U, (uint8_t)(0x40U + slot));
            write8(vec_dst_paddr + AI_MICRO_VECTOR_BYTES,
                (uint8_t)(0xc0U + slot));
            write8(scalar_dst_paddr - 1U, (uint8_t)(0x60U + slot));
            write8(scalar_dst_paddr + AI_MICRO_SCALAR_BYTES,
                (uint8_t)(0xa0U + slot));
        }

        goto_priv(PRIV_HS);
        for (unsigned slot = 0; slot < AI_MICRO_MIXED_MEMOP_WINDOW_COUNT;
            ++slot) {
            const uintptr_t vec_src_vaddr = vec_src_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t vec_dst_vaddr = vec_dst_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t scalar_dst_vaddr = scalar_dst_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_SCALAR_STEP;
            const uint64_t scalar_value =
                ai_micro_unit_stride_scalar_pattern(round, slot,
                    0x1144332255667788ULL);

            if (scalar_first) {
                sd(scalar_dst_vaddr, scalar_value);
            }

            vle8_to_v6((const uint8_t *)vec_src_vaddr, AI_MICRO_VECTOR_BYTES);
            vse8_from_v6((uint8_t *)vec_dst_vaddr, AI_MICRO_VECTOR_BYTES);

            if (!scalar_first) {
                sd(scalar_dst_vaddr, scalar_value);
            }
        }

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the dense mixed unit-stride vector-store and scalar-store window retires without exception before integrity checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_MICRO_MIXED_MEMOP_WINDOW_COUNT;
            ++slot) {
            uint8_t expected_vec[AI_MICRO_VECTOR_BYTES];
            const uintptr_t vec_dst_paddr = vec_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t scalar_dst_paddr = scalar_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_SCALAR_STEP;
            const uint64_t scalar_value =
                ai_micro_unit_stride_scalar_pattern(round, slot,
                    0x1144332255667788ULL);

            ai_micro_fill_unit_stride_vector_pattern(expected_vec, round, slot,
                0x31U);
            TEST_ASSERT("each mixed scalar/vector store window keeps exactly its own vector payload and scalar payload with no overwrite, skip, or cross-window bleed",
                ai_micro_segment_bytes_match(vec_dst_paddr, expected_vec,
                    AI_MICRO_VECTOR_BYTES) &&
                read64(scalar_dst_paddr) == scalar_value
            );
            TEST_ASSERT("each mixed scalar/vector store window also preserves the guard bytes around both the vector and scalar destinations",
                read8(vec_dst_paddr - 1U) == (uint8_t)(0x40U + slot) &&
                read8(vec_dst_paddr + AI_MICRO_VECTOR_BYTES) ==
                    (uint8_t)(0xc0U + slot) &&
                read8(scalar_dst_paddr - 1U) == (uint8_t)(0x60U + slot) &&
                read8(scalar_dst_paddr + AI_MICRO_SCALAR_BYTES) ==
                    (uint8_t)(0xa0U + slot)
            );
        }

        scalar_first = !scalar_first;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_unit_stride_vector_scalar_load_window(bool scalar_first,
    bool repeat_rounds)
{
    const uintptr_t vec_src_base_vaddr = hs_page_base(SWITCH2) + 0x500U;
    const uintptr_t vec_dst_base_vaddr = hs_page_base(SCRATCHPAD) + 0x700U;
    const uintptr_t scalar_src_base_vaddr = hs_page_base(VSRWX_GRWX) + 0x6a0U;
    const uintptr_t scalar_dst_base_vaddr = hs_page_base(SWITCH1) + 0x680U;
    const uintptr_t vec_src_base_paddr = phys_page_base(SWITCH2) + 0x500U;
    const uintptr_t vec_dst_base_paddr = phys_page_base(SCRATCHPAD) + 0x700U;
    const uintptr_t scalar_src_base_paddr = phys_page_base(VSRWX_GRWX) + 0x6a0U;
    const uintptr_t scalar_dst_base_paddr = phys_page_base(SWITCH1) + 0x680U;
    const unsigned rounds = repeat_rounds ? 3U : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_MICRO_MIXED_MEMOP_WINDOW_COUNT;
            ++slot) {
            uint8_t expected_vec[AI_MICRO_VECTOR_BYTES];
            uint8_t seed_vec[AI_MICRO_VECTOR_BYTES];
            const uintptr_t vec_src_paddr = vec_src_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t vec_dst_paddr = vec_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t scalar_src_paddr = scalar_src_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_SCALAR_STEP;
            const uintptr_t scalar_dst_paddr = scalar_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_SCALAR_STEP;
            const uint64_t scalar_value =
                ai_micro_unit_stride_scalar_pattern(round, slot,
                    0x8877665544332211ULL);

            ai_micro_fill_unit_stride_vector_pattern(expected_vec, round, slot,
                0x57U);
            ai_micro_fill_unit_stride_vector_pattern(seed_vec, round + 2U,
                slot + 2U, 0xb0U);
            ai_micro_segment_write_bytes(vec_src_paddr, expected_vec,
                AI_MICRO_VECTOR_BYTES);
            ai_micro_segment_write_bytes(vec_dst_paddr, seed_vec,
                AI_MICRO_VECTOR_BYTES);
            write64(scalar_src_paddr, scalar_value);
            write64(scalar_dst_paddr, ~scalar_value);
            write8(vec_dst_paddr - 1U, (uint8_t)(0x20U + slot));
            write8(vec_dst_paddr + AI_MICRO_VECTOR_BYTES,
                (uint8_t)(0x90U + slot));
            write8(scalar_dst_paddr - 1U, (uint8_t)(0x70U + slot));
            write8(scalar_dst_paddr + AI_MICRO_SCALAR_BYTES,
                (uint8_t)(0xd0U + slot));
        }

        goto_priv(PRIV_HS);
        for (unsigned slot = 0; slot < AI_MICRO_MIXED_MEMOP_WINDOW_COUNT;
            ++slot) {
            uint64_t scalar_observed = 0;
            const uintptr_t vec_src_vaddr = vec_src_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t vec_dst_vaddr = vec_dst_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t scalar_src_vaddr = scalar_src_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_SCALAR_STEP;
            const uintptr_t scalar_dst_vaddr = scalar_dst_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_SCALAR_STEP;

            if (scalar_first) {
                scalar_observed = ld(scalar_src_vaddr);
                vle8_to_v6((const uint8_t *)vec_src_vaddr,
                    AI_MICRO_VECTOR_BYTES);
            } else {
                vle8_to_v6((const uint8_t *)vec_src_vaddr,
                    AI_MICRO_VECTOR_BYTES);
                scalar_observed = ld(scalar_src_vaddr);
            }

            vse8_from_v6((uint8_t *)vec_dst_vaddr, AI_MICRO_VECTOR_BYTES);
            sd(scalar_dst_vaddr, scalar_observed);
        }

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the dense mixed unit-stride vector-load and scalar-load window retires without exception before integrity checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_MICRO_MIXED_MEMOP_WINDOW_COUNT;
            ++slot) {
            uint8_t expected_vec[AI_MICRO_VECTOR_BYTES];
            const uintptr_t vec_dst_paddr = vec_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t scalar_dst_paddr = scalar_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_SCALAR_STEP;
            const uint64_t scalar_value =
                ai_micro_unit_stride_scalar_pattern(round, slot,
                    0x8877665544332211ULL);

            ai_micro_fill_unit_stride_vector_pattern(expected_vec, round, slot,
                0x57U);
            TEST_ASSERT("each mixed scalar/vector load window keeps exactly its own vector payload copy and scalar observed value with no overwrite, skip, or cross-window bleed",
                ai_micro_segment_bytes_match(vec_dst_paddr, expected_vec,
                    AI_MICRO_VECTOR_BYTES) &&
                read64(scalar_dst_paddr) == scalar_value
            );
            TEST_ASSERT("each mixed scalar/vector load window also preserves the guard bytes around both the vector and scalar destinations",
                read8(vec_dst_paddr - 1U) == (uint8_t)(0x20U + slot) &&
                read8(vec_dst_paddr + AI_MICRO_VECTOR_BYTES) ==
                    (uint8_t)(0x90U + slot) &&
                read8(scalar_dst_paddr - 1U) == (uint8_t)(0x70U + slot) &&
                read8(scalar_dst_paddr + AI_MICRO_SCALAR_BYTES) ==
                    (uint8_t)(0xd0U + slot)
            );
        }

        scalar_first = !scalar_first;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_lsq_enq_multielem_offsets_near_full_no_overlap_or_skip_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_unit_stride_vector_scalar_store_window(false, true);
    TEST_END();
}

bool ai_micro_hs_mixed_scalar_vector_memop_same_cycle_lq_sq_index_spacing_correct_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_unit_stride_vector_scalar_load_window(true, true);
    TEST_END();
}

static bool ai_micro_lsq_redirect_recovery_mixed_memop_window(bool store_first,
    bool repeat_rounds)
{
    const uintptr_t bad_store_base_vaddr = hs_page_base(VSI_GI) + 0x180U;
    const uintptr_t bad_load_base_vaddr = hs_page_base(VSI_GI) + 0x1c0U;
    const unsigned rounds = repeat_rounds ? 3U : 1U;
    bool test_status = true;

    for (unsigned round = 0; round < rounds; ++round) {
        const uintptr_t bad_store_vaddr = bad_store_base_vaddr +
            (uintptr_t)round * AI_MICRO_SCALAR_BYTES;
        const uintptr_t bad_load_vaddr = bad_load_base_vaddr +
            (uintptr_t)round * AI_MICRO_SCALAR_BYTES;
        const uint64_t bad_store_value =
            ai_micro_unit_stride_scalar_pattern(round, 0U,
                0xdeadbeef01020304ULL);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        if (store_first) {
            TEST_SETUP_EXCEPT();
            sd(bad_store_vaddr, bad_store_value);
            TEST_ASSERT("the redirect-driving invalid-leaf store still reports store page fault before the immediate post-trap mixed store-allocation window",
                excpt.triggered == true &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == bad_store_vaddr
            );
            test_status = test_status &&
                ai_micro_unit_stride_vector_scalar_store_window(
                    (round & 1U) != 0U, false);

            TEST_SETUP_EXCEPT();
            (void)ld(bad_load_vaddr);
            TEST_ASSERT("the redirect-driving invalid-leaf load still reports load page fault before the immediate post-trap mixed load-allocation window",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == bad_load_vaddr
            );
            test_status = test_status &&
                ai_micro_unit_stride_vector_scalar_load_window(
                    (round & 1U) == 0U, false);
        } else {
            TEST_SETUP_EXCEPT();
            (void)ld(bad_load_vaddr);
            TEST_ASSERT("the redirect-driving invalid-leaf load still reports load page fault before the immediate post-trap mixed load-allocation window",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == bad_load_vaddr
            );
            test_status = test_status &&
                ai_micro_unit_stride_vector_scalar_load_window(
                    (round & 1U) != 0U, false);

            TEST_SETUP_EXCEPT();
            sd(bad_store_vaddr, bad_store_value);
            TEST_ASSERT("the redirect-driving invalid-leaf store still reports store page fault before the immediate post-trap mixed store-allocation window",
                excpt.triggered == true &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == bad_store_vaddr
            );
            test_status = test_status &&
                ai_micro_unit_stride_vector_scalar_store_window(
                    (round & 1U) == 0U, false);
        }

        store_first = !store_first;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_lsq_enq_redirect_recovery_no_duplicate_or_lost_lq_sq_index_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_lsq_redirect_recovery_mixed_memop_window(true, false);
    TEST_END();
}

bool ai_micro_hs_lsq_enq_multicycle_redirect_near_wrap_accept_and_ptrs_stay_consistent_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_lsq_redirect_recovery_mixed_memop_window(true, true);
    TEST_END();
}

static bool ai_micro_shared_vecmem_feedback_window(bool unit_first,
    bool repeat_rounds)
{
    const uintptr_t unit_src_base_vaddr = hs_page_base(SWITCH1) + 0x780U;
    const uintptr_t unit_dst_base_vaddr = hs_page_base(VSRWX_GRWX) + 0x7c0U;
    const uintptr_t seg_src_base_vaddr = hs_page_base(SWITCH2) + 0x880U;
    const uintptr_t seg_dst_base_vaddr = hs_page_base(SCRATCHPAD) + 0x8c0U;
    const uintptr_t unit_src_base_paddr = phys_page_base(SWITCH1) + 0x780U;
    const uintptr_t unit_dst_base_paddr = phys_page_base(VSRWX_GRWX) + 0x7c0U;
    const uintptr_t seg_src_base_paddr = phys_page_base(SWITCH2) + 0x880U;
    const uintptr_t seg_dst_base_paddr = phys_page_base(SCRATCHPAD) + 0x8c0U;
    const unsigned rounds = repeat_rounds ? 3U : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_MICRO_MIXED_MEMOP_WINDOW_COUNT;
            ++slot) {
            uint8_t expected_unit[AI_MICRO_VECTOR_BYTES];
            uint8_t seed_unit[AI_MICRO_VECTOR_BYTES];
            uint8_t expected_seg[AI_MICRO_SEGMENT_BYTES];
            uint8_t seed_seg[AI_MICRO_SEGMENT_BYTES];
            const uintptr_t unit_src_paddr = unit_src_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t unit_dst_paddr = unit_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t seg_src_paddr = seg_src_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t seg_dst_paddr = seg_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;

            ai_micro_fill_unit_stride_vector_pattern(expected_unit, round, slot,
                0x69U);
            ai_micro_fill_unit_stride_vector_pattern(seed_unit, round + 1U,
                slot + 1U, 0xd2U);
            ai_micro_fill_unit_stride_vector_pattern(expected_seg, round, slot,
                0x27U);
            ai_micro_fill_unit_stride_vector_pattern(seed_seg, round + 2U,
                slot + 2U, 0xb8U);

            ai_micro_segment_write_bytes(unit_src_paddr, expected_unit,
                AI_MICRO_VECTOR_BYTES);
            ai_micro_segment_write_bytes(unit_dst_paddr, seed_unit,
                AI_MICRO_VECTOR_BYTES);
            ai_micro_segment_write_bytes(seg_src_paddr, expected_seg,
                AI_MICRO_SEGMENT_BYTES);
            ai_micro_segment_write_bytes(seg_dst_paddr, seed_seg,
                AI_MICRO_SEGMENT_BYTES);

            write8(unit_dst_paddr - 1U, (uint8_t)(0x10U + slot));
            write8(unit_dst_paddr + AI_MICRO_VECTOR_BYTES,
                (uint8_t)(0xe0U + slot));
            write8(seg_dst_paddr - 1U, (uint8_t)(0x30U + slot));
            write8(seg_dst_paddr + AI_MICRO_SEGMENT_BYTES,
                (uint8_t)(0xa0U + slot));
        }

        goto_priv(PRIV_HS);
        for (unsigned slot = 0; slot < AI_MICRO_MIXED_MEMOP_WINDOW_COUNT;
            ++slot) {
            const uintptr_t unit_src_vaddr = unit_src_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t unit_dst_vaddr = unit_dst_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t seg_src_vaddr = seg_src_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t seg_dst_vaddr = seg_dst_base_vaddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;

            if (unit_first) {
                vle8_to_v6((const uint8_t *)unit_src_vaddr,
                    AI_MICRO_VECTOR_BYTES);
                ai_micro_vlseg2e8_to_v8((const uint8_t *)seg_src_vaddr,
                    AI_MICRO_SEGMENT_VL);
                vse8_from_v6((uint8_t *)unit_dst_vaddr,
                    AI_MICRO_VECTOR_BYTES);
                ai_micro_vsseg2e8_from_v8((uint8_t *)seg_dst_vaddr,
                    AI_MICRO_SEGMENT_VL);
            } else {
                ai_micro_vlseg2e8_to_v8((const uint8_t *)seg_src_vaddr,
                    AI_MICRO_SEGMENT_VL);
                vle8_to_v6((const uint8_t *)unit_src_vaddr,
                    AI_MICRO_VECTOR_BYTES);
                ai_micro_vsseg2e8_from_v8((uint8_t *)seg_dst_vaddr,
                    AI_MICRO_SEGMENT_VL);
                vse8_from_v6((uint8_t *)unit_dst_vaddr,
                    AI_MICRO_VECTOR_BYTES);
            }
        }

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the shared vec-mem IQ mixed unit-stride-plus-segment load/store window retires without exception before integrity checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_MICRO_MIXED_MEMOP_WINDOW_COUNT;
            ++slot) {
            uint8_t expected_unit[AI_MICRO_VECTOR_BYTES];
            uint8_t expected_seg[AI_MICRO_SEGMENT_BYTES];
            const uintptr_t unit_dst_paddr = unit_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;
            const uintptr_t seg_dst_paddr = seg_dst_base_paddr +
                (uintptr_t)slot * AI_MICRO_MIXED_VECTOR_STEP;

            ai_micro_fill_unit_stride_vector_pattern(expected_unit, round, slot,
                0x69U);
            ai_micro_fill_unit_stride_vector_pattern(expected_seg, round, slot,
                0x27U);
            TEST_ASSERT("each shared vec-mem IQ mixed window keeps the unit-stride payload and segment payload bound to their own destinations with no drop, duplicate, or cross-binding",
                ai_micro_segment_bytes_match(unit_dst_paddr, expected_unit,
                    AI_MICRO_VECTOR_BYTES) &&
                ai_micro_segment_bytes_match(seg_dst_paddr, expected_seg,
                    AI_MICRO_SEGMENT_BYTES)
            );
            TEST_ASSERT("each shared vec-mem IQ mixed window also preserves the guard bytes around both destinations",
                read8(unit_dst_paddr - 1U) == (uint8_t)(0x10U + slot) &&
                read8(unit_dst_paddr + AI_MICRO_VECTOR_BYTES) ==
                    (uint8_t)(0xe0U + slot) &&
                read8(seg_dst_paddr - 1U) == (uint8_t)(0x30U + slot) &&
                read8(seg_dst_paddr + AI_MICRO_SEGMENT_BYTES) ==
                    (uint8_t)(0xa0U + slot)
            );
        }

        unit_first = !unit_first;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_shared_vecmem_iq_vector_load_feedback_not_lost_to_neighbor_store_feedback_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_shared_vecmem_feedback_window(true, true);
    TEST_END();
}

bool ai_micro_hs_shared_vecmem_iq_mixed_unitstride_segment_memops_no_feedback_cross_binding_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_shared_vecmem_feedback_window(false, true);
    TEST_END();
}

static bool ai_micro_segment_masked_all_inactive_store_window(bool repeat_rounds)
{
    static const uint8_t masked_src_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x10, 0x80, 0x11, 0x81, 0x12, 0x82, 0x13, 0x83,
          0x14, 0x84, 0x15, 0x85, 0x16, 0x86, 0x17, 0x87 },
        { 0x21, 0x91, 0x22, 0x92, 0x23, 0x93, 0x24, 0x94,
          0x25, 0x95, 0x26, 0x96, 0x27, 0x97, 0x28, 0x98 },
        { 0x32, 0xa2, 0x33, 0xa3, 0x34, 0xa4, 0x35, 0xa5,
          0x36, 0xa6, 0x37, 0xa7, 0x38, 0xa8, 0x39, 0xa9 },
    };
    static const uint8_t protected_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xe0, 0x70, 0xe1, 0x71, 0xe2, 0x72, 0xe3, 0x73,
          0xe4, 0x74, 0xe5, 0x75, 0xe6, 0x76, 0xe7, 0x77 },
        { 0xd0, 0x60, 0xd1, 0x61, 0xd2, 0x62, 0xd3, 0x63,
          0xd4, 0x64, 0xd5, 0x65, 0xd6, 0x66, 0xd7, 0x67 },
        { 0xc0, 0x50, 0xc1, 0x51, 0xc2, 0x52, 0xc3, 0x53,
          0xc4, 0x54, 0xc5, 0x55, 0xc6, 0x56, 0xc7, 0x57 },
    };
    static const uint8_t live_src_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x40, 0xb0, 0x41, 0xb1, 0x42, 0xb2, 0x43, 0xb3,
          0x44, 0xb4, 0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7 },
        { 0x51, 0xc1, 0x52, 0xc2, 0x53, 0xc3, 0x54, 0xc4,
          0x55, 0xc5, 0x56, 0xc6, 0x57, 0xc7, 0x58, 0xc8 },
        { 0x62, 0xd2, 0x63, 0xd3, 0x64, 0xd4, 0x65, 0xd5,
          0x66, 0xd6, 0x67, 0xd7, 0x68, 0xd8, 0x69, 0xd9 },
    };
    static const uint8_t live_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x9a, 0x0a, 0x9b, 0x0b, 0x9c, 0x0c, 0x9d, 0x0d,
          0x9e, 0x0e, 0x9f, 0x0f, 0x8a, 0x1a, 0x8b, 0x1b },
        { 0x7c, 0x2c, 0x7d, 0x2d, 0x7e, 0x2e, 0x7f, 0x2f,
          0x6c, 0x3c, 0x6d, 0x3d, 0x6e, 0x3e, 0x6f, 0x3f },
        { 0x5e, 0x4e, 0x5f, 0x4f, 0x4e, 0x5e, 0x4f, 0x5f,
          0x3e, 0x6e, 0x3f, 0x6f, 0x2e, 0x7e, 0x2f, 0x7f },
    };
    static const unsigned live_vls[] = { 8U, 5U, 6U };
    const uintptr_t masked_src_vaddr = hs_page_base(SWITCH1) + 0x580U;
    const uintptr_t protected_dst_vaddr = hs_page_base(VSR_GRWX) + 0x5c0U;
    const uintptr_t live_src_vaddr = hs_page_base(SWITCH2) + 0x640U;
    const uintptr_t live_dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x6c0U;
    const uintptr_t masked_src_paddr = phys_page_base(SWITCH1) + 0x580U;
    const uintptr_t protected_dst_paddr = phys_page_base(VSR_GRWX) + 0x5c0U;
    const uintptr_t live_src_paddr = phys_page_base(SWITCH2) + 0x640U;
    const uintptr_t live_dst_paddr = phys_page_base(VSRWX_GRWX) + 0x6c0U;
    const uintptr_t protected_before = protected_dst_paddr - 1U;
    const uintptr_t protected_after = protected_dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uintptr_t live_before = live_dst_paddr - 1U;
    const uintptr_t live_after = live_dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t protected_before_sentinel = 0x52U;
    const uint8_t protected_after_sentinel = 0x25U;
    const uint8_t live_before_sentinel = 0x61U;
    const uint8_t live_after_sentinel = 0x16U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(masked_src_patterns) / sizeof(masked_src_patterns[0])) :
        1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        const unsigned live_vl = live_vls[round];

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(masked_src_paddr, masked_src_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(protected_dst_paddr, protected_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(live_src_paddr, live_src_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(live_dst_paddr, live_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(protected_before, protected_before_sentinel);
        write8(protected_after, protected_after_sentinel);
        write8(live_before, live_before_sentinel);
        write8(live_after, live_after_sentinel);

        goto_priv(PRIV_HS);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)masked_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        TEST_SETUP_EXCEPT();
        ai_micro_vsseg2e8_masked_from_v8((uint8_t *)protected_dst_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)live_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)live_dst_vaddr, (int)live_vl);
        fence_iorw_iorw();
        TEST_ASSERT("the all-inactive masked segment-store to an unwritable HS page retires without exception and does not block the immediate live follow-up segment store",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the protected target of the all-inactive masked segment-store stays byte-for-byte unchanged while the live follow-up segment store updates only its active prefix",
            ai_micro_segment_bytes_match(protected_dst_paddr,
                protected_seed[round], AI_MICRO_SEGMENT_BYTES) &&
            ai_micro_segment_prefix_and_seed_tail_match(live_dst_paddr,
                live_src_patterns[round], live_vl, live_seed[round])
        );
        TEST_ASSERT("the masked-inactive segment-store window also preserves the guard bytes around both the protected target and the live follow-up destination",
            read8(protected_before) == protected_before_sentinel &&
            read8(protected_after) == protected_after_sentinel &&
            read8(live_before) == live_before_sentinel &&
            read8(live_after) == live_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_store_all_inactive_masked_no_fault_or_side_effect_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_masked_all_inactive_store_window(false);
    TEST_END();
}

bool ai_micro_hs_vsegment_store_repeated_all_inactive_masked_then_live_store_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_masked_all_inactive_store_window(true);
    TEST_END();
}

static bool ai_micro_segment_partial_inactive_tail_owner_bound(bool repeat_rounds)
{
    static const uint8_t older_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x10, 0x80, 0x11, 0x81, 0x12, 0x82, 0x13, 0x83,
          0x14, 0x84, 0x15, 0x85, 0x16, 0x86, 0x17, 0x87 },
        { 0x21, 0x91, 0x22, 0x92, 0x23, 0x93, 0x24, 0x94,
          0x25, 0x95, 0x26, 0x96, 0x27, 0x97, 0x28, 0x98 },
        { 0x32, 0xa2, 0x33, 0xa3, 0x34, 0xa4, 0x35, 0xa5,
          0x36, 0xa6, 0x37, 0xa7, 0x38, 0xa8, 0x39, 0xa9 },
    };
    static const uint8_t younger_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x40, 0xb0, 0x41, 0xb1, 0x42, 0xb2, 0x43, 0xb3,
          0x44, 0xb4, 0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7 },
        { 0x51, 0xc1, 0x52, 0xc2, 0x53, 0xc3, 0x54, 0xc4,
          0x55, 0xc5, 0x56, 0xc6, 0x57, 0xc7, 0x58, 0xc8 },
        { 0x62, 0xd2, 0x63, 0xd3, 0x64, 0xd4, 0x65, 0xd5,
          0x66, 0xd6, 0x67, 0xd7, 0x68, 0xd8, 0x69, 0xd9 },
    };
    static const uint8_t rewrite_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x70, 0xe0, 0x71, 0xe1, 0x72, 0xe2, 0x73, 0xe3,
          0x74, 0xe4, 0x75, 0xe5, 0x76, 0xe6, 0x77, 0xe7 },
        { 0x81, 0xf1, 0x82, 0xf2, 0x83, 0xf3, 0x84, 0xf4,
          0x85, 0xf5, 0x86, 0xf6, 0x87, 0xf7, 0x88, 0xf8 },
        { 0x92, 0x02, 0x93, 0x03, 0x94, 0x04, 0x95, 0x05,
          0x96, 0x06, 0x97, 0x07, 0x98, 0x08, 0x99, 0x09 },
    };
    static const uint8_t dst0_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd0, 0x40, 0xd1, 0x41, 0xd2, 0x42, 0xd3, 0x43,
          0xd4, 0x44, 0xd5, 0x45, 0xd6, 0x46, 0xd7, 0x47 },
        { 0xc0, 0x50, 0xc1, 0x51, 0xc2, 0x52, 0xc3, 0x53,
          0xc4, 0x54, 0xc5, 0x55, 0xc6, 0x56, 0xc7, 0x57 },
        { 0xb0, 0x60, 0xb1, 0x61, 0xb2, 0x62, 0xb3, 0x63,
          0xb4, 0x64, 0xb5, 0x65, 0xb6, 0x66, 0xb7, 0x67 },
    };
    static const uint8_t dst1_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x9a, 0x0a, 0x9b, 0x0b, 0x9c, 0x0c, 0x9d, 0x0d,
          0x9e, 0x0e, 0x9f, 0x0f, 0x8a, 0x1a, 0x8b, 0x1b },
        { 0x7c, 0x2c, 0x7d, 0x2d, 0x7e, 0x2e, 0x7f, 0x2f,
          0x6c, 0x3c, 0x6d, 0x3d, 0x6e, 0x3e, 0x6f, 0x3f },
        { 0x5e, 0x4e, 0x5f, 0x4f, 0x4e, 0x5e, 0x4f, 0x5f,
          0x3e, 0x6e, 0x3f, 0x6f, 0x2e, 0x7e, 0x2f, 0x7f },
    };
    static const unsigned older_vls[] = { 3U, 5U, 2U };
    const uintptr_t older_src_vaddr = hs_page_base(SWITCH1) + 0x580U;
    const uintptr_t younger_src_vaddr = hs_page_base(SWITCH2) + 0x600U;
    const uintptr_t rewrite_src_vaddr = hs_page_base(SCRATCHPAD) + 0x680U;
    const uintptr_t dst0_vaddr = hs_page_base(VSRWX_GRWX) + 0x700U;
    const uintptr_t dst1_vaddr = hs_page_base(VSRWX_GRWX) + 0x740U;
    const uintptr_t older_src_paddr = phys_page_base(SWITCH1) + 0x580U;
    const uintptr_t younger_src_paddr = phys_page_base(SWITCH2) + 0x600U;
    const uintptr_t rewrite_src_paddr = phys_page_base(SCRATCHPAD) + 0x680U;
    const uintptr_t dst0_paddr = phys_page_base(VSRWX_GRWX) + 0x700U;
    const uintptr_t dst1_paddr = phys_page_base(VSRWX_GRWX) + 0x740U;
    const uintptr_t dst0_before = dst0_paddr - 1U;
    const uintptr_t dst0_after = dst0_paddr + AI_MICRO_SEGMENT_BYTES;
    const uintptr_t dst1_before = dst1_paddr - 1U;
    const uintptr_t dst1_after = dst1_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t dst0_before_sentinel = 0x4aU;
    const uint8_t dst0_after_sentinel = 0xa4U;
    const uint8_t dst1_before_sentinel = 0x5bU;
    const uint8_t dst1_after_sentinel = 0xb5U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(older_partial_src) / sizeof(older_partial_src[0])) :
        1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        const unsigned older_vl = older_vls[round];

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(older_src_paddr, older_partial_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(younger_src_paddr, younger_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(rewrite_src_paddr, rewrite_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst0_paddr, dst0_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst1_paddr, dst1_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(dst0_before, dst0_before_sentinel);
        write8(dst0_after, dst0_after_sentinel);
        write8(dst1_before, dst1_before_sentinel);
        write8(dst1_after, dst1_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vlseg2e8_to_v8((const uint8_t *)older_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst0_vaddr, (int)older_vl);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)younger_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst1_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)rewrite_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst0_vaddr,
            AI_MICRO_SEGMENT_VL);
        fence_iorw_iorw();
        TEST_ASSERT("the partial-inactive older segment store, adjacent younger full segment store, and same-target full rewrite all retire without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the adjacent younger full segment store keeps only its own payload while the later same-target full rewrite fully replaces the older partial-inactive tail state",
            ai_micro_segment_bytes_match(dst0_paddr, rewrite_full_src[round],
                AI_MICRO_SEGMENT_BYTES) &&
            ai_micro_segment_bytes_match(dst1_paddr, younger_full_src[round],
                AI_MICRO_SEGMENT_BYTES)
        );
        TEST_ASSERT("the partial-inactive segment-store ownership window also preserves adjacent sentinels around both targets",
            read8(dst0_before) == dst0_before_sentinel &&
            read8(dst0_after) == dst0_after_sentinel &&
            read8(dst1_before) == dst1_before_sentinel &&
            read8(dst1_after) == dst1_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_partial_inactive_tail_not_rebound_to_adjacent_younger_store_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_partial_inactive_tail_owner_bound(false);
    TEST_END();
}

bool ai_micro_hs_vsegment_partial_inactive_tail_repeated_same_target_full_rewrite_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_partial_inactive_tail_owner_bound(true);
    TEST_END();
}

static bool ai_micro_segment_same_target_partial_full_partial_window(
    bool check_adjacent)
{
    static const uint8_t older_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x10, 0x80, 0x11, 0x81, 0x12, 0x82, 0x13, 0x83,
          0x14, 0x84, 0x15, 0x85, 0x16, 0x86, 0x17, 0x87 },
        { 0x21, 0x91, 0x22, 0x92, 0x23, 0x93, 0x24, 0x94,
          0x25, 0x95, 0x26, 0x96, 0x27, 0x97, 0x28, 0x98 },
        { 0x32, 0xa2, 0x33, 0xa3, 0x34, 0xa4, 0x35, 0xa5,
          0x36, 0xa6, 0x37, 0xa7, 0x38, 0xa8, 0x39, 0xa9 },
    };
    static const uint8_t middle_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x40, 0xb0, 0x41, 0xb1, 0x42, 0xb2, 0x43, 0xb3,
          0x44, 0xb4, 0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7 },
        { 0x51, 0xc1, 0x52, 0xc2, 0x53, 0xc3, 0x54, 0xc4,
          0x55, 0xc5, 0x56, 0xc6, 0x57, 0xc7, 0x58, 0xc8 },
        { 0x62, 0xd2, 0x63, 0xd3, 0x64, 0xd4, 0x65, 0xd5,
          0x66, 0xd6, 0x67, 0xd7, 0x68, 0xd8, 0x69, 0xd9 },
    };
    static const uint8_t younger_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x70, 0xe0, 0x71, 0xe1, 0x72, 0xe2, 0x73, 0xe3,
          0x74, 0xe4, 0x75, 0xe5, 0x76, 0xe6, 0x77, 0xe7 },
        { 0x81, 0xf1, 0x82, 0xf2, 0x83, 0xf3, 0x84, 0xf4,
          0x85, 0xf5, 0x86, 0xf6, 0x87, 0xf7, 0x88, 0xf8 },
        { 0x92, 0x02, 0x93, 0x03, 0x94, 0x04, 0x95, 0x05,
          0x96, 0x06, 0x97, 0x07, 0x98, 0x08, 0x99, 0x09 },
    };
    static const uint8_t dst_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd0, 0x40, 0xd1, 0x41, 0xd2, 0x42, 0xd3, 0x43,
          0xd4, 0x44, 0xd5, 0x45, 0xd6, 0x46, 0xd7, 0x47 },
        { 0xc0, 0x50, 0xc1, 0x51, 0xc2, 0x52, 0xc3, 0x53,
          0xc4, 0x54, 0xc5, 0x55, 0xc6, 0x56, 0xc7, 0x57 },
        { 0xb0, 0x60, 0xb1, 0x61, 0xb2, 0x62, 0xb3, 0x63,
          0xb4, 0x64, 0xb5, 0x65, 0xb6, 0x66, 0xb7, 0x67 },
    };
    static const unsigned older_vls[] = { 3U, 5U, 2U };
    static const unsigned younger_vls[] = { 2U, 4U, 3U };
    const uintptr_t older_src_vaddr = hs_page_base(SWITCH1) + 0x580U;
    const uintptr_t middle_src_vaddr = hs_page_base(SWITCH2) + 0x600U;
    const uintptr_t younger_src_vaddr = hs_page_base(SCRATCHPAD) + 0x680U;
    const uintptr_t dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x700U;
    const uintptr_t older_src_paddr = phys_page_base(SWITCH1) + 0x580U;
    const uintptr_t middle_src_paddr = phys_page_base(SWITCH2) + 0x600U;
    const uintptr_t younger_src_paddr = phys_page_base(SCRATCHPAD) + 0x680U;
    const uintptr_t dst_paddr = phys_page_base(VSRWX_GRWX) + 0x700U;
    const uintptr_t dst_before = dst_paddr - 1U;
    const uintptr_t dst_after = dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t dst_before_sentinel = 0x4aU;
    const uint8_t dst_after_sentinel = 0xa4U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        const unsigned older_vl = older_vls[round];
        const unsigned younger_vl = younger_vls[round];

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(older_src_paddr, older_partial_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(middle_src_paddr, middle_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(younger_src_paddr,
            younger_partial_src[round], AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, dst_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(dst_before, dst_before_sentinel);
        write8(dst_after, dst_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vlseg2e8_to_v8((const uint8_t *)older_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)older_vl);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)middle_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)younger_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)younger_vl);
        fence_iorw_iorw();
        TEST_ASSERT("the same-target partial-full-partial segment-store generation window retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after an older partial segment-store, a middle full refresh, and a younger partial segment-store to the same target, the final image keeps only the younger active prefix plus the middle full tail with no oldest-tail leakage",
            ai_micro_segment_prefix_and_seed_tail_match(dst_paddr,
                younger_partial_src[round], younger_vl,
                middle_full_src[round])
        );
        if (check_adjacent) {
            TEST_ASSERT("the same-target partial-full-partial segment-store window also preserves the adjacent sentinels around the destination",
                read8(dst_before) == dst_before_sentinel &&
                read8(dst_after) == dst_after_sentinel
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_same_target_partial_full_partial_no_old_tail_leak_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_partial_full_partial_window(false);
    TEST_END();
}

bool ai_micro_hs_vsegment_same_target_partial_full_partial_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_partial_full_partial_window(true);
    TEST_END();
}

static bool ai_micro_segment_same_target_full_partial_full_window(
    bool check_adjacent)
{
    static const uint8_t older_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x14, 0x84, 0x15, 0x85, 0x16, 0x86, 0x17, 0x87,
          0x18, 0x88, 0x19, 0x89, 0x1a, 0x8a, 0x1b, 0x8b },
        { 0x25, 0x95, 0x26, 0x96, 0x27, 0x97, 0x28, 0x98,
          0x29, 0x99, 0x2a, 0x9a, 0x2b, 0x9b, 0x2c, 0x9c },
        { 0x36, 0xa6, 0x37, 0xa7, 0x38, 0xa8, 0x39, 0xa9,
          0x3a, 0xaa, 0x3b, 0xab, 0x3c, 0xac, 0x3d, 0xad },
    };
    static const uint8_t middle_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x40, 0xb0, 0x41, 0xb1, 0x42, 0xb2, 0x43, 0xb3,
          0x44, 0xb4, 0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7 },
        { 0x51, 0xc1, 0x52, 0xc2, 0x53, 0xc3, 0x54, 0xc4,
          0x55, 0xc5, 0x56, 0xc6, 0x57, 0xc7, 0x58, 0xc8 },
        { 0x62, 0xd2, 0x63, 0xd3, 0x64, 0xd4, 0x65, 0xd5,
          0x66, 0xd6, 0x67, 0xd7, 0x68, 0xd8, 0x69, 0xd9 },
    };
    static const uint8_t younger_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x74, 0xe4, 0x75, 0xe5, 0x76, 0xe6, 0x77, 0xe7,
          0x78, 0xe8, 0x79, 0xe9, 0x7a, 0xea, 0x7b, 0xeb },
        { 0x85, 0xf5, 0x86, 0xf6, 0x87, 0xf7, 0x88, 0xf8,
          0x89, 0xf9, 0x8a, 0xfa, 0x8b, 0xfb, 0x8c, 0xfc },
        { 0x96, 0x06, 0x97, 0x07, 0x98, 0x08, 0x99, 0x09,
          0x9a, 0x0a, 0x9b, 0x0b, 0x9c, 0x0c, 0x9d, 0x0d },
    };
    static const uint8_t dst_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd4, 0x44, 0xd5, 0x45, 0xd6, 0x46, 0xd7, 0x47,
          0xd8, 0x48, 0xd9, 0x49, 0xda, 0x4a, 0xdb, 0x4b },
        { 0xc5, 0x55, 0xc6, 0x56, 0xc7, 0x57, 0xc8, 0x58,
          0xc9, 0x59, 0xca, 0x5a, 0xcb, 0x5b, 0xcc, 0x5c },
        { 0xb6, 0x66, 0xb7, 0x67, 0xb8, 0x68, 0xb9, 0x69,
          0xba, 0x6a, 0xbb, 0x6b, 0xbc, 0x6c, 0xbd, 0x6d },
    };
    static const unsigned middle_vls[] = { 3U, 5U, 2U };
    const uintptr_t older_src_vaddr = hs_page_base(SWITCH1) + 0x780U;
    const uintptr_t middle_src_vaddr = hs_page_base(SWITCH2) + 0x800U;
    const uintptr_t younger_src_vaddr = hs_page_base(SCRATCHPAD) + 0x880U;
    const uintptr_t dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x900U;
    const uintptr_t older_src_paddr = phys_page_base(SWITCH1) + 0x780U;
    const uintptr_t middle_src_paddr = phys_page_base(SWITCH2) + 0x800U;
    const uintptr_t younger_src_paddr = phys_page_base(SCRATCHPAD) + 0x880U;
    const uintptr_t dst_paddr = phys_page_base(VSRWX_GRWX) + 0x900U;
    const uintptr_t dst_before = dst_paddr - 1U;
    const uintptr_t dst_after = dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t dst_before_sentinel = 0x5aU;
    const uint8_t dst_after_sentinel = 0xa5U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        const unsigned middle_vl = middle_vls[round];

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(older_src_paddr, older_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(middle_src_paddr,
            middle_partial_src[round], AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(younger_src_paddr, younger_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, dst_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(dst_before, dst_before_sentinel);
        write8(dst_after, dst_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vlseg2e8_to_v8((const uint8_t *)older_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)middle_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)middle_vl);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)younger_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        fence_iorw_iorw();
        TEST_ASSERT("the same-target full-partial-full segment-store generation window retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after an older full segment-store, a middle partial segment-store, and a younger full segment-store to the same target, the final image equals only the younger full payload with no middle-generation leak",
            ai_micro_segment_bytes_match(dst_paddr, younger_full_src[round],
                AI_MICRO_SEGMENT_BYTES)
        );
        if (check_adjacent) {
            TEST_ASSERT("the same-target full-partial-full segment-store window also preserves the adjacent sentinels around the destination",
                read8(dst_before) == dst_before_sentinel &&
                read8(dst_after) == dst_after_sentinel
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_same_target_full_partial_full_no_middle_tail_leak_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_full_partial_full_window(false);
    TEST_END();
}

bool ai_micro_hs_vsegment_same_target_full_partial_full_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_full_partial_full_window(true);
    TEST_END();
}

static bool ai_micro_segment_same_target_partial_partial_full_window(
    bool check_adjacent)
{
    static const uint8_t older_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x13, 0x83, 0x14, 0x84, 0x15, 0x85, 0x16, 0x86,
          0x17, 0x87, 0x18, 0x88, 0x19, 0x89, 0x1a, 0x8a },
        { 0x24, 0x94, 0x25, 0x95, 0x26, 0x96, 0x27, 0x97,
          0x28, 0x98, 0x29, 0x99, 0x2a, 0x9a, 0x2b, 0x9b },
        { 0x35, 0xa5, 0x36, 0xa6, 0x37, 0xa7, 0x38, 0xa8,
          0x39, 0xa9, 0x3a, 0xaa, 0x3b, 0xab, 0x3c, 0xac },
    };
    static const uint8_t middle_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x41, 0xb1, 0x42, 0xb2, 0x43, 0xb3, 0x44, 0xb4,
          0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7, 0x48, 0xb8 },
        { 0x52, 0xc2, 0x53, 0xc3, 0x54, 0xc4, 0x55, 0xc5,
          0x56, 0xc6, 0x57, 0xc7, 0x58, 0xc8, 0x59, 0xc9 },
        { 0x63, 0xd3, 0x64, 0xd4, 0x65, 0xd5, 0x66, 0xd6,
          0x67, 0xd7, 0x68, 0xd8, 0x69, 0xd9, 0x6a, 0xda },
    };
    static const uint8_t younger_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x74, 0xe4, 0x75, 0xe5, 0x76, 0xe6, 0x77, 0xe7,
          0x78, 0xe8, 0x79, 0xe9, 0x7a, 0xea, 0x7b, 0xeb },
        { 0x85, 0xf5, 0x86, 0xf6, 0x87, 0xf7, 0x88, 0xf8,
          0x89, 0xf9, 0x8a, 0xfa, 0x8b, 0xfb, 0x8c, 0xfc },
        { 0x96, 0x06, 0x97, 0x07, 0x98, 0x08, 0x99, 0x09,
          0x9a, 0x0a, 0x9b, 0x0b, 0x9c, 0x0c, 0x9d, 0x0d },
    };
    static const uint8_t dst_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd3, 0x43, 0xd4, 0x44, 0xd5, 0x45, 0xd6, 0x46,
          0xd7, 0x47, 0xd8, 0x48, 0xd9, 0x49, 0xda, 0x4a },
        { 0xc4, 0x54, 0xc5, 0x55, 0xc6, 0x56, 0xc7, 0x57,
          0xc8, 0x58, 0xc9, 0x59, 0xca, 0x5a, 0xcb, 0x5b },
        { 0xb5, 0x65, 0xb6, 0x66, 0xb7, 0x67, 0xb8, 0x68,
          0xb9, 0x69, 0xba, 0x6a, 0xbb, 0x6b, 0xbc, 0x6c },
    };
    static const unsigned older_vls[] = { 2U, 5U, 3U };
    static const unsigned middle_vls[] = { 5U, 3U, 4U };
    const uintptr_t older_src_vaddr = hs_page_base(SWITCH1) + 0x980U;
    const uintptr_t middle_src_vaddr = hs_page_base(SWITCH2) + 0xa00U;
    const uintptr_t younger_src_vaddr = hs_page_base(SCRATCHPAD) + 0xa80U;
    const uintptr_t dst_vaddr = hs_page_base(VSRWX_GRWX) + 0xb00U;
    const uintptr_t older_src_paddr = phys_page_base(SWITCH1) + 0x980U;
    const uintptr_t middle_src_paddr = phys_page_base(SWITCH2) + 0xa00U;
    const uintptr_t younger_src_paddr = phys_page_base(SCRATCHPAD) + 0xa80U;
    const uintptr_t dst_paddr = phys_page_base(VSRWX_GRWX) + 0xb00U;
    const uintptr_t dst_before = dst_paddr - 1U;
    const uintptr_t dst_after = dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t dst_before_sentinel = 0x6aU;
    const uint8_t dst_after_sentinel = 0xa6U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        const unsigned older_vl = older_vls[round];
        const unsigned middle_vl = middle_vls[round];

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(older_src_paddr, older_partial_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(middle_src_paddr,
            middle_partial_src[round], AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(younger_src_paddr, younger_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, dst_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(dst_before, dst_before_sentinel);
        write8(dst_after, dst_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vlseg2e8_to_v8((const uint8_t *)older_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)older_vl);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)middle_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)middle_vl);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)younger_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        fence_iorw_iorw();
        TEST_ASSERT("the same-target partial-partial-full segment-store generation window retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after two same-target partial segment-store generations are followed by a younger full segment-store rewrite, the final image equals only the younger full payload with no older or middle tail leak",
            ai_micro_segment_bytes_match(dst_paddr, younger_full_src[round],
                AI_MICRO_SEGMENT_BYTES)
        );
        if (check_adjacent) {
            TEST_ASSERT("the same-target partial-partial-full segment-store window also preserves the adjacent sentinels around the destination",
                read8(dst_before) == dst_before_sentinel &&
                read8(dst_after) == dst_after_sentinel
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_same_target_partial_partial_full_no_tail_accumulation_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_partial_partial_full_window(false);
    TEST_END();
}

bool ai_micro_hs_vsegment_same_target_partial_partial_full_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_partial_partial_full_window(true);
    TEST_END();
}

static bool ai_micro_segment_same_target_full_partial_partial_window(
    bool check_adjacent)
{
    static const uint8_t older_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x15, 0x85, 0x16, 0x86, 0x17, 0x87, 0x18, 0x88,
          0x19, 0x89, 0x1a, 0x8a, 0x1b, 0x8b, 0x1c, 0x8c },
        { 0x26, 0x96, 0x27, 0x97, 0x28, 0x98, 0x29, 0x99,
          0x2a, 0x9a, 0x2b, 0x9b, 0x2c, 0x9c, 0x2d, 0x9d },
        { 0x37, 0xa7, 0x38, 0xa8, 0x39, 0xa9, 0x3a, 0xaa,
          0x3b, 0xab, 0x3c, 0xac, 0x3d, 0xad, 0x3e, 0xae },
    };
    static const uint8_t middle_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x40, 0xb0, 0x41, 0xb1, 0x42, 0xb2, 0x43, 0xb3,
          0x44, 0xb4, 0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7 },
        { 0x51, 0xc1, 0x52, 0xc2, 0x53, 0xc3, 0x54, 0xc4,
          0x55, 0xc5, 0x56, 0xc6, 0x57, 0xc7, 0x58, 0xc8 },
        { 0x62, 0xd2, 0x63, 0xd3, 0x64, 0xd4, 0x65, 0xd5,
          0x66, 0xd6, 0x67, 0xd7, 0x68, 0xd8, 0x69, 0xd9 },
    };
    static const uint8_t younger_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x73, 0xe3, 0x74, 0xe4, 0x75, 0xe5, 0x76, 0xe6,
          0x77, 0xe7, 0x78, 0xe8, 0x79, 0xe9, 0x7a, 0xea },
        { 0x84, 0xf4, 0x85, 0xf5, 0x86, 0xf6, 0x87, 0xf7,
          0x88, 0xf8, 0x89, 0xf9, 0x8a, 0xfa, 0x8b, 0xfb },
        { 0x95, 0x05, 0x96, 0x06, 0x97, 0x07, 0x98, 0x08,
          0x99, 0x09, 0x9a, 0x0a, 0x9b, 0x0b, 0x9c, 0x0c },
    };
    static const uint8_t dst_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd5, 0x45, 0xd6, 0x46, 0xd7, 0x47, 0xd8, 0x48,
          0xd9, 0x49, 0xda, 0x4a, 0xdb, 0x4b, 0xdc, 0x4c },
        { 0xc6, 0x56, 0xc7, 0x57, 0xc8, 0x58, 0xc9, 0x59,
          0xca, 0x5a, 0xcb, 0x5b, 0xcc, 0x5c, 0xcd, 0x5d },
        { 0xb7, 0x67, 0xb8, 0x68, 0xb9, 0x69, 0xba, 0x6a,
          0xbb, 0x6b, 0xbc, 0x6c, 0xbd, 0x6d, 0xbe, 0x6e },
    };
    static const unsigned middle_vls[] = { 5U, 3U, 6U };
    static const unsigned younger_vls[] = { 2U, 5U, 3U };
    const uintptr_t older_src_vaddr = hs_page_base(SWITCH1) + 0xc00U;
    const uintptr_t middle_src_vaddr = hs_page_base(SWITCH2) + 0xc80U;
    const uintptr_t younger_src_vaddr = hs_page_base(SCRATCHPAD) + 0xd00U;
    const uintptr_t dst_vaddr = hs_page_base(VSRWX_GRWX) + 0xd80U;
    const uintptr_t older_src_paddr = phys_page_base(SWITCH1) + 0xc00U;
    const uintptr_t middle_src_paddr = phys_page_base(SWITCH2) + 0xc80U;
    const uintptr_t younger_src_paddr = phys_page_base(SCRATCHPAD) + 0xd00U;
    const uintptr_t dst_paddr = phys_page_base(VSRWX_GRWX) + 0xd80U;
    const uintptr_t dst_before = dst_paddr - 1U;
    const uintptr_t dst_after = dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t dst_before_sentinel = 0x7aU;
    const uint8_t dst_after_sentinel = 0xa7U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        const unsigned middle_vl = middle_vls[round];
        const unsigned younger_vl = younger_vls[round];
        uint8_t expected[AI_MICRO_SEGMENT_BYTES];

        for (unsigned i = 0; i < AI_MICRO_SEGMENT_BYTES; ++i) {
            expected[i] = older_full_src[round][i];
        }
        for (unsigned i = 0; i < middle_vl * AI_MICRO_SEGMENT_FIELDS; ++i) {
            expected[i] = middle_partial_src[round][i];
        }
        for (unsigned i = 0; i < younger_vl * AI_MICRO_SEGMENT_FIELDS; ++i) {
            expected[i] = younger_partial_src[round][i];
        }

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(older_src_paddr, older_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(middle_src_paddr,
            middle_partial_src[round], AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(younger_src_paddr,
            younger_partial_src[round], AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, dst_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(dst_before, dst_before_sentinel);
        write8(dst_after, dst_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vlseg2e8_to_v8((const uint8_t *)older_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)middle_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)middle_vl);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)younger_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)younger_vl);
        fence_iorw_iorw();
        TEST_ASSERT("the same-target full-partial-partial segment-store generation window retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after an older full segment-store is followed by middle and younger same-target partial segment-store generations, the final image keeps the exact byte-age layering with no stale owner mixup",
            ai_micro_segment_bytes_match(dst_paddr, expected,
                AI_MICRO_SEGMENT_BYTES)
        );
        if (check_adjacent) {
            TEST_ASSERT("the same-target full-partial-partial segment-store window also preserves the adjacent sentinels around the destination",
                read8(dst_before) == dst_before_sentinel &&
                read8(dst_after) == dst_after_sentinel
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_same_target_full_partial_partial_keeps_precise_byte_age_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_full_partial_partial_window(false);
    TEST_END();
}

bool ai_micro_hs_vsegment_same_target_full_partial_partial_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_full_partial_partial_window(true);
    TEST_END();
}

static bool ai_micro_segment_same_target_partial_partial_partial_window(
    bool check_adjacent)
{
    static const uint8_t older_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x12, 0x82, 0x13, 0x83, 0x14, 0x84, 0x15, 0x85,
          0x16, 0x86, 0x17, 0x87, 0x18, 0x88, 0x19, 0x89 },
        { 0x23, 0x93, 0x24, 0x94, 0x25, 0x95, 0x26, 0x96,
          0x27, 0x97, 0x28, 0x98, 0x29, 0x99, 0x2a, 0x9a },
        { 0x34, 0xa4, 0x35, 0xa5, 0x36, 0xa6, 0x37, 0xa7,
          0x38, 0xa8, 0x39, 0xa9, 0x3a, 0xaa, 0x3b, 0xab },
    };
    static const uint8_t middle_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x40, 0xb0, 0x41, 0xb1, 0x42, 0xb2, 0x43, 0xb3,
          0x44, 0xb4, 0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7 },
        { 0x51, 0xc1, 0x52, 0xc2, 0x53, 0xc3, 0x54, 0xc4,
          0x55, 0xc5, 0x56, 0xc6, 0x57, 0xc7, 0x58, 0xc8 },
        { 0x62, 0xd2, 0x63, 0xd3, 0x64, 0xd4, 0x65, 0xd5,
          0x66, 0xd6, 0x67, 0xd7, 0x68, 0xd8, 0x69, 0xd9 },
    };
    static const uint8_t younger_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x73, 0xe3, 0x74, 0xe4, 0x75, 0xe5, 0x76, 0xe6,
          0x77, 0xe7, 0x78, 0xe8, 0x79, 0xe9, 0x7a, 0xea },
        { 0x84, 0xf4, 0x85, 0xf5, 0x86, 0xf6, 0x87, 0xf7,
          0x88, 0xf8, 0x89, 0xf9, 0x8a, 0xfa, 0x8b, 0xfb },
        { 0x95, 0x05, 0x96, 0x06, 0x97, 0x07, 0x98, 0x08,
          0x99, 0x09, 0x9a, 0x0a, 0x9b, 0x0b, 0x9c, 0x0c },
    };
    static const uint8_t dst_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd2, 0x42, 0xd3, 0x43, 0xd4, 0x44, 0xd5, 0x45,
          0xd6, 0x46, 0xd7, 0x47, 0xd8, 0x48, 0xd9, 0x49 },
        { 0xc3, 0x53, 0xc4, 0x54, 0xc5, 0x55, 0xc6, 0x56,
          0xc7, 0x57, 0xc8, 0x58, 0xc9, 0x59, 0xca, 0x5a },
        { 0xb4, 0x64, 0xb5, 0x65, 0xb6, 0x66, 0xb7, 0x67,
          0xb8, 0x68, 0xb9, 0x69, 0xba, 0x6a, 0xbb, 0x6b },
    };
    static const unsigned older_vls[] = { 6U, 5U, 4U };
    static const unsigned middle_vls[] = { 4U, 3U, 2U };
    static const unsigned younger_vls[] = { 2U, 1U, 3U };
    const uintptr_t older_src_vaddr = hs_page_base(SWITCH1) + 0xe00U;
    const uintptr_t middle_src_vaddr = hs_page_base(SWITCH2) + 0xe80U;
    const uintptr_t younger_src_vaddr = hs_page_base(SCRATCHPAD) + 0xf00U;
    const uintptr_t dst_vaddr = hs_page_base(VSRWX_GRWX) + 0xf80U;
    const uintptr_t older_src_paddr = phys_page_base(SWITCH1) + 0xe00U;
    const uintptr_t middle_src_paddr = phys_page_base(SWITCH2) + 0xe80U;
    const uintptr_t younger_src_paddr = phys_page_base(SCRATCHPAD) + 0xf00U;
    const uintptr_t dst_paddr = phys_page_base(VSRWX_GRWX) + 0xf80U;
    const uintptr_t dst_before = dst_paddr - 1U;
    const uintptr_t dst_after = dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t dst_before_sentinel = 0x8aU;
    const uint8_t dst_after_sentinel = 0xa8U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        const unsigned older_vl = older_vls[round];
        const unsigned middle_vl = middle_vls[round];
        const unsigned younger_vl = younger_vls[round];
        uint8_t expected[AI_MICRO_SEGMENT_BYTES];

        for (unsigned i = 0; i < AI_MICRO_SEGMENT_BYTES; ++i) {
            expected[i] = dst_seed[round][i];
        }
        for (unsigned i = 0; i < older_vl * AI_MICRO_SEGMENT_FIELDS; ++i) {
            expected[i] = older_partial_src[round][i];
        }
        for (unsigned i = 0; i < middle_vl * AI_MICRO_SEGMENT_FIELDS; ++i) {
            expected[i] = middle_partial_src[round][i];
        }
        for (unsigned i = 0; i < younger_vl * AI_MICRO_SEGMENT_FIELDS; ++i) {
            expected[i] = younger_partial_src[round][i];
        }

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(older_src_paddr, older_partial_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(middle_src_paddr,
            middle_partial_src[round], AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(younger_src_paddr,
            younger_partial_src[round], AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, dst_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(dst_before, dst_before_sentinel);
        write8(dst_after, dst_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vlseg2e8_to_v8((const uint8_t *)older_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)older_vl);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)middle_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)middle_vl);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)younger_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)younger_vl);
        fence_iorw_iorw();
        TEST_ASSERT("the same-target partial-partial-partial segment-store generation window retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after three same-target partial segment-store generations with shrinking active ranges, the final image keeps exact byte age across younger, middle, older, and untouched tail bytes",
            ai_micro_segment_bytes_match(dst_paddr, expected,
                AI_MICRO_SEGMENT_BYTES)
        );
        if (check_adjacent) {
            TEST_ASSERT("the same-target partial-partial-partial segment-store window also preserves the adjacent sentinels around the destination",
                read8(dst_before) == dst_before_sentinel &&
                read8(dst_after) == dst_after_sentinel
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_same_target_partial_partial_partial_keeps_precise_byte_age_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_partial_partial_partial_window(false);
    TEST_END();
}

bool ai_micro_hs_vsegment_same_target_partial_partial_partial_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_partial_partial_partial_window(true);
    TEST_END();
}

static bool ai_micro_segment_same_target_partial_full_full_window(
    bool check_adjacent)
{
    static const uint8_t older_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x11, 0x81, 0x12, 0x82, 0x13, 0x83, 0x14, 0x84,
          0x15, 0x85, 0x16, 0x86, 0x17, 0x87, 0x18, 0x88 },
        { 0x22, 0x92, 0x23, 0x93, 0x24, 0x94, 0x25, 0x95,
          0x26, 0x96, 0x27, 0x97, 0x28, 0x98, 0x29, 0x99 },
        { 0x33, 0xa3, 0x34, 0xa4, 0x35, 0xa5, 0x36, 0xa6,
          0x37, 0xa7, 0x38, 0xa8, 0x39, 0xa9, 0x3a, 0xaa },
    };
    static const uint8_t middle_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x40, 0xb0, 0x41, 0xb1, 0x42, 0xb2, 0x43, 0xb3,
          0x44, 0xb4, 0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7 },
        { 0x51, 0xc1, 0x52, 0xc2, 0x53, 0xc3, 0x54, 0xc4,
          0x55, 0xc5, 0x56, 0xc6, 0x57, 0xc7, 0x58, 0xc8 },
        { 0x62, 0xd2, 0x63, 0xd3, 0x64, 0xd4, 0x65, 0xd5,
          0x66, 0xd6, 0x67, 0xd7, 0x68, 0xd8, 0x69, 0xd9 },
    };
    static const uint8_t younger_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x74, 0xe4, 0x75, 0xe5, 0x76, 0xe6, 0x77, 0xe7,
          0x78, 0xe8, 0x79, 0xe9, 0x7a, 0xea, 0x7b, 0xeb },
        { 0x85, 0xf5, 0x86, 0xf6, 0x87, 0xf7, 0x88, 0xf8,
          0x89, 0xf9, 0x8a, 0xfa, 0x8b, 0xfb, 0x8c, 0xfc },
        { 0x96, 0x06, 0x97, 0x07, 0x98, 0x08, 0x99, 0x09,
          0x9a, 0x0a, 0x9b, 0x0b, 0x9c, 0x0c, 0x9d, 0x0d },
    };
    static const uint8_t dst_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd1, 0x41, 0xd2, 0x42, 0xd3, 0x43, 0xd4, 0x44,
          0xd5, 0x45, 0xd6, 0x46, 0xd7, 0x47, 0xd8, 0x48 },
        { 0xc2, 0x52, 0xc3, 0x53, 0xc4, 0x54, 0xc5, 0x55,
          0xc6, 0x56, 0xc7, 0x57, 0xc8, 0x58, 0xc9, 0x59 },
        { 0xb3, 0x63, 0xb4, 0x64, 0xb5, 0x65, 0xb6, 0x66,
          0xb7, 0x67, 0xb8, 0x68, 0xb9, 0x69, 0xba, 0x6a },
    };
    static const unsigned older_vls[] = { 3U, 5U, 2U };
    const uintptr_t older_src_vaddr = hs_page_base(SWITCH1) + 0x1000U;
    const uintptr_t middle_src_vaddr = hs_page_base(SWITCH2) + 0x1080U;
    const uintptr_t younger_src_vaddr = hs_page_base(SCRATCHPAD) + 0x1100U;
    const uintptr_t dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x1180U;
    const uintptr_t older_src_paddr = phys_page_base(SWITCH1) + 0x1000U;
    const uintptr_t middle_src_paddr = phys_page_base(SWITCH2) + 0x1080U;
    const uintptr_t younger_src_paddr = phys_page_base(SCRATCHPAD) + 0x1100U;
    const uintptr_t dst_paddr = phys_page_base(VSRWX_GRWX) + 0x1180U;
    const uintptr_t dst_before = dst_paddr - 1U;
    const uintptr_t dst_after = dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t dst_before_sentinel = 0x9aU;
    const uint8_t dst_after_sentinel = 0xa9U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        const unsigned older_vl = older_vls[round];

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(older_src_paddr, older_partial_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(middle_src_paddr, middle_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(younger_src_paddr, younger_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, dst_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(dst_before, dst_before_sentinel);
        write8(dst_after, dst_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vlseg2e8_to_v8((const uint8_t *)older_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)older_vl);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)middle_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)younger_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        fence_iorw_iorw();
        TEST_ASSERT("the same-target partial-full-full segment-store generation window retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after an older partial segment-store is followed by two same-target full rewrites, the final image equals only the youngest full payload with no stale tail survival",
            ai_micro_segment_bytes_match(dst_paddr, younger_full_src[round],
                AI_MICRO_SEGMENT_BYTES)
        );
        if (check_adjacent) {
            TEST_ASSERT("the same-target partial-full-full segment-store window also preserves the adjacent sentinels around the destination",
                read8(dst_before) == dst_before_sentinel &&
                read8(dst_after) == dst_after_sentinel
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_same_target_partial_full_full_no_stale_tail_after_double_full_rewrite_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_partial_full_full_window(false);
    TEST_END();
}

bool ai_micro_hs_vsegment_same_target_partial_full_full_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_partial_full_full_window(true);
    TEST_END();
}

static bool ai_micro_segment_same_target_full_full_partial_window(
    bool check_adjacent)
{
    static const uint8_t older_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x16, 0x86, 0x17, 0x87, 0x18, 0x88, 0x19, 0x89,
          0x1a, 0x8a, 0x1b, 0x8b, 0x1c, 0x8c, 0x1d, 0x8d },
        { 0x27, 0x97, 0x28, 0x98, 0x29, 0x99, 0x2a, 0x9a,
          0x2b, 0x9b, 0x2c, 0x9c, 0x2d, 0x9d, 0x2e, 0x9e },
        { 0x38, 0xa8, 0x39, 0xa9, 0x3a, 0xaa, 0x3b, 0xab,
          0x3c, 0xac, 0x3d, 0xad, 0x3e, 0xae, 0x3f, 0xaf },
    };
    static const uint8_t middle_full_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x40, 0xb0, 0x41, 0xb1, 0x42, 0xb2, 0x43, 0xb3,
          0x44, 0xb4, 0x45, 0xb5, 0x46, 0xb6, 0x47, 0xb7 },
        { 0x51, 0xc1, 0x52, 0xc2, 0x53, 0xc3, 0x54, 0xc4,
          0x55, 0xc5, 0x56, 0xc6, 0x57, 0xc7, 0x58, 0xc8 },
        { 0x62, 0xd2, 0x63, 0xd3, 0x64, 0xd4, 0x65, 0xd5,
          0x66, 0xd6, 0x67, 0xd7, 0x68, 0xd8, 0x69, 0xd9 },
    };
    static const uint8_t younger_partial_src[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x73, 0xe3, 0x74, 0xe4, 0x75, 0xe5, 0x76, 0xe6,
          0x77, 0xe7, 0x78, 0xe8, 0x79, 0xe9, 0x7a, 0xea },
        { 0x84, 0xf4, 0x85, 0xf5, 0x86, 0xf6, 0x87, 0xf7,
          0x88, 0xf8, 0x89, 0xf9, 0x8a, 0xfa, 0x8b, 0xfb },
        { 0x95, 0x05, 0x96, 0x06, 0x97, 0x07, 0x98, 0x08,
          0x99, 0x09, 0x9a, 0x0a, 0x9b, 0x0b, 0x9c, 0x0c },
    };
    static const uint8_t dst_seed[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd6, 0x46, 0xd7, 0x47, 0xd8, 0x48, 0xd9, 0x49,
          0xda, 0x4a, 0xdb, 0x4b, 0xdc, 0x4c, 0xdd, 0x4d },
        { 0xc7, 0x57, 0xc8, 0x58, 0xc9, 0x59, 0xca, 0x5a,
          0xcb, 0x5b, 0xcc, 0x5c, 0xcd, 0x5d, 0xce, 0x5e },
        { 0xb8, 0x68, 0xb9, 0x69, 0xba, 0x6a, 0xbb, 0x6b,
          0xbc, 0x6c, 0xbd, 0x6d, 0xbe, 0x6e, 0xbf, 0x6f },
    };
    static const unsigned younger_vls[] = { 2U, 5U, 3U };
    const uintptr_t older_src_vaddr = hs_page_base(SWITCH1) + 0x1200U;
    const uintptr_t middle_src_vaddr = hs_page_base(SWITCH2) + 0x1280U;
    const uintptr_t younger_src_vaddr = hs_page_base(SCRATCHPAD) + 0x1300U;
    const uintptr_t dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x1380U;
    const uintptr_t older_src_paddr = phys_page_base(SWITCH1) + 0x1200U;
    const uintptr_t middle_src_paddr = phys_page_base(SWITCH2) + 0x1280U;
    const uintptr_t younger_src_paddr = phys_page_base(SCRATCHPAD) + 0x1300U;
    const uintptr_t dst_paddr = phys_page_base(VSRWX_GRWX) + 0x1380U;
    const uintptr_t dst_before = dst_paddr - 1U;
    const uintptr_t dst_after = dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t dst_before_sentinel = 0xaaU;
    const uint8_t dst_after_sentinel = 0xabU;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        const unsigned younger_vl = younger_vls[round];

        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(older_src_paddr, older_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(middle_src_paddr, middle_full_src[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(younger_src_paddr,
            younger_partial_src[round], AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, dst_seed[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(dst_before, dst_before_sentinel);
        write8(dst_after, dst_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vlseg2e8_to_v8((const uint8_t *)older_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)middle_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)younger_src_vaddr,
            AI_MICRO_SEGMENT_VL);
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, (int)younger_vl);
        fence_iorw_iorw();
        TEST_ASSERT("the same-target full-full-partial segment-store generation window retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after two same-target full rewrites are followed by a younger partial segment-store, the final image keeps only the younger prefix plus the middle full tail with no oldest-full leak",
            ai_micro_segment_prefix_and_seed_tail_match(dst_paddr,
                younger_partial_src[round], younger_vl,
                middle_full_src[round])
        );
        if (check_adjacent) {
            TEST_ASSERT("the same-target full-full-partial segment-store window also preserves the adjacent sentinels around the destination",
                read8(dst_before) == dst_before_sentinel &&
                read8(dst_after) == dst_after_sentinel
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsegment_same_target_full_full_partial_no_oldest_full_leak_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_full_full_partial_window(false);
    TEST_END();
}

bool ai_micro_hs_vsegment_same_target_full_full_partial_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_segment_same_target_full_full_partial_window(true);
    TEST_END();
}

static bool ai_micro_same_cacheline_two_word_unitstride_store_window(void)
{
    static const uint8_t src_patterns[][AI_MICRO_VECTOR_BYTES] = {
        { 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe,
          0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01 },
        { 0x21, 0x43, 0x65, 0x87, 0xa9, 0xcb, 0xed, 0x0f,
          0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12 },
        { 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0xff, 0xee,
          0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66 },
    };
    static const uint8_t seed_patterns[][AI_MICRO_VECTOR_BYTES] = {
        { 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
          0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf },
        { 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
          0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf },
        { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
          0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf },
    };
    const uintptr_t src_vaddr = hs_page_base(SWITCH1) + 0x740U;
    const uintptr_t dst_vaddr = hs_page_base(SCRATCHPAD) + 0x708U;
    const uintptr_t src_paddr = phys_page_base(SWITCH1) + 0x740U;
    const uintptr_t dst_paddr = phys_page_base(SCRATCHPAD) + 0x708U;
    const uintptr_t before = dst_paddr - 1U;
    const uintptr_t after = dst_paddr + AI_MICRO_VECTOR_BYTES;
    const uint8_t before_sentinel = 0x3aU;
    const uint8_t after_sentinel = 0xa3U;
    const unsigned rounds =
        (unsigned)(sizeof(src_patterns) / sizeof(src_patterns[0]));
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(src_paddr, src_patterns[round],
            AI_MICRO_VECTOR_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, seed_patterns[round],
            AI_MICRO_VECTOR_BYTES);
        write8(before, before_sentinel);
        write8(after, after_sentinel);

        goto_priv(PRIV_HS);
        vle8_to_v6((const uint8_t *)src_vaddr, AI_MICRO_VECTOR_BYTES);
        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)dst_vaddr, AI_MICRO_VECTOR_BYTES);
        fence_iorw_iorw();
        TEST_ASSERT("the same-cacheline two-word unit-stride vector store retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-cacheline two-word unit-stride vector store preserves the exact 16-byte payload with no dropped or mixed half",
            ai_micro_segment_bytes_match(dst_paddr, src_patterns[round],
                AI_MICRO_VECTOR_BYTES)
        );
        TEST_ASSERT("the same-cacheline two-word unit-stride vector store also preserves adjacent sentinel bytes",
            read8(before) == before_sentinel &&
            read8(after) == after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_same_cacheline_two_word_segment_store_window(void)
{
    static const uint8_t src_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0x10, 0x80, 0x11, 0x81, 0x12, 0x82, 0x13, 0x83,
          0x14, 0x84, 0x15, 0x85, 0x16, 0x86, 0x17, 0x87 },
        { 0x21, 0x91, 0x22, 0x92, 0x23, 0x93, 0x24, 0x94,
          0x25, 0x95, 0x26, 0x96, 0x27, 0x97, 0x28, 0x98 },
        { 0x32, 0xa2, 0x33, 0xa3, 0x34, 0xa4, 0x35, 0xa5,
          0x36, 0xa6, 0x37, 0xa7, 0x38, 0xa8, 0x39, 0xa9 },
    };
    static const uint8_t seed_patterns[][AI_MICRO_SEGMENT_BYTES] = {
        { 0xd0, 0x40, 0xd1, 0x41, 0xd2, 0x42, 0xd3, 0x43,
          0xd4, 0x44, 0xd5, 0x45, 0xd6, 0x46, 0xd7, 0x47 },
        { 0xc0, 0x50, 0xc1, 0x51, 0xc2, 0x52, 0xc3, 0x53,
          0xc4, 0x54, 0xc5, 0x55, 0xc6, 0x56, 0xc7, 0x57 },
        { 0xb0, 0x60, 0xb1, 0x61, 0xb2, 0x62, 0xb3, 0x63,
          0xb4, 0x64, 0xb5, 0x65, 0xb6, 0x66, 0xb7, 0x67 },
    };
    const uintptr_t src_vaddr = hs_page_base(SWITCH2) + 0x780U;
    const uintptr_t dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x748U;
    const uintptr_t src_paddr = phys_page_base(SWITCH2) + 0x780U;
    const uintptr_t dst_paddr = phys_page_base(VSRWX_GRWX) + 0x748U;
    const uintptr_t before = dst_paddr - 1U;
    const uintptr_t after = dst_paddr + AI_MICRO_SEGMENT_BYTES;
    const uint8_t before_sentinel = 0x5cU;
    const uint8_t after_sentinel = 0xc5U;
    const unsigned rounds =
        (unsigned)(sizeof(src_patterns) / sizeof(src_patterns[0]));
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        ai_micro_segment_write_bytes(src_paddr, src_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        ai_micro_segment_write_bytes(dst_paddr, seed_patterns[round],
            AI_MICRO_SEGMENT_BYTES);
        write8(before, before_sentinel);
        write8(after, after_sentinel);

        goto_priv(PRIV_HS);
        ai_micro_vlseg2e8_to_v8((const uint8_t *)src_vaddr,
            AI_MICRO_SEGMENT_VL);
        TEST_SETUP_EXCEPT();
        ai_micro_vsseg2e8_from_v8((uint8_t *)dst_vaddr, AI_MICRO_SEGMENT_VL);
        fence_iorw_iorw();
        TEST_ASSERT("the same-cacheline two-word segment store retires without exception before payload checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-cacheline two-word segment store preserves the exact 16-byte interleaved payload with no dropped or mixed half",
            ai_micro_segment_bytes_match(dst_paddr, src_patterns[round],
                AI_MICRO_SEGMENT_BYTES)
        );
        TEST_ASSERT("the same-cacheline two-word segment store also preserves adjacent sentinel bytes",
            read8(before) == before_sentinel &&
            read8(after) == after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_sbuffer_same_cacheline_unitstride_vector_store_two_words_no_drop_or_mix_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_cacheline_two_word_unitstride_store_window();
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_cacheline_segment_store_two_words_no_drop_or_mix_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_cacheline_two_word_segment_store_window();
    TEST_END();
}
