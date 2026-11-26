#include "backend_it_0911.h"

static volatile uint64_t pressure_mem[512][8] __attribute__((aligned(4096)));
static uint8_t pressure_vdump[4096] __attribute__((aligned(64)));

/* kind: integer, VF, V0, VL, load, store, mixed. An old DIV keeps younger
 * allocations live. A responder may hold the advertised load to extend it.
 * Queue fullness and which resource fills first are monitor obligations.
 */
bool be_it_pressure(unsigned kind, unsigned rounds)
{
    uint64_t got = 0, div_result = 0;
    for (unsigned i = 0; i < 512; ++i) pressure_mem[i][0] = 17 + i;
    backend_it_control.target_addr = (uintptr_t)pressure_mem;
    asm volatile("vsetivli zero,4,e32,m1,tu,mu\nvmv.v.i v8,3\nvmv.v.i v0,0\n"
        "li t0,7\nfcvt.d.l f0,t0\n" ::: "t0", "f0", "memory");
    for (unsigned round = 0; round < rounds; ++round) {
        uint64_t out = 0;
#define PRESSURE(body) asm volatile(BE_ASM_BEGIN \
    "mv t3,%[p]\nli t0,17\nli t1,0\n" BE_DELAY \
    body "mv %[got],t0\nmv %[div],t5\n" BE_ASM_END \
    : [got] "=r"(out), [div] "=r"(div_result) \
    : [p] "r"(pressure_mem), [v] "r"(pressure_vdump) \
    : "t0","t1","t2","t3","t4","t5","t6","f0","f1","f2","memory")
        switch (kind) {
        case 0: PRESSURE(".rept 192\naddi t0,t0,1\naddi t1,t1,3\nadd t2,t0,t1\n.endr\n"); break;
        case 1: PRESSURE(".rept 128\nfadd.d f1,f0,f0\nvadd.vi v8,v8,1\n.endr\nvse32.v v8,(%[v])\n"); break;
        case 2: PRESSURE(".rept 128\nvadd.vi v0,v0,1\n.endr\nvse32.v v0,(%[v])\n"); break;
        case 3:
            /* Compressed moves delay RAB mapping retirement without using VL. */
            PRESSURE(".rept 68\nmv t2,t0\n.endr\n.rept 96\nvsetivli zero,3,e32,m1,tu,mu\nvsetivli zero,4,e32,m1,tu,mu\n.endr\ncsrr t0,vl\n");
            break;
        case 4: PRESSURE(".rept 192\nld t2,0(t3)\nadd t0,t0,t2\naddi t3,t3,64\n.endr\n"); break;
        case 5: PRESSURE(".rept 192\nsd t0,0(t3)\naddi t0,t0,1\naddi t3,t3,64\n.endr\n"); break;
        case 7:
            /* Keep distinct architectural mappings and make moves sparse:
             * move elimination still consumes RAB entries, but no new preg. */
            asm volatile(BE_ASM_BEGIN
                "li s2,258\nli s3,259\nli s4,260\nli s5,261\nli s6,262\n"
                "li s7,263\nli s8,264\nli s9,265\nli s10,266\nli s11,267\n"
                "li t0,17\nli t1,0\n" BE_DELAY
                ".rept 12\n.rept 16\naddi t0,t0,1\naddi t1,t1,3\nadd t2,t0,t1\n.endr\n"
                "mv t4,t2\n.endr\nsub t0,t4,t1\nmv %[got],t0\nmv %[div],t5\n" BE_ASM_END
                : [got] "=r"(out), [div] "=r"(div_result) ::
                "t0","t1","t2","t4","t5","t6","s2","s3","s4","s5","s6",
                "s7","s8","s9","s10","s11","memory");
            break;
        default: PRESSURE(".rept 96\naddi t0,t0,1\nfadd.d f1,f0,f0\nvadd.vi v8,v8,1\n"
            "vadd.vi v0,v0,1\nvsetivli zero,4,e32,m1,tu,mu\n"
            "sd t0,0(t3)\nld t2,0(t3)\naddi t3,t3,64\n.endr\nvse32.v v8,(%[v])\n"); break;
        }
#undef PRESSURE
        uint64_t expected = kind == 0 || kind == 5 || kind == 7 ? 209 : kind == 3 ? 4 :
            kind == 4 ? 17 + 192 * 17 + 191 * 192 / 2 : kind >= 6 ? 113 : 17;
        uint64_t d = UINT64_MAX;
        for (unsigned i = 0; i < 8; ++i) d /= 3;
        if (out != expected || div_result != d) return false;
        if (kind == 5 || kind == 6) {
            unsigned count = kind == 5 ? 192 : 96;
            for (unsigned i = 0; i < count; ++i)
                if (pressure_mem[i][0] != 17 + i + (kind >= 6)) return false;
            if (pressure_mem[count][0] != 17 + count) return false;
        }
        if (kind == 1 || kind == 2) {
            uint32_t *v = (uint32_t *)pressure_vdump;
            for (unsigned i = 0; i < 4; ++i)
                if (v[i] != (kind == 1 ? 3 : 0) + 128 * (round + 1)) return false;
        }
        if (kind == 6) {
            uint64_t fp;
            asm volatile("fmv.x.d %0,f1" : "=r"(fp));
            if (fp != 0x402c000000000000ULL) return false;
            uint32_t *v = (uint32_t *)pressure_vdump;
            for (unsigned i=0;i<4;++i)
                if (v[i] != 3 + 96 * (round+1)) return false;
        }
        got += out;
    }
    return got != 0;
}

/* The two entry paths deliberately reverse branch/load age. The store
 * address is dependent, while the aliasing load's address is independent.
 */
static uint64_t pressure_redirect(unsigned kind, unsigned taken)
{
    uint64_t out;
    asm volatile(BE_ASM_BEGIN
        "li t0,1\nsd t0,0(%[p])\nsd zero,8(%[p])\nsd zero,16(%[p])\n"
        "fence rw,rw\nli t1,73\n"
        "beqz %[kind],4f\n"
        "mv t2,%[p]\nli t6,3\n.rept 5\nslli t3,t2,1\nadd t3,t3,t2\ndivu t2,t3,t6\n.endr\n"
        "sd t1,0(t2)\nld t4,0(%[p])\n"
        BE_DELAY "sltu t5,t5,zero\nor t5,t5,%[taken]\nbnez t5,1f\n"
        "sd t1,8(%[p])\n.rept 192\naddi t0,t0,1\n.endr\nj 2f\n1: sd t1,16(%[p])\n2: mv %[out],t4\nj 9f\n"
        "4:\n" BE_DELAY "sltu t5,t5,zero\nor t5,t5,%[taken]\nbnez t5,5f\n"
        "mv t2,%[p]\nli t6,3\n.rept 5\nslli t3,t2,1\nadd t3,t3,t2\ndivu t2,t3,t6\n.endr\n"
        "sd t1,0(t2)\nld t4,0(%[p])\nsd t4,8(%[p])\n.rept 192\naddi t0,t0,1\n.endr\nj 6f\n"
        "5: sd t1,16(%[p])\n6: ld %[out],0(%[p])\n9:\n" BE_ASM_END
        : [out] "=&r"(out)
        : [p] "r"(pressure_mem), [kind] "r"(kind), [taken] "r"((uint64_t)taken)
        : "t0","t1","t2","t3","t4","t5","t6","memory");
    return out;
}

