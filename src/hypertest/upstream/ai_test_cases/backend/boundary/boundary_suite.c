#include "boundary_suite.h"
#include <stddef.h>

/* nhv5_1_ap. Plain cacheable DRAM; no guest or Device/NC accesses.
 * RTL windows are accepted only after an external monitor acknowledgement.
 * BOUNDARY_ARCH_SMOKE removes that handshake, never the software oracle. */
#pragma GCC optimize ("no-tree-vectorize", "no-tree-loop-distribute-patterns")
#define CODE_VA 0x40000000ULL
#define HANDLER_VA (CODE_VA + 0x3000)
#define DATA_VA 0x50000000ULL
#define SEED 0x1122334455667788ULL
#define VALUE 0x2468ULL
#define ECALL 0x00000073U
#define NOP 0x00000013U

volatile struct boundary_control boundary_control __attribute__((aligned(64)));
volatile struct boundary_context boundary_context __attribute__((aligned(64)));
_Static_assert(offsetof(struct boundary_context, entry) == 96, "entry ABI");
_Static_assert(offsetof(struct boundary_context, sret) == 152, "sret ABI");
/* Back these scratch pages with ELF data: the reset stub need not clear a
 * large BSS before every standalone test. Each used table is cleared below. */
static pte_t tables[2][8][512] __attribute__((aligned(4096))) = {{{1}}};
static unsigned used[2];
static uint32_t code[4][1024] __attribute__((aligned(4096))) = {{1}};
static volatile uint64_t data[3][512] __attribute__((aligned(4096))) = {{1}};

struct saved_state {
    uint64_t status, satp, mtvec, stvec, mscratch, mie, mideleg, medeleg;
    uint64_t pmpcfg, pmpaddr[3], pending;
};

static uint32_t insn_i(unsigned op, unsigned rd, unsigned f, unsigned rs, int imm)
{
    return ((uint32_t)imm & 4095) << 20 | rs << 15 | f << 12 | rd << 7 | op;
}
static uint32_t insn_s(unsigned rs, unsigned base, int imm)
{
    return (((uint32_t)imm >> 5) & 127) << 25 | rs << 20 | base << 15 |
        3 << 12 | ((uint32_t)imm & 31) << 7 | 0x23;
}
static uint32_t insn_b(unsigned rs, unsigned rt, unsigned f, int off)
{
    unsigned u = (unsigned)off;
    return (u >> 12 & 1) << 31 | (u >> 5 & 63) << 25 | rt << 20 |
        rs << 15 | f << 12 | (u >> 1 & 15) << 8 | (u >> 11 & 1) << 7 | 0x63;
}
static uint32_t insn_j(unsigned rd, int off)
{
    unsigned u = (unsigned)off;
    return (u >> 20 & 1) << 31 | (u >> 1 & 1023) << 21 |
        (u >> 11 & 1) << 20 | (u >> 12 & 255) << 12 | rd << 7 | 0x6f;
}
static uint32_t csr(unsigned rd, unsigned addr, unsigned f, unsigned rs)
{
    return addr << 20 | rs << 15 | f << 12 | rd << 7 | 0x73;
}
static uint32_t addi(unsigned rd, unsigned rs, int imm)
{
    return insn_i(0x13,rd,0,rs,imm);
}
static uint32_t encode_ld(unsigned rd, unsigned rs, int off)
{
    return insn_i(3,rd,3,rs,off);
}

