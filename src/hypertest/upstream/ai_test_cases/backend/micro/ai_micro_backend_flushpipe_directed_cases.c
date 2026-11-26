#include <rvh_test.h>
#include <page_tables.h>

/* Setup stores must not become implicit VSETVLI instructions that reset
 * VSTART or change VL/vtype before the instruction under test. */
#pragma GCC optimize("no-tree-vectorize")

/* nhv5_1_ap; manual; reason_code: D-MANUAL-NONGATE.
 * Workbook rows 775-805 marked for directed coverage. No IT mailbox or
 * injection agent is required. Spike checks the architectural oracle only;
 * ROB flush sources and actual ITLB/cache hits need RTL observation.
 */
#define BF_B39 (1ULL << 38)
#define BF_B48 (1ULL << 47)
#define BF_STATE_BITS ((3ULL << 13) | (3ULL << 9))
#define BF_MPP_MASK (3ULL << 11)
#define BF_MPRV (1ULL << 17)
#define BF_TSR (1ULL << 22)
#define BF_SPP (1ULL << 8)
#define BF_SSIP 2ULL
#define BF_CSR_SIE 0x104U
#define BF_CSR_SIP 0x144U
#define BF_CSR_SSTATUS 0x100U
#define BF_ECALL 0x00000073U
#define BF_CSRW_SIE_ZERO 0x10401073U
#define BF_CSRW_SIP_ZERO 0x14401073U
#define BF_RD_SENTINEL 0x5a5aULL

static pte_t bf_root[512] __attribute__((aligned(4096)));
static pte_t bf_tables[8][512] __attribute__((aligned(4096)));
static uint8_t bf_code[3][4096] __attribute__((aligned(4096)));
static unsigned bf_table_count, bf_levels;
static uint64_t bf_satp, bf_source_pc, bf_next_pc;
static uint64_t bf_guard[3];

struct bf_trap_record {
    uint64_t count, cause, epc, tval, status, a0, a2;
    uint64_t sie, sip, vstart, frm, vl, vtype;
};
static volatile struct bf_trap_record bf_log __attribute__((used));
static volatile uint64_t bf_marker __attribute__((used));
static volatile uint64_t bf_seed __attribute__((used));
static volatile uint64_t bf_entry_status __attribute__((used));
static volatile uint64_t bf_return_status __attribute__((used));
static volatile uint64_t bf_return_pc __attribute__((used));
static volatile uint64_t bf_saved_ra __attribute__((used));
static volatile uint64_t bf_s_count __attribute__((used));
static volatile uint64_t bf_s_cause __attribute__((used));
static volatile uint64_t bf_s_epc __attribute__((used));
static volatile uint64_t bf_s_tval __attribute__((used));

extern void bf_enter(uint64_t pc, uint64_t rs1, uint64_t rs2);
extern void bf_m_trap(void);
extern void bf_s_trap(void);
extern void bf_s_recover(void);

asm(
    ".pushsection .text.bf_directed,\"ax\",@progbits\n"
    ".option push\n.option norvc\n.balign 4\n"
    ".global bf_enter\nbf_enter:\n"
    "la t3,bf_saved_ra\nsd ra,0(t3)\n"
    "la t3,bf_return_pc\nla t4,.Lbf_return\nsd t4,0(t3)\n"
    "mv t6,a0\nmv t0,a1\nmv t1,a2\n"
    "la a1,bf_marker\nla a0,bf_seed\nld a0,0(a0)\nli a2,0x5a5a\n"
    "la t3,bf_entry_status\nld t3,0(t3)\ncsrw mstatus,t3\n"
    "csrw mepc,t6\nmret\n"
    ".Lbf_return: ret\n"

    ".balign 4\n.global bf_m_trap\nbf_m_trap:\n"
    "addi sp,sp,-64\nsd t0,0(sp)\nsd t1,8(sp)\nsd t2,16(sp)\n"
    "sd t3,24(sp)\nsd t4,32(sp)\nsd t5,40(sp)\nsd t6,48(sp)\nsd ra,56(sp)\n"
    "la t0,bf_log\nld t1,0(t0)\naddi t1,t1,1\nsd t1,0(t0)\n"
    "csrr t1,mcause\nsd t1,8(t0)\ncsrr t1,mepc\nsd t1,16(t0)\n"
    "csrr t1,mtval\nsd t1,24(t0)\ncsrr t1,mstatus\nsd t1,32(t0)\n"
    "sd a0,40(t0)\nsd a2,48(t0)\ncsrr t1,sie\nsd t1,56(t0)\n"
    "csrr t1,sip\nsd t1,64(t0)\n"
    /* Capture FS/VS before re-enabling them for safe CSR snapshots. */
    "la t2,bf_return_status\nld t2,0(t2)\ncsrw mstatus,t2\n"
    "csrr t1,vstart\nsd t1,72(t0)\ncsrr t1,frm\nsd t1,80(t0)\n"
    "csrr t1,vl\nsd t1,88(t0)\ncsrr t1,vtype\nsd t1,96(t0)\n"
    "la t0,bf_return_pc\nld t0,0(t0)\ncsrw mepc,t0\n"
    "ld t0,0(sp)\nld t1,8(sp)\nld t2,16(sp)\nld t3,24(sp)\n"
    "ld t4,32(sp)\nld t5,40(sp)\nld t6,48(sp)\nld ra,56(sp)\naddi sp,sp,64\n"
    "la ra,bf_saved_ra\nld ra,0(ra)\nmret\n"

    ".balign 4\n.global bf_s_trap\nbf_s_trap:\n"
    "addi sp,sp,-32\nsd t0,0(sp)\nsd t1,8(sp)\nsd t2,16(sp)\nsd t3,24(sp)\n"
    "la t0,bf_s_count\nld t1,0(t0)\naddi t1,t1,1\nsd t1,0(t0)\n"
    "csrr t1,scause\nla t0,bf_s_cause\nsd t1,0(t0)\n"
    "csrr t1,sepc\nla t0,bf_s_epc\nsd t1,0(t0)\n"
    "csrr t1,stval\nla t0,bf_s_tval\nsd t1,0(t0)\n"
    "csrw sip,zero\n"
    "la t0,bf_marker\nld t1,0(t0)\naddi t1,t1,1\nsd t1,0(t0)\n"
    "la t0,bf_s_recover\ncsrw sepc,t0\n"
    "ld t0,0(sp)\nld t1,8(sp)\nld t2,16(sp)\nld t3,24(sp)\naddi sp,sp,32\nsret\n"
    ".global bf_s_recover\nbf_s_recover: ecall\nj bf_s_recover\n"
    ".option pop\n.popsection\n");

