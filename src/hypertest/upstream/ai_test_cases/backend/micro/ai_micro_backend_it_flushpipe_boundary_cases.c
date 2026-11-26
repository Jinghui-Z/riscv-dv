#include "backend_it_0911.h"

/*
 * AP-IT-only boundary tests. Software builds the architectural candidates;
 * backend_it_monitor_top aligns the external event/refill/debug stimulus to
 * the accepted ROB/FTQ generation. These cases are intentionally not part of
 * the ordinary Spike registration set.
 */

#define BE_BND_SV39_I0              0x0000003ffffffffcULL
#define BE_BND_SV39_I1              0x0000004000000000ULL
#define BE_BND_SV39_NEW_I1          0x0000004000001000ULL
#define BE_BND_DATA_FAULT_VA        0x0000000200000000ULL
#define BE_BND_SATP_SV39            8ULL
#define BE_BND_SATP_SV48            9ULL
#define BE_BND_LOG_DEPTH            8U

#define BE_BND_FENCE_RW_RW          0x0330000fU
#define BE_BND_FENCE_I              0x0000100fU
#define BE_BND_CSRW_SIE_ZERO        0x10401073U
#define BE_BND_CSRW_SIE_T0          0x10429073U
#define BE_BND_CSRSI_SSTATUS_SIE    0x10016073U
#define BE_BND_CSRCI_SSTATUS_SIE    0x10017073U
#define BE_BND_CSRC_SIE_T0          0x1042b073U
#define BE_BND_CSRC_SIP_T0          0x1442b073U
#define BE_BND_VADD_V8_V8_V8        0x02840457U
#define BE_BND_ECALL                0x00000073U
#define BE_BND_ILLEGAL              0xffffffffU

#define BE_BND_MCONTROL6_S_EXEC_BKP 0x6000000000000014ULL
#define BE_BND_NMI31                (CAUSE_INT_BIT | 31ULL)
#define BE_BND_NMI43                (CAUSE_INT_BIT | 43ULL)

extern unsigned real_priv;

static pte_t be_bnd_sv39_root[512] __attribute__((aligned(PAGE_SIZE)));
static pte_t be_bnd_sv39_high_l1[512] __attribute__((aligned(PAGE_SIZE)));
static pte_t be_bnd_sv39_high_l0[512] __attribute__((aligned(PAGE_SIZE)));
static pte_t be_bnd_sv48_root[512] __attribute__((aligned(PAGE_SIZE)));
static pte_t be_bnd_sv48_l2[512] __attribute__((aligned(PAGE_SIZE)));
static pte_t be_bnd_sv48_target_l1[512] __attribute__((aligned(PAGE_SIZE)));
static pte_t be_bnd_sv48_target_l0[512] __attribute__((aligned(PAGE_SIZE)));

static uint8_t be_bnd_boundary_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint8_t be_bnd_sv48_target_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint8_t be_bnd_pmp_handler_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint8_t be_bnd_illegal_handler_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint8_t be_bnd_old_refill_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static volatile uint64_t be_bnd_store_pressure[256] __attribute__((aligned(64)));

volatile uint64_t be_bnd_scenario;
volatile uint64_t be_bnd_resume_pc;
volatile uint64_t be_bnd_sv48_satp_value;
volatile uint64_t be_bnd_m_count;
volatile uint64_t be_bnd_m_cause[BE_BND_LOG_DEPTH];
volatile uint64_t be_bnd_m_epc[BE_BND_LOG_DEPTH];
volatile uint64_t be_bnd_m_tval[BE_BND_LOG_DEPTH];
volatile uint64_t be_bnd_s_count;
volatile uint64_t be_bnd_s_cause[BE_BND_LOG_DEPTH];
volatile uint64_t be_bnd_s_epc[BE_BND_LOG_DEPTH];
volatile uint64_t be_bnd_s_tval[BE_BND_LOG_DEPTH];
volatile uint64_t be_bnd_rnmi_count;
volatile uint64_t be_bnd_rnmi_cause[BE_BND_LOG_DEPTH];
volatile uint64_t be_bnd_rnmi_epc[BE_BND_LOG_DEPTH];
volatile uint64_t be_bnd_rnmi_status[BE_BND_LOG_DEPTH];
volatile uint64_t be_bnd_direct_hits;
volatile uint64_t be_bnd_vectored_hits;

extern void be_bnd_m_trap(void);
extern void be_bnd_s_trap(void);
extern void be_bnd_s_direct_entry(void);
extern void be_bnd_s_vector_base(void);
extern void be_bnd_s_exit_trampoline(void);
extern void be_bnd_older_load_sequence(void);
extern void be_bnd_debug_resume(void);

