#include "backend_it_0911.h"

/* nhv5_1_ap, RTL-only coverage; ordinary cacheable DRAM and Sv39.
 * Public registration/build/monitor changes belong to the integration owner.
 * 0x5201..0x5203 are proposed stable selectors; see boundary_work/report.md.
 * A software PASS alone does not establish any backend IT witness.
 */
#pragma GCC optimize ("no-tree-vectorize")

#define BFU_I0             0x0000003ffffffffcULL
#define BFU_I1             0x0000004000000000ULL
#define BFU_DATA_VA        0x0000000200000000ULL
#define BFU_FAULT_LEAF     16U
#define BFU_FAULT_VA       (BFU_DATA_VA + BFU_FAULT_LEAF * 4096ULL)
#define BFU_YOUNG_VA       (BFU_DATA_VA + 0x200000ULL)
#define BFU_PAGE           4096U
#define BFU_WORDS          512U
#define BFU_MAX_ELEMS      32U
#define BFU_SCAN_PADS      256U
#define BFU_SC_LOAD_PF     0x5201U
#define BFU_SC_STORE_RETRY 0x5202U
#define BFU_SC_OLDER_PTW   0x5203U
#define BFU_SENTINEL       0xa55a5aa55aa55aa5ULL
#define BFU_PREFIX_MARK    0x13579bdf2468ace0ULL

extern unsigned real_priv;

static pte_t bfu_root[512] __attribute__((aligned(BFU_PAGE)));
static pte_t bfu_code_l1[512] __attribute__((aligned(BFU_PAGE)));
static pte_t bfu_code_l0[512] __attribute__((aligned(BFU_PAGE)));
static pte_t bfu_data_l1[512] __attribute__((aligned(BFU_PAGE)));
static pte_t bfu_data_l0[512] __attribute__((aligned(BFU_PAGE)));
static pte_t bfu_young_l0[512] __attribute__((aligned(BFU_PAGE)));
static uint32_t bfu_code[1024] __attribute__((aligned(BFU_PAGE)));
static volatile uint64_t bfu_data[2][BFU_WORDS]
    __attribute__((aligned(BFU_PAGE)));
static uint64_t bfu_seed[BFU_MAX_ELEMS] __attribute__((aligned(64)));
static uint64_t bfu_index[BFU_MAX_ELEMS] __attribute__((aligned(64)));
static volatile uint64_t bfu_dump[BFU_MAX_ELEMS] __attribute__((aligned(64)));

struct bfu_event {
    uint64_t cause, epc, tval, vstart, status, pending, vl, reserved;
};
_Static_assert(sizeof(struct bfu_event) == 64, "trap assembly event stride");
volatile struct bfu_event be_bfu_events[4];
volatile uint64_t be_bfu_count, be_bfu_resume, be_bfu_clean_marker;

/* Software descriptors for a passive monitor / external response agent.
 * They are not injection registers. Publish before phase=1; do not modify
 * these while the epoch is active. All fields are little-endian uint64_t.
 */
struct bfu_ptw_contract {
    uint64_t version, row, i0, old_va, young_va, old_pte_pa, young_pte_pa;
    uint64_t start, old_element, young_element, vl, sew, lmul, reserved[3];
};
_Static_assert(sizeof(struct bfu_ptw_contract) == 128, "two descriptor cache lines");
volatile struct bfu_ptw_contract be_bfu_ptw_contract
    __attribute__((aligned(64)));

extern void be_bfu_enter(uintptr_t entry, uintptr_t base, uint64_t pending);
extern void be_bfu_trap(void);
extern void be_bfu_clean_probe(void);
extern void be_bfu_clean_ecall(void);
extern const uint32_t be_bfu_load_insn, be_bfu_store_insn, be_bfu_index_insn;

