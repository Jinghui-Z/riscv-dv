#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_VDEP_PMP_TOR_RWX 0x0fULL
#define AI_VDEP_PMP_TOR_NO_R 0x0cULL
#define AI_VDEP_ELEMS 4U
#define AI_VDEP_RAW_PRESSURE_LOADS 48U
#define AI_VDEP_RAW_ADDR_CHAIN_LINKS 8U
#define AI_VDEP_RAW_ADDR_CHAIN_STRIDE 0x40U
#define AI_VDEP_ESCAPE_MIN_PRESSURE_LOADS 24U
#define AI_VDEP_ESCAPE_MIN_ADDR_CHAIN_LINKS 4U

static inline pte_t *ai_vdep_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_vdep_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ai_vdep_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_vdep_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPADDR3, 0ULL);
    CSRW(CSR_PMPADDR4, 0ULL);
    CSRW(CSR_PMPCFG0, AI_VDEP_PMP_TOR_RWX);
}

static inline void ai_vdep_configure_pmp_no_read_page(uintptr_t deny_paddr)
{
    const uintptr_t deny_start = deny_paddr & PAGE_ADDR_MSK;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_VDEP_PMP_TOR_RWX |
        (AI_VDEP_PMP_TOR_NO_R << 8) | (AI_VDEP_PMP_TOR_RWX << 16));
}

static inline void ai_vdep_configure_pmp_no_read_two_pages(uintptr_t first_paddr,
    uintptr_t second_paddr)
{
    uintptr_t low_start = first_paddr & PAGE_ADDR_MSK;
    uintptr_t high_start = second_paddr & PAGE_ADDR_MSK;

    if (high_start < low_start) {
        uintptr_t tmp = low_start;
        low_start = high_start;
        high_start = tmp;
    }

    const uintptr_t low_end = low_start + PAGE_SIZE;
    const uintptr_t high_end = high_start + PAGE_SIZE;

    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, low_start >> 2);
    CSRW(CSR_PMPADDR1, low_end >> 2);
    CSRW(CSR_PMPADDR2, high_start >> 2);
    CSRW(CSR_PMPADDR3, high_end >> 2);
    CSRW(CSR_PMPADDR4, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_VDEP_PMP_TOR_RWX |
        (AI_VDEP_PMP_TOR_NO_R << 8) |
        (AI_VDEP_PMP_TOR_RWX << 16) |
        (AI_VDEP_PMP_TOR_NO_R << 24) |
        (AI_VDEP_PMP_TOR_RWX << 32));
}

static inline void ai_vdep_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
}

static inline void ai_vdep_load_index_offsets_to_v8(const uint64_t *offsets)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        ".option pop\n\t"
        :
        : [offsets] "r"(offsets),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "memory");
}

static inline void ai_vdep_load_sentinel_to_v6(const uint64_t *src)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "memory");
}

static inline uint64_t ai_vdep_load_store_vload_flow(uintptr_t at_addr,
    uintptr_t hash_addr, uintptr_t vector_base, const uint64_t *sentinel_src,
    const uint64_t *offset_src)
{
    uint64_t at_value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[sentinel_src])\n\t"
        "vle64.v v8, (%[offset_src])\n\t"
        "ld %[at_value], 0(%[at_addr])\n\t"
        "sd %[at_value], 0(%[hash_addr])\n\t"
        "vloxei64.v v6, (%[vector_base]), v8\n\t"
        ".option pop\n\t"
        : [at_value] "=&r"(at_value)
        : [at_addr] "r"(at_addr),
          [hash_addr] "r"(hash_addr),
          [vector_base] "r"(vector_base),
          [sentinel_src] "r"(sentinel_src),
          [offset_src] "r"(offset_src),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "memory");

    return at_value;
}

static inline uint64_t ai_vdep_load_store_addr_dep_vload_flow(uintptr_t at_addr,
    uint64_t hash_data, uintptr_t vector_base, const uint64_t *sentinel_src,
    const uint64_t *offset_src)
{
    uint64_t at_value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[sentinel_src])\n\t"
        "vle64.v v8, (%[offset_src])\n\t"
        "ld %[at_value], 0(%[at_addr])\n\t"
        "sd %[hash_data], 0(%[at_value])\n\t"
        "vloxei64.v v6, (%[vector_base]), v8\n\t"
        ".option pop\n\t"
        : [at_value] "=&r"(at_value)
        : [at_addr] "r"(at_addr),
          [hash_data] "r"(hash_data),
          [vector_base] "r"(vector_base),
          [sentinel_src] "r"(sentinel_src),
          [offset_src] "r"(offset_src),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "memory");

    return at_value;
}

static inline uint64_t ai_vdep_chain_store_addr_dep_vload_flow(uintptr_t at_addr,
    uint64_t hash_data, uintptr_t vector_base, const uint64_t *sentinel_src,
    const uint64_t *offset_src, const uint64_t *pressure_src)
{
    uint64_t store_target = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[sentinel_src])\n\t"
        "vle64.v v8, (%[offset_src])\n\t"
        "ld %[store_target], 0(%[at_addr])\n\t"
        "ld t1, 0(%[pressure_src])\n\t"
        "ld t2, 8(%[pressure_src])\n\t"
        "ld t3, 16(%[pressure_src])\n\t"
        "ld t4, 24(%[pressure_src])\n\t"
        "ld t5, 32(%[pressure_src])\n\t"
        "ld t6, 40(%[pressure_src])\n\t"
        "ld t1, 48(%[pressure_src])\n\t"
        "ld t2, 56(%[pressure_src])\n\t"
        "ld t3, 64(%[pressure_src])\n\t"
        "ld t4, 72(%[pressure_src])\n\t"
        "ld t5, 80(%[pressure_src])\n\t"
        "ld t6, 88(%[pressure_src])\n\t"
        "ld t1, 96(%[pressure_src])\n\t"
        "ld t2, 104(%[pressure_src])\n\t"
        "ld t3, 112(%[pressure_src])\n\t"
        "ld t4, 120(%[pressure_src])\n\t"
        "ld t5, 128(%[pressure_src])\n\t"
        "ld t6, 136(%[pressure_src])\n\t"
        "ld t1, 144(%[pressure_src])\n\t"
        "ld t2, 152(%[pressure_src])\n\t"
        "ld t3, 160(%[pressure_src])\n\t"
        "ld t4, 168(%[pressure_src])\n\t"
        "ld t5, 176(%[pressure_src])\n\t"
        "ld t6, 184(%[pressure_src])\n\t"
        "ld t1, 192(%[pressure_src])\n\t"
        "ld t2, 200(%[pressure_src])\n\t"
        "ld t3, 208(%[pressure_src])\n\t"
        "ld t4, 216(%[pressure_src])\n\t"
        "ld t5, 224(%[pressure_src])\n\t"
        "ld t6, 232(%[pressure_src])\n\t"
        "ld t1, 240(%[pressure_src])\n\t"
        "ld t2, 248(%[pressure_src])\n\t"
        "ld t3, 256(%[pressure_src])\n\t"
        "ld t4, 264(%[pressure_src])\n\t"
        "ld t5, 272(%[pressure_src])\n\t"
        "ld t6, 280(%[pressure_src])\n\t"
        "ld t1, 288(%[pressure_src])\n\t"
        "ld t2, 296(%[pressure_src])\n\t"
        "ld t3, 304(%[pressure_src])\n\t"
        "ld t4, 312(%[pressure_src])\n\t"
        "ld t5, 320(%[pressure_src])\n\t"
        "ld t6, 328(%[pressure_src])\n\t"
        "ld t1, 336(%[pressure_src])\n\t"
        "ld t2, 344(%[pressure_src])\n\t"
        "ld t3, 352(%[pressure_src])\n\t"
        "ld t4, 360(%[pressure_src])\n\t"
        "ld t5, 368(%[pressure_src])\n\t"
        "ld t6, 376(%[pressure_src])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "sd %[hash_data], 0(%[store_target])\n\t"
        "vloxei64.v v6, (%[vector_base]), v8\n\t"
        ".option pop\n\t"
        : [store_target] "=&r"(store_target)
        : [at_addr] "r"(at_addr),
          [hash_data] "r"(hash_data),
          [vector_base] "r"(vector_base),
          [sentinel_src] "r"(sentinel_src),
          [offset_src] "r"(offset_src),
          [pressure_src] "r"(pressure_src),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    return store_target;
}

static inline uint64_t ai_vdep_chain_store_addr_dep_vload_landing_dump_flow(
    uintptr_t at_addr, uint64_t hash_data, uintptr_t vector_base,
    const uint64_t *sentinel_src, const uint64_t *offset_src,
    const uint64_t *pressure_src, uint64_t *dump_dest, uint64_t *vstart_after)
{
    uint64_t store_target = 0ULL;
    uint64_t captured_vstart = ~0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[sentinel_src])\n\t"
        "vle64.v v8, (%[offset_src])\n\t"
        "ld %[store_target], 0(%[at_addr])\n\t"
        "ld t1, 0(%[pressure_src])\n\t"
        "ld t2, 8(%[pressure_src])\n\t"
        "ld t3, 16(%[pressure_src])\n\t"
        "ld t4, 24(%[pressure_src])\n\t"
        "ld t5, 32(%[pressure_src])\n\t"
        "ld t6, 40(%[pressure_src])\n\t"
        "ld t1, 48(%[pressure_src])\n\t"
        "ld t2, 56(%[pressure_src])\n\t"
        "ld t3, 64(%[pressure_src])\n\t"
        "ld t4, 72(%[pressure_src])\n\t"
        "ld t5, 80(%[pressure_src])\n\t"
        "ld t6, 88(%[pressure_src])\n\t"
        "ld t1, 96(%[pressure_src])\n\t"
        "ld t2, 104(%[pressure_src])\n\t"
        "ld t3, 112(%[pressure_src])\n\t"
        "ld t4, 120(%[pressure_src])\n\t"
        "ld t5, 128(%[pressure_src])\n\t"
        "ld t6, 136(%[pressure_src])\n\t"
        "ld t1, 144(%[pressure_src])\n\t"
        "ld t2, 152(%[pressure_src])\n\t"
        "ld t3, 160(%[pressure_src])\n\t"
        "ld t4, 168(%[pressure_src])\n\t"
        "ld t5, 176(%[pressure_src])\n\t"
        "ld t6, 184(%[pressure_src])\n\t"
        "ld t1, 192(%[pressure_src])\n\t"
        "ld t2, 200(%[pressure_src])\n\t"
        "ld t3, 208(%[pressure_src])\n\t"
        "ld t4, 216(%[pressure_src])\n\t"
        "ld t5, 224(%[pressure_src])\n\t"
        "ld t6, 232(%[pressure_src])\n\t"
        "ld t1, 240(%[pressure_src])\n\t"
        "ld t2, 248(%[pressure_src])\n\t"
        "ld t3, 256(%[pressure_src])\n\t"
        "ld t4, 264(%[pressure_src])\n\t"
        "ld t5, 272(%[pressure_src])\n\t"
        "ld t6, 280(%[pressure_src])\n\t"
        "ld t1, 288(%[pressure_src])\n\t"
        "ld t2, 296(%[pressure_src])\n\t"
        "ld t3, 304(%[pressure_src])\n\t"
        "ld t4, 312(%[pressure_src])\n\t"
        "ld t5, 320(%[pressure_src])\n\t"
        "ld t6, 328(%[pressure_src])\n\t"
        "ld t1, 336(%[pressure_src])\n\t"
        "ld t2, 344(%[pressure_src])\n\t"
        "ld t3, 352(%[pressure_src])\n\t"
        "ld t4, 360(%[pressure_src])\n\t"
        "ld t5, 368(%[pressure_src])\n\t"
        "ld t6, 376(%[pressure_src])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "sd %[hash_data], 0(%[store_target])\n\t"
        "vloxei64.v v6, (%[vector_base]), v8\n\t"
        "csrr %[captured_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vse64.v v6, (%[dump_dest])\n\t"
        ".option pop\n\t"
        : [store_target] "=&r"(store_target),
          [captured_vstart] "=&r"(captured_vstart)
        : [at_addr] "r"(at_addr),
          [hash_data] "r"(hash_data),
          [vector_base] "r"(vector_base),
          [sentinel_src] "r"(sentinel_src),
          [offset_src] "r"(offset_src),
          [pressure_src] "r"(pressure_src),
          [dump_dest] "r"(dump_dest),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    *vstart_after = captured_vstart;
    return store_target;
}