asm(
    ".pushsection .text.be_bnd_traps,\"ax\",@progbits\n"
    BE_ASM_BEGIN
    ".balign 4\n"
    ".global be_bnd_s_exit_trampoline\n"
    "be_bnd_s_exit_trampoline:\n"
    /* ECALL is not interrupt-safe in Nanhu. Supply a real acceptance point. */
    "addi t0,t0,1\n"
    "ecall\n"
    "j be_bnd_s_exit_trampoline\n"
    ".global be_bnd_older_load_sequence\n"
    "be_bnd_older_load_sequence:\n"
    "ld t0,0(a0)\n"
    "jr a1\n"

    ".global be_bnd_s_direct_entry\n"
    "be_bnd_s_direct_entry:\n"
    "la t0,be_bnd_direct_hits\n"
    "ld t1,0(t0)\n"
    "addi t1,t1,1\n"
    "sd t1,0(t0)\n"
    "j be_bnd_s_trap\n"

    ".balign 64\n"
    ".global be_bnd_s_vector_base\n"
    "be_bnd_s_vector_base:\n"
    ".rept 9\n"
    "j be_bnd_s_trap\n"
    ".endr\n"
    "j be_bnd_s_vectored_entry\n"
    "be_bnd_s_vectored_entry:\n"
    "la t0,be_bnd_vectored_hits\n"
    "ld t1,0(t0)\n"
    "addi t1,t1,1\n"
    "sd t1,0(t0)\n"
    "j be_bnd_s_trap\n"

    ".balign 4\n"
    ".global be_bnd_s_trap\n"
    "be_bnd_s_trap:\n"
    "addi sp,sp,-64\n"
    "sd ra,0(sp)\n"
    "sd t0,8(sp)\n"
    "sd t1,16(sp)\n"
    "sd t2,24(sp)\n"
    "sd t3,32(sp)\n"
    "sd t4,40(sp)\n"
    "sd t5,48(sp)\n"
    "sd t6,56(sp)\n"
    "csrr t0,scause\n"
    "csrr t1,sepc\n"
    "csrr t2,stval\n"
    "la t3,be_bnd_s_count\n"
    "ld t4,0(t3)\n"
    "li t5,8\n"
    "bgeu t4,t5,.Lbe_bnd_s_log_done\n"
    "slli t5,t4,3\n"
    "la t6,be_bnd_s_cause\n"
    "add t6,t6,t5\n"
    "sd t0,0(t6)\n"
    "la t6,be_bnd_s_epc\n"
    "add t6,t6,t5\n"
    "sd t1,0(t6)\n"
    "la t6,be_bnd_s_tval\n"
    "add t6,t6,t5\n"
    "sd t2,0(t6)\n"
    ".Lbe_bnd_s_log_done:\n"
    "addi t4,t4,1\n"
    "sd t4,0(t3)\n"
    "bgez t0,.Lbe_bnd_s_exception\n"
    /* Clear software pending locally and wait for the event agent to lower
     * an external SEI before sret can expose the same level again. */
    "li t3,2\n"
    "csrc sip,t3\n"
    "li t3,4096\n"
    ".Lbe_bnd_s_wait_source:\n"
    "csrr t4,sip\n"
    "andi t4,t4,512\n"
    "beqz t4,.Lbe_bnd_s_source_down\n"
    "addi t3,t3,-1\n"
    "bnez t3,.Lbe_bnd_s_wait_source\n"
    ".Lbe_bnd_s_source_down:\n"
    "la t3,be_bnd_scenario\n"
    "ld t3,0(t3)\n"
    "li t4,0x5108\n"
    "bne t3,t4,.Lbe_bnd_s_not_sv48\n"
    "la t4,be_bnd_sv48_satp_value\n"
    "ld t4,0(t4)\n"
    "csrw satp,t4\n"
    "sfence.vma zero,zero\n"
    ".Lbe_bnd_s_not_sv48:\n"
    "li t4,0x5109\n"
    "bne t3,t4,.Lbe_bnd_s_return\n"
    "li t4,0x0000004000001000\n"
    "csrw sepc,t4\n"
    "j .Lbe_bnd_s_return\n"
    ".Lbe_bnd_s_exception:\n"
    /* A delegated final IPF exits through a low canonical S ecall. The M
     * handler then returns directly to the C continuation in M-mode. */
    "la t3,be_bnd_s_exit_trampoline\n"
    "csrw sepc,t3\n"
    ".Lbe_bnd_s_return:\n"
    "ld ra,0(sp)\n"
    "ld t0,8(sp)\n"
    "ld t1,16(sp)\n"
    "ld t2,24(sp)\n"
    "ld t3,32(sp)\n"
    "ld t4,40(sp)\n"
    "ld t5,48(sp)\n"
    "ld t6,56(sp)\n"
    "addi sp,sp,64\n"
    "sret\n"

    ".balign 4\n"
    ".global be_bnd_m_trap\n"
    "be_bnd_m_trap:\n"
    "addi sp,sp,-64\n"
    "sd ra,0(sp)\n"
    "sd t0,8(sp)\n"
    "sd t1,16(sp)\n"
    "sd t2,24(sp)\n"
    "sd t3,32(sp)\n"
    "sd t4,40(sp)\n"
    "sd t5,48(sp)\n"
    "sd t6,56(sp)\n"
    /* RNMI uses the same mtvec but owns mn* CSR state and returns by mnret. */
    "csrr t0,0x744\n"
    "andi t1,t0,8\n"
    "bnez t1,.Lbe_bnd_m_ordinary\n"
    "csrr t1,0x742\n"
    "bgez t1,.Lbe_bnd_m_ordinary\n"
    "slli t2,t1,1\n"
    "srli t2,t2,1\n"
    "li t3,31\n"
    "beq t2,t3,.Lbe_bnd_rnmi\n"
    "li t3,43\n"
    "bne t2,t3,.Lbe_bnd_m_ordinary\n"
    ".Lbe_bnd_rnmi:\n"
    "csrr t2,0x741\n"
    "la t3,be_bnd_rnmi_count\n"
    "ld t4,0(t3)\n"
    "li t5,8\n"
    "bgeu t4,t5,.Lbe_bnd_rnmi_log_done\n"
    "slli t5,t4,3\n"
    "la t6,be_bnd_rnmi_cause\n"
    "add t6,t6,t5\n"
    "sd t1,0(t6)\n"
    "la t6,be_bnd_rnmi_epc\n"
    "add t6,t6,t5\n"
    "sd t2,0(t6)\n"
    "la t6,be_bnd_rnmi_status\n"
    "add t6,t6,t5\n"
    "sd t0,0(t6)\n"
    ".Lbe_bnd_rnmi_log_done:\n"
    "addi t4,t4,1\n"
    "sd t4,0(t3)\n"
    "li t3,256\n"
    ".Lbe_bnd_rnmi_delay:\n"
    "addi t3,t3,-1\n"
    "bnez t3,.Lbe_bnd_rnmi_delay\n"
    "j .Lbe_bnd_m_restore_mnret\n"

    ".Lbe_bnd_m_ordinary:\n"
    "csrr t0,mcause\n"
    "csrr t1,mepc\n"
    "csrr t2,mtval\n"
    "la t3,be_bnd_m_count\n"
    "ld t4,0(t3)\n"
    "li t5,8\n"
    "bgeu t4,t5,.Lbe_bnd_m_log_done\n"
    "slli t5,t4,3\n"
    "la t6,be_bnd_m_cause\n"
    "add t6,t6,t5\n"
    "sd t0,0(t6)\n"
    "la t6,be_bnd_m_epc\n"
    "add t6,t6,t5\n"
    "sd t1,0(t6)\n"
    "la t6,be_bnd_m_tval\n"
    "add t6,t6,t5\n"
    "sd t2,0(t6)\n"
    ".Lbe_bnd_m_log_done:\n"
    "addi t4,t4,1\n"
    "sd t4,0(t3)\n"
    "bgez t0,.Lbe_bnd_m_exception\n"
    /* Wait until the event agent lowers MEI before returning to S-mode. */
    "li t3,4096\n"
    ".Lbe_bnd_m_wait_source:\n"
    "csrr t4,mip\n"
    "li t5,0x800\n"
    "and t4,t4,t5\n"
    "beqz t4,.Lbe_bnd_m_restore_mret\n"
    "addi t3,t3,-1\n"
    "bnez t3,.Lbe_bnd_m_wait_source\n"
    "j .Lbe_bnd_m_restore_mret\n"

    ".Lbe_bnd_m_exception:\n"
    "li t3,3\n"
    "bne t0,t3,.Lbe_bnd_m_not_breakpoint\n"
    "csrw 0x7a1,zero\n"
    "j .Lbe_bnd_m_restore_mret\n"
    ".Lbe_bnd_m_not_breakpoint:\n"
    "li t3,13\n"
    "bne t0,t3,.Lbe_bnd_m_finish\n"
    "la t4,be_bnd_scenario\n"
    "ld t4,0(t4)\n"
    "li t5,0x5112\n"
    "bne t4,t5,.Lbe_bnd_m_finish\n"
    "la t4,be_bnd_s_exit_trampoline\n"
    "csrw mepc,t4\n"
    "li t5,0x21800\n"
    "csrc mstatus,t5\n"
    "li t5,0x800\n"
    "csrs mstatus,t5\n"
    "j .Lbe_bnd_m_restore_mret\n"
    ".Lbe_bnd_m_finish:\n"
    "la t4,be_bnd_resume_pc\n"
    "ld t4,0(t4)\n"
    "csrw mepc,t4\n"
    "li t5,0x21800\n"
    "csrc mstatus,t5\n"
    "li t5,0x1800\n"
    "csrs mstatus,t5\n"
    ".Lbe_bnd_m_restore_mret:\n"
    "ld ra,0(sp)\n"
    "ld t0,8(sp)\n"
    "ld t1,16(sp)\n"
    "ld t2,24(sp)\n"
    "ld t3,32(sp)\n"
    "ld t4,40(sp)\n"
    "ld t5,48(sp)\n"
    "ld t6,56(sp)\n"
    "addi sp,sp,64\n"
    "mret\n"
    ".Lbe_bnd_m_restore_mnret:\n"
    "ld ra,0(sp)\n"
    "ld t0,8(sp)\n"
    "ld t1,16(sp)\n"
    "ld t2,24(sp)\n"
    "ld t3,32(sp)\n"
    "ld t4,40(sp)\n"
    "ld t5,48(sp)\n"
    "ld t6,56(sp)\n"
    "addi sp,sp,64\n"
    ".insn r 0x73,0,0x38,x0,x0,x2\n"
    BE_ASM_END
    ".popsection\n");

