#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <stddef.h>

#define AP22_MAX_BYTES 256U
#define AP22_SEG_VL 8U
#define AP22_SEG2_E8_BYTES (2U * AP22_SEG_VL)
#define AP22_MATRIX_GUARD_LO 0x5bU
#define AP22_MATRIX_GUARD_HI 0xb5U

static uint8_t ap22_src[AP22_MAX_BYTES] __attribute__((aligned(64)));
static uint8_t ap22_src_b[AP22_MAX_BYTES] __attribute__((aligned(64)));
static uint8_t ap22_dst[AP22_MAX_BYTES] __attribute__((aligned(64)));
static uint8_t ap22_expected[AP22_MAX_BYTES] __attribute__((aligned(64)));
static uint8_t ap22_seed0[AP22_SEG_VL] __attribute__((aligned(64)));
static uint8_t ap22_seed1[AP22_SEG_VL] __attribute__((aligned(64)));
static uint8_t ap22_mask[8] __attribute__((aligned(64)));

static inline void ap22_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline pte_t *ap22_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ap22_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ap22_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ap22_fill_pattern(uint8_t *buf, unsigned len, uint8_t base)
{
    for (unsigned i = 0; i < len; ++i) {
        buf[i] = (uint8_t)(base + (i * 0x13U) + (i >> 1));
    }
}

static inline void ap22_fill_constant(uint8_t *buf, unsigned len, uint8_t value)
{
    for (unsigned i = 0; i < len; ++i) {
        buf[i] = value;
    }
}

