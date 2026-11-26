#include "../boundary/ai_micro_cross_page_fault_common.h"
#include <instructions.h>
#include <stdint.h>

#define AI_AP69_VS_OFF 9U
#define AI_AP69_VS_MASK (3ULL << AI_AP69_VS_OFF)
#define AI_AP69_VS_DIRTY (3ULL << AI_AP69_VS_OFF)
#define AI_AP69_MATRIX_BYTES 96U
#define AI_AP69_INDEX_VL 4U

static inline void ai_ap69_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, AI_AP69_VS_MASK);
    CSRS(CSR_MSTATUS, AI_AP69_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint32_t ai_ap69_le32_from_bytes(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0]) |
        ((uint32_t)bytes[1] << 8U) |
        ((uint32_t)bytes[2] << 16U) |
        ((uint32_t)bytes[3] << 24U);
}

static inline uint32_t ai_ap69_le32_from_paddr(uintptr_t paddr)
{
    uint8_t bytes[sizeof(uint32_t)];

    for (unsigned i = 0; i < sizeof(uint32_t); ++i) {
        bytes[i] = read8(paddr + i);
    }

    return ai_ap69_le32_from_bytes(bytes);
}

static inline uint32_t ai_ap69_le32_from_two_pages(enum test_page low_page,
    enum test_page high_page, uintptr_t offset)
{
    uint8_t bytes[sizeof(uint32_t)];

    for (unsigned i = 0; i < sizeof(uint32_t); ++i) {
        const uintptr_t byte_offset = offset + i;

        if (byte_offset < PAGE_SIZE) {
            bytes[i] = read8(phys_page_base(low_page) + byte_offset);
        } else {
            bytes[i] = read8(phys_page_base(high_page) +
                (byte_offset - PAGE_SIZE));
        }
    }

    return ai_ap69_le32_from_bytes(bytes);
}

static inline bool ai_ap69_words_match(uintptr_t paddr,
    const uint32_t *expected, unsigned words)
{
    for (unsigned i = 0; i < words; ++i) {
        if (read32(paddr + i * sizeof(uint32_t)) != expected[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_ap69_fill_bytes(uintptr_t paddr, unsigned len,
    uint8_t base, uint8_t step)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, (uint8_t)(base + (uint8_t)(i * step)));
    }
}

static inline void ai_ap69_indexed_load_dump(uintptr_t base_vaddr,
    uintptr_t offsets_vaddr, uintptr_t dump_vaddr, unsigned vl,
    unsigned ordered)
{
    if (ordered) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
            "vle64.v v16, (%[offsets])\n\t"
            "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
            "vloxei64.v v8, (%[base]), v16\n\t"
            "vse32.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base_vaddr),
              [offsets] "r"(offsets_vaddr),
              [dump] "r"(dump_vaddr),
              [vl] "r"(vl)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
            "vle64.v v16, (%[offsets])\n\t"
            "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
            "vluxei64.v v8, (%[base]), v16\n\t"
            "vse32.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base_vaddr),
              [offsets] "r"(offsets_vaddr),
              [dump] "r"(dump_vaddr),
              [vl] "r"(vl)
            : "t0", "memory");
    }
}

static inline void ai_ap69_unitstride_load_dump(uintptr_t src_vaddr,
    uintptr_t dump_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v8, (%[src])\n\t"
        "vse32.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap69_strided_load_dump(uintptr_t src_vaddr,
    intptr_t stride, uintptr_t dump_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlse32.v v8, (%[src]), %[stride]\n\t"
        "vse32.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr),
          [stride] "r"(stride),
          [dump] "r"(dump_vaddr),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap87_indexed_load_fault(uintptr_t base_vaddr,
    uintptr_t offsets_vaddr, unsigned vl, unsigned ordered)
{
    if (ordered) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
            "vle64.v v16, (%[offsets])\n\t"
            "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
            "vloxei64.v v8, (%[base]), v16\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base_vaddr),
              [offsets] "r"(offsets_vaddr),
              [vl] "r"(vl)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
            "vle64.v v16, (%[offsets])\n\t"
            "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
            "vluxei64.v v8, (%[base]), v16\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base_vaddr),
              [offsets] "r"(offsets_vaddr),
              [vl] "r"(vl)
            : "t0", "memory");
    }
}