static pte_t leaf(uintptr_t pa, unsigned perm)
{
    return (pa >> 2) | PTE_V | PTE_AD | perm;
}
static void clear_table(pte_t *pt)
{
    for (unsigned i=0;i<512;++i) pt[i]=0;
}
static void map(unsigned root, unsigned levels, uintptr_t va, uintptr_t pa, unsigned perm)
{
    pte_t *pt = tables[root][0];
    for (unsigned l = levels - 1; l; --l) {
        unsigned ix = va >> (12 + 9*l) & 511;
        if (!(pt[ix] & PTE_V)) {
            pte_t *next = tables[root][++used[root]];
            clear_table(next);
            pt[ix] = ((uintptr_t)next >> 2) | PTE_V;
        }
        pt = (pte_t *)(uintptr_t)((pt[ix] >> 10) << 12);
    }
    pt[va >> 12 & 511] = leaf(pa,perm);
}
static uint64_t satp(unsigned root, unsigned levels)
{
    return (uint64_t)(levels == 4 ? 9 : 8) << 60 |
        (uint64_t)(root + 1) << 44 | (uintptr_t)tables[root][0] >> 12;
}
static void root_init(unsigned r, unsigned levels)
{
    used[r] = 0;
    clear_table(tables[r][0]);
    /* Identity map 0..4GiB with 1GiB leaves for setup/recovery code only. */
    pte_t *pt = tables[r][0];
    if (levels == 4) {
        pt[0] = ((uintptr_t)tables[r][++used[r]] >> 2) | PTE_V;
        pt = tables[r][used[r]];
        clear_table(pt);
    }
    for (unsigned i=0;i<4;++i) pt[i] = leaf((uintptr_t)i << 30,PTE_RWX);
    /* Split the CODE/DATA 1GiB window instead of treating a leaf as a table. */
    pt[1] = 0;
    for (unsigned i=0;i<4;++i) map(r,levels,CODE_VA+i*4096,(uintptr_t)code[i],PTE_R|PTE_X);
    map(r,levels,DATA_VA,(uintptr_t)data[0],PTE_R|PTE_W);
    map(r,levels,DATA_VA+4096,(uintptr_t)data[2],PTE_R|PTE_W);
}
static void save(struct saved_state *s)
{
    goto_priv(PRIV_M);
    s->status=CSRR(CSR_MSTATUS); s->satp=CSRR(CSR_SATP);
    s->mtvec=CSRR(CSR_MTVEC); s->stvec=CSRR(CSR_STVEC);
    s->mscratch=CSRR(CSR_MSCRATCH); s->mie=CSRR(CSR_MIE);
    s->mideleg=CSRR(CSR_MIDELEG); s->medeleg=CSRR(CSR_MEDELEG);
    s->pmpcfg=CSRR(CSR_PMPCFG0); s->pmpaddr[0]=CSRR(CSR_PMPADDR0);
    s->pmpaddr[1]=CSRR(CSR_PMPADDR1); s->pmpaddr[2]=CSRR(CSR_PMPADDR2);
    s->pending=CSRR(CSR_MIP)&2;
    CSRC(CSR_MSTATUS,MSTATUS_MIE|MSTATUS_SIE|MSTATUS_MPRV|MSTATUS_TVM|MSTATUS_MXR|MSTATUS_SUM);
    CSRW(CSR_MIE,0); CSRC(CSR_MIP,2); CSRW(CSR_MEDELEG,0); CSRW(CSR_MIDELEG,0);
    CSRW(CSR_SATP,0); sfence_vma();
    CSRW(CSR_PMPCFG0,0); CSRW(CSR_PMPADDR0,UINT64_MAX); CSRW(CSR_PMPCFG0,0x0f);
    CSRW(CSR_MTVEC,(uintptr_t)boundary_mtrap);
}
static void restore(const struct saved_state *s)
{
    curr_priv=PRIV_M;
    extern unsigned real_priv;
    real_priv=PRIV_M;
    CSRW(CSR_MIE,0); CSRC(CSR_MIP,2);
    CSRC(CSR_MSTATUS,MSTATUS_MIE|MSTATUS_MPRV);
    CSRW(CSR_SATP,0); sfence_vma();
    CSRW(CSR_PMPCFG0,0);
    CSRW(CSR_PMPADDR0,s->pmpaddr[0]); CSRW(CSR_PMPADDR1,s->pmpaddr[1]); CSRW(CSR_PMPADDR2,s->pmpaddr[2]);
    CSRW(CSR_PMPCFG0,s->pmpcfg);
    CSRW(CSR_MTVEC,s->mtvec); CSRW(CSR_STVEC,s->stvec); CSRW(CSR_MSCRATCH,s->mscratch);
    CSRW(CSR_MEDELEG,s->medeleg); CSRW(CSR_MIDELEG,s->mideleg);
    CSRS(CSR_MIP,s->pending); CSRW(CSR_SATP,s->satp); sfence_vma();
    CSRW(CSR_MIE,s->mie); CSRW(CSR_MSTATUS,s->status);
}
static void deny(uintptr_t pa, unsigned permission)
{
    /* Entry 0 denies one 4KiB page; entry 1 allows the remaining address space. */
    CSRW(CSR_PMPCFG0,0);
    CSRW(CSR_PMPADDR0,(pa >> 2) | 511);
    CSRW(CSR_PMPADDR1,UINT64_MAX);
    /* PTE R/W/X occupy bits 1/2/3; PMP R/W/X occupy bits 0/1/2. */
    CSRW(CSR_PMPCFG0,(0x18|(permission >> 1)) | 0x1f00);
}
static void publish(void)
{
    asm volatile("cbo.flush 0(%0)\nfence rw,rw" :: "r"(&boundary_control) : "memory");
}
static bool acknowledge(unsigned phase)
{
#ifdef BOUNDARY_ARCH_SMOKE
    (void)phase;
    return true;
#else
    for (unsigned i=0;i<100000;++i) {
        asm volatile("cbo.inval 0(%0)\nfence rw,rw" :: "r"(&boundary_control.ready) : "memory");
        uint64_t observed=phase==3 ? boundary_control.done :
            phase==2 ? boundary_control.reserved[0] : boundary_control.ready;
        if (observed == boundary_control.epoch)
            return !boundary_control.errors;
    }
    printf("\nBOUNDARY missing monitor acknowledgement point=%lu phase=%lu\n",
        boundary_control.point,boundary_control.phase);
    return false;
#endif
}