static inline void ap22_write_bytes(uintptr_t addr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ap22_bytes_match(const uint8_t *actual,
    const uint8_t *expected, unsigned len, unsigned *bad_index)
{
    for (unsigned i = 0; i < len; ++i) {
        if (actual[i] != expected[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = len;
    return true;
}

static inline bool ap22_paddr_bytes_match(uintptr_t paddr,
    const uint8_t *expected, unsigned len, unsigned *bad_index)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != expected[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = len;
    return true;
}

static inline void ap22_roundtrip_vlseg2e8(const uint8_t *src, uint8_t *dst,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e8.v v8, (%[src])\n\t"
        "vsseg2e8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src), [dst] "r"(dst), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ap22_roundtrip_vlseg4e8(const uint8_t *src, uint8_t *dst,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vlseg4e8.v v8, (%[src])\n\t"
        "vsseg4e8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src), [dst] "r"(dst), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ap22_roundtrip_vlseg8e8(const uint8_t *src, uint8_t *dst,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vlseg8e8.v v8, (%[src])\n\t"
        "vsseg8e8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src), [dst] "r"(dst), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ap22_roundtrip_vlseg2e16(const uint8_t *src, uint8_t *dst,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e16.v v8, (%[src])\n\t"
        "vsseg2e16.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src), [dst] "r"(dst), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ap22_roundtrip_vlseg2e32(const uint8_t *src, uint8_t *dst,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e32.v v8, (%[src])\n\t"
        "vsseg2e32.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src), [dst] "r"(dst), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ap22_roundtrip_vlseg2e64(const uint8_t *src, uint8_t *dst,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e64.v v8, (%[src])\n\t"
        "vsseg2e64.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src), [dst] "r"(dst), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ap22_masked_vstart_vlseg2e8_dump(
    const uint8_t *seed0,
    const uint8_t *seed1,
    const uint8_t *src,
    const uint8_t *mask,
    uint8_t *dst,
    unsigned vl,
    unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed0])\n\t"
        "vle8.v v9, (%[seed1])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, %[start]\n\t"
        "vlseg2e8.v v8, (%[src]), v0.t\n\t"
        "csrw vstart, zero\n\t"
        "vsseg2e8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [seed0] "r"(seed0), [seed1] "r"(seed1),
          [src] "r"(src), [mask] "r"(mask), [dst] "r"(dst),
          [vl] "r"(vl), [start] "r"(start)
        : "t0", "memory");
}

bool ai_micro_ap22_vlseg_nf_eew_visible_payload_matrix_corner()
{
    TEST_START();

    unsigned bad_index = 0;

    ap22_enable_vector_state();

    ap22_fill_pattern(ap22_src, AP22_MAX_BYTES, 0x21U);
    ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0xccU);

    TEST_SETUP_EXCEPT();
    ap22_roundtrip_vlseg2e8(ap22_src, ap22_dst, 8U);
    TEST_ASSERT("AP-22 vlseg2e8 cache-hit baseline preserves every interleaved byte instead of only data_delayed bit0",
        excpt.triggered == false &&
        CSRR(CSR_VL) == 8U &&
        ap22_bytes_match(ap22_dst, ap22_src, 16U, &bad_index),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, ap22_dst[bad_index], ap22_src[bad_index]
    );

    ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0xddU);
    TEST_SETUP_EXCEPT();
    ap22_roundtrip_vlseg4e8(ap22_src + 0x20U, ap22_dst, 4U);
    TEST_ASSERT("AP-22 vlseg4e8 keeps four fields bound to the correct element-major bytes",
        excpt.triggered == false &&
        CSRR(CSR_VL) == 4U &&
        ap22_bytes_match(ap22_dst, ap22_src + 0x20U, 16U, &bad_index),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, ap22_dst[bad_index], ap22_src[0x20U + bad_index]
    );

    ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0xeeU);
    TEST_SETUP_EXCEPT();
    ap22_roundtrip_vlseg8e8(ap22_src + 0x40U, ap22_dst, 2U);
    TEST_ASSERT("AP-22 vlseg8e8 keeps all eight field registers independent and byte-complete",
        excpt.triggered == false &&
        CSRR(CSR_VL) == 2U &&
        ap22_bytes_match(ap22_dst, ap22_src + 0x40U, 16U, &bad_index),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, ap22_dst[bad_index], ap22_src[0x40U + bad_index]
    );

    ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0x99U);
    TEST_SETUP_EXCEPT();
    ap22_roundtrip_vlseg2e16(ap22_src + 0x60U, ap22_dst, 4U);
    TEST_ASSERT("AP-22 vlseg2e16 preserves high bytes within every halfword slice from data_delayed",
        excpt.triggered == false &&
        CSRR(CSR_VL) == 4U &&
        ap22_bytes_match(ap22_dst, ap22_src + 0x60U, 16U, &bad_index),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, ap22_dst[bad_index], ap22_src[0x60U + bad_index]
    );

    ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0x88U);
    TEST_SETUP_EXCEPT();
    ap22_roundtrip_vlseg2e32(ap22_src + 0x80U, ap22_dst, 2U);
    TEST_ASSERT("AP-22 vlseg2e32 preserves every byte of both field words",
        excpt.triggered == false &&
        CSRR(CSR_VL) == 2U &&
        ap22_bytes_match(ap22_dst, ap22_src + 0x80U, 16U, &bad_index),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, ap22_dst[bad_index], ap22_src[0x80U + bad_index]
    );

    ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0x77U);
    TEST_SETUP_EXCEPT();
    ap22_roundtrip_vlseg2e64(ap22_src + 0xa0U, ap22_dst, 1U);
    TEST_ASSERT("AP-22 vlseg2e64 preserves the full dword field payload, including high-order bytes",
        excpt.triggered == false &&
        CSRR(CSR_VL) == 1U &&
        ap22_bytes_match(ap22_dst, ap22_src + 0xa0U, 16U, &bad_index),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, ap22_dst[bad_index], ap22_src[0xa0U + bad_index]
    );

    TEST_END();
}