static inline uint64_t ai_vdep_chain_store_addr_dep_vload_min_landing_dump_flow(
    uintptr_t at_addr, uint64_t hash_data, uintptr_t vector_base,
    const uint64_t *sentinel_src, const uint64_t *offset_src,
    const uint64_t *pressure_src, uint64_t *dump_dest, uint64_t *vstart_after)
{
    uint64_t store_target = 0ULL;
    uint64_t captured_vstart = ~0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[sentinel_src])\n\t"
        "vle64.v v8, (%[offset_src])\n\t"
        "ld %[store_target], 0(%[at_addr])\n\t"
        "ld t1, 0(%[pressure_src])\n\t"
        "ld t2, 8(%[pressure_src])\n\t"
        "ld t3, 16(%[pressure_src])\n\t"
        "ld t4, 24(%[pressure_src])\n\t"
        "ld t5, 32(%[pressure_src])\n\t"
        "ld t6, 40(%[pressure_src])\n\t"
        "ld t1, 48(%[pressure_src])\n\t"
        "ld t2, 56(%[pressure_src])\n\t"
        "ld t3, 64(%[pressure_src])\n\t"
        "ld t4, 72(%[pressure_src])\n\t"
        "ld t5, 80(%[pressure_src])\n\t"
        "ld t6, 88(%[pressure_src])\n\t"
        "ld t1, 96(%[pressure_src])\n\t"
        "ld t2, 104(%[pressure_src])\n\t"
        "ld t3, 112(%[pressure_src])\n\t"
        "ld t4, 120(%[pressure_src])\n\t"
        "ld t5, 128(%[pressure_src])\n\t"
        "ld t6, 136(%[pressure_src])\n\t"
        "ld t1, 144(%[pressure_src])\n\t"
        "ld t2, 152(%[pressure_src])\n\t"
        "ld t3, 160(%[pressure_src])\n\t"
        "ld t4, 168(%[pressure_src])\n\t"
        "ld t5, 176(%[pressure_src])\n\t"
        "ld t6, 184(%[pressure_src])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "ld %[store_target], 0(%[store_target])\n\t"
        "sd %[hash_data], 0(%[store_target])\n\t"
        "vloxei64.v v6, (%[vector_base]), v8\n\t"
        "csrr %[captured_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vse64.v v6, (%[dump_dest])\n\t"
        ".option pop\n\t"
        : [store_target] "=&r"(store_target),
          [captured_vstart] "=&r"(captured_vstart)
        : [at_addr] "r"(at_addr),
          [hash_data] "r"(hash_data),
          [vector_base] "r"(vector_base),
          [sentinel_src] "r"(sentinel_src),
          [offset_src] "r"(offset_src),
          [pressure_src] "r"(pressure_src),
          [dump_dest] "r"(dump_dest),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    *vstart_after = captured_vstart;
    return store_target;
}

static inline uint64_t ai_vdep_pressure_raw_store_addr_dep_vload_flow(uintptr_t at_addr,
    uintptr_t side_at_addr0, uintptr_t side_at_addr1, uint64_t hash_data,
    uint64_t side_hash0, uint64_t side_hash1, uintptr_t vector_base,
    const uint64_t *sentinel_src, const uint64_t *offset_src,
    const uint64_t *raw_probe_src)
{
    uint64_t at_value = 0ULL;
    uint64_t side_value0 = 0ULL;
    uint64_t side_value1 = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[sentinel_src])\n\t"
        "vle64.v v8, (%[offset_src])\n\t"
        "ld %[at_value], 0(%[at_addr])\n\t"
        "ld %[side_value0], 0(%[side_at_addr0])\n\t"
        "ld %[side_value1], 0(%[side_at_addr1])\n\t"
        "ld %[at_value], 0(%[at_value])\n\t"
        "ld %[side_value0], 0(%[side_value0])\n\t"
        "ld %[side_value1], 0(%[side_value1])\n\t"
        "ld %[at_value], 0(%[at_value])\n\t"
        "ld %[side_value0], 0(%[side_value0])\n\t"
        "ld %[side_value1], 0(%[side_value1])\n\t"
        "ld %[at_value], 0(%[at_value])\n\t"
        "ld %[side_value0], 0(%[side_value0])\n\t"
        "ld %[side_value1], 0(%[side_value1])\n\t"
        "ld %[at_value], 0(%[at_value])\n\t"
        "ld %[side_value0], 0(%[side_value0])\n\t"
        "ld %[side_value1], 0(%[side_value1])\n\t"
        "ld %[at_value], 0(%[at_value])\n\t"
        "ld %[side_value0], 0(%[side_value0])\n\t"
        "ld %[side_value1], 0(%[side_value1])\n\t"
        "ld %[at_value], 0(%[at_value])\n\t"
        "ld %[side_value0], 0(%[side_value0])\n\t"
        "ld %[side_value1], 0(%[side_value1])\n\t"
        "ld %[at_value], 0(%[at_value])\n\t"
        "ld %[side_value0], 0(%[side_value0])\n\t"
        "ld %[side_value1], 0(%[side_value1])\n\t"
        "ld %[at_value], 0(%[at_value])\n\t"
        "ld %[side_value0], 0(%[side_value0])\n\t"
        "ld %[side_value1], 0(%[side_value1])\n\t"
        "sd %[side_hash0], 0(%[side_value0])\n\t"
        "sd %[side_hash1], 0(%[side_value1])\n\t"
        "ld t1, 0(%[raw_probe_src])\n\t"
        "ld t2, 8(%[raw_probe_src])\n\t"
        "ld t3, 16(%[raw_probe_src])\n\t"
        "ld t4, 24(%[raw_probe_src])\n\t"
        "ld t5, 32(%[raw_probe_src])\n\t"
        "ld t6, 40(%[raw_probe_src])\n\t"
        "ld t1, 48(%[raw_probe_src])\n\t"
        "ld t2, 56(%[raw_probe_src])\n\t"
        "ld t3, 64(%[raw_probe_src])\n\t"
        "ld t4, 72(%[raw_probe_src])\n\t"
        "ld t5, 80(%[raw_probe_src])\n\t"
        "ld t6, 88(%[raw_probe_src])\n\t"
        "ld t1, 96(%[raw_probe_src])\n\t"
        "ld t2, 104(%[raw_probe_src])\n\t"
        "ld t3, 112(%[raw_probe_src])\n\t"
        "ld t4, 120(%[raw_probe_src])\n\t"
        "ld t5, 128(%[raw_probe_src])\n\t"
        "ld t6, 136(%[raw_probe_src])\n\t"
        "ld t1, 144(%[raw_probe_src])\n\t"
        "ld t2, 152(%[raw_probe_src])\n\t"
        "ld t3, 160(%[raw_probe_src])\n\t"
        "ld t4, 168(%[raw_probe_src])\n\t"
        "ld t5, 176(%[raw_probe_src])\n\t"
        "ld t6, 184(%[raw_probe_src])\n\t"
        "ld t1, 192(%[raw_probe_src])\n\t"
        "ld t2, 200(%[raw_probe_src])\n\t"
        "ld t3, 208(%[raw_probe_src])\n\t"
        "ld t4, 216(%[raw_probe_src])\n\t"
        "ld t5, 224(%[raw_probe_src])\n\t"
        "ld t6, 232(%[raw_probe_src])\n\t"
        "ld t1, 240(%[raw_probe_src])\n\t"
        "ld t2, 248(%[raw_probe_src])\n\t"
        "sd %[hash_data], 0(%[at_value])\n\t"
        "vloxei64.v v6, (%[vector_base]), v8\n\t"
        "ld t3, 256(%[raw_probe_src])\n\t"
        "ld t4, 264(%[raw_probe_src])\n\t"
        "ld t5, 272(%[raw_probe_src])\n\t"
        "ld t6, 280(%[raw_probe_src])\n\t"
        "ld t1, 288(%[raw_probe_src])\n\t"
        "ld t2, 296(%[raw_probe_src])\n\t"
        "ld t3, 304(%[raw_probe_src])\n\t"
        "ld t4, 312(%[raw_probe_src])\n\t"
        "ld t5, 320(%[raw_probe_src])\n\t"
        "ld t6, 328(%[raw_probe_src])\n\t"
        "ld t1, 336(%[raw_probe_src])\n\t"
        "ld t2, 344(%[raw_probe_src])\n\t"
        "ld t3, 352(%[raw_probe_src])\n\t"
        "ld t4, 360(%[raw_probe_src])\n\t"
        "ld t5, 368(%[raw_probe_src])\n\t"
        "ld t6, 376(%[raw_probe_src])\n\t"
        ".option pop\n\t"
        : [at_value] "=&r"(at_value),
          [side_value0] "=&r"(side_value0),
          [side_value1] "=&r"(side_value1)
        : [at_addr] "r"(at_addr),
          [side_at_addr0] "r"(side_at_addr0),
          [side_at_addr1] "r"(side_at_addr1),
          [hash_data] "r"(hash_data),
          [side_hash0] "r"(side_hash0),
          [side_hash1] "r"(side_hash1),
          [vector_base] "r"(vector_base),
          [sentinel_src] "r"(sentinel_src),
          [offset_src] "r"(offset_src),
          [raw_probe_src] "r"(raw_probe_src),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    return at_value;
}

static inline void ai_vdep_indexed_vload_v6_m2_flow(uintptr_t vector_base,
    const uint64_t *sentinel_src, const uint64_t *offset_src)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[sentinel_src])\n\t"
        "vle64.v v8, (%[offset_src])\n\t"
        "vloxei64.v v6, (%[vector_base]), v8\n\t"
        ".option pop\n\t"
        :
        : [vector_base] "r"(vector_base),
          [sentinel_src] "r"(sentinel_src),
          [offset_src] "r"(offset_src),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "memory");
}

static inline uintptr_t ai_vdep_indexed_vload_v6_m2_capture_pc_flow(
    uintptr_t vector_base, const uint64_t *sentinel_src,
    const uint64_t *offset_src)
{
    uintptr_t vector_pc = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vle64.v v6, (%[sentinel_src])\n\t"
        "vle64.v v8, (%[offset_src])\n\t"
        "la %[vector_pc], 1f\n\t"
        "1:\n\t"
        "vloxei64.v v6, (%[vector_base]), v8\n\t"
        ".option pop\n\t"
        : [vector_pc] "=&r"(vector_pc)
        : [vector_base] "r"(vector_base),
          [sentinel_src] "r"(sentinel_src),
          [offset_src] "r"(offset_src),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "memory");

    return vector_pc;
}

static inline void ai_vdep_store_v6_m2(uint64_t *dest)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "vse64.v v6, (%[dest])\n\t"
        ".option pop\n\t"
        :
        : [dest] "r"(dest),
          [vl] "r"((int)AI_VDEP_ELEMS)
        : "t0", "memory");
}

