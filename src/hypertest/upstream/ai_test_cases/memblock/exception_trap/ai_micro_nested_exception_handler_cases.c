#include <rvh_test.h>
#include <csrs.h>
#include <stdint.h>

static volatile uint64_t ai_micro_nested_exception_depth;
static volatile uint64_t ai_micro_nested_exception_outer_cause;
static volatile uint64_t ai_micro_nested_exception_outer_epc;
static volatile uint64_t ai_micro_nested_exception_outer_tval;
static volatile uint64_t ai_micro_nested_exception_inner_cause;
static volatile uint64_t ai_micro_nested_exception_inner_epc;
static volatile uint64_t ai_micro_nested_exception_inner_tval;
static volatile uint64_t ai_micro_nested_exception_fell_through;

static volatile uint64_t ai_micro_no_mepc_save_depth;
static volatile uint64_t ai_micro_no_mepc_save_outer_cause;
static volatile uint64_t ai_micro_no_mepc_save_nested_cause;
static volatile uint64_t ai_micro_no_mepc_save_nested_epc;
static volatile uint64_t ai_micro_no_mepc_save_retry_cause;
static volatile uint64_t ai_micro_no_mepc_save_retry_epc;
static volatile uint64_t ai_micro_no_mepc_save_fell_through;

static volatile uint64_t ai_micro_mtvec_update_first_hits;
static volatile uint64_t ai_micro_mtvec_update_second_hits;
static volatile uint64_t ai_micro_mtvec_update_first_cause;
static volatile uint64_t ai_micro_mtvec_update_second_cause;
static volatile uint64_t ai_micro_mtvec_update_first_epc;
static volatile uint64_t ai_micro_mtvec_update_second_epc;
static volatile uint64_t ai_micro_mtvec_update_fell_through;

static volatile uint64_t ai_micro_deleg_update_m_hits;
static volatile uint64_t ai_micro_deleg_update_s_hits;
static volatile uint64_t ai_micro_deleg_update_m_cause;
static volatile uint64_t ai_micro_deleg_update_m_second_cause;
static volatile uint64_t ai_micro_deleg_update_s_cause;
static volatile uint64_t ai_micro_deleg_update_m_epc;
static volatile uint64_t ai_micro_deleg_update_m_second_epc;
static volatile uint64_t ai_micro_deleg_update_s_epc;
static volatile uint64_t ai_micro_deleg_update_medeleg_seen;
static volatile uint64_t ai_micro_deleg_update_fell_through;

extern uint64_t ecall_args[2];
extern void ai_micro_m_nested_exception_mtvec_entry(void);
extern void ai_micro_m_nested_exception_outer_ecall_site(void);
extern void ai_micro_m_nested_exception_inner_illegal_site(void);
extern void ai_micro_m_nested_exception_after_inner_trap(void);
extern void ai_micro_m_no_mepc_save_mtvec_entry(void);
extern void ai_micro_m_no_mepc_save_outer_ecall_site(void);
extern void ai_micro_m_no_mepc_save_inner_illegal_site(void);
extern void ai_micro_m_no_mepc_save_after_rescue(void);
extern void ai_micro_m_mtvec_update_first_entry(void);
extern void ai_micro_m_mtvec_update_second_entry(void);
extern void ai_micro_m_mtvec_update_first_ecall_site(void);
extern void ai_micro_m_mtvec_update_second_ecall_site(void);
extern void ai_micro_m_mtvec_update_after_first(void);
extern void ai_micro_m_mtvec_update_after_second(void);
extern void ai_micro_m_deleg_update_m_entry(void);
extern void ai_micro_m_deleg_update_s_entry(void);
extern void ai_micro_m_deleg_update_initial_ecall_site(void);
extern void ai_micro_m_deleg_update_hs_ebreak_site(void);
extern void ai_micro_m_deleg_update_after_hs_trap(void);
extern void ai_micro_m_deleg_update_after_cleanup(void);

