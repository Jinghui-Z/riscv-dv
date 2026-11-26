#include <rvh_test.h>
#include <stdint.h>
#include <stddef.h>

#define AI_VFREG_PREG0_MSTATUS_VS_DIRTY (3ULL << 9)
#define AI_VFREG_PREG0_MSTATUS_FS_DIRTY (3ULL << 13)
#define AI_VFREG_PREG0_ROUNDS           260U
#define AI_VFREG_PREG0_BUF_BYTES        256U

static uint8_t ai_vfreg_preg0_probe_buf[AI_VFREG_PREG0_BUF_BYTES]
    __attribute__((aligned(64)));

static inline void ai_vfreg_preg0_enable_vf_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, AI_VFREG_PREG0_MSTATUS_VS_DIRTY);
    CSRS(CSR_MSTATUS, AI_VFREG_PREG0_MSTATUS_FS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static void ai_vfreg_preg0_clear_buf(void)
{
    for (size_t i = 0; i < AI_VFREG_PREG0_BUF_BYTES; ++i) {
        ai_vfreg_preg0_probe_buf[i] = 0xa5U;
    }
}

__attribute__((naked, noinline, aligned(64))) static void
ai_vfreg_preg0_pressure_kernel(uint8_t *dst, uint64_t rounds)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 64\n\t"
        "vsetvli t1, t0, e8, m1, ta, ma\n\t"
        "beqz a1, 2f\n\t"
        "1:\n\t"
        "vmv.v.i v0, 0\n\t"
        "vmv.v.i v1, 1\n\t"
        "vmv.v.i v2, 2\n\t"
        "vmv.v.i v3, 3\n\t"
        "vmv.v.i v4, 4\n\t"
        "vmv.v.i v5, 5\n\t"
        "vmv.v.i v6, 6\n\t"
        "vmv.v.i v7, 7\n\t"
        "vmv.v.i v8, 8\n\t"
        "vmv.v.i v9, 9\n\t"
        "vmv.v.i v10, 10\n\t"
        "vmv.v.i v11, 11\n\t"
        "vmv.v.i v12, 12\n\t"
        "vmv.v.i v13, 13\n\t"
        "vmv.v.i v14, 14\n\t"
        "vmv.v.i v15, 15\n\t"
        "vmv.v.i v16, -16\n\t"
        "vmv.v.i v17, -15\n\t"
        "vmv.v.i v18, -14\n\t"
        "vmv.v.i v19, -13\n\t"
        "vmv.v.i v20, -12\n\t"
        "vmv.v.i v21, -11\n\t"
        "vmv.v.i v22, -10\n\t"
        "vmv.v.i v23, -9\n\t"
        "vmv.v.i v24, -8\n\t"
        "vmv.v.i v25, -7\n\t"
        "vmv.v.i v26, -6\n\t"
        "vmv.v.i v27, -5\n\t"
        "vmv.v.i v28, -4\n\t"
        "vmv.v.i v29, -3\n\t"
        "vmv.v.i v30, -2\n\t"
        "vmv.v.i v31, -1\n\t"
        "vse8.v v1, 0(a0)\n\t"
        "addi t3, a0, 64\n\t"
        "vse8.v v16, (t3)\n\t"
        "li t2, 0x3ff0000000000000\n\t"
        "fmv.d.x f0, t2\n\t"
        "fmv.d.x f1, t2\n\t"
        "fmv.d.x f2, t2\n\t"
        "fmv.d.x f3, t2\n\t"
        "fmv.d.x f4, t2\n\t"
        "fmv.d.x f5, t2\n\t"
        "fmv.d.x f6, t2\n\t"
        "fmv.d.x f7, t2\n\t"
        "fmv.d.x f8, t2\n\t"
        "fmv.d.x f9, t2\n\t"
        "fmv.d.x f10, t2\n\t"
        "fmv.d.x f11, t2\n\t"
        "fmv.d.x f12, t2\n\t"
        "fmv.d.x f13, t2\n\t"
        "fmv.d.x f14, t2\n\t"
        "fmv.d.x f15, t2\n\t"
        "fmv.d.x f16, t2\n\t"
        "fmv.d.x f17, t2\n\t"
        "fmv.d.x f18, t2\n\t"
        "fmv.d.x f19, t2\n\t"
        "fmv.d.x f20, t2\n\t"
        "fmv.d.x f21, t2\n\t"
        "fmv.d.x f22, t2\n\t"
        "fmv.d.x f23, t2\n\t"
        "fmv.d.x f24, t2\n\t"
        "fmv.d.x f25, t2\n\t"
        "fmv.d.x f26, t2\n\t"
        "fmv.d.x f27, t2\n\t"
        "fmv.d.x f28, t2\n\t"
        "fmv.d.x f29, t2\n\t"
        "fmv.d.x f30, t2\n\t"
        "fmv.d.x f31, t2\n\t"
        "addi a1, a1, -1\n\t"
        "bnez a1, 1b\n\t"
        "fence rw, rw\n\t"
        "2:\n\t"
        "ret\n\t"
        ".option pop\n\t");
}

bool ai_micro_backend_vfregfile_preg0_write_pressure_corner()
{
    TEST_START();

    ai_vfreg_preg0_enable_vf_state();
    ai_vfreg_preg0_clear_buf();

    TEST_SETUP_EXCEPT();
    ai_vfreg_preg0_pressure_kernel(ai_vfreg_preg0_probe_buf,
        AI_VFREG_PREG0_ROUNDS);
    TEST_ASSERT("VF/FP register pressure probe completes without an architectural exception",
        excpt.triggered == false,
        "triggered=%d cause=%lu tval=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval);

    TEST_ASSERT("vector pressure kernel visibly wrote non-v0 VF data",
        ai_vfreg_preg0_probe_buf[0] == 0x01U &&
        ai_vfreg_preg0_probe_buf[64] == 0xf0U,
        "byte0=0x%x byte64=0x%x",
        ai_vfreg_preg0_probe_buf[0], ai_vfreg_preg0_probe_buf[64]);

    TEST_END();
}
