#include <rvh_test.h>
#include <stdint.h>
#include <stddef.h>

#define AI_VSETVL_FLUSH_MSTATUS_VS_DIRTY (3ULL << 9)
#define AI_VSETVL_FLUSH_VTYPE_VILL       (1ULL << 63)
#define AI_VSETVL_FLUSH_MAX_BYTES        256U
#define AI_VSETVL_FLUSH_DENSE_BLOCK      64U
#define AI_VSETVL_FLUSH_DENSE_REGS       7U
#define AI_VSETVL_FLUSH_SPLIT_BYTES      4096U
#define AI_VSETVL_FLUSH_GUARD_BYTES      64U
#define AI_VSETVL_FLUSH_WIDE_EEW_BYTES   8U

#define AI_VSETVL_FLUSH_KEEPVL_HOT_BODY(value) \
        "li t1, -1\n\t" \
        "vsetvl zero, zero, t1\n\t" \
        "li t3, -1\n\t" \
        "li t4, 3\n\t" \
        "divu t3, t3, t4\n\t" \
        "divu t3, t3, t4\n\t" \
        "divu t3, t3, t4\n\t" \
        "divu t3, t3, t4\n\t" \
        "beq t4, t4, 1f\n\t" \
        "addi t3, t3, 1\n\t" \
        "1:\n\t" \
        "li s4, 6\n\t" \
        "vsetvl s8, zero, s4\n\t" \
        "vsetvli zero, zero, e8, mf4, tu, mu\n\t" \
        "divu t3, t3, t4\n\t" \
        "divu t3, t3, t4\n\t" \
        "divu t3, t3, t4\n\t" \
        "divu t3, t3, t4\n\t" \
        "beq t3, zero, 2f\n\t" \
        "csrr t5, vl\n\t" \
        "2:\n\t" \
        "vmv.v.i v1, " value "\n\t" \
        "vse8.v v1, (%[dst])\n\t"