asm(
    ".balign 4\n"
    ".global ai_micro_m_nested_exception_mtvec_entry\n"
    "ai_micro_m_nested_exception_mtvec_entry:\n"
    "  addi sp, sp, -64\n"
    "  sd x5, 0(sp)\n"
    "  sd x6, 8(sp)\n"
    "  sd x7, 16(sp)\n"
    "  sd x8, 24(sp)\n"
    "  sd x9, 32(sp)\n"
    "  la x5, ai_micro_nested_exception_depth\n"
    "  ld x6, 0(x5)\n"
    "  addi x6, x6, 1\n"
    "  sd x6, 0(x5)\n"
    "  li x7, 1\n"
    "  beq x6, x7, ai_micro_m_nested_exception_outer_handler\n"
    "  li x7, 2\n"
    "  beq x6, x7, ai_micro_m_nested_exception_inner_handler\n"
    "  la x5, ai_micro_nested_exception_fell_through\n"
    "  li x6, 0xbad0\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_nested_exception_after_inner_trap\n"
    "  csrw mepc, x5\n"
    "  j ai_micro_m_nested_exception_restore_one_frame_mret\n"
    "ai_micro_m_nested_exception_outer_handler:\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_micro_nested_exception_outer_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mepc\n"
    "  la x5, ai_micro_nested_exception_outer_epc\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mtval\n"
    "  la x5, ai_micro_nested_exception_outer_tval\n"
    "  sd x6, 0(x5)\n"
    "  .global ai_micro_m_nested_exception_inner_illegal_site\n"
    "ai_micro_m_nested_exception_inner_illegal_site:\n"
    "  .word 0xffffffff\n"
    "  la x5, ai_micro_nested_exception_fell_through\n"
    "  li x6, 0x1111\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_nested_exception_after_inner_trap\n"
    "  csrw mepc, x5\n"
    "  j ai_micro_m_nested_exception_restore_one_frame_mret\n"
    "ai_micro_m_nested_exception_inner_handler:\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_micro_nested_exception_inner_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mepc\n"
    "  la x5, ai_micro_nested_exception_inner_epc\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mtval\n"
    "  la x5, ai_micro_nested_exception_inner_tval\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_nested_exception_after_inner_trap\n"
    "  csrw mepc, x5\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  ld x8, 24(sp)\n"
    "  ld x9, 32(sp)\n"
    "  addi sp, sp, 64\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  ld x8, 24(sp)\n"
    "  ld x9, 32(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "ai_micro_m_nested_exception_restore_one_frame_mret:\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  ld x8, 24(sp)\n"
    "  ld x9, 32(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
);

asm(
    ".balign 4\n"
    ".global ai_micro_m_mtvec_update_first_entry\n"
    "ai_micro_m_mtvec_update_first_entry:\n"
    "  addi sp, sp, -48\n"
    "  sd x5, 0(sp)\n"
    "  sd x6, 8(sp)\n"
    "  sd x7, 16(sp)\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_micro_mtvec_update_first_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mepc\n"
    "  la x5, ai_micro_mtvec_update_first_epc\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_mtvec_update_first_hits\n"
    "  ld x6, 0(x5)\n"
    "  addi x6, x6, 1\n"
    "  sd x6, 0(x5)\n"
    "  li x7, 1\n"
    "  bne x6, x7, ai_micro_m_mtvec_update_first_unexpected_reentry\n"
    "  la x5, ai_micro_m_mtvec_update_second_entry\n"
    "  csrw mtvec, x5\n"
    "  la x5, ai_micro_m_mtvec_update_second_ecall_site\n"
    "  csrw mepc, x5\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  addi sp, sp, 48\n"
    "  mret\n"
    "ai_micro_m_mtvec_update_first_unexpected_reentry:\n"
    "  la x5, ai_micro_mtvec_update_fell_through\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_mtvec_update_after_second\n"
    "  csrw mepc, x5\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  addi sp, sp, 48\n"
    "  mret\n"
    ".global ai_micro_m_mtvec_update_second_entry\n"
    "ai_micro_m_mtvec_update_second_entry:\n"
    "  addi sp, sp, -48\n"
    "  sd x5, 0(sp)\n"
    "  sd x6, 8(sp)\n"
    "  sd x7, 16(sp)\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_micro_mtvec_update_second_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mepc\n"
    "  la x5, ai_micro_mtvec_update_second_epc\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_mtvec_update_second_hits\n"
    "  ld x6, 0(x5)\n"
    "  addi x6, x6, 1\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_mtvec_update_after_second\n"
    "  csrw mepc, x5\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  addi sp, sp, 48\n"
    "  mret\n"
);

