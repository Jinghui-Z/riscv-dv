#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_LSQ_SMOKE_VS_OFF   (9U)
#define AI_LSQ_SMOKE_VS_MASK  (3ULL << AI_LSQ_SMOKE_VS_OFF)
#define AI_LSQ_SMOKE_VS_DIRTY (3ULL << AI_LSQ_SMOKE_VS_OFF)
#define AI_LSQ_SMOKE_PTE_HIGH_PPN_BIT (1ULL << 46)

static uint64_t ai_lsq_smoke_saved_menvcfg;

static inline pte_t *ai_lsq_smoke_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_lsq_smoke_map_alias_to_base(enum test_page alias_page,
    enum test_page base_page)
{
    *ai_lsq_smoke_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline void ai_lsq_smoke_map_alias_to_base_high_ppn(
    enum test_page alias_page, enum test_page base_page)
{
    *ai_lsq_smoke_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX |
        AI_LSQ_SMOKE_PTE_HIGH_PPN_BIT;
}

static inline uint64_t ai_lsq_smoke_le64(const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (i * 8U);
    }

    return value;
}

static inline void ai_lsq_smoke_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_lsq_smoke_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_lsq_smoke_enable_hs_cbo_inval(void)
{
    goto_priv(PRIV_M);
    ai_lsq_smoke_saved_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(CSR_MENVCFG,
        (ai_lsq_smoke_saved_menvcfg & ~MENVCFG_CBIE11) | MENVCFG_CBIE01);
    goto_priv(PRIV_HS);
}

static inline void ai_lsq_smoke_restore_hs_cbo_inval(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, ai_lsq_smoke_saved_menvcfg);
}

static inline void ai_lsq_smoke_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, AI_LSQ_SMOKE_VS_MASK);
    CSRS(CSR_MSTATUS, AI_LSQ_SMOKE_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_lsq_smoke_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_lsq_smoke_hs_leaf_entry(page) = (phys_page_base(page) >> 2) |
        flags;
}

static inline void ai_lsq_smoke_vle64_v6_m2(const uint64_t *src, int vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_lsq_smoke_vse64_v6_m2(uint64_t *dest, int vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vse64.v v6, (%[dest])\n\t"
        ".option pop\n\t"
        :
        : [dest] "r"(dest),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_lsq_smoke_vec_store_then_faulting_sd(
    const uint8_t *src, uint8_t *dest, uintptr_t fault_addr,
    uint64_t fault_value, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m2, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v6, (%[src])\n\t"
        "vse8.v v6, (%[dest])\n\t"
        "sd %[fault_value], 0(%[fault_addr])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dest] "r"(dest),
          [fault_addr] "r"(fault_addr),
          [fault_value] "r"(fault_value),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_lsq_smoke_dense_scalar_store8(uintptr_t base,
    uint64_t seed)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[v0], 0(%[base])\n\t"
        "sd %[v1], 8(%[base])\n\t"
        "sd %[v2], 16(%[base])\n\t"
        "sd %[v3], 24(%[base])\n\t"
        "sd %[v4], 32(%[base])\n\t"
        "sd %[v5], 40(%[base])\n\t"
        "sd %[v6], 48(%[base])\n\t"
        "sd %[v7], 56(%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [v0] "r"(seed ^ 0x0011223344556677ULL),
          [v1] "r"(seed ^ 0x1021324354657687ULL),
          [v2] "r"(seed ^ 0x2031425364758697ULL),
          [v3] "r"(seed ^ 0x30415263748596a7ULL),
          [v4] "r"(seed ^ 0x405162738495a6b7ULL),
          [v5] "r"(seed ^ 0x5061728394a5b6c7ULL),
          [v6] "r"(seed ^ 0x60718293a4b5c6d7ULL),
          [v7] "r"(seed ^ 0x708192a3b4c5d6e7ULL)
        : "memory");
}

static inline bool ai_lsq_smoke_dense_scalar_store8_match(uintptr_t paddr,
    uint64_t seed)
{
    return read64(paddr) == (seed ^ 0x0011223344556677ULL) &&
        read64(paddr + 8U) == (seed ^ 0x1021324354657687ULL) &&
        read64(paddr + 16U) == (seed ^ 0x2031425364758697ULL) &&
        read64(paddr + 24U) == (seed ^ 0x30415263748596a7ULL) &&
        read64(paddr + 32U) == (seed ^ 0x405162738495a6b7ULL) &&
        read64(paddr + 40U) == (seed ^ 0x5061728394a5b6c7ULL) &&
        read64(paddr + 48U) == (seed ^ 0x60718293a4b5c6d7ULL) &&
        read64(paddr + 56U) == (seed ^ 0x708192a3b4c5d6e7ULL);
}

static inline uint64_t ai_lsq_smoke_cross_page_store_low_partial(
    uint64_t old_low, uint64_t fault_value)
{
    return (old_low & 0xffffffffULL) | ((fault_value & 0xffffffffULL) << 32);
}

static inline bool ai_lsq_smoke_cross_page_fault_footprint_ok(
    uintptr_t low_paddr, uintptr_t high_paddr, uint64_t old_low,
    uint64_t old_high, uint64_t fault_value)
{
    const uint64_t low_image = read64(low_paddr);
    const uint64_t low_partial =
        ai_lsq_smoke_cross_page_store_low_partial(old_low, fault_value);

    return (low_image == old_low || low_image == low_partial) &&
        read64(high_paddr) == old_high;
}

static inline void ai_lsq_smoke_ap106_pattern(uint8_t *bytes, unsigned len,
    uint8_t base)
{
    for (unsigned i = 0; i < len; ++i) {
        bytes[i] = (uint8_t)(base + i * 0x17U + (i >> 1));
    }
}

static inline void ai_lsq_smoke_delayed_older_load_then_two_loads(
    uintptr_t older_load_vaddr, uintptr_t younger_load_vaddr,
    uintptr_t nearby_load_vaddr, uint64_t *older_out,
    uint64_t *younger_out, uint64_t *nearby_out)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0xfedcba9876543210\n\t"
        "li t1, 7\n\t"
        "divu t2, t0, t1\n\t"
        "addi t2, t2, 1\n\t"
        "divu t2, t0, t2\n\t"
        "andi t2, t2, 0\n\t"
        "add t2, t2, %[older]\n\t"
        "ld t3, 0(t2)\n\t"
        "ld t4, 0(%[younger])\n\t"
        "ld t5, 0(%[nearby])\n\t"
        "sd t3, 0(%[older_out])\n\t"
        "sd t4, 0(%[younger_out])\n\t"
        "sd t5, 0(%[nearby_out])\n\t"
        ".option pop\n\t"
        :
        : [older] "r"(older_load_vaddr),
          [younger] "r"(younger_load_vaddr),
          [nearby] "r"(nearby_load_vaddr),
          [older_out] "r"(older_out),
          [younger_out] "r"(younger_out),
          [nearby_out] "r"(nearby_out)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

static inline void ai_lsq_smoke_prefetch_bad_then_same_line_demand_loads(
    uintptr_t bad_prefetch_vaddr, uintptr_t good_alias_vaddr,
    uintptr_t base_vaddr, uint64_t store_value, uint64_t *first_out,
    uint64_t *after_store_out, uint64_t *adjacent_out)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x13, 0x6, x0, %[bad_prefetch], 1\n\t"
        "ld t3, 0(%[good_alias])\n\t"
        "sd %[store_value], 0(%[good_alias])\n\t"
        "ld t4, 0(%[base])\n\t"
        "ld t5, 8(%[good_alias])\n\t"
        "sd t3, 0(%[first_out])\n\t"
        "sd t4, 0(%[after_store_out])\n\t"
        "sd t5, 0(%[adjacent_out])\n\t"
        ".option pop\n\t"
        :
        : [bad_prefetch] "r"(bad_prefetch_vaddr),
          [good_alias] "r"(good_alias_vaddr),
          [base] "r"(base_vaddr),
          [store_value] "r"(store_value),
          [first_out] "r"(first_out),
          [after_store_out] "r"(after_store_out),
          [adjacent_out] "r"(adjacent_out)
        : "t3", "t4", "t5", "memory");
}

