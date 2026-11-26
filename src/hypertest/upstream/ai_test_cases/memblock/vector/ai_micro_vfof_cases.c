#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_MICRO_VFOF_BYTES 16U
#define AI_MICRO_VFOF_PARTIAL_BYTES 8U
#define AI_MICRO_VFOF_EEW_LANES 8U
#define AI_MICRO_VFOF_EEW_MAX_BYTES 64U

static inline pte_t *ai_micro_vfof_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_vfof_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ai_micro_vfof_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_micro_vfof_write_bytes(uintptr_t addr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_vfof_bytes_match(uintptr_t addr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline bool ai_micro_vfof_bytes_are(uintptr_t addr,
    uint8_t value,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != value) {
            return false;
        }
    }
    return true;
}

static inline bool ai_micro_vfof_addr_bytes_match(uintptr_t addr,
    uintptr_t expected_addr,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != read8(expected_addr + i)) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_vfof_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint8_t ai_arch_vsegment_fof_expected_field_byte(
    unsigned episode,
    unsigned field,
    unsigned lane)
{
    const uint8_t bases[2][2] = {
        {0x21U, 0x51U},
        {0xa1U, 0xd1U}
    };

    return (uint8_t)(bases[episode][field] + lane);
}

static inline uint8_t ai_arch_vsegment_fof_expected_src_byte(
    unsigned episode,
    unsigned byte_index)
{
    return ai_arch_vsegment_fof_expected_field_byte(episode,
        byte_index & 1U, byte_index >> 1U);
}

