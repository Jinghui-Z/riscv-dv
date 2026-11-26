#include "../boundary/ai_micro_cross_page_fault_common.h"
#include <stdint.h>

#define AI_ARCH_INDEXED_VSTORE_VL 4U
#define AI_ARCH_INDEXED_VSTORE_MARKER_BYTES 8U

static inline void ai_arch_indexed_vstore_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uintptr_t ai_arch_indexed_vstore_page_offset(
    enum test_page base_page, enum test_page target_page, uintptr_t offset)
{
    return hs_page_base(target_page) + offset - hs_page_base(base_page);
}

static inline void ai_arch_indexed_vstore_write_words(uintptr_t paddr,
    const uint32_t *words, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        write32(paddr + i * sizeof(uint32_t), words[i]);
    }
}

static inline bool ai_arch_indexed_vstore_words_match(uintptr_t paddr,
    const uint32_t *words, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        if (read32(paddr + i * sizeof(uint32_t)) != words[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_arch_indexed_vsoxei32_start(const uint32_t *src,
    const uint32_t *offsets, uint8_t *base, unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[src])\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "csrw vstart, %[start]\n\t"
        "vsoxei32.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_ARCH_INDEXED_VSTORE_VL),
          [start] "r"(start)
        : "t0", "memory");
}

static inline void ai_arch_indexed_vse8_marker(const uint8_t *src,
    uint8_t *dst)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v10, (%[src])\n\t"
        "vse8.v v10, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_INDEXED_VSTORE_MARKER_BYTES)
        : "t0", "memory");
}