asm(
    ".balign 4\n"
    ".global ai_micro_m_deleg_update_m_entry\n"
    "ai_micro_m_deleg_update_m_entry:\n"
    "  addi sp, sp, -48\n"
    "  sd x5, 0(sp)\n"
    "  sd x6, 8(sp)\n"
    "  sd x7, 16(sp)\n"
    "  la x5, ai_micro_deleg_update_m_hits\n"
    "  ld x6, 0(x5)\n"
    "  addi x6, x6, 1\n"
    "  sd x6, 0(x5)\n"
    "  li x7, 1\n"
    "  beq x6, x7, ai_micro_m_deleg_update_first_m_handler\n"
    "  li x7, 2\n"
    "  beq x6, x7, ai_micro_m_deleg_update_cleanup_m_handler\n"
    "  la x5, ai_micro_deleg_update_fell_through\n"
    "  sd x6, 0(x5)\n"
    "  j ai_micro_m_deleg_update_cleanup_m_handler\n"
    "ai_micro_m_deleg_update_first_m_handler:\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_micro_deleg_update_m_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mepc\n"
    "  la x5, ai_micro_deleg_update_m_epc\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_deleg_update_s_entry\n"
    "  csrw stvec, x5\n"
    "  li x7, 8\n"
    "  csrs medeleg, x7\n"
    "  csrr x6, medeleg\n"
    "  la x5, ai_micro_deleg_update_medeleg_seen\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_deleg_update_hs_ebreak_site\n"
    "  csrw mepc, x5\n"
    "  li x7, 0x1800\n"
    "  csrc mstatus, x7\n"
    "  li x7, 0x800\n"
    "  csrs mstatus, x7\n"
    "  j ai_micro_m_deleg_update_m_restore\n"
    "ai_micro_m_deleg_update_cleanup_m_handler:\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_micro_deleg_update_m_second_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mepc\n"
    "  la x5, ai_micro_deleg_update_m_second_epc\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_deleg_update_after_cleanup\n"
    "  csrw mepc, x5\n"
    "  li x7, 0x1800\n"
    "  csrc mstatus, x7\n"
    "  csrs mstatus, x7\n"
    "ai_micro_m_deleg_update_m_restore:\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  addi sp, sp, 48\n"
    "  mret\n"
    ".balign 4\n"
    ".global ai_micro_m_deleg_update_s_entry\n"
    "ai_micro_m_deleg_update_s_entry:\n"
    "  addi sp, sp, -48\n"
    "  sd x5, 0(sp)\n"
    "  sd x6, 8(sp)\n"
    "  sd x7, 16(sp)\n"
    "  csrr x6, scause\n"
    "  la x5, ai_micro_deleg_update_s_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, sepc\n"
    "  la x5, ai_micro_deleg_update_s_epc\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_deleg_update_s_hits\n"
    "  ld x6, 0(x5)\n"
    "  addi x6, x6, 1\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_deleg_update_after_hs_trap\n"
    "  csrw sepc, x5\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  addi sp, sp, 48\n"
    "  sret\n"
);