struct bf_saved_state {
    uint64_t mstatus, mie, mip, medeleg, mideleg, mtvec, stvec, satp, sepc;
    uint64_t sie, fcsr, vstart, vl, vtype, pmpcfg0, pmpaddr0, pmpaddr1;
};

static pte_t bf_leaf(uint64_t pa, uint64_t flags)
{
    return ((pa & ~4095ULL) >> 2) | flags;
}

static pte_t *bf_alloc_table(void)
{
    if (bf_table_count == sizeof(bf_tables) / sizeof(bf_tables[0])) return NULL;
    pte_t *table = bf_tables[bf_table_count++];
    for (unsigned i = 0; i < 512; ++i) table[i] = 0;
    return table;
}

static bool bf_map(uint64_t va, uint64_t pa, uint64_t flags)
{
    pte_t *table = bf_root;
    for (unsigned level = bf_levels - 1; level; --level) {
        unsigned index = (va >> (12 + 9 * level)) & 511;
        pte_t entry = table[index];
        if (!(entry & PTE_V) || (entry & PTE_RWX)) {
            pte_t *next = bf_alloc_table();
            if (!next) return false;
            /* Split an identity superpage while preserving every other leaf. */
            if (entry & PTE_RWX) {
                uint64_t base = (entry >> 10) << 12;
                uint64_t step = 1ULL << (12 + 9 * (level - 1));
                for (unsigned n = 0; n < 512; ++n)
                    next[n] = bf_leaf(base + n * step, entry & 0x3ff);
            }
            table[index] = ((uintptr_t)next >> 2) | PTE_V;
        }
        table = (pte_t *)(uintptr_t)((table[index] >> 10) << 12);
    }
    table[(va >> 12) & 511] = bf_leaf(pa, flags);
    return true;
}

static void bf_clear_log(void)
{
    volatile uint64_t *words = (volatile uint64_t *)&bf_log;
    for (unsigned i = 0; i < sizeof(bf_log) / 8; ++i) words[i] = 0;
    bf_s_count = bf_s_cause = bf_s_epc = bf_s_tval = 0;
    bf_marker = bf_seed = 0;
}

static void bf_save(struct bf_saved_state *saved)
{
    goto_priv(PRIV_M);
    saved->mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, BF_STATE_BITS);
    saved->mie = CSRR(CSR_MIE);
    saved->mip = CSRR(CSR_MIP);
    saved->medeleg = CSRR(CSR_MEDELEG);
    saved->mideleg = CSRR(CSR_MIDELEG);
    saved->mtvec = CSRR(CSR_MTVEC);
    saved->stvec = CSRR(CSR_STVEC);
    saved->satp = CSRR(CSR_SATP);
    saved->sepc = CSRR(CSR_SEPC);
    saved->sie = CSRR(CSR_SIE);
    saved->fcsr = CSRR(CSR_FCSR);
    saved->vstart = CSRR(CSR_VSTART);
    saved->vl = CSRR(CSR_VL);
    saved->vtype = CSRR(CSR_VTYPE);
    saved->pmpcfg0 = CSRR(CSR_PMPCFG0);
    saved->pmpaddr0 = CSRR(CSR_PMPADDR0);
    saved->pmpaddr1 = CSRR(CSR_PMPADDR1);
    CSRW(CSR_MIE, 0);
    CSRC(CSR_MSTATUS, BF_MPRV | MSTATUS_MIE | MSTATUS_TVM | BF_TSR);
    CSRW(CSR_MEDELEG, 0);
    CSRW(CSR_MIDELEG, BF_SSIP);
    CSRC(CSR_MIP, BF_SSIP);
    CSRW(CSR_SIE, 0);
    CSRW(CSR_SATP, 0);
    sfence_vma();
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, ~0ULL);
    CSRW(CSR_PMPCFG0, 0x1f);
    CSRW(CSR_VSTART, 0);
    bf_return_status = (CSRR(CSR_MSTATUS) &
        ~(BF_MPP_MASK | BF_MPRV | MSTATUS_MIE | MSTATUS_SIE | (1ULL << 7))) |
        BF_MPP_MASK | BF_STATE_BITS;
    bf_entry_status = (bf_return_status & ~BF_MPP_MASK) | (1ULL << 11);
    bf_clear_log();
    bf_guard[0] = 0x1122334455667788ULL;
    bf_guard[1] = 0x8877665544332211ULL;
    bf_guard[2] = 0x13579bdf2468ace0ULL;
    CSRW(CSR_MTVEC, (uintptr_t)bf_m_trap);
    CSRW(CSR_STVEC, (uintptr_t)bf_s_trap);
}

