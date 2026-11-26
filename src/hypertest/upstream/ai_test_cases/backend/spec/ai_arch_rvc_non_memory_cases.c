#include <rvh_test.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_RVC_ATTR __attribute__((noinline, noclone, aligned(32)))

static uint64_t ai_arch_rvc_nonmem_obs[64] __attribute__((aligned(16)));

#define AI_RVC_EXEC_HALF_CAPTURE(half_, pc_) \
    asm volatile( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        "lla %[pc], 1f\n\t" \
        "1:\n\t" \
        ".2byte " #half_ "\n\t" \
        ".option pop\n\t" \
        : [pc] "=r"(pc_) \
        : \
        : "memory")

#define AI_RVC_EXEC_WORD_CAPTURE(word_, pc_) \
    asm volatile( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        "lla %[pc], 1f\n\t" \
        "1:\n\t" \
        ".word " #word_ "\n\t" \
        ".option pop\n\t" \
        : [pc] "=r"(pc_) \
        : \
        : "memory")

static AI_RVC_ATTR void ai_arch_rvc_control_transfer_probe(uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"

        "li t0, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        "c.j 3f\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "li t0, 0xbadc0de\n\t"
        "j 4f\n\t"
        "3:\n\t"
        ".option norvc\n\t"
        "li t0, 0x11\n\t"
        "4:\n\t"
        "sd t0, 0(%[obs])\n\t"
        "lla t1, 2b\n\t"
        "lla t2, 1b\n\t"
        "sub t1, t1, t2\n\t"
        "sd t1, 8(%[obs])\n\t"

        "lla a5, 7f\n\t"
        "li t0, 0\n\t"
        "5:\n\t"
        ".option rvc\n\t"
        "c.jr a5\n\t"
        "6:\n\t"
        ".option norvc\n\t"
        "li t0, 0xbadc0de\n\t"
        "j 8f\n\t"
        "7:\n\t"
        ".option norvc\n\t"
        "li t0, 0x22\n\t"
        "8:\n\t"
        "sd t0, 16(%[obs])\n\t"
        "lla t1, 6b\n\t"
        "lla t2, 5b\n\t"
        "sub t1, t1, t2\n\t"
        "sd t1, 24(%[obs])\n\t"

        "mv t6, ra\n\t"
        "lla a5, 11f\n\t"
        "9:\n\t"
        ".option rvc\n\t"
        "c.jalr a5\n\t"
        "10:\n\t"
        ".option norvc\n\t"
        "mv ra, t6\n\t"
        "j 12f\n\t"
        "11:\n\t"
        ".option norvc\n\t"
        "lla t1, 9b\n\t"
        "sub t0, ra, t1\n\t"
        "sd t0, 32(%[obs])\n\t"
        "jr ra\n\t"
        "12:\n\t"
        "lla t1, 10b\n\t"
        "lla t2, 9b\n\t"
        "sub t1, t1, t2\n\t"
        "sd t1, 40(%[obs])\n\t"

        "li a5, 0\n\t"
        "li t0, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        "c.beqz a5, 3f\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "li t0, 0xbadc0de\n\t"
        "j 4f\n\t"
        "3:\n\t"
        ".option norvc\n\t"
        "li t0, 0x31\n\t"
        "4:\n\t"
        "sd t0, 48(%[obs])\n\t"
        "lla t1, 2b\n\t"
        "lla t2, 1b\n\t"
        "sub t1, t1, t2\n\t"
        "sd t1, 56(%[obs])\n\t"

        "li a5, 0\n\t"
        "li t0, 0\n\t"
        "5:\n\t"
        ".option rvc\n\t"
        "c.bnez a5, 7f\n\t"
        "6:\n\t"
        ".option norvc\n\t"
        "li t0, 0x32\n\t"
        "j 8f\n\t"
        "7:\n\t"
        ".option norvc\n\t"
        "li t0, 0xbadc0de\n\t"
        "8:\n\t"
        "sd t0, 64(%[obs])\n\t"
        "lla t1, 6b\n\t"
        "lla t2, 5b\n\t"
        "sub t1, t1, t2\n\t"
        "sd t1, 72(%[obs])\n\t"

        "li a5, 1\n\t"
        "li t0, 0\n\t"
        "9:\n\t"
        ".option rvc\n\t"
        "c.beqz a5, 11f\n\t"
        "10:\n\t"
        ".option norvc\n\t"
        "li t0, 0x33\n\t"
        "j 12f\n\t"
        "11:\n\t"
        ".option norvc\n\t"
        "li t0, 0xbadc0de\n\t"
        "12:\n\t"
        "sd t0, 80(%[obs])\n\t"
        "lla t1, 10b\n\t"
        "lla t2, 9b\n\t"
        "sub t1, t1, t2\n\t"
        "sd t1, 88(%[obs])\n\t"

        "li a5, 1\n\t"
        "li t0, 0\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        "c.bnez a5, 3f\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "li t0, 0xbadc0de\n\t"
        "j 4f\n\t"
        "3:\n\t"
        ".option norvc\n\t"
        "li t0, 0x34\n\t"
        "4:\n\t"
        "sd t0, 96(%[obs])\n\t"
        "lla t1, 2b\n\t"
        "lla t2, 1b\n\t"
        "sub t1, t1, t2\n\t"
        "sd t1, 104(%[obs])\n\t"

        ".option pop\n\t"
        :
        : [obs] "r"(obs)
        : "ra", "a5", "t0", "t1", "t2", "t6", "memory");
}

