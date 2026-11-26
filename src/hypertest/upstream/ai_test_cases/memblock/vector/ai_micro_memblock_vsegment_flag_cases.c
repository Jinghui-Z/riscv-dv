#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_VSEG_FIELDS 2U
#define AI_MEMBLOCK_VSEG_VL 8U
#define AI_MEMBLOCK_VSEG_BYTES (AI_MEMBLOCK_VSEG_FIELDS * AI_MEMBLOCK_VSEG_VL)

static inline pte_t *ai_memblock_vseg_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_vseg_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_memblock_vseg_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_memblock_vseg_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_memblock_vseg_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_vseg_expected_cross_page(
    uint8_t *expected, const uint8_t *low, const uint8_t *high)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_VSEG_VL; ++i) {
        expected[i] = low[i];
        expected[AI_MEMBLOCK_VSEG_VL + i] = high[i];
    }
}

static inline void ai_memblock_vseg_vlseg2e8_to_v8(const uint8_t *src)
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
          [vl] "r"((int)AI_MEMBLOCK_VSEG_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_memblock_vseg_vsseg2e8_from_v8(uint8_t *dst)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vsseg2e8.v v8, (%[dst])\n\t"
        ".option pop\n\t"
        : [dst] "+r"(dst)
        : [vl] "r"((int)AI_MEMBLOCK_VSEG_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_vsegment_fault_clears_memblock_flag_followup_scalar_load_progress_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page scalar_page = VSRWX_GRWX;
    const enum test_page dst_page = SCRATCHPAD;
    const uintptr_t fault_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_VSEG_VL;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_VSEG_VL;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t scalar_vaddr = hs_page_base(scalar_page) + 0x300U;
    const uintptr_t scalar_paddr = phys_page_base(scalar_page) + 0x300U;
    const uintptr_t scalar_guard_paddr = scalar_paddr + sizeof(uint64_t);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x380U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x380U;
    const uintptr_t dst_before_paddr = dst_paddr - 1U;
    const uintptr_t dst_after_paddr = dst_paddr + AI_MEMBLOCK_VSEG_BYTES;
    const uint8_t low_bytes[AI_MEMBLOCK_VSEG_VL] = {
        0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
    };
    const uint8_t high_bytes[AI_MEMBLOCK_VSEG_VL] = {
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    };
    uint8_t expected[AI_MEMBLOCK_VSEG_BYTES];
    const uint8_t dst_fill = 0x3eU;
    const uint8_t dst_before = 0x6aU;
    const uint8_t dst_after = 0xa6U;
    const uint64_t scalar_seed = 0x1122334455667788ULL;
    const uint64_t scalar_update = 0x8877665544332211ULL;
    const uint64_t scalar_guard = 0xfeedfacecafebeefULL;
    uint64_t observed_vstart = 0ULL;

    ai_memblock_vseg_expected_cross_page(expected, low_bytes, high_bytes);

    goto_priv(PRIV_M);
    ai_memblock_vseg_write_bytes(low_tail_paddr, low_bytes,
        AI_MEMBLOCK_VSEG_VL);
    ai_memblock_vseg_write_bytes(high_head_paddr, high_bytes,
        AI_MEMBLOCK_VSEG_VL);
    for (unsigned i = 0; i < AI_MEMBLOCK_VSEG_BYTES; ++i) {
        write8(dst_paddr + i, dst_fill);
    }
    write8(dst_before_paddr, dst_before);
    write8(dst_after_paddr, dst_after);
    write64(scalar_paddr, scalar_seed);
    write64(scalar_guard_paddr, scalar_guard);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_vseg_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_vseg_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_memblock_vseg_set_hs_leaf(scalar_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_vseg_set_hs_leaf(dst_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_vseg_vlseg2e8_to_v8((const uint8_t *)dst_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the vsegment destination registers are prefilled so later faulting/later fields can be checked for writeback suppression",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_memblock_vseg_vlseg2e8_to_v8((const uint8_t *)fault_src_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("cross-page vlseg2e8 reports a load page fault on the first segment whose second field reaches the execute-only page",
        excpt.triggered &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == hs_page_base(high_page) &&
        observed_vstart == 4ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_memblock_vseg_vsseg2e8_from_v8((uint8_t *)dst_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the vsegment load fault, dumping the destination registers for prefix/fault suppression inspection does not take a second exception",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first-field segment fault, only prefix segment bytes before vstart are architecturally visible while fault and later bytes stay prefilled",
        ai_memblock_vseg_bytes_match(dst_paddr, low_bytes, AI_MEMBLOCK_VSEG_VL)
    );
    for (unsigned i = AI_MEMBLOCK_VSEG_VL; i < AI_MEMBLOCK_VSEG_BYTES; ++i) {
        TEST_ASSERT("the faulting and later segment bytes remain prefilled after the vsegment load fault",
            read8(dst_paddr + i) == dst_fill
        );
    }
    TEST_ASSERT("the vsegment prefix/fault suppression probe also preserves adjacent destination sentinels",
        read8(dst_before_paddr) == dst_before &&
        read8(dst_after_paddr) == dst_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    const uint64_t scalar_observed = ld(scalar_vaddr);
    sd(scalar_vaddr, scalar_update);
    const uint64_t scalar_reloaded = ld(scalar_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the faulting vsegment load returns, MemBlock segment preemption state is cleared enough for scalar load/store/load to make progress",
        scalar_observed == scalar_seed &&
        scalar_reloaded == scalar_update
    );

    goto_priv(PRIV_HS);
    ai_memblock_vseg_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_vseg_vlseg2e8_to_v8((const uint8_t *)fault_src_vaddr);
    ai_memblock_vseg_vsseg2e8_from_v8((uint8_t *)dst_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after repairing the second page, the same vsegment load/store path completes normally instead of inheriting stale fault state",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired vsegment path stores the full cross-page segment payload while scalar recovery data and sentinels remain intact",
        ai_memblock_vseg_bytes_match(dst_paddr, expected,
            AI_MEMBLOCK_VSEG_BYTES) &&
        read8(dst_before_paddr) == dst_before &&
        read8(dst_after_paddr) == dst_after &&
        read64(scalar_paddr) == scalar_update &&
        read64(scalar_guard_paddr) == scalar_guard
    );

    TEST_END();
}