static void bf_restore(const struct bf_saved_state *saved)
{
    curr_priv = PRIV_M;
    CSRS(CSR_MSTATUS, BF_STATE_BITS);
    CSRC(CSR_MSTATUS, BF_MPRV | MSTATUS_MIE);
    CSRW(CSR_SATP, 0);
    sfence_vma();
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, saved->pmpaddr0);
    CSRW(CSR_PMPADDR1, saved->pmpaddr1);
    CSRW(CSR_PMPCFG0, saved->pmpcfg0);
    asm volatile("vsetvl zero,%0,%1" :: "r"(saved->vl), "r"(saved->vtype) : "memory");
    CSRW(CSR_VSTART, saved->vstart);
    CSRW(CSR_FCSR, saved->fcsr);
    CSRW(CSR_MTVEC, saved->mtvec);
    CSRW(CSR_STVEC, saved->stvec);
    CSRW(CSR_SEPC, saved->sepc);
    CSRW(CSR_MIDELEG, saved->mideleg);
    CSRW(CSR_SIE, saved->sie);
    CSRC(CSR_MIP, BF_SSIP);
    if (saved->mip & BF_SSIP) CSRS(CSR_MIP, BF_SSIP);
    CSRW(CSR_MEDELEG, saved->medeleg);
    CSRW(CSR_SATP, saved->satp);
    sfence_vma();
    CSRW(CSR_MIE, saved->mie);
    CSRW(CSR_MSTATUS, saved->mstatus);
}

static void bf_word(unsigned page, unsigned offset, uint32_t instruction)
{
    for (unsigned byte = 0; byte < 4; ++byte)
        bf_code[page][offset + byte] = instruction >> (8 * byte);
}

static void bf_half(unsigned page, unsigned offset, uint16_t instruction)
{
    bf_code[page][offset] = instruction;
    bf_code[page][offset + 1] = instruction >> 8;
}

static uint32_t bf_csr(unsigned csr, unsigned form, unsigned rd, unsigned rs1)
{
    return (csr << 20) | (rs1 << 15) | (form << 12) | (rd << 7) | 0x73;
}

static bool bf_layout(unsigned mode, uint64_t source, uint64_t next, uint64_t next_flags)
{
    bf_table_count = 0;
    bf_levels = mode == 8 ? 3 : 4;
    for (unsigned n = 0; n < 512; ++n) bf_root[n] = 0;
    pte_t *low = mode == 8 ? bf_root : bf_alloc_table();
    if (!low) return false;
    for (unsigned n = 0; n < 4; ++n)
        low[n] = bf_leaf((uint64_t)n << 30, PTE_V | PTE_AD | PTE_RWX);
    if (mode == 9) bf_root[0] = ((uintptr_t)low >> 2) | PTE_V;
    for (unsigned page = 0; page < 3; ++page)
        for (unsigned offset = 0; offset < 4096; offset += 4)
            bf_word(page, offset, 0x00000013);
    if (!bf_map(source, (uintptr_t)bf_code[0], PTE_V | PTE_AD | PTE_RWX)) return false;
    bool same_page = (source & ~4095ULL) == (next & ~4095ULL);
    unsigned target_page = same_page ? 0 : 1;
    if (!same_page && !bf_map(next, (uintptr_t)bf_code[1], next_flags)) return false;
    unsigned offset = next & 4095;
    if (offset + 12 <= 4096) {
        bf_word(target_page, offset, 0x00150513); /* addi a0,a0,1 */
        bf_word(target_page, offset + 4, 0x00a5b023); /* sd a0,0(a1) */
        bf_word(target_page, offset + 8, BF_ECALL);
    }
    bf_word(2, 0, 0x00150513);
    bf_word(2, 4, 0x00a5b023);
    bf_word(2, 8, BF_ECALL);
    bf_source_pc = source;
    bf_next_pc = next;
    bf_satp = ((uint64_t)mode << 60) | (1ULL << 44) | ((uintptr_t)bf_root >> 12);
    CSRW(CSR_SATP, bf_satp);
    sfence_vma();
    return true;
}

