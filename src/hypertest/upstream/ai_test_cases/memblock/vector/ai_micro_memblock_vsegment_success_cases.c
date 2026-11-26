#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_MEMBLOCK_VSEG_SUCCESS_FIELDS 2U
#define AI_MEMBLOCK_VSEG_SUCCESS_VL 8U
#define AI_MEMBLOCK_VSEG_SUCCESS_BYTES \
    (AI_MEMBLOCK_VSEG_SUCCESS_FIELDS * AI_MEMBLOCK_VSEG_SUCCESS_VL)
#define AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS 3U

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline pte_t *ai_memblock_vseg_success_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_vseg_success_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_memblock_vseg_success_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_memblock_vseg_success_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_memblock_vseg_success_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_vseg_success_prefill(uintptr_t paddr,
    uint8_t fill, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, fill);
    }
}

static inline void ai_memblock_vseg_then_scalar_window(uintptr_t seg_src,
    uintptr_t seg_dst,
    uintptr_t scalar_src,
    uintptr_t scalar_dst,
    uint64_t scalar_update,
    uint64_t *scalar_observed,
    uint64_t *scalar_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t observed = 0ULL;
    uint64_t reloaded = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "ld %[observed], 0(%[scalar_src])\n\t"
        "sd %[scalar_update], 0(%[scalar_dst])\n\t"
        "ld %[reloaded], 0(%[scalar_dst])\n\t"
        ".option pop\n\t"
        : [observed] "=&r" (observed),
          [reloaded] "=&r" (reloaded)
        : [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [scalar_src] "r" (scalar_src),
          [scalar_dst] "r" (scalar_dst),
          [scalar_update] "r" (scalar_update),
          [vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *scalar_observed = observed;
    *scalar_reloaded = reloaded;
}

bool ai_micro_hs_vsegment_success_then_scalar_load_store_no_preempt_leak_corner()
{
    TEST_START();

    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0x11, 0xa1, 0x12, 0xa2, 0x13, 0xa3, 0x14, 0xa4,
          0x15, 0xa5, 0x16, 0xa6, 0x17, 0xa7, 0x18, 0xa8 },
        { 0x21, 0xb1, 0x22, 0xb2, 0x23, 0xb3, 0x24, 0xb4,
          0x25, 0xb5, 0x26, 0xb6, 0x27, 0xb7, 0x28, 0xb8 },
        { 0x31, 0xc1, 0x32, 0xc2, 0x33, 0xc3, 0x34, 0xc4,
          0x35, 0xc5, 0x36, 0xc6, 0x37, 0xc7, 0x38, 0xc8 },
    };
    static const uint64_t scalar_src_values[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0xa1b2c3d4e5f60718ULL,
    };
    static const uint64_t scalar_dst_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    static const uint64_t scalar_updates[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x55aa33cc77ee11ffULL,
    };
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page scalar_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x300U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0x500U;
    const uintptr_t scalar_src_vaddr = hs_page_base(scalar_page) + 0x280U;
    const uintptr_t scalar_dst_vaddr = hs_page_base(scalar_page) + 0x2c0U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x300U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0x500U;
    const uintptr_t scalar_src_paddr = phys_page_base(scalar_page) + 0x280U;
    const uintptr_t scalar_dst_paddr = phys_page_base(scalar_page) + 0x2c0U;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t scalar_before_paddr = scalar_dst_paddr - sizeof(uint64_t);
    const uintptr_t scalar_after_paddr = scalar_dst_paddr + sizeof(uint64_t);
    const uint8_t seg_fill = 0x4dU;
    const uint8_t seg_before = 0xd4U;
    const uint8_t seg_after = 0x2bU;
    const uint64_t scalar_before = 0xfeedfacecafebeefULL;
    const uint64_t scalar_after = 0xdeadc0def00dbabeULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        uint64_t scalar_observed = 0ULL;
        uint64_t scalar_reloaded = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write64(scalar_src_paddr, scalar_src_values[round]);
        write64(scalar_dst_paddr, scalar_dst_seeds[round]);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);
        write64(scalar_before_paddr, scalar_before);
        write64(scalar_after_paddr, scalar_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(scalar_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vseg_then_scalar_window(seg_src_vaddr, seg_dst_vaddr,
            scalar_src_vaddr, scalar_dst_vaddr, scalar_updates[round],
            &scalar_observed, &scalar_reloaded);
        fence_iorw_iorw();

        AI_ASSERT_NO_EXCEPT_AND("after a successful vsegment load/store, the immediately following scalar load/store/load window is not held by stale segment preempt state",
            scalar_observed == scalar_src_values[round] &&
            scalar_reloaded == scalar_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the successful vsegment destination and the following scalar store each keep their own payloads without dcache or sbuffer cross-binding",
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            read64(scalar_src_paddr) == scalar_src_values[round] &&
            read64(scalar_dst_paddr) == scalar_updates[round]
        );
        TEST_ASSERT("the successful vsegment followed by scalar memory window preserves all segment and scalar guard bytes",
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after &&
            read64(scalar_before_paddr) == scalar_before &&
            read64(scalar_after_paddr) == scalar_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_vseg_store_fence_marker_window(
    uintptr_t seg_src,
    uintptr_t seg_dst,
    uintptr_t marker_dst,
    uint64_t marker_update,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t reloaded = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "fence iorw, iorw\n\t"
        "sd %[marker_update], 0(%[marker_dst])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[reloaded], 0(%[marker_dst])\n\t"
        ".option pop\n\t"
        : [reloaded] "=&r" (reloaded)
        : [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [marker_dst] "r" (marker_dst),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *marker_reloaded = reloaded;
}

static inline void ai_memblock_fence_then_vseg_store_marker_window(
    uintptr_t pressure_dst,
    uint64_t pressure_update,
    uintptr_t seg_src,
    uintptr_t seg_dst,
    uintptr_t marker_dst,
    uint64_t marker_update,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t reloaded = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[pressure_update], 0(%[pressure_dst])\n\t"
        "fence iorw, iorw\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "sd %[marker_update], 0(%[marker_dst])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[reloaded], 0(%[marker_dst])\n\t"
        ".option pop\n\t"
        : [reloaded] "=&r" (reloaded)
        : [pressure_dst] "r" (pressure_dst),
          [pressure_update] "r" (pressure_update),
          [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [marker_dst] "r" (marker_dst),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *marker_reloaded = reloaded;
}

bool ai_micro_hs_vsegment_store_then_fence_no_stale_sbisempty_or_scalar_drop_corner()
{
    TEST_START();

    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0x41, 0xd1, 0x42, 0xd2, 0x43, 0xd3, 0x44, 0xd4,
          0x45, 0xd5, 0x46, 0xd6, 0x47, 0xd7, 0x48, 0xd8 },
        { 0x51, 0xe1, 0x52, 0xe2, 0x53, 0xe3, 0x54, 0xe4,
          0x55, 0xe5, 0x56, 0xe6, 0x57, 0xe7, 0x58, 0xe8 },
        { 0x61, 0xf1, 0x62, 0xf2, 0x63, 0xf3, 0x64, 0xf4,
          0x65, 0xf5, 0x66, 0xf6, 0x67, 0xf7, 0x68, 0xf8 },
    };
    static const uint64_t marker_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    static const uint64_t marker_updates[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x8899aabbccddeeffULL,
        0x1029384756aabbccULL,
        0x7766554433221100ULL,
    };
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x680U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0x700U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x780U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x680U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0x700U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x780U;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t marker_before_paddr = marker_paddr - sizeof(uint64_t);
    const uintptr_t marker_after_paddr = marker_paddr + sizeof(uint64_t);
    const uint8_t seg_fill = 0x9aU;
    const uint8_t seg_before = 0x36U;
    const uint8_t seg_after = 0x63U;
    const uint64_t marker_before = 0x1234fedc5678ba90ULL;
    const uint64_t marker_after = 0x0badc0de55aa33ccULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write64(marker_paddr, marker_seeds[round]);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);
        write64(marker_before_paddr, marker_before);
        write64(marker_after_paddr, marker_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(marker_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vseg_store_fence_marker_window(seg_src_vaddr,
            seg_dst_vaddr, marker_vaddr, marker_updates[round],
            &marker_reloaded);

        AI_ASSERT_NO_EXCEPT_AND("a vsegment store followed by an immediate fence does not complete on stale sbIsEmpty or drop the following scalar marker store",
            marker_reloaded == marker_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the vsegment-store then fence boundary preserves the segment payload and the post-fence scalar marker payload independently",
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            read64(marker_paddr) == marker_updates[round]
        );
        TEST_ASSERT("the vsegment-store then fence boundary preserves guards around both the segment destination and scalar marker",
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after &&
            read64(marker_before_paddr) == marker_before &&
            read64(marker_after_paddr) == marker_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fence_then_vsegment_store_no_stale_fence_owner_or_segment_payload_corner()
{
    TEST_START();

    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0x71, 0xa7, 0x72, 0xb7, 0x73, 0xc7, 0x74, 0xd7,
          0x75, 0xe7, 0x76, 0xf7, 0x77, 0x87, 0x78, 0x97 },
        { 0x81, 0xa8, 0x82, 0xb8, 0x83, 0xc8, 0x84, 0xd8,
          0x85, 0xe8, 0x86, 0xf8, 0x87, 0x88, 0x88, 0x98 },
        { 0x91, 0xa9, 0x92, 0xb9, 0x93, 0xc9, 0x94, 0xd9,
          0x95, 0xe9, 0x96, 0xf9, 0x97, 0x89, 0x98, 0x99 },
    };
    static const uint64_t pressure_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    static const uint64_t pressure_updates[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x8899aabbccddeeffULL,
        0x1029384756aabbccULL,
        0x7766554433221100ULL,
    };
    static const uint64_t marker_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    static const uint64_t marker_updates[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0xb80U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0xbc0U;
    const uintptr_t pressure_vaddr = seg_dst_vaddr + 64U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xbc0U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0xb80U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0xbc0U;
    const uintptr_t pressure_paddr = seg_dst_paddr + 64U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xbc0U;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t pressure_guard_paddr = pressure_paddr + sizeof(uint64_t);
    const uintptr_t marker_before_paddr = marker_paddr - sizeof(uint64_t);
    const uintptr_t marker_after_paddr = marker_paddr + sizeof(uint64_t);
    const uint8_t seg_fill = 0x3cU;
    const uint8_t seg_before = 0x4dU;
    const uint8_t seg_after = 0xd4U;
    const uint64_t pressure_guard = 0x55aa33cc7799bbddULL;
    const uint64_t marker_before = 0x1234fedc5678ba90ULL;
    const uint64_t marker_after = 0x0badc0de55aa33ccULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write64(pressure_paddr, pressure_seeds[round]);
        write64(marker_paddr, marker_seeds[round]);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);
        write64(pressure_guard_paddr, pressure_guard);
        write64(marker_before_paddr, marker_before);
        write64(marker_after_paddr, marker_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(marker_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_fence_then_vseg_store_marker_window(pressure_vaddr,
            pressure_updates[round], seg_src_vaddr, seg_dst_vaddr,
            marker_vaddr, marker_updates[round], &marker_reloaded);

        AI_ASSERT_NO_EXCEPT_AND("a fence drain immediately followed by vsegment store releases fence ownership before segment payload writeback and does not drop the following marker",
            ld(pressure_vaddr) == pressure_updates[round] &&
            marker_reloaded == marker_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the fence then vsegment-store boundary preserves the drained pressure store, segment payload, and post-segment marker independently",
            read64(pressure_paddr) == pressure_updates[round] &&
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            read64(marker_paddr) == marker_updates[round]
        );
        TEST_ASSERT("the fence then vsegment-store boundary preserves guards around segment destination, pressure word, and marker",
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after &&
            read64(pressure_guard_paddr) == pressure_guard &&
            read64(marker_before_paddr) == marker_before &&
            read64(marker_after_paddr) == marker_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_lrsc_then_vseg_store_marker_window(
    uintptr_t lrsc_addr,
    uint64_t sc_payload,
    uintptr_t seg_src,
    uintptr_t seg_dst,
    uintptr_t marker_dst,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t observed_lr = 0ULL;
    uint64_t status = 1ULL;
    uint64_t reloaded = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[lr_out], 0(%[lrsc_addr])\n\t"
        "sc.d %[status_out], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "sd %[marker_update], 0(%[marker_dst])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[reloaded], 0(%[marker_dst])\n\t"
        ".option pop\n\t"
        : [lr_out] "=&r" (observed_lr),
          [status_out] "=&r" (status),
          [reloaded] "=&r" (reloaded)
        : [lrsc_addr] "r" (lrsc_addr),
          [sc_payload] "r" (sc_payload),
          [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [marker_dst] "r" (marker_dst),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *lr_observed = observed_lr;
    *sc_status = status;
    *marker_reloaded = reloaded;
}

bool ai_micro_hs_lrsc_then_vsegment_store_no_stale_reservation_owner_or_segment_payload_corner()
{
    TEST_START();

    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0xa1, 0x1a, 0xa2, 0x2a, 0xa3, 0x3a, 0xa4, 0x4a,
          0xa5, 0x5a, 0xa6, 0x6a, 0xa7, 0x7a, 0xa8, 0x8a },
        { 0xb1, 0x1b, 0xb2, 0x2b, 0xb3, 0x3b, 0xb4, 0x4b,
          0xb5, 0x5b, 0xb6, 0x6b, 0xb7, 0x7b, 0xb8, 0x8b },
        { 0xc1, 0x1c, 0xc2, 0x2c, 0xc3, 0x3c, 0xc4, 0x4c,
          0xc5, 0x5c, 0xc6, 0x6c, 0xc7, 0x7c, 0xc8, 0x8c },
    };
    static const uint64_t lrsc_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    static const uint64_t sc_payloads[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x8877665544332211ULL,
        0x1029384756aabbccULL,
        0x55aa33cc77ee11ddULL,
    };
    static const uint64_t marker_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x1111222233334444ULL,
        0x2222333344445555ULL,
        0x3333444455556666ULL,
    };
    static const uint64_t marker_updates[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x445566778899aabbULL,
        0x5566778899aabbccULL,
        0x66778899aabbccddULL,
    };
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page lrsc_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0xc80U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0xc80U;
    const uintptr_t lrsc_vaddr = hs_page_base(lrsc_page) + 0xf00U;
    const uintptr_t marker_vaddr = hs_page_base(lrsc_page) + 0xf80U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0xc80U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0xc80U;
    const uintptr_t lrsc_paddr = phys_page_base(lrsc_page) + 0xf00U;
    const uintptr_t marker_paddr = phys_page_base(lrsc_page) + 0xf80U;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t lrsc_guard_paddr = lrsc_paddr + sizeof(uint64_t);
    const uintptr_t marker_before_paddr = marker_paddr - sizeof(uint64_t);
    const uintptr_t marker_after_paddr = marker_paddr + sizeof(uint64_t);
    const uint8_t seg_fill = 0x6dU;
    const uint8_t seg_before = 0xd6U;
    const uint8_t seg_after = 0x6eU;
    const uint64_t lrsc_guard = 0xa5a55a5af00ff00fULL;
    const uint64_t marker_before = 0x0badcafe55aa33ccULL;
    const uint64_t marker_after = 0x123456789abcdef0ULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        uint64_t lr_observed = 0ULL;
        uint64_t sc_status = 1ULL;
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write64(lrsc_paddr, lrsc_seeds[round]);
        write64(marker_paddr, marker_seeds[round]);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);
        write64(lrsc_guard_paddr, lrsc_guard);
        write64(marker_before_paddr, marker_before);
        write64(marker_after_paddr, marker_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(lrsc_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_lrsc_then_vseg_store_marker_window(lrsc_vaddr,
            sc_payloads[round], seg_src_vaddr, seg_dst_vaddr, marker_vaddr,
            marker_updates[round], &lr_observed, &sc_status,
            &marker_reloaded);

        AI_ASSERT_NO_EXCEPT_AND("an LR/SC immediately followed by vsegment store releases reservation flush ownership before segment payload writeback and marker progress",
            lr_observed == lrsc_seeds[round] &&
            sc_status == 0ULL &&
            ld(lrsc_vaddr) == sc_payloads[round] &&
            marker_reloaded == marker_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the LR/SC then vsegment-store boundary preserves the SC result, segment payload, and post-segment marker independently",
            read64(lrsc_paddr) == sc_payloads[round] &&
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            read64(marker_paddr) == marker_updates[round]
        );
        TEST_ASSERT("the LR/SC then vsegment-store boundary preserves guards around the segment destination, LR/SC target, and marker",
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after &&
            read64(lrsc_guard_paddr) == lrsc_guard &&
            read64(marker_before_paddr) == marker_before &&
            read64(marker_after_paddr) == marker_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_amo_then_vseg_store_marker_window(
    uintptr_t amo_addr,
    uint64_t amo_addend,
    uintptr_t seg_src,
    uintptr_t seg_dst,
    uintptr_t marker_dst,
    uint64_t marker_update,
    uint64_t *amo_old,
    uint64_t *amo_observed,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t old_value = 0ULL;
    uint64_t observed = 0ULL;
    uint64_t reloaded = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[old_value], %[amo_addend], (%[amo_addr])\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "sd %[marker_update], 0(%[marker_dst])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[observed], 0(%[amo_addr])\n\t"
        "ld %[reloaded], 0(%[marker_dst])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r" (old_value),
          [observed] "=&r" (observed),
          [reloaded] "=&r" (reloaded)
        : [amo_addr] "r" (amo_addr),
          [amo_addend] "r" (amo_addend),
          [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [marker_dst] "r" (marker_dst),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *amo_old = old_value;
    *amo_observed = observed;
    *marker_reloaded = reloaded;
}

bool ai_micro_hs_amo_then_vsegment_store_no_stale_atomic_owner_or_segment_payload_corner()
{
    TEST_START();

    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0xd1, 0x1d, 0xd2, 0x2d, 0xd3, 0x3d, 0xd4, 0x4d,
          0xd5, 0x5d, 0xd6, 0x6d, 0xd7, 0x7d, 0xd8, 0x8d },
        { 0xe1, 0x1e, 0xe2, 0x2e, 0xe3, 0x3e, 0xe4, 0x4e,
          0xe5, 0x5e, 0xe6, 0x6e, 0xe7, 0x7e, 0xe8, 0x8e },
        { 0xf1, 0x1f, 0xf2, 0x2f, 0xf3, 0x3f, 0xf4, 0x4f,
          0xf5, 0x5f, 0xf6, 0x6f, 0xf7, 0x7f, 0xf8, 0x8f },
    };
    static const uint64_t amo_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0xa1b2c3d4e5f60718ULL,
    };
    static const uint64_t amo_addends[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0001000200030004ULL,
        0x0005000600070008ULL,
        0x0009000a000b000cULL,
    };
    static const uint64_t marker_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    static const uint64_t marker_updates[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x55aa33cc77ee11ffULL,
    };
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page amo_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0xe80U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0xe80U;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0x900U;
    const uintptr_t marker_vaddr = hs_page_base(amo_page) + 0x980U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0xe80U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0xe80U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0x900U;
    const uintptr_t marker_paddr = phys_page_base(amo_page) + 0x980U;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t amo_before_paddr = amo_paddr - sizeof(uint64_t);
    const uintptr_t amo_after_paddr = amo_paddr + sizeof(uint64_t);
    const uintptr_t marker_before_paddr = marker_paddr - sizeof(uint64_t);
    const uintptr_t marker_after_paddr = marker_paddr + sizeof(uint64_t);
    const uint8_t seg_fill = 0xa9U;
    const uint8_t seg_before = 0x9aU;
    const uint8_t seg_after = 0x29U;
    const uint64_t amo_before = 0x1234fedc5678ba90ULL;
    const uint64_t amo_after = 0x0badc0de55aa33ccULL;
    const uint64_t marker_before = 0xa5a55a5af00ff00fULL;
    const uint64_t marker_after = 0x2468ace013579bdfULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        const uint64_t expected_amo = amo_seeds[round] + amo_addends[round];
        uint64_t amo_old = 0ULL;
        uint64_t amo_observed = 0ULL;
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write64(amo_paddr, amo_seeds[round]);
        write64(marker_paddr, marker_seeds[round]);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);
        write64(amo_before_paddr, amo_before);
        write64(amo_after_paddr, amo_after);
        write64(marker_before_paddr, marker_before);
        write64(marker_after_paddr, marker_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(amo_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_amo_then_vseg_store_marker_window(amo_vaddr,
            amo_addends[round], seg_src_vaddr, seg_dst_vaddr, marker_vaddr,
            marker_updates[round], &amo_old, &amo_observed,
            &marker_reloaded);

        AI_ASSERT_NO_EXCEPT_AND("an AMO immediately followed by vsegment store releases atomic flush ownership before segment payload writeback and marker progress",
            amo_old == amo_seeds[round] &&
            amo_observed == expected_amo &&
            marker_reloaded == marker_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the AMO then vsegment-store boundary preserves the AMO result, segment payload, and post-segment marker independently",
            read64(amo_paddr) == expected_amo &&
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            read64(marker_paddr) == marker_updates[round]
        );
        TEST_ASSERT("the AMO then vsegment-store boundary preserves guards around the AMO target, segment destination, and marker",
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after &&
            read64(amo_before_paddr) == amo_before &&
            read64(amo_after_paddr) == amo_after &&
            read64(marker_before_paddr) == marker_before &&
            read64(marker_after_paddr) == marker_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_vseg_then_vector_load_window(
    uintptr_t seg_src,
    uintptr_t seg_dst,
    uintptr_t vec_src,
    uintptr_t vec_dst)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[seg_vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "vsetvl t0, %[vec_vl], %[vtype]\n\t"
        "vle8.v v16, (%[vec_src])\n\t"
        "vse8.v v16, (%[vec_dst])\n\t"
        ".option pop\n\t"
        :
        : [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [vec_src] "r" (vec_src),
          [vec_dst] "r" (vec_dst),
          [seg_vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vec_vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_BYTES),
          [vtype] "r" (vtype)
        : "t0", "memory");
}

bool ai_micro_hs_vsegment_then_normal_vector_load_no_writeback_drop_corner()
{
    TEST_START();

    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0x72, 0x17, 0x73, 0x27, 0x74, 0x37, 0x75, 0x47,
          0x76, 0x57, 0x77, 0x67, 0x78, 0x77, 0x79, 0x87 },
        { 0x82, 0x18, 0x83, 0x28, 0x84, 0x38, 0x85, 0x48,
          0x86, 0x58, 0x87, 0x68, 0x88, 0x78, 0x89, 0x88 },
        { 0x92, 0x19, 0x93, 0x29, 0x94, 0x39, 0x95, 0x49,
          0x96, 0x59, 0x97, 0x69, 0x98, 0x79, 0x99, 0x89 },
    };
    static const uint8_t vector_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
          0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf },
        { 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
          0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf },
        { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
          0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf },
    };
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page vec_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0x880U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0x900U;
    const uintptr_t vec_src_vaddr = hs_page_base(vec_page) + 0x980U;
    const uintptr_t vec_dst_vaddr = hs_page_base(vec_page) + 0xa40U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0x880U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0x900U;
    const uintptr_t vec_src_paddr = phys_page_base(vec_page) + 0x980U;
    const uintptr_t vec_dst_paddr = phys_page_base(vec_page) + 0xa40U;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t vec_before_paddr = vec_dst_paddr - 1U;
    const uintptr_t vec_after_paddr =
        vec_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uint8_t seg_fill = 0x5eU;
    const uint8_t vec_fill = 0xe5U;
    const uint8_t seg_before = 0x4aU;
    const uint8_t seg_after = 0xa4U;
    const uint8_t vec_before = 0x6bU;
    const uint8_t vec_after = 0xb6U;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_write_bytes(vec_src_paddr,
            vector_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(vec_dst_paddr, vec_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);
        write8(vec_before_paddr, vec_before);
        write8(vec_after_paddr, vec_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(vec_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vseg_then_vector_load_window(seg_src_vaddr,
            seg_dst_vaddr, vec_src_vaddr, vec_dst_vaddr);
        fence_iorw_iorw();

        TEST_ASSERT("a normal vector load/store immediately after vsegment completion is not starved by stale vSegment writeback arbitration",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the adjacent vsegment and normal vector-load writeback window preserves both independent payloads",
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            ai_memblock_vseg_success_bytes_match(vec_dst_paddr,
                vector_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES)
        );
        TEST_ASSERT("the adjacent vsegment and normal vector-load writeback window preserves all destination guards",
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after &&
            read8(vec_before_paddr) == vec_before &&
            read8(vec_after_paddr) == vec_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_vseg_store_then_amo_window(
    uintptr_t seg_src,
    uintptr_t seg_dst,
    uintptr_t amo_addr,
    uint64_t amo_addend,
    uint64_t *amo_old,
    uint64_t *amo_observed)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t old_value = 0ULL;
    uint64_t observed = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "amoadd.d %[old_value], %[amo_addend], (%[amo_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[observed], 0(%[amo_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r" (old_value),
          [observed] "=&r" (observed)
        : [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [amo_addr] "r" (amo_addr),
          [amo_addend] "r" (amo_addend),
          [vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *amo_old = old_value;
    *amo_observed = observed;
}

bool ai_micro_hs_vsegment_store_then_amo_no_flush_owner_or_stale_old_value_corner()
{
    TEST_START();

    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0x23, 0xd0, 0x24, 0xd1, 0x25, 0xd2, 0x26, 0xd3,
          0x27, 0xd4, 0x28, 0xd5, 0x29, 0xd6, 0x2a, 0xd7 },
        { 0x33, 0xe0, 0x34, 0xe1, 0x35, 0xe2, 0x36, 0xe3,
          0x37, 0xe4, 0x38, 0xe5, 0x39, 0xe6, 0x3a, 0xe7 },
        { 0x43, 0xf0, 0x44, 0xf1, 0x45, 0xf2, 0x46, 0xf3,
          0x47, 0xf4, 0x48, 0xf5, 0x49, 0xf6, 0x4a, 0xf7 },
    };
    static const uint64_t amo_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x1111222233334444ULL,
        0x2021324354657687ULL,
        0x55aa33cc77ee11ddULL,
    };
    static const uint64_t amo_addends[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page amo_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0xb00U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0xb80U;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0xc40U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0xb00U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0xb80U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0xc40U;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t amo_before_paddr = amo_paddr - sizeof(uint64_t);
    const uintptr_t amo_after_paddr = amo_paddr + sizeof(uint64_t);
    const uint8_t seg_fill = 0x7cU;
    const uint8_t seg_before = 0xc7U;
    const uint8_t seg_after = 0x3dU;
    const uint64_t amo_before = 0x01230123456789abULL;
    const uint64_t amo_after = 0xfedcfedcba987654ULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        const uint64_t expected_amo = amo_seeds[round] + amo_addends[round];
        uint64_t amo_old = 0ULL;
        uint64_t amo_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write64(amo_paddr, amo_seeds[round]);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);
        write64(amo_before_paddr, amo_before);
        write64(amo_after_paddr, amo_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(amo_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vseg_store_then_amo_window(seg_src_vaddr, seg_dst_vaddr,
            amo_vaddr, amo_addends[round], &amo_old, &amo_observed);

        AI_ASSERT_NO_EXCEPT_AND("an AMO immediately after vsegment store observes only the seeded old value and current addend",
            amo_old == amo_seeds[round] &&
            amo_observed == expected_amo
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the vsegment-store then AMO window preserves segment payload and the final AMO result without flush-owner cross-binding",
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            read64(amo_paddr) == expected_amo
        );
        TEST_ASSERT("the vsegment-store then AMO window preserves segment and AMO guard locations",
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after &&
            read64(amo_before_paddr) == amo_before &&
            read64(amo_after_paddr) == amo_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_vseg_load_then_amo_store_window(
    uintptr_t seg_src,
    uintptr_t seg_dst,
    uintptr_t amo_addr,
    uint64_t amo_addend,
    uint64_t *amo_old,
    uint64_t *amo_observed)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t old_value = 0ULL;
    uint64_t observed = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "amoadd.d %[old_value], %[amo_addend], (%[amo_addr])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[observed], 0(%[amo_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r" (old_value),
          [observed] "=&r" (observed)
        : [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [amo_addr] "r" (amo_addr),
          [amo_addend] "r" (amo_addend),
          [vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *amo_old = old_value;
    *amo_observed = observed;
}

bool ai_micro_hs_vsegment_load_then_amo_then_store_no_stale_load0_or_atomic_owner_corner()
{
    TEST_START();

    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0x5a, 0xa5, 0x5b, 0xb5, 0x5c, 0xc5, 0x5d, 0xd5,
          0x5e, 0xe5, 0x5f, 0xf5, 0x60, 0x06, 0x61, 0x16 },
        { 0x6a, 0xa6, 0x6b, 0xb6, 0x6c, 0xc6, 0x6d, 0xd6,
          0x6e, 0xe6, 0x6f, 0xf6, 0x70, 0x07, 0x71, 0x17 },
        { 0x7a, 0xa7, 0x7b, 0xb7, 0x7c, 0xc7, 0x7d, 0xd7,
          0x7e, 0xe7, 0x7f, 0xf7, 0x80, 0x08, 0x81, 0x18 },
    };
    static const uint64_t amo_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0101020203030404ULL,
        0x1111222233334444ULL,
        0x2121324354657687ULL,
    };
    static const uint64_t amo_addends[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0004000400040004ULL,
        0x0005000500050005ULL,
        0x0006000600060006ULL,
    };
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page amo_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0xd00U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0xd80U;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0xe40U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0xd00U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0xd80U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0xe40U;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t amo_before_paddr = amo_paddr - sizeof(uint64_t);
    const uintptr_t amo_after_paddr = amo_paddr + sizeof(uint64_t);
    const uint8_t seg_fill = 0x91U;
    const uint8_t seg_before = 0x19U;
    const uint8_t seg_after = 0x91U;
    const uint64_t amo_before = 0x2468ace013579bdfULL;
    const uint64_t amo_after = 0xfdb97531eca86420ULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        const uint64_t expected_amo = amo_seeds[round] + amo_addends[round];
        uint64_t amo_old = 0ULL;
        uint64_t amo_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write64(amo_paddr, amo_seeds[round]);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);
        write64(amo_before_paddr, amo_before);
        write64(amo_after_paddr, amo_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(amo_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vseg_load_then_amo_store_window(seg_src_vaddr,
            seg_dst_vaddr, amo_vaddr, amo_addends[round], &amo_old,
            &amo_observed);

        AI_ASSERT_NO_EXCEPT_AND("an AMO between vsegment load and vsegment store does not inherit stale load0 or atomic owner state",
            amo_old == amo_seeds[round] &&
            amo_observed == expected_amo
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the vsegment load data survives an intervening AMO and stores only the current segment payload",
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            read64(amo_paddr) == expected_amo
        );
        TEST_ASSERT("the vsegment load then AMO then store window preserves segment and AMO guards",
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after &&
            read64(amo_before_paddr) == amo_before &&
            read64(amo_after_paddr) == amo_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_vector_load_then_vsegment_window(
    uintptr_t vec_src,
    uintptr_t vec_dst,
    uintptr_t seg_src,
    uintptr_t seg_dst)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vec_vl], %[vtype]\n\t"
        "vle8.v v16, (%[vec_src])\n\t"
        "vsetvl t0, %[seg_vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "vsetvl t0, %[vec_vl], %[vtype]\n\t"
        "vse8.v v16, (%[vec_dst])\n\t"
        ".option pop\n\t"
        :
        : [vec_src] "r" (vec_src),
          [vec_dst] "r" (vec_dst),
          [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [vec_vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_BYTES),
          [seg_vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vtype] "r" (vtype)
        : "t0", "memory");
}

bool ai_micro_hs_normal_vector_load_then_vsegment_no_writeback_preempt_drop_corner()
{
    TEST_START();

    static const uint8_t vector_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0x0a, 0x1a, 0x2a, 0x3a, 0x4a, 0x5a, 0x6a, 0x7a,
          0x8a, 0x9a, 0xaa, 0xba, 0xca, 0xda, 0xea, 0xfa },
        { 0x0b, 0x1b, 0x2b, 0x3b, 0x4b, 0x5b, 0x6b, 0x7b,
          0x8b, 0x9b, 0xab, 0xbb, 0xcb, 0xdb, 0xeb, 0xfb },
        { 0x0c, 0x1c, 0x2c, 0x3c, 0x4c, 0x5c, 0x6c, 0x7c,
          0x8c, 0x9c, 0xac, 0xbc, 0xcc, 0xdc, 0xec, 0xfc },
    };
    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0x13, 0x31, 0x14, 0x41, 0x15, 0x51, 0x16, 0x61,
          0x17, 0x71, 0x18, 0x81, 0x19, 0x91, 0x1a, 0xa1 },
        { 0x23, 0x32, 0x24, 0x42, 0x25, 0x52, 0x26, 0x62,
          0x27, 0x72, 0x28, 0x82, 0x29, 0x92, 0x2a, 0xa2 },
        { 0x33, 0x33, 0x34, 0x43, 0x35, 0x53, 0x36, 0x63,
          0x37, 0x73, 0x38, 0x83, 0x39, 0x93, 0x3a, 0xa3 },
    };
    const enum test_page vec_page = VSRWX_GRWX;
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const uintptr_t vec_src_vaddr = hs_page_base(vec_page) + 0xf00U;
    const uintptr_t vec_dst_vaddr = hs_page_base(vec_page) + 0xf40U;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0xf00U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0xf40U;
    const uintptr_t vec_src_paddr = phys_page_base(vec_page) + 0xf00U;
    const uintptr_t vec_dst_paddr = phys_page_base(vec_page) + 0xf40U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0xf00U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0xf40U;
    const uintptr_t vec_before_paddr = vec_dst_paddr - 1U;
    const uintptr_t vec_after_paddr =
        vec_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uint8_t vec_fill = 0x3cU;
    const uint8_t seg_fill = 0xc3U;
    const uint8_t vec_before = 0x5dU;
    const uint8_t vec_after = 0xd5U;
    const uint8_t seg_before = 0x6eU;
    const uint8_t seg_after = 0xe6U;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(vec_src_paddr,
            vector_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(vec_dst_paddr, vec_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write8(vec_before_paddr, vec_before);
        write8(vec_after_paddr, vec_after);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(vec_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vector_load_then_vsegment_window(vec_src_vaddr,
            vec_dst_vaddr, seg_src_vaddr, seg_dst_vaddr);
        fence_iorw_iorw();

        TEST_ASSERT("a normal vector load issued before vsegment is not dropped or preempted by the later vsegment writeback",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the normal vector payload loaded before vsegment survives until the post-vsegment vector store",
            ai_memblock_vseg_success_bytes_match(vec_dst_paddr,
                vector_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES)
        );
        TEST_ASSERT("the normal-vector then vsegment writeback window preserves all destination guards",
            read8(vec_before_paddr) == vec_before &&
            read8(vec_after_paddr) == vec_after &&
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_vseg_store_then_lrsc_marker_window(
    uintptr_t seg_src,
    uintptr_t seg_dst,
    uintptr_t lrsc_addr,
    uint64_t sc_payload,
    uintptr_t marker_dst,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *lrsc_reloaded,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t observed_lr = 0ULL;
    uint64_t status = 1ULL;
    uint64_t lrsc_after = 0ULL;
    uint64_t marker_after = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[seg_src])\n\t"
        "vsseg2e8.v v8, (%[seg_dst])\n\t"
        "lr.d %[lr_out], 0(%[lrsc_addr])\n\t"
        "sc.d %[status_out], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "sd %[marker_update], 0(%[marker_dst])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[lrsc_after], 0(%[lrsc_addr])\n\t"
        "ld %[marker_after], 0(%[marker_dst])\n\t"
        ".option pop\n\t"
        : [lr_out] "=&r" (observed_lr),
          [status_out] "=&r" (status),
          [lrsc_after] "=&r" (lrsc_after),
          [marker_after] "=&r" (marker_after)
        : [seg_src] "r" (seg_src),
          [seg_dst] "r" (seg_dst),
          [lrsc_addr] "r" (lrsc_addr),
          [sc_payload] "r" (sc_payload),
          [marker_dst] "r" (marker_dst),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_VSEG_SUCCESS_VL),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *lr_observed = observed_lr;
    *sc_status = status;
    *lrsc_reloaded = lrsc_after;
    *marker_reloaded = marker_after;
}

bool ai_micro_hs_vsegment_store_then_lrsc_no_stale_segment_writeback_or_reservation_loss_corner()
{
    TEST_START();

    static const uint8_t segment_patterns[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS]
        [AI_MEMBLOCK_VSEG_SUCCESS_BYTES] = {
        { 0x62, 0x26, 0x63, 0x36, 0x64, 0x46, 0x65, 0x56,
          0x66, 0x66, 0x67, 0x76, 0x68, 0x86, 0x69, 0x96 },
        { 0x72, 0x27, 0x73, 0x37, 0x74, 0x47, 0x75, 0x57,
          0x76, 0x67, 0x77, 0x77, 0x78, 0x87, 0x79, 0x97 },
        { 0x82, 0x28, 0x83, 0x38, 0x84, 0x48, 0x85, 0x58,
          0x86, 0x68, 0x87, 0x78, 0x88, 0x88, 0x89, 0x98 },
    };
    static const uint64_t lrsc_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    static const uint64_t sc_payloads[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x0ddc0ffeebadbeefULL,
        0x1234432112344321ULL,
        0xcafebabef00dd00dULL,
    };
    static const uint64_t marker_seeds[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x1010101020202020ULL,
        0x3030303040404040ULL,
        0x5050505060606060ULL,
    };
    static const uint64_t marker_updates[AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS] = {
        0x8877665544332211ULL,
        0x99aabbccddeeff00ULL,
        0x0123456789abcdefULL,
    };
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page lrsc_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_src_page) + 0xa40U;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0xa40U;
    const uintptr_t lrsc_vaddr = hs_page_base(lrsc_page) + 0x700U;
    const uintptr_t marker_vaddr = hs_page_base(lrsc_page) + 0x780U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_src_page) + 0xa40U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0xa40U;
    const uintptr_t lrsc_paddr = phys_page_base(lrsc_page) + 0x700U;
    const uintptr_t marker_paddr = phys_page_base(lrsc_page) + 0x780U;
    const uintptr_t seg_before_paddr = seg_dst_paddr - 1U;
    const uintptr_t seg_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_VSEG_SUCCESS_BYTES;
    const uintptr_t lrsc_before_paddr = lrsc_paddr - sizeof(uint64_t);
    const uintptr_t lrsc_after_paddr = lrsc_paddr + sizeof(uint64_t);
    const uintptr_t marker_before_paddr = marker_paddr - sizeof(uint64_t);
    const uintptr_t marker_after_paddr = marker_paddr + sizeof(uint64_t);
    const uint8_t seg_fill = 0xa6U;
    const uint8_t seg_before = 0x6aU;
    const uint8_t seg_after = 0x9aU;
    const uint64_t lrsc_before = 0xabcdabcd12341234ULL;
    const uint64_t lrsc_after = 0x56785678fedcfedcULL;
    const uint64_t marker_before = 0x0f0e0d0c0b0a0908ULL;
    const uint64_t marker_after = 0xf0e0d0c0b0a09080ULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VSEG_SUCCESS_ROUNDS; ++round) {
        uint64_t lr_observed = 0ULL;
        uint64_t sc_status = 1ULL;
        uint64_t lrsc_reloaded = 0ULL;
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_vseg_success_write_bytes(seg_src_paddr,
            segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        ai_memblock_vseg_success_prefill(seg_dst_paddr, seg_fill,
            AI_MEMBLOCK_VSEG_SUCCESS_BYTES);
        write64(lrsc_paddr, lrsc_seeds[round]);
        write64(marker_paddr, marker_seeds[round]);
        write8(seg_before_paddr, seg_before);
        write8(seg_after_paddr, seg_after);
        write64(lrsc_before_paddr, lrsc_before);
        write64(lrsc_after_paddr, lrsc_after);
        write64(marker_before_paddr, marker_before);
        write64(marker_after_paddr, marker_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vseg_success_set_hs_leaf(seg_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(seg_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vseg_success_set_hs_leaf(lrsc_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vseg_store_then_lrsc_marker_window(seg_src_vaddr,
            seg_dst_vaddr, lrsc_vaddr, sc_payloads[round], marker_vaddr,
            marker_updates[round], &lr_observed, &sc_status,
            &lrsc_reloaded, &marker_reloaded);

        AI_ASSERT_NO_EXCEPT_AND("an LR/SC immediately after vsegment store waits for segment store ownership to retire and keeps reservation success",
            lr_observed == lrsc_seeds[round] &&
            sc_status == 0ULL &&
            lrsc_reloaded == sc_payloads[round] &&
            marker_reloaded == marker_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the vsegment-store then LR/SC window preserves segment payload, SC result, and post-LRSC marker independently",
            ai_memblock_vseg_success_bytes_match(seg_dst_paddr,
                segment_patterns[round], AI_MEMBLOCK_VSEG_SUCCESS_BYTES) &&
            read64(lrsc_paddr) == sc_payloads[round] &&
            read64(marker_paddr) == marker_updates[round]
        );
        TEST_ASSERT("the vsegment-store then LR/SC window preserves guards around segment destination, LR/SC target, and marker",
            read8(seg_before_paddr) == seg_before &&
            read8(seg_after_paddr) == seg_after &&
            read64(lrsc_before_paddr) == lrsc_before &&
            read64(lrsc_after_paddr) == lrsc_after &&
            read64(marker_before_paddr) == marker_before &&
            read64(marker_after_paddr) == marker_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