bool be_it_redirect_probe(unsigned row)
{
    /* The tag confines the new kernels to redirect_age_matrix callers. */
    if (row & 0x10000u) {
        bool response_controlled = row & 0x20000u;
        row &= 0xffffu;
        if (response_controlled) {
            uint64_t got = 0, div_result = 0, a_value = 0, b_value = 0;
            volatile uint64_t *a = &pressure_mem[80][0];
            volatile uint64_t *b = &pressure_mem[82][0];
#define RG_LABEL(tag, name) ".global be_it_rg" #tag "_" #name "\nbe_it_rg" #tag "_" #name ":\n"
#define RG_MEM(tag, addr) RG_LABEL(tag,store) "sd t1,0(" addr ")\n" RG_LABEL(tag,alias) "ld t4,0(%[p])\n"
#define RG_KERNEL(tag, prefix, ready, mem, branch_code) asm volatile(BE_ASM_BEGIN \
                "li t1,73\nli t6,3\nli %[div],0\n" prefix \
                RG_LABEL(tag,a) "ld s2,0(%[a])\n" ready \
                RG_LABEL(tag,b) "ld s3,0(%[b])\n" \
                mem \
                RG_LABEL(tag,branch) branch_code \
                ".rept 112\nbnez zero,.+4\n.endr\n" \
                "j 2f\n1: sd t1,16(%[p])\n2: fence rw,rw\n" \
                "mv %[got],t4\nmv %[av],s2\nmv %[bv],s3\n" BE_ASM_END \
                : [got] "=&r"(got), [div] "=&r"(div_result), [av] "=&r"(a_value), [bv] "=&r"(b_value) \
                : [p] "r"(pressure_mem), [a] "r"(a), [b] "r"(b) \
                : "t1","t4","t6","s2","s3","memory")
#define RG_FULL_READY ".rept 32\nbnez zero,.+4\n.endr\n"
            switch (row) {
            case 675: RG_KERNEL(675,"","",RG_MEM(675,"s2"),"bnez s3,1f\nsd t1,8(%[p])\n"); break;
            case 685: RG_KERNEL(685,RG_FULL_READY,RG_FULL_READY,RG_LABEL(685,store) "addi zero,zero,0\n" RG_LABEL(685,alias) "li t4,73\n","bnez s3,1f\nsd t1,8(%[p])\n"); break;
            case 687: RG_KERNEL(687,"","divu %[div],s2,t6\n",RG_MEM(687,"s3"),"bnez zero,1f\n"); break;
            case 688: RG_KERNEL(688,"",RG_FULL_READY,RG_MEM(688,"s3"),"bnez zero,1f\n"); break;
            default: return false;
            }
#undef RG_FULL_READY
#undef RG_KERNEL
#undef RG_MEM
#undef RG_LABEL
            uint64_t expect_a = row == 675 ? (uintptr_t)pressure_mem : UINT64_MAX;
            uint64_t expect_b = row == 675 || row == 685 ? 1 : (uintptr_t)pressure_mem;
            bool ok = got == 73 && a_value == expect_a && b_value == expect_b &&
                *a == expect_a && *b == expect_b && pressure_mem[0][0] == 73 &&
                pressure_mem[0][1] == 0 && pressure_mem[0][2] == (row == 675 || row == 685 ? 73 : 0) &&
                pressure_mem[0][3] == 0x11223344 && pressure_mem[0][4] == 0 &&
                div_result == (row == 687 ? UINT64_MAX / 3 : 0);
            if (!ok) printf("BACKEND_REDIRECT_GATE_ARCH_FAIL row=%u got=%lx a=%lx b=%lx div=%lx markers=%lx/%lx/%lx/%lx/%lx\n",
                row, got, a_value, b_value, div_result, pressure_mem[0][0], pressure_mem[0][1],
                pressure_mem[0][2], pressure_mem[0][3], pressure_mem[0][4]);
            return ok;
        }
        bool branch_first = row == 674 || row == 677 || row == 678 ||
            (row >= 680 && row <= 685);
        bool targeted = row == 674 || row == 675 || row == 676 || row == 678 || row == 687 || row == 688;
        for (unsigned variant = 0; variant < (targeted ? 3u : 8u); ++variant) {
            for (unsigned i = 0; i < 33; ++i) {
                uint64_t taken = i == 32, got = 0, div_result = 0;
                volatile uint64_t *store = &pressure_mem[0][taken ? 0 : 4];
                pressure_mem[0][0] = 1;
                pressure_mem[0][1] = 0;
                pressure_mem[0][2] = 0;
                pressure_mem[0][3] = 0x11223344;
                pressure_mem[0][4] = 0;
                asm volatile("fence rw,rw" ::: "memory");
                if (row == 678) {
                    volatile uint64_t *cold = &pressure_mem[64 + variant][0];
                    *cold = 1;
                    asm volatile("cbo.flush 0(%0)\nfence rw,rw" :: "r"(cold) : "memory");
                    /* The miss and branch share the late DIV operand. */
#define REDIRECT_COLD(delay) asm volatile(BE_ASM_BEGIN \
                        "li t0,1\nli t1,73\n" BE_DELAY \
                        "sltu t5,t5,zero\nor t3,t5,%[taken]\n" delay \
                        "bnez t3,1f\nadd t4,t5,%[cold]\nld t4,0(t4)\n" \
                        "sd t4,8(%[p])\nadd t2,t5,%[store]\n" \
                        "mul t2,t2,t0\nmul t2,t2,t0\nsd t1,0(t2)\n" \
                        ".rept 12\nadd t0,t5,t6\nmul t2,t0,t6\n.endr\nj 2f\n" \
                        "1: sd t1,16(%[p])\n2: ld %[got],0(%[p])\n" BE_ASM_END \
                        : [got] "=&r"(got) \
                        : [p] "r"(pressure_mem), [store] "r"(store), \
                          [taken] "r"(taken), [cold] "r"(cold) \
                        : "t0","t1","t2","t3","t4","t5","t6","memory")
                    switch (variant) {
                    case 0: REDIRECT_COLD(".rept 2\nandi t3,t3,255\n.endr\n"); break;
                    case 1: REDIRECT_COLD("mul t3,t3,t6\n"); break;
                    default: REDIRECT_COLD("mul t3,t3,t6\nandi t3,t3,255\n"); break;
                    }
#undef REDIRECT_COLD
                    /* Keep the independent selfcheck from prewarming cold. */
                    asm volatile("fence rw,rw" ::: "memory");
                    if (got != 1 || pressure_mem[0][0] != 1 || *cold != 1 ||
                        pressure_mem[0][1] != (taken ? 0 : 1) ||
                        pressure_mem[0][2] != (taken ? 73 : 0) ||
                        pressure_mem[0][4] != (taken ? 0 : 73)) return false;
                } else if (row == 683) {
                    /* An independent old DIV keeps the head unready. */
                    asm volatile(BE_ASM_BEGIN
                        "li t5,-1\nli t6,3\nli t1,73\n"
                        "divu t5,t5,t6\nbnez %[taken],1f\n"
                        "fence rw,rw\nsd t1,8(%[p])\nj 2f\n"
                        "1: fence rw,rw\nsd t1,16(%[p])\n"
                        "2: mv %[div],t5\nld %[got],0(%[p])\n" BE_ASM_END
                        : [got] "=&r"(got), [div] "=&r"(div_result)
                        : [p] "r"(pressure_mem), [taken] "r"(taken)
                        : "t1","t5","t6","memory");
                    if (got != 1 || div_result != UINT64_MAX / 3 ||
                        pressure_mem[0][0] != 1 || pressure_mem[0][4] != 0 ||
                        pressure_mem[0][1] != (taken ? 0 : 73) ||
                        pressure_mem[0][2] != (taken ? 73 : 0)) return false;
                } else if (row == 686) {
                    asm volatile(BE_ASM_BEGIN
                        "li t5,-1\nli t6,3\nli t1,73\nli t0,1\n"
                        "divu t5,t5,t6\nmul t2,%[store],t0\nmul t2,t2,t0\n"
                        "sd t1,0(t2)\nld t4,0(%[p])\nfence rw,rw\n"
                        "sd t4,8(%[p])\nmv %[div],t5\nmv %[got],t4\n" BE_ASM_END
                        : [got] "=&r"(got), [div] "=&r"(div_result)
                        : [p] "r"(pressure_mem), [store] "r"(store)
                        : "t0","t1","t2","t4","t5","t6","memory");
                    uint64_t expected = taken ? 73 : 1;
                    if (got != expected || div_result != UINT64_MAX / 3 ||
                        pressure_mem[0][0] != expected || pressure_mem[0][1] != expected ||
                        pressure_mem[0][2] != 0 || pressure_mem[0][4] != (taken ? 0 : 73)) return false;
                } else {
                    /* Train non-aliasing addresses, then introduce the alias.
                     * Both resolution chains depend on the same real DIV. */
#define REDIRECT_KERNEL_EX(bdelay, skid, pad, align, ready, store_skid) do { \
                    if (branch_first) { \
                        asm volatile(BE_ASM_BEGIN \
                            align "li t0,1\nli t1,73\n" BE_DELAY \
                            "sltu t5,t5,zero\nor t3,t5,%[taken]\n" \
                            ".rept " bdelay "\nmul t3,t3,t6\n.endr\n" skid "bnez t3,1f\n" \
                            "add t2,t5,%[store]\nmul t2,t2,t0\nmul t2,t2,t0\n" \
                            "sd t1,0(t2)\nld t4,0(%[p])\nsd t4,8(%[p])\n" \
                            pad "j 2f\n1: sd t1,16(%[p])\n2: ld %[got],0(%[p])\n" BE_ASM_END \
                            : [got] "=&r"(got) \
                            : [p] "r"(pressure_mem), [store] "r"(store), [taken] "r"(taken) \
                            : "t0","t1","t2","t3","t4","t5","t6","memory"); \
                    } else { \
                        asm volatile(BE_ASM_BEGIN \
                            align "li t0,1\nli t1,73\n" BE_DELAY \
                            "sltu t5,t5,zero\nadd t2,t5,%[store]\n" \
                            "mul t2,t2,t0\nmul t2,t2,t0\n" store_skid "sd t1,0(t2)\nld t4,0(%[p])\n" ready \
                            "or t3,t5,%[taken]\n.rept " bdelay "\nmul t3,t3,t6\n.endr\n" skid "bnez t3,1f\n" \
                            "sd t4,8(%[p])\n" pad "j 2f\n1: sd t1,16(%[p])\n2: mv %[got],t4\n" BE_ASM_END \
                            : [got] "=&r"(got) \
                            : [p] "r"(pressure_mem), [store] "r"(store), [taken] "r"(taken) \
                            : "t0","t1","t2","t3","t4","t5","t6","memory"); \
                    } \
                } while (0)
#define REDIRECT_FILL ".rept 112\nbnez zero,.+4\n.endr\n"
#define REDIRECT_WORK ".rept 12\nadd t0,t5,t6\nmul t2,t0,t6\n.endr\n"
#define REDIRECT_KERNEL(delay, skid, pad, align) REDIRECT_KERNEL_EX(delay, skid, pad, align, "", "")
#define REDIRECT_PAIR(delay, pad) REDIRECT_KERNEL(delay, "", pad, "")
                    /* Distinct encodings preserve per-row static PCs. */
                    if (row == 674) {
                        switch (variant) {
                        case 0: REDIRECT_KERNEL("3", "", REDIRECT_WORK, "addi zero,zero,674\n"); break;
                        case 1: REDIRECT_KERNEL("3", "andi t3,t3,255\n", REDIRECT_WORK, "addi zero,zero,674\n"); break;
                        default: REDIRECT_KERNEL("2", ".rept 2\nandi t3,t3,255\n.endr\n", REDIRECT_WORK, "addi zero,zero,674\n"); break;
                        }
                    } else if (row == 675) {
                        switch (variant) {
                        case 0: REDIRECT_KERNEL_EX("3", "andi t3,t3,255\n", REDIRECT_WORK, "addi zero,zero,675\n", "", "add t2,t2,t5\n"); break;
                        case 1: REDIRECT_KERNEL_EX("3", ".rept 2\nandi t3,t3,255\n.endr\n", REDIRECT_WORK, "addi zero,zero,675\n", "", "add t2,t2,t5\n"); break;
                        default: REDIRECT_KERNEL_EX("3", "andi t3,t3,255\n", REDIRECT_WORK, "addi zero,zero,675\n", "", ".rept 2\nadd t2,t2,t5\n.endr\n"); break;
                        }
                    } else if (row == 676) {
                        switch (variant) {
                        case 0: REDIRECT_KERNEL("2", "", REDIRECT_WORK, "addi zero,zero,676\n"); break;
                        case 1: REDIRECT_KERNEL("2", "", REDIRECT_WORK, "addi zero,zero,676\nbnez zero,.+4\n"); break;
                        default: REDIRECT_KERNEL("1", "", REDIRECT_WORK, "addi zero,zero,676\n"); break;
                        }
                    } else if (row == 687) {
                        /* Prefixes target bank 7 from the v3 allocation trace. */
                        switch (variant) {
                        case 0: REDIRECT_KERNEL("8", "", REDIRECT_FILL, "addi zero,zero,687\n.rept 3\nbnez zero,.+4\n.endr\n"); break;
                        case 1: REDIRECT_KERNEL("8", "", REDIRECT_FILL, "addi zero,zero,687\n.rept 7\nbnez zero,.+4\n.endr\n"); break;
                        default: REDIRECT_KERNEL("8", "", REDIRECT_FILL, "addi zero,zero,687\n.rept 10\nbnez zero,.+4\n.endr\n"); break;
                        }
                    } else if (row == 688) {
                        /* Independent ready entries precede the slow branch chain. */
#define REDIRECT_READY ".rept 8\nbnez zero,.+4\n.endr\n"
                        switch (variant) {
                        case 0: REDIRECT_KERNEL_EX("8", "", REDIRECT_FILL, "addi zero,zero,688\n.rept 6\nbnez zero,.+4\n.endr\n", REDIRECT_READY, ""); break;
                        case 1: REDIRECT_KERNEL_EX("8", "", REDIRECT_FILL, "addi zero,zero,688\n.rept 5\nbnez zero,.+4\n.endr\n", REDIRECT_READY, ""); break;
                        default: REDIRECT_KERNEL_EX("8", "", REDIRECT_FILL, "addi zero,zero,688\n.rept 9\nbnez zero,.+4\n.endr\n", REDIRECT_READY, ""); break;
                        }
#undef REDIRECT_READY
                    } else {
                    switch (variant) {
                    case 0: REDIRECT_PAIR("0", REDIRECT_WORK); break;
                    case 1: REDIRECT_PAIR("2", REDIRECT_WORK); break;
                    case 2: REDIRECT_PAIR("4", REDIRECT_WORK); break;
                    case 3: REDIRECT_PAIR("6", REDIRECT_WORK); break;
                    case 4: REDIRECT_PAIR("8", REDIRECT_FILL); break;
                    case 5: REDIRECT_PAIR("10", REDIRECT_FILL); break;
                    case 6: REDIRECT_PAIR("12", REDIRECT_FILL); break;
                    default: REDIRECT_PAIR("14", REDIRECT_FILL); break;
                    }
                    }
#undef REDIRECT_WORK
#undef REDIRECT_FILL
#undef REDIRECT_PAIR
#undef REDIRECT_KERNEL
#undef REDIRECT_KERNEL_EX
                    uint64_t expected = !branch_first && taken ? 73 : 1;
                    if (got != expected || pressure_mem[0][0] != expected ||
                        pressure_mem[0][1] != (taken ? 0 : 1) ||
                        pressure_mem[0][2] != (taken ? 73 : 0) ||
                        pressure_mem[0][4] != (taken ? 0 : 73)) return false;
                }
                if (pressure_mem[0][3] != 0x11223344) return false;
            }
        }
        return true;
    }
    unsigned kind = row == 674 || row == 677 || row == 678 || row == 599 ||
        row == 623 || row == 633 || (row >= 680 && row <= 685) ? 0 : 1;
    for (unsigned i = 0; i < 33; ++i) {
        unsigned taken = i == 32;
        uint64_t got = pressure_redirect(kind,taken);
        uint64_t expected = kind == 0 && taken ? 1 : 73;
        if (got != expected || pressure_mem[0][1] != (taken ? 0 : 73) ||
            pressure_mem[0][2] != (taken ? 73 : 0)) return false;
    }
    return true;
}