static uint8_t ai_vsetvl_flush_dst[AI_VSETVL_FLUSH_MAX_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_vsetvl_flush_dense_dst
    [AI_VSETVL_FLUSH_DENSE_REGS][AI_VSETVL_FLUSH_DENSE_BLOCK]
    __attribute__((aligned(64)));
static uint8_t ai_vsetvl_flush_split_src[AI_VSETVL_FLUSH_SPLIT_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_vsetvl_flush_split_dst[AI_VSETVL_FLUSH_SPLIT_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_vsetvl_flush_group_dump[AI_VSETVL_FLUSH_SPLIT_BYTES]
    __attribute__((aligned(64)));

static inline void ai_vsetvl_flush_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, AI_VSETVL_FLUSH_MSTATUS_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint64_t ai_vsetvl_flush_fractional_vl_request(uint64_t cap)
{
    uint64_t vlmax = CSRR(CSR_VLENB) / 4ULL;

    if (vlmax == 0ULL) {
        vlmax = 1ULL;
    }
    if (vlmax > cap) {
        vlmax = cap;
    }
    return vlmax;
}

static void ai_vsetvl_flush_fill(uint8_t *dst, size_t bytes, uint8_t value)
{
    for (size_t i = 0; i < bytes; ++i) {
        *((volatile uint8_t *)&dst[i]) = value;
    }
}

static bool ai_vsetvl_flush_prefix_equals(const uint8_t *dst,
    size_t bytes, uint8_t value)
{
    for (size_t i = 0; i < bytes; ++i) {
        if (*((volatile const uint8_t *)&dst[i]) != value) {
            return false;
        }
    }
    return true;
}

static bool ai_vsetvl_flush_suffix_equals(const uint8_t *dst,
    size_t start, size_t bytes, uint8_t value)
{
    for (size_t i = start; i < bytes; ++i) {
        if (*((volatile const uint8_t *)&dst[i]) != value) {
            return false;
        }
    }
    return true;
}

static inline void ai_vsetvl_flush_old_m8_to_mf4(uint8_t *dst,
    uint64_t requested_vl, uint64_t *old_vtype, uint64_t *new_vtype,
    uint64_t *observed_vl, uint64_t *observed_vstart)
{
    uint64_t old_type = 0ULL;
    uint64_t new_type = 0ULL;
    uint64_t vl = 0ULL;
    uint64_t vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[req], e64, m8, ta, ma\n\t"
        "csrr %[old_type], vtype\n\t"
        "li t1, 6\n\t"
        "vsetvl t0, %[req], t1\n\t"
        "vmv.v.i v1, 5\n\t"
        "vse8.v v1, (%[dst])\n\t"
        "fence rw, rw\n\t"
        "csrr %[vl], vl\n\t"
        "csrr %[new_type], vtype\n\t"
        "csrr %[vstart], vstart\n\t"
        ".option pop\n\t"
        : [old_type] "=&r"(old_type),
          [new_type] "=&r"(new_type),
          [vl] "=&r"(vl),
          [vstart] "=&r"(vstart)
        : [dst] "r"(dst),
          [req] "r"(requested_vl)
        : "t0", "t1", "memory");

    *old_vtype = old_type;
    *new_vtype = new_type;
    *observed_vl = vl;
    *observed_vstart = vstart;
}

static inline void ai_vsetvl_flush_vill_to_mf4(uint8_t *dst,
    uint64_t requested_vl, uint64_t *vill_vtype, uint64_t *new_vtype,
    uint64_t *observed_vl, uint64_t *observed_vstart)
{
    uint64_t vill_type = 0ULL;
    uint64_t new_type = 0ULL;
    uint64_t vl = 0ULL;
    uint64_t vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t1, -1\n\t"
        "vsetvl zero, zero, t1\n\t"
        "csrr %[vill_type], vtype\n\t"
        "li t1, 6\n\t"
        "vsetvl t0, %[req], t1\n\t"
        "vmv.v.i v2, 7\n\t"
        "vse8.v v2, (%[dst])\n\t"
        "fence rw, rw\n\t"
        "csrr %[vl], vl\n\t"
        "csrr %[new_type], vtype\n\t"
        "csrr %[vstart], vstart\n\t"
        ".option pop\n\t"
        : [vill_type] "=&r"(vill_type),
          [new_type] "=&r"(new_type),
          [vl] "=&r"(vl),
          [vstart] "=&r"(vstart)
        : [dst] "r"(dst),
          [req] "r"(requested_vl)
        : "t0", "t1", "memory");

    *vill_vtype = vill_type;
    *new_vtype = new_type;
    *observed_vl = vl;
    *observed_vstart = vstart;
}

static inline void ai_vsetvl_flush_dense_uops(uint8_t *dst,
    uint64_t requested_vl, uint64_t *old_vtype, uint64_t *new_vtype,
    uint64_t *observed_vl, uint64_t *observed_vstart)
{
    uint64_t old_type = 0ULL;
    uint64_t new_type = 0ULL;
    uint64_t vl = 0ULL;
    uint64_t vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[req], e64, m8, ta, ma\n\t"
        "csrr %[old_type], vtype\n\t"
        "li t2, 6\n\t"
        "vsetvl t0, %[req], t2\n\t"
        "vmv.v.i v1, 1\n\t"
        "vmv.v.i v2, 2\n\t"
        "vmv.v.i v3, 3\n\t"
        "vmv.v.i v4, 4\n\t"
        "vmv.v.i v5, 5\n\t"
        "vmv.v.i v6, 6\n\t"
        "vmv.v.i v7, 7\n\t"
        "mv t1, %[dst]\n\t"
        "vse8.v v1, (t1)\n\t"
        "addi t1, t1, 64\n\t"
        "vse8.v v2, (t1)\n\t"
        "addi t1, t1, 64\n\t"
        "vse8.v v3, (t1)\n\t"
        "addi t1, t1, 64\n\t"
        "vse8.v v4, (t1)\n\t"
        "addi t1, t1, 64\n\t"
        "vse8.v v5, (t1)\n\t"
        "addi t1, t1, 64\n\t"
        "vse8.v v6, (t1)\n\t"
        "addi t1, t1, 64\n\t"
        "vse8.v v7, (t1)\n\t"
        "fence rw, rw\n\t"
        "csrr %[vl], vl\n\t"
        "csrr %[new_type], vtype\n\t"
        "csrr %[vstart], vstart\n\t"
        ".option pop\n\t"
        : [old_type] "=&r"(old_type),
          [new_type] "=&r"(new_type),
          [vl] "=&r"(vl),
          [vstart] "=&r"(vstart)
        : [dst] "r"(dst),
          [req] "r"(requested_vl)
        : "t0", "t1", "t2", "memory");

    *old_vtype = old_type;
    *new_vtype = new_type;
    *observed_vl = vl;
    *observed_vstart = vstart;
}

static inline void ai_vsetvl_flush_user_sequence(uint8_t *dst,
    uint64_t *old_vtype, uint64_t *first_vtype, uint64_t *final_vtype,
    uint64_t *first_vl, uint64_t *final_vl, uint64_t *observed_vstart)
{
    uint64_t old_type = 0ULL;
    uint64_t first_type = 0ULL;
    uint64_t final_type = 0ULL;
    uint64_t first_len = 0ULL;
    uint64_t final_len = 0ULL;
    uint64_t vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e64, m8, ta, ma\n\t"
        "csrr %[old_type], vtype\n\t"
        "li s4, 6\n\t"
        "vsetvl s8, zero, s4\n\t"
        "vsetvli zero, zero, e8, mf4, tu, mu\n\t"
        "mv %[first_len], s8\n\t"
        "mv %[first_type], s4\n\t"
        "csrr %[final_len], vl\n\t"
        "csrr %[final_type], vtype\n\t"
        "vmv.v.i v1, 9\n\t"
        "vse8.v v1, (%[dst])\n\t"
        "fence rw, rw\n\t"
        "csrr %[vstart], vstart\n\t"
        ".option pop\n\t"
        : [old_type] "=&r"(old_type),
          [first_type] "=&r"(first_type),
          [final_type] "=&r"(final_type),
          [first_len] "=&r"(first_len),
          [final_len] "=&r"(final_len),
          [vstart] "=&r"(vstart)
        : [dst] "r"(dst)
        : "t0", "s4", "s8", "memory");

    *old_vtype = old_type;
    *first_vtype = first_type;
    *final_vtype = final_type;
    *first_vl = first_len;
    *final_vl = final_len;
    *observed_vstart = vstart;
}

