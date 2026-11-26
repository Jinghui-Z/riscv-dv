#include "backend_it_0911.h"

/* Keep this timing-sensitive standalone case out of other cases' constant pools. */
static volatile uint64_t pressure_mem[512][8] __attribute__((aligned(4096)));
static uint8_t pressure_vdump[4096] __attribute__((aligned(64)));

static bool cancel_dual_source_pressure(unsigned row)
{
    volatile uint64_t *p = pressure_mem[256];
    for (unsigned i = 0; i < 64; ++i) {
        p[16 * i] = 101;
        p[16 * i + 8] = 307;
        p[16 * i + 1] = 0x1122334455667788ULL;
    }
    bool ok = true;
    for (unsigned trial = 0; trial < 16; ++trial) {
        for (unsigned i = 0; i < 128; ++i)
            asm volatile("cbo.flush 0(%0)" :: "r"(p + 8 * i) : "memory");
        asm volatile("fence rw,rw" ::: "memory");
        uint64_t total, last;
#define CANCEL_DUAL_RUN(gap, producers) asm volatile(BE_ASM_BEGIN \
            "mv s6,%[p]\nli s2,7\nli s3,11\nli s4,0\nli s7,0\nli s8,128\n" \
            "vsetvli zero,s8,e8,m8,tu,mu\n" BE_DELAY \
            ".rept 64\nvle8.v v8,(s6)\n" producers \
            "sub t5,t1,t4\nadd s4,s4,t5\naddi s6,s6,128\n.endr\n" \
            "mv %[total],s4\nmv %[last],t5\n" BE_ASM_END \
            : [total] "=&r"(total), [last] "=&r"(last) : [p] "r"(p) \
            : "t0","t1","t2","t3","t4","t5","t6", \
              "s2","s3","s4","s6","s7","s8","memory")
#define CANCEL_FAST(gap) CANCEL_DUAL_RUN(gap, \
            "ld t4,0(s6)\n.rept " #gap "\naddi s7,s7,1\n.endr\n" \
            "add t0,s2,s3\nadd t1,s2,s3\nadd t2,s2,s3\nadd t3,s2,s3\n" \
            "mul t0,s2,s3\njal t6,1f\n1:\n")
#define CANCEL_LOAD(gap) CANCEL_DUAL_RUN(gap, \
            "ld t4,0(s6)\n.rept " #gap "\naddi s7,s7,1\n.endr\n" \
            "ld t1,64(s6)\nld t2,0(s6)\nld t3,64(s6)\n")
#define CANCEL_GAP(gap) case gap: \
            if (row == 648) { CANCEL_FAST(gap); } else { CANCEL_LOAD(gap); } break
        switch (trial % 8) {
        CANCEL_GAP(0); CANCEL_GAP(1); CANCEL_GAP(2); CANCEL_GAP(3);
        CANCEL_GAP(4); CANCEL_GAP(5); CANCEL_GAP(6); CANCEL_GAP(7);
        }
#undef CANCEL_GAP
#undef CANCEL_LOAD
#undef CANCEL_FAST
#undef CANCEL_DUAL_RUN
        uint64_t expected = row == 648 ? (uint64_t)(18 - 101) : 307 - 101;
        ok &= total == 64 * expected && last == expected;
        for (unsigned i = 0; i < 64; ++i)
            ok &= p[16 * i] == 101 && p[16 * i + 8] == 307 &&
                p[16 * i + 1] == 0x1122334455667788ULL;
    }
    return ok;
}