struct be_bnd_saved_state {
    uint64_t mstatus, mie, mideleg, medeleg, mtvec, stvec, satp;
    uint64_t tselect, tdata1, tdata2, mnstatus, vstart;
    uint64_t pmpcfg0, pmpaddr0, pmpaddr1;
};

static inline pte_t be_bnd_table_pte(const pte_t *next)
{
    return PTE_V | ((uintptr_t)next >> 2U);
}

static inline pte_t be_bnd_leaf_pte(uintptr_t paddr)
{
    return ((paddr & PAGE_ADDR_MSK) >> 2U) |
        PTE_V | PTE_AD | PTE_RWX;
}

static void be_bnd_zero_page(pte_t *page)
{
    for (unsigned i = 0; i < 512; ++i) page[i] = 0;
}

static void be_bnd_install_page_tables(void)
{
    be_bnd_zero_page(be_bnd_sv39_root);
    be_bnd_zero_page(be_bnd_sv39_high_l1);
    be_bnd_zero_page(be_bnd_sv39_high_l0);
    be_bnd_zero_page(be_bnd_sv48_root);
    be_bnd_zero_page(be_bnd_sv48_l2);
    be_bnd_zero_page(be_bnd_sv48_target_l1);
    be_bnd_zero_page(be_bnd_sv48_target_l0);

    for (unsigned i = 0; i < 4; ++i) {
        be_bnd_sv39_root[i] = be_bnd_leaf_pte((uintptr_t)i << 30U);
        be_bnd_sv48_l2[i] = be_bnd_leaf_pte((uintptr_t)i << 30U);
    }
    be_bnd_sv39_root[255] = be_bnd_table_pte(be_bnd_sv39_high_l1);
    be_bnd_sv39_high_l1[511] = be_bnd_table_pte(be_bnd_sv39_high_l0);
    be_bnd_sv39_high_l0[511] =
        be_bnd_leaf_pte((uintptr_t)be_bnd_boundary_page);

    be_bnd_sv48_root[0] = be_bnd_table_pte(be_bnd_sv48_l2);
    be_bnd_sv48_l2[256] = be_bnd_table_pte(be_bnd_sv48_target_l1);
    be_bnd_sv48_target_l1[0] = be_bnd_table_pte(be_bnd_sv48_target_l0);
    be_bnd_sv48_target_l0[0] =
        be_bnd_leaf_pte((uintptr_t)be_bnd_sv48_target_page);

    be_bnd_sv48_satp_value = (BE_BND_SATP_SV48 << 60U) |
        ((uintptr_t)be_bnd_sv48_root >> 12U);
    write32((uintptr_t)be_bnd_sv48_target_page, BE_BND_ECALL);
    write32((uintptr_t)be_bnd_pmp_handler_page, BE_BND_FENCE_RW_RW);
    write32((uintptr_t)be_bnd_illegal_handler_page, BE_BND_ILLEGAL);
    for (unsigned slot=0; slot<8; ++slot) {
        uint32_t *probe = (uint32_t *)(be_bnd_old_refill_page+256*slot);
        for (unsigned n=0; n<32; ++n) probe[n]=0x00000013;
        probe[0]=0x0080006f;
        probe[1]=0x00008067;
        probe[2]=0xfff00f13;
        probe[3]=0x00300f93;
        probe[4]=probe[5]=0x03ff5f33;
        probe[6]=0x000f3f33;
        probe[7]=0xfe0f04e3;
    }
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");
}

static void be_bnd_reset_log(unsigned scenario)
{
    be_bnd_scenario = scenario;
    be_bnd_resume_pc = 0;
    be_bnd_m_count = 0;
    be_bnd_s_count = 0;
    be_bnd_rnmi_count = 0;
    be_bnd_direct_hits = 0;
    be_bnd_vectored_hits = 0;
    for (unsigned i = 0; i < BE_BND_LOG_DEPTH; ++i) {
        be_bnd_m_cause[i] = be_bnd_m_epc[i] = be_bnd_m_tval[i] = 0;
        be_bnd_s_cause[i] = be_bnd_s_epc[i] = be_bnd_s_tval[i] = 0;
        be_bnd_rnmi_cause[i] = be_bnd_rnmi_epc[i] =
            be_bnd_rnmi_status[i] = 0;
    }
}

static void be_bnd_save_state(struct be_bnd_saved_state *saved)
{
    goto_priv(PRIV_M);
    saved->mstatus = CSRR(CSR_MSTATUS);
    saved->mie = CSRR(CSR_MIE);
    saved->mideleg = CSRR(CSR_MIDELEG);
    saved->medeleg = CSRR(CSR_MEDELEG);
    saved->mtvec = CSRR(CSR_MTVEC);
    saved->stvec = CSRR(CSR_STVEC);
    saved->satp = CSRR(CSR_SATP);
    saved->tselect = CSRR(CSR_TSELECT);
    CSRW(CSR_TSELECT, 0);
    saved->tdata1 = CSRR(CSR_TDATA1);
    saved->tdata2 = CSRR(CSR_TDATA2);
    saved->mnstatus = CSRR(CSR_MNSTATUS);
    saved->vstart = CSRR(CSR_VSTART);
    saved->pmpcfg0 = CSRR(CSR_PMPCFG0);
    saved->pmpaddr0 = CSRR(CSR_PMPADDR0);
    saved->pmpaddr1 = CSRR(CSR_PMPADDR1);
}