static inline void
ai_lsq_smoke_repeated_prefetch_bad_alias_then_demand_reuse_window(
    uintptr_t bad_prefetch_vaddr, uintptr_t good_alias_vaddr,
    uintptr_t base_vaddr, uint64_t store0_value, uint64_t store1_value,
    uint64_t *first_out, uint64_t *after_store0_out,
    uint64_t *before_store1_out, uint64_t *after_store1_out,
    uint64_t *guard_out)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x13, 0x6, x0, %[bad_prefetch], 1\n\t"
        ".insn i 0x13, 0x6, x0, %[bad_prefetch_next], 1\n\t"
        "ld t3, 0(%[good_alias])\n\t"
        "sd %[store0_value], 0(%[good_alias])\n\t"
        "ld t4, 0(%[base])\n\t"
        ".insn i 0x13, 0x6, x0, %[bad_prefetch], 1\n\t"
        "ld t5, 8(%[good_alias])\n\t"
        "sd %[store1_value], 8(%[base])\n\t"
        "ld t6, 8(%[good_alias])\n\t"
        "ld t0, 16(%[good_alias])\n\t"
        "sd t3, 0(%[first_out])\n\t"
        "sd t4, 0(%[after_store0_out])\n\t"
        "sd t5, 0(%[before_store1_out])\n\t"
        "sd t6, 0(%[after_store1_out])\n\t"
        "sd t0, 0(%[guard_out])\n\t"
        ".option pop\n\t"
        :
        : [bad_prefetch] "r"(bad_prefetch_vaddr),
          [bad_prefetch_next] "r"(bad_prefetch_vaddr + sizeof(uint64_t)),
          [good_alias] "r"(good_alias_vaddr),
          [base] "r"(base_vaddr),
          [store0_value] "r"(store0_value),
          [store1_value] "r"(store1_value),
          [first_out] "r"(first_out),
          [after_store0_out] "r"(after_store0_out),
          [before_store1_out] "r"(before_store1_out),
          [after_store1_out] "r"(after_store1_out),
          [guard_out] "r"(guard_out)
        : "t0", "t3", "t4", "t5", "t6", "memory");
}

static inline void ai_lsq_smoke_delayed_store_then_vluxei64_fault1(
    uintptr_t store_vaddr, uintptr_t vector_base_vaddr,
    uintptr_t offsets_vaddr, uint64_t store_value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0xfedcba9876543210\n\t"
        "li t1, 7\n\t"
        "divu t2, t0, t1\n\t"
        "addi t2, t2, 1\n\t"
        "divu t2, t0, t2\n\t"
        "andi t2, t2, 0\n\t"
        "add t2, t2, %[store]\n\t"
        "sd %[store_value], 0(t2)\n\t"
        "li t1, 2\n\t"
        "vsetvli t0, t1, e64, m2, ta, ma\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vluxei64.v v6, (%[base]), v8\n\t"
        ".option pop\n\t"
        :
        : [store] "r"(store_vaddr),
          [base] "r"(vector_base_vaddr),
          [offsets] "r"(offsets_vaddr),
          [store_value] "r"(store_value)
        : "t0", "t1", "t2", "memory");
}

