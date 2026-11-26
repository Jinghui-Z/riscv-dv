#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <stddef.h>

#define AI_AP92_VS_OFF             9U
#define AI_AP92_VS_MASK            (3ULL << AI_AP92_VS_OFF)
#define AI_AP92_VS_DIRTY           (3ULL << AI_AP92_VS_OFF)
#define AI_AP92_MAX_VECTOR_BYTES   256U
#define AI_AP92_UNIT_SLOTS         8U
#define AI_AP92_SEG_SLOTS          4U
#define AI_AP92_UNIT_STEP          0x180U
#define AI_AP92_SEG_STEP           0x300U
#define AI_AP92_SRC_BASE_OFF       0x80U
#define AI_AP92_DST_BASE_OFF       0x80U
#define AI_AP92_STORE_WINDOW_BYTES 24U

static inline void ai_ap92_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, AI_AP92_VS_MASK);
    CSRS(CSR_MSTATUS, AI_AP92_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint64_t ai_ap92_read_vlenb(void)
{
    return CSRR(CSR_VLENB);
}

static inline pte_t *ai_ap92_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ap92_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ai_ap92_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_ap92_prepare_hs_default(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

static inline uint8_t ai_ap92_pattern_byte(unsigned round, unsigned slot,
    unsigned index, uint8_t salt)
{
    return (uint8_t)(salt + round * 0x23U + slot * 0x17U +
        index * 0x0dU + ((index ^ slot) & 0x7U));
}

static inline void ai_ap92_fill_pattern(uintptr_t paddr, unsigned bytes,
    unsigned round, unsigned slot, uint8_t salt)
{
    for (unsigned i = 0; i < bytes; ++i) {
        write8(paddr + i, ai_ap92_pattern_byte(round, slot, i, salt));
    }
}

static inline void ai_ap92_fill_seed(uintptr_t paddr, unsigned bytes,
    uint8_t seed)
{
    for (unsigned i = 0; i < bytes; ++i) {
        write8(paddr + i, (uint8_t)(seed + i * 3U));
    }
}

static inline bool ai_ap92_check_pattern(uintptr_t paddr, unsigned bytes,
    unsigned round, unsigned slot, uint8_t salt, unsigned *bad_index,
    uint8_t *bad_actual, uint8_t *bad_expected)
{
    for (unsigned i = 0; i < bytes; ++i) {
        const uint8_t expected =
            ai_ap92_pattern_byte(round, slot, i, salt);
        const uint8_t actual = read8(paddr + i);

        if (actual != expected) {
            *bad_index = i;
            *bad_actual = actual;
            *bad_expected = expected;
            return false;
        }
    }

    *bad_index = bytes;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static inline bool ai_ap92_check_seed(uintptr_t paddr, unsigned bytes,
    uint8_t seed)
{
    for (unsigned i = 0; i < bytes; ++i) {
        if (read8(paddr + i) != (uint8_t)(seed + i * 3U)) {
            return false;
        }
    }
    return true;
}

static inline void ai_ap92_unit_stride_burst4(
    const uint8_t *src0, uint8_t *dst0,
    const uint8_t *src1, uint8_t *dst1,
    const uint8_t *src2, uint8_t *dst2,
    const uint8_t *src3, uint8_t *dst3,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[src0])\n\t"
        "vle8.v v9, (%[src1])\n\t"
        "vle8.v v10, (%[src2])\n\t"
        "vle8.v v11, (%[src3])\n\t"
        "vse8.v v8, (%[dst0])\n\t"
        "vse8.v v9, (%[dst1])\n\t"
        "vse8.v v10, (%[dst2])\n\t"
        "vse8.v v11, (%[dst3])\n\t"
        ".option pop\n\t"
        :
        : [src0] "r"(src0), [dst0] "r"(dst0),
          [src1] "r"(src1), [dst1] "r"(dst1),
          [src2] "r"(src2), [dst2] "r"(dst2),
          [src3] "r"(src3), [dst3] "r"(dst3),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap92_vle8_single(const uint8_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap92_vlseg2e8_burst4(
    const uint8_t *src0, uint8_t *dst0,
    const uint8_t *src1, uint8_t *dst1,
    const uint8_t *src2, uint8_t *dst2,
    const uint8_t *src3, uint8_t *dst3,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vlseg2e8.v v8, (%[src0])\n\t"
        "vlseg2e8.v v10, (%[src1])\n\t"
        "vlseg2e8.v v12, (%[src2])\n\t"
        "vlseg2e8.v v14, (%[src3])\n\t"
        "vsseg2e8.v v8, (%[dst0])\n\t"
        "vsseg2e8.v v10, (%[dst1])\n\t"
        "vsseg2e8.v v12, (%[dst2])\n\t"
        "vsseg2e8.v v14, (%[dst3])\n\t"
        ".option pop\n\t"
        :
        : [src0] "r"(src0), [dst0] "r"(dst0),
          [src1] "r"(src1), [dst1] "r"(dst1),
          [src2] "r"(src2), [dst2] "r"(dst2),
          [src3] "r"(src3), [dst3] "r"(dst3),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline uint64_t ai_ap92_pack_le64(const uint8_t *bytes,
    unsigned start)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= (uint64_t)bytes[start + i] << (i * 8U);
    }
    return value;
}

static inline void ai_ap92_store_le64(uint8_t *bytes, unsigned start,
    uint64_t value)
{
    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        bytes[start + i] = (uint8_t)(value >> (i * 8U));
    }
}

static inline void ai_ap92_write_window(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_ap92_window_matches(uintptr_t paddr,
    const uint8_t *expected, unsigned len, unsigned *bad_index,
    uint8_t *bad_actual, uint8_t *bad_expected)
{
    for (unsigned i = 0; i < len; ++i) {
        const uint8_t actual = read8(paddr + i);

        if (actual != expected[i]) {
            *bad_index = i;
            *bad_actual = actual;
            *bad_expected = expected[i];
            return false;
        }
    }

    *bad_index = len;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

bool ai_micro_ap92_vlmergebuffer_dense_unit_stride_progress_corner()
{
    TEST_START();

    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page dst_page = SWITCH1;
    uint64_t vlenb = 0ULL;
    unsigned active_vlenb = 16U;
    bool all_ok = true;
    unsigned bad_slot = 0U;
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_ap92_enable_vector_state();
    vlenb = ai_ap92_read_vlenb();
    TEST_ASSERT("AP-92 dense unit-stride vload pressure uses a VLENB that fits one page per source/destination arena",
        vlenb > 0U && vlenb <= AI_AP92_MAX_VECTOR_BYTES,
        "vlenb=%lu max=%u", vlenb, AI_AP92_MAX_VECTOR_BYTES
    );
    if (vlenb > 0U && vlenb <= AI_AP92_MAX_VECTOR_BYTES) {
        active_vlenb = (unsigned)vlenb;
    }

    for (unsigned round = 0; round < 2U; ++round) {
        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_AP92_UNIT_SLOTS; ++slot) {
            const uintptr_t src_paddr = phys_page_base(src_page) +
                AI_AP92_SRC_BASE_OFF + (uintptr_t)slot * AI_AP92_UNIT_STEP;
            const uintptr_t dst_paddr = phys_page_base(dst_page) +
                AI_AP92_DST_BASE_OFF + (uintptr_t)slot * AI_AP92_UNIT_STEP;

            ai_ap92_fill_pattern(src_paddr, active_vlenb, round, slot,
                0x31U);
            ai_ap92_fill_seed(dst_paddr, active_vlenb, 0xa0U + slot);
        }

        ai_ap92_prepare_hs_default();
        CSRW(CSR_VSTART, 0ULL);

        TEST_SETUP_EXCEPT();
        ai_ap92_unit_stride_burst4(
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF),
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                1U * AI_AP92_UNIT_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                1U * AI_AP92_UNIT_STEP),
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                2U * AI_AP92_UNIT_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                2U * AI_AP92_UNIT_STEP),
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                3U * AI_AP92_UNIT_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                3U * AI_AP92_UNIT_STEP),
            active_vlenb);
        ai_ap92_unit_stride_burst4(
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                4U * AI_AP92_UNIT_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                4U * AI_AP92_UNIT_STEP),
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                5U * AI_AP92_UNIT_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                5U * AI_AP92_UNIT_STEP),
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                6U * AI_AP92_UNIT_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                6U * AI_AP92_UNIT_STEP),
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                7U * AI_AP92_UNIT_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                7U * AI_AP92_UNIT_STEP),
            active_vlenb);
        fence_iorw_iorw();
        TEST_ASSERT("AP-92 dense unit-stride vload burst retires without a no-progress exception while younger vloads are queued behind the first vector load",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_AP92_UNIT_SLOTS; ++slot) {
            const uintptr_t dst_paddr = phys_page_base(dst_page) +
                AI_AP92_DST_BASE_OFF + (uintptr_t)slot * AI_AP92_UNIT_STEP;

            if (!ai_ap92_check_pattern(dst_paddr, active_vlenb, round,
                slot, 0x31U, &bad_index, &bad_actual, &bad_expected)) {
                all_ok = false;
                bad_slot = slot;
                break;
            }
        }
        TEST_ASSERT("AP-92 dense unit-stride vload burst writes every destination slot exactly once, showing VLMergeBuffer entries drain and the oldest vload is not starved",
            all_ok,
            "round=%u slot=%u bad_index=%u actual=0x%x expected=0x%x",
            round, bad_slot, bad_index, bad_actual, bad_expected
        );
    }

    TEST_END();
}