static void bf_publish(void)
{
    asm volatile("fence rw,rw\nfence.i" ::: "memory");
}

static void bf_execute(uint64_t rs1, uint64_t rs2)
{
    bf_enter(bf_source_pc, rs1, rs2);
    curr_priv = PRIV_M;
}

static bool bf_expect(unsigned row, unsigned variant, uint64_t cause, uint64_t epc, uint64_t tval)
{
    bool ok = bf_log.count == 1 && bf_log.cause == cause && bf_log.epc == epc &&
        bf_log.tval == tval && ((bf_log.status >> 11) & 3) == 1 &&
        bf_guard[0] == 0x1122334455667788ULL && bf_guard[1] == 0x8877665544332211ULL &&
        bf_guard[2] == 0x13579bdf2468ace0ULL;
    printf("BE_DIRECTED row=%u variant=%u source=%lx next=%lx cause=%lx epc=%lx tval=%lx count=%lu marker=%lu rd=%lx status=%s\n",
        row, variant, bf_source_pc, bf_next_pc, bf_log.cause, bf_log.epc, bf_log.tval,
        bf_log.count, bf_marker, bf_log.a2, ok ? "PASS" : "FAIL");
    return ok;
}

static bool bf_basic(unsigned row, unsigned mode)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    uint64_t boundary = mode == 8 ? BF_B39 : BF_B48;
    bool ok = bf_layout(mode, boundary - 4, boundary, PTE_V | PTE_AD | PTE_RWX);
    bf_word(0, 4092, BF_CSRW_SIE_ZERO);
    CSRW(CSR_SIE, BF_SSIP);
    bf_publish();
    if (ok) bf_execute(0, 0);
    ok &= bf_expect(row, 0, CAUSE_IPF, boundary, boundary) &&
        !(bf_log.sie & BF_SSIP) && bf_marker == 0 && bf_log.a0 == 0;
    bf_restore(&saved);
    return ok;
}

static bool bf_write_forms(void)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = true;
    const unsigned forms[] = {1, 2, 3, 5, 6, 7};
    for (unsigned csr_index = 0; csr_index < 2; ++csr_index) {
        unsigned csr = csr_index ? BF_CSR_SIP : BF_CSR_SIE;
        for (unsigned variant = 0; variant < 6; ++variant) {
            unsigned form = forms[variant];
            uint64_t before = form == 2 || form == 6 ? 0 : BF_SSIP;
            uint64_t data = form == 1 ? 0 : BF_SSIP;
            uint64_t after = form == 2 || form == 6 || form == 5 ? BF_SSIP : 0;
            bf_clear_log();
            ok &= bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
            CSRW(CSR_SIE, csr_index ? 0 : before);
            CSRC(CSR_MIP, BF_SSIP);
            if (csr_index && before) CSRS(CSR_MIP, BF_SSIP);
            bf_word(0, 4092, bf_csr(csr, form, 12, form > 4 ? 2 : 5));
            bf_publish();
            bf_execute(data, 0);
            uint64_t observed = csr_index ? bf_log.sip : bf_log.sie;
            bool variant_ok = bf_expect(777, csr_index * 6 + variant, CAUSE_IPF, BF_B39, BF_B39) &&
                (observed & BF_SSIP) == after && bf_log.a2 == before && bf_marker == 0;
            ok &= variant_ok;
            printf("BE_DIRECTED_CSR csr=%x form=%u before=%lx after=%lx expected=%lx status=%s\n",
                csr, form, before, observed, after, variant_ok ? "PASS" : "FAIL");
        }
    }
    bf_restore(&saved);
    return ok;
}

static bool bf_same_value(void)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = true;
    for (unsigned form = 1; form <= 3; ++form) {
        bf_clear_log();
        ok &= bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
        CSRW(CSR_SIE, 0);
        bf_word(0, 4092, bf_csr(BF_CSR_SIE, form, 0, form == 1 ? 0 : 5));
        bf_publish();
        bf_execute(0, 0);
        ok &= bf_expect(778, form, CAUSE_IPF, BF_B39, BF_B39) &&
            !(bf_log.sie & BF_SSIP) && bf_marker == 0;
    }
    bf_restore(&saved);
    return ok;
}

static bool bf_status_switch(unsigned row)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = true;
    unsigned switches = row == 779 ? 2 : 4;
    for (unsigned variant = 0; variant < switches; ++variant) {
        uint64_t field = row == 779 ? MSTATUS_SIE : variant < 2 ? (3ULL << 13) : (3ULL << 9);
        unsigned shift = row == 779 ? 1 : variant < 2 ? 13 : 9;
        bool enable = !(variant & 1);
        bf_entry_status = (bf_return_status & ~BF_MPP_MASK) | (1ULL << 11);
        if (enable) bf_entry_status &= ~field;
        else bf_entry_status |= field;
        uint64_t expected_other = bf_entry_status & (BF_STATE_BITS | MSTATUS_SIE) & ~field;
        bf_clear_log();
        ok &= bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
        bf_word(0, 4092, bf_csr(BF_CSR_SSTATUS, enable ? 2 : 3, 0, 5));
        bf_publish();
        bf_execute(enable ? 1ULL << shift : field, 0);
        bool variant_ok = bf_expect(row, variant, CAUSE_IPF, BF_B39, BF_B39) &&
            !!(bf_log.status & field) == enable &&
            (bf_log.status & (BF_STATE_BITS | MSTATUS_SIE) & ~field) == expected_other && bf_marker == 0;
        ok &= variant_ok;
        printf("BE_DIRECTED_STATUS field=%lx before=%lx after=%lx status=%s\n",
            field, bf_entry_status, bf_log.status, variant_ok ? "PASS" : "FAIL");
    }
    bf_restore(&saved);
    return ok;
}