static void be_bnd_restore_state(const struct be_bnd_saved_state *saved)
{
    curr_priv = PRIV_M;
    real_priv = PRIV_M;
    CSRW(CSR_SATP, 0);
    sfence_vma();
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, saved->pmpaddr0);
    CSRW(CSR_PMPADDR1, saved->pmpaddr1);
    CSRW(CSR_PMPCFG0, saved->pmpcfg0);
    CSRW(CSR_TSELECT, 0);
    CSRW(CSR_TDATA1, 0);
    CSRW(CSR_TDATA2, saved->tdata2);
    CSRW(CSR_TDATA1, saved->tdata1);
    CSRW(CSR_TSELECT, saved->tselect);
    CSRW(CSR_VSTART, saved->vstart);
    CSRW(CSR_MNSTATUS, saved->mnstatus);
    CSRW(CSR_STVEC, saved->stvec);
    CSRW(CSR_MTVEC, saved->mtvec);
    CSRW(CSR_MEDELEG, saved->medeleg);
    CSRW(CSR_MIDELEG, saved->mideleg);
    CSRW(CSR_MIE, saved->mie);
    CSRW(CSR_SATP, saved->satp);
    sfence_vma();
    CSRW(CSR_MSTATUS, saved->mstatus);
}

static void be_bnd_prepare(unsigned scenario)
{
    curr_priv = PRIV_M;
    real_priv = PRIV_M;
    CSRC(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE | MSTATUS_MPRV | MSTATUS_TVM);
    CSRW(CSR_SATP, 0);
    sfence_vma();
    CSRW(CSR_MEDELEG, 0);
    CSRW(CSR_MIDELEG, 0);
    CSRW(CSR_MIE, 0);
    CSRW(CSR_SIP, 0);
    CSRW(CSR_TSELECT, 0);
    CSRW(CSR_TDATA1, 0);
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, 0x0f);
    CSRW(CSR_MTVEC, (uintptr_t)be_bnd_m_trap);
    CSRW(CSR_STVEC, (uintptr_t)be_bnd_s_trap);
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
    be_bnd_reset_log(scenario);
}

static inline uint64_t be_bnd_sv39_satp(void)
{
    return (BE_BND_SATP_SV39 << 60U) |
        ((uintptr_t)be_bnd_sv39_root >> 12U);
}

static uintptr_t be_bnd_refill_target(void)
{
    unsigned slot = ((be_bnd_scenario >> 4) & 1)*4 + (be_bnd_scenario & 3);
    return (uintptr_t)be_bnd_old_refill_page + 256*slot + 64;
}

static inline void be_bnd_prefetch_i(uintptr_t address)
{
    /* Fetch the target through an unresolved, actually taken branch. The
     * return is on the first line, so a held second line cannot deadlock it. */
    asm volatile(BE_ASM_BEGIN "jalr ra,0(%0)\n" BE_ASM_END
        :: "r"(address-64) : "ra","t5","t6","memory");
}

static void be_bnd_execute(uint32_t instruction, uintptr_t entry,
    uint64_t t0_seed, uintptr_t prefetch, bool store_pressure)
{
    bool direct_ssip = be_bnd_scenario == BE_IT_SC_BOUNDARY_IRQ_DISABLE &&
        instruction == BE_BND_CSRC_SIP_T0;
    write32((uintptr_t)be_bnd_boundary_page + PAGE_SIZE - 4U, instruction);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");
    CSRW(CSR_SATP, be_bnd_sv39_satp());
    sfence_vma();
    if (direct_ssip) {
        /* Keep SSIP masked by privilege until mret enters the target CSR.
         * Taking the usual S-mode setup path would accept it before I0. */
        CSRC(CSR_MSTATUS, 3ULL << 11);
        CSRS(CSR_MSTATUS, 1ULL << 11);
        CSRW(CSR_MEPC, entry);
        curr_priv = PRIV_HS;
        real_priv = PRIV_HS;
    } else {
        goto_priv(PRIV_HS);
    }

    if (prefetch != 0) {
        be_bnd_prefetch_i(prefetch);
    }
    if (store_pressure) {
        for (unsigned i = 0; i < 256; ++i)
            be_bnd_store_pressure[i] = i ^ 0x783ULL;
    }

    asm volatile(
        BE_ASM_BEGIN
        "lla t1,1f\n"
        "la t2,be_bnd_resume_pc\n"
        "sd t1,0(t2)\n"
        "mv t0,%[seed]\n"
        "mv a0,%[fault]\n"
        "mv a1,%[boundary]\n"
        "beqz %[ssip],2f\n"
        "li t1,2\n"
        "csrs mip,t1\n"
        "mret\n"
        "2:\n"
        "jr %[entry]\n"
        "1:\n"
        BE_ASM_END
        :: [seed] "r"(t0_seed), [fault] "r"(BE_BND_DATA_FAULT_VA),
           [boundary] "r"(BE_BND_SV39_I0), [entry] "r"(entry),
           [ssip] "r"(direct_ssip)
        : "t0", "t1", "t2", "a0", "a1", "memory");
    curr_priv = PRIV_M;
    real_priv = PRIV_M;
}

static __attribute__((noinline, noclone)) bool be_bnd_execute_debug(
    unsigned row, unsigned scenario, uint32_t instruction,
    uintptr_t target_pc, uintptr_t target_addr)
{
    write32((uintptr_t)be_bnd_boundary_page + PAGE_SIZE - 4U, instruction);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");

    backend_it_control.target_pc = target_pc;
    backend_it_control.target_addr = target_addr;
    /* Debug failure recovery resumes with dcsr.prv=S. Enter through an S-mode
     * ecall so the trap handler returns to the C continuation in M-mode. */
    backend_it_control.auxiliary = (uintptr_t)be_bnd_s_exit_trampoline;
    be_bnd_resume_pc = (uintptr_t)be_bnd_debug_resume;
    CSRW(CSR_SATP, be_bnd_sv39_satp());
    sfence_vma();

    if (!be_it_prepare_scenario(row, scenario, 0)) return false;
    be_it_activate();
    goto_priv(PRIV_HS);

    asm volatile(
        BE_ASM_BEGIN
        "mv t0,zero\n"
        "li a0,%[fault]\n"
        "li a1,%[boundary]\n"
        "jr %[entry]\n"
        ".global be_bnd_debug_resume\n"
        "be_bnd_debug_resume:\n"
        BE_ASM_END
        :: [fault] "i"(BE_BND_DATA_FAULT_VA),
           [boundary] "i"(BE_BND_SV39_I0), [entry] "r"(BE_BND_SV39_I0)
        : "t0", "a0", "a1", "memory");
    curr_priv = PRIV_M;
    real_priv = PRIV_M;
    return true;
}