asm(
    ".pushsection .text.be_bfu_helpers,\"ax\",@progbits\n"
    BE_ASM_BEGIN
    ".balign 4\n"
    ".global be_bfu_enter\n"
    "be_bfu_enter:\n"
    "mv t4,a2\n"
    "mv a2,a1\n"
    "lla t0,.Lbe_bfu_return\n"
    "la t1,be_bfu_resume\n"
    "sd t0,0(t1)\n"
    "csrw mepc,a0\n"
    "li t0,0x21888\n"
    "csrc mstatus,t0\n"
    "li t0,0x800\n"
    "csrs mstatus,t0\n"
    /* A delegated SSIP cannot interrupt M-mode setup. Enter I0 directly;
     * otherwise a scalar S-mode prologue could consume it before the VLS. */
    "beqz t4,.Lbe_bfu_no_pending\n"
    "csrsi mstatus,2\n"
    "csrsi mip,2\n"
    ".Lbe_bfu_no_pending:\n"
    "mret\n"
    ".Lbe_bfu_return:\n"
    "ret\n"

    ".balign 4\n"
    ".global be_bfu_trap\n"
    "be_bfu_trap:\n"
    "addi sp,sp,-64\n"
    "sd t0,0(sp)\nsd t1,8(sp)\nsd t2,16(sp)\n"
    "sd t3,24(sp)\nsd t4,32(sp)\nsd t5,40(sp)\nsd t6,48(sp)\n"
    "la t0,be_bfu_count\n"
    "ld t1,0(t0)\n"
    "li t2,4\n"
    "bgeu t1,t2,.Lbe_bfu_count_only\n"
    "slli t2,t1,6\n"
    "la t3,be_bfu_events\n"
    "add t3,t3,t2\n"
    "csrr t2,mcause\nsd t2,0(t3)\n"
    "csrr t2,mepc\nsd t2,8(t3)\n"
    "csrr t2,mtval\nsd t2,16(t3)\n"
    "csrr t2,vstart\nsd t2,24(t3)\n"
    "csrr t2,mstatus\nsd t2,32(t3)\n"
    "csrr t2,mip\nsd t2,40(t3)\n"
    "csrr t2,vl\nsd t2,48(t3)\n"
    ".Lbe_bfu_count_only:\n"
    "addi t1,t1,1\nsd t1,0(t0)\n"
    "csrci mip,2\n"
    /* Return to a known canonical M continuation, never epc+4. In row 814
     * C repairs the PTE then mret re-enters I0 with the recorded vstart. */
    "la t0,be_bfu_resume\nld t0,0(t0)\ncsrw mepc,t0\n"
    "li t0,0x2188a\ncsrc mstatus,t0\n"
    "li t0,0x1800\ncsrs mstatus,t0\n"
    "ld t0,0(sp)\nld t1,8(sp)\nld t2,16(sp)\n"
    "ld t3,24(sp)\nld t4,32(sp)\nld t5,40(sp)\nld t6,48(sp)\n"
    "addi sp,sp,64\nmret\n"

    ".balign 4\n"
    ".global be_bfu_clean_probe\n"
    "be_bfu_clean_probe:\n"
    "la t0,be_bfu_clean_marker\nld t1,0(t0)\n"
    "addi t1,t1,1\nsd t1,0(t0)\n"
    ".global be_bfu_clean_ecall\n"
    "be_bfu_clean_ecall:\necall\n"
    /* These are copied as instruction words to the final four bytes of
     * the boundary page, so their encoding is checked by the assembler. */
    ".global be_bfu_load_insn\nbe_bfu_load_insn:\nvle64.v v8,(a2)\n"
    ".global be_bfu_store_insn\nbe_bfu_store_insn:\nvsoxei64.v v8,(a2),v16\n"
    ".global be_bfu_index_insn\nbe_bfu_index_insn:\nvluxei64.v v8,(a2),v16\n"
    BE_ASM_END
    ".popsection\n");

struct bfu_saved {
    uint64_t mstatus, mie, mideleg, medeleg, mtvec, stvec, satp;
    uint64_t pmpcfg0, pmpaddr0, ssip, vtype, vl, vstart;
};

static pte_t bfu_table(const pte_t *table)
{
    return ((uintptr_t)table >> 2) | PTE_V;
}

static pte_t bfu_leaf(uintptr_t pa, pte_t permission)
{
    return (pa >> 2) | PTE_V | PTE_AD | permission;
}