static bool bf_frm_alias(void)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = true;
    for (unsigned variant = 0; variant < 4; ++variant) {
        bool alias_fcsr = variant >= 2, to_reserved = !(variant & 1);
        uint64_t old_frm = to_reserved ? 0 : 5, new_frm = to_reserved ? 5 : 0;
        bf_clear_log();
        ok &= bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
        CSRW(CSR_FCSR, (old_frm << 5) | 0x10);
        bf_word(0, 4092, bf_csr(alias_fcsr ? 3 : 2, 1, 12, 5));
        bf_publish();
        bf_execute(alias_fcsr ? (new_frm << 5) | 0x10 : new_frm, 0);
        ok &= bf_expect(781, variant, CAUSE_IPF, BF_B39, BF_B39) && bf_log.frm == new_frm &&
            bf_log.a2 == (alias_fcsr ? (old_frm << 5) | 0x10 : old_frm) &&
            (CSRR(CSR_FCSR) & 31) == 0x10 && bf_marker == 0;
    }
    bf_restore(&saved);
    return ok;
}

static bool bf_vstart(void)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = true;
    for (unsigned before = 0; before < 2; ++before) {
        bf_clear_log();
        ok &= bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
        CSRW(CSR_VSTART, before);
        bf_word(0, 4092, bf_csr(8, 1, 12, 5));
        bf_publish();
        bf_execute(1 - before, 0);
        ok &= bf_expect(782, before, CAUSE_IPF, BF_B39, BF_B39) &&
            bf_log.a2 == before && bf_log.vstart == 1 - before && bf_marker == 0;
    }
    bf_restore(&saved);
    return ok;
}

static bool bf_sfence(void)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = true;
    for (unsigned variant = 0; variant < 4; ++variant) {
        bf_clear_log();
        ok &= bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
        uint32_t instruction = 0x12000073U | ((variant & 1 ? 5 : 0) << 15) |
            ((variant & 2 ? 6 : 0) << 20);
        bf_word(0, 4092, instruction);
        bf_publish();
        bf_execute(BF_B39 - 4096, 1);
        ok &= bf_expect(785, variant, CAUSE_IPF, BF_B39, BF_B39) &&
            CSRR(CSR_SATP) == bf_satp && bf_marker == 0;
    }
    bf_restore(&saved);
    return ok;
}

static bool bf_vsetvl(void)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
    asm volatile("vsetvli zero,zero,e8,m1,tu,mu" ::: "memory");
    CSRW(CSR_VSTART, 0);
    bf_word(0, 4092, 0x8062f657U); /* vsetvl a2,t0,t1 */
    bf_publish();
    bf_execute(3, 9); /* e16,m2,tu,mu; AVL=3 */
    ok &= bf_expect(786, 0, CAUSE_IPF, BF_B39, BF_B39) &&
        bf_log.vtype == 9 && bf_log.vl == 3 && bf_log.a2 == 3 && bf_log.vstart == 0 && bf_marker == 0;
    bf_restore(&saved);
    return ok;
}

static bool bf_legal_targets(unsigned row)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = true;
    for (unsigned mode = 8; mode <= 9; ++mode) {
        uint64_t source, target;
        if (row == 788) source = BF_B39 - 4, target = BF_B39;
        else if (row == 789) source = mode == 8 ? 0xffffffc000001000ULL : 0xffff800000001000ULL, target = source + 4;
        else source = ~3ULL, target = 0;
        bf_clear_log();
        ok &= bf_layout(mode, source, target, PTE_V | PTE_AD | PTE_RWX);
        bf_word(0, source & 4095, BF_CSRW_SIE_ZERO);
        CSRW(CSR_SIE, BF_SSIP);
        bf_publish();
        bf_execute(0, 0);
        bool faults = row == 788 && mode == 8;
        ok &= bf_expect(row, mode, faults ? CAUSE_IPF : CAUSE_ECS,
            faults ? target : target + 8, faults ? target : 0) &&
            !(bf_log.sie & BF_SSIP) && bf_marker == (faults ? 0 : 1) && bf_log.a0 == (faults ? 0 : 1);
    }
    bf_restore(&saved);
    return ok;
}