asm(
    ".balign 4\n"
    ".global ai_micro_m_no_mepc_save_mtvec_entry\n"
    "ai_micro_m_no_mepc_save_mtvec_entry:\n"
    "  addi sp, sp, -64\n"
    "  sd x5, 0(sp)\n"
    "  sd x6, 8(sp)\n"
    "  sd x7, 16(sp)\n"
    "  sd x8, 24(sp)\n"
    "  sd x9, 32(sp)\n"
    "  la x5, ai_micro_no_mepc_save_depth\n"
    "  ld x6, 0(x5)\n"
    "  addi x6, x6, 1\n"
    "  sd x6, 0(x5)\n"
    "  li x7, 1\n"
    "  beq x6, x7, ai_micro_m_no_mepc_save_outer_handler\n"
    "  li x7, 2\n"
    "  beq x6, x7, ai_micro_m_no_mepc_save_nested_handler\n"
    "  li x7, 3\n"
    "  beq x6, x7, ai_micro_m_no_mepc_save_retry_rescue_handler\n"
    "  la x5, ai_micro_no_mepc_save_fell_through\n"
    "  li x6, 0xbad00bad\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_no_mepc_save_after_rescue\n"
    "  csrw mepc, x5\n"
    "  j ai_micro_m_no_mepc_save_restore_one_frame_mret\n"
    "ai_micro_m_no_mepc_save_outer_handler:\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_micro_no_mepc_save_outer_cause\n"
    "  sd x6, 0(x5)\n"
    "  .global ai_micro_m_no_mepc_save_inner_illegal_site\n"
    "ai_micro_m_no_mepc_save_inner_illegal_site:\n"
    "  .word 0xffffffff\n"
    "  la x5, ai_micro_no_mepc_save_fell_through\n"
    "  li x6, 0x11112222\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_no_mepc_save_after_rescue\n"
    "  csrw mepc, x5\n"
    "  j ai_micro_m_no_mepc_save_restore_one_frame_mret\n"
    "ai_micro_m_no_mepc_save_nested_handler:\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_micro_no_mepc_save_nested_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mepc\n"
    "  la x5, ai_micro_no_mepc_save_nested_epc\n"
    "  sd x6, 0(x5)\n"
    "  j ai_micro_m_no_mepc_save_restore_one_frame_mret\n"
    "ai_micro_m_no_mepc_save_retry_rescue_handler:\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_micro_no_mepc_save_retry_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mepc\n"
    "  la x5, ai_micro_no_mepc_save_retry_epc\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_micro_m_no_mepc_save_after_rescue\n"
    "  csrw mepc, x5\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  ld x8, 24(sp)\n"
    "  ld x9, 32(sp)\n"
    "  addi sp, sp, 64\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  ld x8, 24(sp)\n"
    "  ld x9, 32(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "ai_micro_m_no_mepc_save_restore_one_frame_mret:\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  ld x8, 24(sp)\n"
    "  ld x9, 32(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
);

static inline void ai_micro_nested_exception_reset_state(void)
{
    ai_micro_nested_exception_depth = 0;
    ai_micro_nested_exception_outer_cause = 0;
    ai_micro_nested_exception_outer_epc = 0;
    ai_micro_nested_exception_outer_tval = 0;
    ai_micro_nested_exception_inner_cause = 0;
    ai_micro_nested_exception_inner_epc = 0;
    ai_micro_nested_exception_inner_tval = 0;
    ai_micro_nested_exception_fell_through = 0;
}

static inline void ai_micro_no_mepc_save_reset_state(void)
{
    ai_micro_no_mepc_save_depth = 0;
    ai_micro_no_mepc_save_outer_cause = 0;
    ai_micro_no_mepc_save_nested_cause = 0;
    ai_micro_no_mepc_save_nested_epc = 0;
    ai_micro_no_mepc_save_retry_cause = 0;
    ai_micro_no_mepc_save_retry_epc = 0;
    ai_micro_no_mepc_save_fell_through = 0;
}

static inline void ai_micro_mtvec_update_reset_state(void)
{
    ai_micro_mtvec_update_first_hits = 0;
    ai_micro_mtvec_update_second_hits = 0;
    ai_micro_mtvec_update_first_cause = 0;
    ai_micro_mtvec_update_second_cause = 0;
    ai_micro_mtvec_update_first_epc = 0;
    ai_micro_mtvec_update_second_epc = 0;
    ai_micro_mtvec_update_fell_through = 0;
}

static inline void ai_micro_deleg_update_reset_state(void)
{
    ai_micro_deleg_update_m_hits = 0;
    ai_micro_deleg_update_s_hits = 0;
    ai_micro_deleg_update_m_cause = 0;
    ai_micro_deleg_update_m_second_cause = 0;
    ai_micro_deleg_update_s_cause = 0;
    ai_micro_deleg_update_m_epc = 0;
    ai_micro_deleg_update_m_second_epc = 0;
    ai_micro_deleg_update_s_epc = 0;
    ai_micro_deleg_update_medeleg_seen = 0;
    ai_micro_deleg_update_fell_through = 0;
}