static bool cancel_fast_load_pressure(void)
{
    volatile uint64_t *p = pressure_mem[256];
    for (unsigned i = 0; i < 64; ++i) {
        p[16 * i] = 101;
        p[16 * i + 1] = 0x1122334455667788ULL;
    }
    bool ok = true;
    for (unsigned trial = 0; trial < 32; ++trial) {
        for (unsigned i = 0; i < 64; ++i)
            asm volatile("cbo.flush 0(%0)" :: "r"(p + 16 * i) : "memory");
        asm volatile("fence rw,rw" ::: "memory");
        uint64_t sum, mixed;
#define CANCEL_FAST_LOAD(gap) asm volatile(BE_ASM_BEGIN \
            "mv s6,%[p]\nli s2,7\nli s3,11\nli s4,0\nli s5,0\nli s7,0\n" BE_DELAY \
            ".set cancel_fast_frame,0\n.rept 64\nld t4,0(s6)\n" \
            ".if (" #gap " == 5) && (cancel_fast_frame == 10)\n" \
            "addi t1,s7,5\naddi s7,t1,-3\n.rept 3\naddi s7,s7,1\n.endr\n" \
            "add t0,s2,s3\nadd t2,s2,s3\nadd t2,s2,s3\nadd t3,s2,s3\n" \
            "jal t6,1f\n1: add s4,s4,t0\nsub t5,t1,t4\nadd s5,s5,t5\n.else\n" \
            ".rept " #gap "\naddi s7,s7,1\n.endr\n" \
            "add t0,s2,s3\nadd t1,s2,s3\nadd t2,s2,s3\nadd t3,s2,s3\n" \
            "jal t6,1f\n1: add s4,s4,t0\nsub t5,s7,t4\nadd s5,s5,t5\n" \
            ".endif\naddi s6,s6,128\n.set cancel_fast_frame,cancel_fast_frame+1\n" \
            ".endr\nmv %[sum],s4\nmv %[mixed],s5\n" BE_ASM_END \
            : [sum] "=&r"(sum), [mixed] "=&r"(mixed) : [p] "r"(p) \
            : "t0","t1","t2","t3","t4","t5","t6", \
              "s2","s3","s4","s5","s6","s7","memory")
#define CANCEL_FAST_GAP(gap) case gap: CANCEL_FAST_LOAD(gap); break
        switch (trial % 8) {
        case 0: CANCEL_FAST_LOAD(8); break;
        CANCEL_FAST_GAP(1); CANCEL_FAST_GAP(2); CANCEL_FAST_GAP(3); CANCEL_FAST_GAP(4);
        CANCEL_FAST_GAP(5); CANCEL_FAST_GAP(6); CANCEL_FAST_GAP(7);
        }
#undef CANCEL_FAST_GAP
#undef CANCEL_FAST_LOAD
        unsigned gap = trial % 8 ? trial % 8 : 8;
        ok &= sum == 64 * 18 && mixed == (uint64_t)gap * 64 * 65 / 2 - 64 * 101;
        for (unsigned i = 0; i < 64; ++i)
            ok &= p[16 * i] == 101 && p[16 * i + 1] == 0x1122334455667788ULL;
    }
    return ok;
}