static bool bf_split_instruction(unsigned row)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool self_fault = row == 793;
    uint64_t source = BF_B39 - (self_fault ? 2 : 6);
    bool ok = bf_layout(8, source, BF_B39, PTE_V | PTE_AD | PTE_RWX);
    if (self_fault) {
        bf_half(0, 4094, BF_CSRW_SIE_ZERO & 0xffff);
        bf_half(1, 0, BF_CSRW_SIE_ZERO >> 16);
    } else {
        bf_word(0, 4090, BF_CSRW_SIE_ZERO);
        if (row == 791) bf_half(0, 4094, 0x0505); /* c.addi a0,1 */
        else {
            bf_half(0, 4094, 0xb023); /* lower half of sd a0,0(a1) */
            bf_half(1, 0, 0x00a5);
            bf_seed = 0x55;
        }
    }
    CSRW(CSR_SIE, BF_SSIP);
    bf_publish();
    bf_execute(0, 0);
    uint64_t epc = row == 791 ? BF_B39 : BF_B39 - 2;
    ok &= bf_expect(row, 0, CAUSE_IPF, epc, BF_B39) && bf_marker == 0 &&
        (bf_log.sie & BF_SSIP) == (self_fault ? BF_SSIP : 0);
    if (row == 791) ok &= bf_log.a0 == 1;
    if (row == 792) ok &= bf_log.a0 == 0x55;
    bf_restore(&saved);
    return ok;
}

static bool bf_illegal_or_sret(unsigned row)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
    if (row == 794) bf_word(0, 4092, bf_csr(0x304, 1, 0, 5));
    else {
        bf_word(0, 4092, 0x10200073U);
        CSRW(CSR_SEPC, (uintptr_t)bf_code[2]);
        bf_entry_status |= BF_SPP;
    }
    uint64_t before_mie = CSRR(CSR_MIE);
    bf_publish();
    bf_execute(~0ULL, 0);
    if (row == 794) {
        bool trap_ok = bf_log.count == 1 && bf_log.cause == CAUSE_ILI && bf_log.epc == BF_B39 - 4;
        printf("BE_DIRECTED row=794 cause=%lx epc=%lx mie=%lx status=%s\n",
            bf_log.cause, bf_log.epc, CSRR(CSR_MIE), trap_ok ? "PASS" : "FAIL");
        ok &= trap_ok && CSRR(CSR_MIE) == before_mie && bf_marker == 0;
    } else ok &= bf_expect(row, 0, CAUSE_ECS, (uintptr_t)bf_code[2] + 8, 0) &&
        bf_marker == 1 && !(bf_log.status & BF_SPP);
    bf_restore(&saved);
    return ok;
}

static bool bf_read_only(void)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = true;
    const unsigned forms[] = {2, 6, 7};
    for (unsigned variant = 0; variant < 3; ++variant) {
        bf_clear_log();
        ok &= bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
        CSRW(CSR_SIE, BF_SSIP);
        bf_word(0, 4092, bf_csr(BF_CSR_SIE, forms[variant], 12, 0));
        bf_publish();
        bf_execute(0, 0);
        ok &= bf_expect(796, variant, CAUSE_IPF, BF_B39, BF_B39) &&
            bf_log.a2 == BF_SSIP && (bf_log.sie & BF_SSIP) == BF_SSIP && bf_marker == 0;
    }
    bf_restore(&saved);
    return ok;
}

static bool bf_warm_alias(void)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
    bf_word(0, 4092, BF_CSRW_SIE_ZERO);
    bf_publish();
    bf_enter(0xffffffc000000000ULL, 0, 0);
    curr_priv = PRIV_M;
    ok &= bf_log.count == 1 && bf_log.cause == CAUSE_ECS &&
        bf_log.epc == 0xffffffc000000008ULL && bf_marker == 1;
    bf_clear_log();
    CSRW(CSR_SIE, BF_SSIP);
    /* No SATP write, SFENCE or FENCE.I between alias warmup and the target. */
    bf_execute(0, 0);
    ok &= bf_expect(797, 0, CAUSE_IPF, BF_B39, BF_B39) &&
        bf_marker == 0 && !(bf_log.sie & BF_SSIP);
    bf_restore(&saved);
    return ok;
}

static bool bf_two_flushes(unsigned row)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = bf_layout(8, BF_B39 - 8, BF_B39, PTE_V | PTE_AD | PTE_RWX);
    bf_word(0, 4088, row == 800 ? BF_CSRW_SIP_ZERO : bf_csr(0x180, 1, 0, 5));
    bf_word(0, 4092, BF_CSRW_SIE_ZERO);
    CSRW(CSR_SIE, BF_SSIP);
    CSRS(CSR_MIP, BF_SSIP);
    bf_publish();
    bf_execute(bf_satp, 0);
    ok &= bf_expect(row, 0, CAUSE_IPF, BF_B39, BF_B39) &&
        !(bf_log.sie & BF_SSIP) && bf_marker == 0 && CSRR(CSR_SATP) == bf_satp;
    if (row == 800) ok &= !(bf_log.sip & BF_SSIP);
    bf_restore(&saved);
    return ok;
}