static inline uintptr_t ai_micro_priv_non_address_fire_ebreak(void)
{
    uintptr_t ebreak_epc;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "auipc t0, 0\n\t"
        "addi t0, t0, 12\n\t"
        "sd t0, %0\n\t"
        "ebreak\n\t"
        ".option pop\n\t"
        : "=m"(ebreak_epc)
        :
        : "t0", "memory");

    return ebreak_epc;
}

bool ai_micro_m_nested_exception_inside_mtvec_handler_recovers_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    const uint64_t old_mtvec = CSRR(CSR_MTVEC);
    const uint64_t old_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t old_mie = CSRR(CSR_MIE);
    const uint64_t old_mip = CSRR(CSR_MIP);

    ai_micro_nested_exception_reset_state();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0);
    CSRW(CSR_MIP, 0);
    CSRW(CSR_MTVEC, (uintptr_t)ai_micro_m_nested_exception_mtvec_entry);

    asm volatile(
        ".global ai_micro_m_nested_exception_outer_ecall_site\n"
        "ai_micro_m_nested_exception_outer_ecall_site:\n"
        "ecall\n"
        ".global ai_micro_m_nested_exception_after_inner_trap\n"
        "ai_micro_m_nested_exception_after_inner_trap:\n"
        ::: "memory");

    CSRW(CSR_MTVEC, old_mtvec);
    CSRW(CSR_MIE, old_mie);
    CSRW(CSR_MIP, old_mip);
    CSRW(CSR_MSTATUS, old_mstatus);

    TEST_ASSERT("the custom M-mode trap entry is reached twice: first for the outer ecall, then again for the illegal instruction raised inside that handler",
        ai_micro_nested_exception_depth == 2,
        "depth=%llu outer_cause=0x%llx inner_cause=0x%llx",
        (unsigned long long)ai_micro_nested_exception_depth,
        (unsigned long long)ai_micro_nested_exception_outer_cause,
        (unsigned long long)ai_micro_nested_exception_inner_cause
    );
    TEST_ASSERT("the first trap observed by the custom handler is the M-mode ecall from the test body",
        ai_micro_nested_exception_outer_cause == CAUSE_MACHINE_ECALL &&
        ai_micro_nested_exception_outer_epc == (uintptr_t)ai_micro_m_nested_exception_outer_ecall_site,
        "outer_cause=0x%llx outer_epc=0x%llx expected_outer_epc=0x%llx outer_tval=0x%llx",
        (unsigned long long)ai_micro_nested_exception_outer_cause,
        (unsigned long long)ai_micro_nested_exception_outer_epc,
        (unsigned long long)(uintptr_t)ai_micro_m_nested_exception_outer_ecall_site,
        (unsigned long long)ai_micro_nested_exception_outer_tval
    );
    TEST_ASSERT("the second trap is the illegal instruction deliberately executed inside the first trap handler, and the handler redirects mepc back to the test body",
        ai_micro_nested_exception_inner_cause == CAUSE_ILLEGAL_INSTRUCTION &&
        ai_micro_nested_exception_inner_epc == (uintptr_t)ai_micro_m_nested_exception_inner_illegal_site &&
        ai_micro_nested_exception_fell_through == 0,
        "inner_cause=0x%llx inner_epc=0x%llx expected_inner_epc=0x%llx inner_tval=0x%llx fell_through=0x%llx",
        (unsigned long long)ai_micro_nested_exception_inner_cause,
        (unsigned long long)ai_micro_nested_exception_inner_epc,
        (unsigned long long)(uintptr_t)ai_micro_m_nested_exception_inner_illegal_site,
        (unsigned long long)ai_micro_nested_exception_inner_tval,
        (unsigned long long)ai_micro_nested_exception_fell_through
    );

    TEST_END();
}