/* Keep the unresolved older instruction and the V0 allocations in one asm
 * block. The monitor must prove genuine resource shortage before redirect. */
static bool freelist_redirect_pressure(unsigned row)
{
    unsigned rounds = row == 599 ? 33 : 1;
    for (unsigned i = 0; i < rounds; ++i) {
        uint64_t taken = i == 32, got = 0;
        pressure_mem[0][0] = 1;
        pressure_mem[0][1] = 0;
        pressure_mem[0][2] = 0;
        pressure_mem[0][3] = 0x11223344;
        asm volatile("vsetivli zero,4,e32,m1,tu,mu\nvmv.v.i v0,0\nfence rw,rw\n" ::: "memory");
        if (row == 599) {
            asm volatile(BE_ASM_BEGIN
                BE_DELAY "sltu t5,t5,zero\nor t5,t5,%[taken]\nbnez t5,1f\n"
                ".rept 64\nvadd.vi v0,v0,1\n.endr\n"
                "li t0,73\nsd t0,8(%[p])\nj 2f\n"
                "1: li t0,73\nsd t0,16(%[p])\n"
                "2: vse32.v v0,(%[v])\n" BE_ASM_END
                :: [p] "r"(pressure_mem), [v] "r"(pressure_vdump), [taken] "r"(taken)
                : "t0","t5","t6","memory");
            if (pressure_mem[0][0] != 1 || pressure_mem[0][1] != (taken ? 0 : 73) ||
                pressure_mem[0][2] != (taken ? 73 : 0)) return false;
        } else {
            asm volatile(BE_ASM_BEGIN
                "mv t2,%[p]\nli t1,73\nli t6,3\n"
                ".rept 8\nslli t3,t2,1\nadd t3,t3,t2\ndivu t2,t3,t6\n.endr\n"
                "sd t1,0(t2)\nld t4,0(%[p])\n"
                ".rept 64\nvadd.vi v0,v0,1\n.endr\n"
                "sd t4,8(%[p])\nvse32.v v0,(%[v])\nmv %[got],t4\n" BE_ASM_END
                : [got] "=&r"(got) : [p] "r"(pressure_mem), [v] "r"(pressure_vdump)
                : "t1","t2","t3","t4","t6","memory");
            if (got != 73 || pressure_mem[0][0] != 73 || pressure_mem[0][1] != 73 ||
                pressure_mem[0][2] != 0) return false;
        }
        if (pressure_mem[0][3] != 0x11223344) return false;
        for (unsigned lane = 0; lane < 4; ++lane)
            if (((uint32_t *)pressure_vdump)[lane] != (taken ? 0 : 64)) return false;
    }
    return true;
}