bool ai_micro_ap69_ordered_indexed_crosspage_mab_writeback_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page offsets_page = SCRATCHPAD;
    const enum test_page dump_page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(low_page);
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x100U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x200U;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE - 32U;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x100U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x200U;
    const uint64_t offsets[AI_AP69_INDEX_VL] = {
        PAGE_SIZE - 7U,
        PAGE_SIZE - 3U,
        PAGE_SIZE + 1U,
        PAGE_SIZE + 5U
    };
    uint32_t expected[AI_AP69_INDEX_VL];
    uint8_t low_round_base = 0x20U;
    uint8_t high_round_base = 0x80U;
    bool ok = true;

    ai_ap69_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    for (unsigned i = 0; i < AI_AP69_INDEX_VL; ++i) {
        write64(offsets_paddr + i * sizeof(uint64_t), offsets[i]);
    }

    for (unsigned round = 0; round < 2U; ++round) {
        goto_priv(PRIV_M);
        ai_ap69_fill_bytes(low_tail_paddr, 32U,
            (uint8_t)(low_round_base + round * 0x30U), 3U);
        ai_ap69_fill_bytes(high_head_paddr, 32U,
            (uint8_t)(high_round_base + round * 0x20U), 5U);
        ai_ap69_fill_bytes(dump_paddr, AI_AP69_INDEX_VL * sizeof(uint32_t),
            (uint8_t)(0xe0U + round), 1U);

        for (unsigned i = 0; i < AI_AP69_INDEX_VL; ++i) {
            expected[i] = ai_ap69_le32_from_two_pages(low_page, high_page,
                offsets[i]);
        }

        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_ap69_indexed_load_dump(base_vaddr, offsets_vaddr, dump_vaddr,
            AI_AP69_INDEX_VL, 1U);
        ok = ok && (excpt.triggered == false) &&
            (CSRR(CSR_VL) == AI_AP69_INDEX_VL);

        goto_priv(PRIV_M);
        ok = ok && ai_ap69_words_match(dump_paddr, expected,
            AI_AP69_INDEX_VL);
    }

    TEST_ASSERT("AP-69 ordered indexed non-aligned vector load completes twice across low/high split windows and dumps only the final combined element data",
        ok
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap69_mab_load_mode_matrix_final_writeback_manual_corner()
{
    TEST_START();

    const enum test_page data_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t data_vaddr = hs_page_base(data_page) + 0x300U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x100U;
    const uintptr_t unit_dump_vaddr = hs_page_base(dump_page) + 0x100U;
    const uintptr_t stride_dump_vaddr = hs_page_base(dump_page) + 0x140U;
    const uintptr_t index_dump_vaddr = hs_page_base(dump_page) + 0x180U;
    const uintptr_t data_paddr = phys_page_base(data_page) + 0x300U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x100U;
    const uintptr_t unit_dump_paddr = phys_page_base(dump_page) + 0x100U;
    const uintptr_t stride_dump_paddr = phys_page_base(dump_page) + 0x140U;
    const uintptr_t index_dump_paddr = phys_page_base(dump_page) + 0x180U;
    const uint64_t index_offsets[AI_AP69_INDEX_VL] = {
        3ULL, 11ULL, 19ULL, 27ULL
    };
    uint32_t unit_expected[AI_AP69_INDEX_VL];
    uint32_t stride_expected[AI_AP69_INDEX_VL];
    uint32_t index_expected[AI_AP69_INDEX_VL];

    ai_ap69_enable_vector_state();
    goto_priv(PRIV_M);
    ai_ap69_fill_bytes(data_paddr, AI_AP69_MATRIX_BYTES, 0x31U, 7U);
    ai_ap69_fill_bytes(unit_dump_paddr, 64U, 0xd0U, 1U);
    ai_ap69_fill_bytes(stride_dump_paddr, 64U, 0xe0U, 1U);
    ai_ap69_fill_bytes(index_dump_paddr, 64U, 0xf0U, 1U);
    for (unsigned i = 0; i < AI_AP69_INDEX_VL; ++i) {
        write64(offsets_paddr + i * sizeof(uint64_t), index_offsets[i]);
        unit_expected[i] = ai_ap69_le32_from_paddr(data_paddr + 1U +
            i * sizeof(uint32_t));
        stride_expected[i] = ai_ap69_le32_from_paddr(data_paddr + 2U +
            i * 5U);
        index_expected[i] = ai_ap69_le32_from_paddr(data_paddr +
            index_offsets[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(data_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap69_unitstride_load_dump(data_vaddr + 1U, unit_dump_vaddr,
        AI_AP69_INDEX_VL);
    TEST_ASSERT("AP-69 unit-stride same-page non-aligned vector load completes without a partial split-flow pipeline writeback symptom",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP69_INDEX_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap69_strided_load_dump(data_vaddr + 2U, 5, stride_dump_vaddr,
        AI_AP69_INDEX_VL);
    TEST_ASSERT("AP-69 strided non-aligned vector load also reaches final architectural writeback instead of an early split-flow completion",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP69_INDEX_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap69_indexed_load_dump(data_vaddr, offsets_vaddr, index_dump_vaddr,
        AI_AP69_INDEX_VL, 0U);
    TEST_ASSERT("AP-69 unordered indexed non-aligned vector load completes with the same final-writeback ownership rule",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP69_INDEX_VL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-69 mode matrix returns exact unit-stride, strided, and indexed e32 payloads from the non-aligned addresses",
        ai_ap69_words_match(unit_dump_paddr, unit_expected,
            AI_AP69_INDEX_VL) &&
        ai_ap69_words_match(stride_dump_paddr, stride_expected,
            AI_AP69_INDEX_VL) &&
        ai_ap69_words_match(index_dump_paddr, index_expected,
            AI_AP69_INDEX_VL)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap69_aligned_vector_load_pipeline_contrast_manual_corner()
{
    TEST_START();

    const enum test_page data_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t data_vaddr = hs_page_base(data_page) + 0x500U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x200U;
    const uintptr_t unit_dump_vaddr = hs_page_base(dump_page) + 0x240U;
    const uintptr_t index_dump_vaddr = hs_page_base(dump_page) + 0x280U;
    const uintptr_t data_paddr = phys_page_base(data_page) + 0x500U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x200U;
    const uintptr_t unit_dump_paddr = phys_page_base(dump_page) + 0x240U;
    const uintptr_t index_dump_paddr = phys_page_base(dump_page) + 0x280U;
    const uint64_t aligned_offsets[AI_AP69_INDEX_VL] = {
        12ULL, 0ULL, 8ULL, 4ULL
    };
    uint32_t unit_expected[AI_AP69_INDEX_VL];
    uint32_t index_expected[AI_AP69_INDEX_VL];

    ai_ap69_enable_vector_state();
    goto_priv(PRIV_M);
    ai_ap69_fill_bytes(data_paddr, AI_AP69_MATRIX_BYTES, 0x42U, 9U);
    ai_ap69_fill_bytes(unit_dump_paddr, 64U, 0xa0U, 1U);
    ai_ap69_fill_bytes(index_dump_paddr, 64U, 0xb0U, 1U);
    for (unsigned i = 0; i < AI_AP69_INDEX_VL; ++i) {
        write64(offsets_paddr + i * sizeof(uint64_t), aligned_offsets[i]);
        unit_expected[i] = ai_ap69_le32_from_paddr(data_paddr +
            i * sizeof(uint32_t));
        index_expected[i] = ai_ap69_le32_from_paddr(data_paddr +
            aligned_offsets[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(data_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap69_unitstride_load_dump(data_vaddr, unit_dump_vaddr,
        AI_AP69_INDEX_VL);
    TEST_ASSERT("AP-69 aligned unit-stride vector load remains a normal pipeline-writeback contrast case",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP69_INDEX_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap69_indexed_load_dump(data_vaddr, offsets_vaddr, index_dump_vaddr,
        AI_AP69_INDEX_VL, 1U);
    TEST_ASSERT("AP-69 aligned ordered indexed vector load also still completes through the ordinary vector-load path",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP69_INDEX_VL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-69 aligned pipeline contrast preserves both aligned unit-stride and aligned indexed payloads",
        ai_ap69_words_match(unit_dump_paddr, unit_expected,
            AI_AP69_INDEX_VL) &&
        ai_ap69_words_match(index_dump_paddr, index_expected,
            AI_AP69_INDEX_VL)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap87_mab_split_fault_elem1_tval_vstart_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = ai_micro_cross_page_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x340U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x340U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x380U;
    const uintptr_t scratch_paddr = phys_page_base(scratch_page) + 0x380U;
    const uint8_t src_bytes[16] = {
        0x61, 0x62, 0x63, 0x64,
        0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x6b, 0x6c,
        0x6d, 0x6e, 0x6f, 0x70
    };
    const uint8_t old_bytes[16] = {
        0xc1, 0xc2, 0xc3, 0xc4,
        0xc5, 0xc6, 0xc7, 0xc8,
        0xc9, 0xca, 0xcb, 0xcc,
        0xcd, 0xce, 0xcf, 0xd0
    };
    uint8_t expected[16];
    uint64_t observed_vstart = 0ULL;

    for (unsigned i = 0; i < sizeof(expected); ++i) {
        expected[i] = old_bytes[i];
    }
    for (unsigned i = 0; i < sizeof(uint32_t); ++i) {
        expected[i] = src_bytes[i];
    }

    ai_ap69_enable_vector_state();
    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_paddr, src_bytes, 7U);
    ai_micro_write_bytes(high_paddr, src_bytes + 7U, 9U);
    ai_micro_write_bytes(old_paddr, old_bytes, sizeof(old_bytes));
    ai_micro_write_bytes(scratch_paddr, old_bytes, sizeof(old_bytes));

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("AP-87 setup preloads v6 before the element1 MAB split load fault",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-87 element1 non-aligned load faults through MAB split with vstart=1 and tval at the high split address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, high_fault_vaddr, observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 4);
    TEST_ASSERT("AP-87 can dump v6 after clearing vstart so the prefix/faulting-element footprint is visible",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-87 element1 MAB split fault updates only the prefix element0 and leaves element1+ at the old destination image",
        ai_micro_bytes_match(scratch_paddr, expected, sizeof(expected))
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap87_exception_buffer_mab_vs_normal_fault_priority_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page normal_fault_page = SWITCH1;
    const enum test_page split_low_page = SWITCH2;
    const enum test_page split_high_page = VSRWXPbmt0_GURWXPbmt0;
    const enum test_page offsets_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t elem0_vaddr = hs_page_base(base_page) + 0x100U;
    const uintptr_t normal_fault_vaddr =
        hs_page_base(normal_fault_page) + 0x180U;
    const uintptr_t split_fault_vaddr =
        hs_page_base(split_low_page) + PAGE_SIZE - 3U;
    const uintptr_t split_high_vaddr = hs_page_base(split_high_page);
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x200U;
    const uintptr_t elem0_paddr = phys_page_base(base_page) + 0x100U;
    const uintptr_t normal_fault_paddr =
        phys_page_base(normal_fault_page) + 0x180U;
    const uintptr_t split_low_paddr =
        phys_page_base(split_low_page) + PAGE_SIZE - 3U;
    const uintptr_t split_high_paddr = phys_page_base(split_high_page);
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x200U;
    const uint64_t offsets[3] = {
        elem0_vaddr - base_vaddr,
        normal_fault_vaddr - base_vaddr,
        split_fault_vaddr - base_vaddr
    };
    uint64_t observed_vstart = 0ULL;

    ai_ap69_enable_vector_state();
    goto_priv(PRIV_M);
    write32(elem0_paddr, 0x13572468U);
    write32(normal_fault_paddr, 0x24681357U);
    ai_micro_write_bytes(split_low_paddr,
        (const uint8_t []){ 0x91, 0x92, 0x93 }, 3U);
    ai_micro_write_bytes(split_high_paddr,
        (const uint8_t []){ 0x94, 0x95, 0x96, 0x97 }, 4U);
    for (unsigned i = 0; i < 3U; ++i) {
        write64(offsets_paddr + i * sizeof(uint64_t), offsets[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(normal_fault_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(split_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(split_high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap87_indexed_load_fault(base_vaddr, offsets_vaddr, 3U, 0U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-87 when a normal element1 fault competes with a younger MAB split fault, exception selection keeps the lower architectural elemIdx",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == normal_fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, normal_fault_vaddr, observed_vstart
    );

    ai_micro_set_hs_leaf(normal_fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap87_indexed_load_fault(base_vaddr, offsets_vaddr, 3U, 0U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-87 after repairing the normal element, the younger non-zero MAB split fault reports elemIdx/vstart=2 rather than falling back to element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == split_high_vaddr &&
        observed_vstart == 2ULL,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, split_high_vaddr, observed_vstart
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}