static inline void ai_vsetvl_flush_initial_vill_keepvl_zero_gap(uint8_t *dst,
    uint64_t *vill_vtype, uint64_t *vill_vl,
    uint64_t *first_vtype, uint64_t *final_vtype, uint64_t *first_vl,
    uint64_t *final_vl, uint64_t *observed_vstart)
{
    uint64_t old_type = 0ULL;
    uint64_t old_len = 0ULL;
    uint64_t first_type = 0ULL;
    uint64_t final_type = 0ULL;
    uint64_t first_len = 0ULL;
    uint64_t final_len = 0ULL;
    uint64_t vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t1, -1\n\t"
        "vsetvl zero, zero, t1\n\t"
        "csrr %[old_type], vtype\n\t"
        "csrr %[old_len], vl\n\t"
        "li t2, 8\n\t"
        "1:\n\t"
        AI_VSETVL_FLUSH_KEEPVL_HOT_BODY("12")
        "addi t2, t2, -1\n\t"
        "bnez t2, 1b\n\t"
        ".balign 64\n\t"
        AI_VSETVL_FLUSH_KEEPVL_HOT_BODY("12")
        ".balign 64\n\t"
        "nop\n\t"
        AI_VSETVL_FLUSH_KEEPVL_HOT_BODY("12")
        ".balign 64\n\t"
        "nop\n\t"
        "nop\n\t"
        AI_VSETVL_FLUSH_KEEPVL_HOT_BODY("12")
        ".balign 64\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        AI_VSETVL_FLUSH_KEEPVL_HOT_BODY("12")
        "mv %[first_len], s8\n\t"
        "mv %[first_type], s4\n\t"
        "csrr %[final_len], vl\n\t"
        "csrr %[final_type], vtype\n\t"
        "fence rw, rw\n\t"
        "csrr %[vstart], vstart\n\t"
        ".option pop\n\t"
        : [old_type] "=&r"(old_type),
          [old_len] "=&r"(old_len),
          [first_type] "=&r"(first_type),
          [final_type] "=&r"(final_type),
          [first_len] "=&r"(first_len),
          [final_len] "=&r"(final_len),
          [vstart] "=&r"(vstart)
        : [dst] "r"(dst)
        : "t0", "t1", "t2", "t3", "t4", "t5", "s4", "s8", "memory");

    *vill_vtype = old_type;
    *vill_vl = old_len;
    *first_vtype = first_type;
    *final_vtype = final_type;
    *first_vl = first_len;
    *final_vl = final_len;
    *observed_vstart = vstart;
}

static inline void ai_vsetvl_flush_old_large_new_small_split(uint8_t *src,
    uint8_t *dst, uint64_t *old_vtype, uint64_t *new_vtype,
    uint64_t *old_vl, uint64_t *new_vl, uint64_t *observed_vstart)
{
    uint64_t old_type = 0ULL;
    uint64_t new_type = 0ULL;
    uint64_t old_len = 0ULL;
    uint64_t new_len = 0ULL;
    uint64_t vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e64, m8, ta, ma\n\t"
        "csrr %[old_type], vtype\n\t"
        "csrr %[old_len], vl\n\t"
        "li t1, 6\n\t"
        "vsetvl t0, zero, t1\n\t"
        "vle64.v v8, (%[src])\n\t"
        "vse64.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        "csrr %[new_len], vl\n\t"
        "csrr %[new_type], vtype\n\t"
        "csrr %[vstart], vstart\n\t"
        ".option pop\n\t"
        : [old_type] "=&r"(old_type),
          [new_type] "=&r"(new_type),
          [old_len] "=&r"(old_len),
          [new_len] "=&r"(new_len),
          [vstart] "=&r"(vstart)
        : [src] "r"(src),
          [dst] "r"(dst)
        : "t0", "t1", "memory");

    *old_vtype = old_type;
    *new_vtype = new_type;
    *old_vl = old_len;
    *new_vl = new_len;
    *observed_vstart = vstart;
}

