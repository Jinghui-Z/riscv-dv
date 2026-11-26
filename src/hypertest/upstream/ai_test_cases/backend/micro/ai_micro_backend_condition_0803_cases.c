#include <rvh_test.h>
#include <stdint.h>

#define AI_COND0803_MSTATUS_VS_DIRTY (3ULL << 9)
#define AI_COND0803_TIMEOUT_LINES 192U
#define AI_COND0803_CANCEL_LINES 96U
#define AI_COND0803_CANCEL_TARGETS 8U

static volatile uint64_t ai_cond0803_timeout_lines[AI_COND0803_TIMEOUT_LINES][8]
    __attribute__((aligned(4096)));
static volatile uint64_t ai_cond0803_cancel_lines[AI_COND0803_CANCEL_LINES][8]
    __attribute__((aligned(4096)));
static volatile uint64_t ai_cond0803_cancel_targets[AI_COND0803_CANCEL_TARGETS][8]
    __attribute__((aligned(4096)));
static volatile uint64_t ai_cond0803_vec_src_a[8]
    __attribute__((aligned(64)));
static volatile uint64_t ai_cond0803_vec_src_b[8]
    __attribute__((aligned(64)));
static volatile uint64_t ai_cond0803_vec_dst[8]
    __attribute__((aligned(64)));