static AI_RVC_ATTR void ai_arch_rvc_integer_probe(uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"

        ".option rvc\n\t"
        "c.li a5, -1\n\t"
        ".option norvc\n\t"
        "sd a5, 160(%[obs])\n\t"

        ".option rvc\n\t"
        "c.lui a5, 1\n\t"
        ".option norvc\n\t"
        "sd a5, 168(%[obs])\n\t"

        "li a5, 10\n\t"
        ".option rvc\n\t"
        "c.addi a5, -3\n\t"
        ".option norvc\n\t"
        "sd a5, 176(%[obs])\n\t"

        "li a5, 0x100000000\n\t"
        ".option rvc\n\t"
        "c.addiw a5, -1\n\t"
        ".option norvc\n\t"
        "sd a5, 184(%[obs])\n\t"

        "mv t6, sp\n\t"
        ".option rvc\n\t"
        "c.addi16sp sp, 16\n\t"
        ".option norvc\n\t"
        "sub a5, sp, t6\n\t"
        "mv sp, t6\n\t"
        "sd a5, 192(%[obs])\n\t"

        "mv t6, sp\n\t"
        ".option rvc\n\t"
        "c.addi4spn a5, sp, 64\n\t"
        ".option norvc\n\t"
        "sub a5, a5, t6\n\t"
        "sd a5, 200(%[obs])\n\t"

        "li a5, 3\n\t"
        ".option rvc\n\t"
        "c.slli a5, 3\n\t"
        ".option norvc\n\t"
        "sd a5, 208(%[obs])\n\t"

        "li a5, 0xf0\n\t"
        ".option rvc\n\t"
        "c.srli a5, 2\n\t"
        ".option norvc\n\t"
        "sd a5, 216(%[obs])\n\t"

        "li a5, -16\n\t"
        ".option rvc\n\t"
        "c.srai a5, 2\n\t"
        ".option norvc\n\t"
        "sd a5, 224(%[obs])\n\t"

        "li a5, 0xf3\n\t"
        ".option rvc\n\t"
        "c.andi a5, -16\n\t"
        ".option norvc\n\t"
        "sd a5, 232(%[obs])\n\t"

        "li a4, 0x123456789abcdef0\n\t"
        "li a5, 0\n\t"
        ".option rvc\n\t"
        "c.mv a5, a4\n\t"
        ".option norvc\n\t"
        "sd a5, 240(%[obs])\n\t"

        "li a5, 5\n\t"
        "li a4, 7\n\t"
        ".option rvc\n\t"
        "c.add a5, a4\n\t"
        ".option norvc\n\t"
        "sd a5, 248(%[obs])\n\t"

        "li a5, 20\n\t"
        "li a4, 3\n\t"
        ".option rvc\n\t"
        "c.sub a5, a4\n\t"
        ".option norvc\n\t"
        "sd a5, 256(%[obs])\n\t"

        "li a5, 0xaa\n\t"
        "li a4, 0x0f\n\t"
        ".option rvc\n\t"
        "c.xor a5, a4\n\t"
        ".option norvc\n\t"
        "sd a5, 264(%[obs])\n\t"

        "li a5, 0xa0\n\t"
        "li a4, 0x0f\n\t"
        ".option rvc\n\t"
        "c.or a5, a4\n\t"
        ".option norvc\n\t"
        "sd a5, 272(%[obs])\n\t"

        "li a5, 0xaf\n\t"
        "li a4, 0xf0\n\t"
        ".option rvc\n\t"
        "c.and a5, a4\n\t"
        ".option norvc\n\t"
        "sd a5, 280(%[obs])\n\t"

        "li a5, 1\n\t"
        "li a4, 2\n\t"
        ".option rvc\n\t"
        "c.subw a5, a4\n\t"
        ".option norvc\n\t"
        "sd a5, 288(%[obs])\n\t"

        "li a5, 0x7fffffff\n\t"
        "li a4, 1\n\t"
        ".option rvc\n\t"
        "c.addw a5, a4\n\t"
        ".option norvc\n\t"
        "sd a5, 296(%[obs])\n\t"

        "li a5, 0x1234\n\t"
        ".option rvc\n\t"
        ".2byte 0x0001\n\t"
        "c.li x0, 1\n\t"
        "c.addi x0, 1\n\t"
        "c.addi a5, 0\n\t"
        ".2byte 0x6005\n\t"
        "c.slli x0, 1\n\t"
        ".2byte 0x0782\n\t"
        "c.mv x0, a5\n\t"
        "c.add x0, a5\n\t"
        ".option norvc\n\t"
        "sd zero, 304(%[obs])\n\t"
        "sd a5, 312(%[obs])\n\t"

        "li a5, 0x55\n\t"
        ".option rvc\n\t"
        ".2byte 0x8381\n\t"
        ".option norvc\n\t"
        "sd a5, 320(%[obs])\n\t"

        "li a5, -32\n\t"
        ".option rvc\n\t"
        ".2byte 0x8781\n\t"
        ".option norvc\n\t"
        "sd a5, 328(%[obs])\n\t"

        ".option pop\n\t"
        :
        : [obs] "r"(obs)
        : "a4", "a5", "t6", "memory");
}

