#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>
#include <string.h>

#define AI_AP65_SQ_VS_OFF      9U
#define AI_AP65_SQ_VS_MASK     (3ULL << AI_AP65_SQ_VS_OFF)
#define AI_AP65_SQ_VS_DIRTY    (3ULL << AI_AP65_SQ_VS_OFF)
#define AI_AP65_SQ_FULL_VL     32U
#define AI_AP65_SQ_HALF_VL     16U
#define AI_AP65_SQ_STRIDE      8U
#define AI_AP65_SQ_WINDOW      (AI_AP65_SQ_FULL_VL * AI_AP65_SQ_STRIDE)

static inline pte_t *ai_ap65_sq_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ap65_sq_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, AI_AP65_SQ_VS_MASK);
    CSRS(CSR_MSTATUS, AI_AP65_SQ_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_ap65_sq_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ai_ap65_sq_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_ap65_sq_fill_bytes(uintptr_t paddr, uint8_t seed,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, (uint8_t)(seed + i * 5U + (i >> 2)));
    }
}

static inline void ai_ap65_sq_fill_pattern(uint8_t *bytes, unsigned len,
    uint8_t seed)
{
    for (unsigned i = 0; i < len; ++i) {
        bytes[i] = (uint8_t)(seed + i * 7U + (i ^ (i >> 1)));
    }
}

static inline void ai_ap65_sq_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_ap65_sq_bytes_match(uintptr_t paddr,
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

static inline bool ai_ap65_sq_strided_bytes_match(uintptr_t paddr,
    intptr_t stride, const uint8_t *expected, unsigned vl,
    unsigned *bad_index, uint8_t *bad_actual, uint8_t *bad_expected)
{
    for (unsigned i = 0; i < vl; ++i) {
        const uintptr_t lane_paddr = paddr + (uintptr_t)i * (uintptr_t)stride;
        const uint8_t actual = read8(lane_paddr);

        if (actual != expected[i]) {
            *bad_index = i;
            *bad_actual = actual;
            *bad_expected = expected[i];
            return false;
        }
    }

    *bad_index = vl;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static inline void ai_ap65_sq_vle8_m2_vsse8_m2_marker(
    const uint8_t *src, uint8_t *dst, intptr_t stride, unsigned vl,
    uint64_t *marker_addr, uint64_t marker)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 1);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src])\n\t"
        "vsse8.v v8, (%[dst]), %[stride]\n\t"
        "sd %[marker], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dst] "r"(dst),
          [stride] "r"(stride),
          [vl] "r"(vl),
          [vtype] "r"(vtype),
          [marker_addr] "r"(marker_addr),
          [marker] "r"(marker)
        : "t0", "memory");
}

static inline void ai_ap65_sq_vle8_m2_vsse8_m2_then_fault_load(
    const uint8_t *src, uint8_t *dst, intptr_t stride, unsigned vl,
    const uint64_t *fault_addr, uint64_t *sink)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 1);
    uint64_t value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src])\n\t"
        "vsse8.v v8, (%[dst]), %[stride]\n\t"
        "ld %[value], 0(%[fault_addr])\n\t"
        ".option pop\n\t"
        : [value] "=&r"(value)
        : [src] "r"(src),
          [dst] "r"(dst),
          [stride] "r"(stride),
          [vl] "r"(vl),
          [vtype] "r"(vtype),
          [fault_addr] "r"(fault_addr)
        : "t0", "memory");

    *sink = value;
}

static inline void ai_ap82_two_strided_stores_then_lbu(
    const uint8_t *src0, uint8_t *dst0, const uint8_t *src1, uint8_t *dst1,
    intptr_t stride, const uint8_t *load_addr, uint64_t *observed,
    uint64_t *marker_addr, uint64_t marker)
{
    uint64_t value = 0ULL;
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src0])\n\t"
        "vle8.v v16, (%[src1])\n\t"
        "vsse8.v v8, (%[dst0]), %[stride]\n\t"
        "vsse8.v v16, (%[dst1]), %[stride]\n\t"
        "lbu %[value], 0(%[load_addr])\n\t"
        "sd %[marker], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [value] "=&r"(value)
        : [src0] "r"(src0),
          [dst0] "r"(dst0),
          [src1] "r"(src1),
          [dst1] "r"(dst1),
          [stride] "r"(stride),
          [load_addr] "r"(load_addr),
          [marker_addr] "r"(marker_addr),
          [marker] "r"(marker),
          [vl] "r"(AI_AP65_SQ_HALF_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *observed = value;
}