static inline void ai_vsetvl_flush_old_small_new_large_split(uint8_t *src,
    uint8_t *dst, uint64_t *old_vtype, uint64_t *new_vtype,
    uint64_t *old_vl, uint64_t *new_vl, uint64_t *observed_vstart)
{
    uint64_t old_type = 0ULL;
    uint64_t new_type = 0ULL;
    uint64_t target_type = 0ULL;
    uint64_t old_len = 0ULL;
    uint64_t new_len = 0ULL;
    uint64_t vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e64, m8, ta, ma\n\t"
        "csrr %[target_type], vtype\n\t"
        "li t1, 6\n\t"
        "vsetvl t0, zero, t1\n\t"
        "csrr %[old_type], vtype\n\t"
        "csrr %[old_len], vl\n\t"
        "vsetvl t0, zero, %[target_type]\n\t"
        "vle64.v v8, (%[src])\n\t"
        "vse64.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        "csrr %[new_len], vl\n\t"
        "csrr %[new_type], vtype\n\t"
        "csrr %[vstart], vstart\n\t"
        ".option pop\n\t"
        : [old_type] "=&r"(old_type),
          [new_type] "=&r"(new_type),
          [target_type] "=&r"(target_type),
          [old_len] "=&r"(old_len),
          [new_len] "=&r"(new_len),
          [vstart] "=&r"(vstart)
        : [src] "r"(src),
          [dst] "r"(dst)
        : "t0", "t1", "memory");

    *old_vtype = old_type;
    *new_vtype = new_type;
    *old_vl = old_len;
    *new_vl = new_len;
    *observed_vstart = vstart;
}

static inline void ai_vsetvl_flush_old_lmul_group_side_effect(uint8_t *src,
    uint8_t *dst, uint8_t *group_dump, uint64_t *old_vtype,
    uint64_t *new_vtype, uint64_t *new_vl, uint64_t *dump_vl,
    uint64_t *observed_vstart)
{
    uint64_t old_type = 0ULL;
    uint64_t new_type = 0ULL;
    uint64_t new_len = 0ULL;
    uint64_t dump_len = 0ULL;
    uint64_t vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e8, m1, ta, ma\n\t"
        "vmv.v.i v10, 13\n\t"
        "vsetvli t0, zero, e64, m8, ta, ma\n\t"
        "csrr %[old_type], vtype\n\t"
        "li t1, 6\n\t"
        "vsetvl t0, zero, t1\n\t"
        "vle64.v v8, (%[src])\n\t"
        "vse64.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        "csrr %[new_len], vl\n\t"
        "csrr %[new_type], vtype\n\t"
        "csrr %[vstart], vstart\n\t"
        "vsetvli t0, zero, e8, m1, ta, ma\n\t"
        "csrr %[dump_len], vl\n\t"
        "vse8.v v10, (%[group_dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [old_type] "=&r"(old_type),
          [new_type] "=&r"(new_type),
          [new_len] "=&r"(new_len),
          [dump_len] "=&r"(dump_len),
          [vstart] "=&r"(vstart)
        : [src] "r"(src),
          [dst] "r"(dst),
          [group_dump] "r"(group_dump)
        : "t0", "t1", "memory");

    *old_vtype = old_type;
    *new_vtype = new_type;
    *new_vl = new_len;
    *dump_vl = dump_len;
    *observed_vstart = vstart;
}

static inline void ai_vsetvl_flush_commit_delayed_old_lmul_illegal(
    uint8_t *dst, uint64_t *old_vtype, uint64_t *new_vtype,
    uint64_t *old_vl, uint64_t *new_vl, uint64_t *observed_vstart,
    uint64_t *older_sink)
{
    uint64_t old_type = 0ULL;
    uint64_t new_type = 0ULL;
    uint64_t old_len = 0ULL;
    uint64_t new_len = 0ULL;
    uint64_t vstart = 0ULL;
    uint64_t sink = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e64, m8, ta, ma\n\t"
        "csrr %[old_type], vtype\n\t"
        "csrr %[old_len], vl\n\t"
        "li t3, -1\n\t"
        "li t4, 3\n\t"
        "divu t3, t3, t4\n\t"
        "divu t3, t3, t4\n\t"
        "divu t3, t3, t4\n\t"
        "divu t3, t3, t4\n\t"
        "divu t3, t3, t4\n\t"
        "divu t3, t3, t4\n\t"
        "li t1, 6\n\t"
        "vsetvl t0, zero, t1\n\t"
        "vmv.v.i v1, 11\n\t"
        "vse8.v v1, (%[dst])\n\t"
        "fence rw, rw\n\t"
        "csrr %[new_len], vl\n\t"
        "csrr %[new_type], vtype\n\t"
        "csrr %[vstart], vstart\n\t"
        "mv %[sink], t3\n\t"
        ".option pop\n\t"
        : [old_type] "=&r"(old_type),
          [new_type] "=&r"(new_type),
          [old_len] "=&r"(old_len),
          [new_len] "=&r"(new_len),
          [vstart] "=&r"(vstart),
          [sink] "=&r"(sink)
        : [dst] "r"(dst)
        : "t0", "t1", "t3", "t4", "memory");

    *old_vtype = old_type;
    *new_vtype = new_type;
    *old_vl = old_len;
    *new_vl = new_len;
    *observed_vstart = vstart;
    *older_sink = sink;
}

