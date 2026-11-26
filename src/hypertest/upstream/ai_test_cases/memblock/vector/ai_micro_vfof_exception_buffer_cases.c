#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <csrs.h>
#include <stdint.h>

#define AI_VFOF_EB_FULL_VL 16U
#define AI_VFOF_EB_LOW_BYTES 8U
#define AI_VFOF_EB_DUMP_BYTES 64U
#define AI_VFOF_EB_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_VFOF_EB_MCONTROL6_LOAD 0x1ULL

typedef struct {
    bool triggered;
    uint64_t cause;
    uint64_t tval;
} ai_vfof_eb_excpt_snapshot_t;

static inline ai_vfof_eb_excpt_snapshot_t ai_vfof_eb_snapshot_excpt(void)
{
    ai_vfof_eb_excpt_snapshot_t snapshot = {
        .triggered = excpt.triggered,
        .cause = excpt.cause,
        .tval = excpt.tval,
    };
    return snapshot;
}

static inline void ai_vfof_eb_clear_mcontrol6_slot(unsigned slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0ULL);
}

static inline void ai_vfof_eb_arm_mcontrol6_slot(unsigned slot,
    uintptr_t addr, uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, AI_VFOF_EB_MCONTROL6_MSU_BASE | access_bits);
}

static inline uint64_t ai_vfof_eb_enable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
    return saved_mstatus;
}

static inline void ai_vfof_eb_restore_breakpoint_delivery(
    uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline pte_t *ai_vfof_eb_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline pte_t *ai_vfof_eb_gstage_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hpt[5][page];
#else
    return &hpt[2][page];
#endif
}

static inline void ai_vfof_eb_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_vfof_eb_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_vfof_eb_set_gstage_leaf(enum test_page guest_page,
    enum test_page phys_page, uint64_t flags)
{
    *ai_vfof_eb_gstage_leaf_entry(guest_page) =
        (phys_page_base(phys_page) >> 2) | flags;
}

static inline uint64_t ai_vfof_eb_guest_expected_tval2(uintptr_t vaddr)
{
    return ((uint64_t)vaddr) >> 2;
}

static inline void ai_vfof_eb_prepare_vs_guest_env(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    hpt_init();
    vspt_init();
    sfence_vma();
    hfence();
}

static inline void ai_vfof_eb_cleanup_vs_guest_env(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_HGATP, 0ULL);
    CSRW(CSR_VSATP, 0ULL);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();
    hfence();
}

static inline void ai_vfof_eb_enable_vs_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 3ULL << 9);
    CSRS(CSR_VSSTATUS, 3ULL << 9);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_vfof_eb_prepare_three_pages(enum test_page p0,
    uint64_t f0, enum test_page p1, uint64_t f1, enum test_page p2,
    uint64_t f2)
{
    goto_priv(PRIV_HS);
    hspt_init();
    ai_vfof_eb_set_hs_leaf(p0, f0);
    ai_vfof_eb_set_hs_leaf(p1, f1);
    ai_vfof_eb_set_hs_leaf(p2, f2);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_vfof_eb_prepare_four_pages(enum test_page p0,
    uint64_t f0, enum test_page p1, uint64_t f1, enum test_page p2,
    uint64_t f2, enum test_page p3, uint64_t f3)
{
    goto_priv(PRIV_HS);
    hspt_init();
    ai_vfof_eb_set_hs_leaf(p0, f0);
    ai_vfof_eb_set_hs_leaf(p1, f1);
    ai_vfof_eb_set_hs_leaf(p2, f2);
    ai_vfof_eb_set_hs_leaf(p3, f3);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_vfof_eb_cleanup_hs(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();
}

static inline void ai_vfof_eb_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 3ULL << 9);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_vfof_eb_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline void ai_vfof_eb_fill_bytes(uintptr_t paddr, uint8_t value,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, value);
    }
}

static inline bool ai_vfof_eb_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline bool ai_vfof_eb_bytes_are(uintptr_t paddr, uint8_t value,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != value) {
            return false;
        }
    }
    return true;
}

