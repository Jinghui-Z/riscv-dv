#include "../boundary/ai_micro_cross_page_fault_common.h"
#include <stdint.h>

#define AI_ARCH_INDEXED_VLOAD_VL 4U
#define AI_ARCH_INDEXED_VLOAD_LARGE_VL 10U
#define AI_ARCH_INDEXED_VLOAD_LARGE_FAULT_LANE 5U

static inline void ai_arch_indexed_vload_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uintptr_t ai_arch_indexed_vload_page_offset(
    enum test_page base_page, enum test_page target_page, uintptr_t offset)
{
    return hs_page_base(target_page) + offset - hs_page_base(base_page);
}

static inline void ai_arch_indexed_vload_write64s(uintptr_t paddr,
    const uint64_t *words, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        write64(paddr + i * sizeof(uint64_t), words[i]);
    }
}

static inline void ai_arch_indexed_vload_write32s(uintptr_t paddr,
    const uint32_t *words, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        write32(paddr + i * sizeof(uint32_t), words[i]);
    }
}

static inline void ai_arch_indexed_vload_warm_vloxei64(
    const uint64_t *offsets, const uint8_t *base, uint64_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e64, m1, ta, ma\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vloxei64.v v24, (%[base]), v8\n\t"
        "vse64.v v24, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [offsets] "r"(offsets),
          [base] "r"(base),
          [dump] "r"(dump),
          [vl] "r"((unsigned)2U)
        : "t0", "memory");
}

static inline void ai_arch_indexed_vload_seeded_vloxei64(
    const uint64_t *sentinel, const uint64_t *offsets, const uint8_t *base,
    unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v6, (%[sentinel])\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "csrw vstart, %[start]\n\t"
        "vloxei64.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [sentinel] "r"(sentinel),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_INDEXED_VLOAD_VL)
        : "t0", "memory");
}

static inline void ai_arch_indexed_vload_resume_vloxei64(
    const uint64_t *offsets, const uint8_t *base, unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "csrw vstart, %[start]\n\t"
        "vloxei64.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [offsets] "r"(offsets),
          [base] "r"(base),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_INDEXED_VLOAD_VL)
        : "t0", "memory");
}

static inline void ai_arch_indexed_vload_seeded_masked_vloxei32(
    const uint32_t *sentinel, const uint32_t *offsets, const uint8_t *mask,
    const uint8_t *base, unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e32, m4, tu, mu\n\t"
        "vle32.v v16, (%[sentinel])\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vsetvli t0, %[vl], e32, m4, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vloxei32.v v16, (%[base]), v8, v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [sentinel] "r"(sentinel),
          [offsets] "r"(offsets),
          [mask] "r"(mask),
          [base] "r"(base),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_INDEXED_VLOAD_LARGE_VL)
        : "t0", "memory");
}

static inline void ai_arch_indexed_vload_resume_masked_vloxei32(
    const uint32_t *offsets, const uint8_t *mask, const uint8_t *base,
    unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vsetvli t0, %[vl], e32, m4, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vloxei32.v v16, (%[base]), v8, v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [offsets] "r"(offsets),
          [mask] "r"(mask),
          [base] "r"(base),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_ARCH_INDEXED_VLOAD_LARGE_VL)
        : "t0", "memory");
}

static inline void ai_arch_indexed_vload_dump_v6(uint64_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vse64.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_INDEXED_VLOAD_VL)
        : "t0", "memory");
}

static inline void ai_arch_indexed_vload_dump_v16_e32m4(uint32_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m4, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vse32.v v16, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dump] "r"(dump),
          [vl] "r"((unsigned)AI_ARCH_INDEXED_VLOAD_LARGE_VL)
        : "t0", "memory");
}