static inline void ai_micro_vle8ff_then_vle8_capture(uintptr_t fof_src,
    uintptr_t vec_src,
    uintptr_t fof_dst,
    uintptr_t vec_dst,
    unsigned req_vl,
    unsigned *fof_vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned observed_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[req_vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[fof_src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vsetvl t0, %[req_vl], %[vtype]\n\t"
        "vle8.v v16, (%[vec_src])\n\t"
        "vse8.v v16, (%[vec_dst])\n\t"
        "vsetvl t0, %[out_vl], %[vtype]\n\t"
        "vse8.v v8, (%[fof_dst])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (observed_vl)
        : [fof_src] "r" (fof_src),
          [vec_src] "r" (vec_src),
          [fof_dst] "r" (fof_dst),
          [vec_dst] "r" (vec_dst),
          [req_vl] "r" (req_vl),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *fof_vl = observed_vl;
}

static inline void ai_micro_vle8ff_then_faulting_load_then_vle8_capture(
    uintptr_t fof_src,
    uintptr_t redirect_fault_addr,
    uintptr_t vec_src,
    uintptr_t fof_dst,
    uintptr_t vec_dst,
    unsigned req_vl,
    unsigned *fof_vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned observed_vl = 0U;
    uint64_t redirected_load_sink = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[req_vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[fof_src])\n\t"
        "ld %[fault_sink], 0(%[redirect_fault_addr])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vsetvl t0, %[req_vl], %[vtype]\n\t"
        "vle8.v v16, (%[vec_src])\n\t"
        "vse8.v v16, (%[vec_dst])\n\t"
        "vsetvl t0, %[out_vl], %[vtype]\n\t"
        "vse8.v v8, (%[fof_dst])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (observed_vl),
          [fault_sink] "=&r" (redirected_load_sink)
        : [fof_src] "r" (fof_src),
          [redirect_fault_addr] "r" (redirect_fault_addr),
          [vec_src] "r" (vec_src),
          [fof_dst] "r" (fof_dst),
          [vec_dst] "r" (vec_dst),
          [req_vl] "r" (req_vl),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *fof_vl = observed_vl;
}

static inline void ai_arch_vsegment_fof_vlseg2e8ff_prefix_capture(
    uintptr_t seed0,
    uintptr_t seed1,
    uintptr_t src,
    uintptr_t dump0,
    uintptr_t dump1,
    unsigned req_vl,
    unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed0])\n\t"
        "vle8.v v9, (%[seed1])\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
        "vlseg2e8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vse8.v v8, (%[dump0])\n\t"
        "vse8.v v9, (%[dump1])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (captured_vl),
          [out_vstart] "=&r" (captured_vstart)
        : [seed0] "r" (seed0),
          [seed1] "r" (seed1),
          [src] "r" (src),
          [dump0] "r" (dump0),
          [dump1] "r" (dump1),
          [req_vl] "r" (req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_micro_dual_vle8ff_capture(uintptr_t old_src,
    uintptr_t young_src,
    uintptr_t old_dst,
    uintptr_t young_dst,
    unsigned req_vl,
    unsigned *old_vl,
    unsigned *young_vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned observed_old_vl = 0U;
    unsigned observed_young_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[req_vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[old_src])\n\t"
        "csrr %[old_out_vl], vl\n\t"
        "vsetvl t0, %[req_vl], %[vtype]\n\t"
        "vle8ff.v v16, (%[young_src])\n\t"
        "csrr %[young_out_vl], vl\n\t"
        "vsetvl t0, %[old_out_vl], %[vtype]\n\t"
        "vse8.v v8, (%[old_dst])\n\t"
        "vsetvl t0, %[young_out_vl], %[vtype]\n\t"
        "vse8.v v16, (%[young_dst])\n\t"
        ".option pop\n\t"
        : [old_out_vl] "=&r" (observed_old_vl),
          [young_out_vl] "=&r" (observed_young_vl)
        : [old_src] "r" (old_src),
          [young_src] "r" (young_src),
          [old_dst] "r" (old_dst),
          [young_dst] "r" (young_dst),
          [req_vl] "r" (req_vl),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *old_vl = observed_old_vl;
    *young_vl = observed_young_vl;
}

static inline void ai_micro_single_vle8ff_capture(uintptr_t src,
    uintptr_t dst,
    unsigned req_vl,
    unsigned *observed_vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[req_vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vsetvl t0, %[out_vl], %[vtype]\n\t"
        "vse8.v v8, (%[dst])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (captured_vl)
        : [src] "r" (src),
          [dst] "r" (dst),
          [req_vl] "r" (req_vl),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_micro_masked_vle8ff_full_dump_policy(
    uintptr_t guard_src,
    uintptr_t fof_src,
    uintptr_t dump,
    unsigned req_vl,
    unsigned active_limit,
    bool mask_agnostic,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    if (mask_agnostic) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "csrw vstart, zero\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
            "vle8.v v8, (%[guard_src])\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, ma\n\t"
            "vid.v v4\n\t"
            "vmsleu.vx v0, v4, %[active_limit]\n\t"
            "vle8ff.v v8, (%[fof_src]), v0.t\n\t"
            "csrr %[out_vl], vl\n\t"
            "csrw vstart, zero\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
            "vse8.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r" (captured_vl)
            : [guard_src] "r" (guard_src),
              [fof_src] "r" (fof_src),
              [dump] "r" (dump),
              [req_vl] "r" (req_vl),
              [active_limit] "r" (active_limit)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "csrw vstart, zero\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
            "vle8.v v8, (%[guard_src])\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
            "vid.v v4\n\t"
            "vmsleu.vx v0, v4, %[active_limit]\n\t"
            "vle8ff.v v8, (%[fof_src]), v0.t\n\t"
            "csrr %[out_vl], vl\n\t"
            "csrw vstart, zero\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
            "vse8.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r" (captured_vl)
            : [guard_src] "r" (guard_src),
              [fof_src] "r" (fof_src),
              [dump] "r" (dump),
              [req_vl] "r" (req_vl),
              [active_limit] "r" (active_limit)
            : "t0", "memory");
    }

    *observed_vl = captured_vl;
}

static inline void ai_micro_masked_vle16ff_full_dump_ma(
    uintptr_t guard_src,
    uintptr_t fof_src,
    uintptr_t dump,
    unsigned req_vl,
    unsigned active_limit,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e16, m1, tu, mu\n\t"
        "vle16.v v8, (%[guard_src])\n\t"
        "vsetvli t0, %[req_vl], e16, m1, tu, ma\n\t"
        "vid.v v4\n\t"
        "vmsleu.vx v0, v4, %[active_limit]\n\t"
        "vle16ff.v v8, (%[fof_src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e16, m1, tu, mu\n\t"
        "vse16.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (captured_vl)
        : [guard_src] "r" (guard_src),
          [fof_src] "r" (fof_src),
          [dump] "r" (dump),
          [req_vl] "r" (req_vl),
          [active_limit] "r" (active_limit)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_micro_masked_vle32ff_full_dump_ma(
    uintptr_t guard_src,
    uintptr_t fof_src,
    uintptr_t dump,
    unsigned req_vl,
    unsigned active_limit,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e32, m2, tu, mu\n\t"
        "vle32.v v8, (%[guard_src])\n\t"
        "vsetvli t0, %[req_vl], e32, m2, tu, ma\n\t"
        "vid.v v4\n\t"
        "vmsleu.vx v0, v4, %[active_limit]\n\t"
        "vle32ff.v v8, (%[fof_src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e32, m2, tu, mu\n\t"
        "vse32.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (captured_vl)
        : [guard_src] "r" (guard_src),
          [fof_src] "r" (fof_src),
          [dump] "r" (dump),
          [req_vl] "r" (req_vl),
          [active_limit] "r" (active_limit)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_micro_masked_vle64ff_full_dump_ma(
    uintptr_t guard_src,
    uintptr_t fof_src,
    uintptr_t dump,
    unsigned req_vl,
    unsigned active_limit,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e64, m4, tu, mu\n\t"
        "vle64.v v8, (%[guard_src])\n\t"
        "vsetvli t0, %[req_vl], e64, m4, tu, ma\n\t"
        "vid.v v4\n\t"
        "vmsleu.vx v0, v4, %[active_limit]\n\t"
        "vle64ff.v v8, (%[fof_src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e64, m4, tu, mu\n\t"
        "vse64.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (captured_vl)
        : [guard_src] "r" (guard_src),
          [fof_src] "r" (fof_src),
          [dump] "r" (dump),
          [req_vl] "r" (req_vl),
          [active_limit] "r" (active_limit)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_micro_masked_vleff_eew_full_dump_ma(
    unsigned eew_bytes,
    uintptr_t guard_src,
    uintptr_t fof_src,
    uintptr_t dump,
    unsigned req_vl,
    unsigned active_limit,
    unsigned *observed_vl)
{
    if (eew_bytes == 1U) {
        ai_micro_masked_vle8ff_full_dump_policy(guard_src, fof_src, dump,
            req_vl, active_limit, true, observed_vl);
    } else if (eew_bytes == 2U) {
        ai_micro_masked_vle16ff_full_dump_ma(guard_src, fof_src, dump,
            req_vl, active_limit, observed_vl);
    } else if (eew_bytes == 4U) {
        ai_micro_masked_vle32ff_full_dump_ma(guard_src, fof_src, dump,
            req_vl, active_limit, observed_vl);
    } else {
        ai_micro_masked_vle64ff_full_dump_ma(guard_src, fof_src, dump,
            req_vl, active_limit, observed_vl);
    }
}

static inline void ai_micro_masked_vlseg2e8ff_full_dump_ma(
    uintptr_t guard0_src,
    uintptr_t guard1_src,
    uintptr_t fof_src,
    uintptr_t dump0,
    uintptr_t dump1,
    unsigned req_vl,
    unsigned active_limit,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[guard0_src])\n\t"
        "vle8.v v9, (%[guard1_src])\n\t"
        "vsetvli t0, %[req_vl], e8, m1, tu, ma\n\t"
        "vid.v v4\n\t"
        "vmsleu.vx v0, v4, %[active_limit]\n\t"
        "vlseg2e8ff.v v8, (%[fof_src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump0])\n\t"
        "vse8.v v9, (%[dump1])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (captured_vl)
        : [guard0_src] "r" (guard0_src),
          [guard1_src] "r" (guard1_src),
          [fof_src] "r" (fof_src),
          [dump0] "r" (dump0),
          [dump1] "r" (dump1),
          [req_vl] "r" (req_vl),
          [active_limit] "r" (active_limit)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_micro_masked_vle8ff_vstart_tail_full_dump_ma(
    uintptr_t guard_src,
    uintptr_t fof_src,
    uintptr_t dump,
    unsigned full_vl,
    unsigned req_vl,
    unsigned vstart_in,
    unsigned active_limit,
    unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[guard_src])\n\t"
        "vsetvli t0, %[req_vl], e8, m1, ta, ma\n\t"
        "vid.v v4\n\t"
        "vmsleu.vx v0, v4, %[active_limit]\n\t"
        "csrw vstart, %[vstart_in]\n\t"
        "vle8ff.v v8, (%[fof_src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (captured_vl),
          [out_vstart] "=&r" (captured_vstart)
        : [guard_src] "r" (guard_src),
          [fof_src] "r" (fof_src),
          [dump] "r" (dump),
          [full_vl] "r" (full_vl),
          [req_vl] "r" (req_vl),
          [vstart_in] "r" (vstart_in),
          [active_limit] "r" (active_limit)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static bool ai_micro_vfof_with_normal_vec_window(bool repeat_rounds)
{
    static const uint8_t fof_patterns[][AI_MICRO_VFOF_BYTES] = {
        { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
          0x81, 0x71, 0x61, 0x51, 0x41, 0x31, 0x21, 0x11 },
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
          0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 },
        { 0x55, 0xaa, 0x44, 0xbb, 0x33, 0xcc, 0x22, 0xdd,
          0x11, 0xee, 0x00, 0xff, 0x99, 0x66, 0x88, 0x77 },
    };
    static const uint8_t vec_patterns[][AI_MICRO_VFOF_BYTES] = {
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87,
          0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
        { 0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
          0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0 },
        { 0x7f, 0x6e, 0x5d, 0x4c, 0x3b, 0x2a, 0x19, 0x08,
          0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f },
    };
    const uintptr_t fof_src_vaddr = hs_page_base(SWITCH1) + 0x300U;
    const uintptr_t vec_src_vaddr = hs_page_base(SWITCH2) + 0x380U;
    const uintptr_t fof_dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x440U;
    const uintptr_t vec_dst_vaddr = hs_page_base(SCRATCHPAD) + 0x4c0U;
    const uintptr_t fof_src_paddr = phys_page_base(SWITCH1) + 0x300U;
    const uintptr_t vec_src_paddr = phys_page_base(SWITCH2) + 0x380U;
    const uintptr_t fof_dst_paddr = phys_page_base(VSRWX_GRWX) + 0x440U;
    const uintptr_t vec_dst_paddr = phys_page_base(SCRATCHPAD) + 0x4c0U;
    const uintptr_t fof_before = fof_dst_paddr - 1U;
    const uintptr_t fof_after = fof_dst_paddr + AI_MICRO_VFOF_BYTES;
    const uintptr_t vec_before = vec_dst_paddr - 1U;
    const uintptr_t vec_after = vec_dst_paddr + AI_MICRO_VFOF_BYTES;
    const uint8_t fof_before_sentinel = 0x6aU;
    const uint8_t fof_after_sentinel = 0xa6U;
    const uint8_t vec_before_sentinel = 0x39U;
    const uint8_t vec_after_sentinel = 0x93U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(fof_patterns) / sizeof(fof_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        unsigned observed_fof_vl = 0U;

        goto_priv(PRIV_M);
        ai_micro_vfof_write_bytes(fof_src_paddr, fof_patterns[round],
            AI_MICRO_VFOF_BYTES);
        ai_micro_vfof_write_bytes(vec_src_paddr, vec_patterns[round],
            AI_MICRO_VFOF_BYTES);
        ai_micro_vfof_write_bytes(fof_dst_paddr, vec_patterns[0],
            AI_MICRO_VFOF_BYTES);
        ai_micro_vfof_write_bytes(vec_dst_paddr, fof_patterns[0],
            AI_MICRO_VFOF_BYTES);
        write8(fof_before, fof_before_sentinel);
        write8(fof_after, fof_after_sentinel);
        write8(vec_before, vec_before_sentinel);
        write8(vec_after, vec_after_sentinel);

        goto_priv(PRIV_HS);
        ai_micro_vle8ff_then_vle8_capture(fof_src_vaddr, vec_src_vaddr,
            fof_dst_vaddr, vec_dst_vaddr, AI_MICRO_VFOF_BYTES,
            &observed_fof_vl);

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the back-to-back VFOF and normal vector-load completion window retires without exception before writeback checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the VFOF path still writes back exactly one full payload while the competing normal vector load also writes back exactly its own payload",
            observed_fof_vl == AI_MICRO_VFOF_BYTES &&
            ai_micro_vfof_bytes_match(fof_dst_paddr, fof_patterns[round],
                AI_MICRO_VFOF_BYTES) &&
            ai_micro_vfof_bytes_match(vec_dst_paddr, vec_patterns[round],
                AI_MICRO_VFOF_BYTES)
        );
        TEST_ASSERT("the VFOF plus normal vector-load arbitration window also preserves adjacent sentinel bytes around both destinations",
            read8(fof_before) == fof_before_sentinel &&
            read8(fof_after) == fof_after_sentinel &&
            read8(vec_before) == vec_before_sentinel &&
            read8(vec_after) == vec_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_back_to_back_vfof_window(bool repeat_rounds)
{
    static const uint8_t old_patterns[][AI_MICRO_VFOF_BYTES] = {
        { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
          0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xf0, 0x0f },
        { 0x0f, 0xf0, 0x1e, 0xe1, 0x2d, 0xd2, 0x3c, 0xc3,
          0x4b, 0xb4, 0x5a, 0xa5, 0x69, 0x96, 0x78, 0x87 },
        { 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18,
          0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90 },
    };
    static const uint8_t young_patterns[][AI_MICRO_VFOF_BYTES] = {
        { 0xfe, 0xed, 0xdc, 0xcb, 0xba, 0xa9, 0x98, 0x87,
          0x76, 0x65, 0x54, 0x43, 0x32, 0x21, 0x10, 0x01 },
        { 0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a,
          0x9b, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02 },
        { 0x5f, 0x4e, 0x3d, 0x2c, 0x1b, 0x0a, 0x19, 0x28,
          0x37, 0x46, 0x55, 0x64, 0x73, 0x82, 0x91, 0xa0 },
    };
    const uintptr_t old_src_vaddr = hs_page_base(SWITCH1) + 0x500U;
    const uintptr_t young_src_vaddr = hs_page_base(SWITCH2) + 0x580U;
    const uintptr_t old_dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x640U;
    const uintptr_t young_dst_vaddr = hs_page_base(SCRATCHPAD) + 0x6c0U;
    const uintptr_t old_src_paddr = phys_page_base(SWITCH1) + 0x500U;
    const uintptr_t young_src_paddr = phys_page_base(SWITCH2) + 0x580U;
    const uintptr_t old_dst_paddr = phys_page_base(VSRWX_GRWX) + 0x640U;
    const uintptr_t young_dst_paddr = phys_page_base(SCRATCHPAD) + 0x6c0U;
    const uintptr_t old_before = old_dst_paddr - 1U;
    const uintptr_t old_after = old_dst_paddr + AI_MICRO_VFOF_BYTES;
    const uintptr_t young_before = young_dst_paddr - 1U;
    const uintptr_t young_after = young_dst_paddr + AI_MICRO_VFOF_BYTES;
    const uint8_t old_before_sentinel = 0x4dU;
    const uint8_t old_after_sentinel = 0xd4U;
    const uint8_t young_before_sentinel = 0x2bU;
    const uint8_t young_after_sentinel = 0xb2U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(old_patterns) / sizeof(old_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        unsigned observed_old_vl = 0U;
        unsigned observed_young_vl = 0U;

        goto_priv(PRIV_M);
        ai_micro_vfof_write_bytes(old_src_paddr, old_patterns[round],
            AI_MICRO_VFOF_BYTES);
        ai_micro_vfof_write_bytes(young_src_paddr, young_patterns[round],
            AI_MICRO_VFOF_BYTES);
        ai_micro_vfof_write_bytes(old_dst_paddr, young_patterns[0],
            AI_MICRO_VFOF_BYTES);
        ai_micro_vfof_write_bytes(young_dst_paddr, old_patterns[0],
            AI_MICRO_VFOF_BYTES);
        write8(old_before, old_before_sentinel);
        write8(old_after, old_after_sentinel);
        write8(young_before, young_before_sentinel);
        write8(young_after, young_after_sentinel);

        goto_priv(PRIV_HS);
        ai_micro_dual_vle8ff_capture(old_src_vaddr, young_src_vaddr,
            old_dst_vaddr, young_dst_vaddr, AI_MICRO_VFOF_BYTES,
            &observed_old_vl, &observed_young_vl);

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the back-to-back VFOF instruction pair retires without exception before live-entry overwrite checks",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the older and younger VFOF instructions each preserve their own full VL and payload instead of overwriting one another's live entry context",
            observed_old_vl == AI_MICRO_VFOF_BYTES &&
            observed_young_vl == AI_MICRO_VFOF_BYTES &&
            ai_micro_vfof_bytes_match(old_dst_paddr, old_patterns[round],
                AI_MICRO_VFOF_BYTES) &&
            ai_micro_vfof_bytes_match(young_dst_paddr, young_patterns[round],
                AI_MICRO_VFOF_BYTES)
        );
        TEST_ASSERT("the back-to-back VFOF pair also leaves adjacent sentinel bytes around both writeback destinations unchanged",
            read8(old_before) == old_before_sentinel &&
            read8(old_after) == old_after_sentinel &&
            read8(young_before) == young_before_sentinel &&
            read8(young_after) == young_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_vfof_partial_stop_then_followup_vec_window(bool repeat_rounds)
{
    static const uint8_t low_patterns[][AI_MICRO_VFOF_PARTIAL_BYTES] = {
        { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87 },
        { 0xa0, 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x16, 0x27 },
        { 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f, 0x7d, 0x6c },
    };
    static const uint8_t blocked_patterns[][AI_MICRO_VFOF_PARTIAL_BYTES] = {
        { 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
        { 0x17, 0x28, 0x39, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e },
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87 },
    };
    static const uint8_t followup_patterns[][AI_MICRO_VFOF_BYTES] = {
        { 0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78,
          0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf1 },
        { 0xf1, 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88,
          0x79, 0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10 },
        { 0x55, 0x44, 0x33, 0x22, 0x11, 0xaa, 0xbb, 0xcc,
          0xdd, 0xee, 0xff, 0x00, 0x13, 0x24, 0x35, 0x46 },
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page followup_src_page = VSRWX_GRWX;
    const enum test_page fof_dst_page = SCRATCHPAD;
    const enum test_page followup_dst_page = VSRW_GRWX;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t followup_src_vaddr = hs_page_base(followup_src_page) + 0x2c0U;
    const uintptr_t fof_dst_vaddr = hs_page_base(fof_dst_page) + 0x380U;
    const uintptr_t followup_dst_vaddr = hs_page_base(followup_dst_page) + 0x480U;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t followup_src_paddr = phys_page_base(followup_src_page) + 0x2c0U;
    const uintptr_t fof_dst_paddr = phys_page_base(fof_dst_page) + 0x380U;
    const uintptr_t followup_dst_paddr = phys_page_base(followup_dst_page) + 0x480U;
    const uintptr_t fof_before = fof_dst_paddr - 1U;
    const uintptr_t fof_after = fof_dst_paddr + AI_MICRO_VFOF_BYTES;
    const uintptr_t followup_before = followup_dst_paddr - 1U;
    const uintptr_t followup_after = followup_dst_paddr + AI_MICRO_VFOF_BYTES;
    const uint8_t partial_fill = 0xc7U;
    const uint8_t followup_fill = 0x3eU;
    const uint8_t fof_before_sentinel = 0x6dU;
    const uint8_t fof_after_sentinel = 0xd6U;
    const uint8_t followup_before_sentinel = 0x2fU;
    const uint8_t followup_after_sentinel = 0xf2U;
    const uint64_t scause_seed = 0x5a5ULL;
    const uint64_t stval_seed = 0x12345000ULL;
    const uint64_t sepc_seed = (uint64_t)&&partial_vfof_no_trap_csr_anchor;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(low_patterns) / sizeof(low_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        unsigned observed_fof_vl = 0U;

        goto_priv(PRIV_M);
        ai_micro_vfof_write_bytes(low_tail_paddr, low_patterns[round],
            AI_MICRO_VFOF_PARTIAL_BYTES);
        ai_micro_vfof_write_bytes(high_head_paddr, blocked_patterns[round],
            AI_MICRO_VFOF_PARTIAL_BYTES);
        ai_micro_vfof_write_bytes(followup_src_paddr, followup_patterns[round],
            AI_MICRO_VFOF_BYTES);
        for (unsigned i = 0; i < AI_MICRO_VFOF_BYTES; ++i) {
            write8(fof_dst_paddr + i, partial_fill);
            write8(followup_dst_paddr + i, followup_fill);
        }
        write8(fof_before, fof_before_sentinel);
        write8(fof_after, fof_after_sentinel);
        write8(followup_before, followup_before_sentinel);
        write8(followup_after, followup_after_sentinel);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
        ai_micro_vfof_set_hs_leaf(followup_src_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(fof_dst_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(followup_dst_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        CSRW(CSR_SCAUSE, scause_seed + round);
        CSRW(CSR_STVAL, stval_seed + round);
        CSRW(CSR_SEPC, sepc_seed);
        TEST_SETUP_EXCEPT();
        ai_micro_vle8ff_then_vle8_capture(fof_src_vaddr, followup_src_vaddr,
            fof_dst_vaddr, followup_dst_vaddr, AI_MICRO_VFOF_BYTES,
            &observed_fof_vl);

        TEST_ASSERT("a cross-page VFOF with only the first half readable stops after the first-page bytes without taking a visible trap or overwriting S trap CSRs",
            excpt.triggered == false &&
            observed_fof_vl == AI_MICRO_VFOF_PARTIAL_BYTES &&
            CSRR(CSR_SCAUSE) == scause_seed + round &&
            CSRR(CSR_STVAL) == stval_seed + round &&
            CSRR(CSR_SEPC) == sepc_seed
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the partial VFOF stop, only the first-page bytes are stored to the VFOF destination while the blocked upper-half bytes remain untouched",
            ai_micro_vfof_bytes_match(fof_dst_paddr, low_patterns[round],
                AI_MICRO_VFOF_PARTIAL_BYTES)
        );
        for (unsigned i = AI_MICRO_VFOF_PARTIAL_BYTES; i < AI_MICRO_VFOF_BYTES; ++i) {
            TEST_ASSERT("the blocked upper-half of the partial VFOF destination still keeps its prefill bytes",
                read8(fof_dst_paddr + i) == partial_fill
            );
        }
        TEST_ASSERT("the normal vector load issued immediately after the partial VFOF stop still writes back its full payload with no stale VFOF context bleed",
            ai_micro_vfof_bytes_match(followup_dst_paddr, followup_patterns[round],
                AI_MICRO_VFOF_BYTES)
        );
        TEST_ASSERT("the partial VFOF plus follow-up vector-load window also preserves adjacent sentinel bytes around both destinations",
            read8(fof_before) == fof_before_sentinel &&
            read8(fof_after) == fof_after_sentinel &&
            read8(followup_before) == followup_before_sentinel &&
            read8(followup_after) == followup_after_sentinel
        );
    }

partial_vfof_no_trap_csr_anchor:
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_vfof_repeated_partial_stop_then_recover_window(void)
{
    static const uint8_t first_low[AI_MICRO_VFOF_PARTIAL_BYTES] = {
        0x0c, 0x1d, 0x2e, 0x3f, 0x40, 0x51, 0x62, 0x73
    };
    static const uint8_t first_high[AI_MICRO_VFOF_PARTIAL_BYTES] = {
        0x84, 0x95, 0xa6, 0xb7, 0xc8, 0xd9, 0xea, 0xfb
    };
    static const uint8_t second_low[AI_MICRO_VFOF_PARTIAL_BYTES] = {
        0x91, 0x82, 0x73, 0x64, 0x55, 0x46, 0x37, 0x28
    };
    static const uint8_t second_high[AI_MICRO_VFOF_PARTIAL_BYTES] = {
        0x17, 0x26, 0x35, 0x44, 0x53, 0x62, 0x71, 0x80
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dst_page = SCRATCHPAD;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x5c0U;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x5c0U;
    const uintptr_t dst_before = dst_paddr - 1U;
    const uintptr_t dst_after = dst_paddr + AI_MICRO_VFOF_BYTES;
    const uint8_t fill_byte = 0x4eU;
    const uint8_t dst_before_sentinel = 0x1bU;
    const uint8_t dst_after_sentinel = 0xb1U;
    const uint64_t scause_seed = 0x6b0ULL;
    const uint64_t stval_seed = 0x23456000ULL;
    const uint64_t sepc_seed = (uint64_t)&&repeated_vfof_no_trap_csr_anchor;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_vfof_write_bytes(low_tail_paddr, first_low, AI_MICRO_VFOF_PARTIAL_BYTES);
    ai_micro_vfof_write_bytes(high_head_paddr, first_high, AI_MICRO_VFOF_PARTIAL_BYTES);
    for (unsigned i = 0; i < AI_MICRO_VFOF_BYTES; ++i) {
        write8(dst_paddr + i, fill_byte);
    }
    write8(dst_before, dst_before_sentinel);
    write8(dst_after, dst_after_sentinel);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_vfof_set_hs_leaf(dst_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_SCAUSE, scause_seed);
    CSRW(CSR_STVAL, stval_seed);
    CSRW(CSR_SEPC, sepc_seed);
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        ai_micro_single_vle8ff_capture(fof_src_vaddr, dst_vaddr, AI_MICRO_VFOF_BYTES,
            &observed_vl);
        TEST_ASSERT("the first partial VFOF stop reports the shortened VL with no visible trap and no S trap CSR overwrite",
            excpt.triggered == false &&
            observed_vl == AI_MICRO_VFOF_PARTIAL_BYTES &&
            CSRR(CSR_SCAUSE) == scause_seed &&
            CSRR(CSR_STVAL) == stval_seed &&
            CSRR(CSR_SEPC) == sepc_seed
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first partial VFOF stop, only the readable low-half bytes are written while the blocked half stays untouched",
        ai_micro_vfof_bytes_match(dst_paddr, first_low, AI_MICRO_VFOF_PARTIAL_BYTES)
    );
    for (unsigned i = AI_MICRO_VFOF_PARTIAL_BYTES; i < AI_MICRO_VFOF_BYTES; ++i) {
        TEST_ASSERT("the blocked half still keeps the original fill bytes after the first partial VFOF stop",
            read8(dst_paddr + i) == fill_byte
        );
    }

    ai_micro_vfof_write_bytes(low_tail_paddr, second_low, AI_MICRO_VFOF_PARTIAL_BYTES);
    ai_micro_vfof_write_bytes(high_head_paddr, second_high, AI_MICRO_VFOF_PARTIAL_BYTES);
    for (unsigned i = 0; i < AI_MICRO_VFOF_BYTES; ++i) {
        write8(dst_paddr + i, fill_byte);
    }

    goto_priv(PRIV_HS);
    sfence_vma();
    CSRW(CSR_SCAUSE, scause_seed + 1ULL);
    CSRW(CSR_STVAL, stval_seed + 1ULL);
    CSRW(CSR_SEPC, sepc_seed);
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        ai_micro_single_vle8ff_capture(fof_src_vaddr, dst_vaddr, AI_MICRO_VFOF_BYTES,
            &observed_vl);
        TEST_ASSERT("repeating the same partial VFOF stop still reports the same shortened VL without surfacing a duplicate exception or overwriting S trap CSRs",
            excpt.triggered == false &&
            observed_vl == AI_MICRO_VFOF_PARTIAL_BYTES &&
            CSRR(CSR_SCAUSE) == scause_seed + 1ULL &&
            CSRR(CSR_STVAL) == stval_seed + 1ULL &&
            CSRR(CSR_SEPC) == sepc_seed
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated partial VFOF stop still writes only the new low-half bytes and leaves the blocked half untouched",
        ai_micro_vfof_bytes_match(dst_paddr, second_low, AI_MICRO_VFOF_PARTIAL_BYTES)
    );
    for (unsigned i = AI_MICRO_VFOF_PARTIAL_BYTES; i < AI_MICRO_VFOF_BYTES; ++i) {
        TEST_ASSERT("the blocked half still keeps the fill bytes after the repeated partial VFOF stop",
            read8(dst_paddr + i) == fill_byte
        );
    }

    goto_priv(PRIV_HS);
    ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        uint8_t full_pattern[AI_MICRO_VFOF_BYTES];

        for (unsigned i = 0; i < AI_MICRO_VFOF_PARTIAL_BYTES; ++i) {
            full_pattern[i] = second_low[i];
            full_pattern[i + AI_MICRO_VFOF_PARTIAL_BYTES] = second_high[i];
        }
        ai_micro_single_vle8ff_capture(fof_src_vaddr, dst_vaddr, AI_MICRO_VFOF_BYTES,
            &observed_vl);
        TEST_ASSERT("after restoring the blocked upper page, the same VFOF source recovers to a full-VL completion with no stale shortened-state retention",
            excpt.triggered == false &&
            observed_vl == AI_MICRO_VFOF_BYTES &&
            ai_micro_vfof_bytes_match(dst_paddr, full_pattern, AI_MICRO_VFOF_BYTES)
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated-partial-stop recovery probe also preserves adjacent sentinel bytes around the destination",
        read8(dst_before) == dst_before_sentinel &&
        read8(dst_after) == dst_after_sentinel
    );

repeated_vfof_no_trap_csr_anchor:
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_arch_vsegment_fof_back_to_back_vl_prefix_owner_window(void)
{
    static const unsigned stop_indices[2] = {2U, 6U};
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page seed0_page = VSRWX_GRWX;
    const enum test_page seed1_page = VSRW_GRWX;
    const enum test_page dump0_page = SCRATCHPAD;
    const enum test_page dump1_page = VSR_GRWX;
    const uintptr_t low_base_vaddr = hs_page_base(low_page);
    const uintptr_t low_base_paddr = phys_page_base(low_page);
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t seed0_vaddr = hs_page_base(seed0_page) + 0x120U;
    const uintptr_t seed1_vaddr = hs_page_base(seed1_page) + 0x160U;
    const uintptr_t dump0_vaddr = hs_page_base(dump0_page) + 0x220U;
    const uintptr_t dump1_vaddr = hs_page_base(dump1_page) + 0x260U;
    const uintptr_t seed0_paddr = phys_page_base(seed0_page) + 0x120U;
    const uintptr_t seed1_paddr = phys_page_base(seed1_page) + 0x160U;
    const uintptr_t dump0_paddr = phys_page_base(dump0_page) + 0x220U;
    const uintptr_t dump1_paddr = phys_page_base(dump1_page) + 0x260U;
    const uintptr_t dump0_before = dump0_paddr - 1U;
    const uintptr_t dump0_after = dump0_paddr + AI_MICRO_VFOF_EEW_LANES;
    const uintptr_t dump1_before = dump1_paddr - 1U;
    const uintptr_t dump1_after = dump1_paddr + AI_MICRO_VFOF_EEW_LANES;
    const uint8_t seed0_poison = 0x6aU;
    const uint8_t seed1_poison = 0x95U;
    const uint8_t dump0_fill = 0xc3U;
    const uint8_t dump1_fill = 0x3cU;
    const uint8_t dump0_before_sentinel = 0xe1U;
    const uint8_t dump0_after_sentinel = 0x1eU;
    const uint8_t dump1_before_sentinel = 0xb4U;
    const uint8_t dump1_after_sentinel = 0x4bU;
    const uint64_t scause_seed = 0x715ULL;
    const uint64_t stval_seed = 0x13579000ULL;
    const uint64_t sepc_seed = (uint64_t)&&vsegment_fof_no_trap_anchor;
    bool test_status = true;

    ai_micro_vfof_enable_vector_state();

    for (unsigned episode = 0; episode < 2U; ++episode) {
        const unsigned stop_index = stop_indices[episode];
        const unsigned low_bytes = 2U * stop_index;
        const unsigned total_bytes = 2U * AI_MICRO_VFOF_EEW_LANES;
        const uintptr_t src_vaddr = low_base_vaddr + PAGE_SIZE - low_bytes;
        const uintptr_t low_tail_paddr = low_base_paddr + PAGE_SIZE - low_bytes;
        unsigned observed_vl = 0U;
        unsigned observed_vstart = 0U;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_VFOF_EEW_LANES; ++i) {
            write8(seed0_paddr + i, seed0_poison);
            write8(seed1_paddr + i, seed1_poison);
            write8(dump0_paddr + i, dump0_fill);
            write8(dump1_paddr + i, dump1_fill);
        }
        write8(dump0_before, dump0_before_sentinel);
        write8(dump0_after, dump0_after_sentinel);
        write8(dump1_before, dump1_before_sentinel);
        write8(dump1_after, dump1_after_sentinel);
        for (unsigned i = 0; i < low_bytes; ++i) {
            write8(low_tail_paddr + i,
                ai_arch_vsegment_fof_expected_src_byte(episode, i));
        }
        for (unsigned i = low_bytes; i < total_bytes; ++i) {
            write8(high_head_paddr + i - low_bytes,
                ai_arch_vsegment_fof_expected_src_byte(episode, i));
        }

        goto_priv(PRIV_HS);
        hspt_init();
        ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
        ai_micro_vfof_set_hs_leaf(seed0_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(seed1_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(dump0_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(dump1_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        CSRW(CSR_SCAUSE, scause_seed + episode);
        CSRW(CSR_STVAL, stval_seed + 0x100ULL * episode);
        CSRW(CSR_SEPC, sepc_seed);
        TEST_SETUP_EXCEPT();
        ai_arch_vsegment_fof_vlseg2e8ff_prefix_capture(seed0_vaddr,
            seed1_vaddr, src_vaddr, dump0_vaddr, dump1_vaddr,
            AI_MICRO_VFOF_EEW_LANES, &observed_vl, &observed_vstart);
        TEST_ASSERT("back-to-back vlseg2e8ff partial stop reports this episode's shortened VL without a visible trap or stale S trap CSR overwrite",
            excpt.triggered == false &&
            observed_vl == stop_index &&
            observed_vstart == 0U &&
            CSRR(CSR_VSTART) == 0ULL &&
            CSRR(CSR_SCAUSE) == scause_seed + episode &&
            CSRR(CSR_STVAL) == stval_seed + 0x100ULL * episode &&
            CSRR(CSR_SEPC) == sepc_seed,
            "episode=%u expected_vl=%u observed_vl=%u observed_vstart=%u",
            episode, stop_index, observed_vl, observed_vstart
        );

        goto_priv(PRIV_M);
        for (unsigned lane = 0; lane < stop_index; ++lane) {
            TEST_ASSERT("vlseg2e8ff field0 prefix dump belongs to the accepted episode before the first-fault boundary",
                read8(dump0_paddr + lane) ==
                    ai_arch_vsegment_fof_expected_field_byte(episode, 0U, lane),
                "episode=%u lane=%u actual=0x%x",
                episode, lane, read8(dump0_paddr + lane)
            );
            TEST_ASSERT("vlseg2e8ff field1 prefix dump belongs to the accepted episode before the first-fault boundary",
                read8(dump1_paddr + lane) ==
                    ai_arch_vsegment_fof_expected_field_byte(episode, 1U, lane),
                "episode=%u lane=%u actual=0x%x",
                episode, lane, read8(dump1_paddr + lane)
            );
        }
        for (unsigned lane = stop_index; lane < AI_MICRO_VFOF_EEW_LANES; ++lane) {
            TEST_ASSERT("the shortened-VL prefix capture does not store stale field0 suffix bytes into the dump buffer",
                read8(dump0_paddr + lane) == dump0_fill,
                "episode=%u lane=%u actual=0x%x",
                episode, lane, read8(dump0_paddr + lane)
            );
            TEST_ASSERT("the shortened-VL prefix capture does not store stale field1 suffix bytes into the dump buffer",
                read8(dump1_paddr + lane) == dump1_fill,
                "episode=%u lane=%u actual=0x%x",
                episode, lane, read8(dump1_paddr + lane)
            );
        }
        TEST_ASSERT("back-to-back segment-FOF prefix capture preserves adjacent dump sentinels for both fields",
            read8(dump0_before) == dump0_before_sentinel &&
            read8(dump0_after) == dump0_after_sentinel &&
            read8(dump1_before) == dump1_before_sentinel &&
            read8(dump1_after) == dump1_after_sentinel,
            "episode=%u", episode
        );
    }

    {
        const unsigned episode = 1U;
        const unsigned stop_index = stop_indices[episode];
        const unsigned low_bytes = 2U * stop_index;
        const uintptr_t src_vaddr = low_base_vaddr + PAGE_SIZE - low_bytes;
        unsigned observed_vl = 0U;
        unsigned observed_vstart = 0U;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_VFOF_EEW_LANES; ++i) {
            write8(dump0_paddr + i, dump0_fill);
            write8(dump1_paddr + i, dump1_fill);
        }

        goto_priv(PRIV_HS);
        ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();
        TEST_SETUP_EXCEPT();
        ai_arch_vsegment_fof_vlseg2e8ff_prefix_capture(seed0_vaddr,
            seed1_vaddr, src_vaddr, dump0_vaddr, dump1_vaddr,
            AI_MICRO_VFOF_EEW_LANES, &observed_vl, &observed_vstart);
        TEST_ASSERT("after repairing the high page, the latest vlseg2e8ff episode completes with full VL and no stale shortened-state retention",
            excpt.triggered == false &&
            observed_vl == AI_MICRO_VFOF_EEW_LANES &&
            observed_vstart == 0U &&
            CSRR(CSR_VSTART) == 0ULL,
            "observed_vl=%u observed_vstart=%u", observed_vl, observed_vstart
        );

        goto_priv(PRIV_M);
        for (unsigned lane = 0; lane < AI_MICRO_VFOF_EEW_LANES; ++lane) {
            TEST_ASSERT("repaired vlseg2e8ff field0 full dump keeps every lane tied to the latest episode payload",
                read8(dump0_paddr + lane) ==
                    ai_arch_vsegment_fof_expected_field_byte(episode, 0U, lane),
                "lane=%u actual=0x%x", lane, read8(dump0_paddr + lane)
            );
            TEST_ASSERT("repaired vlseg2e8ff field1 full dump keeps every lane tied to the latest episode payload",
                read8(dump1_paddr + lane) ==
                    ai_arch_vsegment_fof_expected_field_byte(episode, 1U, lane),
                "lane=%u actual=0x%x", lane, read8(dump1_paddr + lane)
            );
        }
    }

vsegment_fof_no_trap_anchor:
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_vfof_partial_stop_then_trap_redirect_vec_window(
    bool repeat_rounds)
{
    static const uint8_t low_patterns[][AI_MICRO_VFOF_PARTIAL_BYTES] = {
        { 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89 },
        { 0xa9, 0xb8, 0xc7, 0xd6, 0xe5, 0xf4, 0x13, 0x22 },
        { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x21, 0x32, 0x43 },
    };
    static const uint8_t blocked_patterns[][AI_MICRO_VFOF_PARTIAL_BYTES] = {
        { 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf1, 0x02 },
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8 },
        { 0xfe, 0xed, 0xdc, 0xcb, 0xba, 0xa9, 0x98, 0x87 },
    };
    static const uint8_t followup_patterns[][AI_MICRO_VFOF_BYTES] = {
        { 0x03, 0x14, 0x25, 0x36, 0x47, 0x58, 0x69, 0x7a,
          0x8b, 0x9c, 0xad, 0xbe, 0xcf, 0xd0, 0xe1, 0xf2 },
        { 0xf2, 0xe3, 0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89,
          0x7a, 0x6b, 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01 },
        { 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0xaa, 0xbb,
          0xcc, 0xdd, 0xee, 0xff, 0x24, 0x35, 0x46, 0x57 },
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page redirect_fault_page = VSRW_GRW;
    const enum test_page followup_src_page = VSRWX_GRWX;
    const enum test_page fof_dst_page = SCRATCHPAD;
    const enum test_page followup_dst_page = VSRW_GRWX;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t redirect_fault_vaddr = hs_page_base(redirect_fault_page) + 0x340U;
    const uintptr_t followup_src_vaddr = hs_page_base(followup_src_page) + 0x300U;
    const uintptr_t fof_dst_vaddr = hs_page_base(fof_dst_page) + 0x6c0U;
    const uintptr_t followup_dst_vaddr = hs_page_base(followup_dst_page) + 0x780U;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t followup_src_paddr = phys_page_base(followup_src_page) + 0x300U;
    const uintptr_t fof_dst_paddr = phys_page_base(fof_dst_page) + 0x6c0U;
    const uintptr_t followup_dst_paddr = phys_page_base(followup_dst_page) + 0x780U;
    const uintptr_t fof_before = fof_dst_paddr - 1U;
    const uintptr_t fof_after = fof_dst_paddr + AI_MICRO_VFOF_BYTES;
    const uintptr_t followup_before = followup_dst_paddr - 1U;
    const uintptr_t followup_after = followup_dst_paddr + AI_MICRO_VFOF_BYTES;
    const uint8_t partial_fill = 0x3cU;
    const uint8_t followup_fill = 0xc3U;
    const uint8_t fof_before_sentinel = 0x6eU;
    const uint8_t fof_after_sentinel = 0xe6U;
    const uint8_t followup_before_sentinel = 0x27U;
    const uint8_t followup_after_sentinel = 0x72U;
    const unsigned rounds = repeat_rounds ?
        (unsigned)(sizeof(low_patterns) / sizeof(low_patterns[0])) : 1U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        unsigned observed_fof_vl = 0U;

        goto_priv(PRIV_M);
        ai_micro_vfof_write_bytes(low_tail_paddr, low_patterns[round],
            AI_MICRO_VFOF_PARTIAL_BYTES);
        ai_micro_vfof_write_bytes(high_head_paddr, blocked_patterns[round],
            AI_MICRO_VFOF_PARTIAL_BYTES);
        ai_micro_vfof_write_bytes(followup_src_paddr, followup_patterns[round],
            AI_MICRO_VFOF_BYTES);
        for (unsigned i = 0; i < AI_MICRO_VFOF_BYTES; ++i) {
            write8(fof_dst_paddr + i, partial_fill);
            write8(followup_dst_paddr + i, followup_fill);
        }
        write8(fof_before, fof_before_sentinel);
        write8(fof_after, fof_after_sentinel);
        write8(followup_before, followup_before_sentinel);
        write8(followup_after, followup_after_sentinel);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
        ai_micro_vfof_set_hs_leaf(redirect_fault_page, PTE_V | PTE_AD | PTE_X);
        ai_micro_vfof_set_hs_leaf(followup_src_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(fof_dst_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_vfof_set_hs_leaf(followup_dst_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_vle8ff_then_faulting_load_then_vle8_capture(fof_src_vaddr,
            redirect_fault_vaddr, followup_src_vaddr, fof_dst_vaddr, followup_dst_vaddr,
            AI_MICRO_VFOF_BYTES, &observed_fof_vl);

        TEST_ASSERT("a partial VFOF immediately followed by a later HS load-page-fault redirect still reports only that later trap while preserving the shortened VL for post-redirect writeback",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == redirect_fault_vaddr &&
            observed_fof_vl == AI_MICRO_VFOF_PARTIAL_BYTES
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the trap-driven redirect window, the VFOF destination still contains only the readable low-half bytes while the blocked high-half stays untouched",
            ai_micro_vfof_bytes_match(fof_dst_paddr, low_patterns[round],
                AI_MICRO_VFOF_PARTIAL_BYTES)
        );
        for (unsigned i = AI_MICRO_VFOF_PARTIAL_BYTES; i < AI_MICRO_VFOF_BYTES; ++i) {
            TEST_ASSERT("the blocked upper-half of the redirected partial VFOF destination still keeps its prefill bytes",
                read8(fof_dst_paddr + i) == partial_fill
            );
        }
        TEST_ASSERT("the normal vector load issued after the trap-driven redirect still writes back its own full payload with no stale partial-VFOF context bleed",
            ai_micro_vfof_bytes_match(followup_dst_paddr, followup_patterns[round],
                AI_MICRO_VFOF_BYTES)
        );
        TEST_ASSERT("the redirected partial VFOF plus follow-up vector-load window also preserves adjacent sentinel bytes around both destinations",
            read8(fof_before) == fof_before_sentinel &&
            read8(fof_after) == fof_after_sentinel &&
            read8(followup_before) == followup_before_sentinel &&
            read8(followup_after) == followup_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_vfof_masked_suffix_vma_policy_full_dump_window(void)
{
    static const uint8_t low_payload[AI_MICRO_VFOF_PARTIAL_BYTES] = {
        0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page guard_page = VSRWX_GRWX;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x100U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x100U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x100U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x100U;
    const uint8_t guard_byte = 0x5aU;
    const uint8_t high_byte = 0xe9U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_vfof_set_hs_leaf(guard_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_vfof_write_bytes(low_tail_paddr, low_payload,
        AI_MICRO_VFOF_PARTIAL_BYTES);
    for (unsigned i = 0; i < AI_MICRO_VFOF_BYTES; ++i) {
        write8(high_head_paddr + i, high_byte);
        write8(guard_paddr + i, guard_byte);
        write8(dump_paddr + i, 0U);
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        ai_micro_masked_vle8ff_full_dump_policy(guard_vaddr, fof_src_vaddr,
            dump_vaddr, AI_MICRO_VFOF_BYTES, AI_MICRO_VFOF_PARTIAL_BYTES,
            true, &observed_vl);
        TEST_ASSERT("masked vle8ff with vma=1 still shortens VL at the first active fault while completing the AP-47 full-register dump window",
            excpt.triggered == false &&
            observed_vl == AI_MICRO_VFOF_PARTIAL_BYTES &&
            CSRR(CSR_VSTART) == 0ULL
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("vma=1 masked VFOF keeps the readable prefix payload in the full destination dump",
        ai_micro_vfof_bytes_match(dump_paddr, low_payload,
            AI_MICRO_VFOF_PARTIAL_BYTES)
    );
    TEST_ASSERT("vma=1 masked VFOF leaves the active faulting element itself undisturbed",
        read8(dump_paddr + AI_MICRO_VFOF_PARTIAL_BYTES) == guard_byte
    );
    for (unsigned i = AI_MICRO_VFOF_PARTIAL_BYTES + 1U;
         i < AI_MICRO_VFOF_BYTES; ++i) {
        TEST_ASSERT("vma=1 masked-off suffix lanes after the first VFOF fault follow the Nanhu all-ones agnostic policy instead of keeping the guard byte",
            read8(dump_paddr + i) == 0xffU,
            "lane=%u actual=0x%x guard=0x%x", i, read8(dump_paddr + i),
            guard_byte
        );
    }

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_MICRO_VFOF_BYTES; ++i) {
        write8(guard_paddr + i, guard_byte);
        write8(dump_paddr + i, 0U);
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        ai_micro_masked_vle8ff_full_dump_policy(guard_vaddr, fof_src_vaddr,
            dump_vaddr, AI_MICRO_VFOF_BYTES, AI_MICRO_VFOF_PARTIAL_BYTES,
            false, &observed_vl);
        TEST_ASSERT("masked vle8ff with vma=0 is the AP-47 control path and still reports the same first-fault VL",
            excpt.triggered == false &&
            observed_vl == AI_MICRO_VFOF_PARTIAL_BYTES &&
            CSRR(CSR_VSTART) == 0ULL
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("vma=0 masked VFOF keeps the readable prefix payload in the full destination dump",
        ai_micro_vfof_bytes_match(dump_paddr, low_payload,
            AI_MICRO_VFOF_PARTIAL_BYTES)
    );
    for (unsigned i = AI_MICRO_VFOF_PARTIAL_BYTES; i < AI_MICRO_VFOF_BYTES; ++i) {
        TEST_ASSERT("vma=0 masked-off suffix lanes after the VFOF fault remain undisturbed and prevent an over-broad AP-47 fix",
            read8(dump_paddr + i) == guard_byte,
            "lane=%u actual=0x%x expected_guard=0x%x", i,
            read8(dump_paddr + i), guard_byte
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_vfof_masked_suffix_eew_matrix_window(void)
{
    static const unsigned eew_bytes_list[] = { 1U, 2U, 4U, 8U };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page guard_page = VSRWX_GRWX;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x200U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x200U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x200U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x200U;
    const unsigned fault_index = 4U;
    const uint8_t high_byte = 0xecU;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_vfof_set_hs_leaf(guard_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    for (unsigned round = 0; round < sizeof(eew_bytes_list) / sizeof(eew_bytes_list[0]);
         ++round) {
        const unsigned eew_bytes = eew_bytes_list[round];
        const unsigned prefix_bytes = fault_index * eew_bytes;
        const unsigned total_bytes = AI_MICRO_VFOF_EEW_LANES * eew_bytes;
        const uint8_t guard_byte = (uint8_t)(0x41U + round * 0x11U);
        const uintptr_t fof_src_vaddr =
            hs_page_base(low_page) + PAGE_SIZE - prefix_bytes;
        const uintptr_t low_tail_paddr =
            phys_page_base(low_page) + PAGE_SIZE - prefix_bytes;
        const uintptr_t high_head_paddr = phys_page_base(high_page);

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_VFOF_EEW_MAX_BYTES; ++i) {
            write8(guard_paddr + i, guard_byte);
            write8(dump_paddr + i, 0U);
            write8(high_head_paddr + i, high_byte);
        }
        for (unsigned i = 0; i < prefix_bytes; ++i) {
            write8(low_tail_paddr + i, (uint8_t)(0x20U + round * 0x20U + i));
        }

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        {
            unsigned observed_vl = 0U;
            ai_micro_masked_vleff_eew_full_dump_ma(eew_bytes, guard_vaddr,
                fof_src_vaddr, dump_vaddr, AI_MICRO_VFOF_EEW_LANES,
                fault_index, &observed_vl);
            TEST_ASSERT("masked VFOF EEW matrix entry reports the first active fault index without taking a visible trap",
                excpt.triggered == false &&
                observed_vl == fault_index &&
                CSRR(CSR_VSTART) == 0ULL,
                "eew_bytes=%u observed_vl=%u", eew_bytes, observed_vl
            );
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("masked VFOF EEW matrix preserves all prefix bytes before the first fault",
            ai_micro_vfof_addr_bytes_match(dump_paddr, low_tail_paddr,
                prefix_bytes),
            "eew_bytes=%u prefix_bytes=%u", eew_bytes, prefix_bytes
        );
        TEST_ASSERT("masked VFOF EEW matrix leaves the active faulting element undisturbed",
            ai_micro_vfof_bytes_are(dump_paddr + prefix_bytes, guard_byte,
                eew_bytes),
            "eew_bytes=%u", eew_bytes
        );
        TEST_ASSERT("masked VFOF EEW matrix applies all-ones agnostic policy to every masked-off suffix element",
            ai_micro_vfof_bytes_are(dump_paddr + (fault_index + 1U) * eew_bytes,
                0xffU, total_bytes - (fault_index + 1U) * eew_bytes),
            "eew_bytes=%u total_bytes=%u", eew_bytes, total_bytes
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_vfof_segment_masked_suffix_fields_window(void)
{
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page guard0_page = VSRWX_GRWX;
    const enum test_page guard1_page = VSRW_GRWX;
    const enum test_page dump0_page = SCRATCHPAD;
    const enum test_page dump1_page = VSR_GRWX;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t guard0_paddr = phys_page_base(guard0_page) + 0x300U;
    const uintptr_t guard1_paddr = phys_page_base(guard1_page) + 0x300U;
    const uintptr_t guard0_vaddr = hs_page_base(guard0_page) + 0x300U;
    const uintptr_t guard1_vaddr = hs_page_base(guard1_page) + 0x300U;
    const uintptr_t dump0_paddr = phys_page_base(dump0_page) + 0x300U;
    const uintptr_t dump1_paddr = phys_page_base(dump1_page) + 0x300U;
    const uintptr_t dump0_vaddr = hs_page_base(dump0_page) + 0x300U;
    const uintptr_t dump1_vaddr = hs_page_base(dump1_page) + 0x300U;
    const uint8_t guard0 = 0x6cU;
    const uint8_t guard1 = 0x93U;
    const unsigned fault_index = 4U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_vfof_set_hs_leaf(guard0_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(guard1_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(dump0_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(dump1_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < 16U; ++i) {
        write8(high_head_paddr + i, (uint8_t)(0xd0U + i));
        write8(guard0_paddr + i, guard0);
        write8(guard1_paddr + i, guard1);
        write8(dump0_paddr + i, 0U);
        write8(dump1_paddr + i, 0U);
    }
    for (unsigned i = 0; i < fault_index; ++i) {
        write8(low_tail_paddr + 2U * i, (uint8_t)(0x20U + i));
        write8(low_tail_paddr + 2U * i + 1U, (uint8_t)(0x70U + i));
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        ai_micro_masked_vlseg2e8ff_full_dump_ma(guard0_vaddr, guard1_vaddr,
            fof_src_vaddr, dump0_vaddr, dump1_vaddr, AI_MICRO_VFOF_EEW_LANES,
            fault_index, &observed_vl);
        TEST_ASSERT("masked vlseg2e8ff reports the first active element fault without surfacing a trap",
            excpt.triggered == false &&
            observed_vl == fault_index &&
            CSRR(CSR_VSTART) == 0ULL
        );
    }

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < fault_index; ++i) {
        TEST_ASSERT("masked segment VFOF field0 prefix payload is preserved before the first fault",
            read8(dump0_paddr + i) == (uint8_t)(0x20U + i),
            "lane=%u actual=0x%x", i, read8(dump0_paddr + i)
        );
        TEST_ASSERT("masked segment VFOF field1 prefix payload is preserved before the first fault",
            read8(dump1_paddr + i) == (uint8_t)(0x70U + i),
            "lane=%u actual=0x%x", i, read8(dump1_paddr + i)
        );
    }
    TEST_ASSERT("masked segment VFOF leaves the first faulting element undisturbed in every field",
        read8(dump0_paddr + fault_index) == guard0 &&
        read8(dump1_paddr + fault_index) == guard1
    );
    TEST_ASSERT("masked segment VFOF applies all-ones agnostic policy to masked-off suffix lanes in field0",
        ai_micro_vfof_bytes_are(dump0_paddr + fault_index + 1U, 0xffU,
            AI_MICRO_VFOF_EEW_LANES - fault_index - 1U)
    );
    TEST_ASSERT("masked segment VFOF applies all-ones agnostic policy to masked-off suffix lanes in field1",
        ai_micro_vfof_bytes_are(dump1_paddr + fault_index + 1U, 0xffU,
            AI_MICRO_VFOF_EEW_LANES - fault_index - 1U)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_vfof_vstart_tail_ap94_masked_suffix_window(void)
{
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page guard_page = VSRWX_GRWX;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 5U;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE - 5U;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x380U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x380U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x380U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x380U;
    const uint8_t guard_byte = 0x4dU;
    const unsigned full_vl = 16U;
    const unsigned req_vl = 12U;
    const unsigned vstart_in = 3U;
    const unsigned first_fault_index = 5U;
    const unsigned active_limit = 7U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_vfof_set_hs_leaf(guard_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < full_vl; ++i) {
        write8(guard_paddr + i, guard_byte);
        write8(dump_paddr + i, 0U);
        write8(high_head_paddr + i, (uint8_t)(0xa0U + i));
    }
    for (unsigned i = 0; i < first_fault_index; ++i) {
        write8(low_tail_paddr + i, (uint8_t)(0x30U + i));
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        unsigned observed_vstart = 0U;
        ai_micro_masked_vle8ff_vstart_tail_full_dump_ma(guard_vaddr,
            fof_src_vaddr, dump_vaddr, full_vl, req_vl, vstart_in,
            active_limit, &observed_vl, &observed_vstart);
        TEST_ASSERT("vstart!=0 masked VFOF reports the architectural first active fault index once while later active faults stay silent",
            excpt.triggered == false &&
            observed_vl == first_fault_index &&
            observed_vstart == 0U &&
            CSRR(CSR_VSTART) == 0ULL,
            "observed_vl=%u observed_vstart=%u", observed_vl,
            observed_vstart
        );
    }

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < vstart_in; ++i) {
        TEST_ASSERT("vstart!=0 masked VFOF keeps pre-vstart lanes untouched",
            read8(dump_paddr + i) == guard_byte,
            "lane=%u actual=0x%x", i, read8(dump_paddr + i)
        );
    }
    for (unsigned i = vstart_in; i < first_fault_index; ++i) {
        TEST_ASSERT("vstart!=0 masked VFOF loads only the active prefix from vstart to the first fault",
            read8(dump_paddr + i) == (uint8_t)(0x30U + i),
            "lane=%u actual=0x%x", i, read8(dump_paddr + i)
        );
    }
    for (unsigned i = first_fault_index; i <= active_limit; ++i) {
        TEST_ASSERT("active faulting and later active-fault suffix lanes do not become valid payload after AP-94 traversal",
            read8(dump_paddr + i) == guard_byte,
            "lane=%u actual=0x%x", i, read8(dump_paddr + i)
        );
    }
    for (unsigned i = active_limit + 1U; i < req_vl; ++i) {
        TEST_ASSERT("masked-off in-vl suffix lanes after the first VFOF fault still receive vma=1 all-ones agnostic handling",
            read8(dump_paddr + i) == 0xffU,
            "lane=%u actual=0x%x", i, read8(dump_paddr + i)
        );
    }
    for (unsigned i = req_vl; i < full_vl; ++i) {
        TEST_ASSERT("tail lanes remain separated from the AP-47 mask suffix and follow the Nanhu all-ones tail-agnostic policy",
            read8(dump_paddr + i) == 0xffU,
            "lane=%u actual=0x%x", i, read8(dump_paddr + i)
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_vfof_repeated_mask_policy_no_stale_suffix_window(void)
{
    static const uint8_t first_prefix[4] = {
        0x11, 0x22, 0x33, 0x44
    };
    static const uint8_t second_prefix[AI_MICRO_VFOF_PARTIAL_BYTES] = {
        0xa0, 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x16, 0x27
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page guard_page = VSRWX_GRWX;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t first_fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t second_fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t first_low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t second_low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        AI_MICRO_VFOF_PARTIAL_BYTES;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x400U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x400U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x400U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x400U;
    const uint8_t first_guard = 0x61U;
    const uint8_t second_guard = 0x27U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_vfof_set_hs_leaf(guard_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_vfof_write_bytes(first_low_tail_paddr, first_prefix,
        sizeof(first_prefix));
    for (unsigned i = 0; i < AI_MICRO_VFOF_BYTES; ++i) {
        write8(high_head_paddr + i, (uint8_t)(0xc0U + i));
        write8(guard_paddr + i, first_guard);
        write8(dump_paddr + i, 0U);
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        ai_micro_masked_vle8ff_full_dump_policy(guard_vaddr,
            first_fof_src_vaddr, dump_vaddr, AI_MICRO_VFOF_BYTES,
            sizeof(first_prefix), true, &observed_vl);
        TEST_ASSERT("the first repeated masked VFOF episode uses vma=1 and shortens VL at its own first fault",
            excpt.triggered == false &&
            observed_vl == sizeof(first_prefix) &&
            CSRR(CSR_VSTART) == 0ULL
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("the first repeated masked VFOF episode writes its own readable prefix",
        ai_micro_vfof_bytes_match(dump_paddr, first_prefix,
            sizeof(first_prefix))
    );
    TEST_ASSERT("the first repeated masked VFOF episode applies vma=1 all-ones to its own masked-off suffix",
        read8(dump_paddr + sizeof(first_prefix)) == first_guard &&
        ai_micro_vfof_bytes_are(dump_paddr + sizeof(first_prefix) + 1U,
            0xffU, AI_MICRO_VFOF_BYTES - sizeof(first_prefix) - 1U)
    );

    ai_micro_vfof_write_bytes(second_low_tail_paddr, second_prefix,
        AI_MICRO_VFOF_PARTIAL_BYTES);
    for (unsigned i = 0; i < AI_MICRO_VFOF_BYTES; ++i) {
        write8(high_head_paddr + i, (uint8_t)(0xe0U + i));
        write8(guard_paddr + i, second_guard);
        write8(dump_paddr + i, 0U);
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        ai_micro_masked_vle8ff_full_dump_policy(guard_vaddr,
            second_fof_src_vaddr, dump_vaddr, AI_MICRO_VFOF_BYTES,
            AI_MICRO_VFOF_PARTIAL_BYTES, false, &observed_vl);
        TEST_ASSERT("the second repeated masked VFOF episode uses vma=0 and keeps its own first-fault VL independent from the first episode",
            excpt.triggered == false &&
            observed_vl == AI_MICRO_VFOF_PARTIAL_BYTES &&
            CSRR(CSR_VSTART) == 0ULL
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("the second repeated masked VFOF episode writes its newer readable prefix",
        ai_micro_vfof_bytes_match(dump_paddr, second_prefix,
            AI_MICRO_VFOF_PARTIAL_BYTES)
    );
    TEST_ASSERT("the second repeated masked VFOF episode keeps every masked-off suffix lane at its new guard and does not inherit stale vma=1 agnostic context",
        ai_micro_vfof_bytes_are(dump_paddr + AI_MICRO_VFOF_PARTIAL_BYTES,
            second_guard, AI_MICRO_VFOF_BYTES - AI_MICRO_VFOF_PARTIAL_BYTES)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_vfof_ap94_second_fault_keeps_first_vl_window(void)
{
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page guard_page = VSRWX_GRWX;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 2U;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE - 2U;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x480U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x480U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x480U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x480U;
    const uint8_t guard_byte = 0x7bU;
    const unsigned req_vl = AI_MICRO_VFOF_BYTES;
    const unsigned first_fault_index = 2U;
    const unsigned active_limit = 5U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_vfof_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_vfof_set_hs_leaf(guard_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_M);
    write8(low_tail_paddr, 0x41U);
    write8(low_tail_paddr + 1U, 0x52U);
    for (unsigned i = 0; i < req_vl; ++i) {
        write8(high_head_paddr + i, (uint8_t)(0xc0U + i));
        write8(guard_paddr + i, guard_byte);
        write8(dump_paddr + i, 0U);
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    {
        unsigned observed_vl = 0U;
        ai_micro_masked_vle8ff_full_dump_policy(guard_vaddr, fof_src_vaddr,
            dump_vaddr, req_vl, active_limit, true, &observed_vl);
        TEST_ASSERT("AP-47/AP-94 masked VFOF keeps the first fault index even when later active elements would fault again",
            excpt.triggered == false &&
            observed_vl == first_fault_index &&
            CSRR(CSR_VSTART) == 0ULL,
            "observed_vl=%u", observed_vl
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-47/AP-94 masked VFOF preserves the prefix before the first fault",
        read8(dump_paddr) == 0x41U &&
        read8(dump_paddr + 1U) == 0x52U
    );
    for (unsigned i = first_fault_index; i <= active_limit; ++i) {
        TEST_ASSERT("AP-47/AP-94 later active fault candidates do not become valid loaded payload and do not overwrite the first-fault VL",
            read8(dump_paddr + i) == guard_byte,
            "lane=%u actual=0x%x", i, read8(dump_paddr + i)
        );
    }
    TEST_ASSERT("AP-47/AP-94 masked-off suffix after the active fault window still receives vma=1 all-ones agnostic handling",
        ai_micro_vfof_bytes_are(dump_paddr + active_limit + 1U, 0xffU,
            req_vl - active_limit - 1U)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vfof_single_writeback_not_duplicated_across_two_vldu_ports_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_with_normal_vec_window(false);
    TEST_END();
}

bool ai_micro_hs_vfof_writeback_ready_not_overwritten_by_dual_port_arbitration_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_with_normal_vec_window(true);
    TEST_END();
}

bool ai_micro_hs_vfof_back_to_back_instructions_no_entry_context_overwrite_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_back_to_back_vfof_window(false);
    TEST_END();
}

bool ai_micro_hs_younger_vfof_must_not_replace_older_live_vfof_entry_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_back_to_back_vfof_window(true);
    TEST_END();
}

bool ai_micro_hs_vfof_partial_vl_then_followup_vector_load_no_stale_context_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_partial_stop_then_followup_vec_window(false);
    TEST_END();
}

bool ai_micro_hs_vfof_repeated_partial_stop_then_repaired_full_reload_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_repeated_partial_stop_then_recover_window();
    TEST_END();
}

bool ai_micro_hs_vfof_partial_vl_then_trap_redirect_followup_vector_load_no_stale_context_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_partial_stop_then_trap_redirect_vec_window(false);
    TEST_END();
}

bool ai_micro_hs_vfof_repeated_partial_vl_then_trap_redirect_followup_vector_load_no_stale_context_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_partial_stop_then_trap_redirect_vec_window(true);
    TEST_END();
}

bool ai_micro_hs_vfof_masked_suffix_vma_policy_full_dump_after_first_fault_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_masked_suffix_vma_policy_full_dump_window();
    TEST_END();
}

bool ai_micro_hs_vfof_masked_suffix_eew_matrix_after_first_fault_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_masked_suffix_eew_matrix_window();
    TEST_END();
}

bool ai_micro_hs_vfof_segment_masked_suffix_fields_after_first_fault_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_segment_masked_suffix_fields_window();
    TEST_END();
}

bool ai_arch_vsegment_fof_back_to_back_vl_prefix_owner_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_arch_vsegment_fof_back_to_back_vl_prefix_owner_window();
    TEST_END();
}

bool ai_micro_hs_vfof_vstart_tail_ap94_masked_suffix_after_first_fault_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_vstart_tail_ap94_masked_suffix_window();
    TEST_END();
}

bool ai_micro_hs_vfof_repeated_mask_policy_no_stale_suffix_context_after_first_fault_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_repeated_mask_policy_no_stale_suffix_window();
    TEST_END();
}

bool ai_micro_hs_vfof_ap94_second_fault_keeps_first_vl_and_mask_suffix_after_first_fault_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vfof_ap94_second_fault_keeps_first_vl_window();
    TEST_END();
}