bool ai_micro_m_nested_exception_no_mepc_save_reenters_handler_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    const uint64_t old_mtvec = CSRR(CSR_MTVEC);
    const uint64_t old_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t old_mie = CSRR(CSR_MIE);
    const uint64_t old_mip = CSRR(CSR_MIP);

    ai_micro_no_mepc_save_reset_state();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0);
    CSRW(CSR_MIP, 0);
    CSRW(CSR_MTVEC, (uintptr_t)ai_micro_m_no_mepc_save_mtvec_entry);

    asm volatile(
        ".global ai_micro_m_no_mepc_save_outer_ecall_site\n"
        "ai_micro_m_no_mepc_save_outer_ecall_site:\n"
        "ecall\n"
        ".global ai_micro_m_no_mepc_save_after_rescue\n"
        "ai_micro_m_no_mepc_save_after_rescue:\n"
        ::: "memory");

    CSRW(CSR_MTVEC, old_mtvec);
    CSRW(CSR_MIE, old_mie);
    CSRW(CSR_MIP, old_mip);
    CSRW(CSR_MSTATUS, old_mstatus);

    TEST_ASSERT("the first trap handler intentionally does not save mepc before a handler-local illegal instruction re-enters mtvec",
        ai_micro_no_mepc_save_depth == 3 &&
        ai_micro_no_mepc_save_outer_cause == CAUSE_MACHINE_ECALL,
        "depth=%llu outer_cause=0x%llx nested_cause=0x%llx retry_cause=0x%llx",
        (unsigned long long)ai_micro_no_mepc_save_depth,
        (unsigned long long)ai_micro_no_mepc_save_outer_cause,
        (unsigned long long)ai_micro_no_mepc_save_nested_cause,
        (unsigned long long)ai_micro_no_mepc_save_retry_cause
    );
    TEST_ASSERT("without an outer mepc save, the second trap observes only the handler-local illegal instruction pc in mepc",
        ai_micro_no_mepc_save_nested_cause == CAUSE_ILLEGAL_INSTRUCTION &&
        ai_micro_no_mepc_save_nested_epc == (uintptr_t)ai_micro_m_no_mepc_save_inner_illegal_site,
        "nested_cause=0x%llx nested_epc=0x%llx illegal_site=0x%llx outer_ecall_site=0x%llx",
        (unsigned long long)ai_micro_no_mepc_save_nested_cause,
        (unsigned long long)ai_micro_no_mepc_save_nested_epc,
        (unsigned long long)(uintptr_t)ai_micro_m_no_mepc_save_inner_illegal_site,
        (unsigned long long)(uintptr_t)ai_micro_m_no_mepc_save_outer_ecall_site
    );
    TEST_ASSERT("a naive mret from the nested handler returns to the same overwritten mepc, so the handler exception repeats until software explicitly rescues it",
        ai_micro_no_mepc_save_retry_cause == CAUSE_ILLEGAL_INSTRUCTION &&
        ai_micro_no_mepc_save_retry_epc == (uintptr_t)ai_micro_m_no_mepc_save_inner_illegal_site &&
        ai_micro_no_mepc_save_fell_through == 0,
        "retry_cause=0x%llx retry_epc=0x%llx illegal_site=0x%llx fell_through=0x%llx",
        (unsigned long long)ai_micro_no_mepc_save_retry_cause,
        (unsigned long long)ai_micro_no_mepc_save_retry_epc,
        (unsigned long long)(uintptr_t)ai_micro_m_no_mepc_save_inner_illegal_site,
        (unsigned long long)ai_micro_no_mepc_save_fell_through
    );

    TEST_END();
}