bool ai_micro_backend_it_freelist_pressure(void)
{
    TEST_START();
    be_it_enable();
    for (unsigned row = 593; row <= 600; ++row) {
        backend_it_control.target_addr = (uintptr_t)pressure_mem;
        if (!be_it_begin(row,0)) return false;
        TEST_SETUP_EXCEPT();
        bool ok = row >= 599 ? freelist_redirect_pressure(row) :
            be_it_pressure(row == 593 || row == 597 ? 7 : row <= 596 ? row - 593 : 6, 2);
        TEST_ASSERT("allocation pressure drains with exact scalar/vector values", ok && !excpt.triggered);
        TEST_ASSERT("IT observed the selected FreeList boundary and single allocation", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_freelist_pressure");
}

bool ai_micro_backend_it_dispatch_pressure(void)
{
    TEST_START();
    be_it_enable();
    const unsigned kinds[] = {0,0,1,6,4,5,3,6,6,6,0,0,0};
    for (unsigned row = 615; row <= 627; ++row) {
        backend_it_control.target_addr = (uintptr_t)pressure_mem;
        if (!be_it_begin(row,0)) return false;
        TEST_SETUP_EXCEPT();
        bool ok = true;
        if (row == 617 || row == 618 || row == 619 || row == 627) {
            for (unsigned round = 0; round < 4; ++round) {
                uint64_t got = 0, second = 0;
                uint64_t delayed = UINT64_MAX;
                for (unsigned i = 0; i < 8; ++i) delayed /= 3;
                pressure_mem[0][0] = 101;
                asm volatile("fence rw,rw" ::: "memory");
                if (row == 617) {
                    /* All younger FP operations wait for the old DIV result. */
                    asm volatile(BE_ASM_BEGIN
                        "li t0,7\nfcvt.d.l f2,t0\n" BE_DELAY
                        "sltu t5,t5,zero\nfmv.d.x f0,t5\n"
                        ".rept 80\nfadd.d f1,f0,f2\n.endr\nfmv.x.d %[out],f1\n" BE_ASM_END
                        : [out] "=r"(got) :: "t0","t5","t6","f0","f1","f2","memory");
                    ok &= got == 0x401c000000000000ULL;
                } else if (row == 618) {
                    /* Keep load addresses unresolved while LS DQ fills. */
                    asm volatile(BE_ASM_BEGIN
                        "mv t2,%[p]\nli t6,3\n"
                        ".rept 8\nslli t3,t2,1\nadd t3,t3,t2\ndivu t2,t3,t6\n.endr\n"
                        ".rept 80\nld t0,0(t2)\n.endr\nmv %[out],t0\n" BE_ASM_END
                        : [out] "=r"(got) : [p] "r"(pressure_mem)
                        : "t0","t2","t3","t6","memory");
                    ok &= got == 101 && pressure_mem[0][0] == 101;
                } else if (row == 619) {
                    /* The oldest load holds VLQ dequeue; younger loads finish. */
                    asm volatile(BE_ASM_BEGIN
                        "mv t2,%[p]\nli t6,3\n"
                        ".rept 8\nslli t3,t2,1\nadd t3,t3,t2\ndivu t2,t3,t6\n.endr\n"
                        "ld t1,0(t2)\n.rept 79\nld t0,0(%[p])\n.endr\n"
                        "mv %[out],t0\nmv %[old],t1\n" BE_ASM_END
                        : [out] "=&r"(got), [old] "=&r"(second)
                        : [p] "r"(pressure_mem)
                        : "t0","t1","t2","t3","t6","memory");
                    ok &= got == 101 && second == 101 && pressure_mem[0][0] == 101;
                } else {
                    asm volatile(BE_ASM_BEGIN
                        BE_DELAY ".rept 40\ndivu t0,t5,t6\n.endr\n"
                        ".rept 16\nmul t1,t5,t6\n.endr\n"
                        "mv %[out],t0\nmv %[mul],t1\n" BE_ASM_END
                        : [out] "=&r"(got), [mul] "=&r"(second) :: "t0","t1","t5","t6","memory");
                    ok &= got == delayed / 3 && second == delayed * 3;
                }
            }
        } else {
            ok = be_it_pressure(kinds[row-615],4);
        }
        if (row == 623 || row == 624) ok &= be_it_redirect_probe(row);
        TEST_ASSERT("dispatch resumes without lost or duplicate architectural work", ok && !excpt.triggered);
        TEST_ASSERT("IT verified the sole blocked resource and dispatch destination", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_dispatch_pressure");
}

bool ai_micro_backend_it_queue_boundary(void)
{
    TEST_START();
    be_it_enable();
    for (unsigned row = 629; row <= 640; ++row) {
        if (row == 635) continue;
        backend_it_control.target_addr = (uintptr_t)pressure_mem;
        if (!be_it_begin(row,0)) return false;
        TEST_SETUP_EXCEPT();
        bool ok = true;
        if (row == 632) {
            /* Restrict dispatch to the two MUL queues while their source waits.
             * The ready-ALU stream did not retain DQ pressure after C1 layout. */
            uint64_t delayed = UINT64_MAX;
            for (unsigned i = 0; i < 8; ++i) delayed /= 3;
            for (unsigned round = 0; round < 8; ++round) {
                uint64_t product = 0, source = 0;
                asm volatile(BE_ASM_BEGIN BE_DELAY
                    ".rept 128\nmul t0,t5,t6\n.endr\n"
                    "mv %[a],t0\nmv %[b],t5\n" BE_ASM_END
                    : [a] "=&r"(product), [b] "=&r"(source) ::
                    "t0","t5","t6","memory");
                ok &= product == delayed * 3 && source == delayed;
            }
        } else if (row == 633 || row == 634) {
            unsigned rounds = row == 633 ? 33 : 8;
            for (unsigned round = 0; round < rounds; ++round) {
                uint64_t taken = round == 32, got = 0;
                uint64_t delayed = UINT64_MAX;
                for (unsigned i = 0; i < 8; ++i) delayed /= 3;
                pressure_mem[0][0] = 1;
                pressure_mem[0][1] = pressure_mem[0][2] = 0;
                pressure_mem[0][3] = 0x11223344;
                asm volatile("fence rw,rw" ::: "memory");
                if (row == 633) {
                    /* A younger DIV keeps MUL entries waiting across redirect. */
                    asm volatile(BE_ASM_BEGIN
                        "li t0,0\nli t4,-1\n" BE_DELAY
                        "sltu t5,t5,zero\nor t5,t5,%[taken]\nbnez t5,1f\n"
                        ".rept 8\ndivu t4,t4,t6\n.endr\n"
                        ".rept 48\nmul t0,t4,t6\n.endr\n"
                        "li t1,73\nsd t1,8(%[p])\nj 2f\n"
                        "1: li t1,73\nsd t1,16(%[p])\n2: mv %[out],t0\n" BE_ASM_END
                        : [out] "=r"(got) : [p] "r"(pressure_mem), [taken] "r"(taken)
                        : "t0","t1","t4","t5","t6","memory");
                    ok &= got == (taken ? 0 : delayed * 3) && pressure_mem[0][0] == 1 &&
                        pressure_mem[0][1] == (taken ? 0 : 73) && pressure_mem[0][2] == (taken ? 73 : 0);
                } else {
                    asm volatile(BE_ASM_BEGIN
                        "mv t2,%[p]\nli t1,73\nli t6,3\nli t4,-1\n"
                        ".rept 8\nslli t3,t2,1\nadd t3,t3,t2\ndivu t2,t3,t6\n.endr\n"
                        "sd t1,0(t2)\nld t5,0(%[p])\n"
                        ".rept 8\ndivu t4,t4,t6\n.endr\n"
                        ".rept 48\nmul t0,t4,t6\n.endr\n"
                        "sd t5,8(%[p])\nsd t0,16(%[p])\nmv %[out],t5\n" BE_ASM_END
                        : [out] "=r"(got) : [p] "r"(pressure_mem)
                        : "t0","t1","t2","t3","t4","t5","t6","memory");
                    ok &= got == 73 && pressure_mem[0][0] == 73 && pressure_mem[0][1] == 73 &&
                        pressure_mem[0][2] == delayed * 3;
                }
                ok &= pressure_mem[0][3] == 0x11223344;
            }
        } else if (row >= 637) {
            /* Scheduling is an RTL obligation; arithmetic is independent. */
            uint64_t delayed = UINT64_MAX;
            for (unsigned i = 0; i < 8; ++i) delayed /= 3;
            for (unsigned attempt = 0; attempt < (row == 640 ? 256U : 64U); ++attempt) {
                uint64_t out0 = 0, out1 = 0, out2 = 0;
                unsigned variant = attempt % 16;
                if (row == 637) {
                    asm volatile(BE_ASM_BEGIN
                        BE_DELAY "li s2,0\n"
                        ".rept 64\nmul t0,t5,t6\nadd s2,s2,t0\n.endr\n"
                        "mv %[a],s2\nmv %[b],t5\n" BE_ASM_END
                        : [a] "=&r"(out0), [b] "=&r"(out1) ::
                        "t0","t5","t6","s2","memory");
                    ok &= out0 == delayed * 3 * 64 && out1 == delayed;
                } else if (row == 638) {
                    /* Waiting MULs occupy both MUL IQs. A ready ADD may issue
                     * from another IQ as a new dependent MUL enters. Its other
                     * source stays delayed so EnqEntry migration is observable. */
#define QUEUE_FAST(count,gap) asm volatile(BE_ASM_BEGIN \
                        "li s5,11\nli s7,7\n" BE_DELAY \
                        ".rept " #count "\nmul t0,t5,t6\n.endr\n" \
                        "add s3,s5,s7\n" \
                        ".rept " #gap "\nfsgnj.d f2,f0,f0\n.endr\n" \
                        "mul t1,s3,t5\nmul t2,s3,t5\n" \
                        "mv %[a],t0\nmv %[b],t1\nmv %[c],t2\n" BE_ASM_END \
                        : [a] "=&r"(out0), [b] "=&r"(out1), [c] "=&r"(out2) :: \
                        "t0","t1","t2","t5","t6","s3","s5","s7","f2","memory")
#define QUEUE_FAST_CASE(n,count,gap) case n: QUEUE_FAST(count,gap); break
                    switch (variant) {
                    QUEUE_FAST_CASE(0,28,0); QUEUE_FAST_CASE(1,30,0);
                    QUEUE_FAST_CASE(2,32,0); QUEUE_FAST_CASE(3,34,0);
                    QUEUE_FAST_CASE(4,28,1); QUEUE_FAST_CASE(5,30,1);
                    QUEUE_FAST_CASE(6,32,1); QUEUE_FAST_CASE(7,34,1);
                    QUEUE_FAST_CASE(8,28,2); QUEUE_FAST_CASE(9,30,2);
                    QUEUE_FAST_CASE(10,32,2); QUEUE_FAST_CASE(11,34,2);
                    QUEUE_FAST_CASE(12,28,3); QUEUE_FAST_CASE(13,30,3);
                    QUEUE_FAST_CASE(14,32,3); QUEUE_FAST_CASE(15,34,3);
                    }
#undef QUEUE_FAST_CASE
#undef QUEUE_FAST
                    ok &= out0 == delayed * 3 && out1 == delayed * 18 && out2 == delayed * 18;
                } else if (row == 639) {
                    /* Keep ready MULs draining while DIV-dependent MULs enter.
                     * Per-operation sum chains otherwise occupy the ALU ports
                     * and can leave no release on the DIV's writeback cycle. */
                    uint64_t divisor = delayed >> variant;
                    uint64_t quotient = delayed / divisor;
#define QUEUE_WB(count) asm volatile(BE_ASM_BEGIN \
                        "mv s6,%[d]\n" BE_DELAY \
                        "divu s3,t5,s6\n" \
                        ".rept " #count "\nmul t0,t5,t6\n.endr\n" \
                        ".rept 64\nmul t1,s3,t6\nmul t0,t5,t6\n" \
                        "mul t1,s3,t6\nmul t0,t5,t6\n.endr\n" \
                        "mv %[a],t0\nmv %[b],t1\nmv %[c],s3\n" BE_ASM_END \
                        : [a] "=&r"(out0), [b] "=&r"(out1), [c] "=&r"(out2) : [d] "r"(divisor) : \
                        "t0","t1","t5","t6","s2","s3","s4","s6","memory")
#define QUEUE_WB_CASE(n,count) case n: QUEUE_WB(count); break
                    switch (attempt / 16) {
                    QUEUE_WB_CASE(0,50); QUEUE_WB_CASE(1,52); QUEUE_WB_CASE(2,54); QUEUE_WB_CASE(3,56);
                    }
#undef QUEUE_WB_CASE
#undef QUEUE_WB
                    ok &= out0 == delayed * 3 && out1 == quotient * 3 && out2 == quotient;
                } else {
                    /* The independent MULs broadcast to the same ADD through
                     * distinct fast ports (confirmed in v18). All FP values
                     * are small exact integers, so the oracle needs no tolerance. */
                    uint64_t numerator = UINT64_MAX >> (attempt / 64);
#define QUEUE_DUAL(gap) asm volatile(BE_ASM_BEGIN \
                        "li t6,3\nli t0,7\nli t1,1\n" \
                        "fcvt.d.l f0,t0\nfcvt.d.l f1,t1\nfsgnj.d f2,f0,f0\n" \
                        "divu s2,%[n],t6\n" \
                        ".rept " #gap "\nfadd.d f2,f2,f1\n.endr\n" \
                        "fcvt.l.d s3,f2,rtz\nmul t2,s2,s3\nmul t3,s2,s3\n" \
                        "mv %[a],s2\nmv %[b],s3\nadd %[c],t2,t3\n" BE_ASM_END \
                        : [a] "=&r"(out0), [b] "=&r"(out1), [c] "=&r"(out2) \
                        : [n] "r"(numerator) \
                        : "t0","t1","t2","t3","t6","s2","s3","f0","f1","f2","memory")
#define QUEUE_DUAL_CASE(gap) case gap: QUEUE_DUAL(gap); break
                    switch (attempt % 32) {
                    QUEUE_DUAL_CASE(0); QUEUE_DUAL_CASE(1); QUEUE_DUAL_CASE(2); QUEUE_DUAL_CASE(3);
                    QUEUE_DUAL_CASE(4); QUEUE_DUAL_CASE(5); QUEUE_DUAL_CASE(6); QUEUE_DUAL_CASE(7);
                    QUEUE_DUAL_CASE(8); QUEUE_DUAL_CASE(9); QUEUE_DUAL_CASE(10); QUEUE_DUAL_CASE(11);
                    QUEUE_DUAL_CASE(12); QUEUE_DUAL_CASE(13); QUEUE_DUAL_CASE(14); QUEUE_DUAL_CASE(15);
                    QUEUE_DUAL_CASE(16); QUEUE_DUAL_CASE(17); QUEUE_DUAL_CASE(18); QUEUE_DUAL_CASE(19);
                    QUEUE_DUAL_CASE(20); QUEUE_DUAL_CASE(21); QUEUE_DUAL_CASE(22); QUEUE_DUAL_CASE(23);
                    QUEUE_DUAL_CASE(24); QUEUE_DUAL_CASE(25); QUEUE_DUAL_CASE(26); QUEUE_DUAL_CASE(27);
                    QUEUE_DUAL_CASE(28); QUEUE_DUAL_CASE(29); QUEUE_DUAL_CASE(30); QUEUE_DUAL_CASE(31);
                    }
#undef QUEUE_DUAL_CASE
#undef QUEUE_DUAL
                    ok &= out0 == numerator / 3 && out1 == 7 + attempt % 32 &&
                        out2 == (numerator / 3) * (7 + attempt % 32) * 2;
                }
                if (!ok) printf("BE_IT_QUEUE_ARCH_FAIL row=%u attempt=%u values=%lx/%lx/%lx\n",
                    row, attempt, out0, out1, out2);
            }
        } else {
            ok = be_it_pressure(0,8);
        }
        TEST_ASSERT("queue fill/drain and redirect preserve data and progress", ok && !excpt.triggered);
        TEST_ASSERT("IT checked occupancy, handshake, actual release and wakeup ports", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_queue_boundary");
}


bool ai_micro_backend_it_redirect_age_matrix(void)
{
    static uint64_t response_plan[8] __attribute__((aligned(64)));
    extern void be_it_rg681_run(volatile uint64_t *, volatile uint64_t *, volatile uint64_t *, uint64_t *);
    extern void be_it_rg684_run(volatile uint64_t *, volatile uint64_t *, volatile uint64_t *, uint64_t *);
    /* Keep follow-up instruction sequences outside the calibrated helper. */
#define RG_FOLLOW_LABEL(tag, name) ".global be_it_rg" #tag "_" #name "\nbe_it_rg" #tag "_" #name ":\n"
#define RG_FOLLOW_KERNEL(tag, old, condition, young) \
        ".balign 4\n.global be_it_rg" #tag "_run\n.type be_it_rg" #tag "_run,@function\n" \
        "be_it_rg" #tag "_run:\nli t0,73\nli t6,3\nli t2,0\nli t4,0\n" \
        RG_FOLLOW_LABEL(tag,a) "ld t5,0(a0)\n" old \
        RG_FOLLOW_LABEL(tag,b) "ld t1,0(a1)\n" condition \
        RG_FOLLOW_LABEL(tag,branch) "bnez t3,1f\n" young \
        "sd t2,8(a2)\n.rept 112\nbnez zero,.+4\n.endr\nj 2f\n" \
        "1: sd t0,16(a2)\n2: fence rw,rw\n" \
        "sd t5,0(a3)\nsd t1,8(a3)\nsd t2,16(a3)\nsd t4,24(a3)\nret\n" \
        ".size be_it_rg" #tag "_run,.-be_it_rg" #tag "_run\n"
    asm(BE_ASM_BEGIN ".pushsection .text.be_it_redirect_followup,\"ax\",@progbits\n"
        RG_FOLLOW_KERNEL(681,"","mv t3,t1\n",
            RG_FOLLOW_LABEL(681,store) "addi t2,t1,1\naddi t2,t2,1\n"
            RG_FOLLOW_LABEL(681,alias) "addi t2,t2,1\n")
        RG_FOLLOW_KERNEL(684,"divu t4,t5,t6\n","andi t3,t1,1\n",
            RG_FOLLOW_LABEL(684,store) "addi t2,t1,1\n"
            RG_FOLLOW_LABEL(684,alias) "addi t2,t2,1\n")
        ".popsection\n" BE_ASM_END);
#undef RG_FOLLOW_KERNEL
#undef RG_FOLLOW_LABEL
#define RG_EXTERN(tag) extern char be_it_rg##tag##_a[], be_it_rg##tag##_b[], \
        be_it_rg##tag##_store[], be_it_rg##tag##_alias[], be_it_rg##tag##_branch[]
    RG_EXTERN(675); RG_EXTERN(681); RG_EXTERN(684); RG_EXTERN(685); RG_EXTERN(687); RG_EXTERN(688);
#undef RG_EXTERN
    TEST_START();
    be_it_enable();
    for (unsigned row = 674; row <= 688; ++row) {
        if (row == 679 || row == 682) continue;
        bool followup = row == 681 || row == 684;
        bool controlled = row == 675 || followup || row == 685 || row == 687 || row == 688;
        backend_it_control.target_pc = 0;
        backend_it_control.target_addr = (uintptr_t)pressure_mem;
        backend_it_control.auxiliary = 0;
        if (controlled) {
#define RG_PLAN(tag) do { \
                backend_it_control.target_pc = (uintptr_t)be_it_rg##tag##_a; \
                response_plan[0] = (uintptr_t)be_it_rg##tag##_b; \
                response_plan[1] = (uintptr_t)be_it_rg##tag##_store; \
                response_plan[2] = (uintptr_t)be_it_rg##tag##_alias; \
                response_plan[3] = (uintptr_t)be_it_rg##tag##_branch; \
            } while (0)
            switch (row) {
            case 675: RG_PLAN(675); break;
            case 681: RG_PLAN(681); break;
            case 684: RG_PLAN(684); break;
            case 685: RG_PLAN(685); break;
            case 687: RG_PLAN(687); break;
            default: RG_PLAN(688); break;
            }
#undef RG_PLAN
            for (unsigned i=0;i<5;i++) pressure_mem[0][i]=0;
            pressure_mem[0][0]=row == 685 ? 73 : 1; pressure_mem[0][3]=0x11223344;
            pressure_mem[80][0] = row == 675 ? (uintptr_t)pressure_mem : UINT64_MAX;
            pressure_mem[82][0] = row == 675 || row == 685 || followup ? 1 : (uintptr_t)pressure_mem;
            response_plan[4] = (uintptr_t)&pressure_mem[82][0];
            /* v18: align memory/branch candidates and retain A until accept. */
            response_plan[5] = row == 687 ? 5 : (row == 675 ? 4ULL<<16 : 0);
            if (followup) response_plan[5] = row == 681 ? 3 : 2;
            response_plan[6] = 0x5245444952454354ULL;
            response_plan[7] = 0;
            backend_it_control.target_addr = (uintptr_t)&pressure_mem[80][0];
            backend_it_control.auxiliary = (uintptr_t)response_plan;
            asm volatile("cbo.flush 0(%0)\ncbo.flush 0(%1)\ncbo.flush 0(%2)\nfence rw,rw"
                :: "r"(&pressure_mem[80][0]), "r"(&pressure_mem[82][0]), "r"(response_plan) : "memory");
        }
        if (!be_it_begin_scenario(row, controlled ? 0x9101U : 0, 0)) return false;
        TEST_SETUP_EXCEPT();
        bool architectural_ok;
        if (followup) {
            uint64_t result[4] = {0};
            if (row == 681) be_it_rg681_run(&pressure_mem[80][0], &pressure_mem[82][0], &pressure_mem[0][0], result);
            else be_it_rg684_run(&pressure_mem[80][0], &pressure_mem[82][0], &pressure_mem[0][0], result);
            architectural_ok = result[0] == UINT64_MAX && result[1] == 1 && result[2] == 0 &&
                result[3] == (row == 684 ? UINT64_MAX / 3 : 0) &&
                pressure_mem[80][0] == UINT64_MAX && pressure_mem[82][0] == 1 &&
                pressure_mem[0][0] == 1 && pressure_mem[0][1] == 0 && pressure_mem[0][2] == 73 &&
                pressure_mem[0][3] == 0x11223344 && pressure_mem[0][4] == 0 && !excpt.triggered;
            if (!architectural_ok) printf("BACKEND_REDIRECT_FOLLOWUP_FAIL row=%u a=%lx b=%lx young=%lx div=%lx markers=%lx/%lx/%lx/%lx/%lx\n",
                row, result[0], result[1], result[2], result[3], pressure_mem[0][0], pressure_mem[0][1],
                pressure_mem[0][2], pressure_mem[0][3], pressure_mem[0][4]);
        } else architectural_ok = be_it_redirect_probe((controlled ? 0x30000u : 0x10000u) | row) && !excpt.triggered;
        if (!architectural_ok) printf("BACKEND_REDIRECT_ARCH_FAIL row=%u\n", row);
        TEST_ASSERT("older redirect preserves only the selected architectural path", architectural_ok);
        TEST_ASSERT("IT checked ROB age, arrival gap, WB arbitration and commit suppression", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_redirect_age_matrix");
}

bool ai_micro_backend_it_rob_commit_pressure(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_begin(690,0)) return false;
    TEST_SETUP_EXCEPT();
    TEST_ASSERT("mixed integer/FP/vector/memory work retires with exact results", be_it_pressure(6,32) && !excpt.triggered);
    TEST_ASSERT("IT observed maximum-width consecutive enqueue/commit with no interrupt", be_it_finish());
    TEST_END("ai_micro_backend_it_rob_commit_pressure");
}

bool ai_micro_backend_it_backpressure_release(void)
{
    TEST_START();
    be_it_enable();
    extern uint64_t be_pressure_down(volatile uint64_t *, uint64_t *, unsigned);
    extern uint64_t be_pressure_up(volatile uint64_t *, uint64_t *, unsigned);
    extern uint64_t be_pressure_middle(volatile uint64_t *, uint64_t *, unsigned);
    extern uint64_t be_pressure_rob(volatile uint64_t *, uint64_t *, unsigned);
    extern uint64_t be_pressure_load(volatile uint64_t *, uint64_t *, unsigned);
    extern uint64_t be_pressure_repeat(volatile uint64_t *, uint64_t *, unsigned);
    extern char be_pressure_down_old[], be_pressure_down_young[];
    extern char be_pressure_up_old[], be_pressure_up_young[], be_pressure_up_ic[];
    extern char be_pressure_middle_old[], be_pressure_middle_young[], be_pressure_middle_alu[];
    extern char be_pressure_rob_old[], be_pressure_rob_young[], be_pressure_rob_prefix[];
    extern char be_pressure_load_old[], be_pressure_load_young[];
    extern char be_pressure_repeat_old[], be_pressure_repeat_young[];
    extern char be_pressure_repeat_branch[], be_pressure_repeat_wrong[];
    extern char be_pressure_repeat_targets[];
    uint64_t (*const run[6])(volatile uint64_t *,uint64_t *,unsigned) = {
        be_pressure_down,be_pressure_up,be_pressure_middle,be_pressure_rob,be_pressure_load,be_pressure_repeat};
    const uintptr_t old_pc[6] = {(uintptr_t)be_pressure_down_old,(uintptr_t)be_pressure_up_old,
        (uintptr_t)be_pressure_middle_old,(uintptr_t)be_pressure_rob_old,
        (uintptr_t)be_pressure_load_old,(uintptr_t)be_pressure_repeat_old};
    const uintptr_t young_pc[6] = {(uintptr_t)be_pressure_down_young,(uintptr_t)be_pressure_up_young,
        (uintptr_t)be_pressure_middle_young,(uintptr_t)be_pressure_rob_young,
        (uintptr_t)be_pressure_load_young,(uintptr_t)be_pressure_repeat_young};
    uint64_t plan[8] __attribute__((aligned(64)));
    uint64_t result[8] __attribute__((aligned(64)));
    for (unsigned row = 884; row <= 889; ++row) {
        for (unsigned iteration = 0; iteration < 8; ++iteration) {
            const unsigned kind = row - 884;
            pressure_mem[0][0] = 101;
            pressure_mem[1][0] = 307;
            pressure_mem[2][0] = 41;
            if (row == 889) {
                /* Train the same indirect PC to the old target, then choose a
                 * distinct legal target for each measured window. */
                pressure_mem[0][1] = (uintptr_t)be_pressure_repeat_wrong;
                for (unsigned train=0; train<4; ++train)
                    be_pressure_repeat(pressure_mem[0],result,1);
                pressure_mem[0][1] = (uintptr_t)be_pressure_repeat_targets + 64U*iteration;
            }
            for (unsigned word=0;word<8;word++) result[word]=0;
            result[4]=0x123456789abcdef0ULL;
            plan[0]=young_pc[kind];
            plan[1]=row==886 ? (uintptr_t)be_pressure_middle_alu :
                row==887 ? (uintptr_t)be_pressure_rob_prefix : 0;
            plan[2]=row==889 ? (uintptr_t)be_pressure_repeat_branch : 0;
            plan[3]=row==889 ? (uintptr_t)be_pressure_repeat_wrong : 0;
            plan[4]=row==885 ? (uintptr_t)be_pressure_up_ic : 0;
            plan[5]=(uintptr_t)&pressure_mem[1][0];
            plan[6]=0x5052455353555245ULL;
            plan[7]=iteration;
            backend_it_control.target_pc=old_pc[kind];
            backend_it_control.target_addr = (uintptr_t)pressure_mem;
            backend_it_control.auxiliary = (uintptr_t)plan;
            if (row==885) asm volatile("cbo.flush 0(%0)" :: "r"(be_pressure_up_ic) : "memory");
            asm volatile("cbo.flush 0(%0)\ncbo.flush 0(%1)\ncbo.flush 0(%2)\nfence rw,rw\n"
                :: "r"(plan),"r"(&pressure_mem[0][0]),"r"(&pressure_mem[1][0]) : "memory");
            if (!be_it_begin_scenario(row,0x8001U+kind,iteration)) return false;
            TEST_SETUP_EXCEPT();
            if (row==885) asm volatile("fence.i" ::: "memory");
            uint64_t got=run[kind](pressure_mem[0],result,0);
            uint64_t expected_aux=0;
            if (row==886 || row==888) {
                expected_aux=UINT64_MAX;
                for (unsigned i=0;i<(row==886 ? 8U : 16U);i++) expected_aux/=3;
                if (row==886) expected_aux*=3;
            } else if (row==887) expected_aux=9;
            TEST_ASSERT("released transactions preserve both values and dependent result",
                !excpt.triggered && result[0]==101 && result[1]==307 && got==408 && result[2]==expected_aux);
            TEST_ASSERT("wrong-path store is suppressed and adjacent guard is unchanged",
                result[3]==0 && result[4]==0x123456789abcdef0ULL &&
                pressure_mem[0][0]==101 && pressure_mem[1][0]==307 && pressure_mem[2][0]==41);
            TEST_ASSERT("IT observed the row-specific release order and final drain", be_it_finish());
        }
    }
    TEST_END("ai_micro_backend_it_backpressure_release");
}