bool ai_arch_indexed_vload_fault_repair_prefix_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = VSRWX_GRW;
    const enum test_page offsets_page = SWITCH2;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t elem0_offset = 0x200U;
    const uintptr_t elem1_offset = 0x208U;
    const uintptr_t elem3_offset = 0x218U;
    const uintptr_t fault_offset = 0x280U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x180U;
    const uintptr_t sentinel_vaddr = hs_page_base(scratch_page) + 0x200U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0x280U;
    const uintptr_t base_paddr = phys_page_base(base_page);
    const uintptr_t elem0_paddr = base_paddr + elem0_offset;
    const uintptr_t elem1_paddr = base_paddr + elem1_offset;
    const uintptr_t elem3_paddr = base_paddr + elem3_offset;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + fault_offset;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + fault_offset;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x180U;
    const uintptr_t sentinel_paddr = phys_page_base(scratch_page) + 0x200U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0x280U;
    const uintptr_t base_guard_lo_paddr = elem0_paddr - sizeof(uint64_t);
    const uintptr_t base_guard_hi_paddr = elem3_paddr + sizeof(uint64_t);
    const uintptr_t fault_guard_hi_paddr = fault_paddr + sizeof(uint64_t);
    const uint64_t elem0_value = 0x1011223344556677ULL;
    const uint64_t elem1_value = 0x2021324354657687ULL;
    const uint64_t elem3_value = 0x3031425364758697ULL;
    const uint64_t fault_value = 0x40415263748596a7ULL;
    const uint64_t elem0_poison = 0xdead0000000000a0ULL;
    const uint64_t elem1_poison = 0xdead0000000000b1ULL;
    const uint64_t base_guard_lo = 0x5152535455565758ULL;
    const uint64_t base_guard_hi = 0x6162636465666768ULL;
    const uint64_t fault_guard_hi = 0x7172737475767778ULL;
    const uint64_t sentinel[AI_ARCH_INDEXED_VLOAD_VL] = {
        0x0bad000000000000ULL,
        0x0bad000000000001ULL,
        0x0bad000000000002ULL,
        0x0bad000000000003ULL
    };
    const uint64_t offsets[AI_ARCH_INDEXED_VLOAD_VL] = {
        (uint64_t)elem0_offset,
        (uint64_t)elem1_offset,
        (uint64_t)ai_arch_indexed_vload_page_offset(base_page, fault_page,
            fault_offset),
        (uint64_t)elem3_offset
    };
    unsigned fault_vstart = 0U;
    unsigned retry_vstart = 0U;

    ai_arch_indexed_vload_enable_vector_state();

    goto_priv(PRIV_M);
    write64(elem0_paddr, elem0_value);
    write64(elem1_paddr, elem1_value);
    write64(elem3_paddr, elem3_value);
    write64(fault_paddr, fault_value);
    write64(base_guard_lo_paddr, base_guard_lo);
    write64(base_guard_hi_paddr, base_guard_hi);
    write64(fault_guard_hi_paddr, fault_guard_hi);
    ai_arch_indexed_vload_write64s(offsets_paddr, offsets,
        AI_ARCH_INDEXED_VLOAD_VL);
    ai_arch_indexed_vload_write64s(sentinel_paddr, sentinel,
        AI_ARCH_INDEXED_VLOAD_VL);
    for (unsigned i = 0; i < AI_ARCH_INDEXED_VLOAD_VL; ++i) {
        write64(dump_paddr + i * sizeof(uint64_t),
            0xfeed000000000000ULL + i);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(fault_page, PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("indexed vload precise-fault setup maps lane2 to an invalid HS leaf while lanes0/1/3 stay on the ordered base page",
        base_vaddr + offsets[0] == hs_page_base(base_page) + elem0_offset &&
        base_vaddr + offsets[1] == hs_page_base(base_page) + elem1_offset &&
        base_vaddr + offsets[2] == fault_vaddr &&
        base_vaddr + offsets[3] == hs_page_base(base_page) + elem3_offset
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vload_seeded_vloxei64((const uint64_t *)sentinel_vaddr,
        (const uint64_t *)offsets_vaddr, (const uint8_t *)base_vaddr, 0U);
    fault_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("ordered indexed load reports the lane2 load page fault with precise tval and vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        fault_vstart == 2U,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=%u",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)fault_vaddr,
        fault_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vload_dump_v6((uint64_t *)dump_vaddr);
    TEST_ASSERT("dumping v6 after the precise fault succeeds after clearing vstart",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repair, ordered indexed-load prefix lanes are committed and fault/tail lanes preserve their seed values",
        read64(dump_paddr + 0U * sizeof(uint64_t)) == elem0_value &&
        read64(dump_paddr + 1U * sizeof(uint64_t)) == elem1_value &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == sentinel[2] &&
        read64(dump_paddr + 3U * sizeof(uint64_t)) == sentinel[3] &&
        read64(base_guard_lo_paddr) == base_guard_lo &&
        read64(base_guard_hi_paddr) == base_guard_hi &&
        read64(fault_guard_hi_paddr) == fault_guard_hi
    );

    write64(elem0_paddr, elem0_poison);
    write64(elem1_paddr, elem1_poison);

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vload_resume_vloxei64((const uint64_t *)offsets_vaddr,
        (const uint8_t *)base_vaddr, fault_vstart);
    retry_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("after repairing lane2, retry from vstart=2 completes without replaying poisoned prefix addresses",
        excpt.triggered == false &&
        retry_vstart == 0U,
        "retry_vstart=%u",
        retry_vstart
    );

    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vload_dump_v6((uint64_t *)dump_vaddr);
    TEST_ASSERT("dumping v6 after indexed-load repair is not poisoned by stale exception state",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repair, final vector image keeps pre-vstart prefix values, loads the repaired fault lane and suffix lane, and proves prefix memory was poisoned",
        read64(dump_paddr + 0U * sizeof(uint64_t)) == elem0_value &&
        read64(dump_paddr + 1U * sizeof(uint64_t)) == elem1_value &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == fault_value &&
        read64(dump_paddr + 3U * sizeof(uint64_t)) == elem3_value &&
        read64(elem0_paddr) == elem0_poison &&
        read64(elem1_paddr) == elem1_poison &&
        read64(base_guard_lo_paddr) == base_guard_lo &&
        read64(base_guard_hi_paddr) == base_guard_hi &&
        read64(fault_guard_hi_paddr) == fault_guard_hi
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_indexed_vload_masked_vlmax_exception_owner_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page inactive_page = VSRWX_GR;
    const enum test_page fault_page = VSRWX_GRW;
    const enum test_page offsets_page = SWITCH2;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t base_paddr = phys_page_base(base_page);
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x2a0U;
    const uintptr_t inactive_paddr = phys_page_base(inactive_page) + 0x280U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x2a0U;
    const uintptr_t warm_offsets_vaddr = hs_page_base(offsets_page) + 0x000U;
    const uintptr_t large_offsets_vaddr = hs_page_base(offsets_page) + 0x100U;
    const uintptr_t warm_dump_vaddr = hs_page_base(scratch_page) + 0x000U;
    const uintptr_t sentinel_vaddr = hs_page_base(scratch_page) + 0x100U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0x200U;
    const uintptr_t mask_vaddr = hs_page_base(scratch_page) + 0x300U;
    const uintptr_t warm_offsets_paddr = phys_page_base(offsets_page) + 0x000U;
    const uintptr_t large_offsets_paddr = phys_page_base(offsets_page) + 0x100U;
    const uintptr_t warm_dump_paddr = phys_page_base(scratch_page) + 0x000U;
    const uintptr_t sentinel_paddr = phys_page_base(scratch_page) + 0x100U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0x200U;
    const uintptr_t mask_paddr = phys_page_base(scratch_page) + 0x300U;
    const uint32_t base_offsets[AI_ARCH_INDEXED_VLOAD_LARGE_VL] = {
        0x040U, 0x044U, 0x080U, 0x084U, 0x0c0U,
        0x0c4U, 0x100U, 0x104U, 0x140U, 0x144U
    };
    const uint32_t active_values[AI_ARCH_INDEXED_VLOAD_LARGE_VL] = {
        0x41000000U, 0x41000001U, 0x41000002U, 0x41000003U,
        0x41000004U, 0x41000005U, 0x41000006U, 0x41000007U,
        0x41000008U, 0x41000009U
    };
    const uint32_t sentinel[AI_ARCH_INDEXED_VLOAD_LARGE_VL] = {
        0x9bad0000U, 0x9bad0001U, 0x9bad0002U, 0x9bad0003U,
        0x9bad0004U, 0x9bad0005U, 0x9bad0006U, 0x9bad0007U,
        0x9bad0008U, 0x9bad0009U
    };
    const uint64_t warm_offsets[2] = { 0x180U, 0x188U };
    const uint64_t warm_values[2] = {
        0x1122334455667788ULL, 0x99aabbccddeeff00ULL
    };
    uint32_t offsets[AI_ARCH_INDEXED_VLOAD_LARGE_VL];
    unsigned fault_vstart = 0U;
    unsigned retry_vstart = 0U;

    ai_arch_indexed_vload_enable_vector_state();

    for (unsigned i = 0; i < AI_ARCH_INDEXED_VLOAD_LARGE_VL; ++i) {
        offsets[i] = base_offsets[i];
    }
    offsets[1] = (uint32_t)ai_arch_indexed_vload_page_offset(base_page,
        inactive_page, 0x280U);
    offsets[3] = (uint32_t)ai_arch_indexed_vload_page_offset(base_page,
        inactive_page, 0x288U);
    offsets[AI_ARCH_INDEXED_VLOAD_LARGE_FAULT_LANE] =
        (uint32_t)ai_arch_indexed_vload_page_offset(base_page, fault_page,
            0x2a0U);

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_ARCH_INDEXED_VLOAD_LARGE_VL; ++i) {
        write32(base_paddr + base_offsets[i], active_values[i]);
        write32(dump_paddr + i * sizeof(uint32_t), 0xfeed1000U + i);
    }
    write32(inactive_paddr, 0xdead0101U);
    write32(inactive_paddr + 8U, 0xdead0303U);
    write32(fault_paddr, active_values[AI_ARCH_INDEXED_VLOAD_LARGE_FAULT_LANE]);
    write64(base_paddr + warm_offsets[0], warm_values[0]);
    write64(base_paddr + warm_offsets[1], warm_values[1]);
    ai_arch_indexed_vload_write64s(warm_offsets_paddr, warm_offsets, 2U);
    ai_arch_indexed_vload_write32s(large_offsets_paddr, offsets,
        AI_ARCH_INDEXED_VLOAD_LARGE_VL);
    ai_arch_indexed_vload_write32s(sentinel_paddr, sentinel,
        AI_ARCH_INDEXED_VLOAD_LARGE_VL);
    write8(mask_paddr + 0U, 0xa5U);
    write8(mask_paddr + 1U, 0x00U);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(inactive_page, PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(fault_page, PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vload_warm_vloxei64((const uint64_t *)warm_offsets_vaddr,
        (const uint8_t *)base_vaddr, (uint64_t *)warm_dump_vaddr);
    TEST_ASSERT("warm ordered indexed load with a different e64,m1 vlmax completes before the masked e32,m4 fault episode reuses merge resources",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL,
        "vstart=%lu", CSRR(CSR_VSTART)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("warm ordered indexed load writes the expected two e64 payloads before the later vlmax-switch fault test",
        read64(warm_dump_paddr + 0U * sizeof(uint64_t)) == warm_values[0] &&
        read64(warm_dump_paddr + 1U * sizeof(uint64_t)) == warm_values[1]
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vload_seeded_masked_vloxei32(
        (const uint32_t *)sentinel_vaddr, (const uint32_t *)large_offsets_vaddr,
        (const uint8_t *)mask_vaddr, (const uint8_t *)base_vaddr, 0U);
    fault_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("masked ordered indexed load ignores invalid inactive lanes and reports the active lane5 fault without stale vlmax wrapping vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        fault_vstart == AI_ARCH_INDEXED_VLOAD_LARGE_FAULT_LANE,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=%u",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)fault_vaddr,
        fault_vstart
    );

    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vload_dump_v16_e32m4((uint32_t *)dump_vaddr);
    TEST_ASSERT("dumping v16 after the masked ordered indexed fault succeeds after clearing vstart",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repair only active prefix lanes retire; invalid inactive lanes and active fault/suffix lanes keep sentinel values",
        read32(dump_paddr + 0U * sizeof(uint32_t)) == active_values[0] &&
        read32(dump_paddr + 1U * sizeof(uint32_t)) == sentinel[1] &&
        read32(dump_paddr + 2U * sizeof(uint32_t)) == active_values[2] &&
        read32(dump_paddr + 3U * sizeof(uint32_t)) == sentinel[3] &&
        read32(dump_paddr + 4U * sizeof(uint32_t)) == sentinel[4] &&
        read32(dump_paddr + 5U * sizeof(uint32_t)) == sentinel[5] &&
        read32(dump_paddr + 7U * sizeof(uint32_t)) == sentinel[7]
    );

    write32(base_paddr + base_offsets[0], 0xc0010000U);
    write32(base_paddr + base_offsets[2], 0xc0010002U);
    write32(base_paddr + base_offsets[7], active_values[7]);

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vload_resume_masked_vloxei32(
        (const uint32_t *)large_offsets_vaddr, (const uint8_t *)mask_vaddr,
        (const uint8_t *)base_vaddr, fault_vstart);
    retry_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("after repairing the active lane5 leaf, retry resumes from lane5 rather than a stale-vlmax-wrapped low lane",
        excpt.triggered == false &&
        retry_vstart == 0U,
        "retry_vstart=%u", retry_vstart
    );

    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vload_dump_v16_e32m4((uint32_t *)dump_vaddr);
    TEST_ASSERT("dumping v16 after masked indexed repair is not poisoned by stale inactive-lane metadata",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repair the final vector image preserves old active prefix, ignores inactive invalid lanes, and loads the repaired active lane plus suffix",
        read32(dump_paddr + 0U * sizeof(uint32_t)) == active_values[0] &&
        read32(dump_paddr + 1U * sizeof(uint32_t)) == sentinel[1] &&
        read32(dump_paddr + 2U * sizeof(uint32_t)) == active_values[2] &&
        read32(dump_paddr + 3U * sizeof(uint32_t)) == sentinel[3] &&
        read32(dump_paddr + 4U * sizeof(uint32_t)) == sentinel[4] &&
        read32(dump_paddr + 5U * sizeof(uint32_t)) ==
            active_values[AI_ARCH_INDEXED_VLOAD_LARGE_FAULT_LANE] &&
        read32(dump_paddr + 6U * sizeof(uint32_t)) == sentinel[6] &&
        read32(dump_paddr + 7U * sizeof(uint32_t)) == active_values[7] &&
        read32(base_paddr + base_offsets[0]) == 0xc0010000U &&
        read32(base_paddr + base_offsets[2]) == 0xc0010002U
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}