static inline void ai_cond0803_enable_fp_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY | AI_COND0803_MSTATUS_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_FFLAGS, 0ULL);
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_cond0803_i2f_iq_fill_probe(void)
{
    uint64_t result;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0x5ffffffffffffd\n\t"
        "li t1, 3\n\t"
        "divu t2, t0, t1\n\t"
        "fcvt.d.l f0, t2, rne\n\t"
        "fcvt.d.l f1, t2, rne\n\t"
        "fcvt.d.l f2, t2, rne\n\t"
        "fcvt.d.l f3, t2, rne\n\t"
        "fcvt.d.l f4, t2, rne\n\t"
        "fcvt.d.l f5, t2, rne\n\t"
        "fcvt.d.l f6, t2, rne\n\t"
        "fcvt.d.l f7, t2, rne\n\t"
        "fcvt.d.l f8, t2, rne\n\t"
        "fcvt.d.l f9, t2, rne\n\t"
        "fcvt.d.l f10, t2, rne\n\t"
        "fcvt.d.l f11, t2, rne\n\t"
        "fcvt.d.l f12, t2, rne\n\t"
        "fcvt.d.l f13, t2, rne\n\t"
        "fcvt.d.l f14, t2, rne\n\t"
        "fcvt.d.l f15, t2, rne\n\t"
        "fcvt.d.l f16, t2, rne\n\t"
        "fcvt.d.l f17, t2, rne\n\t"
        "fcvt.d.l f18, t2, rne\n\t"
        "fcvt.d.l f19, t2, rne\n\t"
        "fcvt.d.l f20, t2, rne\n\t"
        "fcvt.d.l f21, t2, rne\n\t"
        "fcvt.d.l f22, t2, rne\n\t"
        "fcvt.d.l f23, t2, rne\n\t"
        "fmv.x.d %[result], f23\n\t"
        ".option pop\n\t"
        : [result] "=r"(result)
        :
        : "t0", "t1", "t2", "f0", "f1", "f2", "f3", "f4", "f5",
          "f6", "f7", "f8", "f9", "f10", "f11", "f12", "f13",
          "f14", "f15", "f16", "f17", "f18", "f19", "f20", "f21",
          "f22", "f23", "memory");

    return result;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_cond0803_timeout_load_burst(uintptr_t base)
{
    uint64_t last;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 16\n\t"
        "vsetvli t1, t0, e64, m8, tu, mu\n\t"
        /*
         * Keep thirty blocker loads unresolved with a long, independent FP
         * dependency chain.  Their addresses eventually reduce to base, but
         * they continue occupying all fifteen complex entries in each LDU IQ
         * while the shorter integer chain below wakes the enqueue targets.
         */
        "li t0, 0x5ffffffffffffd\n\t"
        "fcvt.d.lu f0, t0, rne\n\t"
        "li t1, 3\n\t"
        "fcvt.d.lu f1, t1, rne\n\t"
        ".rept 16\n\t"
        "fdiv.d f0, f0, f1, rne\n\t"
        ".endr\n\t"
        "fcvt.lu.d t4, f0, rtz\n\t"
        "and t4, t4, zero\n\t"
        "add t4, t4, %[base]\n\t"
        ".set ai_cond0803_dep_offset, 0\n\t"
        ".rept 30\n\t"
        "ld t6, ai_cond0803_dep_offset(t4)\n\t"
        ".set ai_cond0803_dep_offset, ai_cond0803_dep_offset + 64\n\t"
        ".endr\n\t"
        /*
         * Two consecutive target loads occupy the two enqueue entries.  Three
         * dependent DIV operations delay their wakeup until the following
         * vector stream has reached VLSplit, but finish well before the FP
         * chain releases any complex entry.
         */
        "li t2, 8192\n\t"
        "add t5, %[base], t2\n\t"
        "li t2, 3\n\t"
        ".rept 2\n\t"
        "slli t1, t5, 1\n\t"
        "add t1, t1, t5\n\t"
        "divu t5, t1, t2\n\t"
        ".endr\n\t"
        /* Release VLSplit one DIV latency before the scalar targets. */
        "and t3, t5, zero\n\t"
        "add t3, t3, %[base]\n\t"
        "addi t3, t3, 8\n\t"
        "li t2, 1\n\t"
        "divu t5, t5, t2\n\t"
        "ld t0, 0(t5)\n\t"
        "ld t6, 64(t5)\n\t"
        /*
         * VLSplit feeds LoadUnit0 while the cold dependent burst also fills
         * DCache miss resources.  Vector issue has priority over scalar LDU
         * issue at LoadUnit S0, so the older scalar request remains in
         * enqueue entry0 under valid && !ready pressure after it wakes.
         */
        ".rept 12\n\t"
        "vle64.v v0, (t3)\n\t"
        "vle64.v v8, (t3)\n\t"
        "vle64.v v16, (t3)\n\t"
        "vle64.v v24, (t3)\n\t"
        ".endr\n\t"
        "mv %[last], t0\n\t"
        ".option pop\n\t"
        : [last] "=r"(last)
        : [base] "r"(base)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "f0", "f1",
          "memory");

    return last;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_cond0803_vf_iq_fp_wakeup_probe(uintptr_t src_a, uintptr_t src_b,
    uintptr_t dst)
{
    uint64_t result;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fld f0, 0(%[src_a])\n\t"
        ".rept 128\n\t"
        /* Scalar FP convert is accepted only by the VEXU1 issue block. */
        "fcvt.s.d f1, f0, rne\n\t"
        "fcvt.d.s f0, f1\n\t"
        ".endr\n\t"
        "fmv.x.d %[result], f0\n\t"
        "sd %[result], 0(%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [result] "=r"(result)
        : [src_a] "r"(src_a), [src_b] "r"(src_b), [dst] "r"(dst)
        : "f0", "f1", "memory");

    return result;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_cond0803_vecmem_i2v_wakeup_probe(uintptr_t src, uintptr_t dst)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 8\n\t"
        "vsetvli t1, t0, e64, m8, tu, mu\n\t"
        "vle64.v v8, (%[src])\n\t"
        /*
         * Each store splits into one I2V address uop followed by eight VLSU
         * uops whose source zero is that I2V temporary register.  A continuous
         * stream keeps entry0 resident when delayed I2V port2 writeback arrives.
         */
        ".rept 192\n\t"
        "vse64.v v8, (%[dst])\n\t"
        ".endr\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src), [dst] "r"(dst)
        : "t0", "t1", "memory");

    return ai_cond0803_vec_dst[0];
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_cond0803_load_success_cancel_probe(uintptr_t pointer_base)
{
    uint64_t last;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t3, %[base]\n\t"
        ".rept 8\n\t"
        /*
         * The first cold pointer miss cancels speculative wakeup of 30
         * consumers, filling both LDU paths before the second pair arrives.
         */
        "ld t4, 0(t3)\n\t"
        ".rept 30\n\t"
        "ld a5, 0(t4)\n\t"
        ".endr\n\t"
        /* Keep this pointer/dependent pair in entry0 across its feedback. */
        "ld t0, 64(t3)\n\t"
        "ld t1, 0(t0)\n\t"
        "addi t3, t3, 128\n\t"
        ".endr\n\t"
        "mv %[last], t1\n\t"
        ".option pop\n\t"
        : [last] "=r"(last)
        : [base] "r"(pointer_base)
        : "t0", "t1", "t3", "t4", "a5", "memory");

    return last;
}

bool ai_micro_backend_i2f_iq_full_long_div_dependency_corner()
{
    TEST_START();

    ai_cond0803_enable_fp_vector_state();
    TEST_SETUP_EXCEPT();
    const uint64_t result = ai_cond0803_i2f_iq_fill_probe();

    TEST_ASSERT("a long DIV producer holds at least eighteen dependent I2F uops in the ALU2/BJU2 issue queue and all conversions retire correctly",
        excpt.triggered == false && result == 0x433fffffffffffffULL,
        "result=0x%lx", result);

    TEST_END();
}