static inline void ai_lsq_smoke_two_delayed_stores_then_vluxei64_fault2(
    uintptr_t store0_vaddr, uintptr_t store1_vaddr,
    uintptr_t vector_base_vaddr, uintptr_t offsets_vaddr,
    uint64_t store0_value, uint64_t store1_value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0xfedcba9876543210\n\t"
        "li t1, 7\n\t"
        "divu t2, t0, t1\n\t"
        "addi t2, t2, 1\n\t"
        "divu t2, t0, t2\n\t"
        "andi t2, t2, 0\n\t"
        "add t2, t2, %[store0]\n\t"
        "li t3, 0x123456789abcdef0\n\t"
        "li t4, 11\n\t"
        "divu t5, t3, t4\n\t"
        "addi t5, t5, 1\n\t"
        "divu t5, t3, t5\n\t"
        "andi t5, t5, 0\n\t"
        "add t5, t5, %[store1]\n\t"
        "sd %[store0_value], 0(t2)\n\t"
        "sd %[store1_value], 0(t5)\n\t"
        "li t1, 3\n\t"
        "vsetvli t0, t1, e64, m2, ta, ma\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vluxei64.v v6, (%[base]), v8\n\t"
        ".option pop\n\t"
        :
        : [store0] "r"(store0_vaddr),
          [store1] "r"(store1_vaddr),
          [base] "r"(vector_base_vaddr),
          [offsets] "r"(offsets_vaddr),
          [store0_value] "r"(store0_value),
          [store1_value] "r"(store1_value)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

static inline void ai_lsq_smoke_mab_prefetch_vector_multiflow_window(
    uintptr_t bad_prefetch_vaddr, uintptr_t bad_prefetch_next_vaddr,
    uintptr_t older_mab_vaddr, uintptr_t younger_alias_vaddr,
    uintptr_t base_line_vaddr, uintptr_t store0_vaddr,
    uintptr_t store1_vaddr, uintptr_t vector_base_vaddr,
    uintptr_t offsets_vaddr, uint64_t scalar_store_value,
    uint64_t store0_value, uint64_t store1_value,
    uint64_t *older_out, uint64_t *younger_out,
    uint64_t *after_store_out)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x13, 0x6, x0, %[bad_prefetch], 1\n\t"
        "li t0, 0xfedcba9876543210\n\t"
        "li t1, 7\n\t"
        "divu t2, t0, t1\n\t"
        "addi t2, t2, 1\n\t"
        "divu t2, t0, t2\n\t"
        "andi t2, t2, 0\n\t"
        "add t2, t2, %[older_mab]\n\t"
        "ld t3, 0(t2)\n\t"
        "ld t4, 0(%[younger_alias])\n\t"
        "sd %[scalar_store_value], 0(%[younger_alias])\n\t"
        "ld t5, 0(%[base_line])\n\t"
        "sd t3, 0(%[older_out])\n\t"
        "sd t4, 0(%[younger_out])\n\t"
        "sd t5, 0(%[after_store_out])\n\t"
        ".insn i 0x13, 0x6, x0, %[bad_prefetch_next], 1\n\t"
        "li t0, 0x123456789abcdef0\n\t"
        "li t1, 11\n\t"
        "divu t2, t0, t1\n\t"
        "addi t2, t2, 1\n\t"
        "divu t2, t0, t2\n\t"
        "andi t2, t2, 0\n\t"
        "add t2, t2, %[store0]\n\t"
        "li t3, 0x0f0e0d0c0b0a0908\n\t"
        "li t4, 13\n\t"
        "divu t5, t3, t4\n\t"
        "addi t5, t5, 1\n\t"
        "divu t5, t3, t5\n\t"
        "andi t5, t5, 0\n\t"
        "add t5, t5, %[store1]\n\t"
        "sd %[store0_value], 0(t2)\n\t"
        "sd %[store1_value], 0(t5)\n\t"
        "li t1, 3\n\t"
        "vsetvli t0, t1, e64, m2, ta, ma\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vluxei64.v v6, (%[vector_base]), v8\n\t"
        ".option pop\n\t"
        :
        : [bad_prefetch] "r"(bad_prefetch_vaddr),
          [bad_prefetch_next] "r"(bad_prefetch_next_vaddr),
          [older_mab] "r"(older_mab_vaddr),
          [younger_alias] "r"(younger_alias_vaddr),
          [base_line] "r"(base_line_vaddr),
          [store0] "r"(store0_vaddr),
          [store1] "r"(store1_vaddr),
          [vector_base] "r"(vector_base_vaddr),
          [offsets] "r"(offsets_vaddr),
          [scalar_store_value] "r"(scalar_store_value),
          [store0_value] "r"(store0_value),
          [store1_value] "r"(store1_value),
          [older_out] "r"(older_out),
          [younger_out] "r"(younger_out),
          [after_store_out] "r"(after_store_out)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

bool ai_micro_ap122_non_aligned_old_load_rar_query_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page older_page = SWITCH1;
    const enum test_page younger_page = SWITCH2;
    const enum test_page nearby_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x300U;
    const uintptr_t older_vaddr = hs_page_base(older_page) + 0x30fU;
    const uintptr_t younger_vaddr = hs_page_base(younger_page) + 0x310U;
    const uintptr_t nearby_vaddr = hs_page_base(nearby_page) + 0x180U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x300U;
    const uintptr_t nearby_paddr = phys_page_base(nearby_page) + 0x180U;
    const uint64_t nearby_seed = 0x0badc0de55aa33ccULL;
    uint8_t line_bytes[64];
    uint64_t older_observed = 0ULL;
    uint64_t younger_observed = 0ULL;
    uint64_t nearby_observed = 0ULL;

    for (unsigned i = 0; i < sizeof(line_bytes); ++i) {
        line_bytes[i] = (uint8_t)(0x10U + i);
    }

    goto_priv(PRIV_M);
    ai_lsq_smoke_write_bytes(base_paddr, line_bytes, sizeof(line_bytes));
    write64(nearby_paddr, nearby_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lsq_smoke_map_alias_to_base(older_page, base_page);
    ai_lsq_smoke_map_alias_to_base(younger_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(base_vaddr + 0x10U);
    TEST_ASSERT("AP-122 smoke setup warms the shared cache line before invalidating it for the non-aligned older-load replay window",
        excpt.triggered == false &&
        warmed == ai_lsq_smoke_le64(&line_bytes[0x10U])
    );

    ai_lsq_smoke_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr + 0x10U);
    TEST_ASSERT("AP-122 smoke setup can invalidate the shared line before the older non-aligned load reaches its split-load window",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    TEST_ASSERT("AP-122 smoke setup orders the invalidate before issuing the older non-aligned load and younger same-line load sequence",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_delayed_older_load_then_two_loads(older_vaddr,
        younger_vaddr, nearby_vaddr, &older_observed, &younger_observed,
        &nearby_observed);
    TEST_ASSERT("AP-122 manual smoke: an older non-aligned load crossing a 16B boundary and a younger same-line aligned load both retire with the expected line image instead of hanging or observing stale replay data",
        excpt.triggered == false &&
        older_observed == ai_lsq_smoke_le64(&line_bytes[0x0fU]) &&
        younger_observed == ai_lsq_smoke_le64(&line_bytes[0x10U]) &&
        nearby_observed == nearby_seed,
        "older=0x%lx expected=0x%lx younger=0x%lx expected_y=0x%lx nearby=0x%lx",
        older_observed, ai_lsq_smoke_le64(&line_bytes[0x0fU]),
        younger_observed, ai_lsq_smoke_le64(&line_bytes[0x10U]),
        nearby_observed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-122 manual smoke leaves the shared cache-line bytes and nearby word unchanged; RTL closure still requires ldld_nuke_query/updatePaddrValid waveform checks",
        ai_lsq_smoke_bytes_match(base_paddr, line_bytes, sizeof(line_bytes)) &&
        read64(nearby_paddr) == nearby_seed
    );
    ai_lsq_smoke_restore_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_lsq_mab_prefetch_vector_multiflow_rar_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page bad_prefetch_page = SWITCH1;
    const enum test_page good_alias_page = SWITCH2;
    const enum test_page fault_page = VSRX_GRWX;
    const enum test_page offsets_page = SCRATCHPAD;
    const enum test_page dump_page = VSRW_GRWX;
    const uintptr_t line_offset = 0x700U;
    const uintptr_t vector_offset = 0x780U;
    const uintptr_t bad_prefetch_vaddr =
        hs_page_base(bad_prefetch_page) + line_offset;
    const uintptr_t older_mab_vaddr =
        hs_page_base(good_alias_page) + line_offset + 0x0fU;
    const uintptr_t younger_alias_vaddr =
        hs_page_base(good_alias_page) + line_offset + 0x10U;
    const uintptr_t base_line_vaddr = hs_page_base(base_page) + line_offset +
        0x10U;
    const uintptr_t vector_base_vaddr =
        hs_page_base(base_page) + vector_offset;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x180U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x300U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x280U;
    const uintptr_t base_line_paddr = phys_page_base(base_page) + line_offset;
    const uintptr_t vector_paddr = phys_page_base(base_page) + vector_offset;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x180U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x300U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x280U;
    const uint64_t scalar_store_value = 0x1251225a55aa7711ULL;
    const uint64_t vec_old0 = 0x0102030405060708ULL;
    const uint64_t vec_old1 = 0x1112131415161718ULL;
    const uint64_t store0_value = 0xaaaabbbbccccddddULL;
    const uint64_t store1_value = 0x2122232425262728ULL;
    const uint64_t fault_seed = 0x8988878685848382ULL;
    const uint64_t dump_seed0 = 0x0bad129000000000ULL;
    const uint64_t dump_seed1 = 0x0bad129000000001ULL;
    const uint64_t dump_seed2 = 0x0bad129000000002ULL;
    const uint64_t offsets[3] = {
        0ULL,
        sizeof(uint64_t),
        (uint64_t)(fault_vaddr - vector_base_vaddr)
    };
    uint8_t line_bytes[64];
    uint64_t older_observed = 0ULL;
    uint64_t younger_observed = 0ULL;
    uint64_t after_store_observed = 0ULL;
    uint64_t observed_vstart = 0ULL;

    for (unsigned i = 0; i < sizeof(line_bytes); ++i) {
        line_bytes[i] = (uint8_t)(0x60U + ((i * 5U) & 0x3fU));
    }

    goto_priv(PRIV_M);
    ai_lsq_smoke_write_bytes(base_line_paddr, line_bytes,
        sizeof(line_bytes));
    write64(vector_paddr, vec_old0);
    write64(vector_paddr + sizeof(uint64_t), vec_old1);
    write64(fault_paddr, fault_seed);
    write64(offsets_paddr, offsets[0]);
    write64(offsets_paddr + sizeof(uint64_t), offsets[1]);
    write64(offsets_paddr + 2U * sizeof(uint64_t), offsets[2]);
    write64(dump_paddr, dump_seed0);
    write64(dump_paddr + sizeof(uint64_t), dump_seed1);
    write64(dump_paddr + 2U * sizeof(uint64_t), dump_seed2);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lsq_smoke_map_alias_to_base_high_ppn(bad_prefetch_page, base_page);
    ai_lsq_smoke_map_alias_to_base(good_alias_page, base_page);
    *ai_lsq_smoke_hs_leaf_entry(fault_page) =
        (phys_page_base(fault_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    sfence_vma();

    ai_lsq_smoke_enable_vector_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_vle64_v6_m2((const uint64_t *)dump_vaddr, 3);
    TEST_ASSERT("AP-122/AP-125/AP-129 cross smoke setup initializes v6 before the mixed prefetch, MAB split, and vector precise-fault window",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_mab_prefetch_vector_multiflow_window(
        bad_prefetch_vaddr, bad_prefetch_vaddr + sizeof(uint64_t),
        older_mab_vaddr, younger_alias_vaddr, base_line_vaddr,
        vector_base_vaddr, vector_base_vaddr + sizeof(uint64_t),
        vector_base_vaddr, offsets_vaddr, scalar_store_value,
        store0_value, store1_value, &older_observed, &younger_observed,
        &after_store_observed);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-122/AP-125/AP-129 cross smoke: silent bad-alias software prefetches, a non-aligned MAB demand-load RAR window, and a later vector indexed-load precise fault coexist without losing the expected vstart=2 fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 2ULL,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    TEST_ASSERT("AP-122/AP-125 cross smoke keeps the real demand-load line image coherent while ignored prefetch entries do not poison the MAB split or same-line demand path",
        older_observed == ai_lsq_smoke_le64(&line_bytes[0x0fU]) &&
        younger_observed == ai_lsq_smoke_le64(&line_bytes[0x10U]) &&
        after_store_observed == scalar_store_value,
        "older=0x%lx expected=0x%lx younger=0x%lx expected_y=0x%lx after=0x%lx expected_after=0x%lx",
        older_observed, ai_lsq_smoke_le64(&line_bytes[0x0fU]),
        younger_observed, ai_lsq_smoke_le64(&line_bytes[0x10U]),
        after_store_observed, scalar_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-129 cross smoke confirms both delayed older stores committed before inspecting vector prefix elements, while the fault target remains unchanged",
        read64(vector_paddr) == store0_value &&
        read64(vector_paddr + sizeof(uint64_t)) == store1_value &&
        read64(fault_paddr) == fault_seed,
        "vec0=0x%lx expected0=0x%lx vec1=0x%lx expected1=0x%lx fault=0x%lx seed=0x%lx",
        read64(vector_paddr), store0_value,
        read64(vector_paddr + sizeof(uint64_t)), store1_value,
        read64(fault_paddr), fault_seed
    );

    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_vse64_v6_m2((uint64_t *)dump_vaddr, 3);
    TEST_ASSERT("AP-129 cross smoke can dump the vector destination after clearing vstart",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-129 cross smoke expects the two pre-vstart indexed-load elements to reflect the older stores even with nearby MAB RAR and ignored software-prefetch pressure",
        read64(dump_paddr) == store0_value &&
        read64(dump_paddr + sizeof(uint64_t)) == store1_value &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == dump_seed2,
        "dump0=0x%lx expected0=0x%lx dump1=0x%lx expected1=0x%lx dump2=0x%lx seed2=0x%lx",
        read64(dump_paddr), store0_value,
        read64(dump_paddr + sizeof(uint64_t)), store1_value,
        read64(dump_paddr + 2U * sizeof(uint64_t)), dump_seed2
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(bad_prefetch_vaddr);
    TEST_ASSERT("AP-125 cross smoke confirms that a real demand load to the same bad high-PPN alias is still freshly faulted after the silent prefetches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_prefetch_vaddr,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx",
        excpt.cause, excpt.tval, bad_prefetch_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-122/AP-125/AP-129 cross smoke leaves backing state with only explicit demand stores and no clobber from the prefetch, MAB split, or faulting vector element",
        read64(base_line_paddr + 0x10U) == scalar_store_value &&
        read64(base_line_paddr + 0x18U) == ai_lsq_smoke_le64(
            &line_bytes[0x18U]) &&
        read64(vector_paddr) == store0_value &&
        read64(vector_paddr + sizeof(uint64_t)) == store1_value &&
        read64(fault_paddr) == fault_seed,
        "line_word=0x%lx scalar=0x%lx guard=0x%lx expected_guard=0x%lx",
        read64(base_line_paddr + 0x10U), scalar_store_value,
        read64(base_line_paddr + 0x18U), ai_lsq_smoke_le64(&line_bytes[0x18U])
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap106_vecst_then_scalar_crosspage_store_fault_cancel_manual_smoke_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page vec_page = VSRWX_GRWX;
    const enum test_page low_page = VSRWX_GRW;
    const enum test_page high_page = VSRWX_GRX;
    const enum test_page recovery_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x100U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x100U;
    const uintptr_t vec_vaddr = hs_page_base(vec_page) + 0x180U;
    const uintptr_t vec_paddr = phys_page_base(vec_page) + 0x180U;
    const uintptr_t fault_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t recovery_vaddr = hs_page_base(recovery_page) + 0x200U;
    const uintptr_t recovery_paddr = phys_page_base(recovery_page) + 0x200U;
    const uint64_t fault_value = 0xfedcba9876543210ULL;
    const uint64_t low_seed = 0x1122334455667788ULL;
    const uint64_t high_seed = 0x8877665544332211ULL;
    const uint64_t recovery_value = 0x1061060000000001ULL;
    uint8_t vec_src[16];
    uint64_t recovery_observed = 0ULL;

    ai_lsq_smoke_ap106_pattern(vec_src, sizeof(vec_src), 0x20U);

    goto_priv(PRIV_M);
    ai_lsq_smoke_write_bytes(src_paddr, vec_src, sizeof(vec_src));
    for (unsigned i = 0; i < sizeof(vec_src); ++i) {
        write8(vec_paddr + i, (uint8_t)(0xa0U + i));
    }
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    write64(recovery_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_lsq_smoke_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_lsq_smoke_enable_vector_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_vec_store_then_faulting_sd((const uint8_t *)src_vaddr,
        (uint8_t *)vec_vaddr, fault_vaddr, fault_value, sizeof(vec_src));
    TEST_ASSERT("AP-106 smoke: an older unit-stride vector store followed by a same-redirect scalar cross-page store fault reports the scalar store-page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_vaddr,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx",
        excpt.cause, excpt.tval, high_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-106 smoke: the older vector store remains committed while the faulting scalar split store keeps the denied high page unchanged",
        ai_lsq_smoke_bytes_match(vec_paddr, vec_src, sizeof(vec_src)) &&
        ai_lsq_smoke_cross_page_fault_footprint_ok(low_paddr, high_paddr,
            low_seed, high_seed, fault_value)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_value);
    recovery_observed = ld(recovery_vaddr);
    TEST_ASSERT("AP-106 smoke: after the scalar redirect, later LSQ enqueue/dequeue is not stuck behind a stale scalar SQ entry",
        excpt.triggered == false &&
        recovery_observed == recovery_value
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap106_scalar_fault_redirect_then_dense_store_enqueue_recovers_manual_smoke_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page vec_page = VSRWX_GRWX;
    const enum test_page low_page = VSRWX_GRW;
    const enum test_page high_page = VSRWX_GRX;
    const enum test_page dense_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x240U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x240U;
    const uintptr_t vec_vaddr = hs_page_base(vec_page) + 0x280U;
    const uintptr_t vec_paddr = phys_page_base(vec_page) + 0x280U;
    const uintptr_t fault_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t dense_vaddr = hs_page_base(dense_page) + 0x300U;
    const uintptr_t dense_paddr = phys_page_base(dense_page) + 0x300U;
    const uint64_t fault_value = 0x0102030405060708ULL;
    const uint64_t low_seed = 0xa1a2a3a4a5a6a7a8ULL;
    const uint64_t high_seed = 0xb1b2b3b4b5b6b7b8ULL;
    const uint64_t dense_seed = 0x106106d000000000ULL;
    uint8_t vec_src[16];

    ai_lsq_smoke_ap106_pattern(vec_src, sizeof(vec_src), 0x40U);

    goto_priv(PRIV_M);
    ai_lsq_smoke_write_bytes(src_paddr, vec_src, sizeof(vec_src));
    for (unsigned i = 0; i < sizeof(vec_src); ++i) {
        write8(vec_paddr + i, (uint8_t)(0x10U + i));
    }
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    for (unsigned i = 0; i < 64U; i += sizeof(uint64_t)) {
        write64(dense_paddr + i, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_lsq_smoke_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_lsq_smoke_enable_vector_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_vec_store_then_faulting_sd((const uint8_t *)src_vaddr,
        (uint8_t *)vec_vaddr, fault_vaddr, fault_value, sizeof(vec_src));
    TEST_ASSERT("AP-106 dense-recovery smoke: the scalar store at the redirect ROB reports a high-half store page fault after an older vector store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_dense_scalar_store8(dense_vaddr, dense_seed);
    TEST_ASSERT("AP-106 dense-recovery smoke: a dense burst of later scalar stores can enqueue and retire after the redirect cleanup",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-106 dense-recovery smoke: dense later stores are visible, the older vector store survives, and the faulting split-store keeps the denied high page old",
        ai_lsq_smoke_dense_scalar_store8_match(dense_paddr, dense_seed) &&
        ai_lsq_smoke_bytes_match(vec_paddr, vec_src, sizeof(vec_src)) &&
        ai_lsq_smoke_cross_page_fault_footprint_ok(low_paddr, high_paddr,
            low_seed, high_seed, fault_value)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap106_repeated_scalar_store_fault_no_stale_sq_nonempty_manual_smoke_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page vec_page = VSRWX_GRWX;
    const enum test_page low_page = VSRWX_GRW;
    const enum test_page high_page = VSRWX_GRX;
    const enum test_page recovery_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x380U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x380U;
    const uintptr_t vec_vaddr = hs_page_base(vec_page) + 0x3c0U;
    const uintptr_t vec_paddr = phys_page_base(vec_page) + 0x3c0U;
    const uintptr_t fault_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t recovery_vaddr = hs_page_base(recovery_page) + 0x480U;
    const uintptr_t recovery_paddr = phys_page_base(recovery_page) + 0x480U;
    const uint64_t low_seed = 0xc1c2c3c4c5c6c7c8ULL;
    const uint64_t high_seed = 0xd1d2d3d4d5d6d7d8ULL;
    const uint64_t recovery0 = 0x1061060000000100ULL;
    const uint64_t recovery1 = 0x1061060000000200ULL;
    uint8_t vec_src0[16];
    uint8_t vec_src1[16];
    uint64_t observed = 0ULL;

    ai_lsq_smoke_ap106_pattern(vec_src0, sizeof(vec_src0), 0x60U);
    ai_lsq_smoke_ap106_pattern(vec_src1, sizeof(vec_src1), 0x80U);

    goto_priv(PRIV_M);
    ai_lsq_smoke_write_bytes(src_paddr, vec_src0, sizeof(vec_src0));
    ai_lsq_smoke_write_bytes(src_paddr + 0x40U, vec_src1, sizeof(vec_src1));
    for (unsigned i = 0; i < sizeof(vec_src0); ++i) {
        write8(vec_paddr + i, 0U);
    }
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    write64(recovery_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_lsq_smoke_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_lsq_smoke_enable_vector_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_vec_store_then_faulting_sd((const uint8_t *)src_vaddr,
        (uint8_t *)vec_vaddr, fault_vaddr, 0x1111222233334444ULL,
        sizeof(vec_src0));
    TEST_ASSERT("AP-106 repeat smoke: first scalar cross-page store fault is precise after an older vector-store SQ entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery0);
    TEST_ASSERT("AP-106 repeat smoke: immediate recovery store after the first redirect is accepted",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_vec_store_then_faulting_sd((const uint8_t *)(src_vaddr + 0x40U),
        (uint8_t *)(vec_vaddr + 0x20U), fault_vaddr,
        0x5555666677778888ULL, sizeof(vec_src1));
    TEST_ASSERT("AP-106 repeat smoke: a second vector-plus-scalar-fault episode does not inherit stale SQ non-empty state from the first redirect",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery1);
    observed = ld(recovery_vaddr);
    TEST_ASSERT("AP-106 repeat smoke: later store/load still retires after two redirect cleanups, guarding against stale scalar exception entries keeping SQ non-empty",
        excpt.triggered == false &&
        observed == recovery1
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-106 repeat smoke: both older vector-store payloads survive and both scalar faults keep the denied high page old across repeated redirects",
        ai_lsq_smoke_bytes_match(vec_paddr, vec_src0, sizeof(vec_src0)) &&
        ai_lsq_smoke_bytes_match(vec_paddr + 0x20U, vec_src1,
            sizeof(vec_src1)) &&
        ai_lsq_smoke_cross_page_fault_footprint_ok(low_paddr, high_paddr,
            low_seed, high_seed, 0x5555666677778888ULL) &&
        read64(recovery_paddr) == recovery1
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap106_vector_committed_entry_survives_scalar_redirect_manual_smoke_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page vec_page = VSRWX_GRWX;
    const enum test_page low_page = VSRWX_GRW;
    const enum test_page high_page = VSRWX_GRX;
    const enum test_page younger_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x500U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x500U;
    const uintptr_t vec_vaddr = hs_page_base(vec_page) + 0x540U;
    const uintptr_t vec_paddr = phys_page_base(vec_page) + 0x540U;
    const uintptr_t fault_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t younger_vaddr = hs_page_base(younger_page) + 0x580U;
    const uintptr_t younger_paddr = phys_page_base(younger_page) + 0x580U;
    const uint64_t low_seed = 0xe1e2e3e4e5e6e7e8ULL;
    const uint64_t high_seed = 0xf1f2f3f4f5f6f7f8ULL;
    const uint64_t younger_value = 0x1061060000000300ULL;
    uint8_t vec_src[32];

    ai_lsq_smoke_ap106_pattern(vec_src, sizeof(vec_src), 0xa0U);

    goto_priv(PRIV_M);
    ai_lsq_smoke_write_bytes(src_paddr, vec_src, sizeof(vec_src));
    for (unsigned i = 0; i < sizeof(vec_src); ++i) {
        write8(vec_paddr + i, (uint8_t)(0x55U + i));
    }
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    write64(younger_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_lsq_smoke_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_lsq_smoke_enable_vector_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_vec_store_then_faulting_sd((const uint8_t *)src_vaddr,
        (uint8_t *)vec_vaddr, fault_vaddr, 0xa5a55a5adeadc0deULL,
        sizeof(vec_src));
    TEST_ASSERT("AP-106 vector-policy smoke: scalar redirect after a wider older vector store reports the scalar store-page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-106 vector-policy smoke: the older vector committed SQ entry is not incorrectly canceled by the scalar redirect",
        ai_lsq_smoke_bytes_match(vec_paddr, vec_src, sizeof(vec_src)) &&
        ai_lsq_smoke_cross_page_fault_footprint_ok(low_paddr, high_paddr,
            low_seed, high_seed, 0xa5a55a5adeadc0deULL)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(younger_vaddr, younger_value);
    TEST_ASSERT("AP-106 vector-policy smoke: a younger scalar store after the redirect can still allocate and retire",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-106 vector-policy smoke: younger post-redirect store is visible while vector payload remains intact",
        read64(younger_paddr) == younger_value &&
        ai_lsq_smoke_bytes_match(vec_paddr, vec_src, sizeof(vec_src))
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap106_mixed_vector_scalar_redirect_wrap_pressure_manual_smoke_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page vec_page = VSRWX_GRWX;
    const enum test_page low_page = VSRWX_GRW;
    const enum test_page high_page = VSRWX_GRX;
    const enum test_page dense_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x700U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x700U;
    const uintptr_t vec_vaddr = hs_page_base(vec_page) + 0x740U;
    const uintptr_t vec_paddr = phys_page_base(vec_page) + 0x740U;
    const uintptr_t fault_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t dense_vaddr = hs_page_base(dense_page) + 0x700U;
    const uintptr_t dense_paddr = phys_page_base(dense_page) + 0x700U;
    const uint64_t low_seed = 0x123456789abcdef0ULL;
    const uint64_t high_seed = 0x0fedcba987654321ULL;
    uint8_t vec_src[16];
    bool ok = true;

    ai_lsq_smoke_ap106_pattern(vec_src, sizeof(vec_src), 0xc0U);

    goto_priv(PRIV_M);
    ai_lsq_smoke_write_bytes(src_paddr, vec_src, sizeof(vec_src));
    for (unsigned i = 0; i < sizeof(vec_src); ++i) {
        write8(vec_paddr + i, (uint8_t)(0x33U + i));
    }
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    for (unsigned i = 0; i < 96U; i += sizeof(uint64_t)) {
        write64(dense_paddr + i, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_lsq_smoke_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_lsq_smoke_enable_vector_state();
    goto_priv(PRIV_HS);
    for (unsigned round = 0; round < 3U; ++round) {
        const uint64_t dense_seed = 0x1061060000001000ULL + round;

        TEST_SETUP_EXCEPT();
        ai_lsq_smoke_vec_store_then_faulting_sd((const uint8_t *)src_vaddr,
            (uint8_t *)vec_vaddr, fault_vaddr,
            0x9000000000000000ULL + round, sizeof(vec_src));
        ok = ok && excpt.triggered == true && excpt.cause == CAUSE_SPF &&
            excpt.tval == high_vaddr;

        TEST_SETUP_EXCEPT();
        ai_lsq_smoke_dense_scalar_store8(dense_vaddr + round * 8U,
            dense_seed);
        ok = ok && excpt.triggered == false;

        goto_priv(PRIV_M);
        ok = ok && ai_lsq_smoke_dense_scalar_store8_match(
            dense_paddr + round * 8U, dense_seed) &&
            ai_lsq_smoke_bytes_match(vec_paddr, vec_src, sizeof(vec_src)) &&
            ai_lsq_smoke_cross_page_fault_footprint_ok(low_paddr, high_paddr,
                low_seed, high_seed, 0x9000000000000000ULL + round);
        goto_priv(PRIV_HS);
    }

    TEST_ASSERT("AP-106 wrap-pressure smoke: repeated vector-then-scalar-fault redirects plus dense later stores keep forward progress and avoid stale SQ non-empty backpressure",
        ok
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap129_vluxei64_fault2_dual_prefix_rawq_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x600U;
    const uintptr_t store0_vaddr = base_vaddr;
    const uintptr_t store1_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x180U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x180U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x400U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x600U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x180U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x180U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x400U;
    const uint64_t old0 = 0x0101010102020202ULL;
    const uint64_t old1 = 0x0303030304040404ULL;
    const uint64_t store0_value = 0xaaaabbbbccccddddULL;
    const uint64_t store1_value = 0x1122334455667788ULL;
    const uint64_t fault_seed = 0x9988776655443322ULL;
    const uint64_t dump_seed0 = 0x0bad000000000001ULL;
    const uint64_t dump_seed1 = 0x0bad000000000002ULL;
    const uint64_t dump_seed2 = 0x0bad000000000003ULL;
    const uint64_t offsets[3] = {
        0ULL,
        sizeof(uint64_t),
        (uint64_t)(fault_vaddr - base_vaddr)
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    write64(base_paddr, old0);
    write64(base_paddr + sizeof(uint64_t), old1);
    write64(fault_paddr, fault_seed);
    write64(offsets_paddr, offsets[0]);
    write64(offsets_paddr + sizeof(uint64_t), offsets[1]);
    write64(offsets_paddr + 2U * sizeof(uint64_t), offsets[2]);
    write64(dump_paddr, dump_seed0);
    write64(dump_paddr + sizeof(uint64_t), dump_seed1);
    write64(dump_paddr + 2U * sizeof(uint64_t), dump_seed2);

    goto_priv(PRIV_HS);
    hspt_init();
    *ai_lsq_smoke_hs_leaf_entry(fault_page) =
        (phys_page_base(fault_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    sfence_vma();

    ai_lsq_smoke_enable_vector_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_vle64_v6_m2((const uint64_t *)dump_vaddr, 3);
    TEST_ASSERT("AP-129 fault2 smoke setup initializes three v6 elements before the indexed vector-load precise-fault window",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_two_delayed_stores_then_vluxei64_fault2(store0_vaddr,
        store1_vaddr, base_vaddr, offsets_vaddr, store0_value,
        store1_value);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-129 manual smoke: two delayed older scalar stores before vluxei64 plus element2 page fault reports a precise vector load fault at vstart=2",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 2ULL,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-129 fault2 smoke confirms both older delayed scalar stores committed before inspecting the two pre-vstart vector prefix elements",
        read64(base_paddr) == store0_value &&
        read64(base_paddr + sizeof(uint64_t)) == store1_value &&
        read64(fault_paddr) == fault_seed,
        "base0=0x%lx expected0=0x%lx base1=0x%lx expected1=0x%lx",
        read64(base_paddr), store0_value,
        read64(base_paddr + sizeof(uint64_t)), store1_value
    );

    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_vse64_v6_m2((uint64_t *)dump_vaddr, 3);
    TEST_ASSERT("AP-129 fault2 smoke can dump three vector destination elements after clearing vstart",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-129 fault2 smoke expects both pre-vstart indexed-load elements to reflect older store values while the faulting element keeps its seed; RAWQ element-granular release still requires RTL waveform checks",
        read64(dump_paddr) == store0_value &&
        read64(dump_paddr + sizeof(uint64_t)) == store1_value &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == dump_seed2,
        "dump0=0x%lx expected0=0x%lx dump1=0x%lx expected1=0x%lx dump2=0x%lx seed2=0x%lx",
        read64(dump_paddr), store0_value,
        read64(dump_paddr + sizeof(uint64_t)), store1_value,
        read64(dump_paddr + 2U * sizeof(uint64_t)), dump_seed2
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap125_prefetch_repeated_faulting_alias_ignore_lifetime_reuse_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page bad_prefetch_page = SWITCH1;
    const enum test_page good_alias_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x680U;
    const uintptr_t bad_prefetch_vaddr =
        hs_page_base(bad_prefetch_page) + 0x680U;
    const uintptr_t good_alias_vaddr =
        hs_page_base(good_alias_page) + 0x680U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x680U;
    const uint64_t seed0 = 0x0a0b0c0d10111213ULL;
    const uint64_t seed1 = 0x2021222324252627ULL;
    const uint64_t guard_seed = 0x3031323334353637ULL;
    const uint64_t store0_value = 0xbad0bad0125a0000ULL;
    const uint64_t store1_value = 0xbad1bad1125b1111ULL;
    uint64_t first_observed = 0ULL;
    uint64_t after_store0_observed = 0ULL;
    uint64_t before_store1_observed = 0ULL;
    uint64_t after_store1_observed = 0ULL;
    uint64_t guard_observed = 0ULL;
    uint64_t remap_observed = 0ULL;

    goto_priv(PRIV_M);
    write64(base_paddr, seed0);
    write64(base_paddr + sizeof(uint64_t), seed1);
    write64(base_paddr + 2U * sizeof(uint64_t), guard_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lsq_smoke_map_alias_to_base_high_ppn(bad_prefetch_page, base_page);
    ai_lsq_smoke_map_alias_to_base(good_alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_repeated_prefetch_bad_alias_then_demand_reuse_window(
        bad_prefetch_vaddr, good_alias_vaddr, base_vaddr,
        store0_value, store1_value, &first_observed,
        &after_store0_observed, &before_store1_observed,
        &after_store1_observed, &guard_observed);
    TEST_ASSERT("AP-125 lifetime smoke: repeated software prefetches through a faulting high-PPN alias stay ignored while real same-line demand loads and stores observe only ordinary demand state",
        excpt.triggered == false &&
        first_observed == seed0 &&
        after_store0_observed == store0_value &&
        before_store1_observed == seed1 &&
        after_store1_observed == store1_value &&
        guard_observed == guard_seed,
        "first=0x%lx seed0=0x%lx after0=0x%lx store0=0x%lx before1=0x%lx seed1=0x%lx after1=0x%lx store1=0x%lx guard=0x%lx guard_seed=0x%lx",
        first_observed, seed0, after_store0_observed, store0_value,
        before_store1_observed, seed1, after_store1_observed, store1_value,
        guard_observed, guard_seed
    );

    TEST_SETUP_EXCEPT();
    ld(bad_prefetch_vaddr);
    TEST_ASSERT("AP-125 lifetime smoke confirms a real demand load to the same high-PPN alias still raises access fault after repeated silent prefetches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_prefetch_vaddr,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx",
        excpt.cause, excpt.tval, bad_prefetch_vaddr
    );

    goto_priv(PRIV_M);
    ai_lsq_smoke_map_alias_to_base(bad_prefetch_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    remap_observed = ld(bad_prefetch_vaddr);
    TEST_ASSERT("AP-125 lifetime smoke remaps the former faulting prefetch alias to a valid demand-load alias and observes the latest real store value",
        excpt.triggered == false &&
        remap_observed == store0_value,
        "remap=0x%lx expected=0x%lx",
        remap_observed, store0_value
    );

    goto_priv(PRIV_M);
    ai_lsq_smoke_map_alias_to_base_high_ppn(bad_prefetch_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x13, 0x6, x0, %[bad_prefetch], 1\n\t"
        ".option pop\n\t"
        :
        : [bad_prefetch] "r"(bad_prefetch_vaddr)
        : "memory");
    TEST_ASSERT("AP-125 lifetime smoke reuses the alias as a software prefetch after remapping it back to high-PPN faulting form without raising a visible exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(bad_prefetch_vaddr);
    TEST_ASSERT("AP-125 lifetime smoke checks that reused high-PPN demand access is freshly judged as a real load and is not hidden by stale prefetch ignore state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_prefetch_vaddr,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx",
        excpt.cause, excpt.tval, bad_prefetch_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-125 lifetime smoke leaves backing memory with only the two explicit demand stores and an unchanged guard word; RTL closure still needs ignoreRARCheck lifetime waveform checks",
        read64(base_paddr) == store0_value &&
        read64(base_paddr + sizeof(uint64_t)) == store1_value &&
        read64(base_paddr + 2U * sizeof(uint64_t)) == guard_seed,
        "word0=0x%lx store0=0x%lx word1=0x%lx store1=0x%lx guard=0x%lx guard_seed=0x%lx",
        read64(base_paddr), store0_value,
        read64(base_paddr + sizeof(uint64_t)), store1_value,
        read64(base_paddr + 2U * sizeof(uint64_t)), guard_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap125_prefetch_then_same_line_demand_load_rar_ignore_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page bad_prefetch_page = SWITCH1;
    const enum test_page good_alias_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x580U;
    const uintptr_t bad_prefetch_vaddr =
        hs_page_base(bad_prefetch_page) + 0x580U;
    const uintptr_t good_alias_vaddr =
        hs_page_base(good_alias_page) + 0x580U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x580U;
    const uint64_t seed_value = 0x1122334455667788ULL;
    const uint64_t store_value = 0xa5a55a5adeadc0deULL;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    uint64_t first_observed = 0ULL;
    uint64_t after_store_observed = 0ULL;
    uint64_t adjacent_observed = 0ULL;

    goto_priv(PRIV_M);
    write64(base_paddr, seed_value);
    write64(base_paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lsq_smoke_map_alias_to_base_high_ppn(bad_prefetch_page, base_page);
    ai_lsq_smoke_map_alias_to_base(good_alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_prefetch_bad_then_same_line_demand_loads(
        bad_prefetch_vaddr, good_alias_vaddr, base_vaddr, store_value,
        &first_observed, &after_store_observed, &adjacent_observed);
    TEST_ASSERT("AP-125 manual smoke: a software prefetch through a high-PPN same-line alias stays ignored by the later real demand load/store window instead of poisoning RAR or rollback state",
        excpt.triggered == false &&
        first_observed == seed_value &&
        after_store_observed == store_value &&
        adjacent_observed == adjacent_seed,
        "first=0x%lx expected=0x%lx after=0x%lx expected_after=0x%lx adjacent=0x%lx expected_adj=0x%lx",
        first_observed, seed_value, after_store_observed, store_value,
        adjacent_observed, adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    ld(bad_prefetch_vaddr);
    TEST_ASSERT("AP-125 manual smoke confirms the same high-PPN alias still reports access fault for a real demand load after the silent prefetch, so the prefetch did not publish a normal demand-load paddr state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_prefetch_vaddr,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx",
        excpt.cause, excpt.tval, bad_prefetch_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-125 manual smoke leaves only the explicit same-line demand store visible in backing memory and preserves the adjacent word",
        read64(base_paddr) == store_value &&
        read64(base_paddr + sizeof(uint64_t)) == adjacent_seed,
        "base=0x%lx expected=0x%lx adjacent=0x%lx expected_adj=0x%lx",
        read64(base_paddr), store_value,
        read64(base_paddr + sizeof(uint64_t)), adjacent_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap129_vluxei64_fault1_delayed_store_rawq_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x400U;
    const uintptr_t store_vaddr = base_vaddr;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x80U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x100U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x200U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x400U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x80U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x100U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x200U;
    const uint64_t old_value = 0x1111222233334444ULL;
    const uint64_t delayed_store_value = 0xaabbccddeeff0011ULL;
    const uint64_t fault_seed = 0x5566778899aabbccULL;
    const uint64_t dump_seed0 = 0x0102030405060708ULL;
    const uint64_t dump_seed1 = 0x8877665544332211ULL;
    const uint64_t offsets[2] = {
        0ULL,
        (uint64_t)(fault_vaddr - base_vaddr)
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    write64(base_paddr, old_value);
    write64(fault_paddr, fault_seed);
    write64(offsets_paddr, offsets[0]);
    write64(offsets_paddr + sizeof(uint64_t), offsets[1]);
    write64(dump_paddr, dump_seed0);
    write64(dump_paddr + sizeof(uint64_t), dump_seed1);

    goto_priv(PRIV_HS);
    hspt_init();
    *ai_lsq_smoke_hs_leaf_entry(fault_page) =
        (phys_page_base(fault_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    sfence_vma();

    ai_lsq_smoke_enable_vector_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)dump_vaddr, 2);
    TEST_ASSERT("AP-129 smoke setup initializes v6 before the indexed vector-load precise-fault window",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_delayed_store_then_vluxei64_fault1(store_vaddr,
        base_vaddr, offsets_vaddr, delayed_store_value);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-129 manual smoke: a delayed older scalar store before vluxei64 plus element1 page fault reports a precise vector load fault at vstart=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-129 manual smoke confirms the older delayed scalar store itself committed before inspecting the vector prefix element",
        read64(base_paddr) == delayed_store_value &&
        read64(fault_paddr) == fault_seed
    );

    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)dump_vaddr, 2);
    TEST_ASSERT("AP-129 manual smoke can dump the vector destination after clearing vstart",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-129 manual smoke expects the pre-fault indexed-load element0 to reflect the older store value; RAWQ lifetime closure still requires LoadQueueRAW vec feedback waveform checks",
        read64(dump_paddr) == delayed_store_value &&
        read64(dump_paddr + sizeof(uint64_t)) == dump_seed1,
        "dump0=0x%lx expected=0x%lx dump1=0x%lx seed1=0x%lx",
        read64(dump_paddr), delayed_store_value,
        read64(dump_paddr + sizeof(uint64_t)), dump_seed1
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap122_mab_split_alias_reuse_no_paddr_pollution_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base0_page = VSRWX_GRWX;
    const enum test_page base1_page = SCRATCHPAD;
    const enum test_page older_alias_page = SWITCH1;
    const enum test_page younger_alias_page = SWITCH2;
    const uintptr_t line_offset = 0x500U;
    const uintptr_t older_vaddr =
        hs_page_base(older_alias_page) + line_offset + 0x0fU;
    const uintptr_t younger_vaddr =
        hs_page_base(younger_alias_page) + line_offset + 0x10U;
    const uintptr_t base0_vaddr = hs_page_base(base0_page) + line_offset;
    const uintptr_t base1_vaddr = hs_page_base(base1_page) + line_offset;
    const uintptr_t base0_paddr = phys_page_base(base0_page) + line_offset;
    const uintptr_t base1_paddr = phys_page_base(base1_page) + line_offset;
    const uint64_t base0_poison = 0xdeaddead12201220ULL;
    uint8_t line0[64];
    uint8_t line1[64];
    uint64_t older0 = 0ULL;
    uint64_t younger0 = 0ULL;
    uint64_t nearby0 = 0ULL;
    uint64_t older1 = 0ULL;
    uint64_t younger1 = 0ULL;
    uint64_t nearby1 = 0ULL;

    for (unsigned i = 0; i < sizeof(line0); ++i) {
        line0[i] = (uint8_t)(0x40U + i);
        line1[i] = (uint8_t)(0xa0U + ((i * 3U) & 0x3fU));
    }

    goto_priv(PRIV_M);
    ai_lsq_smoke_write_bytes(base0_paddr, line0, sizeof(line0));
    ai_lsq_smoke_write_bytes(base1_paddr, line1, sizeof(line1));

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lsq_smoke_map_alias_to_base(older_alias_page, base0_page);
    ai_lsq_smoke_map_alias_to_base(younger_alias_page, base0_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed0 = ld(base0_vaddr + 0x10U);
    TEST_ASSERT("AP-122 P122B smoke phase0 warms the first physical line before exercising the MAB split alias path",
        excpt.triggered == false &&
        warmed0 == ai_lsq_smoke_le64(&line0[0x10U])
    );

    ai_lsq_smoke_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    cbo_inval(base0_vaddr + 0x10U);
    TEST_ASSERT("AP-122 P122B smoke phase0 invalidates the first physical line without taking an exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_delayed_older_load_then_two_loads(older_vaddr,
        younger_vaddr, base1_vaddr, &older0, &younger0, &nearby0);
    TEST_ASSERT("AP-122 P122B smoke phase0 reads the first alias target correctly through an older non-aligned MAB split and a younger same-line load",
        excpt.triggered == false &&
        older0 == ai_lsq_smoke_le64(&line0[0x0fU]) &&
        younger0 == ai_lsq_smoke_le64(&line0[0x10U]) &&
        nearby0 == ai_lsq_smoke_le64(&line1[0x00U]),
        "older0=0x%lx expected=0x%lx younger0=0x%lx expected_y=0x%lx nearby0=0x%lx",
        older0, ai_lsq_smoke_le64(&line0[0x0fU]),
        younger0, ai_lsq_smoke_le64(&line0[0x10U]), nearby0
    );

    goto_priv(PRIV_M);
    write64(base0_paddr + 0x10U, base0_poison);
    ai_lsq_smoke_map_alias_to_base(older_alias_page, base1_page);
    ai_lsq_smoke_map_alias_to_base(younger_alias_page, base1_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed1 = ld(base1_vaddr + 0x10U);
    TEST_ASSERT("AP-122 P122B smoke phase1 remaps the same alias pages to a second physical line and observes the new backing image",
        excpt.triggered == false &&
        warmed1 == ai_lsq_smoke_le64(&line1[0x10U])
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base1_vaddr + 0x10U);
    TEST_ASSERT("AP-122 P122B smoke phase1 invalidates the second physical line before reusing the same alias virtual addresses",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_lsq_smoke_delayed_older_load_then_two_loads(older_vaddr,
        younger_vaddr, base0_vaddr + 0x10U, &older1, &younger1, &nearby1);
    TEST_ASSERT("AP-122 P122B smoke phase1 reuses the same non-aligned alias VAs after remapping and still reads only the second physical line, not stale paddr/release state from phase0",
        excpt.triggered == false &&
        older1 == ai_lsq_smoke_le64(&line1[0x0fU]) &&
        younger1 == ai_lsq_smoke_le64(&line1[0x10U]) &&
        nearby1 == base0_poison,
        "older1=0x%lx expected=0x%lx younger1=0x%lx expected_y=0x%lx nearby1=0x%lx",
        older1, ai_lsq_smoke_le64(&line1[0x0fU]),
        younger1, ai_lsq_smoke_le64(&line1[0x10U]), nearby1
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-122 P122B smoke keeps both backing lines coherent with only the explicit poison write in phase0; internal closure still requires updatePaddrValid and paddr/release waveform checks",
        read64(base0_paddr + 0x10U) == base0_poison &&
        ai_lsq_smoke_bytes_match(base1_paddr, line1, sizeof(line1)),
        "base0_word=0x%lx expected=0x%lx",
        read64(base0_paddr + 0x10U), base0_poison
    );

    ai_lsq_smoke_restore_hs_cbo_inval();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