bool ai_arch_indexed_vstore_lane_fault_repair_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_offset = 0x200U;
    const uintptr_t distinct_offset = 0x240U;
    const uintptr_t fault_offset = 0x280U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x180U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x180U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x300U;
    const uintptr_t marker_src_vaddr = hs_page_base(marker_page) + 0x340U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x180U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x180U;
    const uintptr_t alias_paddr = phys_page_base(base_page) + alias_offset;
    const uintptr_t distinct_paddr = phys_page_base(base_page) + distinct_offset;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + fault_offset;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + fault_offset;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x300U;
    const uintptr_t marker_src_paddr = phys_page_base(marker_page) + 0x340U;
    const uintptr_t alias_guard_lo = alias_paddr - sizeof(uint32_t);
    const uintptr_t alias_guard_hi = alias_paddr + sizeof(uint32_t);
    const uintptr_t fault_guard_hi = fault_paddr + sizeof(uint32_t);
    const uintptr_t marker_guard_lo = marker_paddr - 1U;
    const uintptr_t marker_guard_hi =
        marker_paddr + AI_ARCH_INDEXED_VSTORE_MARKER_BYTES;
    const uint32_t src_words[AI_ARCH_INDEXED_VSTORE_VL] = {
        0x210000a0U, 0x210000b1U, 0x210000c2U, 0x210000d3U
    };
    const uint32_t offsets[AI_ARCH_INDEXED_VSTORE_VL] = {
        (uint32_t)alias_offset,
        (uint32_t)distinct_offset,
        (uint32_t)alias_offset,
        (uint32_t)ai_arch_indexed_vstore_page_offset(base_page,
            fault_page, fault_offset)
    };
    const uint8_t marker_seed[AI_ARCH_INDEXED_VSTORE_MARKER_BYTES] = {
        0x90U, 0x91U, 0x92U, 0x93U, 0x94U, 0x95U, 0x96U, 0x97U
    };
    const uint8_t marker_new[AI_ARCH_INDEXED_VSTORE_MARKER_BYTES] = {
        0x40U, 0x51U, 0x62U, 0x73U, 0x84U, 0x95U, 0xa6U, 0xb7U
    };
    const uint32_t alias_old = 0x11111111U;
    const uint32_t distinct_old = 0x22222222U;
    const uint32_t fault_old = 0x33333333U;
    const uint32_t alias_guard_lo_old = 0x44444444U;
    const uint32_t alias_guard_hi_old = 0x55555555U;
    const uint32_t fault_guard_hi_old = 0x66666666U;
    const uint8_t marker_guard_lo_old = 0x3cU;
    const uint8_t marker_guard_hi_old = 0xc3U;
    unsigned fault_vstart = 0U;
    unsigned retry_vstart = 0U;

    ai_arch_indexed_vstore_enable_vector_state();

    goto_priv(PRIV_M);
    ai_arch_indexed_vstore_write_words(src_paddr, src_words,
        AI_ARCH_INDEXED_VSTORE_VL);
    ai_arch_indexed_vstore_write_words(offsets_paddr, offsets,
        AI_ARCH_INDEXED_VSTORE_VL);
    write32(alias_paddr, alias_old);
    write32(distinct_paddr, distinct_old);
    write32(fault_paddr, fault_old);
    write32(alias_guard_lo, alias_guard_lo_old);
    write32(alias_guard_hi, alias_guard_hi_old);
    write32(fault_guard_hi, fault_guard_hi_old);
    ai_micro_write_bytes(marker_paddr, marker_seed, sizeof(marker_seed));
    ai_micro_write_bytes(marker_src_paddr, marker_new, sizeof(marker_new));
    write8(marker_guard_lo, marker_guard_lo_old);
    write8(marker_guard_hi, marker_guard_hi_old);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(marker_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("indexed vstore fault probe uses ordered vsoxei32 with lane0/lane2 duplicate offset and lane3 targeting a no-write HS leaf",
        offsets[0] == offsets[2] &&
        base_vaddr + offsets[3] == fault_vaddr
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vsoxei32_start((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (uint8_t *)base_vaddr, 0U);
    fault_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("ordered indexed store reports the lane3 store page fault with vstart bound to the faulting index lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        fault_vstart == 3U,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=%u",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)fault_vaddr,
        fault_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repair, ordered duplicate lane2 overwrites lane0, lane1 is visible, and the lane3 fault target plus guards remain old",
        read32(alias_paddr) == src_words[2] &&
        read32(distinct_paddr) == src_words[1] &&
        read32(fault_paddr) == fault_old &&
        read32(alias_guard_lo) == alias_guard_lo_old &&
        read32(alias_guard_hi) == alias_guard_hi_old &&
        read32(fault_guard_hi) == fault_guard_hi_old &&
        ai_micro_bytes_match(marker_paddr, marker_seed,
            sizeof(marker_seed)) &&
        read8(marker_guard_lo) == marker_guard_lo_old &&
        read8(marker_guard_hi) == marker_guard_hi_old
    );

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vsoxei32_start((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (uint8_t *)base_vaddr,
        fault_vstart);
    retry_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("after repairing the lane3 leaf, retrying ordered indexed store from vstart=3 completes without replaying older duplicate lanes",
        excpt.triggered == false &&
        retry_vstart == 0U,
        "retry_vstart=%u",
        retry_vstart
    );

    TEST_SETUP_EXCEPT();
    ai_arch_indexed_vse8_marker((const uint8_t *)marker_src_vaddr,
        (uint8_t *)marker_vaddr);
    TEST_ASSERT("a normal vector store immediately after indexed-store repair is not poisoned by stale indexed exception state",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repair, the ordered duplicate final value, distinct lane, repaired fault lane, marker store, and guards all match the architectural oracle",
        read32(alias_paddr) == src_words[2] &&
        read32(distinct_paddr) == src_words[1] &&
        read32(fault_paddr) == src_words[3] &&
        read32(alias_guard_lo) == alias_guard_lo_old &&
        read32(alias_guard_hi) == alias_guard_hi_old &&
        read32(fault_guard_hi) == fault_guard_hi_old &&
        ai_micro_bytes_match(marker_paddr, marker_new,
            sizeof(marker_new)) &&
        read8(marker_guard_lo) == marker_guard_lo_old &&
        read8(marker_guard_hi) == marker_guard_hi_old
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}