bool boundary_run(unsigned point, bool monitor_required)
{
    struct saved_state s;
    save(&s);
    for (unsigned p=0;p<4;++p)
        for (unsigned i=0;i<64;++i) code[p][i]=NOP;
    for (unsigned p=0;p<3;++p)
        for (unsigned i=0;i<2;++i) data[p][i]=SEED+p;
    memset((void *)&boundary_context,0,sizeof(boundary_context));
    if (point<901) root_init(0,3);
    uint64_t entry=CODE_VA, target=CODE_VA, origin=0, memory_pc=0;
    uint64_t expected_cause=9, expected_pc=0, expected_tval=0;
    uint64_t expected_result=42, expected_data=SEED, expected_scause=UINT64_MAX;
    unsigned out=0;
    bool result_checked=true, link_checked=false, irq_return=false;
    boundary_context.data=DATA_VA; boundary_context.value=VALUE;
    boundary_context.jump=0x0000004000000000ULL;
    boundary_context.other=DATA_VA+4096;
    boundary_context.privilege=1;
    code[0][0]=addi(8,0,42); code[0][1]=ECALL;
    code[3][0]=addi(8,0,42); code[3][1]=ECALL;

    if (point>=301 && point<=406) {
        /* A fresh forward taken branch avoids loop-exit prediction. Resolve
         * its condition late, after the sequential wrong path was fetched. */
        out=0;
        if (point==401) code[0][out++]=insn_s(12,11,0);
        code[0][out++]=addi(5,0,0); code[0][out++]=addi(6,0,1);
        for (unsigned i=0;i<32;++i) code[0][out++]=0x0262d2b3;
        origin=CODE_VA+out*4;
        code[0][out++]=insn_b(5,0,0,8);
        code[0][out++]=0xffffffffU;
        target=CODE_VA+out*4;
        code[0][out]=addi(8,0,42); code[0][out+1]=ECALL;
        if (point==302) {
            out=0;
            code[0][out++]=addi(5,11,0);
            code[0][out++]=addi(6,0,1);
            for (unsigned i=0;i<32;++i) code[0][out++]=0x0262d2b3; /* divu t0,t0,t1 */
            code[0][out++]=insn_s(12,5,0);
            target=CODE_VA+out*4; origin=CODE_VA+(out-1)*4;
            memory_pc=target;
            code[0][out++]=encode_ld(8,11,0); code[0][out]=ECALL;
            expected_data=expected_result=VALUE;
        } else if (point==303) {
            code[0][out]=0xffffffffU;
            expected_cause=2; expected_pc=target; result_checked=false;
        } else if (point==304 || point==305) {
            /* The indirect redirect, rather than its preceding branch, is checked. */
            code[0][out]=insn_i(0x67,0,0,14,0);
            origin=target;
            target=point==304 ? boundary_context.jump : CODE_VA+4096;
            boundary_context.jump=target;
            if (point==305) deny((uintptr_t)code[1],PTE_R|PTE_W);
            expected_cause=point==304 ? 12 : 1;
            expected_pc=expected_tval=target; result_checked=false;
        } else if (point==306) {
            /* Resolve late, with the sequential wrong path crossing into page 1. */
            entry=CODE_VA+0xf00;
            for (unsigned i=960;i<1024;++i) code[0][i]=NOP;
            code[0][960]=addi(5,0,0); code[0][961]=addi(6,0,1);
            for (unsigned i=962;i<1000;++i) code[0][i]=0x0262d2b3;
            /* Leave a full fetch block before the page boundary, so holding
             * the wrong-page refill cannot prevent the branch being decoded. */
            origin=CODE_VA+4000; target=CODE_VA+0x1800;
            code[0][1000]=insn_b(5,0,0,2144);
            code[1][512]=addi(8,0,42); code[1][513]=ECALL;
            code[1][0]=0xffffffffU;
        } else if (point>=401) {
            if (point==401) {
                /* Older store survives the branch flush. */
                expected_data=VALUE;
            }
            code[0][out]=point==402 || point==405 ? insn_s(12,11,0) :
                point==403 ? 0x00c5b42fU : encode_ld(8,11,0);
            code[0][out+1]=ECALL;
            memory_pc=target;
            expected_result=expected_data;
            if (point==402) {expected_data=VALUE; result_checked=false;}
            if (point==403) expected_data=SEED+VALUE;
            if (point==404 || point==405) {
                map(0,3,DATA_VA,(uintptr_t)data[0],point==404 ? PTE_X : PTE_R);
                expected_cause=point==404 ? 13 : 15;
                expected_pc=target; expected_tval=DATA_VA; result_checked=false;
            }
            if (point==406) {
                /* Training stores use another page; only the final wrong-path
                 * store addresses the sentinel checked after the flush. */
                unsigned n=0;
                code[0][n++]=addi(7,0,32);
                code[0][n++]=addi(5,15,0);
                code[0][n++]=addi(7,7,-1);
                code[0][n++]=insn_b(7,0,1,8);
                code[0][n++]=addi(5,11,0);
                code[0][n++]=addi(6,7,0);
                code[0][n++]=addi(28,0,1);
                for (unsigned i=0;i<16;++i)
                    code[0][n++]=0x03c35333U; /* divu t1,t1,t3 */
                origin=CODE_VA+n*4;
                code[0][n++]=insn_b(6,0,0,12);
                code[0][n++]=insn_s(12,5,0);
                code[0][n]=insn_j(0,4-(int)n*4); ++n;
                target=CODE_VA+n*4; memory_pc=target;
                code[0][n++]=encode_ld(8,11,0); code[0][n]=ECALL;
                expected_result=SEED;
            }
        }
    } else if (point>=501 && point<=606) {
        /* U ECALL delegates to S; nested faults and S ECALL recover in M. */
        map(0,3,CODE_VA,(uintptr_t)code[0],PTE_R|PTE_X|PTE_U);
        code[0][0]=ECALL; boundary_context.privilege=0;
        CSRW(CSR_MEDELEG,1ULL<<8); CSRW(CSR_STVEC,HANDLER_VA);
        target=HANDLER_VA;
        if (point==502) {
            code[3][0]=insn_j(9,8); code[3][1]=0xffffffffU;
            code[3][2]=addi(8,0,42); code[3][3]=ECALL; link_checked=true;
        }
        if (point==503) {code[3][0]=0xffffffffU; expected_cause=2; result_checked=false;}
        if (point==504) {deny((uintptr_t)code[3],PTE_R|PTE_W); expected_cause=1; result_checked=false;}
        if (point==505) {
            map(0,3,HANDLER_VA,(uintptr_t)code[3],PTE_R);
            expected_cause=12; result_checked=false;
        }
        if (point==506 || point==606) {
            /* Stay in S with SIE=0 before EBREAK: enabled SSIP is pending
             * at the actual handler entry, without interrupting U setup. */
            map(0,3,CODE_VA,(uintptr_t)code[0],PTE_R|PTE_X);
            boundary_context.privilege=1;
            code[0][0]=0x00100073U;
            CSRW(CSR_MEDELEG,1ULL<<3);
            CSRW(CSR_MIDELEG,2); CSRW(CSR_MIE,2); CSRS(CSR_MIP,2);
        }
        if (point>=601) {
            out=0;
            memory_pc=HANDLER_VA+out*4; target=memory_pc;
            code[3][out++]=point==602 || point==605 ? insn_s(12,11,0) :
                encode_ld(8,11,point==603 ? 1 : 0);
            code[3][out]=ECALL;
            expected_result=SEED;
            if (point==602) {expected_data=VALUE; result_checked=false;}
            if (point==603) expected_result=(SEED>>8)|((SEED&255)<<56);
            if (point==604 || point==606) {
                map(0,3,DATA_VA,(uintptr_t)data[0],PTE_X);
                expected_cause=13; expected_tval=DATA_VA; result_checked=false;
            }
            if (point==605) {
                deny((uintptr_t)data[0],PTE_R);
                expected_cause=7; expected_tval=DATA_VA; result_checked=false;
            }
        }
        if (expected_cause!=9) {expected_pc=target; if (!expected_tval && expected_cause!=2) expected_tval=target;}
        expected_scause=point==506 || point==606 ? 3 : 8;
    } else if (point==703 || point==704 || point==705) {
        if (point==703) {code[0][0]=0xffffffffU; expected_cause=2;}
        if (point==704) {entry=target=0x0000004000000000ULL; expected_cause=12; expected_tval=target;}
        if (point==705) {deny((uintptr_t)code[0],PTE_R|PTE_W); expected_cause=1; expected_tval=target;}
        expected_pc=target; result_checked=false;
    } else if (point==706 || point==806) {
        CSRW(CSR_MIDELEG,2); CSRW(CSR_MIE,2); CSRS(CSR_MIP,2);
        CSRW(CSR_STVEC,HANDLER_VA+4); /* interrupt vector shares its first CSR */
        code[3][1]=csr(18,0x142,2,0);
        code[3][2]=csr(19,0x141,2,0);
        code[3][3]=csr(0,0x144,7,2); /* csrci sip,2 */
        code[3][4]=addi(22,22,1);
        code[3][5]=point==806 ? 0x10200073U : ECALL;
        boundary_context.sret=1; expected_scause=0x8000000000000001ULL;
        if (point==706) expected_result=85;
        else {
            code[0][1]=insn_s(12,11,0); code[0][2]=ECALL;
            expected_data=VALUE; memory_pc=CODE_VA+4; irq_return=true;
        }
    } else if (point==801 || point==804 || point==805) {
        code[0][0]=point==805 ? insn_s(12,11,0) : encode_ld(8,11,0);
        code[0][1]=ECALL; memory_pc=target; expected_result=SEED;
        if (point==801) {
            /* Set MPRV at the last possible point in boundary_enter, with a
             * CSR-only sequence; C and its stack never run under MPRV. */
            boundary_context.sret=2;
        } else {
            map(0,3,DATA_VA,(uintptr_t)data[0],point==804 ? PTE_X : PTE_R);
            expected_cause=point==804 ? 13 : 15; expected_pc=target;
            expected_tval=DATA_VA; result_checked=false;
        }
    } else if (point>=901) {
        unsigned oldlevels=point==904 ? 4 : 3;
        unsigned newlevels=point==904 ? 3 : 4;
        if (point==901) newlevels=3;
        root_init(0,oldlevels); root_init(1,newlevels);
        boundary_context.new_satp=satp(1,newlevels);
        if (point==901 || point==902) {
            entry=(uintptr_t)code[0];
            /* Bare predecessor and translated successor share the same VA.
             * Split its identity 1GiB mapping to install the new code page. */
            pte_t *pt=tables[1][0];
            if (newlevels==4) pt=(pte_t *)(uintptr_t)((pt[0]>>10)<<12);
            pt[entry>>30 & 511]=0;
            map(1,newlevels,entry,(uintptr_t)code[1],PTE_R|PTE_X);
        } else {
            map(1,newlevels,CODE_VA,(uintptr_t)code[1],PTE_R|PTE_X);
        }
        code[0][0]=csr(0,0x180,1,13); /* csrw satp,a3 */
        code[0][1]=addi(8,0,17); code[0][2]=ECALL;
        code[1][0]=code[0][0]; code[1][1]=addi(8,0,42); code[1][2]=ECALL;
        target=entry+4; origin=entry;
        if (point==904) {
            entry=0x0000003ffffffffcULL; target=entry+4; origin=entry;
            map(0,4,entry&~4095ULL,(uintptr_t)code[0],PTE_R|PTE_X);
            code[0][1023]=csr(0,0x180,1,13);
            expected_cause=12; expected_pc=expected_tval=target; result_checked=false;
        }
        if (point==906) {
            map(1,newlevels,CODE_VA,(uintptr_t)code[1],PTE_R);
            expected_cause=12; expected_pc=expected_tval=target; result_checked=false;
        }
        if (point>=1001) {
            code[0][1]=code[1][1]=0x12000073U; /* sfence.vma x0,x0 */
            code[1][2]=point==1002 || point==1004 ? insn_s(12,11,0) : encode_ld(8,11,0);
            code[1][3]=ECALL; memory_pc=entry+8; target=memory_pc;
            map(1,newlevels,DATA_VA,(uintptr_t)data[1],PTE_R|PTE_W);
            expected_result=SEED+1;
            if (point==1002) result_checked=false;
            if (point==1003 || point==1004) {
                map(1,newlevels,DATA_VA,(uintptr_t)data[1],point==1003 ? PTE_X : PTE_R);
                expected_cause=point==1003 ? 13 : 15;
                expected_pc=target; expected_tval=DATA_VA; result_checked=false;
            }
            if (point==1005) {
                deny((uintptr_t)data[1],0);
                expected_cause=5; expected_pc=target; expected_tval=DATA_VA; result_checked=false;
            }
        }
        CSRW(CSR_SATP,point==901 || point==902 ? 0 : satp(0,oldlevels));
    }
    if (point<901) CSRW(CSR_SATP,satp(0,3));
    sfence_vma(); asm volatile("fence rw,rw\nfence.i" ::: "memory");
    boundary_context.entry=entry;
    boundary_control.magic=0x424f554e44415259ULL;
    boundary_control.point=point; boundary_control.epoch++;
    boundary_control.target=target; boundary_control.origin=origin;
    boundary_control.memory_pc=memory_pc;
    boundary_control.wrong_pa=(uintptr_t)code[1];
    boundary_control.phase=1;
    bool ok=true;
    if (monitor_required) {
        asm volatile("cbo.flush 0(%0)\nfence rw,rw" :: "r"(&boundary_control.ready) : "memory");
        publish(); ok=acknowledge(1);
    }
    if (ok) {
        boundary_control.phase=2;
        if (monitor_required) {
            publish();
            if (!acknowledge(2)) {
                boundary_control.phase=0; publish(); restore(&s); return false;
            }
        }
        boundary_enter();
        ok=boundary_context.cause==expected_cause;
        if (expected_pc) ok &= boundary_context.epc==expected_pc;
        if (expected_tval) ok &= boundary_context.tval==expected_tval;
        if (result_checked) ok &= boundary_context.result==expected_result;
        else if (expected_cause!=9) ok &= boundary_context.result==85;
        if (link_checked) ok &= boundary_context.link==HANDLER_VA+4;
        if (expected_scause!=UINT64_MAX) ok &= boundary_context.scause==expected_scause;
        if (point==506 || point==606)
            ok &= (boundary_context.pending&2) && !(boundary_context.status&MSTATUS_SIE);
        if (point==706 || irq_return) {
            ok &= boundary_context.irq_count==1;
            /* Exact first-instruction acceptance is mandatory for this point. */
            ok &= boundary_context.sepc==entry;
        }
        ok &= data[0][0]==expected_data && data[0][1]==SEED;
        if (point==1002) ok &= data[1][0]==VALUE;
        else ok &= data[1][0]==SEED+1;
        ok &= data[1][1]==SEED+1;
        if (point>=901) ok &= CSRR(CSR_SATP)==boundary_context.new_satp;
        boundary_control.phase=3;
        if (monitor_required) {publish(); ok &= acknowledge(3);}
    }
    printf("\nBOUNDARY point=%u cause=%lx epc=%lx tval=%lx result=%lx scause=%lx sepc=%lx data=%lx/%lx status=%s\n",
        point,boundary_context.cause,boundary_context.epc,boundary_context.tval,
        boundary_context.result,boundary_context.scause,boundary_context.sepc,
        data[0][0],data[1][0],ok ? "PASS" : "FAIL");
    boundary_control.phase=0;
    if (monitor_required) publish();
    restore(&s);
    return ok;
}