static inline void ai_vfof_eb_vle8ff_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t dump_vaddr, unsigned req_vl,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_vfof_eb_vle32ff_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t dump_vaddr, unsigned req_vl,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle32ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vse32.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_vfof_eb_vle8ff_then_scalar_load_fault(
    uintptr_t src_vaddr, uintptr_t fault_vaddr, unsigned req_vl,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;
    uint64_t sink = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "ld %[sink], 0(%[fault_addr])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [sink] "=&r"(sink)
        : [src] "r"(src_vaddr),
          [fault_addr] "r"(fault_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_vfof_eb_vs_vle8ff_then_scalar_load_fault(
    uintptr_t src_vaddr, uintptr_t fault_vaddr, unsigned req_vl,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;
    uint64_t sink = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "ld %[sink], 0(%[fault_addr])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [sink] "=&r"(sink)
        : [src] "r"(src_vaddr),
          [fault_addr] "r"(fault_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_vfof_eb_vle8ff_then_scalar_store_fault(
    uintptr_t src_vaddr, uintptr_t fault_vaddr, unsigned req_vl,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;
    const uint64_t store_value = 0x5a5a5a5adeadbeefULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "sd %[value], 0(%[fault_addr])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [src] "r"(src_vaddr),
          [fault_addr] "r"(fault_vaddr),
          [value] "r"(store_value),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_vfof_eb_vle8ff_then_vector_load_fault(
    uintptr_t src_vaddr, uintptr_t fault_vaddr, unsigned req_vl,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vsetvli t0, %[vec_vl], e32, m1, tu, mu\n\t"
        "vle32.v v16, (%[fault_addr])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [src] "r"(src_vaddr),
          [fault_addr] "r"(fault_vaddr),
          [vl] "r"(req_vl),
          [vec_vl] "r"(4U)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_vfof_eb_vle8ff_then_vector_store_fault(
    uintptr_t src_vaddr, uintptr_t store_src_vaddr, uintptr_t fault_vaddr,
    unsigned req_vl, unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vsetvli t0, %[vec_vl], e32, m1, tu, mu\n\t"
        "vle32.v v16, (%[store_src])\n\t"
        "vse32.v v16, (%[fault_addr])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [src] "r"(src_vaddr),
          [store_src] "r"(store_src_vaddr),
          [fault_addr] "r"(fault_vaddr),
          [vl] "r"(req_vl),
          [vec_vl] "r"(4U)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_vfof_eb_vle8ff_then_amo_fault(uintptr_t src_vaddr,
    uintptr_t fault_vaddr, unsigned req_vl, unsigned *observed_vl)
{
    unsigned captured_vl = 0U;
    uint64_t amo_value = 0x0102030405060708ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "amoadd.d %[value], %[value], 0(%[fault_addr])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [value] "+r"(amo_value)
        : [src] "r"(src_vaddr),
          [fault_addr] "r"(fault_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_vfof_eb_dual_vle8ff_then_scalar_load_fault(
    uintptr_t first_src_vaddr, uintptr_t second_src_vaddr,
    uintptr_t fault_vaddr, unsigned req_vl, unsigned *first_vl,
    unsigned *second_vl)
{
    unsigned captured_first_vl = 0U;
    unsigned captured_second_vl = 0U;
    uint64_t sink = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[first_src])\n\t"
        "csrr %[first_out_vl], vl\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v16, (%[second_src])\n\t"
        "csrr %[second_out_vl], vl\n\t"
        "ld %[sink], 0(%[fault_addr])\n\t"
        ".option pop\n\t"
        : [first_out_vl] "=&r"(captured_first_vl),
          [second_out_vl] "=&r"(captured_second_vl),
          [sink] "=&r"(sink)
        : [first_src] "r"(first_src_vaddr),
          [second_src] "r"(second_src_vaddr),
          [fault_addr] "r"(fault_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *first_vl = captured_first_vl;
    *second_vl = captured_second_vl;
}

static inline void ai_vfof_eb_vleff_then_scalar_load_fault(
    unsigned eew_bytes, uintptr_t src_vaddr, uintptr_t fault_vaddr,
    unsigned req_vl, unsigned *observed_vl)
{
    unsigned captured_vl = 0U;
    uint64_t sink = 0U;

    if (eew_bytes == 1U) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle8ff.v v8, (%[src])\n\t"
            "csrr %[out_vl], vl\n\t"
            "ld %[sink], 0(%[fault_addr])\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r"(captured_vl),
              [sink] "=&r"(sink)
            : [src] "r"(src_vaddr),
              [fault_addr] "r"(fault_vaddr),
              [vl] "r"(req_vl)
            : "t0", "memory");
    } else if (eew_bytes == 2U) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle16ff.v v8, (%[src])\n\t"
            "csrr %[out_vl], vl\n\t"
            "ld %[sink], 0(%[fault_addr])\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r"(captured_vl),
              [sink] "=&r"(sink)
            : [src] "r"(src_vaddr),
              [fault_addr] "r"(fault_vaddr),
              [vl] "r"(req_vl)
            : "t0", "memory");
    } else if (eew_bytes == 4U) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle32ff.v v8, (%[src])\n\t"
            "csrr %[out_vl], vl\n\t"
            "ld %[sink], 0(%[fault_addr])\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r"(captured_vl),
              [sink] "=&r"(sink)
            : [src] "r"(src_vaddr),
              [fault_addr] "r"(fault_vaddr),
              [vl] "r"(req_vl)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle64ff.v v8, (%[src])\n\t"
            "csrr %[out_vl], vl\n\t"
            "ld %[sink], 0(%[fault_addr])\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r"(captured_vl),
              [sink] "=&r"(sink)
            : [src] "r"(src_vaddr),
              [fault_addr] "r"(fault_vaddr),
              [vl] "r"(req_vl)
            : "t0", "memory");
    }

    *observed_vl = captured_vl;
}

static inline void ai_vfof_eb_vlseg2e8ff_then_scalar_load_fault(
    uintptr_t src_vaddr, uintptr_t fault_vaddr, unsigned req_vl,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;
    uint64_t sink = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "ld %[sink], 0(%[fault_addr])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart),
          [sink] "=&r"(sink)
        : [src] "r"(src_vaddr),
          [fault_addr] "r"(fault_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_vfof_eb_masked_vle8ff_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t mask_vaddr, uintptr_t dump_vaddr,
    unsigned req_vl, unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [mask] "r"(mask_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_vfof_eb_vstart_vle8ff_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t dump_vaddr, unsigned req_vl,
    unsigned start_index, unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, %[start]\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl),
          [start] "r"(start_index)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_vfof_eb_vle8ff_element0_fault(uintptr_t src_vaddr,
    unsigned req_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");
}

static inline void ai_vfof_eb_vfadd_e8_illegal(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e8, m1, tu, mu\n\t"
        "vfadd.vv v8, v8, v8\n\t"
        ".option pop\n\t"
        :
        :
        : "t0", "memory");
}

bool ai_micro_hs_vfof_later_fault_candidates_keep_first_vl_once_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 2U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 2U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x140U;
    const uint8_t prefix[2] = { 0x31U, 0x42U };
    const uint8_t seed[AI_VFOF_EB_FULL_VL] = {
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf
    };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    bool vfof_visible_trap = false;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    ai_vfof_eb_write_bytes(low_paddr, prefix, sizeof(prefix));
    ai_vfof_eb_write_bytes(seed_paddr, seed, sizeof(seed));
    ai_vfof_eb_fill_bytes(dump_paddr, 0x5cU, AI_VFOF_EB_FULL_VL);

    ai_vfof_eb_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        AI_VFOF_EB_FULL_VL, &observed_vl, &observed_vstart);
    vfof_visible_trap = excpt.triggered;

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-31 VFOF keeps vl at the first nonzero fault candidate instead of letting later fault candidates overwrite it",
        vfof_visible_trap == false &&
        observed_vl == 2U &&
        observed_vstart == 0U &&
        ai_vfof_eb_bytes_match(dump_paddr, prefix, sizeof(prefix)) &&
        ai_vfof_eb_bytes_match(dump_paddr + 2U, seed + 2U,
            AI_VFOF_EB_FULL_VL - 2U),
        "vl=%u vstart=%u suffix2=0x%x suffix5=0x%x",
        observed_vl, observed_vstart, read8(dump_paddr + 2U),
        read8(dump_paddr + 5U)
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_masked_off_fault_skips_to_first_active_fault_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 2U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x200U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 2U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x200U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x280U;
    const uint8_t prefix[2] = { 0x52U, 0x63U };
    const uint8_t seed[AI_VFOF_EB_FULL_VL] = {
        0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
        0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf
    };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    ai_vfof_eb_write_bytes(low_paddr, prefix, sizeof(prefix));
    ai_vfof_eb_write_bytes(seed_paddr, seed, sizeof(seed));
    write8(mask_paddr, 0x13U);
    write8(mask_paddr + 1U, 0x00U);
    ai_vfof_eb_fill_bytes(dump_paddr, 0x6dU, AI_VFOF_EB_FULL_VL);

    ai_vfof_eb_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_masked_vle8ff_full_dump(seed_vaddr, src_vaddr, mask_vaddr,
        dump_vaddr, AI_VFOF_EB_FULL_VL, &observed_vl, &observed_vstart);
    const ai_vfof_eb_excpt_snapshot_t vfof_excpt =
        ai_vfof_eb_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-31 masked VFOF ignores masked-off faulting addresses and reduces vl only at the first active faulting element",
        vfof_excpt.triggered == false &&
        observed_vl == 4U &&
        observed_vstart == 0U &&
        read8(dump_paddr) == prefix[0] &&
        read8(dump_paddr + 1U) == prefix[1] &&
        read8(dump_paddr + 2U) == seed[2] &&
        read8(dump_paddr + 3U) == seed[3] &&
        ai_vfof_eb_bytes_match(dump_paddr + 4U, seed + 4U,
            AI_VFOF_EB_FULL_VL - 4U),
        "vl=%u lane2=0x%x lane4=0x%x", observed_vl,
        read8(dump_paddr + 2U), read8(dump_paddr + 4U)
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_e32_suffix_payload_not_valid_after_first_fault_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x2c0U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x2c0U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x300U;
    const uint32_t seed_words[8] = {
        0xaaa00000U, 0xbbb00001U, 0xccc00002U, 0xddd00003U,
        0xeee00004U, 0xfff00005U, 0x12340006U, 0x56780007U
    };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    write32(low_paddr, 0x11112222U);
    write32(low_paddr + 4U, 0x33334444U);
    for (unsigned i = 0; i < 8U; ++i) {
        write32(seed_paddr + 4U * i, seed_words[i]);
        write32(dump_paddr + 4U * i, 0x5a5a0000U + i);
        write32(high_paddr + 4U * i, 0x90000000U + i);
    }

    ai_vfof_eb_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle32ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr, 8U,
        &observed_vl, &observed_vstart);
    const ai_vfof_eb_excpt_snapshot_t vfof_excpt =
        ai_vfof_eb_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-31 vle32ff first nonzero fault shortens vl at element2 and prevents suffix backing payload from becoming valid vector data",
        vfof_excpt.triggered == false &&
        observed_vl == 2U &&
        observed_vstart == 0U &&
        read32(dump_paddr) == 0x11112222U &&
        read32(dump_paddr + 4U) == 0x33334444U &&
        read32(dump_paddr + 8U) == seed_words[2] &&
        read32(dump_paddr + 12U) == seed_words[3] &&
        read32(dump_paddr + 20U) == 0x5a5a0005U,
        "vl=%u vstart=%u suffix2=0x%x fill5=0x%x",
        observed_vl, observed_vstart, read32(dump_paddr + 8U),
        read32(dump_paddr + 20U)
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_vstart_nonzero_unmasked_first_fault_absolute_index_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 5U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x340U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 5U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x340U;
    const uint8_t seed[AI_VFOF_EB_FULL_VL] = {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
    };
    const unsigned start_index = 3U;
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    ai_vfof_eb_write_bytes(seed_paddr, seed, sizeof(seed));
    for (unsigned i = 0; i < 5U; ++i) {
        write8(low_paddr + i, (uint8_t)(0x30U + i));
    }
    ai_vfof_eb_fill_bytes(dump_paddr, 0x7eU, AI_VFOF_EB_FULL_VL);

    ai_vfof_eb_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vstart_vle8ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        AI_VFOF_EB_FULL_VL, start_index, &observed_vl, &observed_vstart);
    const ai_vfof_eb_excpt_snapshot_t vfof_excpt =
        ai_vfof_eb_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-31 vstart!=0 VFOF reports the absolute architectural first-fault element rather than a vstart-relative index",
        vfof_excpt.triggered == false &&
        observed_vl == 5U &&
        observed_vstart == 0U &&
        ai_vfof_eb_bytes_match(dump_paddr, seed, start_index) &&
        read8(dump_paddr + 3U) == 0x33U &&
        read8(dump_paddr + 4U) == 0x34U &&
        ai_vfof_eb_bytes_match(dump_paddr + 5U, seed + 5U,
            AI_VFOF_EB_FULL_VL - 5U),
        "vl=%u vstart=%u lane3=0x%x lane5=0x%x", observed_vl,
        observed_vstart, read8(dump_paddr + 3U), read8(dump_paddr + 5U)
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_element0_fault_visible_exception_control_corner()
{
    TEST_START();

    const enum test_page fault_page = SWITCH2;
    const uintptr_t src_vaddr = hs_page_base(fault_page) + 0x80U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_vfof_eb_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_element0_fault(src_vaddr, AI_VFOF_EB_FULL_VL);

    TEST_ASSERT("AP-31/AP-46 element0 VFOF fault remains a visible load page fault and is not converted into a silent partial completion",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == src_vaddr &&
        CSRR(CSR_VSTART) == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, src_vaddr, CSRR(CSR_VSTART)
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_nonzero_pf_then_scalar_load_fault_refreshes_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page load_page = VSRW_GRWX;
    const uintptr_t src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t later_fault_vaddr = hs_page_base(load_page) + 0x140U;
    const uintptr_t fof_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t later_fault_paddr = phys_page_base(load_page) + 0x140U;
    const uint8_t payload[AI_VFOF_EB_LOW_BYTES] = {
        0x08, 0x19, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f
    };
    unsigned observed_vl = 0U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    ai_vfof_eb_write_bytes(low_paddr, payload, sizeof(payload));
    write64(later_fault_paddr, 0x0123456789abcdefULL);

    ai_vfof_eb_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        load_page, PTE_V | PTE_AD | PTE_X);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_then_scalar_load_fault(src_vaddr, later_fault_vaddr,
        AI_VFOF_EB_FULL_VL, &observed_vl);

    TEST_ASSERT("AP-46 silent nonzero VFOF page fault followed by scalar load fault refreshes tval to the later load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == later_fault_vaddr &&
        excpt.tval != fof_fault_vaddr &&
        observed_vl == AI_VFOF_EB_LOW_BYTES,
        "cause=0x%lx tval=0x%lx later=0x%lx fof=0x%lx vl=%u",
        excpt.cause, excpt.tval, later_fault_vaddr, fof_fault_vaddr,
        observed_vl
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_nonzero_pf_then_scalar_store_fault_refreshes_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page store_page = VSRW_GRWX;
    const uintptr_t src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x180U;
    const uintptr_t low_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x180U;
    const uint8_t payload[AI_VFOF_EB_LOW_BYTES] = {
        0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87
    };
    unsigned observed_vl = 0U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    ai_vfof_eb_write_bytes(low_paddr, payload, sizeof(payload));
    write64(store_paddr, 0x1122334455667788ULL);

    ai_vfof_eb_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        store_page, PTE_V | PTE_AD | PTE_R);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_then_scalar_store_fault(src_vaddr, store_vaddr,
        AI_VFOF_EB_FULL_VL, &observed_vl);

    TEST_ASSERT("AP-46 silent nonzero VFOF page fault followed by scalar store fault refreshes cause and tval to the later store address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr &&
        observed_vl == AI_VFOF_EB_LOW_BYTES,
        "cause=0x%lx tval=0x%lx store=0x%lx vl=%u",
        excpt.cause, excpt.tval, store_vaddr, observed_vl
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-46 scalar-store follow-up fault leaves the store target unchanged while the earlier silent VFOF only shortened vl",
        read64(store_paddr) == 0x1122334455667788ULL
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_vs_vfof_nonzero_guest_fault_then_guest_load_fault_tval2_refresh_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page fof_fault_page = SWITCH2;
    const enum test_page later_fault_page = VSRWX_GI;
    const uintptr_t src_vaddr =
        vs_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t fof_fault_vaddr = vs_page_base(fof_fault_page);
    const uintptr_t later_fault_vaddr =
        vs_page_base(later_fault_page) + 0x240U;
    const uintptr_t low_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uint8_t payload[AI_VFOF_EB_LOW_BYTES] = {
        0x41, 0x52, 0x63, 0x74, 0x85, 0x96, 0xa7, 0xb8
    };
    unsigned observed_vl = 0U;

    ai_vfof_eb_enable_vs_vector_state();
    goto_priv(PRIV_M);
    ai_vfof_eb_write_bytes(low_paddr, payload, sizeof(payload));
    ai_vfof_eb_prepare_vs_guest_env();

    goto_priv(PRIV_M);
    ai_vfof_eb_set_gstage_leaf(low_page, low_page,
        PTE_V | PTE_U | PTE_AD | PTE_RWX);
    *ai_vfof_eb_gstage_leaf_entry(fof_fault_page) = 0ULL;
    *ai_vfof_eb_gstage_leaf_entry(later_fault_page) = 0ULL;
    sfence_vma();
    hfence();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vs_vle8ff_then_scalar_load_fault(src_vaddr,
        later_fault_vaddr, AI_VFOF_EB_FULL_VL, &observed_vl);

    TEST_ASSERT("AP-46K VS VFOF nonzero guest fault is silent and the following guest-page-faulting scalar load refreshes both stval and htval/mtval2",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == later_fault_vaddr &&
        excpt.tval2 ==
            ai_vfof_eb_guest_expected_tval2(later_fault_vaddr) &&
        excpt.tval != fof_fault_vaddr &&
        excpt.tval2 !=
            ai_vfof_eb_guest_expected_tval2(fof_fault_vaddr) &&
        observed_vl == AI_VFOF_EB_LOW_BYTES,
        "priv=%u cause=0x%lx tval=0x%lx tval2=0x%lx later=0x%lx later_tval2=0x%lx fof=0x%lx fof_tval2=0x%lx vl=%u",
        excpt.priv, excpt.cause, excpt.tval, excpt.tval2,
        later_fault_vaddr,
        ai_vfof_eb_guest_expected_tval2(later_fault_vaddr),
        fof_fault_vaddr, ai_vfof_eb_guest_expected_tval2(fof_fault_vaddr),
        observed_vl
    );

    ai_vfof_eb_cleanup_vs_guest_env();
    TEST_END();
}

bool ai_micro_hs_vfof_exception_buffer_boundary_eew_matrix_corner()
{
    TEST_START();

    const unsigned eew_list[] = { 1U, 2U, 4U, 8U };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page later_page = VSRW_GRWX;
    const uintptr_t later_fault_vaddr = hs_page_base(later_page) + 0x280U;
    const uintptr_t later_fault_paddr = phys_page_base(later_page) + 0x280U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    write64(later_fault_paddr, 0x9988776655443322ULL);

    for (unsigned round = 0; round < sizeof(eew_list) / sizeof(eew_list[0]);
         ++round) {
        const unsigned eew = eew_list[round];
        const unsigned fault_index = round + 1U;
        const unsigned prefix_bytes = eew * fault_index;
        const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
            prefix_bytes;
        const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
            prefix_bytes;
        unsigned observed_vl = 0U;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < prefix_bytes; ++i) {
            write8(low_paddr + i, (uint8_t)(0x40U + round * 0x20U + i));
        }

        ai_vfof_eb_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
            high_page, PTE_V | PTE_AD | PTE_X,
            later_page, PTE_V | PTE_AD | PTE_X);

        TEST_SETUP_EXCEPT();
        ai_vfof_eb_vleff_then_scalar_load_fault(eew, src_vaddr,
            later_fault_vaddr, AI_VFOF_EB_FULL_VL, &observed_vl);

        TEST_ASSERT("AP-46 EEW/fault-index matrix keeps the silent VFOF exception out of MemExceptionBuffer so the later load fault has fresh tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == later_fault_vaddr &&
            observed_vl == fault_index,
            "round=%u eew=%u cause=0x%lx tval=0x%lx expected=0x%lx vl=%u expected_vl=%u",
            round, eew, excpt.cause, excpt.tval, later_fault_vaddr,
            observed_vl, fault_index
        );
    }

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_nonzero_pf_then_vector_and_amo_fault_refreshes_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page vector_fault_page = VSRW_GRWX;
    const enum test_page amo_fault_page = VSR_GRWX;
    const enum test_page store_src_page = VSRWX_GRWX;
    const uintptr_t src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t vector_fault_vaddr =
        hs_page_base(vector_fault_page) + 0x300U;
    const uintptr_t amo_fault_vaddr = hs_page_base(amo_fault_page) + 0x380U;
    const uintptr_t store_src_vaddr = hs_page_base(store_src_page) + 0x3c0U;
    const uintptr_t low_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t vector_fault_paddr =
        phys_page_base(vector_fault_page) + 0x300U;
    const uintptr_t amo_fault_paddr = phys_page_base(amo_fault_page) + 0x380U;
    const uintptr_t store_src_paddr = phys_page_base(store_src_page) + 0x3c0U;
    const uint8_t payload[AI_VFOF_EB_LOW_BYTES] = {
        0x90, 0x81, 0x72, 0x63, 0x54, 0x45, 0x36, 0x27
    };
    unsigned observed_vl = 0U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    ai_vfof_eb_write_bytes(low_paddr, payload, sizeof(payload));
    for (unsigned i = 0; i < 4U; ++i) {
        write32(store_src_paddr + 4U * i, 0x71000000U + i);
        write32(vector_fault_paddr + 4U * i, 0x82000000U + i);
    }
    write64(amo_fault_paddr, 0xaabbccddeeff0011ULL);

    ai_vfof_eb_prepare_four_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        vector_fault_page, PTE_V | PTE_AD | PTE_X,
        store_src_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_then_vector_load_fault(src_vaddr,
        vector_fault_vaddr, AI_VFOF_EB_FULL_VL, &observed_vl);
    TEST_ASSERT("AP-46 silent VFOF followed by ordinary vector load fault refreshes the exception-buffer metadata to the later vector load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vector_fault_vaddr &&
        observed_vl == AI_VFOF_EB_LOW_BYTES,
        "cause=0x%lx tval=0x%lx vector=0x%lx vl=%u",
        excpt.cause, excpt.tval, vector_fault_vaddr, observed_vl
    );

    ai_vfof_eb_prepare_four_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        vector_fault_page, PTE_V | PTE_AD | PTE_R,
        store_src_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_then_vector_store_fault(src_vaddr, store_src_vaddr,
        vector_fault_vaddr, AI_VFOF_EB_FULL_VL, &observed_vl);
    TEST_ASSERT("AP-46 silent VFOF followed by ordinary vector store fault refreshes the exception-buffer metadata to the later vector store address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vector_fault_vaddr &&
        observed_vl == AI_VFOF_EB_LOW_BYTES,
        "cause=0x%lx tval=0x%lx vector_store=0x%lx vl=%u",
        excpt.cause, excpt.tval, vector_fault_vaddr, observed_vl
    );

    ai_vfof_eb_prepare_four_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        vector_fault_page, PTE_V | PTE_AD | PTE_RWX,
        amo_fault_page, PTE_V | PTE_AD | PTE_R);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_then_amo_fault(src_vaddr, amo_fault_vaddr,
        AI_VFOF_EB_FULL_VL, &observed_vl);
    TEST_ASSERT("AP-46 silent VFOF followed by AMO fault lets the single-port producer report its own store-page-fault tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == amo_fault_vaddr &&
        observed_vl == AI_VFOF_EB_LOW_BYTES,
        "cause=0x%lx tval=0x%lx amo=0x%lx vl=%u",
        excpt.cause, excpt.tval, amo_fault_vaddr, observed_vl
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-46 vector/AMO follow-up faults leave the AMO target backing word unchanged before repair",
        read64(amo_fault_paddr) == 0xaabbccddeeff0011ULL
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_back_to_back_vfof_partial_stops_then_scalar_fault_refreshes_tval_corner()
{
    TEST_START();

    const enum test_page first_low_page = SWITCH1;
    const enum test_page second_low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page later_page = VSRW_GRWX;
    const uintptr_t first_src_vaddr = hs_page_base(first_low_page) + PAGE_SIZE - 4U;
    const uintptr_t second_src_vaddr =
        hs_page_base(second_low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t later_fault_vaddr = hs_page_base(later_page) + 0x4c0U;
    const uintptr_t first_low_paddr =
        phys_page_base(first_low_page) + PAGE_SIZE - 4U;
    const uintptr_t second_low_paddr =
        phys_page_base(second_low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t later_fault_paddr = phys_page_base(later_page) + 0x4c0U;
    unsigned first_vl = 0U;
    unsigned second_vl = 0U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    for (unsigned i = 0; i < 4U; ++i) {
        write8(first_low_paddr + i, (uint8_t)(0x20U + i));
    }
    for (unsigned i = 0; i < AI_VFOF_EB_LOW_BYTES; ++i) {
        write8(second_low_paddr + i, (uint8_t)(0x50U + i));
    }
    write64(later_fault_paddr, 0x8877665544332211ULL);

    ai_vfof_eb_prepare_four_pages(first_low_page, PTE_V | PTE_AD | PTE_RWX,
        second_low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        later_page, PTE_V | PTE_AD | PTE_X);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_dual_vle8ff_then_scalar_load_fault(first_src_vaddr,
        second_src_vaddr, later_fault_vaddr, AI_VFOF_EB_FULL_VL,
        &first_vl, &second_vl);

    TEST_ASSERT("AP-46 back-to-back silent VFOF partial stops do not accumulate stale exception metadata before the following scalar load fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == later_fault_vaddr &&
        first_vl == 4U &&
        second_vl == AI_VFOF_EB_LOW_BYTES,
        "cause=0x%lx tval=0x%lx later=0x%lx first_vl=%u second_vl=%u",
        excpt.cause, excpt.tval, later_fault_vaddr, first_vl, second_vl
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_segment_nonzero_field_fault_then_scalar_fault_refreshes_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page later_page = VSRW_GRWX;
    const uintptr_t seg_src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t later_fault_vaddr = hs_page_base(later_page) + 0x400U;
    const uintptr_t seg_low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t later_fault_paddr = phys_page_base(later_page) + 0x400U;
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    for (unsigned i = 0; i < 7U; ++i) {
        write8(seg_low_paddr + i, (uint8_t)(0x60U + i));
    }
    write64(later_fault_paddr, 0x0102030405060708ULL);

    ai_vfof_eb_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        later_page, PTE_V | PTE_AD | PTE_X);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vlseg2e8ff_then_scalar_load_fault(seg_src_vaddr,
        later_fault_vaddr, AI_VFOF_EB_FULL_VL, &observed_vl,
        &observed_vstart);

    TEST_ASSERT("AP-31/AP-46 segment FOF field fault shortens vl at the element index and does not pollute the following scalar load fault tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == later_fault_vaddr &&
        observed_vl == 3U &&
        observed_vstart == 0U,
        "cause=0x%lx tval=0x%lx later=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, later_fault_vaddr, observed_vl,
        observed_vstart
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_repair_full_reload_then_scalar_fault_no_stale_metadata_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const enum test_page later_page = VSRW_GRWX;
    const uintptr_t src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x500U;
    const uintptr_t later_fault_vaddr = hs_page_base(later_page) + 0x540U;
    const uintptr_t low_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x500U;
    const uintptr_t later_fault_paddr = phys_page_base(later_page) + 0x540U;
    const uint8_t low_payload[AI_VFOF_EB_LOW_BYTES] = {
        0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78
    };
    const uint8_t high_payload[AI_VFOF_EB_LOW_BYTES] = {
        0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0
    };
    uint8_t full_payload[AI_VFOF_EB_FULL_VL];
    unsigned observed_vl = 0U;

    for (unsigned i = 0; i < AI_VFOF_EB_LOW_BYTES; ++i) {
        full_payload[i] = low_payload[i];
        full_payload[i + AI_VFOF_EB_LOW_BYTES] = high_payload[i];
    }

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    ai_vfof_eb_write_bytes(low_paddr, low_payload, sizeof(low_payload));
    ai_vfof_eb_write_bytes(high_paddr, high_payload, sizeof(high_payload));
    ai_vfof_eb_fill_bytes(dump_paddr, 0x8fU, AI_VFOF_EB_FULL_VL);
    write64(later_fault_paddr, 0xfedcba9876543210ULL);

    ai_vfof_eb_prepare_four_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX,
        later_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_full_dump(dump_vaddr, src_vaddr, dump_vaddr,
        AI_VFOF_EB_FULL_VL, &observed_vl, &(unsigned){0});
    TEST_ASSERT("AP-46 repair probe starts with a silent partial VFOF stop before page permissions are restored",
        excpt.triggered == false &&
        observed_vl == AI_VFOF_EB_LOW_BYTES
    );

    ai_vfof_eb_prepare_four_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX,
        later_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_full_dump(dump_vaddr, src_vaddr, dump_vaddr,
        AI_VFOF_EB_FULL_VL, &observed_vl, &(unsigned){0});
    const ai_vfof_eb_excpt_snapshot_t repaired_vfof_excpt =
        ai_vfof_eb_snapshot_excpt();
    goto_priv(PRIV_M);
    TEST_ASSERT("after repairing the FOF high page, the same VFOF reloads the full payload and does not retain the old shortened-vl context",
        repaired_vfof_excpt.triggered == false &&
        observed_vl == AI_VFOF_EB_FULL_VL &&
        ai_vfof_eb_bytes_match(dump_paddr, full_payload,
            AI_VFOF_EB_FULL_VL)
    );

    ai_vfof_eb_prepare_four_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX,
        later_page, PTE_V | PTE_AD | PTE_X);

    TEST_SETUP_EXCEPT();
    (void)ld(later_fault_vaddr);
    TEST_ASSERT("after FOF repair and full reload, a later scalar load fault reports its own tval rather than the old silent FOF address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == later_fault_vaddr,
        "cause=0x%lx tval=0x%lx later=0x%lx",
        excpt.cause, excpt.tval, later_fault_vaddr
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vfof_nonzero_fault_then_address_trigger_breakpoint_priority_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t before_fault_trigger_vaddr = src_vaddr + 3U;
    const uintptr_t same_fault_trigger_vaddr = hs_page_base(high_page);
    const uintptr_t after_fault_trigger_vaddr =
        hs_page_base(high_page) + 4U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x580U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x5c0U;
    const uintptr_t low_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_VFOF_EB_LOW_BYTES;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x580U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x5c0U;
    const uint8_t prefix[AI_VFOF_EB_LOW_BYTES] = {
        0x71, 0x62, 0x53, 0x44, 0x35, 0x26, 0x17, 0x08
    };
    const uint8_t seed[AI_VFOF_EB_FULL_VL] = {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
    };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_vfof_eb_enable_vector_state();
    goto_priv(PRIV_M);
    ai_vfof_eb_clear_mcontrol6_slot(0U);
    ai_vfof_eb_write_bytes(low_paddr, prefix, sizeof(prefix));
    ai_vfof_eb_write_bytes(seed_paddr, seed, sizeof(seed));
    ai_vfof_eb_fill_bytes(dump_paddr, 0x9eU, AI_VFOF_EB_FULL_VL);

    ai_vfof_eb_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        AI_VFOF_EB_FULL_VL, &observed_vl, &observed_vstart);
    TEST_ASSERT("AP-46J control: without trigger, a nonzero-element VFOF page fault is silent, shortens vl, clears vstart, and preserves inactive suffix bytes",
        excpt.triggered == false &&
        observed_vl == AI_VFOF_EB_LOW_BYTES &&
        observed_vstart == 0U &&
        ai_vfof_eb_bytes_match(dump_paddr, prefix, sizeof(prefix)) &&
        ai_vfof_eb_bytes_match(dump_paddr + AI_VFOF_EB_LOW_BYTES,
            seed + AI_VFOF_EB_LOW_BYTES,
            AI_VFOF_EB_FULL_VL - AI_VFOF_EB_LOW_BYTES),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u suffix=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(dump_paddr + AI_VFOF_EB_LOW_BYTES)
    );

    const uint64_t saved_mstatus = ai_vfof_eb_enable_breakpoint_delivery();
    ai_vfof_eb_arm_mcontrol6_slot(0U, before_fault_trigger_vaddr,
        AI_VFOF_EB_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_element0_fault(src_vaddr, AI_VFOF_EB_FULL_VL);
    ai_vfof_eb_excpt_snapshot_t before_trigger_excpt =
        ai_vfof_eb_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-46J VFOF trigger-before-fault priority: a prefix element address trigger is visible before the later nonzero FOF data fault can silently shorten vl",
        before_trigger_excpt.triggered == true &&
        before_trigger_excpt.cause == CAUSE_BKP,
        "triggered=%d cause=0x%lx tval=0x%lx trigger_addr=0x%lx",
        before_trigger_excpt.triggered, before_trigger_excpt.cause,
        before_trigger_excpt.tval, before_fault_trigger_vaddr
    );

    ai_vfof_eb_clear_mcontrol6_slot(0U);
    ai_vfof_eb_arm_mcontrol6_slot(0U, same_fault_trigger_vaddr,
        AI_VFOF_EB_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_element0_fault(src_vaddr, AI_VFOF_EB_FULL_VL);
    ai_vfof_eb_excpt_snapshot_t trigger_excpt =
        ai_vfof_eb_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-46J VFOF nonzero fault and same-element address trigger priority: the debug breakpoint remains visible instead of being killed by the FOF silent-fault gate",
        trigger_excpt.triggered == true &&
        trigger_excpt.cause == CAUSE_BKP,
        "triggered=%d cause=0x%lx tval=0x%lx trigger_addr=0x%lx",
        trigger_excpt.triggered, trigger_excpt.cause, trigger_excpt.tval,
        same_fault_trigger_vaddr
    );

    ai_vfof_eb_clear_mcontrol6_slot(0U);
    ai_vfof_eb_arm_mcontrol6_slot(0U, after_fault_trigger_vaddr,
        AI_VFOF_EB_MCONTROL6_LOAD);
    ai_vfof_eb_fill_bytes(dump_paddr, 0x6aU, AI_VFOF_EB_FULL_VL);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        AI_VFOF_EB_FULL_VL, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-46J trigger-after-fault priority: an address trigger beyond the first nonzero FOF fault is not reached, so the operation still completes as a silent partial VFOF stop",
        excpt.triggered == false &&
        observed_vl == AI_VFOF_EB_LOW_BYTES &&
        observed_vstart == 0U &&
        ai_vfof_eb_bytes_match(dump_paddr, prefix, sizeof(prefix)) &&
        ai_vfof_eb_bytes_match(dump_paddr + AI_VFOF_EB_LOW_BYTES,
            seed + AI_VFOF_EB_LOW_BYTES,
            AI_VFOF_EB_FULL_VL - AI_VFOF_EB_LOW_BYTES),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u trigger_addr=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, after_fault_trigger_vaddr
    );

    ai_vfof_eb_clear_mcontrol6_slot(0U);
    ai_vfof_eb_restore_breakpoint_delivery(saved_mstatus);
    ai_vfof_eb_fill_bytes(dump_paddr, 0xa6U, AI_VFOF_EB_FULL_VL);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vle8ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        AI_VFOF_EB_FULL_VL, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-46J after clearing the address trigger, the original nonzero-element VFOF page fault is again silent and does not leave breakpoint metadata in the following retry",
        excpt.triggered == false &&
        observed_vl == AI_VFOF_EB_LOW_BYTES &&
        observed_vstart == 0U &&
        ai_vfof_eb_bytes_match(dump_paddr, prefix, sizeof(prefix)) &&
        ai_vfof_eb_bytes_match(dump_paddr + AI_VFOF_EB_LOW_BYTES,
            seed + AI_VFOF_EB_LOW_BYTES,
            AI_VFOF_EB_FULL_VL - AI_VFOF_EB_LOW_BYTES),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u suffix=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(dump_paddr + AI_VFOF_EB_LOW_BYTES)
    );

    ai_vfof_eb_cleanup_hs();
    TEST_END();
}

bool ai_micro_hs_vector_fp_illegal_path_keeps_fs_clean_spike_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 3ULL << 9);
    CSRC(CSR_MSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_MSTATUS, SSTATUS_FS_CLEAN);

    TEST_SETUP_EXCEPT();
    ai_vfof_eb_vfadd_e8_illegal();

    TEST_ASSERT("AP-31 related Spike side-effect check: an illegal vector-FP path must not dirty mstatus.FS before real FP execution",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        (CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK) == SSTATUS_FS_CLEAN,
        "cause=0x%lx fs=0x%lx", excpt.cause,
        CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
    TEST_END();
}