bool ai_micro_priv_non_address_exception_tval_matrix_corner()
{
    TEST_START();

    uint64_t old_medeleg;
    uint64_t old_ecall_arg0 = ecall_args[0];
    uint64_t old_ecall_arg1 = ecall_args[1];
    uintptr_t ebreak_epc;

    goto_priv(PRIV_M);
    old_medeleg = CSRR(CSR_MEDELEG);
    ecall_args[0] = 0;
    ecall_args[1] = 0;

    TEST_SETUP_EXCEPT();
    ecall_code();
    TEST_ASSERT("an M-mode ecall reports machine-ecall with mtval left zero for the non-address exception",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_MACHINE_ECALL &&
        excpt.tval == 0
    );

    TEST_SETUP_EXCEPT();
    ebreak_epc = ai_micro_priv_non_address_fire_ebreak();
    TEST_ASSERT("an M-mode ebreak reports breakpoint and mtval points at the faulting ebreak PC",
        excpt.triggered == true &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BREAKPOINT &&
        excpt.epc == ebreak_epc &&
        excpt.tval == ebreak_epc
    );

    CSRS(CSR_MEDELEG, 1ULL << CAUSE_BREAKPOINT);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ebreak_epc = ai_micro_priv_non_address_fire_ebreak();
    TEST_ASSERT("a delegated HS ebreak writes the breakpoint PC into stval",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_BREAKPOINT &&
        excpt.epc == ebreak_epc &&
        excpt.tval == ebreak_epc
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ebreak_epc = ai_micro_priv_non_address_fire_ebreak();
    TEST_ASSERT("a delegated HU ebreak traps to HS and still writes the breakpoint PC into stval",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_BREAKPOINT &&
        excpt.epc == ebreak_epc &&
        excpt.tval == ebreak_epc
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MEDELEG, old_medeleg);
    ecall_args[0] = old_ecall_arg0;
    ecall_args[1] = old_ecall_arg1;

    TEST_END();
}

bool ai_micro_m_handler_updates_mtvec_next_trap_uses_new_entry_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    const uint64_t old_mtvec = CSRR(CSR_MTVEC);
    const uint64_t old_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t old_mie = CSRR(CSR_MIE);
    const uint64_t old_mip = CSRR(CSR_MIP);

    ai_micro_mtvec_update_reset_state();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0);
    CSRW(CSR_MIP, 0);
    CSRW(CSR_MTVEC, (uintptr_t)ai_micro_m_mtvec_update_first_entry);

    asm volatile(
        ".global ai_micro_m_mtvec_update_first_ecall_site\n"
        "ai_micro_m_mtvec_update_first_ecall_site:\n"
        "ecall\n"
        ".global ai_micro_m_mtvec_update_after_first\n"
        "ai_micro_m_mtvec_update_after_first:\n"
        ".global ai_micro_m_mtvec_update_second_ecall_site\n"
        "ai_micro_m_mtvec_update_second_ecall_site:\n"
        "ecall\n"
        ".global ai_micro_m_mtvec_update_after_second\n"
        "ai_micro_m_mtvec_update_after_second:\n"
        ::: "memory");

    CSRW(CSR_MTVEC, old_mtvec);
    CSRW(CSR_MIE, old_mie);
    CSRW(CSR_MIP, old_mip);
    CSRW(CSR_MSTATUS, old_mstatus);

    TEST_ASSERT("the first M-mode ecall enters the original mtvec handler, and that handler rewrites mtvec before returning",
        ai_micro_mtvec_update_first_hits == 1 &&
        ai_micro_mtvec_update_first_cause == CAUSE_MACHINE_ECALL &&
        ai_micro_mtvec_update_first_epc ==
            (uintptr_t)ai_micro_m_mtvec_update_first_ecall_site,
        "first_hits=%llu first_cause=0x%llx first_epc=0x%llx expected_epc=0x%llx",
        (unsigned long long)ai_micro_mtvec_update_first_hits,
        (unsigned long long)ai_micro_mtvec_update_first_cause,
        (unsigned long long)ai_micro_mtvec_update_first_epc,
        (unsigned long long)(uintptr_t)ai_micro_m_mtvec_update_first_ecall_site
    );
    TEST_ASSERT("the immediately following M-mode ecall uses the mtvec value written by the previous handler instead of reusing the stale entry",
        ai_micro_mtvec_update_second_hits == 1 &&
        ai_micro_mtvec_update_second_cause == CAUSE_MACHINE_ECALL &&
        ai_micro_mtvec_update_second_epc ==
            (uintptr_t)ai_micro_m_mtvec_update_second_ecall_site &&
        ai_micro_mtvec_update_fell_through == 0,
        "second_hits=%llu second_cause=0x%llx second_epc=0x%llx expected_epc=0x%llx fell_through=0x%llx",
        (unsigned long long)ai_micro_mtvec_update_second_hits,
        (unsigned long long)ai_micro_mtvec_update_second_cause,
        (unsigned long long)ai_micro_mtvec_update_second_epc,
        (unsigned long long)(uintptr_t)ai_micro_m_mtvec_update_second_ecall_site,
        (unsigned long long)ai_micro_mtvec_update_fell_through
    );

    TEST_END();
}