bool ai_micro_ap92_vlmergebuffer_segment_burst_oldest_progress_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH2;
    const enum test_page dst_page = SCRATCHPAD;
    uint64_t vlenb = 0ULL;
    unsigned active_vlenb = 16U;
    const unsigned fields = 2U;
    bool all_ok = true;
    unsigned bad_slot = 0U;
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_ap92_enable_vector_state();
    vlenb = ai_ap92_read_vlenb();
    TEST_ASSERT("AP-92 segmented vload burst uses a VLENB whose nf=2 payload fits the page-local pressure arena",
        vlenb > 0U && (fields * vlenb) <= AI_AP92_MAX_VECTOR_BYTES * 2U,
        "vlenb=%lu max_payload=%u", vlenb,
        AI_AP92_MAX_VECTOR_BYTES * 2U
    );
    if (vlenb > 0U && (fields * vlenb) <= AI_AP92_MAX_VECTOR_BYTES * 2U) {
        active_vlenb = (unsigned)vlenb;
    }

    for (unsigned round = 0; round < 2U; ++round) {
        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_AP92_SEG_SLOTS; ++slot) {
            const uintptr_t src_paddr = phys_page_base(src_page) +
                AI_AP92_SRC_BASE_OFF + (uintptr_t)slot * AI_AP92_SEG_STEP;
            const uintptr_t dst_paddr = phys_page_base(dst_page) +
                AI_AP92_DST_BASE_OFF + (uintptr_t)slot * AI_AP92_SEG_STEP;

            ai_ap92_fill_pattern(src_paddr, fields * active_vlenb, round,
                slot, 0x51U);
            ai_ap92_fill_seed(dst_paddr, fields * active_vlenb,
                0xc0U + slot);
        }

        ai_ap92_prepare_hs_default();
        CSRW(CSR_VSTART, 0ULL);

        TEST_SETUP_EXCEPT();
        ai_ap92_vlseg2e8_burst4(
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF),
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                1U * AI_AP92_SEG_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                1U * AI_AP92_SEG_STEP),
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                2U * AI_AP92_SEG_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                2U * AI_AP92_SEG_STEP),
            (const uint8_t *)(hs_page_base(src_page) + AI_AP92_SRC_BASE_OFF +
                3U * AI_AP92_SEG_STEP),
            (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
                3U * AI_AP92_SEG_STEP),
            active_vlenb);
        fence_iorw_iorw();
        TEST_ASSERT("AP-92 segmented vload burst retires without stuck while multiple younger split flows compete for VLMergeBuffer entries",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned slot = 0; slot < AI_AP92_SEG_SLOTS; ++slot) {
            const uintptr_t dst_paddr = phys_page_base(dst_page) +
                AI_AP92_DST_BASE_OFF + (uintptr_t)slot * AI_AP92_SEG_STEP;

            if (!ai_ap92_check_pattern(dst_paddr, fields * active_vlenb,
                round, slot, 0x51U, &bad_index, &bad_actual,
                &bad_expected)) {
                all_ok = false;
                bad_slot = slot;
                break;
            }
        }
        TEST_ASSERT("AP-92 segmented vload burst preserves every field payload, covering the oldest-vload reservation symptom under heavier split-flow allocation",
            all_ok,
            "round=%u slot=%u bad_index=%u actual=0x%x expected=0x%x",
            round, bad_slot, bad_index, bad_actual, bad_expected
        );
    }

    TEST_END();
}