static bool bf_delegated_handler(void)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    bool ok = bf_layout(8, BF_B39 - 4, BF_B39, PTE_V | PTE_AD | PTE_RWX);
    bf_word(0, 4092, BF_CSRW_SIE_ZERO);
    CSRW(CSR_MEDELEG, 1ULL << CAUSE_IPF);
    CSRW(CSR_SIE, BF_SSIP);
    CSRS(CSR_MIP, BF_SSIP);
    bf_publish();
    bf_execute(0, 0);
    ok &= bf_s_count == 1 && bf_s_cause == CAUSE_IPF && bf_s_epc == BF_B39 && bf_s_tval == BF_B39 &&
        bf_expect(802, 0, CAUSE_ECS, (uintptr_t)bf_s_recover, 0) && bf_marker == 1 &&
        !(bf_log.sie & BF_SSIP) && !(bf_log.sip & BF_SSIP);
    printf("BE_DIRECTED_S cause=%lx epc=%lx tval=%lx count=%lu\n",
        bf_s_cause, bf_s_epc, bf_s_tval, bf_s_count);
    bf_restore(&saved);
    return ok;
}

static bool bf_frontend_permission(unsigned row)
{
    struct bf_saved_state saved;
    bf_save(&saved);
    const uint64_t target = 0x50001000ULL;
    bool ok = bf_layout(8, target - 4, target, PTE_V | PTE_AD | PTE_RWX);
    bf_word(0, 4092, BF_CSRW_SIE_ZERO);
    bf_publish();
    if (row == 804) {
        /* Warm with X, revoke it, then make the change visible before use. */
        bf_enter(target, 0, 0);
        curr_priv = PRIV_M;
        ok &= bf_log.count == 1 && bf_log.cause == CAUSE_ECS && bf_marker == 1;
        ok &= bf_map(target, (uintptr_t)bf_code[1], PTE_V | PTE_AD | PTE_RW);
        asm volatile("fence rw,rw\nsfence.vma zero,zero" ::: "memory");
    } else {
        CSRW(CSR_PMPCFG0, 0);
        CSRW(CSR_PMPADDR0, ((uintptr_t)bf_code[1] >> 2) | 511);
        CSRW(CSR_PMPADDR1, ~0ULL);
        CSRW(CSR_PMPCFG0, 0x1f1b); /* 4KB RW/no-X followed by allow-all RWX. */
        asm volatile("sfence.vma zero,zero\nfence.i" ::: "memory");
    }
    for (unsigned variant = 0; variant < (row == 804 ? 2U : 1U); ++variant) {
        bf_clear_log();
        CSRW(CSR_SIE, BF_SSIP);
        bf_execute(0, 0);
        ok &= bf_expect(row, variant, row == 804 ? CAUSE_IPF : CAUSE_IAF, target, target) &&
            !(bf_log.sie & BF_SSIP) && bf_marker == 0;
    }
    bf_restore(&saved);
    return ok;
}

bool ai_micro_backend_flush_sv39_sie_next_ipf(void)
{
    TEST_START();
    TEST_ASSERT("row775: SIE write precedes one precise Sv39 next-PC IPF", bf_basic(775, 8));
    TEST_END("ai_micro_backend_flush_sv39_sie_next_ipf");
}

bool ai_micro_backend_flush_sv48_sie_next_ipf(void)
{
    TEST_START();
    TEST_ASSERT("row776: SIE write preserves the full Sv48 noncanonical next PC", bf_basic(776, 9));
    TEST_END("ai_micro_backend_flush_sv48_sie_next_ipf");
}

bool ai_micro_backend_flush_sie_sip_write_forms(void)
{
    TEST_START();
    TEST_ASSERT("row777: all six SIE/SIP write forms preserve old rd and precise IPF", bf_write_forms());
    TEST_END("ai_micro_backend_flush_sie_sip_write_forms");
}

bool ai_micro_backend_flush_sie_same_value(void)
{
    TEST_START();
    TEST_ASSERT("row778: same-value CSRRW and zero-data non-x0 CSRRS/CSRRC retain next-PC IPF", bf_same_value());
    TEST_END("ai_micro_backend_flush_sie_same_value");
}

bool ai_micro_backend_flush_sstatus_sie(void)
{
    TEST_START();
    TEST_ASSERT("row779: both SIE transitions preserve unrelated state and precise IPF", bf_status_switch(779));
    TEST_END("ai_micro_backend_flush_sstatus_sie");
}

bool ai_micro_backend_flush_sstatus_fs_vs(void)
{
    TEST_START();
    TEST_ASSERT("row780: FS/VS Off transitions report next-PC IPF rather than illegal instruction", bf_status_switch(780));
    TEST_END("ai_micro_backend_flush_sstatus_fs_vs");
}

bool ai_micro_backend_flush_frm_reserved_alias(void)
{
    TEST_START();
    TEST_ASSERT("row781: FRM/FCSR 0-to-5 and 5-to-0 writes remain legal and retain IPF", bf_frm_alias());
    TEST_END("ai_micro_backend_flush_frm_reserved_alias");
}

bool ai_micro_backend_flush_vstart_change(void)
{
    TEST_START();
    TEST_ASSERT("row782: VSTART changes take effect once before the next-PC IPF", bf_vstart());
    TEST_END("ai_micro_backend_flush_vstart_change");
}