static void bfu_prepare(struct bfu_saved *s)
{
    be_it_enable();
    s->mstatus = CSRR(CSR_MSTATUS);
    s->mie = CSRR(CSR_MIE);
    s->mideleg = CSRR(CSR_MIDELEG);
    s->medeleg = CSRR(CSR_MEDELEG);
    s->mtvec = CSRR(CSR_MTVEC);
    s->stvec = CSRR(CSR_STVEC);
    s->satp = CSRR(CSR_SATP);
    s->pmpcfg0 = CSRR(CSR_PMPCFG0);
    s->pmpaddr0 = CSRR(CSR_PMPADDR0);
    s->ssip = CSRR(CSR_MIP) & 2;
    s->vtype = CSRR(CSR_VTYPE);
    s->vl = CSRR(CSR_VL);
    s->vstart = CSRR(CSR_VSTART);
    CSRC(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE | MSTATUS_MPRV | MSTATUS_TVM);
    CSRW(CSR_MIE, 0);
    CSRC(CSR_MIP, 2);
    CSRW(CSR_MEDELEG, 0);
    CSRW(CSR_MIDELEG, 2);
    CSRW(CSR_SATP, 0);
    sfence_vma();
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, 0x0f);
    CSRW(CSR_MTVEC, (uintptr_t)be_bfu_trap);
    /* If SSIP incorrectly passes the VLS head, the S vector executes ECALL.
     * The unexpected ECS replaces the expected PF in the log and fails. */
    CSRW(CSR_STVEC, (uintptr_t)be_bfu_clean_ecall);
    for (unsigned i = 0; i < 512; ++i) {
        bfu_root[i] = bfu_code_l1[i] = bfu_code_l0[i] = 0;
        bfu_data_l1[i] = bfu_data_l0[i] = bfu_young_l0[i] = 0;
    }
    for (unsigned i = 0; i < 4; ++i)
        bfu_root[i] = bfu_leaf((uintptr_t)i << 30, PTE_RWX);
    bfu_root[255] = bfu_table(bfu_code_l1);
    bfu_code_l1[511] = bfu_table(bfu_code_l0);
    bfu_code_l0[511] = bfu_leaf((uintptr_t)bfu_code, PTE_R | PTE_X);
    bfu_root[8] = bfu_table(bfu_data_l1);
    bfu_data_l1[0] = bfu_table(bfu_data_l0);
    bfu_data_l1[1] = bfu_table(bfu_young_l0);
    bfu_data_l0[0] = bfu_leaf((uintptr_t)bfu_data[0], PTE_R | PTE_W);
    bfu_data_l0[BFU_FAULT_LEAF - 1] = bfu_data_l0[0];
    for (unsigned i = 0; i < 1024; ++i) bfu_code[i] = 0x00000013U;
    for (unsigned p = 0; p < 2; ++p)
        for (unsigned i = 0; i < BFU_WORDS; ++i)
            bfu_data[p][i] = BFU_SENTINEL;
    be_bfu_count = be_bfu_clean_marker = 0;
}

static void bfu_restore(const struct bfu_saved *s)
{
    curr_priv = real_priv = PRIV_M;
    CSRW(CSR_MIE, 0);
    CSRC(CSR_MIP, 2);
    CSRW(CSR_SATP, 0);
    sfence_vma();
    CSRW(CSR_VSTART, 0);
    asm volatile("vsetvl zero,%0,%1" :: "r"(s->vl), "r"(s->vtype) : "memory");
    CSRW(CSR_VSTART, s->vstart);
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, s->pmpaddr0);
    CSRW(CSR_PMPCFG0, s->pmpcfg0);
    CSRW(CSR_MTVEC, s->mtvec);
    CSRW(CSR_STVEC, s->stvec);
    CSRW(CSR_MEDELEG, s->medeleg);
    CSRW(CSR_MIDELEG, s->mideleg);
    CSRS(CSR_MIP, s->ssip);
    CSRW(CSR_SATP, s->satp);
    sfence_vma();
    CSRW(CSR_MIE, s->mie);
    CSRW(CSR_MSTATUS, s->mstatus);
}

static void bfu_publish_code(uint32_t instruction)
{
    bfu_code[1023] = instruction;
    asm volatile("fence rw,rw\nfence.i" ::: "memory");
    CSRW(CSR_SATP, (8ULL << 60) | ((uintptr_t)bfu_root >> 12));
    sfence_vma();
}

static void bfu_enter(uintptr_t pc, uintptr_t base, bool pending)
{
    curr_priv = real_priv = PRIV_HS;
    be_bfu_enter(pc, base, pending);
    curr_priv = real_priv = PRIV_M;
    asm volatile("fence rw,rw" ::: "memory");
}

static bool bfu_event(unsigned n, uint64_t cause, uintptr_t pc,
    uintptr_t tval, uint64_t start, uint64_t vl)
{
    return be_bfu_count > n && be_bfu_events[n].cause == cause &&
        be_bfu_events[n].epc == pc && be_bfu_events[n].tval == tval &&
        be_bfu_events[n].vstart == start && be_bfu_events[n].vl == vl &&
        ((be_bfu_events[n].status >> 11) & 3) == 1;
}