bool ai_micro_m_handler_updates_delegation_and_stvec_next_hs_trap_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    const uint64_t old_mtvec = CSRR(CSR_MTVEC);
    const uint64_t old_stvec = CSRR(CSR_STVEC);
    const uint64_t old_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t old_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t old_mie = CSRR(CSR_MIE);
    const uint64_t old_mip = CSRR(CSR_MIP);

    ai_micro_deleg_update_reset_state();
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_BKP);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0);
    CSRW(CSR_MIP, 0);
    CSRW(CSR_MTVEC, (uintptr_t)ai_micro_m_deleg_update_m_entry);

    asm volatile(
        ".global ai_micro_m_deleg_update_initial_ecall_site\n"
        "ai_micro_m_deleg_update_initial_ecall_site:\n"
        "ecall\n"
        ".global ai_micro_m_deleg_update_hs_ebreak_site\n"
        "ai_micro_m_deleg_update_hs_ebreak_site:\n"
        "ebreak\n"
        ".global ai_micro_m_deleg_update_after_hs_trap\n"
        "ai_micro_m_deleg_update_after_hs_trap:\n"
        "ecall\n"
        ".global ai_micro_m_deleg_update_after_cleanup\n"
        "ai_micro_m_deleg_update_after_cleanup:\n"
        ::: "memory");

    curr_priv = PRIV_M;

    CSRW(CSR_MTVEC, old_mtvec);
    CSRW(CSR_STVEC, old_stvec);
    CSRW(CSR_MEDELEG, old_medeleg);
    CSRW(CSR_MIE, old_mie);
    CSRW(CSR_MIP, old_mip);
    CSRW(CSR_MSTATUS, old_mstatus);

    TEST_ASSERT("the custom M handler is reached first for the setup ecall and later for cleanup, while the setup visit records the original M-mode ecall",
        ai_micro_deleg_update_m_hits == 2 &&
        ai_micro_deleg_update_m_cause == CAUSE_MACHINE_ECALL &&
        ai_micro_deleg_update_m_epc ==
            (uintptr_t)ai_micro_m_deleg_update_initial_ecall_site,
        "m_hits=%llu m_cause=0x%llx m_epc=0x%llx expected_epc=0x%llx medeleg_seen=0x%llx second_cause=0x%llx second_epc=0x%llx",
        (unsigned long long)ai_micro_deleg_update_m_hits,
        (unsigned long long)ai_micro_deleg_update_m_cause,
        (unsigned long long)ai_micro_deleg_update_m_epc,
        (unsigned long long)(uintptr_t)ai_micro_m_deleg_update_initial_ecall_site,
        (unsigned long long)ai_micro_deleg_update_medeleg_seen,
        (unsigned long long)ai_micro_deleg_update_m_second_cause,
        (unsigned long long)ai_micro_deleg_update_m_second_epc
    );
    TEST_ASSERT("the immediately following HS breakpoint uses the stvec and medeleg value written by the previous M handler",
        ai_micro_deleg_update_s_hits == 1 &&
        ai_micro_deleg_update_s_cause == CAUSE_BREAKPOINT &&
        ai_micro_deleg_update_s_epc ==
            (uintptr_t)ai_micro_m_deleg_update_hs_ebreak_site &&
        ai_micro_deleg_update_fell_through == 0,
        "s_hits=%llu s_cause=0x%llx s_epc=0x%llx expected_epc=0x%llx fell_through=0x%llx medeleg_seen=0x%llx m_second_cause=0x%llx m_second_epc=0x%llx",
        (unsigned long long)ai_micro_deleg_update_s_hits,
        (unsigned long long)ai_micro_deleg_update_s_cause,
        (unsigned long long)ai_micro_deleg_update_s_epc,
        (unsigned long long)(uintptr_t)ai_micro_m_deleg_update_hs_ebreak_site,
        (unsigned long long)ai_micro_deleg_update_fell_through,
        (unsigned long long)ai_micro_deleg_update_medeleg_seen,
        (unsigned long long)ai_micro_deleg_update_m_second_cause,
        (unsigned long long)ai_micro_deleg_update_m_second_epc
    );

    TEST_END();
}