bool ai_micro_backend_it_cancel_reissue(void)
{
    TEST_START();
    be_it_enable();
    for (unsigned row = 642; row <= 649; ++row) {
        pressure_mem[0][0] = 101;
        pressure_mem[1][0] = 307;
        pressure_mem[2][0] = 0x11223344;
        backend_it_control.target_addr = (uintptr_t)pressure_mem;
        if (!be_it_begin(row,0)) return false;
        TEST_SETUP_EXCEPT();
        uint64_t got;
        asm volatile(BE_ASM_BEGIN
            "ld t0,0(%[p])\nld t1,64(%[p])\naddi t2,t0,13\n"
            "mul t3,t1,t1\nsub %[out],t3,t2\nsd %[out],192(%[p])\n"
            "ld t4,128(%[p])\nxor t4,t4,t1\nsd t4,256(%[p])\n" BE_ASM_END
            : [out] "=&r"(got) : [p] "r"(pressure_mem)
            : "t0","t1","t2","t3","t4","memory");
        TEST_ASSERT("cancelled producers reissue and both consumers see valid data",
            !excpt.triggered && got == 307*307-114 && pressure_mem[3][0] == got &&
            pressure_mem[4][0] == (0x11223344 ^ 307));
        bool stress_ok = true;
        /* Repeat the first window after its instruction and predictor warmup. */
        for (unsigned replay = 0; replay < (row == 642 ? 3u : 1u); ++replay) {
            for (unsigned burst = 0; burst < 4; ++burst) {
                uint64_t sum, value;
                asm volatile(BE_ASM_BEGIN
                    "li s2,7\nli s3,11\nli s4,0\nli s5,0\n"
                    BE_DELAY
                    ".rept 128\n"
                    "add t0,s2,s3\nadd t1,s2,s3\nadd t2,s2,s3\nadd t3,s2,s3\n"
                    "jal t6,1f\n1: add s4,s4,t0\n"
                    "add t0,%[p],zero\nld t4,0(t0)\nadd t5,t1,t4\n"
                    "add s5,s5,t5\n.endr\nmv %[sum],s4\nmv %[value],s5\n" BE_ASM_END
                    : [sum] "=&r"(sum), [value] "=&r"(value) : [p] "r"(pressure_mem)
                    : "t0","t1","t2","t3","t4","t5","t6","s2","s3","s4","s5","memory");
                stress_ok &= sum == 128 * 18 && value == 128 * 119;
            }
            for (unsigned trial = 0; trial < 16; ++trial) {
                volatile uint64_t *p = pressure_mem[16 + trial * 8];
                p[0] = 101;
                p[8] = 307;
                p[16] = (uintptr_t)p;
                uint64_t total, mixed;
                asm volatile("cbo.flush 0(%0)\ncbo.flush 0(%1)\ncbo.flush 0(%2)\nfence rw,rw\n"
                    :: "r"(p), "r"(p+8), "r"(p+16) : "memory");
                asm volatile(BE_ASM_BEGIN
                    "li s2,7\nli s3,11\nli s4,0\nli s5,128\n"
                    "vsetvli zero,s5,e8,m8,tu,mu\n"
                    "vle8.v v8,(%[p])\n"
                    ".rept 32\n"
                    "ld t0,128(%[p])\nld t1,0(t0)\n"
                    "mul t2,s2,s3\naddi t3,t1,13\nadd t4,t2,t3\n"
                    "add s4,s4,t4\nld t5,64(%[p])\nmul t6,t5,t5\nsub %[mixed],t6,t3\n"
                    ".endr\nmv %[sum],s4\nvse8.v v8,(%[v])\n" BE_ASM_END
                    : [sum] "=&r"(total), [mixed] "=&r"(mixed)
                    : [p] "r"(p), [v] "r"(pressure_vdump)
                    : "t0","t1","t2","t3","t4","t5","t6","s2","s3","s4","s5","memory");
                stress_ok &= total == 32 * 191 && mixed == 307*307-114 &&
                    p[0] == 101 && p[8] == 307 && p[16] == (uintptr_t)p;
                for (unsigned lane = 0; lane < 16; ++lane)
                    stress_ok &= ((uint64_t *)pressure_vdump)[lane] == p[lane];
                uint64_t pair;
                asm volatile(BE_ASM_BEGIN
                    "li s4,0\nli s5,128\nvsetvli zero,s5,e8,m8,tu,mu\n"
                    ".rept 32\nvle8.v v8,(%[p])\n"
                    "ld t0,0(%[p])\nld t1,64(%[p])\n"
                    "ld t2,0(%[p])\nld t3,64(%[p])\n"
                    "add t4,t0,t1\nadd t5,t2,t3\nadd s4,s4,t4\nadd s4,s4,t5\n"
                    ".endr\nmv %[pair],s4\nvse8.v v8,(%[v])\n" BE_ASM_END
                    : [pair] "=&r"(pair) : [p] "r"(p), [v] "r"(pressure_vdump)
                    : "t0","t1","t2","t3","t4","t5","s4","s5","memory");
                stress_ok &= pair == 64 * 408;
            }
        }
        if (row == 648) stress_ok &= cancel_fast_load_pressure();
        if (row == 649) stress_ok &= cancel_dual_source_pressure(row);
        TEST_ASSERT("mixed producers and dependent loads preserve exact results", stress_ok && !excpt.triggered);
        TEST_ASSERT("IT checked OG0/OG1/LDU identity, cancellation and exact reissue", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_cancel_reissue");
}
