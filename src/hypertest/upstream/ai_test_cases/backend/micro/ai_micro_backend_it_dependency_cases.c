#include "backend_it_0911.h"

static uint64_t dep_mem[64] __attribute__((aligned(64)));
static uint8_t dep_vector[4096] __attribute__((aligned(64)));

/* RVC is explicit even though the surrounding stream uses 32-bit encodings. */
#define DEP_C(insn) ".option rvc\n" insn "\n.option norvc\n"
#define DEP_RUN(body) asm volatile(BE_ASM_BEGIN body BE_ASM_END \
    : "=m"(dep_mem) : [p] "r"(dep_mem), [v] "r"(dep_vector) \
    : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "a4", "a5", \
      "f0", "f1", "f2", "f3", "f4", "f5", "memory")

static bool dep_rvc(unsigned row)
{
    dep_mem[0] = 101;
    dep_mem[1] = 211;
    dep_mem[2] = dep_mem[3] = 0;
    dep_mem[4] = 0xabcddcba12344321ULL;
    switch (row) {
    case 575:
        DEP_RUN("lui a4,0x12345\naddi a4,a4,0x678\naddi a5,a4,17\nsd a5,16(%[p])\n");
        return dep_mem[2] == 0x12345689;
    case 576:
        DEP_RUN("li a4,91\n" DEP_C("c.li a4,7\nc.addi a4,5") "sd a4,16(%[p])\n");
        return dep_mem[2] == 12;
    case 577:
        DEP_RUN("li a4,91\n" DEP_C("c.li a4,7") "addi a5,a4,5\nsd a5,16(%[p])\n"
            DEP_C("c.li a4,9") "mul a5,a4,a4\nsd a5,24(%[p])\nli t0,2\n"
            DEP_C("c.li a4,14") "div a5,a4,t0\nsd a5,40(%[p])\n");
        return dep_mem[2] == 12 && dep_mem[3] == 81 && dep_mem[5] == 7;
    case 578:
        DEP_RUN("li a4,1\n" DEP_C("c.li a4,0") "bnez a4,9f\nlla a4,8f\n"
            DEP_C("c.addi a4,4") "jalr t0,0(a4)\n8: j 9f\nli a5,37\nsd a5,16(%[p])\nj 7f\n9: sd zero,16(%[p])\n7:\n");
        return dep_mem[2] == 37;
    case 579:
        DEP_RUN("mv a4,%[p]\n" DEP_C("c.addi a4,8") "ld a5,0(a4)\nsd a5,16(%[p])\n"
            DEP_C("c.li a5,13") "sd a5,0(a4)\namoadd.d t0,a5,(a4)\nsd t0,24(%[p])\n");
        return dep_mem[0] == 101 && dep_mem[1] == 26 && dep_mem[2] == 211 && dep_mem[3] == 13;
    case 580:
        DEP_RUN("csrr t0,mscratch\nli a4,91\n" DEP_C("c.li a4,19")
            "csrw mscratch,a4\ncsrr a5,mscratch\nsd a5,16(%[p])\ncsrw mscratch,t0\n");
        return dep_mem[2] == 19;
    case 581:
        DEP_RUN("li a4,91\n" DEP_C("c.li a4,23") "fcvt.d.l f0,a4\nfcvt.l.d a5,f0,rtz\nsd a5,16(%[p])\n");
        return dep_mem[2] == 23;
    case 582:
        DEP_RUN("vsetivli zero,4,e32,m1,tu,mu\nli a4,91\n" DEP_C("c.li a4,29")
            "vmv.v.x v8,a4\nvmv.x.s a5,v8\nsd a5,16(%[p])\n");
        return dep_mem[2] == 29;
    case 583:
        DEP_RUN("li a5,7\naddi a4,a5,5\n" DEP_C("c.addi a4,1")
            "sd a4,16(%[p])\nmul a4,a5,a5\n" DEP_C("c.addi a4,1")
            "sd a4,24(%[p])\ndiv a4,a4,a5\n" DEP_C("c.addi a4,1") "sd a4,40(%[p])\n");
        return dep_mem[2] == 13 && dep_mem[3] == 50 && dep_mem[5] == 8;
    case 584:
        DEP_RUN("ld a4,0(%[p])\n" DEP_C("c.addi a4,1") "sd a4,16(%[p])\n"
            "li a5,7\namoadd.d a4,a5,(%[p])\n" DEP_C("c.addi a4,2") "sd a4,24(%[p])\n");
        return dep_mem[0] == 108 && dep_mem[2] == 102 && dep_mem[3] == 103;
    case 585:
        DEP_RUN("csrr t0,mscratch\nli a5,31\ncsrw mscratch,a5\ncsrr a4,mscratch\n"
            DEP_C("c.addi a4,1") "sd a4,16(%[p])\ncsrw mscratch,t0\n");
        return dep_mem[2] == 32;
    case 586:
        DEP_RUN("li a5,37\nfcvt.d.l f0,a5\nfcvt.l.d a4,f0,rtz\n"
            DEP_C("c.addi a4,1") "sd a4,16(%[p])\n");
        return dep_mem[2] == 38;
    case 587:
        DEP_RUN("vsetivli zero,4,e32,m1,tu,mu\nvmv.v.i v8,11\nvmv.x.s a4,v8\n"
            DEP_C("c.addi a4,1") "sd a4,16(%[p])\n");
        return dep_mem[2] == 12;
    case 588:
        DEP_RUN("jal a4,1f\n0: j 9f\n1:\n" DEP_C("c.mv a5,a4")
            "lla t0,0b\nsub a5,a5,t0\nsd a5,16(%[p])\nlla t0,2f\njalr a4,0(t0)\n"
            "3: j 9f\n2:\n" DEP_C("c.mv a5,a4") "lla t0,3b\nsub a5,a5,t0\nsd a5,24(%[p])\n9:\n");
        return dep_mem[2] == 0 && dep_mem[3] == 0;
    }
    return false;
}