bool ai_micro_vsetvl_flush_old_lmul_illegal_killed_by_new_vtype_corner()
{
    TEST_START();

    const uint64_t requested_vl =
        ai_vsetvl_flush_fractional_vl_request(4ULL);
    uint64_t old_vtype = 0ULL;
    uint64_t new_vtype = 0ULL;
    uint64_t observed_vl = 0ULL;
    uint64_t observed_vstart = 0ULL;

    ai_vsetvl_flush_enable_vector_state();
    ai_vsetvl_flush_fill(ai_vsetvl_flush_dst,
        sizeof(ai_vsetvl_flush_dst), 0xa5U);

    TEST_SETUP_EXCEPT();
    ai_vsetvl_flush_old_m8_to_mf4(ai_vsetvl_flush_dst, requested_vl,
        &old_vtype, &new_vtype, &observed_vl, &observed_vstart);

    TEST_ASSERT("vsetvli e64/m8 followed by vsetvl e8/mf4 must flush stale old-LMUL illegal metadata for v1",
        excpt.triggered == false &&
        (old_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        (new_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        old_vtype != new_vtype &&
        observed_vl == requested_vl &&
        observed_vstart == 0ULL &&
        ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_dst,
            requested_vl, 5U) &&
        ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_dst,
            requested_vl, requested_vl + 8ULL, 0xa5U),
        "triggered=%d cause=0x%lx old_vtype=0x%lx new_vtype=0x%lx vl=%lu req=%lu vstart=%lu",
        excpt.triggered, excpt.cause, old_vtype, new_vtype,
        observed_vl, requested_vl, observed_vstart
    );

    TEST_END(ai_micro_vsetvl_flush_old_lmul_illegal_killed_by_new_vtype_corner);
}

bool ai_micro_vsetvl_flush_vill_to_fractional_lmul_no_stale_illegal_corner()
{
    TEST_START();

    const uint64_t requested_vl =
        ai_vsetvl_flush_fractional_vl_request(4ULL);
    uint64_t vill_vtype = 0ULL;
    uint64_t new_vtype = 0ULL;
    uint64_t observed_vl = 0ULL;
    uint64_t observed_vstart = 0ULL;

    ai_vsetvl_flush_enable_vector_state();
    ai_vsetvl_flush_fill(ai_vsetvl_flush_dst,
        sizeof(ai_vsetvl_flush_dst), 0x3cU);

    TEST_SETUP_EXCEPT();
    ai_vsetvl_flush_vill_to_mf4(ai_vsetvl_flush_dst, requested_vl,
        &vill_vtype, &new_vtype, &observed_vl, &observed_vstart);

    TEST_ASSERT("vsetvl-created vill must not leak as a stale illegal exception after vsetvl restores legal e8/mf4",
        excpt.triggered == false &&
        (vill_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) != 0ULL &&
        (new_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        observed_vl == requested_vl &&
        observed_vstart == 0ULL &&
        ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_dst,
            requested_vl, 7U) &&
        ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_dst,
            requested_vl, requested_vl + 8ULL, 0x3cU),
        "triggered=%d cause=0x%lx vill_vtype=0x%lx new_vtype=0x%lx vl=%lu req=%lu vstart=%lu",
        excpt.triggered, excpt.cause, vill_vtype, new_vtype,
        observed_vl, requested_vl, observed_vstart
    );

    TEST_END(ai_micro_vsetvl_flush_vill_to_fractional_lmul_no_stale_illegal_corner);
}

