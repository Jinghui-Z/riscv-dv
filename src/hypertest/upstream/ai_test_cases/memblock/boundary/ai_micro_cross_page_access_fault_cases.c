#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_PMP_TOR_R 0x09ULL
#define AI_MICRO_PMP_TOR_RW 0x0bULL
#define AI_MICRO_PMP_TOR_NO_R 0x0eULL
#define AI_MICRO_PMP_TOR_NO_W 0x0dULL
#define AI_MICRO_PMP_NAPOT_RWX 0x1fULL
#define AI_MICRO_PMP_NAPOT_R 0x19ULL
#define AI_MICRO_PMP_NAPOT_X 0x1cULL

static inline uintptr_t ai_micro_cross_page_access_addr(enum test_page low_page, uintptr_t low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - low_bytes;
}

static inline void ai_micro_configure_pmp_window(uintptr_t deny_start, uintptr_t deny_end,
    uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_PMP_TOR_RWX | (deny_cfg << 8) | (AI_MICRO_PMP_TOR_RWX << 16));
}

static inline void ai_micro_configure_pmp_two_page_allow(enum test_page low_page,
    enum test_page high_page, uint64_t low_cfg, uint64_t high_cfg)
{
    uintptr_t low_start = phys_page_base(low_page);
    uintptr_t high_start = phys_page_base(high_page);
    uintptr_t high_end = high_start + PAGE_SIZE;

    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, low_start >> 2);
    CSRW(CSR_PMPADDR1, high_start >> 2);
    CSRW(CSR_PMPADDR2, high_end >> 2);
    CSRW(CSR_PMPADDR3, (uint64_t)-1);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_PMP_TOR_RWX |
        (low_cfg << 8) |
        (high_cfg << 16) |
        (AI_MICRO_PMP_TOR_RWX << 24));
}

static inline void ai_micro_configure_pmp_four_page_allow(enum test_page page0,
    enum test_page page1, enum test_page page2, enum test_page page3,
    uint64_t cfg0, uint64_t cfg1, uint64_t cfg2, uint64_t cfg3)
{
    uintptr_t page0_start = phys_page_base(page0);
    uintptr_t page1_start = phys_page_base(page1);
    uintptr_t page2_start = phys_page_base(page2);
    uintptr_t page3_start = phys_page_base(page3);
    uintptr_t page3_end = page3_start + PAGE_SIZE;

    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, page0_start >> 2);
    CSRW(CSR_PMPADDR1, page1_start >> 2);
    CSRW(CSR_PMPADDR2, page2_start >> 2);
    CSRW(CSR_PMPADDR3, page3_start >> 2);
    CSRW(CSR_PMPADDR4, page3_end >> 2);
    CSRW(CSR_PMPADDR5, (uint64_t)-1);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_PMP_TOR_RWX |
        (cfg0 << 8) |
        (cfg1 << 16) |
        (cfg2 << 24) |
        (cfg3 << 32) |
        (AI_MICRO_PMP_TOR_RWX << 40));
}

static inline uintptr_t ai_micro_pmp_napot_addr(uintptr_t base, uintptr_t size)
{
    return (base >> 2) | ((size - 1U) >> 3);
}

static inline void ai_micro_configure_pmp_overlap_low_priority_window(enum test_page low_page,
    uint64_t low_page_cfg)
{
    uintptr_t low_start = phys_page_base(low_page);

    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, ai_micro_pmp_napot_addr(low_start, PAGE_SIZE));
    CSRW(CSR_PMPADDR1, ai_micro_pmp_napot_addr(low_start, 2U * PAGE_SIZE));
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0,
        low_page_cfg |
        (AI_MICRO_PMP_NAPOT_RWX << 8) |
        (AI_MICRO_PMP_NAPOT_RWX << 16));
}

