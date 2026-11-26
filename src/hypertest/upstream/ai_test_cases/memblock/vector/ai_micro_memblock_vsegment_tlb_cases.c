#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_VTLB_FIELDS 2U
#define AI_MEMBLOCK_VTLB_VL 8U
#define AI_MEMBLOCK_VTLB_BYTES \
    (AI_MEMBLOCK_VTLB_FIELDS * AI_MEMBLOCK_VTLB_VL)
#define AI_MEMBLOCK_VTLB_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static inline pte_t *ai_memblock_vtlb_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_vtlb_set_hs_leaf_to_phys(
    enum test_page va_page, enum test_page phys_page, uint64_t flags)
{
    *ai_memblock_vtlb_hs_leaf_entry(va_page) =
        (phys_page_base(phys_page) >> 2) | flags;
}

static inline void ai_memblock_vtlb_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_memblock_vtlb_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_vtlb_expected_cross_page(
    uint8_t *expected, const uint8_t *low, const uint8_t *high)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_VTLB_VL; ++i) {
        expected[i] = low[i];
        expected[AI_MEMBLOCK_VTLB_VL + i] = high[i];
    }
}

static inline void ai_memblock_vtlb_vlseg2e8_to_v8(const uint8_t *src)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"((int)AI_MEMBLOCK_VTLB_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_memblock_vtlb_vsseg2e8_from_v8(uint8_t *dst)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vsseg2e8.v v8, (%[dst])\n\t"
        ".option pop\n\t"
        : [dst] "+r"(dst)
        : [vl] "r"((int)AI_MEMBLOCK_VTLB_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_vsegment_dtlb_handoff_after_scalar_remap_no_stale_translation_corner()
{
    TEST_START();

    const enum test_page scalar_va_page = VSRWX_GRWX;
    const enum test_page scalar_initial_page = VSRWX_GRWX;
    const enum test_page scalar_remap_page = VSRW_GRW;
    const enum test_page vseg_low_page = SWITCH1;
    const enum test_page vseg_high_page = SWITCH2;
    const enum test_page dst_page = SCRATCHPAD;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_va_page) + 0x180U;
    const uintptr_t scalar_initial_paddr =
        phys_page_base(scalar_initial_page) + 0x180U;
    const uintptr_t scalar_remap_paddr =
        phys_page_base(scalar_remap_page) + 0x180U;
    const uintptr_t scalar_remap_guard_paddr =
        scalar_remap_paddr + sizeof(uint64_t);
    const uintptr_t vseg_src_vaddr =
        hs_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_VTLB_VL;
    const uintptr_t vseg_low_tail_paddr =
        phys_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_VTLB_VL;
    const uintptr_t vseg_high_head_paddr = phys_page_base(vseg_high_page);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x300U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x300U;
    const uintptr_t dst_before_paddr = dst_paddr - 1U;
    const uintptr_t dst_after_paddr = dst_paddr + AI_MEMBLOCK_VTLB_BYTES;
    const uint64_t scalar_initial_values[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
    };
    const uint64_t scalar_remap_values[] = {
        0xa1b2c3d4e5f60718ULL,
        0x8877665544332211ULL,
    };
    const uint64_t scalar_store_values[] = {
        0x55aa33cc77ee11ddULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t scalar_guard_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadbeeff00dbabeULL,
    };
    const uint8_t low_bytes[2][AI_MEMBLOCK_VTLB_VL] = {
        { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87 },
        { 0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f },
    };
    const uint8_t high_bytes[2][AI_MEMBLOCK_VTLB_VL] = {
        { 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
        { 0x80, 0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7 },
    };
    const uint8_t dst_fill[] = { 0x3eU, 0x4dU };
    const uint8_t dst_before[] = { 0x6aU, 0x7bU };
    const uint8_t dst_after[] = { 0xa6U, 0xb7U };

    for (unsigned round = 0;
         round < AI_MEMBLOCK_VTLB_ARRAY_SIZE(scalar_initial_values);
         ++round) {
        uint8_t expected[AI_MEMBLOCK_VTLB_BYTES];

        ai_memblock_vtlb_expected_cross_page(expected, low_bytes[round],
            high_bytes[round]);

        goto_priv(PRIV_M);
        write64(scalar_initial_paddr, scalar_initial_values[round]);
        write64(scalar_remap_paddr, scalar_remap_values[round]);
        write64(scalar_remap_guard_paddr, scalar_guard_values[round]);
        ai_memblock_vtlb_write_bytes(vseg_low_tail_paddr, low_bytes[round],
            AI_MEMBLOCK_VTLB_VL);
        ai_memblock_vtlb_write_bytes(vseg_high_head_paddr, high_bytes[round],
            AI_MEMBLOCK_VTLB_VL);
        for (unsigned i = 0; i < AI_MEMBLOCK_VTLB_BYTES; ++i) {
            write8(dst_paddr + i, dst_fill[round]);
        }
        write8(dst_before_paddr, dst_before[round]);
        write8(dst_after_paddr, dst_after[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vtlb_set_hs_leaf_to_phys(scalar_va_page,
            scalar_initial_page, PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vtlb_set_hs_leaf_to_phys(vseg_low_page, vseg_low_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vtlb_set_hs_leaf_to_phys(vseg_high_page, vseg_high_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vtlb_set_hs_leaf_to_phys(dst_page, dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        const uint64_t warmed_scalar = ld(scalar_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the VSegment/load0 DTLB handoff probe, the scalar VA is warmed through its original HS leaf",
            warmed_scalar == scalar_initial_values[round]
        );

        ai_memblock_vtlb_set_hs_leaf_to_phys(scalar_va_page,
            scalar_remap_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vtlb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
        ai_memblock_vtlb_vsseg2e8_from_v8((uint8_t *)dst_vaddr);
        const uint64_t remapped_scalar = ld(scalar_vaddr);
        sd(scalar_vaddr, scalar_store_values[round]);
        const uint64_t stored_scalar = ld(scalar_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after VSegment borrows the load0 DTLB port, the remapped scalar VA must use the current leaf translation instead of stale scalar or segment translation state",
            remapped_scalar == scalar_remap_values[round] &&
            stored_scalar == scalar_store_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the VSegment/load0 DTLB handoff window keeps segment bytes, remapped scalar payload, original physical page, and sentinels independent",
            read64(scalar_initial_paddr) == scalar_initial_values[round] &&
            read64(scalar_remap_paddr) == scalar_store_values[round] &&
            read64(scalar_remap_guard_paddr) == scalar_guard_values[round] &&
            ai_memblock_vtlb_bytes_match(dst_paddr, expected,
                AI_MEMBLOCK_VTLB_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