bool ai_micro_ap22_vlseg2e8_low4_offset_cross16_roundtrip_corner()
{
    TEST_START();

    unsigned bad_index = 0;
    bool all_offsets_ok = true;
    unsigned first_bad_offset = 0;

    ap22_enable_vector_state();
    ap22_fill_pattern(ap22_src, AP22_MAX_BYTES, 0x31U);

    for (unsigned offset = 0; offset < 16U; ++offset) {
        ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0xa0U + offset);
        ap22_dst[0] = AP22_MATRIX_GUARD_LO;
        ap22_dst[AP22_SEG2_E8_BYTES + 1U] = AP22_MATRIX_GUARD_HI;

        TEST_SETUP_EXCEPT();
        ap22_roundtrip_vlseg2e8(ap22_src + offset, ap22_dst + 1U,
            AP22_SEG_VL);

        if (excpt.triggered ||
            CSRR(CSR_VL) != AP22_SEG_VL ||
            !ap22_bytes_match(ap22_dst + 1U, ap22_src + offset,
                AP22_SEG2_E8_BYTES, &bad_index) ||
            ap22_dst[0] != AP22_MATRIX_GUARD_LO ||
            ap22_dst[AP22_SEG2_E8_BYTES + 1U] != AP22_MATRIX_GUARD_HI) {
            all_offsets_ok = false;
            first_bad_offset = offset;
            break;
        }
    }

    TEST_ASSERT("AP-22 vlseg2e8 base[3:0] sweep covers every data_delayed byte slice including cross-16B offsets 9..15",
        all_offsets_ok,
        "first_bad_offset=%u bad_index=%u actual=0x%x expected=0x%x guard_lo=0x%x guard_hi=0x%x",
        first_bad_offset, bad_index, ap22_dst[1U + bad_index],
        ap22_src[first_bad_offset + bad_index], ap22_dst[0],
        ap22_dst[AP22_SEG2_E8_BYTES + 1U]
    );

    TEST_END();
}

bool ai_micro_ap22_back_to_back_vlseg2e8_no_stale_data_corner()
{
    TEST_START();

    unsigned bad_index_a = 0;
    unsigned bad_index_b = 0;

    ap22_enable_vector_state();
    ap22_fill_pattern(ap22_src, AP22_MAX_BYTES, 0x43U);
    ap22_fill_pattern(ap22_src_b, AP22_MAX_BYTES, 0xb1U);
    ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0xe5U);

    TEST_SETUP_EXCEPT();
    ap22_roundtrip_vlseg2e8(ap22_src + 0x10U, ap22_dst,
        AP22_SEG_VL);
    ap22_roundtrip_vlseg2e8(ap22_src_b + 0x30U,
        ap22_dst + 0x40U, AP22_SEG_VL);

    TEST_ASSERT("AP-22 back-to-back vlseg2e8 instructions keep each response data_delayed owner and do not leak A into B",
        excpt.triggered == false &&
        ap22_bytes_match(ap22_dst, ap22_src + 0x10U, AP22_SEG2_E8_BYTES,
            &bad_index_a) &&
        ap22_bytes_match(ap22_dst + 0x40U, ap22_src_b + 0x30U,
            AP22_SEG2_E8_BYTES, &bad_index_b),
        "bad_a=%u actual_a=0x%x expected_a=0x%x bad_b=%u actual_b=0x%x expected_b=0x%x",
        bad_index_a, ap22_dst[bad_index_a], ap22_src[0x10U + bad_index_a],
        bad_index_b, ap22_dst[0x40U + bad_index_b],
        ap22_src_b[0x30U + bad_index_b]
    );

    TEST_END();
}