bool ai_micro_backend_ldu_final_issue_timeout_backpressure_corner()
{
    TEST_START();

    ai_cond0803_enable_fp_vector_state();
    for (unsigned i = 0; i < AI_COND0803_TIMEOUT_LINES; ++i) {
        ai_cond0803_timeout_lines[i][0] =
            0x0803000000000000ULL ^ ((uint64_t)i << 17);
        ai_cond0803_timeout_lines[i][1] =
            0x1803000000000008ULL ^ ((uint64_t)i << 17);
    }
    TEST_SETUP_EXCEPT();
    for (unsigned i = 0; i < AI_COND0803_TIMEOUT_LINES; ++i) {
        cbo_flush((uintptr_t)&ai_cond0803_timeout_lines[i][0]);
    }
    fence_iorw_iorw();

    const uint64_t last = ai_cond0803_timeout_load_burst(
        (uintptr_t)&ai_cond0803_timeout_lines[0][0]);
    const uint64_t expected = 0x0803000000000000ULL ^
        ((uint64_t)128U << 17);

    TEST_ASSERT("a sustained e64,m8 vector-load stream holds scalar LDU requests behind LoadUnit0 priority arbitration until finalIssueResp retry, without losing the scalar payload",
        excpt.triggered == false && last == expected,
        "last=0x%lx expected=0x%lx", last, expected);

    TEST_END();
}

bool ai_micro_backend_vf_iq_vexu1_fp_wakeup_corner()
{
    TEST_START();

    ai_cond0803_enable_fp_vector_state();
    for (unsigned i = 0; i < 8U; ++i) {
        ai_cond0803_vec_src_a[i] = 0x3ff0000000000000ULL;
        ai_cond0803_vec_src_b[i] = 0x3ff0000000000000ULL;
        ai_cond0803_vec_dst[i] = 0ULL;
    }

    TEST_SETUP_EXCEPT();
    const uint64_t result = ai_cond0803_vf_iq_fp_wakeup_probe(
        (uintptr_t)&ai_cond0803_vec_src_a[0],
        (uintptr_t)&ai_cond0803_vec_src_b[0],
        (uintptr_t)&ai_cond0803_vec_dst[0]);

    TEST_ASSERT("VEXU1 scalar-FP conversion results wake dependent scalar-FP source-zero entries",
        excpt.triggered == false && result == 0x3ff0000000000000ULL,
        "result=0x%lx", result);

    TEST_END();
}

bool ai_micro_backend_vecmem_i2v_wakeup_port2_corner()
{
    TEST_START();

    ai_cond0803_enable_fp_vector_state();
    for (unsigned i = 0; i < 8U; ++i) {
        ai_cond0803_vec_src_a[i] = 0x4010000000000000ULL;
        ai_cond0803_vec_src_b[i] = 0x4000000000000000ULL;
        ai_cond0803_vec_dst[i] = 0ULL;
    }
    TEST_SETUP_EXCEPT();
    const uint64_t result = ai_cond0803_vecmem_i2v_wakeup_probe(
        (uintptr_t)&ai_cond0803_vec_src_a[0],
        (uintptr_t)&ai_cond0803_vec_dst[0]);

    TEST_ASSERT("a continuous e64,m8 unit-stride store stream retires correctly while split I2V address uops wake VecMem entry0 source zero through VF writeback port 2",
        excpt.triggered == false && result == 0x4010000000000000ULL,
        "result=0x%lx", result);

    TEST_END();
}

bool ai_micro_backend_ldu_success_cancel_same_cycle_pressure_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_COND0803_CANCEL_TARGETS; ++i) {
        ai_cond0803_cancel_targets[i][0] =
            0xcace000000000000ULL ^ ((uint64_t)i << 20);
    }
    for (unsigned i = 0; i < AI_COND0803_CANCEL_LINES; ++i) {
        ai_cond0803_cancel_lines[i][0] = (uintptr_t)
            &ai_cond0803_cancel_targets[i % AI_COND0803_CANCEL_TARGETS][0];
    }

    TEST_SETUP_EXCEPT();
    for (unsigned i = 0; i < AI_COND0803_CANCEL_LINES; ++i) {
        cbo_flush((uintptr_t)&ai_cond0803_cancel_lines[i][0]);
    }
    fence_iorw_iorw();

    uint64_t warm_signature = 0ULL;
    for (unsigned i = 0; i < AI_COND0803_CANCEL_TARGETS; ++i) {
        warm_signature += ai_cond0803_cancel_targets[i][0];
    }
    fence_iorw_iorw();

    const uint64_t last = ai_cond0803_load_success_cancel_probe(
        (uintptr_t)&ai_cond0803_cancel_lines[0][0]);
    const unsigned last_target =
        ((2U * 8U) - 1U) % AI_COND0803_CANCEL_TARGETS;
    const uint64_t expected = 0xcace000000000000ULL ^
        ((uint64_t)last_target << 20);

    TEST_ASSERT("cold pointer loads repeatedly cancel speculative dependents while hot dependent loads still retire with the selected target payload",
        excpt.triggered == false && warm_signature != 0ULL && last == expected,
        "last=0x%lx expected=0x%lx warm=0x%lx",
        last, expected, warm_signature);

    TEST_END();
}