static bool be_bnd_m_has(uint64_t cause, uint64_t epc,
    uint64_t tval, bool check_tval)
{
    unsigned count = be_bnd_m_count < BE_BND_LOG_DEPTH ?
        (unsigned)be_bnd_m_count : BE_BND_LOG_DEPTH;
    for (unsigned i = 0; i < count; ++i)
        if (be_bnd_m_cause[i] == cause && be_bnd_m_epc[i] == epc &&
            (!check_tval || be_bnd_m_tval[i] == tval)) return true;
    return false;
}

static bool be_bnd_s_has(uint64_t cause, uint64_t epc,
    uint64_t tval, bool check_tval)
{
    unsigned count = be_bnd_s_count < BE_BND_LOG_DEPTH ?
        (unsigned)be_bnd_s_count : BE_BND_LOG_DEPTH;
    for (unsigned i = 0; i < count; ++i)
        if (be_bnd_s_cause[i] == cause && be_bnd_s_epc[i] == epc &&
            (!check_tval || be_bnd_s_tval[i] == tval)) return true;
    return false;
}

static inline void be_bnd_enable_mei(void)
{
    CSRW(CSR_MIDELEG, 0);
    CSRW(CSR_MIE, MIE_MEIE);
}

static inline void be_bnd_enable_sei(void)
{
    CSRS(CSR_MIDELEG, 1ULL << 9);
    CSRS(CSR_SIE, SIE_SEIE);
    CSRS(CSR_SSTATUS, MSTATUS_SIE);
}

static inline void be_bnd_set_descriptor(uintptr_t target_pc,
    uintptr_t target_addr)
{
    backend_it_control.target_pc = target_pc;
    backend_it_control.target_addr = target_addr;
    backend_it_control.auxiliary = 0;
}

static bool be_bnd_final_ipf(void)
{
    return be_bnd_m_has(CAUSE_IPF, BE_BND_SV39_I1,
        BE_BND_SV39_I1, true);
}