bool ai_micro_ap92_faulted_vector_load_then_repaired_burst_no_ghost_threshold_corner()
{
    TEST_START();

    const enum test_page fault_page = VSRWX_GRWX;
    const enum test_page extra_page = SWITCH2;
    const enum test_page dst_page = SWITCH1;
    uint64_t vlenb = 0ULL;
    unsigned active_vlenb = 16U;
    bool all_ok = true;
    unsigned bad_slot = 0U;
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_ap92_enable_vector_state();
    vlenb = ai_ap92_read_vlenb();
    TEST_ASSERT("AP-92 fault-cancel smoke uses a VLENB that fits the repaired dense vload burst arena",
        vlenb > 0U && vlenb <= AI_AP92_MAX_VECTOR_BYTES,
        "vlenb=%lu max=%u", vlenb, AI_AP92_MAX_VECTOR_BYTES
    );
    if (vlenb > 0U && vlenb <= AI_AP92_MAX_VECTOR_BYTES) {
        active_vlenb = (unsigned)vlenb;
    }

    goto_priv(PRIV_M);
    for (unsigned slot = 0; slot < 2U; ++slot) {
        ai_ap92_fill_pattern(phys_page_base(fault_page) +
            AI_AP92_SRC_BASE_OFF + (uintptr_t)slot * AI_AP92_UNIT_STEP,
            active_vlenb, 0U, slot, 0x71U);
    }
    for (unsigned slot = 0; slot < 2U; ++slot) {
        ai_ap92_fill_pattern(phys_page_base(extra_page) +
            AI_AP92_SRC_BASE_OFF + (uintptr_t)slot * AI_AP92_UNIT_STEP,
            active_vlenb, 0U, slot + 2U, 0x71U);
    }
    for (unsigned slot = 0; slot < 4U; ++slot) {
        ai_ap92_fill_seed(phys_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
            (uintptr_t)slot * AI_AP92_UNIT_STEP, active_vlenb,
            0xe0U + slot);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap92_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_ap92_vle8_single((const uint8_t *)(hs_page_base(fault_page) +
        AI_AP92_SRC_BASE_OFF), active_vlenb);
    TEST_ASSERT("AP-92 fault-cancel smoke first makes the oldest vector load fault before it can allocate a useful VLMergeBuffer entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == hs_page_base(fault_page) + AI_AP92_SRC_BASE_OFF,
        "triggered=%d cause=%lu tval=0x%lx expected=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval,
        hs_page_base(fault_page) + AI_AP92_SRC_BASE_OFF
    );

    goto_priv(PRIV_HS);
    ai_ap92_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_ap92_unit_stride_burst4(
        (const uint8_t *)(hs_page_base(fault_page) + AI_AP92_SRC_BASE_OFF),
        (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF),
        (const uint8_t *)(hs_page_base(fault_page) + AI_AP92_SRC_BASE_OFF +
            1U * AI_AP92_UNIT_STEP),
        (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
            1U * AI_AP92_UNIT_STEP),
        (const uint8_t *)(hs_page_base(extra_page) + AI_AP92_SRC_BASE_OFF),
        (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
            2U * AI_AP92_UNIT_STEP),
        (const uint8_t *)(hs_page_base(extra_page) + AI_AP92_SRC_BASE_OFF +
            1U * AI_AP92_UNIT_STEP),
        (uint8_t *)(hs_page_base(dst_page) + AI_AP92_DST_BASE_OFF +
            3U * AI_AP92_UNIT_STEP),
        active_vlenb);
    fence_iorw_iorw();
    TEST_ASSERT("after repairing the faulted oldest vector load, AP-92 dense burst still makes progress instead of inheriting a ghost near-full threshold from the canceled request",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    for (unsigned slot = 0; slot < 4U; ++slot) {
        const uintptr_t dst_paddr = phys_page_base(dst_page) +
            AI_AP92_DST_BASE_OFF + (uintptr_t)slot * AI_AP92_UNIT_STEP;

        if (!ai_ap92_check_pattern(dst_paddr, active_vlenb, 0U, slot,
            0x71U, &bad_index, &bad_actual, &bad_expected)) {
            all_ok = false;
            bad_slot = slot;
            break;
        }
    }
    TEST_ASSERT("AP-92 repaired burst writes all destinations after the fault episode, so canceled or non-accepted vector requests do not leave sticky VLMergeBuffer pressure",
        all_ok,
        "slot=%u bad_index=%u actual=0x%x expected=0x%x",
        bad_slot, bad_index, bad_actual, bad_expected
    );

    TEST_END();
}

bool ai_micro_ap92_storequeue_non_cross_unaligned_forwarding_no_overreplay_corner()
{
    TEST_START();

    const enum test_page non_cross_page = VSRWX_GRWX;
    const enum test_page cross_page = SWITCH1;
    const enum test_page independent_page = SWITCH2;
    const uintptr_t non_cross_base_vaddr =
        hs_page_base(non_cross_page) + 0x200U;
    const uintptr_t non_cross_base_paddr =
        phys_page_base(non_cross_page) + 0x200U;
    const uintptr_t cross_base_vaddr = hs_page_base(cross_page) + 0x300U;
    const uintptr_t cross_base_paddr = phys_page_base(cross_page) + 0x300U;
    const uintptr_t independent_vaddr =
        hs_page_base(independent_page) + 0x280U;
    const uintptr_t independent_paddr =
        phys_page_base(independent_page) + 0x280U;
    const unsigned non_cross_off = 1U;
    const unsigned cross_off = 9U;
    const uint64_t non_cross_value = 0x1122334455667788ULL;
    const uint64_t cross_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t independent_value = 0x0badc0ffee123456ULL;
    uint8_t non_cross_expected[AI_AP92_STORE_WINDOW_BYTES];
    uint8_t cross_expected[AI_AP92_STORE_WINDOW_BYTES];
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;
    uint64_t non_cross_observed = 0ULL;
    uint64_t cross_observed = 0ULL;
    uint64_t independent_observed = 0ULL;

    for (unsigned i = 0; i < AI_AP92_STORE_WINDOW_BYTES; ++i) {
        non_cross_expected[i] = (uint8_t)(0x40U + i);
        cross_expected[i] = (uint8_t)(0x80U + i * 2U);
    }
    ai_ap92_store_le64(non_cross_expected, non_cross_off,
        non_cross_value);
    ai_ap92_store_le64(cross_expected, cross_off, cross_value);

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_AP92_STORE_WINDOW_BYTES; ++i) {
        write8(non_cross_base_paddr + i, (uint8_t)(0x40U + i));
        write8(cross_base_paddr + i, (uint8_t)(0x80U + i * 2U));
    }
    write64(independent_paddr, independent_value);

    ai_ap92_prepare_hs_default();

    TEST_SETUP_EXCEPT();
    sd(non_cross_base_vaddr + non_cross_off, non_cross_value);
    non_cross_observed = ld(non_cross_base_vaddr + non_cross_off);
    independent_observed = ld(independent_vaddr);
    TEST_ASSERT("AP-92 StoreQueue non-cross unaligned store forwards to an immediate overlapping load and does not over-replay solely because unaligned is set",
        excpt.triggered == false &&
        non_cross_observed == non_cross_value &&
        independent_observed == independent_value,
        "observed=0x%lx expected=0x%lx independent=0x%lx",
        non_cross_observed, non_cross_value, independent_observed
    );

    TEST_SETUP_EXCEPT();
    sd(cross_base_vaddr + cross_off, cross_value);
    cross_observed = ld(cross_base_vaddr + cross_off);
    TEST_ASSERT("AP-92 StoreQueue cross-16B unaligned store/load pair still completes correctly on the real split path after the non-cross direct-path probe",
        excpt.triggered == false &&
        cross_observed == cross_value,
        "observed=0x%lx expected=0x%lx", cross_observed, cross_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-92 non-cross unaligned store updates only the same-16B byte window expected from the committed store",
        ai_ap92_window_matches(non_cross_base_paddr, non_cross_expected,
            AI_AP92_STORE_WINDOW_BYTES, &bad_index, &bad_actual,
            &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );
    TEST_ASSERT("AP-92 cross-16B unaligned store still updates the split byte window while leaving unrelated independent data intact",
        ai_ap92_window_matches(cross_base_paddr, cross_expected,
            AI_AP92_STORE_WINDOW_BYTES, &bad_index, &bad_actual,
            &bad_expected) &&
        read64(independent_paddr) == independent_value,
        "bad_index=%u actual=0x%x expected=0x%x independent=0x%lx",
        bad_index, bad_actual, bad_expected, read64(independent_paddr)
    );

    TEST_END();
}