static inline void ai_ap82_two_vector_loads_dump(
    const uint8_t *src0, const uint8_t *src1, intptr_t stride,
    uint8_t *dump0, uint8_t *dump1, uint64_t *marker_addr, uint64_t marker)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlse8.v v8, (%[src0]), %[stride]\n\t"
        "vlse8.v v16, (%[src1]), %[stride]\n\t"
        "vse8.v v8, (%[dump0])\n\t"
        "vse8.v v16, (%[dump1])\n\t"
        "sd %[marker], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src0] "r"(src0),
          [src1] "r"(src1),
          [stride] "r"(stride),
          [dump0] "r"(dump0),
          [dump1] "r"(dump1),
          [marker_addr] "r"(marker_addr),
          [marker] "r"(marker),
          [vl] "r"(AI_AP65_SQ_HALF_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_ap82_scalar_store_prefill31(uintptr_t base_vaddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < 31U; ++i) {
        sd(base_vaddr + i * sizeof(uint64_t), seed + i);
    }
    asm volatile("fence rw, rw" ::: "memory");
}

static inline bool ai_ap82_scalar_store_prefill31_match(uintptr_t base_paddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < 31U; ++i) {
        if (read64(base_paddr + i * sizeof(uint64_t)) != seed + i) {
            return false;
        }
    }
    return true;
}

static inline void ai_ap82_two_strided_stores_then_fault_load(
    const uint8_t *src0, uint8_t *dst0, const uint8_t *src1, uint8_t *dst1,
    intptr_t stride, const uint64_t *fault_addr, uint64_t *sink)
{
    uint64_t value = 0ULL;
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src0])\n\t"
        "vle8.v v16, (%[src1])\n\t"
        "vsse8.v v8, (%[dst0]), %[stride]\n\t"
        "vsse8.v v16, (%[dst1]), %[stride]\n\t"
        "ld %[value], 0(%[fault_addr])\n\t"
        ".option pop\n\t"
        : [value] "=&r"(value)
        : [src0] "r"(src0),
          [dst0] "r"(dst0),
          [src1] "r"(src1),
          [dst1] "r"(dst1),
          [stride] "r"(stride),
          [fault_addr] "r"(fault_addr),
          [vl] "r"(AI_AP65_SQ_HALF_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *sink = value;
}