bool ai_micro_ap22_masked_and_vstart_vlseg2e8_no_stale_lane_corner()
{
    TEST_START();

    unsigned bad_index = 0;

    ap22_enable_vector_state();
    ap22_fill_pattern(ap22_src, AP22_MAX_BYTES, 0x55U);
    for (unsigned i = 0; i < AP22_SEG_VL; ++i) {
        ap22_seed0[i] = (uint8_t)(0xc0U + i);
        ap22_seed1[i] = (uint8_t)(0xe0U + i);
    }

    ap22_mask[0] = 0x55U;
    for (unsigned i = 1; i < sizeof(ap22_mask); ++i) {
        ap22_mask[i] = 0U;
    }
    for (unsigned i = 0; i < AP22_SEG_VL; ++i) {
        const bool active = ((ap22_mask[0] >> i) & 1U) != 0U;
        ap22_expected[2U * i] = active ? ap22_src[2U * i] : ap22_seed0[i];
        ap22_expected[2U * i + 1U] =
            active ? ap22_src[2U * i + 1U] : ap22_seed1[i];
    }
    ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0x7aU);

    TEST_SETUP_EXCEPT();
    ap22_masked_vstart_vlseg2e8_dump(ap22_seed0, ap22_seed1,
        ap22_src, ap22_mask, ap22_dst, AP22_SEG_VL, 0U);
    TEST_ASSERT("AP-22 masked vlseg2e8 updates only active segment lanes and does not fill inactive lanes with stale data_delayed bytes",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0U &&
        ap22_bytes_match(ap22_dst, ap22_expected, AP22_SEG2_E8_BYTES,
            &bad_index),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, ap22_dst[bad_index], ap22_expected[bad_index]
    );

    ap22_mask[0] = 0xffU;
    for (unsigned i = 0; i < AP22_SEG_VL; ++i) {
        const bool prefix = i < 3U;
        ap22_expected[2U * i] = prefix ? ap22_seed0[i] : ap22_src[2U * i];
        ap22_expected[2U * i + 1U] =
            prefix ? ap22_seed1[i] : ap22_src[2U * i + 1U];
    }
    ap22_fill_constant(ap22_dst, AP22_MAX_BYTES, 0x6aU);

    TEST_SETUP_EXCEPT();
    ap22_masked_vstart_vlseg2e8_dump(ap22_seed0, ap22_seed1,
        ap22_src, ap22_mask, ap22_dst, AP22_SEG_VL, 3U);
    TEST_ASSERT("AP-22 nonzero vstart vlseg2e8 restart keeps prefix lanes undisturbed and writes only suffix segment payload",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0U &&
        ap22_bytes_match(ap22_dst, ap22_expected, AP22_SEG2_E8_BYTES,
            &bad_index),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, ap22_dst[bad_index], ap22_expected[bad_index]
    );

    TEST_END();
}

bool ai_micro_ap22_cross_page_fault_repair_vlseg2e8_roundtrip_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dst_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x240U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x240U;
    unsigned bad_index = 0;

    ap22_enable_vector_state();
    ap22_fill_pattern(ap22_expected, AP22_SEG2_E8_BYTES, 0x72U);

    goto_priv(PRIV_M);
    ap22_write_bytes(low_paddr, ap22_expected, 8U);
    ap22_write_bytes(high_paddr, ap22_expected + 8U, 8U);
    ap22_fill_constant(ap22_dst, AP22_SEG2_E8_BYTES, 0);
    ap22_write_bytes(dst_paddr, ap22_dst, AP22_SEG2_E8_BYTES);

    goto_priv(PRIV_HS);
    hspt_init();
    ap22_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ap22_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_W);
    ap22_set_hs_leaf(dst_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ap22_roundtrip_vlseg2e8((const uint8_t *)src_vaddr,
        (uint8_t *)dst_vaddr, AP22_SEG_VL);
    TEST_ASSERT("AP-22 cross-page vlseg2e8 first attempt reports the high-page load fault instead of silently using stale delayed data",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == hs_page_base(high_page),
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx",
        excpt.cause, excpt.tval, hs_page_base(high_page)
    );

    goto_priv(PRIV_M);
    ap22_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ap22_roundtrip_vlseg2e8((const uint8_t *)src_vaddr,
        (uint8_t *)dst_vaddr, AP22_SEG_VL);
    TEST_ASSERT("AP-22 repaired cross-page vlseg2e8 retry dumps the full low-plus-high-page segment payload",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AP22_SEG_VL,
        "vl=%lu", CSRR(CSR_VL)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-22 repaired cross-page vlseg2e8 retry has no stale low-bit, old-half, or fault-episode payload",
        ap22_paddr_bytes_match(dst_paddr, ap22_expected, AP22_SEG2_E8_BYTES,
            &bad_index),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, read8(dst_paddr + bad_index), ap22_expected[bad_index]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