bool ai_micro_backend_it_boundary_refill_cross(void)
{
    TEST_START();
    be_it_enable();
    struct be_bnd_saved_state saved;
    be_bnd_save_state(&saved);
    be_bnd_install_page_tables();

    be_bnd_prepare(BE_IT_SC_NONE);
    be_bnd_set_descriptor(BE_BND_SV39_I0, 0);
    if (!be_it_begin_scenario(783, BE_IT_SC_NONE, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, true);
    TEST_ASSERT("row 783: FENCE waits for the store-buffer tail and preserves the next-PC IPF",
        be_bnd_final_ipf());
    TEST_ASSERT("row 783: the RTL witness observed the real SBuffer wait and boundary redirect",
        be_it_finish());

    static const unsigned refill_scenarios[] = {
        BE_IT_SC_BOUNDARY_FENCEI_REFILL_EARLY,
        BE_IT_SC_BOUNDARY_FENCEI_REFILL_LATE
    };
    for (unsigned i = 0; i < 2; ++i) {
        be_bnd_prepare(refill_scenarios[i]);
        be_bnd_set_descriptor(BE_BND_SV39_I0,
            be_bnd_refill_target());
        if (!be_it_begin_scenario(784, refill_scenarios[i], i)) {
            be_bnd_restore_state(&saved);
            return false;
        }
        be_bnd_execute(BE_BND_FENCE_I, BE_BND_SV39_I0, 0,
            be_bnd_refill_target(), i == 0);
        TEST_ASSERT("row 784: FENCE.I keeps the boundary IPF across the selected old-refill ordering",
            be_bnd_final_ipf(), "scenario=0x%x", refill_scenarios[i]);
        TEST_ASSERT("row 784: the refill agent and instruction-identity witness both completed",
            be_it_finish(), "scenario=0x%x", refill_scenarios[i]);
    }

    static const unsigned flush_scenarios[] = {
        BE_IT_SC_FLUSH_OLD_RESPONSE_EARLY,
        BE_IT_SC_FLUSH_OLD_RESPONSE_SAME,
        BE_IT_SC_FLUSH_OLD_RESPONSE_LATE
    };
    for (unsigned i = 0; i < 3; ++i) {
        be_bnd_prepare(flush_scenarios[i]);
        CSRS(CSR_MIDELEG, MIE_SSIE);
        CSRS(CSR_SIE, SIE_SSIE);
        be_bnd_set_descriptor(BE_BND_SV39_I0,
            be_bnd_refill_target());
        if (!be_it_begin_scenario(798, flush_scenarios[i], i)) {
            be_bnd_restore_state(&saved);
            return false;
        }
        be_bnd_execute(BE_BND_CSRW_SIE_ZERO, BE_BND_SV39_I0, 0,
            be_bnd_refill_target(), false);
        TEST_ASSERT("row 798: stale fetch response ordering does not clear or move the target IPF",
            be_bnd_final_ipf(), "scenario=0x%x", flush_scenarios[i]);
        TEST_ASSERT("row 798: the selected early/same/late response and frontend stall were witnessed",
            be_it_finish(), "scenario=0x%x", flush_scenarios[i]);
    }

    be_bnd_restore_state(&saved);
    TEST_END("ai_micro_backend_it_boundary_refill_cross");
}

static bool be_bnd_run_mei_ipf(unsigned row, unsigned scenario,
    uint32_t instruction, uintptr_t target_pc, uint64_t t0_seed,
    uintptr_t prefetch)
{
    be_bnd_prepare(scenario);
    be_bnd_enable_mei();
    be_bnd_set_descriptor(target_pc, prefetch);
    if (!be_it_begin_scenario(row, scenario, 0)) return false;
    be_bnd_execute(instruction, BE_BND_SV39_I0, t0_seed, prefetch, false);
    return be_bnd_m_has(CAUSE_MEI, BE_BND_SV39_I1, 0, false) &&
        be_bnd_final_ipf();
}

bool ai_micro_backend_it_boundary_irq_cross(void)
{
    TEST_START();
    be_it_enable();
    struct be_bnd_saved_state saved;
    be_bnd_save_state(&saved);
    be_bnd_install_page_tables();

    bool ok = be_bnd_run_mei_ipf(803, BE_IT_SC_BOUNDARY_IRQ_TARGET,
        BE_BND_FENCE_RW_RW, BE_BND_SV39_I1, 0, 0);
    TEST_ASSERT("row 803: accepted MEI saves I1 and the returned boundary still reports IPF", ok);
    TEST_ASSERT("row 803: monitor aligned the accepted interrupt with the fault-bearing ROB entry",
        be_it_finish());

    /* Nonzero vstart makes vector arithmetic illegal in this RTL, which
     * suppresses the ordinary flushPipe required by row 807. */
    printf("\nBE_IT_EXCLUDED row=807 reason=nonzero_vstart_vector_arithmetic_illegal\n");
    {
        unsigned row = 808;
        unsigned scenario = BE_IT_SC_BOUNDARY_IRQ_LATE;
        be_bnd_prepare(scenario);
        be_bnd_enable_mei();
        CSRW(CSR_VSTART, 0);
        be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
        if (!be_it_begin_scenario(row, scenario, 0)) {
            be_bnd_restore_state(&saved);
            return false;
        }
        be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
        ok = be_bnd_m_count == 2 && be_bnd_m_cause[0] == CAUSE_MEI &&
             be_bnd_m_epc[0] == BE_BND_SV39_I1 &&
             be_bnd_m_cause[1] == CAUSE_IPF &&
             be_bnd_m_epc[1] == BE_BND_SV39_I1;
        ok &= be_bnd_final_ipf() && CSRR(CSR_VSTART) == 0;
        TEST_ASSERT("row 808: the requested flush/IRQ ordering retains the returned I1 IPF",
            ok, "row=%u scenario=0x%x count=%lu cause0=%lx epc0=%lx cause1=%lx epc1=%lx vstart=%lu",
            row, scenario, be_bnd_m_count, be_bnd_m_cause[0], be_bnd_m_epc[0],
            be_bnd_m_cause[1], be_bnd_m_epc[1], CSRR(CSR_VSTART));
        TEST_ASSERT("row 808: RTL observed the late interrupt window",
            be_it_finish(), "row=%u scenario=0x%x", row, scenario);
    }

    be_bnd_prepare(BE_IT_SC_BOUNDARY_IRQ_ENABLE);
    CSRS(CSR_MIDELEG, MIE_SSIE);
    CSRS(CSR_SIE, SIE_SSIE);
    CSRS(CSR_SIP, SIP_SSIP);
    CSRC(CSR_SSTATUS, MSTATUS_SIE);
    be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
    if (!be_it_begin_scenario(809, BE_IT_SC_BOUNDARY_IRQ_ENABLE, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_CSRSI_SSTATUS_SIE, BE_BND_SV39_I0, 0, 0, false);
    ok = be_bnd_s_has(CAUSE_INT_BIT | 1ULL, BE_BND_SV39_I1, 0, false) &&
        be_bnd_final_ipf();
    TEST_ASSERT("row 809: enabling SIE takes effect after the CSR flush and does not consume I1 IPF", ok);
    TEST_ASSERT("row 809: RTL associated the enabled interrupt entry with the boundary target",
        be_it_finish());

    static const uint32_t clear_instructions[] = {
        BE_BND_CSRCI_SSTATUS_SIE, BE_BND_CSRC_SIE_T0, BE_BND_CSRC_SIP_T0
    };
    static const uint64_t clear_values[] = {0, SIE_SEIE, SIP_SSIP};
    for (unsigned i = 0; i < 3; ++i) {
        be_bnd_prepare(BE_IT_SC_BOUNDARY_IRQ_DISABLE);
        if (i == 2) {
            CSRS(CSR_MIDELEG, MIE_SSIE);
            CSRS(CSR_SIE, SIE_SSIE);
            CSRS(CSR_SSTATUS, MSTATUS_SIE);
        } else {
            be_bnd_enable_sei();
        }
        be_bnd_set_descriptor(BE_BND_SV39_I0, 0);
        if (!be_it_begin_scenario(810, BE_IT_SC_BOUNDARY_IRQ_DISABLE, i)) {
            be_bnd_restore_state(&saved);
            return false;
        }
        be_bnd_execute(clear_instructions[i], BE_BND_SV39_I0,
            clear_values[i], 0, false);
        ok = be_bnd_s_count == 0 && !be_bnd_m_has(CAUSE_MEI,
            BE_BND_SV39_I1, 0, false) && be_bnd_final_ipf();
        TEST_ASSERT("row 810: clearing global enable, local enable, or pending source suppresses the stale interrupt but not IPF",
            ok, "variant=%u", i);
        TEST_ASSERT("row 810: RTL observed pending at the CSR and clear state at the target",
            be_it_finish(), "variant=%u", i);
    }

    be_bnd_prepare(BE_IT_SC_BOUNDARY_IRQ_CLEAR);
    be_bnd_enable_mei();
    be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
    if (!be_it_begin_scenario(811, BE_IT_SC_BOUNDARY_IRQ_CLEAR, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
    ok = be_bnd_m_count == 1 && be_bnd_final_ipf();
    TEST_ASSERT("row 811: a source cleared before acceptance causes no handler entry and preserves I1 IPF", ok);
    TEST_ASSERT("row 811: agent proved pending then clear before the target became selectable",
        be_it_finish());

    ok = be_bnd_run_mei_ipf(812, BE_IT_SC_BOUNDARY_IRQ_ACCEPT_CLEAR,
        BE_BND_FENCE_RW_RW, BE_BND_SV39_I1, 0, 0);
    TEST_ASSERT("row 812: clearing an already accepted source does not withdraw its redirect or duplicate it", ok);
    TEST_ASSERT("row 812: accepted-event and source-clear timing were both witnessed",
        be_it_finish());

    be_bnd_prepare(BE_IT_SC_BOUNDARY_BREAKPOINT_IRQ);
    be_bnd_enable_mei();
    CSRS(CSR_MIDELEG, MIE_SSIE);
    CSRS(CSR_SIE, SIE_SSIE);
    CSRW(CSR_TSELECT, 0);
    CSRW(CSR_TDATA2, BE_BND_SV39_I0);
    CSRW(CSR_TDATA1, BE_BND_MCONTROL6_S_EXEC_BKP);
    be_bnd_set_descriptor(BE_BND_SV39_I0, 0);
    if (!be_it_begin_scenario(816, BE_IT_SC_BOUNDARY_BREAKPOINT_IRQ, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    /* Scenario 0x4008 asserts real MEIP before READY, while M-mode masks
     * acceptance. Enter the unsafe CSR directly: the ordinary S-mode
     * helper prologue would otherwise consume this interrupt first.
     */
    write32((uintptr_t)be_bnd_boundary_page + PAGE_SIZE - 4U,
        BE_BND_CSRW_SIE_ZERO);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");
    CSRW(CSR_SATP, be_bnd_sv39_satp());
    sfence_vma();
    bool breakpoint_source_pending = (CSRR(CSR_MIP) & MIE_MEIE) != 0;
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, 1ULL << 11);
    CSRW(CSR_MEPC, BE_BND_SV39_I0);
    curr_priv = PRIV_HS;
    real_priv = PRIV_HS;
    asm volatile(BE_ASM_BEGIN
        "lla t0,1f\nla t1,be_bnd_resume_pc\nsd t0,0(t1)\n"
        "mret\n1:\n" BE_ASM_END ::: "t0", "t1", "memory");
    curr_priv = PRIV_M;
    real_priv = PRIV_M;
    ok = be_bnd_m_has(CAUSE_BKP, BE_BND_SV39_I0,
        BE_BND_SV39_I0, false) && be_bnd_final_ipf() &&
        !be_bnd_m_has(CAUSE_MEI, BE_BND_SV39_I0, 0, false) &&
        CSRR(CSR_SIE) == 0 && breakpoint_source_pending &&
        be_bnd_m_cause[0] == CAUSE_BKP && be_bnd_m_epc[0] == BE_BND_SV39_I0;
    TEST_ASSERT("row 816: breakpoint wins at the non-interrupt-safe CSR, suppresses its first side effect, then retry reaches IPF", ok);
    TEST_ASSERT("row 816: breakpoint decision and pending external source were observed on the same target",
        be_it_finish());

    be_bnd_prepare(BE_IT_SC_BOUNDARY_IRQ_M_IPF_S);
    be_bnd_enable_mei();
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_IPF);
    be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
    if (!be_it_begin_scenario(821, BE_IT_SC_BOUNDARY_IRQ_M_IPF_S, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
    ok = be_bnd_m_has(CAUSE_MEI, BE_BND_SV39_I1, 0, false) &&
        be_bnd_s_has(CAUSE_IPF, BE_BND_SV39_I1, BE_BND_SV39_I1, true);
    TEST_ASSERT("row 821: M interrupt followed by delegated S IPF updates two independent CSR banks", ok);
    TEST_ASSERT("row 821: monitor matched the M-entry and returned S-IPF generations",
        be_it_finish());

    be_bnd_prepare(BE_IT_SC_BOUNDARY_IRQ_S_IPF_M);
    be_bnd_enable_sei();
    be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
    if (!be_it_begin_scenario(821, BE_IT_SC_BOUNDARY_IRQ_S_IPF_M, 1)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
    ok = be_bnd_s_has(CAUSE_SEI, BE_BND_SV39_I1, 0, false) &&
        be_bnd_final_ipf();
    TEST_ASSERT("row 821: delegated S interrupt followed by nondelegated M IPF keeps entry privilege distinct", ok);
    TEST_ASSERT("row 821: monitor matched the S-entry and returned M-IPF generations",
        be_it_finish());

    static const unsigned entry_scenarios[] = {
        BE_IT_SC_BOUNDARY_IRQ_DIRECT, BE_IT_SC_BOUNDARY_IRQ_VECTORED
    };
    for (unsigned i = 0; i < 2; ++i) {
        be_bnd_prepare(entry_scenarios[i]);
        be_bnd_enable_sei();
        CSRW(CSR_STVEC, i == 0 ? (uintptr_t)be_bnd_s_direct_entry :
            ((uintptr_t)be_bnd_s_vector_base | 1ULL));
        be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
        if (!be_it_begin_scenario(822, entry_scenarios[i], i)) {
            be_bnd_restore_state(&saved);
            return false;
        }
        be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
        ok = be_bnd_s_has(CAUSE_SEI, BE_BND_SV39_I1, 0, false) &&
            be_bnd_final_ipf() && (i == 0 ? be_bnd_direct_hits == 1 &&
            be_bnd_vectored_hits == 0 : be_bnd_direct_hits == 0 &&
            be_bnd_vectored_hits == 1);
        TEST_ASSERT("row 822: Direct and Vectored SEI enter their distinct first instruction and return to the same IPF",
            ok, "mode=%u", i);
        TEST_ASSERT("row 822: CSR entry target and vector-mode witness matched the requested mode",
            be_it_finish(), "mode=%u", i);
    }

    be_bnd_prepare(BE_IT_SC_BOUNDARY_IRQ_VECTOR_IPF);
    be_bnd_enable_sei();
    CSRW(CSR_STVEC, BE_BND_SV39_I0 | 1ULL);
    be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
    if (!be_it_begin_scenario(823, BE_IT_SC_BOUNDARY_IRQ_VECTOR_IPF, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
    const uintptr_t vector_ipf = BE_BND_SV39_I0 + 4U * 9U;
    ok = be_bnd_m_has(CAUSE_IPF, vector_ipf, vector_ipf, true);
    TEST_ASSERT("row 823: Vectored offset crosses the Sv39 boundary and replaces old I1 trap metadata", ok);
    TEST_ASSERT("row 823: monitor checked the final target, not only stvec.BASE",
        be_it_finish());

    be_bnd_prepare(BE_IT_SC_BOUNDARY_HANDLER_IAF);
    be_bnd_enable_sei();
    uintptr_t denied_handler = (uintptr_t)be_bnd_pmp_handler_page;
    CSRW(CSR_STVEC, denied_handler);
    CSRW(CSR_PMPCFG0, 0);
    CSRW(CSR_PMPADDR0, (denied_handler >> 2U) | 0x1ffULL);
    CSRW(CSR_PMPADDR1, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, 0x1f18ULL);
    be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
    if (!be_it_begin_scenario(824, BE_IT_SC_BOUNDARY_HANDLER_IAF, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
    ok = be_bnd_m_has(CAUSE_IAF, denied_handler, denied_handler, true);
    TEST_ASSERT("row 824: handler PMP denial raises a fresh IAF at the handler, not the old boundary IPF", ok);
    TEST_ASSERT("row 824: monitor associated the first handler exception with the accepted interrupt",
        be_it_finish());

    be_bnd_prepare(BE_IT_SC_BOUNDARY_HANDLER_ILLEGAL);
    be_bnd_enable_sei();
    uintptr_t illegal_handler = (uintptr_t)be_bnd_illegal_handler_page;
    CSRW(CSR_STVEC, illegal_handler);
    be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
    if (!be_it_begin_scenario(825, BE_IT_SC_BOUNDARY_HANDLER_ILLEGAL, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
    ok = be_bnd_m_has(CAUSE_ILLEGAL_INSTRUCTION, illegal_handler, 0, false);
    TEST_ASSERT("row 825: an executable handler page with 0xffffffff raises illegal instruction at its own PC", ok);
    TEST_ASSERT("row 825: monitor kept the handler decode exception separate from the interrupted IPF",
        be_it_finish());

    be_bnd_prepare(BE_IT_SC_BOUNDARY_RETURN_SV48);
    be_bnd_enable_sei();
    be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
    if (!be_it_begin_scenario(826, BE_IT_SC_BOUNDARY_RETURN_SV48, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
    ok = be_bnd_m_has(CAUSE_ECS, BE_BND_SV39_I1, 0, false) &&
        !be_bnd_m_has(CAUSE_IPF, BE_BND_SV39_I1, 0, false);
    TEST_ASSERT("row 826: S handler switches to Sv48 and executes the instruction at formerly noncanonical I1", ok);
    TEST_ASSERT("row 826: xRET witness used the new satp mode and cleared stale backend-fault metadata",
        be_it_finish());

    be_bnd_prepare(BE_IT_SC_BOUNDARY_RETURN_NEW_IPF);
    be_bnd_enable_sei();
    be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
    if (!be_it_begin_scenario(827, BE_IT_SC_BOUNDARY_RETURN_NEW_IPF, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
    ok = be_bnd_m_has(CAUSE_IPF, BE_BND_SV39_NEW_I1,
        BE_BND_SV39_NEW_I1, true);
    TEST_ASSERT("row 827: sret to a different noncanonical address refreshes epc and tval", ok);
    TEST_ASSERT("row 827: monitor bound the returned IPF to the new xRET target",
        be_it_finish());

    be_bnd_prepare(BE_IT_SC_BOUNDARY_OLDER_LOAD_IRQ);
    be_bnd_enable_mei();
    CSRS(CSR_MIDELEG, MIE_SSIE);
    CSRS(CSR_SIE, SIE_SSIE);
    uint64_t sie_before = CSRR(CSR_SIE);
    /* The old indirect jump reached the CSR after the LPF had already
     * redirected. Put ld and CSR in the same boundary fetch line so the
     * CSR can wait in dispatch behind this exact faulting load.
     */
    const uintptr_t boundary_load_pc = BE_BND_SV39_I0 - 4U;
    write32((uintptr_t)be_bnd_boundary_page + PAGE_SIZE - 8U,
        0x00053283U); /* ld t0,0(a0); a0 is the helper's fault address. */
    be_bnd_set_descriptor(boundary_load_pc, 0);
    if (!be_it_begin_scenario(829, BE_IT_SC_BOUNDARY_OLDER_LOAD_IRQ, 0)) {
        be_bnd_restore_state(&saved);
        return false;
    }
    be_bnd_execute(BE_BND_CSRW_SIE_T0,
        boundary_load_pc, 0, 0, false);
    ok = be_bnd_m_has(CAUSE_LPF, boundary_load_pc,
            BE_BND_DATA_FAULT_VA, true) &&
        be_bnd_m_has(CAUSE_MEI, (uintptr_t)be_bnd_s_exit_trampoline,
            0, false) && !be_bnd_m_has(CAUSE_IPF, BE_BND_SV39_I1, 0, false) &&
        CSRR(CSR_SIE) == sie_before && be_bnd_m_count == 3 &&
        be_bnd_m_cause[0] == CAUSE_LPF && be_bnd_m_cause[1] == CAUSE_MEI &&
        be_bnd_m_cause[2] == CAUSE_ECS &&
        be_bnd_m_epc[2] == (uintptr_t)be_bnd_s_exit_trampoline + 4U;
    TEST_ASSERT("row 829: older load fault kills the waiting boundary CSR, whose side effect and candidate IPF never retire", ok);
    TEST_ASSERT("row 829: pending MEI is accepted only at the later real safe point",
        be_it_finish());

    ok = be_bnd_run_mei_ipf(830, BE_IT_SC_BOUNDARY_TARGET_NOT_ROB,
        BE_BND_FENCE_RW_RW, BE_BND_SV39_I1, 0, 0);
    TEST_ASSERT("row 830: pending interrupt while I1 is outside ROB neither invents an old-head event nor loses IPF", ok);
    TEST_ASSERT("row 830: monitor observed empty ROB, retained fault epoch, and later legal acceptance",
        be_it_finish());

    ok = be_bnd_run_mei_ipf(831, BE_IT_SC_BOUNDARY_HANDLER_OLD_REFILL,
        BE_BND_FENCE_RW_RW, BE_BND_SV39_I1, 0,
        be_bnd_refill_target());
    TEST_ASSERT("row 831: an old refill arriving around handler redirect does not contaminate handler or returned I1", ok);
    TEST_ASSERT("row 831: event and refill agents both checked the same redirect generation",
        be_it_finish());

    be_bnd_restore_state(&saved);
    TEST_END("ai_micro_backend_it_boundary_irq_cross");
}

bool ai_micro_backend_it_boundary_debug_cross(void)
{
    TEST_START();
    be_it_enable();
    struct be_bnd_saved_state saved;
    be_bnd_save_state(&saved);
    be_bnd_install_page_tables();

    static const unsigned rows[] = {817, 818, 819};
    static const unsigned scenarios[] = {
        BE_IT_SC_DEBUG_BOUNDARY_TRIGGER,
        BE_IT_SC_DEBUG_BOUNDARY_STEP,
        BE_IT_SC_DEBUG_BOUNDARY_HALTREQ
    };
    for (unsigned i = 0; i < 3; ++i) {
        be_bnd_prepare(scenarios[i]);
        CSRS(CSR_MIDELEG, MIE_SSIE);
        CSRS(CSR_SIE, SIE_SSIE);
        uintptr_t target_pc = i == 2 ? BE_BND_SV39_I1 : BE_BND_SV39_I0;
        uintptr_t target_addr = i == 1 ? BE_BND_SV39_I1 : 0;
        if (!be_bnd_execute_debug(rows[i], scenarios[i],
                BE_BND_CSRW_SIE_ZERO, target_pc, target_addr)) {
            be_bnd_restore_state(&saved);
            return false;
        }
        bool ok = be_bnd_final_ipf() && CSRR(CSR_SIE) == 0;
        TEST_ASSERT("rows 817-819: Debug entry has no stale IPF and resume executes I0 once before the real I1 fault",
            ok, "row=%u scenario=0x%x", rows[i], scenarios[i]);
        TEST_ASSERT("rows 817-819: JTAG agent checked dpc/dcsr cause and completed resume",
            be_it_finish_timeout(4096), "row=%u scenario=0x%x",
            rows[i], scenarios[i]);
    }

    be_bnd_restore_state(&saved);
    TEST_END("ai_micro_backend_it_boundary_debug_cross");
}

bool ai_micro_backend_it_boundary_rnmi_cross(void)
{
    TEST_START();
    be_it_enable();
    struct be_bnd_saved_state saved;
    be_bnd_save_state(&saved);
    be_bnd_install_page_tables();

    for (unsigned source = 0; source < 2; ++source) {
        be_bnd_prepare(BE_IT_SC_BOUNDARY_RNMI);
        CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
        be_bnd_set_descriptor(BE_BND_SV39_I1, 0);
        if (!be_it_begin_scenario(820, BE_IT_SC_BOUNDARY_RNMI, source)) {
            be_bnd_restore_state(&saved);
            return false;
        }
        be_bnd_execute(BE_BND_FENCE_RW_RW, BE_BND_SV39_I0, 0, 0, false);
        uint64_t expected = source ? BE_BND_NMI43 : BE_BND_NMI31;
        bool ok = be_bnd_rnmi_count == 1 &&
            be_bnd_rnmi_cause[0] == expected &&
            be_bnd_rnmi_epc[0] == BE_BND_SV39_I1 &&
            !(be_bnd_rnmi_status[0] & MNSTATUS_NMIE) &&
            be_bnd_final_ipf();
        TEST_ASSERT("row 820: NMI31/NMI43 owns mn* state, returns by mnret, and leaves the target IPF live",
            ok, "source=%u mncause=0x%lx mnepc=0x%lx",
            source, be_bnd_rnmi_cause[0], be_bnd_rnmi_epc[0]);
        TEST_ASSERT("row 820: event agent matched and cleared the selected RNMI source",
            be_it_finish(), "source=%u", source);
    }

    be_bnd_restore_state(&saved);
    TEST_END("ai_micro_backend_it_boundary_rnmi_cross");
}