bool ai_micro_vsetvl_flush_dense_vector_uops_no_old_vtype_exception_corner()
{
    TEST_START();

    const uint64_t requested_vl =
        ai_vsetvl_flush_fractional_vl_request(4ULL);
    uint64_t old_vtype = 0ULL;
    uint64_t new_vtype = 0ULL;
    uint64_t observed_vl = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool dense_image_ok = true;

    ai_vsetvl_flush_enable_vector_state();
    for (unsigned i = 0; i < AI_VSETVL_FLUSH_DENSE_REGS; ++i) {
        ai_vsetvl_flush_fill(ai_vsetvl_flush_dense_dst[i],
            AI_VSETVL_FLUSH_DENSE_BLOCK, 0xd0U + i);
    }

    TEST_SETUP_EXCEPT();
    ai_vsetvl_flush_dense_uops(&ai_vsetvl_flush_dense_dst[0][0],
        requested_vl, &old_vtype, &new_vtype, &observed_vl,
        &observed_vstart);

    for (unsigned i = 0; i < AI_VSETVL_FLUSH_DENSE_REGS; ++i) {
        dense_image_ok = dense_image_ok &&
            ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_dense_dst[i],
                requested_vl, (uint8_t)(i + 1U)) &&
            ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_dense_dst[i],
                requested_vl, requested_vl + 8ULL,
                (uint8_t)(0xd0U + i));
    }

    TEST_ASSERT("dense v1-v7 younger uops after vsetvl must all be killed/redecoded rather than committing old e64/m8 illegal exceptions",
        excpt.triggered == false &&
        (old_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        (new_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        old_vtype != new_vtype &&
        observed_vl == requested_vl &&
        observed_vstart == 0ULL &&
        dense_image_ok,
        "triggered=%d cause=0x%lx old_vtype=0x%lx new_vtype=0x%lx vl=%lu req=%lu vstart=%lu dense_ok=%d",
        excpt.triggered, excpt.cause, old_vtype, new_vtype,
        observed_vl, requested_vl, observed_vstart, dense_image_ok
    );

    TEST_END(ai_micro_vsetvl_flush_dense_vector_uops_no_old_vtype_exception_corner);
}

bool ai_micro_vsetvl_flush_user_sequence_zero_avl_fractional_lmul_corner()
{
    TEST_START();

    uint64_t old_vtype = 0ULL;
    uint64_t first_vtype = 0ULL;
    uint64_t final_vtype = 0ULL;
    uint64_t first_vl = 0ULL;
    uint64_t final_vl = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool final_image_ok = false;

    ai_vsetvl_flush_enable_vector_state();
    ai_vsetvl_flush_fill(ai_vsetvl_flush_dst,
        sizeof(ai_vsetvl_flush_dst), 0x69U);

    TEST_SETUP_EXCEPT();
    ai_vsetvl_flush_user_sequence(ai_vsetvl_flush_dst, &old_vtype,
        &first_vtype, &final_vtype, &first_vl, &final_vl,
        &observed_vstart);

    final_image_ok =
        final_vl > 0ULL &&
        final_vl <= AI_VSETVL_FLUSH_MAX_BYTES - 8ULL &&
        ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_dst,
            final_vl, 9U) &&
        ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_dst,
            final_vl, final_vl + 8ULL, 0x69U);

    TEST_ASSERT("user sequence vsetvl s8,zero,s4(vtype=6) then vsetvli zero,zero,e8,mf4 must preserve current legal vector metadata",
        excpt.triggered == false &&
        (old_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        (first_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        (final_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        old_vtype != first_vtype &&
        first_vl == final_vl &&
        observed_vstart == 0ULL &&
        final_image_ok,
        "triggered=%d cause=0x%lx old=0x%lx first=0x%lx final=0x%lx first_vl=%lu final_vl=%lu vstart=%lu image_ok=%d",
        excpt.triggered, excpt.cause, old_vtype, first_vtype,
        final_vtype, first_vl, final_vl, observed_vstart,
        final_image_ok
    );

    TEST_END(ai_micro_vsetvl_flush_user_sequence_zero_avl_fractional_lmul_corner);
}

bool ai_micro_vsetvl_flush_initial_vill_keepvl_zero_gap_corner()
{
    TEST_START();

    const uint64_t expected_vl =
        ai_vsetvl_flush_fractional_vl_request(4ULL);
    uint64_t vill_vtype = 0ULL;
    uint64_t vill_vl = 0ULL;
    uint64_t first_vtype = 0ULL;
    uint64_t final_vtype = 0ULL;
    uint64_t first_vl = 0ULL;
    uint64_t final_vl = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool final_image_ok = false;

    ai_vsetvl_flush_enable_vector_state();
    ai_vsetvl_flush_fill(ai_vsetvl_flush_dst,
        sizeof(ai_vsetvl_flush_dst), 0x71U);

    TEST_SETUP_EXCEPT();
    ai_vsetvl_flush_initial_vill_keepvl_zero_gap(ai_vsetvl_flush_dst,
        &vill_vtype, &vill_vl, &first_vtype, &final_vtype,
        &first_vl, &final_vl, &observed_vstart);

    final_image_ok =
        final_vl > 0ULL &&
        final_vl <= AI_VSETVL_FLUSH_MAX_BYTES - 8ULL &&
        ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_dst,
            final_vl, 12U) &&
        ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_dst,
            final_vl, final_vl + 8ULL, 0x71U);

    TEST_ASSERT("vsetvl from committed vill/vl0 to e8/mf4 followed immediately by keep-vl vsetvli must use the just-produced vl rather than stale Decode vtypeSpec",
        excpt.triggered == false &&
        (vill_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) != 0ULL &&
        vill_vl == 0ULL &&
        (first_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        (final_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        first_vtype == final_vtype &&
        first_vl == expected_vl &&
        first_vl == final_vl &&
        observed_vstart == 0ULL &&
        final_image_ok,
        "triggered=%d cause=0x%lx vill=0x%lx vill_vl=%lu first=0x%lx final=0x%lx first_vl=%lu expected=%lu final_vl=%lu vstart=%lu image_ok=%d",
        excpt.triggered, excpt.cause, vill_vtype, vill_vl,
        first_vtype, final_vtype, first_vl, expected_vl, final_vl,
        observed_vstart, final_image_ok
    );

    TEST_END(ai_micro_vsetvl_flush_initial_vill_keepvl_zero_gap_corner);
}

bool ai_micro_vsetvl_flush_decode_old_large_new_small_split_count_corner()
{
    TEST_START();

    uint64_t old_vtype = 0ULL;
    uint64_t new_vtype = 0ULL;
    uint64_t old_vl = 0ULL;
    uint64_t new_vl = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool image_ok = false;
    bool range_ok = false;
    size_t active_bytes = 0U;

    ai_vsetvl_flush_enable_vector_state();
    ai_vsetvl_flush_fill(ai_vsetvl_flush_split_src,
        sizeof(ai_vsetvl_flush_split_src), 0x2aU);
    ai_vsetvl_flush_fill(ai_vsetvl_flush_split_dst,
        sizeof(ai_vsetvl_flush_split_dst), 0xb6U);

    TEST_SETUP_EXCEPT();
    ai_vsetvl_flush_old_large_new_small_split(ai_vsetvl_flush_split_src,
        ai_vsetvl_flush_split_dst, &old_vtype, &new_vtype, &old_vl,
        &new_vl, &observed_vstart);

    range_ok = new_vl > 0ULL &&
        new_vl <= (AI_VSETVL_FLUSH_SPLIT_BYTES -
            AI_VSETVL_FLUSH_GUARD_BYTES) /
            AI_VSETVL_FLUSH_WIDE_EEW_BYTES;
    if (range_ok) {
        active_bytes = (size_t)(new_vl * AI_VSETVL_FLUSH_WIDE_EEW_BYTES);
        image_ok =
            ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_split_dst,
                active_bytes, 0x2aU) &&
            ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_split_dst,
                active_bytes, active_bytes + AI_VSETVL_FLUSH_GUARD_BYTES,
                0xb6U);
    }

    TEST_ASSERT("vsetvl e64/m8 to e8/mf4 must let refetched vle64/vse64 decode with the new small vtype and not over-store old split lanes",
        excpt.triggered == false &&
        (old_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        (new_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        old_vtype != new_vtype &&
        old_vl > new_vl &&
        observed_vstart == 0ULL &&
        range_ok && image_ok,
        "triggered=%d cause=0x%lx old_vtype=0x%lx new_vtype=0x%lx old_vl=%lu new_vl=%lu vstart=%lu range_ok=%d image_ok=%d",
        excpt.triggered, excpt.cause, old_vtype, new_vtype,
        old_vl, new_vl, observed_vstart, range_ok, image_ok
    );

    TEST_END(ai_micro_vsetvl_flush_decode_old_large_new_small_split_count_corner);
}

bool ai_micro_vsetvl_flush_decode_old_small_new_large_split_count_corner()
{
    TEST_START();

    uint64_t old_vtype = 0ULL;
    uint64_t new_vtype = 0ULL;
    uint64_t old_vl = 0ULL;
    uint64_t new_vl = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool image_ok = false;
    bool range_ok = false;
    size_t active_bytes = 0U;

    ai_vsetvl_flush_enable_vector_state();
    ai_vsetvl_flush_fill(ai_vsetvl_flush_split_src,
        sizeof(ai_vsetvl_flush_split_src), 0x3dU);
    ai_vsetvl_flush_fill(ai_vsetvl_flush_split_dst,
        sizeof(ai_vsetvl_flush_split_dst), 0xc7U);

    TEST_SETUP_EXCEPT();
    ai_vsetvl_flush_old_small_new_large_split(ai_vsetvl_flush_split_src,
        ai_vsetvl_flush_split_dst, &old_vtype, &new_vtype, &old_vl,
        &new_vl, &observed_vstart);

    range_ok = new_vl > 0ULL &&
        new_vl <= (AI_VSETVL_FLUSH_SPLIT_BYTES -
            AI_VSETVL_FLUSH_GUARD_BYTES) /
            AI_VSETVL_FLUSH_WIDE_EEW_BYTES;
    if (range_ok) {
        active_bytes = (size_t)(new_vl * AI_VSETVL_FLUSH_WIDE_EEW_BYTES);
        image_ok =
            ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_split_dst,
                active_bytes, 0x3dU) &&
            ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_split_dst,
                active_bytes, active_bytes + AI_VSETVL_FLUSH_GUARD_BYTES,
                0xc7U);
    }

    TEST_ASSERT("vsetvl e8/mf4 to e64/m8 must let refetched vle64/vse64 decode with the new large vtype and not under-store old split lanes",
        excpt.triggered == false &&
        (old_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        (new_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        old_vtype != new_vtype &&
        new_vl > old_vl &&
        observed_vstart == 0ULL &&
        range_ok && image_ok,
        "triggered=%d cause=0x%lx old_vtype=0x%lx new_vtype=0x%lx old_vl=%lu new_vl=%lu vstart=%lu range_ok=%d image_ok=%d",
        excpt.triggered, excpt.cause, old_vtype, new_vtype,
        old_vl, new_vl, observed_vstart, range_ok, image_ok
    );

    TEST_END(ai_micro_vsetvl_flush_decode_old_small_new_large_split_count_corner);
}

bool ai_micro_vsetvl_flush_decode_old_lmul_group_side_effect_corner()
{
    TEST_START();

    uint64_t old_vtype = 0ULL;
    uint64_t new_vtype = 0ULL;
    uint64_t new_vl = 0ULL;
    uint64_t dump_vl = 0ULL;
    uint64_t observed_vstart = 0ULL;
    bool store_image_ok = false;
    bool group_image_ok = false;
    bool range_ok = false;
    size_t active_bytes = 0U;

    ai_vsetvl_flush_enable_vector_state();
    ai_vsetvl_flush_fill(ai_vsetvl_flush_split_src,
        sizeof(ai_vsetvl_flush_split_src), 0x42U);
    ai_vsetvl_flush_fill(ai_vsetvl_flush_split_dst,
        sizeof(ai_vsetvl_flush_split_dst), 0xd8U);
    ai_vsetvl_flush_fill(ai_vsetvl_flush_group_dump,
        sizeof(ai_vsetvl_flush_group_dump), 0xe9U);

    TEST_SETUP_EXCEPT();
    ai_vsetvl_flush_old_lmul_group_side_effect(ai_vsetvl_flush_split_src,
        ai_vsetvl_flush_split_dst, ai_vsetvl_flush_group_dump,
        &old_vtype, &new_vtype, &new_vl, &dump_vl, &observed_vstart);

    range_ok = new_vl > 0ULL && dump_vl > 0ULL &&
        new_vl <= (AI_VSETVL_FLUSH_SPLIT_BYTES -
            AI_VSETVL_FLUSH_GUARD_BYTES) /
            AI_VSETVL_FLUSH_WIDE_EEW_BYTES &&
        dump_vl <= AI_VSETVL_FLUSH_SPLIT_BYTES -
            AI_VSETVL_FLUSH_GUARD_BYTES;
    if (range_ok) {
        active_bytes = (size_t)(new_vl * AI_VSETVL_FLUSH_WIDE_EEW_BYTES);
        store_image_ok =
            ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_split_dst,
                active_bytes, 0x42U) &&
            ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_split_dst,
                active_bytes, active_bytes + AI_VSETVL_FLUSH_GUARD_BYTES,
                0xd8U);
        group_image_ok =
            ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_group_dump,
                dump_vl, 13U) &&
            ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_group_dump,
                dump_vl, dump_vl + AI_VSETVL_FLUSH_GUARD_BYTES, 0xe9U);
    }

    TEST_ASSERT("vsetvl old e64/m8 to e8/mf4 must not let refetched vle64 use the old LMUL group and clobber v10",
        excpt.triggered == false &&
        (old_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        (new_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        old_vtype != new_vtype &&
        observed_vstart == 0ULL &&
        range_ok && store_image_ok && group_image_ok,
        "triggered=%d cause=0x%lx old_vtype=0x%lx new_vtype=0x%lx new_vl=%lu dump_vl=%lu vstart=%lu range_ok=%d store_ok=%d group_ok=%d",
        excpt.triggered, excpt.cause, old_vtype, new_vtype,
        new_vl, dump_vl, observed_vstart, range_ok,
        store_image_ok, group_image_ok
    );

    TEST_END(ai_micro_vsetvl_flush_decode_old_lmul_group_side_effect_corner);
}

bool ai_micro_vsetvl_flush_commit_delayed_old_lmul_illegal_refetch_corner()
{
    TEST_START();

    uint64_t old_vtype = 0ULL;
    uint64_t new_vtype = 0ULL;
    uint64_t old_vl = 0ULL;
    uint64_t new_vl = 0ULL;
    uint64_t observed_vstart = 0ULL;
    uint64_t older_sink = 0ULL;
    bool image_ok = false;
    bool range_ok = false;

    ai_vsetvl_flush_enable_vector_state();
    ai_vsetvl_flush_fill(ai_vsetvl_flush_dst,
        sizeof(ai_vsetvl_flush_dst), 0x5aU);

    TEST_SETUP_EXCEPT();
    ai_vsetvl_flush_commit_delayed_old_lmul_illegal(ai_vsetvl_flush_dst,
        &old_vtype, &new_vtype, &old_vl, &new_vl, &observed_vstart,
        &older_sink);

    range_ok = new_vl > 0ULL && new_vl <= AI_VSETVL_FLUSH_MAX_BYTES - 8ULL;
    if (range_ok) {
        image_ok =
            ai_vsetvl_flush_prefix_equals(ai_vsetvl_flush_dst,
                new_vl, 11U) &&
            ai_vsetvl_flush_suffix_equals(ai_vsetvl_flush_dst,
                new_vl, new_vl + 8ULL, 0x5aU);
    }

    TEST_ASSERT("older long-latency uops before vsetvl should let refetched v1 decode with new e8/mf4 rather than stale old e64/m8 illegal metadata",
        excpt.triggered == false &&
        (old_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        (new_vtype & AI_VSETVL_FLUSH_VTYPE_VILL) == 0ULL &&
        old_vtype != new_vtype &&
        old_vl > new_vl &&
        observed_vstart == 0ULL &&
        older_sink != 0ULL &&
        range_ok && image_ok,
        "triggered=%d cause=0x%lx old_vtype=0x%lx new_vtype=0x%lx old_vl=%lu new_vl=%lu vstart=%lu sink=0x%lx range_ok=%d image_ok=%d",
        excpt.triggered, excpt.cause, old_vtype, new_vtype,
        old_vl, new_vl, observed_vstart, older_sink, range_ok, image_ok
    );

    TEST_END(ai_micro_vsetvl_flush_commit_delayed_old_lmul_illegal_refetch_corner);
}