static inline void ai_micro_cross_page_access_write_bytes(uintptr_t addr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_cross_page_access_bytes_match(uintptr_t addr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline uint64_t ai_micro_cross_page_access_pack_u64(const uint8_t *bytes)
{
    uint64_t value = 0;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }

    return value;
}

static inline bool ai_micro_cross_page_access_store_u64_matches(uintptr_t low_phys,
    uintptr_t high_phys, uint64_t value)
{
    for (unsigned i = 0; i < 4U; ++i) {
        if (read8(low_phys + i) != ((value >> (8U * i)) & 0xffU)) {
            return false;
        }
        if (read8(high_phys + i) != ((value >> (8U * (i + 4U))) & 0xffU)) {
            return false;
        }
    }

    return true;
}

static inline void ai_micro_cross_page_access_vlse64_to_v6(const uint64_t *base,
    intptr_t stride, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vlse64.v v6, (%[base]), %[stride]\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [stride] "r"(stride),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_cross_page_access_vsse64_from_v6(uint64_t *base,
    intptr_t stride, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vsse64.v v6, (%[base]), %[stride]\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [stride] "r"(stride),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_cross_page_access_vle64_to_v6_strided_dump(
    const uint64_t *base, intptr_t stride, uint64_t *dump, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vlse64.v v6, (%[base]), %[stride]\n\t"
        "vse64.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [stride] "r"(stride),
          [dump] "r"(dump),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_cross_page_access_vle64_vsse64_window(
    const uint64_t *src, uint64_t *dst, intptr_t stride, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[src])\n\t"
        "vsse64.v v6, (%[dst]), %[stride]\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dst] "r"(dst),
          [stride] "r"(stride),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline uint64_t ai_micro_cross_page_access_expected_low_after_4b_store(
    uint64_t original_low, uint64_t store_value)
{
    return (original_low & 0xffffffffULL) |
        ((store_value & 0xffffffffULL) << 32);
}

static inline pte_t *ai_micro_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_set_hs_leaf(enum test_page page, uint64_t flags)
{
    const unsigned caller_priv = curr_priv;

    goto_priv(PRIV_M);
    *ai_micro_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
    goto_priv(caller_priv);
}

bool ai_micro_hs_load_cross_page_two_pmp_entries_allow_no_false_access_fault_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRW;
    const enum test_page high_page = VSRWX_GRX;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint8_t payload[8] = {
        0x81, 0x82, 0x83, 0x84,
        0xa1, 0xa2, 0xa3, 0xa4
    };
    const uint64_t expected = ai_micro_cross_page_access_pack_u64(payload);
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_access_write_bytes(low_phys, payload, 4U);
    ai_micro_cross_page_access_write_bytes(high_phys, payload + 4U, 4U);
    ai_micro_configure_pmp_two_page_allow(low_page, high_page,
        AI_MICRO_PMP_TOR_R, AI_MICRO_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the AP-35 two-PMP-entry load probe uses adjacent physical pages split exactly at the misaligned load boundary",
        phys_page_base(high_page) == phys_page_base(low_page) + PAGE_SIZE
    );

    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT("an HS 8-byte load crossing a page boundary covered by two separate PMP-allowed TOR entries must not be rejected by a full-range PMP check",
        excpt.triggered == false &&
        observed == expected,
        "observed=0x%llx expected=0x%llx",
        (unsigned long long)observed, (unsigned long long)expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the two-entry PMP-allowed cross-page load leaves both source halves unchanged",
        ai_micro_cross_page_access_bytes_match(low_phys, payload, 4U) &&
        ai_micro_cross_page_access_bytes_match(high_phys, payload + 4U, 4U)
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    TEST_END();
}

bool ai_micro_hs_store_cross_page_two_pmp_entries_allow_updates_both_entries_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_guard_phys = low_phys - 1U;
    const uintptr_t high_guard_phys = high_phys + 4U;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint8_t low_guard = 0x5aU;
    const uint8_t high_guard = 0xa5U;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < 4U; ++i) {
        write8(low_phys + i, 0x10U + i);
        write8(high_phys + i, 0x20U + i);
    }
    write8(low_guard_phys, low_guard);
    write8(high_guard_phys, high_guard);
    ai_micro_configure_pmp_two_page_allow(low_page, high_page,
        AI_MICRO_PMP_TOR_RW, AI_MICRO_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the AP-35 two-PMP-entry store probe uses adjacent physical pages split exactly at the misaligned store boundary",
        phys_page_base(high_page) == phys_page_base(low_page) + PAGE_SIZE
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("an HS 8-byte store crossing a page boundary covered by two separate PMP-allowed TOR entries must not be rejected by a full-range PMP check",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the two-entry PMP-allowed cross-page store updates exactly the low-page tail bytes, the high-page head bytes, and no adjacent guard bytes",
        read8(low_phys + 0U) == ((store_value >> 0) & 0xffU) &&
        read8(low_phys + 1U) == ((store_value >> 8) & 0xffU) &&
        read8(low_phys + 2U) == ((store_value >> 16) & 0xffU) &&
        read8(low_phys + 3U) == ((store_value >> 24) & 0xffU) &&
        read8(high_phys + 0U) == ((store_value >> 32) & 0xffU) &&
        read8(high_phys + 1U) == ((store_value >> 40) & 0xffU) &&
        read8(high_phys + 2U) == ((store_value >> 48) & 0xffU) &&
        read8(high_phys + 3U) == ((store_value >> 56) & 0xffU) &&
        read8(low_guard_phys) == low_guard &&
        read8(high_guard_phys) == high_guard
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    TEST_END();
}

bool ai_micro_hs_vector_load_unitstride_cross_page_two_pmp_entries_allow_no_false_access_fault_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRW;
    const enum test_page high_page = VSRWX_GRX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x380U;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + 0x380U;
    const uintptr_t low_guard_phys = low_phys - 1U;
    const uintptr_t high_guard_phys = high_phys + 4U;
    const uintptr_t scratch_guard_phys = scratch_phys + 8U;
    const uint8_t payload[8] = {
        0x19, 0x2a, 0x3b, 0x4c,
        0x5d, 0x6e, 0x7f, 0x80
    };
    const uint8_t scratch_seed[8] = {
        0xd1, 0xd2, 0xd3, 0xd4,
        0xd5, 0xd6, 0xd7, 0xd8
    };
    const uint8_t low_guard = 0x91U;
    const uint8_t high_guard = 0x92U;
    const uint8_t scratch_guard = 0x93U;
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_access_write_bytes(low_phys, payload, 4U);
    ai_micro_cross_page_access_write_bytes(high_phys, payload + 4U, 4U);
    ai_micro_cross_page_access_write_bytes(scratch_phys, scratch_seed, sizeof(scratch_seed));
    write8(low_guard_phys, low_guard);
    write8(high_guard_phys, high_guard);
    write8(scratch_guard_phys, scratch_guard);
    ai_micro_configure_pmp_two_page_allow(low_page, high_page,
        AI_MICRO_PMP_TOR_R, AI_MICRO_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the AP-35 vector unit-stride load probe uses adjacent physical pages and places one e64 element across two separately allowed PMP TOR entries",
        phys_page_base(high_page) == phys_page_base(low_page) + PAGE_SIZE
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)vaddr, 1);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a single active vle64.v element crossing a page boundary must not be rejected just because its two halves hit different PMP allow entries",
        excpt.triggered == false &&
        observed_vstart == 0ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)scratch_vaddr, 1);
    TEST_ASSERT("dumping the vector destination after the two-PMP-entry cross-page vle64.v completes without a follow-up exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the two-PMP-entry vector load returns the exact little-endian split element into v6",
        ai_micro_cross_page_access_bytes_match(scratch_phys, payload, sizeof(payload))
    );
    TEST_ASSERT("the vector load positive AP-35 probe leaves source halves and sentinels unchanged",
        ai_micro_cross_page_access_bytes_match(low_phys, payload, 4U) &&
        ai_micro_cross_page_access_bytes_match(high_phys, payload + 4U, 4U) &&
        read8(low_guard_phys) == low_guard &&
        read8(high_guard_phys) == high_guard &&
        read8(scratch_guard_phys) == scratch_guard
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_unitstride_cross_page_two_pmp_entries_allow_updates_split_element_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const enum test_page src_page = SWITCH1;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x3c0U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x3c0U;
    const uintptr_t low_guard_phys = low_phys - 1U;
    const uintptr_t high_guard_phys = high_phys + 4U;
    const uint8_t initial[8] = {
        0x31, 0x32, 0x33, 0x34,
        0x35, 0x36, 0x37, 0x38
    };
    const uint8_t src_pattern[8] = {
        0xb8, 0xa7, 0x96, 0x85,
        0x74, 0x63, 0x52, 0x41
    };
    const uint8_t low_guard = 0xa1U;
    const uint8_t high_guard = 0xa2U;
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_access_write_bytes(low_phys, initial, 4U);
    ai_micro_cross_page_access_write_bytes(high_phys, initial + 4U, 4U);
    ai_micro_cross_page_access_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    write8(low_guard_phys, low_guard);
    write8(high_guard_phys, high_guard);
    ai_micro_configure_pmp_two_page_allow(low_page, high_page,
        AI_MICRO_PMP_TOR_RW, AI_MICRO_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the AP-35 vector unit-stride store probe uses adjacent physical pages and places one e64 store element across two separately allowed PMP TOR entries",
        phys_page_base(high_page) == phys_page_base(low_page) + PAGE_SIZE
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)src_vaddr, 1);
    TEST_ASSERT("the source vector register is seeded before the two-PMP-entry cross-page vse64.v probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)vaddr, 1);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a single active vse64.v element crossing a page boundary must update both PMP-allowed halves without a false store access fault",
        excpt.triggered == false &&
        observed_vstart == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the two-PMP-entry vector store writes exactly the split e64 element bytes and preserves adjacent guards",
        ai_micro_cross_page_access_bytes_match(low_phys, src_pattern, 4U) &&
        ai_micro_cross_page_access_bytes_match(high_phys, src_pattern + 4U, 4U) &&
        read8(low_guard_phys) == low_guard &&
        read8(high_guard_phys) == high_guard
    );
    TEST_ASSERT("the source image for the vector store positive AP-35 probe remains intact",
        ai_micro_cross_page_access_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_strided_load_multi_pmp_entries_allow_no_false_access_fault_corner()
{
    TEST_START();

    const enum test_page page0 = VSRWX_GRWX;
    const enum test_page page1 = VSRWX_GRW;
    const enum test_page page2 = VSRWX_GRX;
    const enum test_page page3 = VSRWX_GR;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t element_offset = 0x80U;
    const uintptr_t base_vaddr = hs_page_base(page0) + element_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x400U;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + 0x400U;
    const uint64_t values[4] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff00ULL,
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL
    };
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    write64(phys_page_base(page0) + element_offset, values[0]);
    write64(phys_page_base(page1) + element_offset, values[1]);
    write64(phys_page_base(page2) + element_offset, values[2]);
    write64(phys_page_base(page3) + element_offset, values[3]);
    for (unsigned i = 0; i < 4U; ++i) {
        write64(scratch_phys + (i * sizeof(uint64_t)), 0xccccccccccccccccULL);
    }
    ai_micro_configure_pmp_four_page_allow(page0, page1, page2, page3,
        AI_MICRO_PMP_TOR_R,
        AI_MICRO_PMP_TOR_RW,
        AI_MICRO_PMP_TOR_RWX,
        AI_MICRO_PMP_TOR_R);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(page0, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(page1, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(page2, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(page3, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the AP-35 strided vector load probe maps four elements to four adjacent physical pages and four distinct PMP allow entries",
        phys_page_base(page1) == phys_page_base(page0) + PAGE_SIZE &&
        phys_page_base(page2) == phys_page_base(page1) + PAGE_SIZE &&
        phys_page_base(page3) == phys_page_base(page2) + PAGE_SIZE
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_access_vle64_to_v6_strided_dump(
        (const uint64_t *)base_vaddr, (intptr_t)PAGE_SIZE,
        (uint64_t *)scratch_vaddr, 4U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("vlse64.v with vl=4 must not collapse four per-element PMP-allowed pages into one over-strict whole-instruction PMP range",
        excpt.triggered == false &&
        observed_vstart == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the strided multi-PMP-entry vector load returns every element from its own PMP entry in element order",
        read64(scratch_phys + 0U * sizeof(uint64_t)) == values[0] &&
        read64(scratch_phys + 1U * sizeof(uint64_t)) == values[1] &&
        read64(scratch_phys + 2U * sizeof(uint64_t)) == values[2] &&
        read64(scratch_phys + 3U * sizeof(uint64_t)) == values[3]
    );
    TEST_ASSERT("the strided load positive AP-35 probe leaves all source pages unchanged",
        read64(phys_page_base(page0) + element_offset) == values[0] &&
        read64(phys_page_base(page1) + element_offset) == values[1] &&
        read64(phys_page_base(page2) + element_offset) == values[2] &&
        read64(phys_page_base(page3) + element_offset) == values[3]
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_multi_pmp_entries_allow_updates_all_lanes_corner()
{
    TEST_START();

    const enum test_page page0 = VSRW_GRWX;
    const enum test_page page1 = VSRW_GRW;
    const enum test_page page2 = VSRW_GRX;
    const enum test_page page3 = VSRW_GR;
    const enum test_page src_page = SWITCH1;
    const uintptr_t element_offset = 0x180U;
    const uintptr_t base_vaddr = hs_page_base(page0) + element_offset;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x480U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x480U;
    const uint64_t values[4] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x1020304050607080ULL,
        0xf0e1d2c3b4a59687ULL
    };
    const uint8_t before_guard[4] = { 0x41U, 0x42U, 0x43U, 0x44U };
    const uint8_t after_guard[4] = { 0x81U, 0x82U, 0x83U, 0x84U };
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < 4U; ++i) {
        write64(src_phys + (i * sizeof(uint64_t)), values[i]);
    }
    write64(phys_page_base(page0) + element_offset, 0x1111111111111111ULL);
    write64(phys_page_base(page1) + element_offset, 0x2222222222222222ULL);
    write64(phys_page_base(page2) + element_offset, 0x3333333333333333ULL);
    write64(phys_page_base(page3) + element_offset, 0x4444444444444444ULL);
    write8(phys_page_base(page0) + element_offset - 1U, before_guard[0]);
    write8(phys_page_base(page1) + element_offset - 1U, before_guard[1]);
    write8(phys_page_base(page2) + element_offset - 1U, before_guard[2]);
    write8(phys_page_base(page3) + element_offset - 1U, before_guard[3]);
    write8(phys_page_base(page0) + element_offset + sizeof(uint64_t), after_guard[0]);
    write8(phys_page_base(page1) + element_offset + sizeof(uint64_t), after_guard[1]);
    write8(phys_page_base(page2) + element_offset + sizeof(uint64_t), after_guard[2]);
    write8(phys_page_base(page3) + element_offset + sizeof(uint64_t), after_guard[3]);
    ai_micro_configure_pmp_four_page_allow(page0, page1, page2, page3,
        AI_MICRO_PMP_TOR_RW,
        AI_MICRO_PMP_TOR_RWX,
        AI_MICRO_PMP_TOR_RW,
        AI_MICRO_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(page0, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(page1, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(page2, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(page3, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the AP-35 strided vector store probe maps four store elements to four adjacent physical pages and four distinct PMP allow entries",
        phys_page_base(page1) == phys_page_base(page0) + PAGE_SIZE &&
        phys_page_base(page2) == phys_page_base(page1) + PAGE_SIZE &&
        phys_page_base(page3) == phys_page_base(page2) + PAGE_SIZE
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_access_vle64_vsse64_window(
        (const uint64_t *)src_vaddr, (uint64_t *)base_vaddr,
        (intptr_t)PAGE_SIZE, 4U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("vsse64.v with vl=4 must not collapse four per-element PMP-allowed pages into one over-strict whole-instruction PMP range",
        excpt.triggered == false &&
        observed_vstart == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the strided multi-PMP-entry vector store writes every active element to its own PMP entry",
        read64(phys_page_base(page0) + element_offset) == values[0] &&
        read64(phys_page_base(page1) + element_offset) == values[1] &&
        read64(phys_page_base(page2) + element_offset) == values[2] &&
        read64(phys_page_base(page3) + element_offset) == values[3]
    );
    TEST_ASSERT("the strided multi-PMP-entry vector store preserves byte guards around every element slot and leaves the source image intact",
        read8(phys_page_base(page0) + element_offset - 1U) == before_guard[0] &&
        read8(phys_page_base(page1) + element_offset - 1U) == before_guard[1] &&
        read8(phys_page_base(page2) + element_offset - 1U) == before_guard[2] &&
        read8(phys_page_base(page3) + element_offset - 1U) == before_guard[3] &&
        read8(phys_page_base(page0) + element_offset + sizeof(uint64_t)) == after_guard[0] &&
        read8(phys_page_base(page1) + element_offset + sizeof(uint64_t)) == after_guard[1] &&
        read8(phys_page_base(page2) + element_offset + sizeof(uint64_t)) == after_guard[2] &&
        read8(phys_page_base(page3) + element_offset + sizeof(uint64_t)) == after_guard[3] &&
        read64(src_phys + 0U * sizeof(uint64_t)) == values[0] &&
        read64(src_phys + 1U * sizeof(uint64_t)) == values[1] &&
        read64(src_phys + 2U * sizeof(uint64_t)) == values[2] &&
        read64(src_phys + 3U * sizeof(uint64_t)) == values[3]
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_start_pmp_deny_tail_allow_tval_original_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t deny_start = phys_page_base(low_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint8_t payload[8] = {
        0x31, 0x32, 0x33, 0x34,
        0x51, 0x52, 0x53, 0x54
    };

    goto_priv(PRIV_M);
    ai_micro_cross_page_access_write_bytes(low_phys, payload, 4U);
    ai_micro_cross_page_access_write_bytes(high_phys, payload + 4U, 4U);
    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an AP-98 start-denied cross-page load must report the original misaligned address and must not be allowed by the tail page's PMP entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-98 start-denied cross-page load leaves both low and high source bytes unchanged",
        ai_micro_cross_page_access_bytes_match(low_phys, payload, 4U) &&
        ai_micro_cross_page_access_bytes_match(high_phys, payload + 4U, 4U)
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    TEST_END();
}

bool ai_micro_hs_cross_page_overlap_pmp_start_priority_deny_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRW;
    const enum test_page high_page = VSRWX_GRX;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t store_value = 0x8877665544332211ULL;
    const uint8_t payload[8] = {
        0x91, 0x92, 0x93, 0x94,
        0xb1, 0xb2, 0xb3, 0xb4
    };

    goto_priv(PRIV_M);
    ai_micro_cross_page_access_write_bytes(low_phys, payload, 4U);
    ai_micro_cross_page_access_write_bytes(high_phys, payload + 4U, 4U);
    ai_micro_configure_pmp_overlap_low_priority_window(low_page, AI_MICRO_PMP_NAPOT_X);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the AP-35 overlapping-PMP priority probe uses adjacent pages and an 8KB-aligned two-page NAPOT allow window",
        phys_page_base(high_page) == phys_page_base(low_page) + PAGE_SIZE &&
        (phys_page_base(low_page) & ((2U * PAGE_SIZE) - 1U)) == 0U
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a lower-numbered overlapping PMP entry denying read on the start page must win over the broader two-page allow entry and report the original load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the overlapping-PMP start-denied load leaves both split source halves unchanged before the store priority probe",
        ai_micro_cross_page_access_bytes_match(low_phys, payload, 4U) &&
        ai_micro_cross_page_access_bytes_match(high_phys, payload + 4U, 4U)
    );
    ai_micro_configure_pmp_overlap_low_priority_window(low_page, AI_MICRO_PMP_NAPOT_R);

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("a lower-numbered overlapping PMP entry denying write on the start page must win over the broader two-page allow entry and report the original store address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the overlapping-PMP start-denied store leaves both low-page tail and high-page head bytes unchanged",
        ai_micro_cross_page_access_bytes_match(low_phys, payload, 4U) &&
        ai_micro_cross_page_access_bytes_match(high_phys, payload + 4U, 4U)
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    TEST_END();
}

bool ai_micro_hs_load_cross_page_pmp_reconfig_allow_deny_allow_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint8_t payload[8] = {
        0x41, 0x42, 0x43, 0x44,
        0x61, 0x62, 0x63, 0x64
    };
    const uint64_t expected = ai_micro_cross_page_access_pack_u64(payload);
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_access_write_bytes(low_phys, payload, 4U);
    ai_micro_cross_page_access_write_bytes(high_phys, payload + 4U, 4U);
    ai_micro_configure_pmp_two_page_allow(low_page, high_page,
        AI_MICRO_PMP_TOR_R, AI_MICRO_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT("the first AP-35 PMP-reconfiguration probe succeeds when both split pages are covered by allowed PMP entries",
        excpt.triggered == false &&
        observed == expected,
        "observed=0x%llx expected=0x%llx vaddr=0x%llx curr_priv=%u",
        (unsigned long long)observed,
        (unsigned long long)expected,
        (unsigned long long)vaddr,
        curr_priv
    );

    goto_priv(PRIV_M);
    ai_micro_configure_pmp_window(high_deny_start, high_deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after reconfiguring the high page to read-deny and synchronizing translations, the same cross-page load must not use a stale two-entry-allow permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_configure_pmp_two_page_allow(low_page, high_page,
        AI_MICRO_PMP_TOR_R, AI_MICRO_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT("after restoring the high page PMP allow entry and synchronizing again, the same cross-page load recovers and returns the original split payload",
        excpt.triggered == false &&
        observed == expected,
        "observed=0x%llx expected=0x%llx vaddr=0x%llx curr_priv=%u",
        (unsigned long long)observed,
        (unsigned long long)expected,
        (unsigned long long)vaddr,
        curr_priv
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the allow-deny-allow PMP reconfiguration sequence does not modify either source half",
        ai_micro_cross_page_access_bytes_match(low_phys, payload, 4U) &&
        ai_micro_cross_page_access_bytes_match(high_phys, payload + 4U, 4U)
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    TEST_END();
}

bool ai_micro_hs_store_cross_page_pmp_reconfig_allow_deny_allow_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t first_store = 0x8877665544332211ULL;
    const uint64_t faulting_store = 0xfedcba9876543210ULL;
    const uint64_t final_store = 0x0123456789abcdefULL;
    const uint8_t initial[8] = {
        0x21, 0x22, 0x23, 0x24,
        0x71, 0x72, 0x73, 0x74
    };

    goto_priv(PRIV_M);
    ai_micro_cross_page_access_write_bytes(low_phys, initial, 4U);
    ai_micro_cross_page_access_write_bytes(high_phys, initial + 4U, 4U);
    ai_micro_configure_pmp_two_page_allow(low_page, high_page,
        AI_MICRO_PMP_TOR_RW, AI_MICRO_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store);
    TEST_ASSERT("the first AP-35 store PMP-reconfiguration probe succeeds when both split pages are covered by allowed PMP entries",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first two-entry-allow store writes the low and high halves before the deny phase starts",
        ai_micro_cross_page_access_store_u64_matches(low_phys, high_phys, first_store)
    );
    ai_micro_configure_pmp_window(high_deny_start, high_deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, faulting_store);
    TEST_ASSERT("after reconfiguring the high page to write-deny and synchronizing translations, the same cross-page store must not use a stale two-entry-allow permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-half-denied store reconfiguration phase leaves the denied high-page bytes intact before the allow entry is restored",
        read8(high_phys + 0U) == ((first_store >> 32) & 0xffU) &&
        read8(high_phys + 1U) == ((first_store >> 40) & 0xffU) &&
        read8(high_phys + 2U) == ((first_store >> 48) & 0xffU) &&
        read8(high_phys + 3U) == ((first_store >> 56) & 0xffU)
    );
    ai_micro_configure_pmp_two_page_allow(low_page, high_page,
        AI_MICRO_PMP_TOR_RW, AI_MICRO_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, final_store);
    TEST_ASSERT("after restoring the high page PMP allow entry and synchronizing again, the same cross-page store recovers and commits a new value",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the restored two-entry-allow store updates both split halves to the final value",
        ai_micro_cross_page_access_store_u64_matches(low_phys, high_phys, final_store)
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_access_addr(VSRWX_GRX, 4U);
    uintptr_t deny_start = phys_page_base(VSRWX_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS cross-page misaligned load reports access fault when the lower half lands in a read-denied page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_access_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t deny_start = phys_page_base(VSRWX_GR);
    uintptr_t deny_end = deny_start + PAGE_SIZE;

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS cross-page misaligned load reports the second-half start address when only the second page hits access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_access_fault_then_valid_load_recovers_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_page_access_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = fault_vaddr + 4U;
    uintptr_t deny_start = phys_page_base(VSRWX_GR);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t ok_vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX) + 0x180U;
    const uint64_t ok_pattern = 0x0f1e2d3c4b5a6978ULL;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(ok_phys, ok_pattern);

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(fault_vaddr);
    TEST_ASSERT("an HS cross-page load first reports a second-half access fault when only the upper page loses read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after the second-half load access fault, a following aligned HS load to a different PMP-allowed page still succeeds immediately",
        excpt.triggered == false &&
        value == ok_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_access_fault_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_page_access_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = fault_vaddr + 4U;
    uintptr_t deny_start = phys_page_base(VSRWX_GR);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t ok_vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t ok_prev_phys = phys_page_base(VSRWX_GRWX) + 0x178U;
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t ok_next_phys = phys_page_base(VSRWX_GRWX) + 0x188U;
    const uint64_t ok_prev_pattern = 0x1122334455667788ULL;
    const uint64_t ok_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t ok_next_pattern = 0x8877665544332211ULL;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(ok_prev_phys, ok_prev_pattern);
    write64(ok_phys, ok_pattern);
    write64(ok_next_phys, ok_next_pattern);

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(fault_vaddr);
    TEST_ASSERT("an HS cross-page load first reports a second-half access fault when only the upper page loses read permission in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second-half load access-fault path still leaves the recovery target word and its adjacent doublewords unchanged before the follow-up aligned load",
        read64(ok_prev_phys) == ok_prev_pattern &&
        read64(ok_phys) == ok_pattern &&
        read64(ok_next_phys) == ok_next_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after the second-half load access fault, a following aligned HS load to a different PMP-allowed page still succeeds immediately in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == ok_pattern
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovery load completes, the recovery target word and its adjacent doublewords still match their expected values",
        read64(ok_prev_phys) == ok_prev_pattern &&
        read64(ok_phys) == ok_pattern &&
        read64(ok_next_phys) == ok_next_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_configure_pmp_window(low_deny_start, low_deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first split-load probe reports a low-half access fault with tval bound to the original misaligned cross-page load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first low-half access-faulting split load leaves both participating pages unchanged before the same-window high-half refresh probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_configure_pmp_window(high_deny_start, high_deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after the earlier low-half access fault has already populated the exception path, the follow-up high-half access fault on the same split-load window must refresh tval to the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refreshed high-half access-fault probe still leaves both boundary doublewords unchanged because faulting split loads must not create visible memory side effects",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cross_page_access_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_store_fault =
        ai_micro_cross_page_access_expected_low_after_4b_store(low_pattern, store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_configure_pmp_window(low_deny_start, low_deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first shared cross-page exception probe comes from LoadUnit and reports a low-half load access fault with tval at the original split-load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first low-half load access fault, both participating boundary doublewords remain unchanged before the follow-up shared store-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_configure_pmp_window(high_deny_start, high_deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("when the next shared cross-page exception comes from StoreUnit on the same window, MemBlock must refresh both cause and tval to the newer high-half store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the load-then-store shared access-fault switch still shows the newer store-side low-half fragment visibility while the protected upper page keeps its original bytes",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    TEST_END();
}

bool ai_micro_hs_cross_page_access_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_store_fault =
        ai_micro_cross_page_access_expected_low_after_4b_store(low_pattern, store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_configure_pmp_window(high_deny_start, high_deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("the first shared cross-page exception probe comes from StoreUnit and reports a high-half store access fault with tval at the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first high-half store access fault, only the low-half fragment from the store becomes visible while the protected upper-page bytes remain unchanged",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );
    ai_micro_configure_pmp_window(low_deny_start, low_deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("when the next shared cross-page exception comes from LoadUnit on the same window, MemBlock must refresh both cause and tval to the newer low-half load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the store-then-load shared access-fault switch keeps the earlier store-side low-half fragment stable while the follow-up load fault adds no new memory side effects",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_access_addr(VSRW_GRW, 4U);
    uintptr_t low_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - sizeof(uint64_t);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t deny_start = phys_page_base(VSRW_GRW);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS cross-page misaligned store reports the original virtual address when the lower half lands in a write-denied page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a low-half store access fault leaves both participating pages unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_access_addr(VSRW_GRW, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRW_GRX);
    uintptr_t deny_start = phys_page_base(VSRW_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(deny_start, high_pattern);

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("an HS cross-page misaligned store reports the upper-half address when only the second page loses write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a high-half store access fault leaves the protected upper-page bytes unchanged",
        read64(deny_start) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_access_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_access_addr(VSRW_GRW, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRW_GRX);
    uintptr_t low_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - sizeof(uint64_t);
    uintptr_t deny_start = phys_page_base(VSRW_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(deny_start, high_pattern);

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("a cross-page store with a high-half access fault reports the second-half address and leaves both participating pages unchanged",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == high_half_vaddr &&
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_access_fault_then_valid_store_recovers_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_page_access_addr(VSRW_GRW, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRW_GRX);
    uintptr_t low_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - sizeof(uint64_t);
    uintptr_t deny_start = phys_page_base(VSRW_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t ok_vaddr = hs_page_base(VSRW_GRWX) + 0x120U;
    uintptr_t ok_phys = phys_page_base(VSRW_GRWX) + 0x120U;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t ok_pattern = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(deny_start, high_pattern);
    write64(ok_phys, 0ULL);

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("the first faulting split store reports a high-half access fault at the second-half address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting split-store access-fault path leaves both participating pages unchanged before the recovery store is issued",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, ok_pattern);
    TEST_ASSERT("after the no-side-effect store access-fault corner, a following aligned HS store to a different PMP-allowed page still commits normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovery store updates only its target word while the earlier access-fault split-store footprint remains unchanged",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern &&
        read64(ok_phys) == ok_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_access_fault_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_page_access_addr(VSRW_GRW, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRW_GRX);
    uintptr_t low_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - sizeof(uint64_t);
    uintptr_t deny_start = phys_page_base(VSRW_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t ok_vaddr = hs_page_base(VSRW_GRWX) + 0x120U;
    uintptr_t ok_prev_phys = phys_page_base(VSRW_GRWX) + 0x118U;
    uintptr_t ok_phys = phys_page_base(VSRW_GRWX) + 0x120U;
    uintptr_t ok_next_phys = phys_page_base(VSRW_GRWX) + 0x128U;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t ok_prev_pattern = 0x8877665544332211ULL;
    const uint64_t ok_pattern = 0x89abcdef01234567ULL;
    const uint64_t ok_next_pattern = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(deny_start, high_pattern);
    write64(ok_prev_phys, ok_prev_pattern);
    write64(ok_phys, 0ULL);
    write64(ok_next_phys, ok_next_pattern);

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("the first faulting split store reports a high-half access fault at the second-half address in the adjacent-preservation case",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting split-store access-fault path leaves both participating pages and the recovery target plus adjacent doublewords unchanged before the recovery store",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern &&
        read64(ok_prev_phys) == ok_prev_pattern &&
        read64(ok_phys) == 0ULL &&
        read64(ok_next_phys) == ok_next_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, ok_pattern);
    TEST_ASSERT("after the no-side-effect store access-fault corner, a following aligned HS store to a different PMP-allowed page still commits normally in the adjacent-preservation case",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovery store updates only its target word while both its adjacent doublewords and the earlier access-fault split-store footprint remain unchanged",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern &&
        read64(ok_prev_phys) == ok_prev_pattern &&
        read64(ok_phys) == ok_pattern &&
        read64(ok_next_phys) == ok_next_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_access_addr(low_page, 4U);
    const uintptr_t high_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t second_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_second_fault =
        ai_micro_cross_page_access_expected_low_after_4b_store(low_pattern, second_store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_configure_pmp_window(low_deny_start, low_deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("the first split-store probe reports a low-half access fault with tval bound to the original misaligned cross-page store address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first low-half access-faulting split store leaves both participating pages unchanged before the same-window high-half refresh probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_configure_pmp_window(high_deny_start, high_deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store_value);
    TEST_ASSERT("after the earlier low-half access fault has already populated the exception path, the follow-up high-half access fault on the same split-store window must refresh tval to the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refreshed high-half access-fault probe exposes only the low-half fragment from the newer store value while the protected upper page still keeps its original bytes",
        read64(low_phys) == expected_low_after_second_fault &&
        read64(high_phys) == high_pattern
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_load_unitstride_split_element_second_flow_access_fault_no_partial_dest_update_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page old_page = SWITCH1;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = ai_micro_cross_page_access_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x260U;
    const uintptr_t old_phys = phys_page_base(old_page) + 0x260U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x2a0U;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + 0x2a0U;
    const uintptr_t boundary_before_addr = low_phys - 1U;
    const uintptr_t boundary_after_addr = high_phys + 9U;
    const uintptr_t scratch_before_addr = scratch_phys - 1U;
    const uintptr_t scratch_after_addr = scratch_phys + 16U;
    const uint8_t boundary_before = 0x7bU;
    const uint8_t boundary_after = 0xb7U;
    const uint8_t scratch_before = 0x4cU;
    const uint8_t scratch_after = 0xc4U;
    const uint8_t old_vd_bytes[16] = {
        0xd0, 0xd1, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb,
        0xdc, 0xdd, 0xde, 0xdf
    };
    const uint8_t fault_src_bytes[16] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab,
        0xac, 0xad, 0xae, 0xaf
    };
    const uint8_t expected_scratch[16] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb,
        0xdc, 0xdd, 0xde, 0xdf
    };
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_access_write_bytes(low_phys, fault_src_bytes, 7U);
    ai_micro_cross_page_access_write_bytes(high_phys, fault_src_bytes + 7U, 9U);
    ai_micro_cross_page_access_write_bytes(old_phys, old_vd_bytes, 16U);
    ai_micro_cross_page_access_write_bytes(scratch_phys, old_vd_bytes, 16U);
    write8(boundary_before_addr, boundary_before);
    write8(boundary_after_addr, boundary_after);
    write8(scratch_before_addr, scratch_before);
    write8(scratch_after_addr, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    goto_priv(PRIV_M);
    *ai_micro_hs_leaf_entry(high_page) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("the preload vector load seeds v6 with the full old 4x32-bit payload before the split-element access-fault load probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a 4x32-bit unit-stride vector load starting at page_end-7 reports load access fault on the second element with vstart=1 when the upper-page leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 4);
    TEST_ASSERT("after the split-element access fault is taken, dumping v6 into scratch completes so the destination register footprint can be checked directly",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the split-element vector load access-fault path updates only element0, while the faulting element1 and later elements retain their old bytes",
        ai_micro_cross_page_access_bytes_match(scratch_phys, expected_scratch, 16U)
    );
    TEST_ASSERT("the split load access-fault probe preserves the original source bytes and the surrounding boundary and scratch sentinels",
        ai_micro_cross_page_access_bytes_match(low_phys, fault_src_bytes, 7U) &&
        ai_micro_cross_page_access_bytes_match(high_phys, fault_src_bytes + 7U, 9U) &&
        read8(boundary_before_addr) == boundary_before &&
        read8(boundary_after_addr) == boundary_after &&
        read8(scratch_before_addr) == scratch_before &&
        read8(scratch_after_addr) == scratch_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_unitstride_split_element_second_flow_access_fault_no_partial_faulting_element_commit_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH1;
    const uintptr_t fault_vaddr = ai_micro_cross_page_access_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x300U;
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uintptr_t boundary_before_addr = low_phys - 1U;
    const uintptr_t boundary_after_addr = high_phys + 9U;
    const uint8_t boundary_before = 0x3cU;
    const uint8_t boundary_after = 0xc3U;
    const uint8_t dest_initial[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t src_pattern[16] = {
        0xc0, 0xc1, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb,
        0xcc, 0xcd, 0xce, 0xcf
    };
    const uint8_t expected_after_fault[16] = {
        0xc0, 0xc1, 0xc2, 0xc3,
        0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_access_write_bytes(low_phys, dest_initial, 7U);
    ai_micro_cross_page_access_write_bytes(high_phys, dest_initial + 7U, 9U);
    ai_micro_cross_page_access_write_bytes(src_phys, src_pattern, 16U);
    write8(boundary_before_addr, boundary_before);
    write8(boundary_after_addr, boundary_after);

    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the source vector load stages the full 4x32-bit payload before the split-element access-fault store probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a 4x32-bit unit-stride vector store starting at page_end-7 reports store access fault on the second element with vstart=1 when PMP denies the upper page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the split-element vector store access-fault path commits only element0, and must not expose the low 3 bytes of the faulting element1",
        ai_micro_cross_page_access_bytes_match(low_phys, expected_after_fault, 7U) &&
        ai_micro_cross_page_access_bytes_match(high_phys, expected_after_fault + 7U, 9U)
    );
    TEST_ASSERT("the split-element vector store access-fault probe also preserves the staged source payload and the outer sentinels around the boundary window",
        ai_micro_cross_page_access_bytes_match(src_phys, src_pattern, 16U) &&
        read8(boundary_before_addr) == boundary_before &&
        read8(boundary_after_addr) == boundary_after
    );

    TEST_END();
}