static void bfu_descriptor(unsigned row, unsigned vl, unsigned k, unsigned j)
{
    backend_it_control.target_pc = BFU_I0;
    backend_it_control.target_addr = (uintptr_t)&bfu_data_l0[BFU_FAULT_LEAF];
    backend_it_control.auxiliary = (uintptr_t)&be_bfu_ptw_contract;
    be_bfu_ptw_contract.version = 1;
    be_bfu_ptw_contract.row = row;
    be_bfu_ptw_contract.i0 = BFU_I0;
    be_bfu_ptw_contract.old_va = BFU_FAULT_VA;
    be_bfu_ptw_contract.young_va = row == 815 ? BFU_YOUNG_VA : 0;
    be_bfu_ptw_contract.old_pte_pa = (uintptr_t)&bfu_data_l0[BFU_FAULT_LEAF];
    be_bfu_ptw_contract.young_pte_pa = row == 815 ? (uintptr_t)&bfu_young_l0[0] : 0;
    be_bfu_ptw_contract.start = 1;
    be_bfu_ptw_contract.old_element = k;
    be_bfu_ptw_contract.young_element = j;
    be_bfu_ptw_contract.vl = vl;
    be_bfu_ptw_contract.sew = 64;
    be_bfu_ptw_contract.lmul = 4;
    /* Real response delay must see current PTE contents in backing DRAM.
     * SFENCE alone does not evict a previous page-table cache-line access. */
#ifndef BACKEND_IT_ARCH_SMOKE
    asm volatile("cbo.flush 0(%0)\ncbo.flush 0(%1)\n"
        "cbo.flush 0(%2)\ncbo.flush 0(%3)\nfence rw,rw"
        :: "r"(&bfu_data_l0[BFU_FAULT_LEAF]), "r"(&bfu_young_l0[0]),
           "r"(&be_bfu_ptw_contract),
           "r"((uintptr_t)&be_bfu_ptw_contract + 64) : "memory");
#endif
}

static unsigned bfu_vector_seed(void)
{
    unsigned bytes = CSRR(CSR_VLENB);
    if (bytes < 16 || bytes > 64 || (bytes & (bytes - 1))) return 0;
    unsigned vl = bytes / 2; /* e64,m4: four whole register groups. */
    for (unsigned i = 0; i < vl; ++i) {
        bfu_seed[i] = 0x8100000000000000ULL + i;
        bfu_index[i] = 8ULL * i;
        bfu_dump[i] = 0;
        bfu_data[0][i] = 0x4200000000000000ULL + i;
    }
    CSRW(CSR_VSTART, 0);
    asm volatile("vsetvli zero,%0,e64,m4,tu,mu\nvle64.v v8,(%1)"
        :: "r"((uint64_t)vl), "r"(bfu_seed) : "memory");
    return vl;
}

static void bfu_load_indices(void)
{
    asm volatile("vle64.v v16,(%0)" :: "r"(bfu_index) : "memory");
}

static void bfu_dump_load(void)
{
    CSRW(CSR_VSTART, 0);
    asm volatile("vse64.v v8,(%0)" :: "r"(bfu_dump) : "memory");
}

