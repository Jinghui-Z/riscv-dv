#include "backend_it_0911.h"

/* nhv5_1_ap; row769 clarified as same-fault-epoch output isolation.
 * RTL must see a target handshake, the target reaching IBuffer, and a
 * different raw-valid request without the fault. Ordering is unrestricted.
 * Raw-valid isolation does not prove that the other request survived flush.
 * This scan neither requires nor manufactures a BPU s3 correction.
 * Register/build/run integration belongs to the main thread.
 */
#pragma GCC optimize ("no-tree-vectorize")

#define FQP_I0       0x0000003ffffffffcULL
#define FQP_I1       0x0000004000000000ULL
#define FQP_PAGE     4096U
#define FQP_SHAPES   3U
#define FQP_PADS     8U
#define FQP_TRIALS   (FQP_SHAPES * FQP_PADS)
#define FQP_GUARD0   0x72f4a3915be806cdULL
#define FQP_GUARD1   0x8c05e2b7193ad46fULL

extern unsigned real_priv;
static pte_t fqp_root[512] __attribute__((aligned(FQP_PAGE)));
static pte_t fqp_l1[512] __attribute__((aligned(FQP_PAGE)));
static pte_t fqp_l0[512] __attribute__((aligned(FQP_PAGE)));
static uint32_t fqp_code[1024] __attribute__((aligned(FQP_PAGE)));

struct fqp_event { uint64_t cause, epc, tval, status; };
_Static_assert(sizeof(struct fqp_event) == 32, "trap event stride");
volatile struct fqp_event be_fqp_events[2];
volatile uint64_t be_fqp_count, be_fqp_resume;
volatile uint64_t be_fqp_probe[3] __attribute__((aligned(64)));
extern void be_fqp_enter(uintptr_t entry);
extern void be_fqp_trap(void);
extern void be_fqp_clean_probe(void);
extern void be_fqp_clean_ecall(void);

asm(
    ".pushsection .text.be_fqp_helpers,\"ax\",@progbits\n"
    BE_ASM_BEGIN
    ".balign 4\n.global be_fqp_enter\nbe_fqp_enter:\n"
    "lla t0,.Lbe_fqp_return\nla t1,be_fqp_resume\nsd t0,0(t1)\n"
    "csrw mepc,a0\n"
    "li t0,0x21888\ncsrc mstatus,t0\n"
    "li t0,0x800\ncsrs mstatus,t0\nmret\n"
    ".Lbe_fqp_return:\nret\n"

    ".balign 4\n.global be_fqp_trap\nbe_fqp_trap:\n"
    "addi sp,sp,-64\n"
    "sd t0,0(sp)\nsd t1,8(sp)\nsd t2,16(sp)\n"
    "sd t3,24(sp)\nsd t4,32(sp)\nsd t5,40(sp)\nsd t6,48(sp)\n"
    "la t0,be_fqp_count\nld t1,0(t0)\nli t2,2\n"
    "bgeu t1,t2,.Lbe_fqp_count_only\n"
    "slli t2,t1,5\nla t3,be_fqp_events\nadd t3,t3,t2\n"
    "csrr t2,mcause\nsd t2,0(t3)\n"
    "csrr t2,mepc\nsd t2,8(t3)\n"
    "csrr t2,mtval\nsd t2,16(t3)\n"
    "csrr t2,mstatus\nsd t2,24(t3)\n"
    ".Lbe_fqp_count_only:\naddi t1,t1,1\nsd t1,0(t0)\n"
    /* Never increment a noncanonical EPC. Every trap returns to the
     * caller's canonical M continuation, including unexpected traps. */
    "la t0,be_fqp_resume\nld t0,0(t0)\ncsrw mepc,t0\n"
    "li t0,0x2188a\ncsrc mstatus,t0\n"
    "li t0,0x1800\ncsrs mstatus,t0\n"
    "ld t0,0(sp)\nld t1,8(sp)\nld t2,16(sp)\n"
    "ld t3,24(sp)\nld t4,32(sp)\nld t5,40(sp)\nld t6,48(sp)\n"
    "addi sp,sp,64\nmret\n"

    ".balign 64\n.global be_fqp_clean_probe\nbe_fqp_clean_probe:\n"
    "la t0,be_fqp_probe\nld t1,8(t0)\naddi t1,t1,1\nsd t1,8(t0)\n"
    ".global be_fqp_clean_ecall\nbe_fqp_clean_ecall:\necall\n"
    BE_ASM_END
    ".popsection\n");

struct fqp_saved {
    uint64_t mstatus, mie, mideleg, medeleg, mtvec, satp, pmpcfg0, pmpaddr0;
};