bool ai_micro_ap65_ap68_full_vector_store_marker_generation_manual_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t src0_vaddr = hs_page_base(source_page) + 0x100U;
    const uintptr_t src1_vaddr = hs_page_base(source_page) + 0x180U;
    const uintptr_t src0_paddr = phys_page_base(source_page) + 0x100U;
    const uintptr_t src1_paddr = phys_page_base(source_page) + 0x180U;
    const uintptr_t dst0_vaddr = hs_page_base(target_page) + 0x200U;
    const uintptr_t dst1_vaddr = hs_page_base(target_page) + 0x500U;
    const uintptr_t dst0_paddr = phys_page_base(target_page) + 0x200U;
    const uintptr_t dst1_paddr = phys_page_base(target_page) + 0x500U;
    const uintptr_t marker0_vaddr = hs_page_base(target_page) + 0x780U;
    const uintptr_t marker1_vaddr = hs_page_base(target_page) + 0x788U;
    const uintptr_t marker0_paddr = phys_page_base(target_page) + 0x780U;
    const uintptr_t marker1_paddr = phys_page_base(target_page) + 0x788U;
    const uint64_t marker0 = 0x65a068a000000001ULL;
    const uint64_t marker1 = 0x65a068a000000002ULL;
    uint8_t payload0[AI_AP65_SQ_FULL_VL];
    uint8_t payload1[AI_AP65_SQ_FULL_VL];
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_ap65_sq_fill_pattern(payload0, sizeof(payload0), 0x20U);
    ai_ap65_sq_fill_pattern(payload1, sizeof(payload1), 0x90U);

    goto_priv(PRIV_M);
    ai_ap65_sq_enable_vector_state();
    ai_ap65_sq_write_bytes(src0_paddr, payload0, sizeof(payload0));
    ai_ap65_sq_write_bytes(src1_paddr, payload1, sizeof(payload1));
    ai_ap65_sq_fill_bytes(dst0_paddr, 0x40U, AI_AP65_SQ_WINDOW);
    ai_ap65_sq_fill_bytes(dst1_paddr, 0x70U, AI_AP65_SQ_WINDOW);
    write64(marker0_paddr, 0ULL);
    write64(marker1_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap65_sq_vle8_m2_vsse8_m2_marker((const uint8_t *)src0_vaddr,
        (uint8_t *)dst0_vaddr, AI_AP65_SQ_STRIDE, AI_AP65_SQ_FULL_VL,
        (uint64_t *)marker0_vaddr, marker0);
    TEST_ASSERT("AP-65/AP-68 full-vector store generation A retires with no architectural exception before the scalar marker",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP65_SQ_FULL_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap65_sq_vle8_m2_vsse8_m2_marker((const uint8_t *)src1_vaddr,
        (uint8_t *)dst1_vaddr, AI_AP65_SQ_STRIDE, AI_AP65_SQ_FULL_VL,
        (uint64_t *)marker1_vaddr, marker1);
    TEST_ASSERT("AP-65/AP-68 full-vector store generation B also retires after the first generation drains enough for a new marker",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP65_SQ_FULL_VL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-65/AP-68 generation A strided payload and marker stay visible, catching empty/full alias or repeated commit corruption at the black-box boundary",
        ai_ap65_sq_strided_bytes_match(dst0_paddr, AI_AP65_SQ_STRIDE,
            payload0, AI_AP65_SQ_FULL_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        read64(marker0_paddr) == marker0,
        "bad_index=%u actual=0x%x expected=0x%x marker=0x%lx",
        bad_index, bad_actual, bad_expected, read64(marker0_paddr)
    );
    TEST_ASSERT("AP-65/AP-68 generation B strided payload and marker stay visible, stressing back-to-back full-vector enqueue/commit generations",
        ai_ap65_sq_strided_bytes_match(dst1_paddr, AI_AP65_SQ_STRIDE,
            payload1, AI_AP65_SQ_FULL_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        read64(marker1_paddr) == marker1,
        "bad_index=%u actual=0x%x expected=0x%x marker=0x%lx",
        bad_index, bad_actual, bad_expected, read64(marker1_paddr)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap65_ap68_full_vector_store_young_fault_redirect_marker_manual_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fault_page = SWITCH2;
    const uintptr_t src_vaddr = hs_page_base(source_page) + 0x240U;
    const uintptr_t src_paddr = phys_page_base(source_page) + 0x240U;
    const uintptr_t dst_vaddr = hs_page_base(target_page) + 0x880U;
    const uintptr_t dst_paddr = phys_page_base(target_page) + 0x880U;
    const uintptr_t marker_vaddr = hs_page_base(target_page) + 0xbe0U;
    const uintptr_t marker_paddr = phys_page_base(target_page) + 0xbe0U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x100U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x100U;
    const uint64_t marker = 0x65a068b000000003ULL;
    const uint64_t marker_old = 0x0102030405060708ULL;
    uint8_t payload[AI_AP65_SQ_FULL_VL];
    uint64_t fault_sink = 0ULL;
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_ap65_sq_fill_pattern(payload, sizeof(payload), 0xc0U);

    goto_priv(PRIV_M);
    ai_ap65_sq_enable_vector_state();
    ai_ap65_sq_write_bytes(src_paddr, payload, sizeof(payload));
    ai_ap65_sq_fill_bytes(dst_paddr, 0x18U, AI_AP65_SQ_WINDOW);
    write64(marker_paddr, marker_old);
    write64(fault_paddr, 0xfacefeed12345678ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap65_sq_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap65_sq_vle8_m2_vsse8_m2_then_fault_load(
        (const uint8_t *)src_vaddr, (uint8_t *)dst_vaddr,
        AI_AP65_SQ_STRIDE, AI_AP65_SQ_FULL_VL,
        (const uint64_t *)fault_vaddr, &fault_sink);
    asm volatile("fence rw, rw" ::: "memory");
    TEST_ASSERT("AP-65/AP-68 redirect smoke creates a younger load page fault after a 32-lane strided vector store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        CSRR(CSR_VSTART) == 0ULL,
        "triggered=%d cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr,
        CSRR(CSR_VSTART)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-65/AP-68 redirect smoke keeps the older full-vector payload visible and does not execute the skipped marker on the faulting path",
        ai_ap65_sq_strided_bytes_match(dst_paddr, AI_AP65_SQ_STRIDE,
            payload, AI_AP65_SQ_FULL_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        read64(marker_paddr) == marker_old,
        "bad_index=%u actual=0x%x expected=0x%x marker=0x%lx",
        bad_index, bad_actual, bad_expected, read64(marker_paddr)
    );

    goto_priv(PRIV_HS);
    ai_ap65_sq_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    sd(marker_vaddr, marker);
    TEST_ASSERT("AP-65/AP-68 post-redirect scalar marker store completes after the fault is repaired, exposing stale SQ generation aliasing if present",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-65/AP-68 post-redirect marker is visible and the older full-vector payload remains intact after repair",
        ai_ap65_sq_strided_bytes_match(dst_paddr, AI_AP65_SQ_STRIDE,
            payload, AI_AP65_SQ_FULL_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        read64(marker_paddr) == marker,
        "bad_index=%u actual=0x%x expected=0x%x marker=0x%lx",
        bad_index, bad_actual, bad_expected, read64(marker_paddr)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap82_dual_strided_vector_store_dependent_load_sqidx_generation_manual_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = VSRWX_GRWX;
    const uintptr_t src0_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src1_vaddr = hs_page_base(src_page) + 0x380U;
    const uintptr_t src0_paddr = phys_page_base(src_page) + 0x300U;
    const uintptr_t src1_paddr = phys_page_base(src_page) + 0x380U;
    const uintptr_t dst0_vaddr = hs_page_base(dst_page) + 0x100U;
    const uintptr_t dst1_vaddr = hs_page_base(dst_page) + 0x300U;
    const uintptr_t dst0_paddr = phys_page_base(dst_page) + 0x100U;
    const uintptr_t dst1_paddr = phys_page_base(dst_page) + 0x300U;
    const unsigned load_lane = 9U;
    const uintptr_t load_vaddr = dst1_vaddr + load_lane * AI_AP65_SQ_STRIDE;
    const uintptr_t marker_vaddr = hs_page_base(dst_page) + 0x5c0U;
    const uintptr_t marker_paddr = phys_page_base(dst_page) + 0x5c0U;
    const uint64_t marker = 0x82a0000000000004ULL;
    uint8_t payload0[AI_AP65_SQ_HALF_VL];
    uint8_t payload1[AI_AP65_SQ_HALF_VL];
    uint64_t observed = 0ULL;
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_ap65_sq_fill_pattern(payload0, sizeof(payload0), 0x30U);
    ai_ap65_sq_fill_pattern(payload1, sizeof(payload1), 0xa0U);

    goto_priv(PRIV_M);
    ai_ap65_sq_enable_vector_state();
    ai_ap65_sq_write_bytes(src0_paddr, payload0, sizeof(payload0));
    ai_ap65_sq_write_bytes(src1_paddr, payload1, sizeof(payload1));
    ai_ap65_sq_fill_bytes(dst0_paddr, 0x10U,
        AI_AP65_SQ_HALF_VL * AI_AP65_SQ_STRIDE);
    ai_ap65_sq_fill_bytes(dst1_paddr, 0x50U,
        AI_AP65_SQ_HALF_VL * AI_AP65_SQ_STRIDE);
    write64(marker_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap82_two_strided_stores_then_lbu((const uint8_t *)src0_vaddr,
        (uint8_t *)dst0_vaddr, (const uint8_t *)src1_vaddr,
        (uint8_t *)dst1_vaddr, AI_AP65_SQ_STRIDE,
        (const uint8_t *)load_vaddr, &observed,
        (uint64_t *)marker_vaddr, marker);
    TEST_ASSERT("AP-82 dual-strided-store smoke: a younger dependent byte load after two 16-lane vector stores observes the second store payload",
        excpt.triggered == false &&
        observed == payload1[load_lane] &&
        CSRR(CSR_VL) == AI_AP65_SQ_HALF_VL,
        "observed=0x%lx expected=0x%x vl=%lu",
        observed, payload1[load_lane], CSRR(CSR_VL)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-82 dual-strided-store smoke leaves both 16-lane store images and the scalar marker visible after the dependent load",
        ai_ap65_sq_strided_bytes_match(dst0_paddr, AI_AP65_SQ_STRIDE,
            payload0, AI_AP65_SQ_HALF_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        ai_ap65_sq_strided_bytes_match(dst1_paddr, AI_AP65_SQ_STRIDE,
            payload1, AI_AP65_SQ_HALF_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        read64(marker_paddr) == marker,
        "bad_index=%u actual=0x%x expected=0x%x marker=0x%lx",
        bad_index, bad_actual, bad_expected, read64(marker_paddr)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap82_prefill31_dual_strided_vector_store_wrap_value_manual_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = VSRWX_GRWX;
    const uintptr_t prefill_vaddr = hs_page_base(dst_page) + 0x40U;
    const uintptr_t prefill_paddr = phys_page_base(dst_page) + 0x40U;
    const uintptr_t src0_vaddr = hs_page_base(src_page) + 0x420U;
    const uintptr_t src1_vaddr = hs_page_base(src_page) + 0x4a0U;
    const uintptr_t src0_paddr = phys_page_base(src_page) + 0x420U;
    const uintptr_t src1_paddr = phys_page_base(src_page) + 0x4a0U;
    const uintptr_t dst0_vaddr = hs_page_base(dst_page) + 0x680U;
    const uintptr_t dst1_vaddr = hs_page_base(dst_page) + 0x880U;
    const uintptr_t dst0_paddr = phys_page_base(dst_page) + 0x680U;
    const uintptr_t dst1_paddr = phys_page_base(dst_page) + 0x880U;
    const unsigned load_lane = 7U;
    const uintptr_t load_vaddr = dst1_vaddr + load_lane * AI_AP65_SQ_STRIDE;
    const uintptr_t marker_vaddr = hs_page_base(dst_page) + 0xae0U;
    const uintptr_t marker_paddr = phys_page_base(dst_page) + 0xae0U;
    const uint64_t prefill_seed = 0x8200000000001000ULL;
    const uint64_t marker = 0x82c0000000000006ULL;
    uint8_t payload0[AI_AP65_SQ_HALF_VL];
    uint8_t payload1[AI_AP65_SQ_HALF_VL];
    uint64_t observed = 0ULL;
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_ap65_sq_fill_pattern(payload0, sizeof(payload0), 0x55U);
    ai_ap65_sq_fill_pattern(payload1, sizeof(payload1), 0xd0U);

    goto_priv(PRIV_M);
    ai_ap65_sq_enable_vector_state();
    for (unsigned i = 0; i < 31U; ++i) {
        write64(prefill_paddr + i * sizeof(uint64_t), 0ULL);
    }
    ai_ap65_sq_write_bytes(src0_paddr, payload0, sizeof(payload0));
    ai_ap65_sq_write_bytes(src1_paddr, payload1, sizeof(payload1));
    ai_ap65_sq_fill_bytes(dst0_paddr, 0x19U,
        AI_AP65_SQ_HALF_VL * AI_AP65_SQ_STRIDE);
    ai_ap65_sq_fill_bytes(dst1_paddr, 0x69U,
        AI_AP65_SQ_HALF_VL * AI_AP65_SQ_STRIDE);
    write64(marker_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_ap82_scalar_store_prefill31(prefill_vaddr, prefill_seed);
    ai_ap82_two_strided_stores_then_lbu((const uint8_t *)src0_vaddr,
        (uint8_t *)dst0_vaddr, (const uint8_t *)src1_vaddr,
        (uint8_t *)dst1_vaddr, AI_AP65_SQ_STRIDE,
        (const uint8_t *)load_vaddr, &observed,
        (uint64_t *)marker_vaddr, marker);
    TEST_ASSERT("AP-82 value-wrap smoke uses 31 scalar stores before two 16-lane strided stores; the younger load still sees the second vector-store lane",
        excpt.triggered == false &&
        observed == payload1[load_lane],
        "observed=0x%lx expected=0x%x",
        observed, payload1[load_lane]
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-82 value-wrap smoke keeps the scalar prefill, both vector-store images, and marker visible after the near-tail SQ pressure",
        ai_ap82_scalar_store_prefill31_match(prefill_paddr, prefill_seed) &&
        ai_ap65_sq_strided_bytes_match(dst0_paddr, AI_AP65_SQ_STRIDE,
            payload0, AI_AP65_SQ_HALF_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        ai_ap65_sq_strided_bytes_match(dst1_paddr, AI_AP65_SQ_STRIDE,
            payload1, AI_AP65_SQ_HALF_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        read64(marker_paddr) == marker,
        "bad_index=%u actual=0x%x expected=0x%x marker=0x%lx",
        bad_index, bad_actual, bad_expected, read64(marker_paddr)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap82_dual_strided_vector_store_young_fault_redirect_manual_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = VSRWX_GRWX;
    const enum test_page fault_page = SWITCH2;
    const uintptr_t src0_vaddr = hs_page_base(src_page) + 0x540U;
    const uintptr_t src1_vaddr = hs_page_base(src_page) + 0x5c0U;
    const uintptr_t src0_paddr = phys_page_base(src_page) + 0x540U;
    const uintptr_t src1_paddr = phys_page_base(src_page) + 0x5c0U;
    const uintptr_t dst0_vaddr = hs_page_base(dst_page) + 0xb80U;
    const uintptr_t dst1_vaddr = hs_page_base(dst_page) + 0xd80U;
    const uintptr_t dst0_paddr = phys_page_base(dst_page) + 0xb80U;
    const uintptr_t dst1_paddr = phys_page_base(dst_page) + 0xd80U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x180U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x180U;
    const uintptr_t marker_vaddr = hs_page_base(dst_page) + 0xfa0U;
    const uintptr_t marker_paddr = phys_page_base(dst_page) + 0xfa0U;
    const uint64_t marker = 0x82a0000000000007ULL;
    const uint64_t marker_old = 0x0706050403020100ULL;
    uint8_t payload0[AI_AP65_SQ_HALF_VL];
    uint8_t payload1[AI_AP65_SQ_HALF_VL];
    uint64_t fault_sink = 0ULL;
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_ap65_sq_fill_pattern(payload0, sizeof(payload0), 0x64U);
    ai_ap65_sq_fill_pattern(payload1, sizeof(payload1), 0xe4U);

    goto_priv(PRIV_M);
    ai_ap65_sq_enable_vector_state();
    ai_ap65_sq_write_bytes(src0_paddr, payload0, sizeof(payload0));
    ai_ap65_sq_write_bytes(src1_paddr, payload1, sizeof(payload1));
    ai_ap65_sq_fill_bytes(dst0_paddr, 0x24U,
        AI_AP65_SQ_HALF_VL * AI_AP65_SQ_STRIDE);
    ai_ap65_sq_fill_bytes(dst1_paddr, 0x84U,
        AI_AP65_SQ_HALF_VL * AI_AP65_SQ_STRIDE);
    write64(fault_paddr, 0xcafebabedeadbeefULL);
    write64(marker_paddr, marker_old);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap65_sq_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap82_two_strided_stores_then_fault_load((const uint8_t *)src0_vaddr,
        (uint8_t *)dst0_vaddr, (const uint8_t *)src1_vaddr,
        (uint8_t *)dst1_vaddr, AI_AP65_SQ_STRIDE,
        (const uint64_t *)fault_vaddr, &fault_sink);
    asm volatile("fence rw, rw" ::: "memory");
    TEST_ASSERT("AP-82 redirect smoke creates a younger load page fault immediately after two 16-lane strided vector stores",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr,
        "triggered=%d cause=0x%lx tval=0x%lx expected=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-82 redirect smoke keeps both older vector-store images visible while the skipped marker is still old",
        ai_ap65_sq_strided_bytes_match(dst0_paddr, AI_AP65_SQ_STRIDE,
            payload0, AI_AP65_SQ_HALF_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        ai_ap65_sq_strided_bytes_match(dst1_paddr, AI_AP65_SQ_STRIDE,
            payload1, AI_AP65_SQ_HALF_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        read64(marker_paddr) == marker_old,
        "bad_index=%u actual=0x%x expected=0x%x marker=0x%lx",
        bad_index, bad_actual, bad_expected, read64(marker_paddr)
    );

    goto_priv(PRIV_HS);
    ai_ap65_sq_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    sd(marker_vaddr, marker);
    TEST_ASSERT("AP-82 redirect smoke can execute a repaired-path scalar marker after the dual-vector-store dispatch window",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-82 redirect smoke keeps both vector-store images and repaired marker visible after the redirect/recovery episode",
        ai_ap65_sq_strided_bytes_match(dst0_paddr, AI_AP65_SQ_STRIDE,
            payload0, AI_AP65_SQ_HALF_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        ai_ap65_sq_strided_bytes_match(dst1_paddr, AI_AP65_SQ_STRIDE,
            payload1, AI_AP65_SQ_HALF_VL, &bad_index, &bad_actual,
            &bad_expected) &&
        read64(marker_paddr) == marker,
        "bad_index=%u actual=0x%x expected=0x%x marker=0x%lx",
        bad_index, bad_actual, bad_expected, read64(marker_paddr)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap82_dual_vector_load_lqidx_generation_manual_corner()
{
    TEST_START();

    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page dump_page = SWITCH1;
    const uintptr_t src0_vaddr = hs_page_base(src_page) + 0x780U;
    const uintptr_t src1_vaddr = hs_page_base(src_page) + 0x980U;
    const uintptr_t src0_paddr = phys_page_base(src_page) + 0x780U;
    const uintptr_t src1_paddr = phys_page_base(src_page) + 0x980U;
    const uintptr_t dump0_vaddr = hs_page_base(dump_page) + 0x100U;
    const uintptr_t dump1_vaddr = hs_page_base(dump_page) + 0x180U;
    const uintptr_t dump0_paddr = phys_page_base(dump_page) + 0x100U;
    const uintptr_t dump1_paddr = phys_page_base(dump_page) + 0x180U;
    const uintptr_t marker_vaddr = hs_page_base(dump_page) + 0x240U;
    const uintptr_t marker_paddr = phys_page_base(dump_page) + 0x240U;
    const uint64_t marker = 0x82e0000000000005ULL;
    uint8_t payload0[AI_AP65_SQ_HALF_VL];
    uint8_t payload1[AI_AP65_SQ_HALF_VL];
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;

    ai_ap65_sq_fill_pattern(payload0, sizeof(payload0), 0x44U);
    ai_ap65_sq_fill_pattern(payload1, sizeof(payload1), 0xbbU);

    goto_priv(PRIV_M);
    ai_ap65_sq_enable_vector_state();
    for (unsigned i = 0; i < AI_AP65_SQ_HALF_VL; ++i) {
        write8(src0_paddr + i * AI_AP65_SQ_STRIDE, payload0[i]);
        write8(src1_paddr + i * AI_AP65_SQ_STRIDE, payload1[i]);
    }
    ai_ap65_sq_fill_bytes(dump0_paddr, 0x11U, AI_AP65_SQ_HALF_VL);
    ai_ap65_sq_fill_bytes(dump1_paddr, 0x22U, AI_AP65_SQ_HALF_VL);
    write64(marker_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap82_two_vector_loads_dump((const uint8_t *)src0_vaddr,
        (const uint8_t *)src1_vaddr, AI_AP65_SQ_STRIDE,
        (uint8_t *)dump0_vaddr, (uint8_t *)dump1_vaddr,
        (uint64_t *)marker_vaddr, marker);
    TEST_ASSERT("AP-82 LQ counterpart smoke completes two 16-lane strided vector loads and dumps both register images",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP65_SQ_HALF_VL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-82 LQ counterpart smoke preserves both vector-load result images and the following scalar marker",
        ai_ap65_sq_bytes_match(dump0_paddr, payload0, AI_AP65_SQ_HALF_VL,
            &bad_index, &bad_actual, &bad_expected) &&
        ai_ap65_sq_bytes_match(dump1_paddr, payload1, AI_AP65_SQ_HALF_VL,
            &bad_index, &bad_actual, &bad_expected) &&
        read64(marker_paddr) == marker,
        "bad_index=%u actual=0x%x expected=0x%x marker=0x%lx",
        bad_index, bad_actual, bad_expected, read64(marker_paddr)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}
