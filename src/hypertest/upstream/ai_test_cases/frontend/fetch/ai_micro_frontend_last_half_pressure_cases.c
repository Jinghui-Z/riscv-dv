#include <rvh_test.h>
#include <stdint.h>

#define AI_FRONTEND_PRESSURE_SPLIT_ADDI1 \
    "j 1f\n\t" \
    ".balign 32, 0\n\t" \
    "1:\n\t" \
    ".option rvc\n\t" \
    ".rept 15\n\t" \
    ".2byte 0x0001\n\t" \
    ".endr\n\t" \
    ".2byte 0x8293\n\t" \
    ".2byte 0x0012\n\t" \
    ".option norvc\n\t"

#define AI_FRONTEND_PRESSURE_DIV_CHAIN \
    "li t2, 3\n\t" \
    "li t3, 0x7fffffff\n\t" \
    ".rept 32\n\t" \
    "mul t3, t3, t2\n\t" \
    "addi t3, t3, 17\n\t" \
    "divu t3, t3, t2\n\t" \
    ".endr\n\t"

static volatile uint64_t ai_micro_frontend_last_half_pressure_data[512] __attribute__((aligned(4096)));

static void ai_micro_frontend_last_half_pressure_init_data(void)
{
    for (uint64_t i = 0; i < 512; ++i) {
        ai_micro_frontend_last_half_pressure_data[i] = i + 1;
    }
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_div_split_chain_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        AI_FRONTEND_PRESSURE_DIV_CHAIN
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_DIV_CHAIN
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_div_backpressure_split_chain_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_pressure_div_split_chain_probe();
    TEST_ASSERT("dependent divu pressure before a dense split-lastHalf chain does not leave stale f3_lastHalf state or drop any split addi",
        excpt.triggered == false &&
        observed == 16ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_load_split_chain_probe(volatile uint64_t *base)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        "ld t1, 0(%[base])\n\t"
        "add t0, t0, t1\n\t"
        "ld t1, 256(%[base])\n\t"
        "add t0, t0, t1\n\t"
        "ld t1, 512(%[base])\n\t"
        "add t0, t0, t1\n\t"
        "ld t1, 768(%[base])\n\t"
        "add t0, t0, t1\n\t"
        "ld t1, 1024(%[base])\n\t"
        "add t0, t0, t1\n\t"
        "ld t1, 1280(%[base])\n\t"
        "add t0, t0, t1\n\t"
        "ld t1, 1536(%[base])\n\t"
        "add t0, t0, t1\n\t"
        "ld t1, 1792(%[base])\n\t"
        "add t0, t0, t1\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [base] "r"(base)
        : "t0", "t1", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_load_backpressure_split_chain_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_micro_frontend_last_half_pressure_init_data();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_pressure_load_split_chain_probe(ai_micro_frontend_last_half_pressure_data);
    TEST_ASSERT("multi-line load pressure before a split-lastHalf chain preserves all load results and all split addi increments",
        excpt.triggered == false &&
        observed == 916ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_store_redirect_probe(volatile uint64_t *base)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        "li t1, 0x11\n\t"
        "sd t1, 0(%[base])\n\t"
        "li t1, 0x22\n\t"
        "sd t1, 256(%[base])\n\t"
        "li t1, 0x33\n\t"
        "sd t1, 512(%[base])\n\t"
        "li t1, 0x44\n\t"
        "sd t1, 768(%[base])\n\t"
        "li t1, 0x55\n\t"
        "sd t1, 1024(%[base])\n\t"
        "li t1, 0x66\n\t"
        "sd t1, 1280(%[base])\n\t"
        "li t1, 0x77\n\t"
        "sd t1, 1536(%[base])\n\t"
        "li t1, 0x88\n\t"
        "sd t1, 1792(%[base])\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 64\n\t"
        "1:\n\t"
        "addi t0, t0, 8\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 64\n\t"
        "1:\n\t"
        "addi t0, t0, 8\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 64\n\t"
        "1:\n\t"
        "addi t0, t0, 8\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 64\n\t"
        "1:\n\t"
        "addi t0, t0, 8\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [base] "r"(base)
        : "t0", "t1", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_store_backpressure_redirect_chain_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    ai_micro_frontend_last_half_pressure_init_data();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_pressure_store_redirect_probe(ai_micro_frontend_last_half_pressure_data);
    TEST_ASSERT("store-queue pressure followed by split-lastHalf plus same-window taken redirect skips poison and preserves store payloads",
        excpt.triggered == false &&
        observed == 36ULL &&
        ai_micro_frontend_last_half_pressure_data[0] == 0x11ULL &&
        ai_micro_frontend_last_half_pressure_data[32] == 0x22ULL &&
        ai_micro_frontend_last_half_pressure_data[64] == 0x33ULL &&
        ai_micro_frontend_last_half_pressure_data[96] == 0x44ULL &&
        ai_micro_frontend_last_half_pressure_data[128] == 0x55ULL &&
        ai_micro_frontend_last_half_pressure_data[160] == 0x66ULL &&
        ai_micro_frontend_last_half_pressure_data[192] == 0x77ULL &&
        ai_micro_frontend_last_half_pressure_data[224] == 0x88ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_alternating_branch_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        AI_FRONTEND_PRESSURE_DIV_CHAIN
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 64\n\t"
        "1:\n\t"
        "addi t0, t0, 3\n\t"
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "bne x0, x0, 1f\n\t"
        "addi t0, t0, 5\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 64\n\t"
        "2:\n\t"
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 64\n\t"
        "1:\n\t"
        "addi t0, t0, 3\n\t"
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "bne x0, x0, 1f\n\t"
        "addi t0, t0, 5\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 64\n\t"
        "2:\n\t"
        AI_FRONTEND_PRESSURE_DIV_CHAIN
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 64\n\t"
        "1:\n\t"
        "addi t0, t0, 3\n\t"
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "bne x0, x0, 1f\n\t"
        "addi t0, t0, 5\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 64\n\t"
        "2:\n\t"
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 64\n\t"
        "1:\n\t"
        "addi t0, t0, 3\n\t"
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "bne x0, x0, 1f\n\t"
        "addi t0, t0, 5\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 64\n\t"
        "2:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_alternating_branch_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_pressure_alternating_branch_probe();
    TEST_ASSERT("alternating split taken and not-taken branches under div pressure keep FTQ redirect repair separate from lastHalf state",
        excpt.triggered == false &&
        observed == 32ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_jalr_div_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        AI_FRONTEND_PRESSURE_DIV_CHAIN
        "la t2, 2f\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "jalr t1, 0(t2)\n\t"
        "1:\n\t"
        "addi t0, t0, 64\n\t"
        "j 3f\n\t"
        "2:\n\t"
        "la t3, 1b\n\t"
        "sub t4, t1, t3\n\t"
        "add t0, t0, t4\n\t"
        "addi t0, t0, 16\n\t"
        "3:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_jalr_redirect_under_div_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = 0;
    for (unsigned i = 0; i < 4; ++i) {
        observed += ai_micro_frontend_last_half_pressure_jalr_div_probe();
    }
    TEST_ASSERT("a lane1 JALR redirect immediately after split high-half under div pressure reaches target and reports the correct link point",
        excpt.triggered == false &&
        observed == 68ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_ebreak_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        AI_FRONTEND_PRESSURE_DIV_CHAIN
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        ".4byte 0x00100073\n\t"
        "addi t0, t0, 8\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_lane1_ebreak_under_backpressure_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_pressure_ebreak_probe();
    TEST_ASSERT("a lane1 ebreak immediately after split high-half remains precise when older div pressure can backpressure frontend enqueue",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        observed == 13ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_refetch_loop_probe(uint64_t iterations)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        "mv t1, %[iterations]\n\t"
        "li t2, 3\n\t"
        "li t3, 0x1000003\n\t"
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".option norvc\n\t"
        ".rept 4\n\t"
        "mul t3, t3, t2\n\t"
        "addi t3, t3, 19\n\t"
        "divu t3, t3, t2\n\t"
        ".endr\n\t"
        "addi t1, t1, -1\n\t"
        "bnez t1, 1b\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [iterations] "r"(iterations)
        : "t0", "t1", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_refetch_loop_under_div_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_pressure_refetch_loop_probe(97);
    TEST_ASSERT("a repeated branch-refetch loop over a 32B+30 split instruction under per-iteration div pressure retires each split exactly once",
        excpt.triggered == false &&
        observed == 97ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_fencei_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        AI_FRONTEND_PRESSURE_DIV_CHAIN
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "fence.i\n\t"
        "j 1f\n\t"
        "addi t0, t0, 64\n\t"
        "1:\n\t"
        "addi t0, t0, 32\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_fencei_flush_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_pressure_fencei_probe();
    TEST_ASSERT("fence.i flush immediately after split-lastHalf consumption followed by redirect does not preserve stale valid_dup or execute poison",
        excpt.triggered == false &&
        observed == 36ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_call_return_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        AI_FRONTEND_PRESSURE_DIV_CHAIN
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "jal ra, 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 5\n\t"
        "j 3f\n\t"
        "2:\n\t"
        "addi t0, t0, 11\n\t"
        "jalr x0, 0(ra)\n\t"
        "3:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "ra", "t0", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_call_return_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = 0;
    for (unsigned i = 0; i < 4; ++i) {
        observed += ai_micro_frontend_last_half_pressure_call_return_probe();
    }
    TEST_ASSERT("a split JAL call followed by return under div pressure keeps the link, RAS-style return path, and lastHalf state consistent",
        excpt.triggered == false &&
        observed == 64ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_pressure_large_stride_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        AI_FRONTEND_PRESSURE_DIV_CHAIN
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "addi t0, t0, 2\n\t"
        ".rept 64\n\t"
        ".4byte 0x00000013\n\t"
        ".endr\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "addi t0, t0, 2\n\t"
        ".rept 64\n\t"
        ".4byte 0x00000013\n\t"
        ".endr\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "addi t0, t0, 2\n\t"
        ".rept 64\n\t"
        ".4byte 0x00000013\n\t"
        ".endr\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "addi t0, t0, 2\n\t"
        ".rept 64\n\t"
        ".4byte 0x00000013\n\t"
        ".endr\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "addi t0, t0, 2\n\t"
        ".rept 64\n\t"
        ".4byte 0x00000013\n\t"
        ".endr\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "addi t0, t0, 2\n\t"
        ".rept 64\n\t"
        ".4byte 0x00000013\n\t"
        ".endr\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "addi t0, t0, 2\n\t"
        ".rept 64\n\t"
        ".4byte 0x00000013\n\t"
        ".endr\n\t"
        AI_FRONTEND_PRESSURE_SPLIT_ADDI1
        "addi t0, t0, 2\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_pressure_large_stride_fetch_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_pressure_large_stride_probe();
    TEST_ASSERT("large instruction-footprint split-lastHalf blocks keep lane0 high-half masking local across i-cache-line stride pressure",
        excpt.triggered == false &&
        observed == 24ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_false_lasthalf_pressure_midblock_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 0\n\t"
        "addi t3, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".p2align 5, 0\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 3\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "beq x0, x0, 2f\n\t"
        "addi t0, t0, 8\n\t"
        "jal x0, 4f\n\t"
        ".p2align 5, 0\n\t"
        "2:\n\t"
        "bnez t3, 3f\n\t"
        "addi t3, x0, 1\n\t"
        "la t1, 1b\n\t"
        "lhu t4, 0(t1)\n\t"
        "lhu t5, 2(t1)\n\t"
        "li t2, 0x8293\n\t"
        "sh t2, 0(t1)\n\t"
        "li t2, 0x0052\n\t"
        "sh t2, 2(t1)\n\t"
        "fence.i\n\t"
        "li t2, 3\n\t"
        "li t6, 0x7fffffff\n\t"
        ".rept 16\n\t"
        "mul t6, t6, t2\n\t"
        "addi t6, t6, 17\n\t"
        "divu t6, t6, t2\n\t"
        ".endr\n\t"
        "addi t0, x0, 1\n\t"
        "jal x0, 0b\n\t"
        "3:\n\t"
        "addi t0, t0, 128\n\t"
        "4:\n\t"
        "la t1, 1b\n\t"
        "sh t4, 0(t1)\n\t"
        "sh t5, 2(t1)\n\t"
        "fence.i\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    return observed;
}

bool ai_micro_frontend_false_lasthalf_pressure_midblock_div_repair_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_false_lasthalf_pressure_midblock_probe();
    TEST_ASSERT("mid-block stale FTQ taken offset rewritten to non-CFI under div pressure must half-flush to sequential PC instead of stale target",
        excpt.triggered == false &&
        observed == 14ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 7; ++i) {
        repeated += ai_micro_frontend_false_lasthalf_pressure_midblock_probe();
    }
    TEST_ASSERT("repeated mid-block false-lastHalf repair under div pressure keeps wb_false_lastHalf path stable across restore/patch cycles",
        excpt.triggered == false &&
        repeated == 98ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_false_lasthalf_pressure_lane14_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 0\n\t"
        "addi t3, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 14\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "beq x0, x0, 2f\n\t"
        "addi t0, t0, 8\n\t"
        "addi t0, t0, 16\n\t"
        "jal x0, 4f\n\t"
        ".p2align 5, 0\n\t"
        "2:\n\t"
        "bnez t3, 3f\n\t"
        "addi t3, x0, 1\n\t"
        "la t1, 1b\n\t"
        "lhu t4, 0(t1)\n\t"
        "lhu t5, 2(t1)\n\t"
        "li t2, 0x8293\n\t"
        "sh t2, 0(t1)\n\t"
        "li t2, 0x0052\n\t"
        "sh t2, 2(t1)\n\t"
        "fence.i\n\t"
        "li t2, 3\n\t"
        "li t6, 0x7fffffff\n\t"
        ".rept 16\n\t"
        "mul t6, t6, t2\n\t"
        "addi t6, t6, 17\n\t"
        "divu t6, t6, t2\n\t"
        ".endr\n\t"
        "addi t0, x0, 1\n\t"
        "jal x0, 0b\n\t"
        "3:\n\t"
        "addi t0, t0, 128\n\t"
        "4:\n\t"
        "la t1, 1b\n\t"
        "sh t4, 0(t1)\n\t"
        "sh t5, 2(t1)\n\t"
        "fence.i\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    return observed;
}

bool ai_micro_frontend_false_lasthalf_pressure_lane14_div_repair_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_false_lasthalf_pressure_lane14_probe();
    TEST_ASSERT("lane14 stale FTQ taken offset rewritten to non-CFI under div pressure must use the next-block f3_half_snpc repair target",
        excpt.triggered == false &&
        observed == 30ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 7; ++i) {
        repeated += ai_micro_frontend_false_lasthalf_pressure_lane14_probe();
    }
    TEST_ASSERT("repeated lane14 false-lastHalf repair under div pressure preserves the PredictWidth-2 next-block target",
        excpt.triggered == false &&
        repeated == 210ULL
    );

    TEST_END();
}

bool ai_micro_frontend_false_lasthalf_pressure_midblock_lane14_pair_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_false_lasthalf_pressure_midblock_probe() +
        ai_micro_frontend_false_lasthalf_pressure_lane14_probe();
    TEST_ASSERT("back-to-back mid-block and lane14 false-lastHalf repairs under div pressure both avoid the stale FTQ target",
        excpt.triggered == false &&
        observed == 44ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 4; ++i) {
        repeated += ai_micro_frontend_false_lasthalf_pressure_midblock_probe();
        repeated += ai_micro_frontend_false_lasthalf_pressure_lane14_probe();
    }
    TEST_ASSERT("alternating mid-block and lane14 false-lastHalf repairs keep both half-snpc target formulas stable under repeated patch/restore pressure",
        excpt.triggered == false &&
        repeated == 176ULL
    );

    TEST_END();
}