static void fqp_prepare(struct fqp_saved *s)
{
    be_it_enable();
    s->mstatus = CSRR(CSR_MSTATUS);
    s->mie = CSRR(CSR_MIE);
    s->mideleg = CSRR(CSR_MIDELEG);
    s->medeleg = CSRR(CSR_MEDELEG);
    s->mtvec = CSRR(CSR_MTVEC);
    s->satp = CSRR(CSR_SATP);
    s->pmpcfg0 = CSRR(CSR_PMPCFG0);
    s->pmpaddr0 = CSRR(CSR_PMPADDR0);
    CSRC(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE | MSTATUS_MPRV | MSTATUS_TVM);
    CSRW(CSR_MIE, 0);
    CSRW(CSR_MEDELEG, 0);
    CSRW(CSR_MIDELEG, 2);
    CSRW(CSR_SATP, 0);
    sfence_vma();
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, 0x0f);
    CSRW(CSR_MTVEC, (uintptr_t)be_fqp_trap);
    for (unsigned i = 0; i < 512; ++i)
        fqp_root[i] = fqp_l1[i] = fqp_l0[i] = 0;
    for (unsigned i = 0; i < 4; ++i)
        fqp_root[i] = ((uint64_t)i << 28) | PTE_V | PTE_AD | PTE_RWX;
    fqp_root[255] = ((uintptr_t)fqp_l1 >> 2) | PTE_V;
    fqp_l1[511] = ((uintptr_t)fqp_l0 >> 2) | PTE_V;
    fqp_l0[511] = ((uintptr_t)fqp_code >> 2) | PTE_V | PTE_AD | PTE_R | PTE_X;
    for (unsigned i = 0; i < 1024; ++i) fqp_code[i] = 0x00000013U;
    fqp_code[1023] = 0x10401073U; /* csrw sie,zero: ordinary flush at I0. */
    be_fqp_probe[0] = FQP_GUARD0;
    be_fqp_probe[1] = 0;
    be_fqp_probe[2] = FQP_GUARD1;
    CSRW(CSR_SATP, (8ULL << 60) | ((uintptr_t)fqp_root >> 12));
    sfence_vma();
}

static void fqp_restore(const struct fqp_saved *s)
{
    curr_priv = real_priv = PRIV_M;
    CSRW(CSR_MIE, 0);
    CSRW(CSR_SATP, 0);
    sfence_vma();
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, s->pmpaddr0);
    CSRW(CSR_PMPCFG0, s->pmpcfg0);
    CSRW(CSR_MTVEC, s->mtvec);
    CSRW(CSR_MEDELEG, s->medeleg);
    CSRW(CSR_MIDELEG, s->mideleg);
    CSRW(CSR_SATP, s->satp);
    sfence_vma();
    CSRW(CSR_MIE, s->mie);
    CSRW(CSR_MSTATUS, s->mstatus);
}

static void fqp_enter(uintptr_t entry)
{
    curr_priv = real_priv = PRIV_HS;
    be_fqp_enter(entry);
    curr_priv = real_priv = PRIV_M;
    asm volatile("fence rw,rw" ::: "memory");
}

static bool fqp_event(unsigned n, uint64_t cause, uintptr_t pc, uintptr_t tval)
{
    return be_fqp_count > n && be_fqp_events[n].cause == cause &&
        be_fqp_events[n].epc == pc && be_fqp_events[n].tval == tval &&
        ((be_fqp_events[n].status >> 11) & 3) == 1;
}

bool ai_micro_backend_it_ftq_fault_pointer_isolation(void)
{
    TEST_START();
    struct fqp_saved saved;
    fqp_prepare(&saved);
    backend_it_control.target_pc = FQP_I0;
    backend_it_control.target_addr = FQP_I1;
    backend_it_control.auxiliary = FQP_TRIALS;
    if (!be_it_begin(769, 0)) {
        fqp_restore(&saved);
        return false;
    }
    bool precise = true, recovered = true;
    unsigned attempts = 0;
    for (unsigned shape = 0; shape < FQP_SHAPES; ++shape) {
        /* Natural fetch history: straight-line, not-taken branches, JAL+4.
         * The pads vary by four instructions; no assumed FTQ pointer stride. */
        uint32_t pad = shape == 0 ? 0x00000013U :
                       shape == 1 ? 0x00001263U : 0x0040006fU;
        for (unsigned i = 0; i < 32; ++i) fqp_code[1022 - i] = pad;
        asm volatile("fence rw,rw\nfence.i" ::: "memory");
        for (unsigned span = 0; span < FQP_PADS; ++span) {
            be_fqp_count = 0;
            for (unsigned i = 0; i < 2; ++i) {
                be_fqp_events[i].cause = be_fqp_events[i].epc = 0;
                be_fqp_events[i].tval = be_fqp_events[i].status = 0;
            }
            CSRW(CSR_SIE, 2);
            fqp_enter(FQP_I0 - 16U * span);
            precise &= be_fqp_count == 1 &&
                fqp_event(0, CAUSE_IPF, FQP_I1, FQP_I1) && CSRR(CSR_SIE) == 0;
            fqp_enter((uintptr_t)be_fqp_clean_probe);
            ++attempts;
            recovered &= be_fqp_count == 2 &&
                fqp_event(1, CAUSE_ECS, (uintptr_t)be_fqp_clean_ecall, 0) &&
                be_fqp_probe[1] == attempts && be_fqp_probe[0] == FQP_GUARD0 &&
                be_fqp_probe[2] == FQP_GUARD1;
            if (!precise || !recovered) break;
        }
        if (!precise || !recovered) break;
    }
    /* Close even a failed software trial; cleanup never depends on a hit. */
    bool observed = be_it_finish();
    fqp_restore(&saved);
    TEST_ASSERT("row769: I0 executes; exactly one IPF names I1, never an adjacent PC",
        precise && attempts == FQP_TRIALS,
        "attempts=%u count=%lu cause=%lu epc=%lx tval=%lx", attempts,
        be_fqp_count, be_fqp_events[0].cause, be_fqp_events[0].epc,
        be_fqp_events[0].tval);
    TEST_ASSERT("row769: canonical recovery fetch executes once and leaves both guards intact",
        recovered && attempts == FQP_TRIALS);
    TEST_ASSERT("row769: each IFU/PF cover joins target delivery and raw output isolation in one fault epoch",
        observed);
    TEST_END("ai_micro_backend_it_ftq_fault_pointer_isolation");
}