bool ai_micro_backend_it_boundary_pointer_wrap_scan(void)
{
    TEST_START();
    struct bfu_saved saved;
    bfu_prepare(&saved);
    CSRW(CSR_VSTART, 0);
    uint64_t vl = CSRR(CSR_VL);
    backend_it_control.target_pc = BFU_I0;
    backend_it_control.target_addr = BFU_I1;
    backend_it_control.auxiliary = 2U * BFU_SCAN_PADS;
    if (!be_it_begin(799, 0)) {
        bfu_restore(&saved);
        return false;
    }
    bool correct = true;
    unsigned attempts = 0;
    for (unsigned taken = 0; taken < 2; ++taken) {
        /* BNE-not-taken resists ROB compression; JAL+4 also advances FTQ
         * through actual control flow. Sweep instruction count, not a loop
         * whose fixed ROB/FTQ stride could alias a subset of the pointers. */
        for (unsigned i = 0; i < BFU_SCAN_PADS; ++i)
            bfu_code[1022 - i] = taken ? 0x0040006fU : 0x00001263U;
        bfu_publish_code(0x10401073U); /* csrw sie,zero at I0 */
        for (unsigned pad = 0; pad < BFU_SCAN_PADS; ++pad) {
            be_bfu_count = 0;
            CSRW(CSR_SIE, 2);
            bfu_enter(BFU_I0 - 4U * pad, 0, false);
            correct &= be_bfu_count == 1 &&
                bfu_event(0, CAUSE_IPF, BFU_I1, BFU_I1, 0, vl) &&
                CSRR(CSR_SIE) == 0;
            bfu_enter((uintptr_t)be_bfu_clean_probe, 0, false);
            ++attempts;
            correct &= be_bfu_count == 2 &&
                bfu_event(1, CAUSE_ECS, (uintptr_t)be_bfu_clean_ecall, 0, 0, vl) &&
                be_bfu_clean_marker == attempts;
            if (!correct) break;
        }
        if (!correct) break;
    }
    TEST_ASSERT("row 799: each I1 IPF has exact metadata; a clean S probe executes once",
        correct && attempts == 2U * BFU_SCAN_PADS,
        "attempts=%u count=%lu cause=%lu epc=%lx tval=%lx", attempts,
        be_bfu_count, be_bfu_events[0].cause, be_bfu_events[0].epc,
        be_bfu_events[0].tval);
    TEST_ASSERT("row 799: both I0-to-I1 generation flags crossed in this bounded scan",
        be_it_finish());
    bfu_restore(&saved);
    TEST_END("ai_micro_backend_it_boundary_pointer_wrap_scan");
}

bool ai_micro_backend_it_boundary_vector_load_pf(void)
{
    TEST_START();
    struct bfu_saved saved;
    bfu_prepare(&saved);
    unsigned vl = bfu_vector_seed();
    if (!vl) { bfu_restore(&saved); return false; }
    unsigned k = vl / 4 + 1;
    uintptr_t base = BFU_FAULT_VA - 8U * k;
    for (unsigned i = 0; i < k; ++i)
        bfu_data[0][BFU_WORDS - k + i] = 0x4200000000000000ULL + i;
    bfu_publish_code(be_bfu_load_insn);
    bfu_descriptor(813, vl, k, 0);
    if (!be_it_begin_scenario(813, BFU_SC_LOAD_PF, 0)) {
        bfu_restore(&saved); return false;
    }
    CSRW(CSR_SIE, 2);
    CSRW(CSR_VSTART, 1);
    bfu_enter(BFU_I0, base, true);
    bool correct = be_bfu_count == 1 &&
        bfu_event(0, CAUSE_LPF, BFU_I0, BFU_FAULT_VA, k, vl) &&
        (be_bfu_events[0].pending & 2);
    bfu_dump_load();
    correct &= bfu_dump[0] == bfu_seed[0];
    for (unsigned i = 1; i < k; ++i)
        correct &= bfu_dump[i] == 0x4200000000000000ULL + i;
    TEST_ASSERT("row 813: LPF owns I0, data tval, vstart=k, VL and completed load prefix",
        correct, "count=%lu cause=%lu epc=%lx tval=%lx vstart=%lu k=%u",
        be_bfu_count, be_bfu_events[0].cause, be_bfu_events[0].epc,
        be_bfu_events[0].tval, be_bfu_events[0].vstart, k);
    TEST_ASSERT("row 813: real VLS exception replaced its own enqueue flush intent",
        be_it_finish());
    bfu_restore(&saved);
    TEST_END("ai_micro_backend_it_boundary_vector_load_pf");
}