bool ai_micro_backend_it_rvc_producer_matrix(void)
{
    TEST_START();
    be_it_enable();
    for (unsigned row = 575; row <= 582; ++row) {
        if (!be_it_begin(row, 0)) return false;
        TEST_SETUP_EXCEPT();
        bool ok = dep_rvc(row);
        TEST_ASSERT("RVC/fusion producer supplies the new value and preserves guards",
            ok && !excpt.triggered && dep_mem[4] == 0xabcddcba12344321ULL, "row=%u", row);
        TEST_ASSERT("IT observed the specified decode/rename boundary", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_rvc_producer_matrix");
}

bool ai_micro_backend_it_rvc_consumer_matrix(void)
{
    TEST_START();
    be_it_enable();
    for (unsigned row = 583; row <= 588; ++row) {
        if (!be_it_begin(row, 0)) return false;
        TEST_SETUP_EXCEPT();
        bool ok = dep_rvc(row);
        TEST_ASSERT("RVC consumer uses the producing domain's result", ok && !excpt.triggered, "row=%u", row);
        TEST_ASSERT("IT observed the required producer completion and rename relation", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_rvc_consumer_matrix");
}

bool ai_micro_backend_it_vector_raw_waw(void)
{
    TEST_START();
    be_it_enable();
    uint64_t bytes = CSRR(CSR_VLENB);
    TEST_ASSERT("vector dump fits", bytes <= 256);
    for (unsigned row = 589; row <= 591; ++row) {
        for (unsigned i = 0; i < sizeof(dep_vector); ++i) dep_vector[i] = 0xa5;
        if (!be_it_begin(row, 0)) return false;
        TEST_SETUP_EXCEPT();
        if (row == 589) {
            DEP_RUN("vsetvli t0,zero,e8,m4,tu,mu\nvmv.v.i v8,3\nvadd.vi v12,v8,7\n"
                "vse8.v v12,(%[v])\n");
        } else if (row == 590) {
            DEP_RUN("vsetvli t0,zero,e8,m4,tu,mu\nvmv.v.i v8,3\nvmv.v.i v16,2\n"
                "vdivu.vv v8,v8,v16\nvmv.v.i v8,11\nvse8.v v8,(%[v])\n");
        } else {
            DEP_RUN("vsetvli t0,zero,e8,m2,tu,mu\nvmv.v.i v16,3\nvwaddu.vv v8,v16,v16\n"
                "vmv.v.i v10,13\nvs4r.v v8,(%[v])\n");
        }
        bool ok = !excpt.triggered;
        for (unsigned i = 0; i < 4 * bytes; ++i) {
            unsigned want = row == 589 ? 10 : row == 590 ? 11 :
                i >= 2 * bytes ? 13 : (i & 1) ? 0 : 6;
            ok &= dep_vector[i] == want;
        }
        TEST_ASSERT("RAW result and each overlapping/nonoverlapping register lane are exact", ok);
        TEST_ASSERT("dump does not overwrite its guard", dep_vector[4 * bytes] == 0xa5);
        TEST_ASSERT("IT observed distinct ROB generations and the required uop overlap", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_vector_raw_waw");
}

bool ai_micro_backend_it_rename_cross_domain(void)
{
    TEST_START();
    be_it_enable();
    for (unsigned row = 602; row <= 613; ++row) {
        dep_mem[2] = 0;
        if (!be_it_begin(row, 0)) return false;
        TEST_SETUP_EXCEPT();
        switch (row) {
        case 602: DEP_RUN("li t0,19\naddi t0,t0,3\naddi t1,t0,7\nsd t1,16(%[p])\n"); break;
        case 603: DEP_RUN("li t0,11\nfcvt.d.l f0,t0\nfadd.d f5,f0,f0\nfadd.d f1,f5,f0\nfcvt.l.d t1,f1,rtz\nsd t1,16(%[p])\n"); break;
        case 604: DEP_RUN("vsetivli zero,4,e32,m1,tu,mu\nvmv.v.i v5,11\nvadd.vi v6,v5,7\nvmv.x.s t1,v6\nsd t1,16(%[p])\n"); break;
        case 605: DEP_RUN("li t1,-1\nli t2,3\ndivu t0,t1,t2\naddi t0,t2,29\nsd t0,16(%[p])\n"); break;
        case 606: DEP_RUN("li t1,41\nmv t0,t1\naddi t2,t0,7\nsd t2,16(%[p])\n"); break;
        case 607: DEP_RUN("li t1,41\nadd zero,t1,t1\nsub t2,zero,t1\nadd t3,t1,zero\nadd t2,t2,t3\nsd t2,16(%[p])\n"); break;
        case 608: DEP_RUN("li t0,11\naddi t0,t0,7\nfcvt.d.l f0,t0\nfcvt.l.d t1,f0,rtz\nsd t1,16(%[p])\n"); break;
        case 609: DEP_RUN("li t0,11\nfcvt.d.l f0,t0\nfadd.d f5,f0,f0\nfcvt.l.d t1,f5,rtz\nsd t1,16(%[p])\n"); break;
        case 610: DEP_RUN("vsetivli zero,4,e32,m1,tu,mu\nli t0,11\naddi t0,t0,7\nvmv.v.x v8,t0\nvmv.x.s t1,v8\nsd t1,16(%[p])\n"); break;
        case 611: DEP_RUN("vsetivli zero,4,e32,m1,tu,mu\nvmv.v.i v8,11\nvmv.x.s t0,v8\naddi t1,t0,7\nsd t1,16(%[p])\n"); break;
        case 612: DEP_RUN("vsetivli zero,2,e64,m1,tu,mu\nli t0,11\nfcvt.d.l f0,t0\nfadd.d f5,f0,f0\nvfmv.v.f v8,f5\nvfmv.f.s f1,v8\nfcvt.l.d t1,f1,rtz\nsd t1,16(%[p])\n"); break;
        case 613: DEP_RUN("vsetivli zero,2,e64,m1,tu,mu\nli t0,11\nfcvt.d.l f0,t0\nvfmv.v.f v8,f0\nvfmv.f.s f5,v8\nfadd.d f1,f5,f0\nfcvt.l.d t1,f1,rtz\nsd t1,16(%[p])\n"); break;
        }
        const unsigned expected[] = {29,33,18,32,48,0,18,22,18,18,22,22};
        TEST_ASSERT("rename RAW/WAW/move/x0 produces the exact new value",
            !excpt.triggered && dep_mem[2] == expected[row - 602] && CSRR(CSR_FFLAGS) == 0, "row=%u got=%lu", row, dep_mem[2]);
        TEST_ASSERT("IT observed same/adjacent rename groups as specified", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_rename_cross_domain");
}

/* Runtime emission gives each source its own producer distance. The TB must
 * check actual dataSources; an instruction distance does not prove bypass. */
static uint32_t dep_code[256] __attribute__((aligned(64)));
static uint32_t dep_addi(unsigned rd, unsigned rs, int imm)
{
    return ((uint32_t)imm & 4095) << 20 | rs << 15 | rd << 7 | 0x13;
}

static unsigned dep_integer_distance(unsigned distance, unsigned sweep)
{
    if (sweep < 8) return distance + sweep;
    /* Vary the old source independently of the recent source. */
    return distance + (distance == 40 ? ((sweep - 8) / 16) * 8 : (sweep - 8) % 16);
}

bool ai_micro_backend_it_integer_source_matrix(void)
{
    TEST_START();
    be_it_enable();
    const uint8_t distances[][2] = {{40,40},{5,40},{40,5},{0,40},{40,0},
        {2,40},{40,2},{5,0},{0,2},{40,40},{40,40},{40,40},{5,5}};
    for (unsigned row = 651; row <= 663; ++row) {
        for (unsigned sweep = 0; sweep < 40; ++sweep) {
            unsigned n = 0;
            unsigned distance0 = dep_integer_distance(distances[row - 651][0], sweep);
            unsigned distance1 = dep_integer_distance(distances[row - 651][1], sweep);
            for (unsigned pos = 0; pos <= 64; ++pos) {
                if (pos == 64 - distance0)
                    dep_code[n++] = dep_addi(5, 10, 7);
                if (pos == 64 - distance1)
                    dep_code[n++] = dep_addi(6, 11, 13);
                dep_code[n++] = dep_addi(7, 7, 1);
            }
            uint64_t a = 0x1234567890ULL, b = 0x23456789ULL;
            uint64_t expected = (a + 7) - (b + 13);
            uintptr_t consumer_pc = (uintptr_t)&dep_code[n];
            if (row == 660) { dep_code[n++] = dep_addi(10,5,-19); expected = a + 7 - 19; }
            else {
                unsigned rs1 = row == 661 ? 0 : 5;
                unsigned rs2 = row == 662 ? 0 : 6;
                dep_code[n++] = 0x40000033 | rs2 << 20 | rs1 << 15 | 10 << 7;
                if (row == 661) expected = 0 - (b + 13);
                if (row == 662) expected = a + 7;
            }
            dep_code[n++] = 0x00008067;
            asm volatile("fence.i" ::: "memory");
            if (sweep == 0) {
                backend_it_control.target_pc = consumer_pc;
                if (!be_it_begin(row, 0)) return false;
            }
            TEST_SETUP_EXCEPT();
            uint64_t got = ((uint64_t (*)(uint64_t,uint64_t))dep_code)(a,b);
            TEST_ASSERT("integer source order is checked with noncommutative subtraction",
                !excpt.triggered && got == expected, "row=%u sweep=%u", row, sweep);
        }
        TEST_ASSERT("IT observed the target consumer source paths during the distance sweep", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_integer_source_matrix");
}

bool ai_micro_backend_it_fma_source_matrix(void)
{
    TEST_START();
    be_it_enable();
    const uint8_t distances[][3] = {{40,40,40},{0,40,40},{40,0,40},{40,40,0},
        {2,40,40},{40,2,40},{40,40,2},{0,2,40},{0,2,4}};
    for (unsigned row = 664; row <= 672; ++row) {
        for (unsigned sweep = 0; sweep < 24; ++sweep) {
            unsigned n = 0;
            /* fa0/fa1/fa2 inputs are raw double bits loaded by integer ABI. */
            dep_code[n++] = 0xf2050053U | 10 << 7;
            dep_code[n++] = 0xf2058053U | 11 << 7;
            dep_code[n++] = 0xf2060053U | 12 << 7;
            for (unsigned pos = 0; pos <= 64; ++pos) {
                for (unsigned src = 0; src < 3; ++src) {
                    /* FPToVec decode reverses rs1/rs2 before the IQ. */
                    unsigned iq_src = src < 2 ? 1 - src : src;
                    if (pos == 64 - distances[row-664][iq_src] - sweep)
                        /* fsgnj.d f[0..2],fa[0..2],fa[0..2] */
                        dep_code[n++] = 0x22000053 | (10+src) << 20 | (10+src) << 15 | src << 7;
                }
                dep_code[n++] = dep_addi(7,7,1);
            }
            /* fmsub.d f3,f0,f1,f2: 3*5-7=8, checks addend position and sign. */
            uintptr_t consumer_pc = (uintptr_t)&dep_code[n];
            dep_code[n++] = 2U << 27 | 1U << 25 | 1U << 20 | 3U << 7 | 0x47;
            dep_code[n++] = 0xe2018553U; /* fmv.x.d a0,f3 */
            dep_code[n++] = 0x00008067;
            asm volatile("fence.i" ::: "memory");
            if (sweep == 0) {
                backend_it_control.target_pc = consumer_pc;
                if (!be_it_begin(row,0)) return false;
            }
            TEST_SETUP_EXCEPT();
            uint64_t got = ((uint64_t (*)(uint64_t,uint64_t,uint64_t))dep_code)(
                0x4008000000000000ULL,0x4014000000000000ULL,0x401c000000000000ULL);
            TEST_ASSERT("FMA source routing preserves the addend and exception flags",
                !excpt.triggered && got == 0x4020000000000000ULL && CSRR(CSR_FFLAGS) == 0,
                "row=%u got=%lx", row, got);
        }
        TEST_ASSERT("IT observed all three target consumer source paths during the distance sweep", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_fma_source_matrix");
}