bool ai_micro_hs_load_store_then_vloxei_fault_dep_flow_corner()
{
    TEST_START();

    const enum test_page shared_page = VSRWX_GRWX;
    const enum test_page elem0_page = SWITCH1;
    const enum test_page elem2_page = SWITCH2;
    const enum test_page elem3_page = SCRATCHPAD;
    const enum test_page scratch_page = VSRW_GRWX;
    const uintptr_t vector_base = hs_page_base(shared_page);
    const uintptr_t at_vaddr = vector_base + 0x100U;
    const uintptr_t elem1_vaddr = vector_base + 0x240U;
    const uintptr_t elem0_vaddr = hs_page_base(elem0_page) + 0x140U;
    const uintptr_t elem2_vaddr = hs_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_vaddr = hs_page_base(elem3_page) + 0x1c0U;
    const uintptr_t at_paddr = phys_page_base(shared_page) + 0x100U;
    const uintptr_t elem1_paddr = phys_page_base(shared_page) + 0x240U;
    const uintptr_t elem0_paddr = phys_page_base(elem0_page) + 0x140U;
    const uintptr_t elem2_paddr = phys_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_paddr = phys_page_base(elem3_page) + 0x1c0U;
    const uintptr_t offsets_vaddr = hs_page_base(scratch_page) + 0x080U;
    const uintptr_t offsets_paddr = phys_page_base(scratch_page) + 0x080U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0x180U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0x180U;
    const uintptr_t addr_chain_vaddr = hs_page_base(scratch_page) + 0x300U;
    const uintptr_t addr_chain_paddr = phys_page_base(scratch_page) + 0x300U;
    const uintptr_t pressure_vaddr = hs_page_base(scratch_page) + 0x580U;
    const uintptr_t pressure_paddr = phys_page_base(scratch_page) + 0x580U;
    const uint64_t at_addr_seed = (uint64_t)addr_chain_vaddr;
    const uint64_t hash_data_seed = 0xa5a5a5a5deadbeefULL;
    const uint64_t elem0_seed = 0x0a0b0c0d0e0f1011ULL;
    const uint64_t elem1_old = 0x11100f0e0d0c0b0aULL;
    const uint64_t elem2_seed = 0x2021222324252627ULL;
    const uint64_t elem3_seed = 0x3031323334353637ULL;
    const uint64_t pressure_seed = 0x4142434445464700ULL;
    const uint64_t sentinel = 0xfeedfacecafebeefULL;
    uint64_t elem1_cache_warm = 0ULL;
    uint64_t warm0 = 0ULL;
    uint64_t warm2 = 0ULL;
    uint64_t warm3 = 0ULL;
    uint64_t at_observed = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool fault_triggered = false;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t dump0_observed = 0ULL;
    uint64_t dump1_observed = 0ULL;
    uint64_t elem1_backing_observed = 0ULL;
    uint64_t elem0_backing_observed = 0ULL;
    uint64_t elem2_backing_observed = 0ULL;
    uint64_t at_backing_observed = 0ULL;
    uint64_t elem3_backing_observed = 0ULL;
    uint64_t addr_chain0_observed = 0ULL;
    uint64_t addr_chain_tail_observed = 0ULL;

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    ai_vdep_enable_vector_state();
    write64(at_paddr, at_addr_seed);
    write64(elem0_paddr, elem0_seed);
    write64(elem1_paddr, elem1_old);
    write64(elem2_paddr, elem2_seed);
    write64(elem3_paddr, elem3_seed);
    write64(offsets_paddr + 0U * sizeof(uint64_t), elem0_vaddr - vector_base);
    write64(offsets_paddr + 1U * sizeof(uint64_t), elem1_vaddr - vector_base);
    write64(offsets_paddr + 2U * sizeof(uint64_t), elem2_vaddr - vector_base);
    write64(offsets_paddr + 3U * sizeof(uint64_t), elem3_vaddr - vector_base);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        const uintptr_t next = (i + 1U == AI_VDEP_RAW_ADDR_CHAIN_LINKS) ?
            elem1_vaddr : addr_chain_vaddr + (i + 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE;
        write64(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE, next);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; ++i) {
        write64(pressure_paddr + i * sizeof(uint64_t), pressure_seed + i);
    }
    for (unsigned i = 0; i < AI_VDEP_ELEMS; ++i) {
        write64(dump_paddr + i * sizeof(uint64_t), sentinel);
    }
    elem1_cache_warm = read64(elem1_paddr);
    cbo_flush(at_paddr);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        cbo_flush(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; i += 8U) {
        cbo_flush(pressure_paddr + i * sizeof(uint64_t));
    }
    fence_iorw_iorw();
    TEST_ASSERT("element1 backing line is warmed in DCache with the old value while 48 pressure-load slots are flushed before the directed vector element1 read",
        elem1_cache_warm == elem1_old
    );

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vdep_set_hs_leaf(shared_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem0_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem2_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem3_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_vdep_configure_pmp_no_read_two_pages(elem2_paddr, elem3_paddr);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    warm0 = ld(elem0_vaddr);
    TEST_ASSERT("element0 is translated and scalar-readable before the directed @/#/vector-load flow starts",
        excpt.triggered == false &&
        warm0 == elem0_seed
    );

    TEST_SETUP_EXCEPT();
    warm2 = ld(elem2_vaddr);
    TEST_ASSERT("element2 prewarm intentionally walks the access-fault path after its PMP no-read page is installed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == elem2_vaddr
    );

    TEST_SETUP_EXCEPT();
    warm3 = ld(elem3_vaddr);
    TEST_ASSERT("element3 prewarm intentionally walks the access-fault path after its PMP no-read page is installed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == elem3_vaddr
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_load_sentinel_to_v6((const uint64_t *)dump_vaddr);
    ai_vdep_load_index_offsets_to_v8((const uint64_t *)offsets_vaddr);
    TEST_ASSERT("the vector index register and destination sentinel can be prepared without touching the @ and element1 shared page",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    at_observed = ai_vdep_chain_store_addr_dep_vload_flow(at_vaddr, hash_data_seed,
        vector_base, (const uint64_t *)dump_vaddr,
        (const uint64_t *)offsets_vaddr, (const uint64_t *)pressure_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    TEST_ASSERT("the directed flow executes scalar load @, drains 48 flushed pressure loads, resolves # store address through the chain to elem1_vaddr, then indexed vector load sees element2 raise load access fault with vstart=2",
        at_observed == elem1_vaddr &&
        fault_triggered == true &&
        fault_cause == CAUSE_LAF &&
        fault_tval == elem2_vaddr &&
        observed_vstart == 2ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_store_v6_m2((uint64_t *)dump_vaddr);
    TEST_ASSERT("the vector destination can be dumped after clearing vstart",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    dump0_observed = read64(dump_paddr + 0U * sizeof(uint64_t));
    dump1_observed = read64(dump_paddr + 1U * sizeof(uint64_t));
    elem1_backing_observed = read64(elem1_paddr);
    elem0_backing_observed = read64(elem0_paddr);
    elem2_backing_observed = read64(elem2_paddr);
    at_backing_observed = read64(at_paddr);
    elem3_backing_observed = read64(elem3_paddr);
    addr_chain0_observed = read64(addr_chain_paddr);
    addr_chain_tail_observed = read64(addr_chain_paddr +
        (AI_VDEP_RAW_ADDR_CHAIN_LINKS - 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    TEST_ASSERT("vector dump element0 keeps the element0 seed before the element2 fault",
        dump0_observed == elem0_seed,
        "dump0_actual=0x%llx dump0_expected=0x%llx",
        (unsigned long long)dump0_observed,
        (unsigned long long)elem0_seed
    );
    TEST_ASSERT("vector dump element1 must see the dependent store # value (hash_data_seed) instead of the prewarmed elem1_old, because # is an address-dep store that targets elem1_paddr through a0=@'s output",
        dump1_observed == hash_data_seed,
        "dump1_actual=0x%llx expected_hash=0x%llx elem1_old=0x%llx",
        (unsigned long long)dump1_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)elem1_old
    );
    TEST_ASSERT("element1 backing memory is updated by the address-dep store # to hash_data_seed",
        elem1_backing_observed == hash_data_seed,
        "elem1_backing_actual=0x%llx expected_hash=0x%llx elem1_old=0x%llx",
        (unsigned long long)elem1_backing_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)elem1_old
    );
    TEST_ASSERT("element0 backing memory remains unchanged across the directed element2 fault",
        elem0_backing_observed == elem0_seed,
        "elem0_actual=0x%llx elem0_expected=0x%llx",
        (unsigned long long)elem0_backing_observed,
        (unsigned long long)elem0_seed
    );
    TEST_ASSERT("element2 backing memory remains unchanged across the directed element2 fault",
        elem2_backing_observed == elem2_seed,
        "elem2_actual=0x%llx elem2_expected=0x%llx",
        (unsigned long long)elem2_backing_observed,
        (unsigned long long)elem2_seed
    );
    TEST_ASSERT("@ backing memory keeps the predeposited pointer-chain head, the chain still resolves to elem1_vaddr, and element3 keeps its backing payload across the directed element2 fault",
        at_backing_observed == at_addr_seed &&
        addr_chain0_observed == addr_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE &&
        addr_chain_tail_observed == elem1_vaddr &&
        elem3_backing_observed == elem3_seed,
        "at_actual=0x%llx at_expected=0x%llx chain0=0x%llx chain_tail=0x%llx elem1_vaddr=0x%llx elem3_actual=0x%llx elem3_expected=0x%llx",
        (unsigned long long)at_backing_observed,
        (unsigned long long)at_addr_seed,
        (unsigned long long)addr_chain0_observed,
        (unsigned long long)addr_chain_tail_observed,
        (unsigned long long)elem1_vaddr,
        (unsigned long long)elem3_backing_observed,
        (unsigned long long)elem3_seed
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_load_store_then_vloxei_elem1_dmiss_replay_fault_corner()
{
    TEST_START();

    const enum test_page shared_page = VSRWX_GRWX;
    const enum test_page elem0_page = SWITCH1;
    const enum test_page elem2_page = SWITCH2;
    const enum test_page elem3_page = SCRATCHPAD;
    const enum test_page scratch_page = VSRW_GRWX;
    const uintptr_t vector_base = hs_page_base(shared_page);
    const uintptr_t at_vaddr = vector_base + 0x100U;
    const uintptr_t elem1_vaddr = vector_base + 0x240U;
    const uintptr_t elem0_vaddr = hs_page_base(elem0_page) + 0x140U;
    const uintptr_t elem2_vaddr = hs_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_vaddr = hs_page_base(elem3_page) + 0x1c0U;
    const uintptr_t at_paddr = phys_page_base(shared_page) + 0x100U;
    const uintptr_t elem1_paddr = phys_page_base(shared_page) + 0x240U;
    const uintptr_t elem0_paddr = phys_page_base(elem0_page) + 0x140U;
    const uintptr_t elem2_paddr = phys_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_paddr = phys_page_base(elem3_page) + 0x1c0U;
    const uintptr_t offsets_vaddr = hs_page_base(scratch_page) + 0x080U;
    const uintptr_t offsets_paddr = phys_page_base(scratch_page) + 0x080U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0x180U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0x180U;
    const uintptr_t addr_chain_vaddr = hs_page_base(scratch_page) + 0x300U;
    const uintptr_t addr_chain_paddr = phys_page_base(scratch_page) + 0x300U;
    const uintptr_t pressure_vaddr = hs_page_base(scratch_page) + 0x580U;
    const uintptr_t pressure_paddr = phys_page_base(scratch_page) + 0x580U;
    const uint64_t at_addr_seed = (uint64_t)addr_chain_vaddr;
    const uint64_t hash_data_seed = 0xd155c0decafef00dULL;
    const uint64_t elem0_seed = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t elem1_old = 0x1716151413121110ULL;
    const uint64_t elem2_seed = 0x2223242526272829ULL;
    const uint64_t elem3_seed = 0x333435363738393aULL;
    const uint64_t pressure_seed = 0x5152535455565700ULL;
    const uint64_t sentinel = 0xfeedfacecafebeefULL;
    uint64_t elem1_cache_prime = 0ULL;
    uint64_t warm0 = 0ULL;
    uint64_t warm2 = 0ULL;
    uint64_t warm3 = 0ULL;
    uint64_t at_observed = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool fault_triggered = false;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t dump0_observed = 0ULL;
    uint64_t dump1_observed = 0ULL;
    uint64_t elem1_backing_observed = 0ULL;
    uint64_t elem0_backing_observed = 0ULL;
    uint64_t elem2_backing_observed = 0ULL;
    uint64_t elem3_backing_observed = 0ULL;
    uint64_t at_backing_observed = 0ULL;
    uint64_t addr_chain0_observed = 0ULL;
    uint64_t addr_chain_tail_observed = 0ULL;
    uint64_t pressure0_observed = 0ULL;
    uint64_t pressure_tail_observed = 0ULL;

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    ai_vdep_enable_vector_state();
    write64(at_paddr, at_addr_seed);
    write64(elem0_paddr, elem0_seed);
    write64(elem1_paddr, elem1_old);
    write64(elem2_paddr, elem2_seed);
    write64(elem3_paddr, elem3_seed);
    write64(offsets_paddr + 0U * sizeof(uint64_t), elem0_vaddr - vector_base);
    write64(offsets_paddr + 1U * sizeof(uint64_t), elem1_vaddr - vector_base);
    write64(offsets_paddr + 2U * sizeof(uint64_t), elem2_vaddr - vector_base);
    write64(offsets_paddr + 3U * sizeof(uint64_t), elem3_vaddr - vector_base);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        const uintptr_t next = (i + 1U == AI_VDEP_RAW_ADDR_CHAIN_LINKS) ?
            elem1_vaddr : addr_chain_vaddr + (i + 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE;
        write64(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE, next);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; ++i) {
        write64(pressure_paddr + i * sizeof(uint64_t), pressure_seed + i);
    }
    for (unsigned i = 0; i < AI_VDEP_ELEMS; ++i) {
        write64(dump_paddr + i * sizeof(uint64_t), sentinel);
    }
    elem1_cache_prime = read64(elem1_paddr);
    cbo_flush(at_paddr);
    cbo_flush(elem1_paddr);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        cbo_flush(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; i += 8U) {
        cbo_flush(pressure_paddr + i * sizeof(uint64_t));
    }
    fence_iorw_iorw();
    TEST_ASSERT("elem1-dmiss replay probe primes then flushes element1 so the vector element1 access must rely on RAW/replay handling instead of a warm old DCache hit",
        elem1_cache_prime == elem1_old
    );

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vdep_set_hs_leaf(shared_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem0_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem2_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem3_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_vdep_configure_pmp_no_read_two_pages(elem2_paddr, elem3_paddr);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    warm0 = ld(elem0_vaddr);
    TEST_ASSERT("elem1-dmiss replay probe prewarms only element0 as the non-faulting earlier vector element",
        excpt.triggered == false &&
        warm0 == elem0_seed
    );

    TEST_SETUP_EXCEPT();
    warm2 = ld(elem2_vaddr);
    TEST_ASSERT("elem1-dmiss replay probe confirms element2 takes load access fault after PMP no-read is installed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == elem2_vaddr
    );

    TEST_SETUP_EXCEPT();
    warm3 = ld(elem3_vaddr);
    TEST_ASSERT("elem1-dmiss replay probe confirms element3 also remains an access-fault element behind the first precise fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == elem3_vaddr
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_load_sentinel_to_v6((const uint64_t *)dump_vaddr);
    ai_vdep_load_index_offsets_to_v8((const uint64_t *)offsets_vaddr);
    TEST_ASSERT("elem1-dmiss replay probe prepares vector index and destination registers without warming the shared @/element1 page in HS mode",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    at_observed = ai_vdep_chain_store_addr_dep_vload_landing_dump_flow(at_vaddr,
        hash_data_seed,
        vector_base, (const uint64_t *)dump_vaddr,
        (const uint64_t *)offsets_vaddr, (const uint64_t *)pressure_vaddr,
        (uint64_t *)dump_vaddr, &observed_vstart);
    fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    TEST_ASSERT("elem1-dmiss replay probe delays # through a flushed @ pointer chain and 48 pressure loads, then lands immediately after the faulting vector load with vstart=2",
        at_observed == elem1_vaddr &&
        fault_triggered == true &&
        fault_cause == CAUSE_LAF &&
        fault_tval == elem2_vaddr &&
        observed_vstart == 2ULL
    );

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    dump0_observed = read64(dump_paddr + 0U * sizeof(uint64_t));
    dump1_observed = read64(dump_paddr + 1U * sizeof(uint64_t));
    elem1_backing_observed = read64(elem1_paddr);
    elem0_backing_observed = read64(elem0_paddr);
    elem2_backing_observed = read64(elem2_paddr);
    elem3_backing_observed = read64(elem3_paddr);
    at_backing_observed = read64(at_paddr);
    addr_chain0_observed = read64(addr_chain_paddr);
    addr_chain_tail_observed = read64(addr_chain_paddr +
        (AI_VDEP_RAW_ADDR_CHAIN_LINKS - 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    pressure0_observed = read64(pressure_paddr);
    pressure_tail_observed = read64(pressure_paddr +
        (AI_VDEP_RAW_PRESSURE_LOADS - 1U) * sizeof(uint64_t));

    TEST_ASSERT("elem1-dmiss replay probe vector dump element0 keeps the element0 seed before the element2 fault",
        dump0_observed == elem0_seed,
        "dump0_actual=0x%llx dump0_expected=0x%llx",
        (unsigned long long)dump0_observed,
        (unsigned long long)elem0_seed
    );
    TEST_ASSERT("elem1-dmiss replay probe vector dump element1 must observe the older address-dependent store value, not the flushed old backing value returned by a DCache-miss replay",
        dump1_observed == hash_data_seed,
        "dump1_actual=0x%llx expected_hash=0x%llx elem1_old=0x%llx",
        (unsigned long long)dump1_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)elem1_old
    );
    TEST_ASSERT("elem1-dmiss replay probe element1 backing memory is updated by the address-dependent store #",
        elem1_backing_observed == hash_data_seed,
        "elem1_backing_actual=0x%llx expected_hash=0x%llx elem1_old=0x%llx",
        (unsigned long long)elem1_backing_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)elem1_old
    );
    TEST_ASSERT("elem1-dmiss replay probe keeps non-target backing memory and the flushed address chain intact across the precise vector fault",
        elem0_backing_observed == elem0_seed &&
        elem2_backing_observed == elem2_seed &&
        elem3_backing_observed == elem3_seed &&
        at_backing_observed == at_addr_seed &&
        addr_chain0_observed == addr_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE &&
        addr_chain_tail_observed == elem1_vaddr &&
        pressure0_observed == pressure_seed &&
        pressure_tail_observed == pressure_seed + AI_VDEP_RAW_PRESSURE_LOADS - 1U,
        "elem0=0x%llx elem2=0x%llx elem3=0x%llx at=0x%llx chain={0x%llx,0x%llx} pressure={0x%llx,0x%llx}",
        (unsigned long long)elem0_backing_observed,
        (unsigned long long)elem2_backing_observed,
        (unsigned long long)elem3_backing_observed,
        (unsigned long long)at_backing_observed,
        (unsigned long long)addr_chain0_observed,
        (unsigned long long)addr_chain_tail_observed,
        (unsigned long long)pressure0_observed,
        (unsigned long long)pressure_tail_observed
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_raw_store_vloxei_fault_then_reuse_probe_corner()
{
    TEST_START();

    const enum test_page shared_page = VSRWX_GRWX;
    const enum test_page elem0_page = SWITCH1;
    const enum test_page elem2_page = SWITCH2;
    const enum test_page elem3_page = SCRATCHPAD;
    const enum test_page scratch_page = VSRW_GRWX;
    const uintptr_t vector_base = hs_page_base(shared_page);
    const uintptr_t at_vaddr = vector_base + 0x100U;
    const uintptr_t elem1_vaddr = vector_base + 0x240U;
    const uintptr_t elem0_vaddr = hs_page_base(elem0_page) + 0x140U;
    const uintptr_t elem2_vaddr = hs_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_vaddr = hs_page_base(elem3_page) + 0x1c0U;
    const uintptr_t at_paddr = phys_page_base(shared_page) + 0x100U;
    const uintptr_t elem1_paddr = phys_page_base(shared_page) + 0x240U;
    const uintptr_t elem0_paddr = phys_page_base(elem0_page) + 0x140U;
    const uintptr_t elem2_paddr = phys_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_paddr = phys_page_base(elem3_page) + 0x1c0U;
    const uintptr_t offsets_vaddr = hs_page_base(scratch_page) + 0x080U;
    const uintptr_t offsets_paddr = phys_page_base(scratch_page) + 0x080U;
    const uintptr_t first_dump_vaddr = hs_page_base(scratch_page) + 0x180U;
    const uintptr_t first_dump_paddr = phys_page_base(scratch_page) + 0x180U;
    const uintptr_t second_dump_vaddr = hs_page_base(scratch_page) + 0x280U;
    const uintptr_t second_dump_paddr = phys_page_base(scratch_page) + 0x280U;
    const uintptr_t second_vector_base = hs_page_base(scratch_page) + 0x380U;
    const uintptr_t second_vector_paddr = phys_page_base(scratch_page) + 0x380U;
    const uintptr_t second_offsets_vaddr = hs_page_base(scratch_page) + 0x480U;
    const uintptr_t second_offsets_paddr = phys_page_base(scratch_page) + 0x480U;
    const uintptr_t raw_probe_vaddr = hs_page_base(scratch_page) + 0x580U;
    const uintptr_t raw_probe_paddr = phys_page_base(scratch_page) + 0x580U;
    const uintptr_t raw_chain_vaddr = hs_page_base(scratch_page) + 0x700U;
    const uintptr_t raw_chain_paddr = phys_page_base(scratch_page) + 0x700U;
    const uintptr_t side0_at_vaddr = hs_page_base(scratch_page) + 0x900U;
    const uintptr_t side0_at_paddr = phys_page_base(scratch_page) + 0x900U;
    const uintptr_t side0_chain_vaddr = hs_page_base(scratch_page) + 0x940U;
    const uintptr_t side0_chain_paddr = phys_page_base(scratch_page) + 0x940U;
    const uintptr_t side0_target_vaddr = hs_page_base(scratch_page) + 0xb40U;
    const uintptr_t side0_target_paddr = phys_page_base(scratch_page) + 0xb40U;
    const uintptr_t side1_at_vaddr = hs_page_base(scratch_page) + 0xb80U;
    const uintptr_t side1_at_paddr = phys_page_base(scratch_page) + 0xb80U;
    const uintptr_t side1_chain_vaddr = hs_page_base(scratch_page) + 0xbc0U;
    const uintptr_t side1_chain_paddr = phys_page_base(scratch_page) + 0xbc0U;
    const uintptr_t side1_target_vaddr = hs_page_base(scratch_page) + 0xdc0U;
    const uintptr_t side1_target_paddr = phys_page_base(scratch_page) + 0xdc0U;
    const uint64_t elem1_pointer_seed = (uint64_t)elem1_vaddr;
    const uint64_t elem1_new = 0xa5a5a5a5deadbeefULL;
    const uint64_t side0_new = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t side1_new = 0x8877665544332211ULL;
    const uint64_t side_target_old = 0x1213141516171819ULL;
    const uint64_t elem0_seed = 0x0a0b0c0d0e0f1011ULL;
    const uint64_t elem1_old = 0x11100f0e0d0c0b0aULL;
    const uint64_t elem2_seed = 0x2021222324252627ULL;
    const uint64_t elem3_seed = 0x3031323334353637ULL;
    const uint64_t second0_seed = 0x4445464748494a4bULL;
    const uint64_t second1_seed = 0x55565758595a5b5cULL;
    const uint64_t second2_seed = 0x666768696a6b6c6dULL;
    const uint64_t second3_seed = 0x7778797a7b7c7d7eULL;
    const uint64_t sentinel = 0xfeedfacecafebeefULL;
    const uint64_t raw_probe_seed = 0x9192939495969700ULL;
    uint64_t elem1_cache_warm = 0ULL;
    uint64_t warm0 = 0ULL;
    uint64_t warm1 = 0ULL;
    uint64_t warm2 = 0ULL;
    uint64_t warm3 = 0ULL;
    uint64_t at_observed = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool fault_triggered = false;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t first_dump0 = 0ULL;
    uint64_t first_dump1 = 0ULL;
    uint64_t second_dump0 = 0ULL;
    uint64_t second_dump1 = 0ULL;
    uint64_t second_dump2 = 0ULL;
    uint64_t second_dump3 = 0ULL;
    uint64_t elem1_backing = 0ULL;
    uint64_t at_backing = 0ULL;
    uint64_t raw_probe_backing = 0ULL;
    uint64_t raw_probe_tail_backing = 0ULL;
    uint64_t raw_chain0_backing = 0ULL;
    uint64_t raw_chain_tail_backing = 0ULL;
    uint64_t side0_target_backing = 0ULL;
    uint64_t side1_target_backing = 0ULL;
    uint64_t side0_chain0_backing = 0ULL;
    uint64_t side1_chain0_backing = 0ULL;
    uint64_t side0_chain_tail_backing = 0ULL;
    uint64_t side1_chain_tail_backing = 0ULL;

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    ai_vdep_enable_vector_state();
    write64(at_paddr, raw_chain_vaddr);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        const uintptr_t next = (i + 1U == AI_VDEP_RAW_ADDR_CHAIN_LINKS) ?
            elem1_vaddr : raw_chain_vaddr + (i + 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE;
        write64(raw_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE, next);
    }
    write64(side0_at_paddr, side0_chain_vaddr);
    write64(side1_at_paddr, side1_chain_vaddr);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        const uintptr_t next0 = (i + 1U == AI_VDEP_RAW_ADDR_CHAIN_LINKS) ?
            side0_target_vaddr : side0_chain_vaddr + (i + 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE;
        const uintptr_t next1 = (i + 1U == AI_VDEP_RAW_ADDR_CHAIN_LINKS) ?
            side1_target_vaddr : side1_chain_vaddr + (i + 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE;
        write64(side0_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE, next0);
        write64(side1_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE, next1);
    }
    write64(elem0_paddr, elem0_seed);
    write64(elem1_paddr, elem1_old);
    write64(elem2_paddr, elem2_seed);
    write64(elem3_paddr, elem3_seed);
    write64(side0_target_paddr, side_target_old);
    write64(side1_target_paddr, side_target_old);
    write64(offsets_paddr + 0U * sizeof(uint64_t), elem1_vaddr - vector_base);
    write64(offsets_paddr + 1U * sizeof(uint64_t), elem0_vaddr - vector_base);
    write64(offsets_paddr + 2U * sizeof(uint64_t), elem2_vaddr - vector_base);
    write64(offsets_paddr + 3U * sizeof(uint64_t), elem3_vaddr - vector_base);
    write64(second_vector_paddr + 0U * sizeof(uint64_t), second0_seed);
    write64(second_vector_paddr + 1U * sizeof(uint64_t), second1_seed);
    write64(second_vector_paddr + 2U * sizeof(uint64_t), second2_seed);
    write64(second_vector_paddr + 3U * sizeof(uint64_t), second3_seed);
    write64(second_offsets_paddr + 0U * sizeof(uint64_t), 0U * sizeof(uint64_t));
    write64(second_offsets_paddr + 1U * sizeof(uint64_t), 1U * sizeof(uint64_t));
    write64(second_offsets_paddr + 2U * sizeof(uint64_t), 2U * sizeof(uint64_t));
    write64(second_offsets_paddr + 3U * sizeof(uint64_t), 3U * sizeof(uint64_t));
    for (unsigned i = 0; i < AI_VDEP_ELEMS; ++i) {
        write64(first_dump_paddr + i * sizeof(uint64_t), sentinel);
        write64(second_dump_paddr + i * sizeof(uint64_t), sentinel);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; ++i) {
        write64(raw_probe_paddr + i * sizeof(uint64_t),
            raw_probe_seed + i);
    }
    elem1_cache_warm = read64(elem1_paddr);
    cbo_flush(at_paddr);
    cbo_flush(side0_at_paddr);
    cbo_flush(side1_at_paddr);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        cbo_flush(raw_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
        cbo_flush(side0_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
        cbo_flush(side1_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; i += 8U) {
        cbo_flush(raw_probe_paddr + i * sizeof(uint64_t));
    }
    fence_iorw_iorw();
    TEST_ASSERT("raw/fault reuse probe prewarms element1 as the old DCache value and flushes three pointer chains plus pressure-load lines",
        elem1_cache_warm == elem1_old
    );

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vdep_set_hs_leaf(shared_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem0_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem2_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem3_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_vdep_configure_pmp_no_read_two_pages(elem2_paddr, elem3_paddr);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    warm0 = ld(elem0_vaddr);
    warm1 = ld(elem1_vaddr);
    TEST_ASSERT("raw/fault reuse probe warms element0 and element1 translations before issuing the store-then-vector-load window",
        excpt.triggered == false &&
        warm0 == elem0_seed &&
        warm1 == elem1_old
    );

    TEST_SETUP_EXCEPT();
    warm2 = ld(elem2_vaddr);
    TEST_ASSERT("raw/fault reuse probe confirms element2 is the first vector access-fault candidate",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == elem2_vaddr
    );

    TEST_SETUP_EXCEPT();
    warm3 = ld(elem3_vaddr);
    TEST_ASSERT("raw/fault reuse probe keeps element3 no-read so element2 remains the earliest failing indexed element",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == elem3_vaddr
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    at_observed = ai_vdep_pressure_raw_store_addr_dep_vload_flow(at_vaddr,
        side0_at_vaddr, side1_at_vaddr, elem1_new, side0_new, side1_new,
        vector_base, (const uint64_t *)first_dump_vaddr,
        (const uint64_t *)offsets_vaddr, (const uint64_t *)raw_probe_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    TEST_ASSERT("raw/fault reuse probe holds three older store addresses behind flushed pointer chains, issues the faulting indexed vector load after 32 RAW pressure loads, and leaves 16 younger pressure loads behind it so vector element0 can collide with C_RAW before vstart=2",
        at_observed == elem1_pointer_seed &&
        fault_triggered == true &&
        fault_cause == CAUSE_LAF &&
        fault_tval == elem2_vaddr &&
        observed_vstart == 2ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_store_v6_m2((uint64_t *)first_dump_vaddr);
    TEST_ASSERT("raw/fault reuse probe can dump the partially completed first vector destination after clearing vstart",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_indexed_vload_v6_m2_flow(second_vector_base,
        (const uint64_t *)second_dump_vaddr,
        (const uint64_t *)second_offsets_vaddr);
    TEST_ASSERT("raw/fault reuse probe immediately reuses the vector load path with a separate no-fault indexed vector load",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_vdep_store_v6_m2((uint64_t *)second_dump_vaddr);
    TEST_ASSERT("raw/fault reuse probe can dump the second vector destination without inheriting the first fault state",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    first_dump0 = read64(first_dump_paddr + 0U * sizeof(uint64_t));
    first_dump1 = read64(first_dump_paddr + 1U * sizeof(uint64_t));
    second_dump0 = read64(second_dump_paddr + 0U * sizeof(uint64_t));
    second_dump1 = read64(second_dump_paddr + 1U * sizeof(uint64_t));
    second_dump2 = read64(second_dump_paddr + 2U * sizeof(uint64_t));
    second_dump3 = read64(second_dump_paddr + 3U * sizeof(uint64_t));
    elem1_backing = read64(elem1_paddr);
    at_backing = read64(at_paddr);
    raw_probe_backing = read64(raw_probe_paddr);
    raw_probe_tail_backing = read64(raw_probe_paddr +
        (AI_VDEP_RAW_PRESSURE_LOADS - 1U) * sizeof(uint64_t));
    raw_chain0_backing = read64(raw_chain_paddr);
    raw_chain_tail_backing = read64(raw_chain_paddr +
        (AI_VDEP_RAW_ADDR_CHAIN_LINKS - 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    side0_target_backing = read64(side0_target_paddr);
    side1_target_backing = read64(side1_target_paddr);
    side0_chain0_backing = read64(side0_chain_paddr);
    side1_chain0_backing = read64(side1_chain_paddr);
    side0_chain_tail_backing = read64(side0_chain_paddr +
        (AI_VDEP_RAW_ADDR_CHAIN_LINKS - 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    side1_chain_tail_backing = read64(side1_chain_paddr +
        (AI_VDEP_RAW_ADDR_CHAIN_LINKS - 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);

    TEST_ASSERT("raw/fault reuse probe first vector element0 observes the older address-dependent store value instead of the stale DCache value while RAW pressure is active",
        first_dump0 == elem1_new,
        "first_dump0_actual=0x%llx expected_new=0x%llx elem1_old=0x%llx",
        (unsigned long long)first_dump0,
        (unsigned long long)elem1_new,
        (unsigned long long)elem1_old
    );
    TEST_ASSERT("raw/fault reuse probe first vector element1 completes the independent elem0 load before the element2 fault",
        first_dump1 == elem0_seed,
        "first_dump1_actual=0x%llx expected_elem0=0x%llx",
        (unsigned long long)first_dump1,
        (unsigned long long)elem0_seed
    );
    TEST_ASSERT("raw/fault reuse probe scalar side effects reached backing memory before the precise vector fault is reported",
        elem1_backing == elem1_new &&
        at_backing == raw_chain_vaddr &&
        raw_probe_backing == raw_probe_seed &&
        raw_probe_tail_backing == raw_probe_seed + AI_VDEP_RAW_PRESSURE_LOADS - 1U &&
        raw_chain0_backing == raw_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE &&
        raw_chain_tail_backing == elem1_pointer_seed,
        "elem1_actual=0x%llx elem1_expected=0x%llx at_actual=0x%llx at_expected=0x%llx raw_probe={0x%llx,0x%llx} raw_probe_expected={0x%llx,0x%llx} raw_chain={0x%llx,0x%llx} raw_chain_expected={0x%llx,0x%llx}",
        (unsigned long long)elem1_backing,
        (unsigned long long)elem1_new,
        (unsigned long long)at_backing,
        (unsigned long long)raw_chain_vaddr,
        (unsigned long long)raw_probe_backing,
        (unsigned long long)raw_probe_tail_backing,
        (unsigned long long)raw_probe_seed,
        (unsigned long long)(raw_probe_seed + AI_VDEP_RAW_PRESSURE_LOADS - 1U),
        (unsigned long long)raw_chain0_backing,
        (unsigned long long)raw_chain_tail_backing,
        (unsigned long long)(raw_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE),
        (unsigned long long)elem1_pointer_seed
    );
    TEST_ASSERT("raw/fault reuse probe side older address-dependent stores resolve through their own flushed chains and update independent scratch targets",
        side0_target_backing == side0_new &&
        side1_target_backing == side1_new &&
        side0_chain0_backing == side0_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE &&
        side1_chain0_backing == side1_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE &&
        side0_chain_tail_backing == side0_target_vaddr &&
        side1_chain_tail_backing == side1_target_vaddr,
        "side_targets={0x%llx,0x%llx} expected={0x%llx,0x%llx} side_chain0={0x%llx,0x%llx} side_chain0_expected={0x%llx,0x%llx} side_tail={0x%llx,0x%llx} side_tail_expected={0x%llx,0x%llx}",
        (unsigned long long)side0_target_backing,
        (unsigned long long)side1_target_backing,
        (unsigned long long)side0_new,
        (unsigned long long)side1_new,
        (unsigned long long)side0_chain0_backing,
        (unsigned long long)side1_chain0_backing,
        (unsigned long long)(side0_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE),
        (unsigned long long)(side1_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE),
        (unsigned long long)side0_chain_tail_backing,
        (unsigned long long)side1_chain_tail_backing,
        (unsigned long long)side0_target_vaddr,
        (unsigned long long)side1_target_vaddr
    );
    TEST_ASSERT("raw/fault reuse probe second indexed vector load is not polluted by the first faulting RAW window",
        second_dump0 == second0_seed &&
        second_dump1 == second1_seed &&
        second_dump2 == second2_seed &&
        second_dump3 == second3_seed,
        "second_actual={0x%llx,0x%llx,0x%llx,0x%llx} expected={0x%llx,0x%llx,0x%llx,0x%llx}",
        (unsigned long long)second_dump0,
        (unsigned long long)second_dump1,
        (unsigned long long)second_dump2,
        (unsigned long long)second_dump3,
        (unsigned long long)second0_seed,
        (unsigned long long)second1_seed,
        (unsigned long long)second2_seed,
        (unsigned long long)second3_seed
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_load_store_then_vloxei_no_fault_dep_flow_probe()
{
    TEST_START();

    const enum test_page shared_page = VSRWX_GRWX;
    const enum test_page elem0_page = SWITCH1;
    const enum test_page elem2_page = SWITCH2;
    const enum test_page elem3_page = SCRATCHPAD;
    const enum test_page scratch_page = VSRW_GRWX;
    const uintptr_t vector_base = hs_page_base(shared_page);
    const uintptr_t at_vaddr = vector_base + 0x100U;
    const uintptr_t elem1_vaddr = vector_base + 0x240U;
    const uintptr_t elem0_vaddr = hs_page_base(elem0_page) + 0x140U;
    const uintptr_t elem2_vaddr = hs_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_vaddr = hs_page_base(elem3_page) + 0x1c0U;
    const uintptr_t at_paddr = phys_page_base(shared_page) + 0x100U;
    const uintptr_t elem1_paddr = phys_page_base(shared_page) + 0x240U;
    const uintptr_t elem0_paddr = phys_page_base(elem0_page) + 0x140U;
    const uintptr_t elem2_paddr = phys_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_paddr = phys_page_base(elem3_page) + 0x1c0U;
    const uintptr_t offsets_vaddr = hs_page_base(scratch_page) + 0x080U;
    const uintptr_t offsets_paddr = phys_page_base(scratch_page) + 0x080U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0x180U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0x180U;
    const uintptr_t addr_chain_vaddr = hs_page_base(scratch_page) + 0x300U;
    const uintptr_t addr_chain_paddr = phys_page_base(scratch_page) + 0x300U;
    const uintptr_t pressure_vaddr = hs_page_base(scratch_page) + 0x580U;
    const uintptr_t pressure_paddr = phys_page_base(scratch_page) + 0x580U;
    const uint64_t at_addr_seed = (uint64_t)addr_chain_vaddr;
    const uint64_t hash_data_seed = 0xd155c0decafef00dULL;
    const uint64_t elem0_seed = 0x0a0b0c0d0e0f1011ULL;
    const uint64_t elem1_old = 0x11100f0e0d0c0b0aULL;
    const uint64_t elem2_seed = 0x2021222324252627ULL;
    const uint64_t elem3_seed = 0x3031323334353637ULL;
    const uint64_t pressure_seed = 0x6162636465666700ULL;
    const uint64_t sentinel = 0xfeedfacecafebeefULL;
    uint64_t elem1_cache_warm = 0ULL;
    uint64_t warm0 = 0ULL;
    uint64_t warm2 = 0ULL;
    uint64_t warm3 = 0ULL;
    uint64_t at_observed = 0ULL;
    uint64_t observed_vstart = 0ULL;
    uint64_t dump0_observed = 0ULL;
    uint64_t dump1_observed = 0ULL;
    uint64_t dump2_observed = 0ULL;
    uint64_t dump3_observed = 0ULL;
    uint64_t elem1_backing_observed = 0ULL;
    uint64_t at_backing_observed = 0ULL;
    uint64_t addr_chain0_observed = 0ULL;
    uint64_t addr_chain_tail_observed = 0ULL;
    uint64_t pressure0_observed = 0ULL;
    uint64_t pressure_tail_observed = 0ULL;

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    ai_vdep_enable_vector_state();
    write64(at_paddr, at_addr_seed);
    write64(elem0_paddr, elem0_seed);
    write64(elem1_paddr, elem1_old);
    write64(elem2_paddr, elem2_seed);
    write64(elem3_paddr, elem3_seed);
    write64(offsets_paddr + 0U * sizeof(uint64_t), elem0_vaddr - vector_base);
    write64(offsets_paddr + 1U * sizeof(uint64_t), elem1_vaddr - vector_base);
    write64(offsets_paddr + 2U * sizeof(uint64_t), elem2_vaddr - vector_base);
    write64(offsets_paddr + 3U * sizeof(uint64_t), elem3_vaddr - vector_base);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        const uintptr_t next = (i + 1U == AI_VDEP_RAW_ADDR_CHAIN_LINKS) ?
            elem1_vaddr : addr_chain_vaddr + (i + 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE;
        write64(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE, next);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; ++i) {
        write64(pressure_paddr + i * sizeof(uint64_t), pressure_seed + i);
    }
    for (unsigned i = 0; i < AI_VDEP_ELEMS; ++i) {
        write64(dump_paddr + i * sizeof(uint64_t), sentinel);
    }
    elem1_cache_warm = read64(elem1_paddr);
    cbo_flush(at_paddr);
    cbo_flush(elem1_paddr);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        cbo_flush(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; i += 8U) {
        cbo_flush(pressure_paddr + i * sizeof(uint64_t));
    }
    fence_iorw_iorw();
    TEST_ASSERT("no-fault probe primes then flushes element1 so the directed vector element1 access must resolve through replay/RAW instead of a warm old DCache hit",
        elem1_cache_warm == elem1_old
    );

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vdep_set_hs_leaf(shared_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem0_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem2_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem3_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warm0 = ld(elem0_vaddr);
    warm2 = ld(elem2_vaddr);
    warm3 = ld(elem3_vaddr);
    TEST_ASSERT("no-fault probe prewarms elements 0, 2, and 3 translations before the directed @/#/vector-load flow",
        excpt.triggered == false &&
        warm0 == elem0_seed &&
        warm2 == elem2_seed &&
        warm3 == elem3_seed
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_load_sentinel_to_v6((const uint64_t *)dump_vaddr);
    ai_vdep_load_index_offsets_to_v8((const uint64_t *)offsets_vaddr);
    TEST_ASSERT("no-fault probe prepares vector index register and destination sentinel",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    at_observed = ai_vdep_chain_store_addr_dep_vload_flow(at_vaddr, hash_data_seed,
        vector_base, (const uint64_t *)dump_vaddr,
        (const uint64_t *)offsets_vaddr, (const uint64_t *)pressure_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("no-fault probe delays # through a flushed @ pointer chain and 48 pressure loads, then indexed vector load completes without the later exception retry masking the first element1 result",
        at_observed == elem1_vaddr &&
        excpt.triggered == false &&
        observed_vstart == 0ULL,
        "at_observed=0x%llx expected_elem1=0x%llx triggered=%d vstart=0x%llx",
        (unsigned long long)at_observed,
        (unsigned long long)elem1_vaddr,
        excpt.triggered,
        (unsigned long long)observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_store_v6_m2((uint64_t *)dump_vaddr);
    TEST_ASSERT("no-fault probe dumps the vector destination after the directed flow",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    dump0_observed = read64(dump_paddr + 0U * sizeof(uint64_t));
    dump1_observed = read64(dump_paddr + 1U * sizeof(uint64_t));
    dump2_observed = read64(dump_paddr + 2U * sizeof(uint64_t));
    dump3_observed = read64(dump_paddr + 3U * sizeof(uint64_t));
    elem1_backing_observed = read64(elem1_paddr);
    at_backing_observed = read64(at_paddr);
    addr_chain0_observed = read64(addr_chain_paddr);
    addr_chain_tail_observed = read64(addr_chain_paddr +
        (AI_VDEP_RAW_ADDR_CHAIN_LINKS - 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    pressure0_observed = read64(pressure_paddr);
    pressure_tail_observed = read64(pressure_paddr +
        (AI_VDEP_RAW_PRESSURE_LOADS - 1U) * sizeof(uint64_t));
    TEST_ASSERT("no-fault probe vector dump element0 sees the element0 seed",
        dump0_observed == elem0_seed,
        "dump0_actual=0x%llx dump0_expected=0x%llx",
        (unsigned long long)dump0_observed,
        (unsigned long long)elem0_seed
    );
    TEST_ASSERT("no-fault probe vector dump element1 sees the address-dependent store # value without relying on exception replay to repair an early stale writeback",
        dump1_observed == hash_data_seed,
        "dump1_actual=0x%llx expected_hash=0x%llx elem1_old=0x%llx",
        (unsigned long long)dump1_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)elem1_old
    );
    TEST_ASSERT("no-fault probe vector dump elements2 and 3 keep their source seeds",
        dump2_observed == elem2_seed &&
        dump3_observed == elem3_seed,
        "dump2_actual=0x%llx dump2_expected=0x%llx dump3_actual=0x%llx dump3_expected=0x%llx",
        (unsigned long long)dump2_observed,
        (unsigned long long)elem2_seed,
        (unsigned long long)dump3_observed,
        (unsigned long long)elem3_seed
    );
    TEST_ASSERT("no-fault probe element1 backing memory is updated by the dependent store #",
        elem1_backing_observed == hash_data_seed,
        "elem1_backing_actual=0x%llx expected_hash=0x%llx elem1_old=0x%llx",
        (unsigned long long)elem1_backing_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)elem1_old
    );
    TEST_ASSERT("no-fault probe @ and pressure backing data keep the intended delay chain intact",
        at_backing_observed == at_addr_seed &&
        addr_chain0_observed == addr_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE &&
        addr_chain_tail_observed == elem1_vaddr &&
        pressure0_observed == pressure_seed &&
        pressure_tail_observed == pressure_seed + AI_VDEP_RAW_PRESSURE_LOADS - 1U,
        "at=0x%llx chain0=0x%llx chain_tail=0x%llx pressure0=0x%llx pressure_tail=0x%llx",
        (unsigned long long)at_backing_observed,
        (unsigned long long)addr_chain0_observed,
        (unsigned long long)addr_chain_tail_observed,
        (unsigned long long)pressure0_observed,
        (unsigned long long)pressure_tail_observed
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_load_store_then_vloxei_elem0_dmiss_fault1_same_uop_escape_corner()
{
    TEST_START();

    const enum test_page shared_page = VSRWX_GRWX;
    const enum test_page fault1_page = SWITCH1;
    const enum test_page elem2_page = SWITCH2;
    const enum test_page elem3_page = SCRATCHPAD;
    const enum test_page scratch_page = VSRW_GRWX;
    const uintptr_t vector_base = hs_page_base(shared_page);
    const uintptr_t at_vaddr = vector_base + 0x100U;
    const uintptr_t elem0_vaddr = vector_base + 0x240U;
    const uintptr_t fault1_vaddr = hs_page_base(fault1_page) + 0x140U;
    const uintptr_t elem2_vaddr = hs_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_vaddr = hs_page_base(elem3_page) + 0x1c0U;
    const uintptr_t at_paddr = phys_page_base(shared_page) + 0x100U;
    const uintptr_t elem0_paddr = phys_page_base(shared_page) + 0x240U;
    const uintptr_t fault1_paddr = phys_page_base(fault1_page) + 0x140U;
    const uintptr_t elem2_paddr = phys_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_paddr = phys_page_base(elem3_page) + 0x1c0U;
    const uintptr_t offsets_vaddr = hs_page_base(scratch_page) + 0x080U;
    const uintptr_t offsets_paddr = phys_page_base(scratch_page) + 0x080U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0x180U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0x180U;
    const uintptr_t addr_chain_vaddr = hs_page_base(scratch_page) + 0x300U;
    const uintptr_t addr_chain_paddr = phys_page_base(scratch_page) + 0x300U;
    const uintptr_t pressure_vaddr = hs_page_base(scratch_page) + 0x580U;
    const uintptr_t pressure_paddr = phys_page_base(scratch_page) + 0x580U;
    const uint64_t at_addr_seed = (uint64_t)addr_chain_vaddr;
    const uint64_t hash_data_seed = 0x9e3779b97f4a7c15ULL;
    const uint64_t elem0_old = 0x1716151413121110ULL;
    const uint64_t fault1_seed = 0x2122232425262728ULL;
    const uint64_t elem2_seed = 0x3233343536373839ULL;
    const uint64_t elem3_seed = 0x434445464748494aULL;
    const uint64_t pressure_seed = 0x7172737475767700ULL;
    const uint64_t sentinel = 0xfeedfacecafebeefULL;
    uint64_t elem0_cache_prime = 0ULL;
    uint64_t warm_fault1 = 0ULL;
    uint64_t warm2 = 0ULL;
    uint64_t warm3 = 0ULL;
    uint64_t at_observed = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool fault_triggered = false;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t dump0_observed = 0ULL;
    uint64_t dump1_observed = 0ULL;
    uint64_t dump2_observed = 0ULL;
    uint64_t dump3_observed = 0ULL;
    uint64_t elem0_backing_observed = 0ULL;
    uint64_t fault1_backing_observed = 0ULL;
    uint64_t elem2_backing_observed = 0ULL;
    uint64_t elem3_backing_observed = 0ULL;
    uint64_t at_backing_observed = 0ULL;
    uint64_t addr_chain0_observed = 0ULL;
    uint64_t addr_chain_tail_observed = 0ULL;
    uint64_t pressure0_observed = 0ULL;
    uint64_t pressure_tail_observed = 0ULL;

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    ai_vdep_enable_vector_state();
    write64(at_paddr, at_addr_seed);
    write64(elem0_paddr, elem0_old);
    write64(fault1_paddr, fault1_seed);
    write64(elem2_paddr, elem2_seed);
    write64(elem3_paddr, elem3_seed);
    write64(offsets_paddr + 0U * sizeof(uint64_t), elem0_vaddr - vector_base);
    write64(offsets_paddr + 1U * sizeof(uint64_t), fault1_vaddr - vector_base);
    write64(offsets_paddr + 2U * sizeof(uint64_t), elem2_vaddr - vector_base);
    write64(offsets_paddr + 3U * sizeof(uint64_t), elem3_vaddr - vector_base);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        const uintptr_t next = (i + 1U == AI_VDEP_RAW_ADDR_CHAIN_LINKS) ?
            elem0_vaddr : addr_chain_vaddr + (i + 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE;
        write64(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE, next);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; ++i) {
        write64(pressure_paddr + i * sizeof(uint64_t), pressure_seed + i);
    }
    for (unsigned i = 0; i < AI_VDEP_ELEMS; ++i) {
        write64(dump_paddr + i * sizeof(uint64_t), sentinel);
    }
    elem0_cache_prime = read64(elem0_paddr);
    cbo_flush(at_paddr);
    cbo_flush(elem0_paddr);
    for (unsigned i = 0; i < AI_VDEP_RAW_ADDR_CHAIN_LINKS; ++i) {
        cbo_flush(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    }
    for (unsigned i = 0; i < AI_VDEP_RAW_PRESSURE_LOADS; i += 8U) {
        cbo_flush(pressure_paddr + i * sizeof(uint64_t));
    }
    fence_iorw_iorw();
    TEST_ASSERT("same-uop escape probe primes then flushes element0 so the first vector element must be repaired by RAW/replay, not by a stale DCache refill",
        elem0_cache_prime == elem0_old
    );

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vdep_set_hs_leaf(shared_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(fault1_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem2_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem3_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_vdep_configure_pmp_no_read_page(fault1_paddr);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    warm_fault1 = ld(fault1_vaddr);
    TEST_ASSERT("same-uop escape probe makes element1 the first precise vector fault candidate",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault1_vaddr
    );

    TEST_SETUP_EXCEPT();
    warm2 = ld(elem2_vaddr);
    warm3 = ld(elem3_vaddr);
    TEST_ASSERT("same-uop escape probe prewarms later element translations while leaving the shared @/element0 page untouched in HS mode",
        excpt.triggered == false &&
        warm2 == elem2_seed &&
        warm3 == elem3_seed
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_load_sentinel_to_v6((const uint64_t *)dump_vaddr);
    ai_vdep_load_index_offsets_to_v8((const uint64_t *)offsets_vaddr);
    TEST_ASSERT("same-uop escape probe prepares vector index and destination registers without warming the shared @/element0 page in HS mode",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    at_observed = ai_vdep_chain_store_addr_dep_vload_landing_dump_flow(at_vaddr,
        hash_data_seed,
        vector_base, (const uint64_t *)dump_vaddr,
        (const uint64_t *)offsets_vaddr, (const uint64_t *)pressure_vaddr,
        (uint64_t *)dump_vaddr, &observed_vstart);
    fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    TEST_ASSERT("same-uop escape probe delays # through a flushed @ pointer chain, then faults at vector element1 so element0 is the committed prefix of the same vector uop",
        at_observed == elem0_vaddr &&
        fault_triggered == true &&
        fault_cause == CAUSE_LAF &&
        fault_tval == fault1_vaddr &&
        observed_vstart == 1ULL,
        "at_observed=0x%llx expected_elem0=0x%llx cause=0x%llx tval=0x%llx expected_tval=0x%llx vstart=0x%llx",
        (unsigned long long)at_observed,
        (unsigned long long)elem0_vaddr,
        (unsigned long long)fault_cause,
        (unsigned long long)fault_tval,
        (unsigned long long)fault1_vaddr,
        (unsigned long long)observed_vstart
    );

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    dump0_observed = read64(dump_paddr + 0U * sizeof(uint64_t));
    dump1_observed = read64(dump_paddr + 1U * sizeof(uint64_t));
    dump2_observed = read64(dump_paddr + 2U * sizeof(uint64_t));
    dump3_observed = read64(dump_paddr + 3U * sizeof(uint64_t));
    elem0_backing_observed = read64(elem0_paddr);
    fault1_backing_observed = read64(fault1_paddr);
    elem2_backing_observed = read64(elem2_paddr);
    elem3_backing_observed = read64(elem3_paddr);
    at_backing_observed = read64(at_paddr);
    addr_chain0_observed = read64(addr_chain_paddr);
    addr_chain_tail_observed = read64(addr_chain_paddr +
        (AI_VDEP_RAW_ADDR_CHAIN_LINKS - 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    pressure0_observed = read64(pressure_paddr);
    pressure_tail_observed = read64(pressure_paddr +
        (AI_VDEP_RAW_PRESSURE_LOADS - 1U) * sizeof(uint64_t));

    TEST_ASSERT("same-uop escape probe vector dump element0 must see the older address-dependent store # value, not the flushed old backing value",
        dump0_observed == hash_data_seed,
        "dump0_actual=0x%llx expected_hash=0x%llx elem0_old=0x%llx",
        (unsigned long long)dump0_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)elem0_old
    );
    TEST_ASSERT("same-uop escape probe elements at and after the first fault keep the sentinel image in the dumped vector register",
        dump1_observed == sentinel &&
        dump2_observed == sentinel &&
        dump3_observed == sentinel,
        "dump_tail_actual={0x%llx,0x%llx,0x%llx} sentinel=0x%llx",
        (unsigned long long)dump1_observed,
        (unsigned long long)dump2_observed,
        (unsigned long long)dump3_observed,
        (unsigned long long)sentinel
    );
    TEST_ASSERT("same-uop escape probe element0 backing memory is updated by the dependent store # while non-target backing data remains stable",
        elem0_backing_observed == hash_data_seed &&
        fault1_backing_observed == fault1_seed &&
        elem2_backing_observed == elem2_seed &&
        elem3_backing_observed == elem3_seed,
        "elem0=0x%llx expected=0x%llx fault1=0x%llx elem2=0x%llx elem3=0x%llx",
        (unsigned long long)elem0_backing_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)fault1_backing_observed,
        (unsigned long long)elem2_backing_observed,
        (unsigned long long)elem3_backing_observed
    );
    TEST_ASSERT("same-uop escape probe @, chain, and pressure backing data keep the intended delay structure intact",
        at_backing_observed == at_addr_seed &&
        addr_chain0_observed == addr_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE &&
        addr_chain_tail_observed == elem0_vaddr &&
        pressure0_observed == pressure_seed &&
        pressure_tail_observed == pressure_seed + AI_VDEP_RAW_PRESSURE_LOADS - 1U,
        "at=0x%llx chain0=0x%llx chain_tail=0x%llx pressure0=0x%llx pressure_tail=0x%llx",
        (unsigned long long)at_backing_observed,
        (unsigned long long)addr_chain0_observed,
        (unsigned long long)addr_chain_tail_observed,
        (unsigned long long)pressure0_observed,
        (unsigned long long)pressure_tail_observed
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_load_store_then_vloxei_elem0_dmiss_fault1_min_escape_corner()
{
    TEST_START();

    const enum test_page shared_page = VSRWX_GRWX;
    const enum test_page fault1_page = SWITCH1;
    const enum test_page elem2_page = SWITCH2;
    const enum test_page elem3_page = SCRATCHPAD;
    const enum test_page scratch_page = VSRW_GRWX;
    const uintptr_t vector_base = hs_page_base(shared_page);
    const uintptr_t at_vaddr = vector_base + 0x100U;
    const uintptr_t elem0_vaddr = vector_base + 0x240U;
    const uintptr_t fault1_vaddr = hs_page_base(fault1_page) + 0x140U;
    const uintptr_t elem2_vaddr = hs_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_vaddr = hs_page_base(elem3_page) + 0x1c0U;
    const uintptr_t at_paddr = phys_page_base(shared_page) + 0x100U;
    const uintptr_t elem0_paddr = phys_page_base(shared_page) + 0x240U;
    const uintptr_t fault1_paddr = phys_page_base(fault1_page) + 0x140U;
    const uintptr_t elem2_paddr = phys_page_base(elem2_page) + 0x180U;
    const uintptr_t elem3_paddr = phys_page_base(elem3_page) + 0x1c0U;
    const uintptr_t offsets_vaddr = hs_page_base(scratch_page) + 0x080U;
    const uintptr_t offsets_paddr = phys_page_base(scratch_page) + 0x080U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0x180U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0x180U;
    const uintptr_t addr_chain_vaddr = hs_page_base(scratch_page) + 0x300U;
    const uintptr_t addr_chain_paddr = phys_page_base(scratch_page) + 0x300U;
    const uintptr_t pressure_vaddr = hs_page_base(scratch_page) + 0x580U;
    const uintptr_t pressure_paddr = phys_page_base(scratch_page) + 0x580U;
    const uint64_t at_addr_seed = (uint64_t)addr_chain_vaddr;
    const uint64_t hash_data_seed = 0x9e3779b97f4a7c15ULL;
    const uint64_t elem0_old = 0x1716151413121110ULL;
    const uint64_t fault1_seed = 0x2122232425262728ULL;
    const uint64_t elem2_seed = 0x3233343536373839ULL;
    const uint64_t elem3_seed = 0x434445464748494aULL;
    const uint64_t pressure_seed = 0x7172737475767700ULL;
    const uint64_t sentinel = 0xfeedfacecafebeefULL;
    uint64_t elem0_cache_prime = 0ULL;
    uint64_t warm_fault1 = 0ULL;
    uint64_t warm2 = 0ULL;
    uint64_t warm3 = 0ULL;
    uint64_t at_observed = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool fault_triggered = false;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t dump0_observed = 0ULL;
    uint64_t dump1_observed = 0ULL;
    uint64_t dump2_observed = 0ULL;
    uint64_t dump3_observed = 0ULL;
    uint64_t elem0_backing_observed = 0ULL;
    uint64_t fault1_backing_observed = 0ULL;
    uint64_t elem2_backing_observed = 0ULL;
    uint64_t elem3_backing_observed = 0ULL;
    uint64_t at_backing_observed = 0ULL;
    uint64_t addr_chain0_observed = 0ULL;
    uint64_t addr_chain_tail_observed = 0ULL;
    uint64_t pressure0_observed = 0ULL;
    uint64_t pressure_tail_observed = 0ULL;

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    ai_vdep_enable_vector_state();
    write64(at_paddr, at_addr_seed);
    write64(elem0_paddr, elem0_old);
    write64(fault1_paddr, fault1_seed);
    write64(elem2_paddr, elem2_seed);
    write64(elem3_paddr, elem3_seed);
    write64(offsets_paddr + 0U * sizeof(uint64_t), elem0_vaddr - vector_base);
    write64(offsets_paddr + 1U * sizeof(uint64_t), fault1_vaddr - vector_base);
    write64(offsets_paddr + 2U * sizeof(uint64_t), elem2_vaddr - vector_base);
    write64(offsets_paddr + 3U * sizeof(uint64_t), elem3_vaddr - vector_base);
    for (unsigned i = 0; i < AI_VDEP_ESCAPE_MIN_ADDR_CHAIN_LINKS; ++i) {
        const uintptr_t next = (i + 1U == AI_VDEP_ESCAPE_MIN_ADDR_CHAIN_LINKS) ?
            elem0_vaddr : addr_chain_vaddr + (i + 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE;
        write64(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE, next);
    }
    for (unsigned i = 0; i < AI_VDEP_ESCAPE_MIN_PRESSURE_LOADS; ++i) {
        write64(pressure_paddr + i * sizeof(uint64_t), pressure_seed + i);
    }
    for (unsigned i = 0; i < AI_VDEP_ELEMS; ++i) {
        write64(dump_paddr + i * sizeof(uint64_t), sentinel);
    }
    elem0_cache_prime = read64(elem0_paddr);
    cbo_flush(at_paddr);
    cbo_flush(elem0_paddr);
    for (unsigned i = 0; i < AI_VDEP_ESCAPE_MIN_ADDR_CHAIN_LINKS; ++i) {
        cbo_flush(addr_chain_paddr + i * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    }
    for (unsigned i = 0; i < AI_VDEP_ESCAPE_MIN_PRESSURE_LOADS; i += 8U) {
        cbo_flush(pressure_paddr + i * sizeof(uint64_t));
    }
    fence_iorw_iorw();
    TEST_ASSERT("min escape probe primes then flushes element0 so RAW/replay, not stale refill data, must repair the first vector element",
        elem0_cache_prime == elem0_old
    );

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vdep_set_hs_leaf(shared_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(fault1_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem2_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem3_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_vdep_configure_pmp_no_read_page(fault1_paddr);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    warm_fault1 = ld(fault1_vaddr);
    TEST_ASSERT("min escape probe makes element1 the first precise vector fault candidate",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault1_vaddr
    );

    TEST_SETUP_EXCEPT();
    warm2 = ld(elem2_vaddr);
    warm3 = ld(elem3_vaddr);
    TEST_ASSERT("min escape probe prewarms later element translations while leaving the shared @/element0 page cold in HS mode",
        excpt.triggered == false &&
        warm2 == elem2_seed &&
        warm3 == elem3_seed
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_load_sentinel_to_v6((const uint64_t *)dump_vaddr);
    ai_vdep_load_index_offsets_to_v8((const uint64_t *)offsets_vaddr);
    TEST_ASSERT("min escape probe prepares vector registers without warming the shared @/element0 page in HS mode",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    at_observed = ai_vdep_chain_store_addr_dep_vload_min_landing_dump_flow(at_vaddr,
        hash_data_seed,
        vector_base, (const uint64_t *)dump_vaddr,
        (const uint64_t *)offsets_vaddr, (const uint64_t *)pressure_vaddr,
        (uint64_t *)dump_vaddr, &observed_vstart);
    fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    TEST_ASSERT("min escape probe still makes # depend on a flushed @ pointer chain and faults at vector element1 with element0 as committed prefix",
        at_observed == elem0_vaddr &&
        fault_triggered == true &&
        fault_cause == CAUSE_LAF &&
        fault_tval == fault1_vaddr &&
        observed_vstart == 1ULL,
        "at_observed=0x%llx expected_elem0=0x%llx cause=0x%llx tval=0x%llx expected_tval=0x%llx vstart=0x%llx",
        (unsigned long long)at_observed,
        (unsigned long long)elem0_vaddr,
        (unsigned long long)fault_cause,
        (unsigned long long)fault_tval,
        (unsigned long long)fault1_vaddr,
        (unsigned long long)observed_vstart
    );

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    dump0_observed = read64(dump_paddr + 0U * sizeof(uint64_t));
    dump1_observed = read64(dump_paddr + 1U * sizeof(uint64_t));
    dump2_observed = read64(dump_paddr + 2U * sizeof(uint64_t));
    dump3_observed = read64(dump_paddr + 3U * sizeof(uint64_t));
    elem0_backing_observed = read64(elem0_paddr);
    fault1_backing_observed = read64(fault1_paddr);
    elem2_backing_observed = read64(elem2_paddr);
    elem3_backing_observed = read64(elem3_paddr);
    at_backing_observed = read64(at_paddr);
    addr_chain0_observed = read64(addr_chain_paddr);
    addr_chain_tail_observed = read64(addr_chain_paddr +
        (AI_VDEP_ESCAPE_MIN_ADDR_CHAIN_LINKS - 1U) * AI_VDEP_RAW_ADDR_CHAIN_STRIDE);
    pressure0_observed = read64(pressure_paddr);
    pressure_tail_observed = read64(pressure_paddr +
        (AI_VDEP_ESCAPE_MIN_PRESSURE_LOADS - 1U) * sizeof(uint64_t));

    TEST_ASSERT("min escape probe vector dump element0 must see the older address-dependent store # value",
        dump0_observed == hash_data_seed,
        "dump0_actual=0x%llx expected_hash=0x%llx elem0_old=0x%llx",
        (unsigned long long)dump0_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)elem0_old
    );
    TEST_ASSERT("min escape probe elements at and after the first fault keep the sentinel image in the dumped vector register",
        dump1_observed == sentinel &&
        dump2_observed == sentinel &&
        dump3_observed == sentinel,
        "dump_tail_actual={0x%llx,0x%llx,0x%llx} sentinel=0x%llx",
        (unsigned long long)dump1_observed,
        (unsigned long long)dump2_observed,
        (unsigned long long)dump3_observed,
        (unsigned long long)sentinel
    );
    TEST_ASSERT("min escape probe element0 backing memory is updated by the dependent store # while non-target data remains stable",
        elem0_backing_observed == hash_data_seed &&
        fault1_backing_observed == fault1_seed &&
        elem2_backing_observed == elem2_seed &&
        elem3_backing_observed == elem3_seed,
        "elem0=0x%llx expected=0x%llx fault1=0x%llx elem2=0x%llx elem3=0x%llx",
        (unsigned long long)elem0_backing_observed,
        (unsigned long long)hash_data_seed,
        (unsigned long long)fault1_backing_observed,
        (unsigned long long)elem2_backing_observed,
        (unsigned long long)elem3_backing_observed
    );
    TEST_ASSERT("min escape probe @, chain, and pressure backing data keep the reduced delay structure intact",
        at_backing_observed == at_addr_seed &&
        addr_chain0_observed == addr_chain_vaddr + AI_VDEP_RAW_ADDR_CHAIN_STRIDE &&
        addr_chain_tail_observed == elem0_vaddr &&
        pressure0_observed == pressure_seed &&
        pressure_tail_observed == pressure_seed + AI_VDEP_ESCAPE_MIN_PRESSURE_LOADS - 1U,
        "at=0x%llx chain0=0x%llx chain_tail=0x%llx pressure0=0x%llx pressure_tail=0x%llx",
        (unsigned long long)at_backing_observed,
        (unsigned long long)addr_chain0_observed,
        (unsigned long long)addr_chain_tail_observed,
        (unsigned long long)pressure0_observed,
        (unsigned long long)pressure_tail_observed
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_vloxei_elem0_first_page_fault_vstart0_corner()
{
    TEST_START();

    const enum test_page elem0_page = SWITCH1;
    const enum test_page elem1_page = VSRWX_GRWX;
    const enum test_page elem2_page = SWITCH2;
    const enum test_page elem3_page = SCRATCHPAD;
    const enum test_page scratch_page = VSRW_GRWX;
    const uintptr_t vector_base = hs_page_base(elem1_page);
    const uintptr_t elem0_vaddr = hs_page_base(elem0_page) + 0x100U;
    const uintptr_t elem1_vaddr = hs_page_base(elem1_page) + 0x180U;
    const uintptr_t elem2_vaddr = hs_page_base(elem2_page) + 0x200U;
    const uintptr_t elem3_vaddr = hs_page_base(elem3_page) + 0x280U;
    const uintptr_t elem0_paddr = phys_page_base(elem0_page) + 0x100U;
    const uintptr_t elem1_paddr = phys_page_base(elem1_page) + 0x180U;
    const uintptr_t elem2_paddr = phys_page_base(elem2_page) + 0x200U;
    const uintptr_t elem3_paddr = phys_page_base(elem3_page) + 0x280U;
    const uintptr_t offsets_vaddr = hs_page_base(scratch_page) + 0x080U;
    const uintptr_t offsets_paddr = phys_page_base(scratch_page) + 0x080U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0x180U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0x180U;
    const uint64_t elem0_seed = 0x1011121314151617ULL;
    const uint64_t elem1_seed = 0x2122232425262728ULL;
    const uint64_t elem2_seed = 0x3233343536373839ULL;
    const uint64_t elem3_seed = 0x434445464748494aULL;
    const uint64_t sentinel = 0xfeedfacecafebeefULL;
    bool fault_triggered = false;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_epc = 0ULL;
    uintptr_t vector_fault_pc = 0ULL;
    uint64_t observed_vstart = ~0ULL;
    uint64_t observed_vl = 0ULL;
    uint64_t dump0_observed = 0ULL;
    uint64_t dump1_observed = 0ULL;
    uint64_t dump2_observed = 0ULL;
    uint64_t dump3_observed = 0ULL;
    uint64_t repaired_dump0_observed = 0ULL;
    uint64_t repaired_dump1_observed = 0ULL;
    uint64_t repaired_dump2_observed = 0ULL;
    uint64_t repaired_dump3_observed = 0ULL;
    uint64_t elem0_backing_observed = 0ULL;
    uint64_t elem1_backing_observed = 0ULL;
    uint64_t elem2_backing_observed = 0ULL;
    uint64_t elem3_backing_observed = 0ULL;

    goto_priv(PRIV_M);
    ai_vdep_restore_pmp_full_access();
    ai_vdep_enable_vector_state();
    CSRC(CSR_MSTATUS, MSTATUS_MXR);
    write64(elem0_paddr, elem0_seed);
    write64(elem1_paddr, elem1_seed);
    write64(elem2_paddr, elem2_seed);
    write64(elem3_paddr, elem3_seed);
    write64(offsets_paddr + 0U * sizeof(uint64_t), elem0_vaddr - vector_base);
    write64(offsets_paddr + 1U * sizeof(uint64_t), elem1_vaddr - vector_base);
    write64(offsets_paddr + 2U * sizeof(uint64_t), elem2_vaddr - vector_base);
    write64(offsets_paddr + 3U * sizeof(uint64_t), elem3_vaddr - vector_base);
    for (unsigned i = 0; i < AI_VDEP_ELEMS; ++i) {
        write64(dump_paddr + i * sizeof(uint64_t), sentinel);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_vdep_set_hs_leaf(elem0_page, PTE_V | PTE_AD | PTE_X);
    ai_vdep_set_hs_leaf(elem1_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem2_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(elem3_page, PTE_V | PTE_AD | PTE_RWX);
    ai_vdep_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    (void)ld(elem1_vaddr);
    (void)ld(elem2_vaddr);
    (void)ld(elem3_vaddr);
    TEST_ASSERT("elem0 first-fault probe prewarms only the non-fault vector element translations",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_load_sentinel_to_v6((const uint64_t *)dump_vaddr);
    ai_vdep_load_index_offsets_to_v8((const uint64_t *)offsets_vaddr);
    TEST_ASSERT("elem0 first-fault probe prepares vector source registers without touching the no-read element0 leaf",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vector_fault_pc = ai_vdep_indexed_vload_v6_m2_capture_pc_flow(vector_base,
        (const uint64_t *)dump_vaddr, (const uint64_t *)offsets_vaddr);
    fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    fault_epc = excpt.epc;
    observed_vstart = CSRR(CSR_VSTART);
    observed_vl = CSRR(CSR_VL);

    TEST_ASSERT("indexed vector load first element raises load page fault with vstart=0 and no committed prefix",
        fault_triggered == true &&
        fault_cause == CAUSE_LPF &&
        fault_tval == elem0_vaddr &&
        fault_epc == vector_fault_pc &&
        observed_vstart == 0ULL &&
        observed_vl == AI_VDEP_ELEMS,
        "triggered=%d cause=0x%llx tval=0x%llx expected_tval=0x%llx vstart=0x%llx vl=0x%llx epc=0x%llx vector_pc=0x%llx",
        fault_triggered,
        (unsigned long long)fault_cause,
        (unsigned long long)fault_tval,
        (unsigned long long)elem0_vaddr,
        (unsigned long long)observed_vstart,
        (unsigned long long)observed_vl,
        (unsigned long long)fault_epc,
        (unsigned long long)vector_fault_pc
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_store_v6_m2((uint64_t *)dump_vaddr);
    TEST_ASSERT("after clearing vstart, the faulted destination register can be dumped for no-prefix checking",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    dump0_observed = read64(dump_paddr + 0U * sizeof(uint64_t));
    dump1_observed = read64(dump_paddr + 1U * sizeof(uint64_t));
    dump2_observed = read64(dump_paddr + 2U * sizeof(uint64_t));
    dump3_observed = read64(dump_paddr + 3U * sizeof(uint64_t));
    elem0_backing_observed = read64(elem0_paddr);
    elem1_backing_observed = read64(elem1_paddr);
    elem2_backing_observed = read64(elem2_paddr);
    elem3_backing_observed = read64(elem3_paddr);

    TEST_ASSERT("elem0 first-fault leaves every destination element at the sentinel because no prefix element retired",
        dump0_observed == sentinel &&
        dump1_observed == sentinel &&
        dump2_observed == sentinel &&
        dump3_observed == sentinel,
        "dump={0x%llx,0x%llx,0x%llx,0x%llx} sentinel=0x%llx",
        (unsigned long long)dump0_observed,
        (unsigned long long)dump1_observed,
        (unsigned long long)dump2_observed,
        (unsigned long long)dump3_observed,
        (unsigned long long)sentinel
    );
    TEST_ASSERT("elem0 first-fault probe keeps backing memory stable for faulting and non-faulting pages",
        elem0_backing_observed == elem0_seed &&
        elem1_backing_observed == elem1_seed &&
        elem2_backing_observed == elem2_seed &&
        elem3_backing_observed == elem3_seed,
        "backing={0x%llx,0x%llx,0x%llx,0x%llx}",
        (unsigned long long)elem0_backing_observed,
        (unsigned long long)elem1_backing_observed,
        (unsigned long long)elem2_backing_observed,
        (unsigned long long)elem3_backing_observed
    );

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_VDEP_ELEMS; ++i) {
        write64(dump_paddr + i * sizeof(uint64_t), sentinel);
    }

    goto_priv(PRIV_HS);
    ai_vdep_set_hs_leaf(elem0_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vdep_indexed_vload_v6_m2_flow(vector_base,
        (const uint64_t *)dump_vaddr, (const uint64_t *)offsets_vaddr);
    ai_vdep_store_v6_m2((uint64_t *)dump_vaddr);
    TEST_ASSERT("after repairing the element0 leaf, replaying the same indexed vector load completes and dumps the full element group",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    repaired_dump0_observed = read64(dump_paddr + 0U * sizeof(uint64_t));
    repaired_dump1_observed = read64(dump_paddr + 1U * sizeof(uint64_t));
    repaired_dump2_observed = read64(dump_paddr + 2U * sizeof(uint64_t));
    repaired_dump3_observed = read64(dump_paddr + 3U * sizeof(uint64_t));
    TEST_ASSERT("the repaired indexed vector load writes back all four indexed element payloads without keeping the old sentinel image",
        repaired_dump0_observed == elem0_seed &&
        repaired_dump1_observed == elem1_seed &&
        repaired_dump2_observed == elem2_seed &&
        repaired_dump3_observed == elem3_seed,
        "repaired_dump={0x%llx,0x%llx,0x%llx,0x%llx} expected={0x%llx,0x%llx,0x%llx,0x%llx}",
        (unsigned long long)repaired_dump0_observed,
        (unsigned long long)repaired_dump1_observed,
        (unsigned long long)repaired_dump2_observed,
        (unsigned long long)repaired_dump3_observed,
        (unsigned long long)elem0_seed,
        (unsigned long long)elem1_seed,
        (unsigned long long)elem2_seed,
        (unsigned long long)elem3_seed
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}