bool ai_micro_backend_it_boundary_vector_store_pf_retry(void)
{
    TEST_START();
    struct bfu_saved saved;
    bfu_prepare(&saved);
    unsigned vl = bfu_vector_seed();
    if (!vl) { bfu_restore(&saved); return false; }
    unsigned k = vl / 4 + 1;
    for (unsigned i = k; i < vl; ++i)
        bfu_index[i] = BFU_FAULT_VA - BFU_DATA_VA + 8U * (i - k);
    bfu_load_indices();
    for (unsigned i = 0; i < vl; ++i) bfu_data[0][i] = BFU_SENTINEL;
    bfu_data_l0[BFU_FAULT_LEAF] = bfu_leaf((uintptr_t)bfu_data[1], PTE_R);
    bfu_publish_code(be_bfu_store_insn);
    bfu_descriptor(814, vl, k, 0);
    if (!be_it_begin_scenario(814, BFU_SC_STORE_RETRY, 0)) {
        bfu_restore(&saved); return false;
    }
    CSRW(CSR_SIE, 2);
    CSRW(CSR_VSTART, 1);
    bfu_enter(BFU_I0, BFU_DATA_VA, true);
    bool first = be_bfu_count == 1 &&
        bfu_event(0, CAUSE_SPF, BFU_I0, BFU_FAULT_VA, k, vl) &&
        (be_bfu_events[0].pending & 2);
    for (unsigned i = 0; i < BFU_WORDS; ++i) {
        uint64_t expected = i >= 1 && i < k ? bfu_seed[i] : BFU_SENTINEL;
        first &= bfu_data[0][i] == expected && bfu_data[1][i] == BFU_SENTINEL;
    }
    bool retry = false;
    if (first) {
        /* Poison the completed prefix. A wrongly restarted element <k
         * would overwrite these values, even though the original store
         * payload would otherwise make duplicate execution invisible. */
        for (unsigned i = 1; i < k; ++i)
            bfu_data[0][i] = BFU_PREFIX_MARK + i;
        bfu_data_l0[BFU_FAULT_LEAF] |= PTE_W;
        asm volatile("fence rw,rw" ::: "memory");
        sfence_vma();
        /* No vset/vload/vstore between trap and retry: preserve the actual
         * hardware vstart and source/index registers across recovery. */
        retry = CSRR(CSR_VSTART) == k;
        bfu_enter(BFU_I0, BFU_DATA_VA, false);
        retry &= be_bfu_count == 2 &&
            bfu_event(1, CAUSE_IPF, BFU_I1, BFU_I1, 0, vl);
        for (unsigned i = 0; i < BFU_WORDS; ++i) {
            uint64_t prefix = i >= 1 && i < k ? BFU_PREFIX_MARK + i : BFU_SENTINEL;
            uint64_t suffix = i < vl - k ? bfu_seed[k + i] : BFU_SENTINEL;
            retry &= bfu_data[0][i] == prefix && bfu_data[1][i] == suffix;
        }
    }
    TEST_ASSERT("row 814: ordered SPF completes exactly the legal active prefix", first);
    TEST_ASSERT("row 814: PTE repair/SFENCE/retry keeps prefix and reaches only then I1 IPF", retry);
    TEST_ASSERT("row 814: delayed SPF displaced the same instruction's flush intent",
        be_it_finish());
    bfu_restore(&saved);
    TEST_END("ai_micro_backend_it_boundary_vector_store_pf_retry");
}

bool ai_micro_backend_it_boundary_vector_older_fault(void)
{
    TEST_START();
    struct bfu_saved saved;
    bfu_prepare(&saved);
    unsigned vl = bfu_vector_seed();
    if (!vl) { bfu_restore(&saved); return false; }
    unsigned k = vl / 4 + 1, j = vl / 2 + 1;
    bfu_index[k] = BFU_FAULT_VA - BFU_DATA_VA;
    bfu_index[j] = BFU_YOUNG_VA - BFU_DATA_VA;
    bfu_load_indices();
    bfu_publish_code(be_bfu_index_insn);
    bfu_descriptor(815, vl, k, j);
    if (!be_it_begin_scenario(815, BFU_SC_OLDER_PTW, 0)) {
        bfu_restore(&saved); return false;
    }
    CSRW(CSR_VSTART, 1);
    bfu_enter(BFU_I0, BFU_DATA_VA, false);
    bool correct = be_bfu_count == 1 &&
        bfu_event(0, CAUSE_LPF, BFU_I0, BFU_FAULT_VA, k, vl);
    bfu_dump_load();
    correct &= bfu_dump[0] == bfu_seed[0];
    for (unsigned i = 1; i < k; ++i)
        correct &= bfu_dump[i] == 0x4200000000000000ULL + i;
    TEST_ASSERT("row 815: final LPF selects older element k, never j or candidate I1",
        correct, "count=%lu cause=%lu epc=%lx tval=%lx vstart=%lu k=%u j=%u",
        be_bfu_count, be_bfu_events[0].cause, be_bfu_events[0].epc,
        be_bfu_events[0].tval, be_bfu_events[0].vstart, k, j);
    TEST_ASSERT("row 815: external PTW response ordering produced j then k in ExceptionGen",
        be_it_finish());
    bfu_restore(&saved);
    TEST_END("ai_micro_backend_it_boundary_vector_older_fault");
}