bool ai_micro_backend_flush_sfence_selectors(void)
{
    TEST_START();
    TEST_ASSERT("row785: all four SFENCE selectors retain precise noncanonical next-PC IPF", bf_sfence());
    TEST_END("ai_micro_backend_flush_sfence_selectors");
}

bool ai_micro_backend_flush_vsetvl_config(void)
{
    TEST_START();
    TEST_ASSERT("row786: VSETVL writes VL/vtype/rd once before precise next-PC IPF", bf_vsetvl());
    TEST_END("ai_micro_backend_flush_vsetvl_config");
}

bool ai_micro_backend_flush_sv39_sv48_same_target(void)
{
    TEST_START();
    TEST_ASSERT("row788: the same next PC faults in Sv39 and executes once in Sv48", bf_legal_targets(788));
    TEST_END("ai_micro_backend_flush_sv39_sv48_same_target");
}

bool ai_micro_backend_flush_negative_canonical(void)
{
    TEST_START();
    TEST_ASSERT("row789: negative canonical next PCs execute once in Sv39 and Sv48", bf_legal_targets(789));
    TEST_END("ai_micro_backend_flush_negative_canonical");
}

bool ai_micro_backend_flush_xlen_wrap_zero(void)
{
    TEST_START();
    TEST_ASSERT("row790: XLEN overflow reaches VA zero mapped to DRAM, not PA zero", bf_legal_targets(790));
    TEST_END("ai_micro_backend_flush_xlen_wrap_zero");
}

bool ai_micro_backend_flush_successor_rvc_boundary(void)
{
    TEST_START();
    TEST_ASSERT("row791: a legal C.ADDI at B39-2 executes once before IPF at B39", bf_split_instruction(791));
    TEST_END("ai_micro_backend_flush_successor_rvc_boundary");
}

bool ai_micro_backend_flush_successor_split32_ipf(void)
{
    TEST_START();
    TEST_ASSERT("row792: a split SD faults at its high half without committing its store", bf_split_instruction(792));
    TEST_END("ai_micro_backend_flush_successor_split32_ipf");
}

bool ai_micro_backend_flush_self_split32_no_write(void)
{
    TEST_START();
    TEST_ASSERT("row793: a split SIE write reports its own IPF without modifying SIE", bf_split_instruction(793));
    TEST_END("ai_micro_backend_flush_self_split32_no_write");
}

bool ai_micro_backend_flush_illegal_csr_priority(void)
{
    TEST_START();
    TEST_ASSERT("row794: illegal S-mode MIE write wins over the candidate next-PC IPF", bf_illegal_or_sret(794));
    TEST_END("ai_micro_backend_flush_illegal_csr_priority");
}

bool ai_micro_backend_flush_sret_target_priority(void)
{
    TEST_START();
    TEST_ASSERT("row795: SRET follows SEPC and preserves S privilege instead of faulting on PC+4", bf_illegal_or_sret(795));
    TEST_END("ai_micro_backend_flush_sret_target_priority");
}

bool ai_micro_backend_flush_csr_read_only(void)
{
    TEST_START();
    TEST_ASSERT("row796: zero-source pure CSR reads preserve SIE/rd and reach ordinary fetch IPF", bf_read_only());
    TEST_END("ai_micro_backend_flush_csr_read_only");
}

bool ai_micro_backend_flush_canonical_alias_warm(void)
{
    TEST_START();
    TEST_ASSERT("row797: a warmed canonical alias cannot execute for a noncanonical target", bf_warm_alias());
    TEST_END("ai_micro_backend_flush_canonical_alias_warm");
}

bool ai_micro_backend_flush_adjacent_csr_flushes(void)
{
    TEST_START();
    TEST_ASSERT("row800: adjacent SIP/SIE writes both take effect before the second next-PC IPF", bf_two_flushes(800));
    TEST_END("ai_micro_backend_flush_adjacent_csr_flushes");
}

bool ai_micro_backend_flush_satp_then_sie(void)
{
    TEST_START();
    TEST_ASSERT("row801: same-SATP write then SIE write reports IPF at the final full target", bf_two_flushes(801));
    TEST_END("ai_micro_backend_flush_satp_then_sie");
}

bool ai_micro_backend_flush_delegated_handler_tag_clear(void)
{
    TEST_START();
    TEST_ASSERT("row802: delegated IPF then handler SIP flush and SRET reach clean recovery once", bf_delegated_handler());
    TEST_END("ai_micro_backend_flush_delegated_handler_tag_clear");
}

bool ai_micro_backend_flush_next_page_no_exec(void)
{
    TEST_START();
    TEST_ASSERT("row804: visible PTE.X revocation faults at the canonical next page on both attempts", bf_frontend_permission(804));
    TEST_END("ai_micro_backend_flush_next_page_no_exec");
}

bool ai_micro_backend_flush_next_page_pmp_deny(void)
{
    TEST_START();
    TEST_ASSERT("row805: PMP denies only next-page execution and reports precise IAF", bf_frontend_permission(805));
    TEST_END("ai_micro_backend_flush_next_page_pmp_deny");
}