static uintptr_t ai_arch_rvc_fire_ebreak(void)
{
    uintptr_t pc;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lla %[pc], 1f\n\t"
        "1:\n\t"
        ".option rvc\n\t"
        "c.ebreak\n\t"
        ".option pop\n\t"
        : [pc] "=r"(pc)
        :
        : "memory");

    return pc;
}

bool ai_arch_rvc_control_integer_expansion_visible_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    uint64_t *obs = ai_arch_rvc_nonmem_obs;
    for (unsigned i = 0; i < 64U; ++i) {
        obs[i] = 0xfeedface00000000ULL | i;
    }

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_control_transfer_probe(obs);
    TEST_ASSERT("compressed C.J/C.JR/C.JALR and C.BEQZ/C.BNEZ control transfers retire as 16-bit instructions and route to the architecturally selected targets",
        excpt.triggered == false &&
        obs[0] == 0x11ULL &&
        obs[1] == 2ULL &&
        obs[2] == 0x22ULL &&
        obs[3] == 2ULL &&
        obs[4] == 2ULL &&
        obs[5] == 2ULL &&
        obs[6] == 0x31ULL &&
        obs[7] == 2ULL &&
        obs[8] == 0x32ULL &&
        obs[9] == 2ULL &&
        obs[10] == 0x33ULL &&
        obs[11] == 2ULL &&
        obs[12] == 0x34ULL &&
        obs[13] == 2ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_integer_probe(obs);
    TEST_ASSERT("compressed integer computation instructions produce the same architectural values as their expanded ALU forms",
        excpt.triggered == false &&
        obs[20] == UINT64_MAX &&
        obs[21] == 0x1000ULL &&
        obs[22] == 7ULL &&
        obs[23] == UINT64_MAX &&
        obs[24] == 16ULL &&
        obs[25] == 64ULL &&
        obs[26] == 24ULL &&
        obs[27] == 0x3cULL &&
        obs[28] == UINT64_MAX - 3ULL &&
        obs[29] == 0xf0ULL &&
        obs[30] == 0x123456789abcdef0ULL &&
        obs[31] == 12ULL &&
        obs[32] == 17ULL &&
        obs[33] == 0xa5ULL &&
        obs[34] == 0xafULL &&
        obs[35] == 0xa0ULL &&
        obs[36] == UINT64_MAX &&
        obs[37] == 0xffffffff80000000ULL
    );

    TEST_ASSERT("compressed HINT forms retire without changing x0 or their destination register payloads",
        obs[38] == 0ULL &&
        obs[39] == 0x1234ULL &&
        obs[40] == 0x55ULL &&
        obs[41] == UINT64_MAX - 31ULL
    );

    uintptr_t pc = 0;

    TEST_SETUP_EXCEPT();
    AI_RVC_EXEC_HALF_CAPTURE(0x0000, pc);
    TEST_ASSERT("all-zero compressed parcel, also C.ADDI4SPN with nzuimm=0, raises illegal instruction at the 16-bit parcel",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_ILI &&
        excpt.epc == pc
    );

    TEST_SETUP_EXCEPT();
    AI_RVC_EXEC_WORD_CAPTURE(0xffffffff, pc);
    TEST_ASSERT("all-one instruction parcel stream raises illegal instruction at its first parcel",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_ILI &&
        excpt.epc == pc
    );

    TEST_SETUP_EXCEPT();
    AI_RVC_EXEC_HALF_CAPTURE(0x001c, pc);
    TEST_ASSERT("C.ADDI4SPN with nzuimm=0 remains reserved even when the compressed destination field is nonzero",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_ILI &&
        excpt.epc == pc
    );

    TEST_SETUP_EXCEPT();
    AI_RVC_EXEC_HALF_CAPTURE(0x8002, pc);
    TEST_ASSERT("reserved C.JR rs1=x0 encoding raises illegal instruction at the compressed halfword",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_ILI &&
        excpt.epc == pc
    );

    TEST_SETUP_EXCEPT();
    AI_RVC_EXEC_HALF_CAPTURE(0x2005, pc);
    TEST_ASSERT("reserved RV64 C.ADDIW rd=x0 encoding raises illegal instruction at the compressed halfword",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_ILI &&
        excpt.epc == pc
    );

    TEST_SETUP_EXCEPT();
    AI_RVC_EXEC_HALF_CAPTURE(0x9ff9, pc);
    TEST_ASSERT("RV64 CA-format funct6=111 funct2=11 field combination remains reserved",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_ILI &&
        excpt.epc == pc
    );

    TEST_SETUP_EXCEPT();
    pc = ai_arch_rvc_fire_ebreak();
    TEST_ASSERT("C.EBREAK raises breakpoint at the compressed instruction PC",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == pc &&
        excpt.tval == pc
    );

    TEST_END();
}
