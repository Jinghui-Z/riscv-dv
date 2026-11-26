#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK (3ULL << 11)
#define AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U 0ULL
#define AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S (1ULL << 11)
#define AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_M (3ULL << 11)
#define AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPIE (1ULL << 7)
#define AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SIE (1ULL << 1)
#define AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SPIE (1ULL << 5)
#define AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SPP (1ULL << 8)
#define AI_MEMBLOCK_BASIC_PRIV_TSR (1ULL << 22)

#define AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MEMBLOCK_BASIC_ASSERT_EXCEPT(message, expected_cause, expected_tval) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == (expected_cause) && \
        excpt.tval == (expected_tval))

#define AI_MEMBLOCK_BASIC_PRIV_CSRR_INST(csr) \
    (((uint32_t)(csr) << 20) | (2U << 12) | (5U << 7) | 0x73U)
#define AI_MEMBLOCK_BASIC_PRIV_CSRW_INST(csr) \
    (((uint32_t)(csr) << 20) | (5U << 15) | (1U << 12) | 0x73U)

extern unsigned real_priv;

static volatile uint64_t ai_memblock_xret_status_hs_landed;
static volatile uint64_t ai_memblock_xret_status_hs_fell_through;
static volatile uint64_t ai_memblock_xret_status_m_landed;
static volatile uint64_t ai_memblock_xret_status_m_handler_seen;
static volatile uint64_t ai_memblock_xret_status_m_handler_mstatus;
static volatile uint64_t ai_memblock_xret_status_m_handler_cause;
static volatile uint64_t ai_memblock_xret_status_m_handler_epc;

extern void ai_memblock_xret_status_m_handler(void);
extern void ai_memblock_xret_status_hs_after_mret(void);
extern void ai_memblock_xret_status_hs_ecall_site(void);
extern void ai_memblock_xret_status_after_custom_m_handler(void);

asm(
    ".balign 4\n"
    ".global ai_memblock_xret_status_hs_after_mret\n"
    "ai_memblock_xret_status_hs_after_mret:\n"
    "  la t0, ai_memblock_xret_status_hs_landed\n"
    "  li t1, 1\n"
    "  sd t1, 0(t0)\n"
    "  .global ai_memblock_xret_status_hs_ecall_site\n"
    "ai_memblock_xret_status_hs_ecall_site:\n"
    "  ecall\n"
    "  la t0, ai_memblock_xret_status_hs_fell_through\n"
    "  li t1, 0xbad1\n"
    "  sd t1, 0(t0)\n"
    "  j ai_memblock_xret_status_after_custom_m_handler\n"
    ".balign 4\n"
    ".global ai_memblock_xret_status_m_handler\n"
    "ai_memblock_xret_status_m_handler:\n"
    "  addi sp, sp, -32\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  csrr t1, mstatus\n"
    "  la t0, ai_memblock_xret_status_m_handler_mstatus\n"
    "  sd t1, 0(t0)\n"
    "  csrr t1, mcause\n"
    "  la t0, ai_memblock_xret_status_m_handler_cause\n"
    "  sd t1, 0(t0)\n"
    "  csrr t1, mepc\n"
    "  la t0, ai_memblock_xret_status_m_handler_epc\n"
    "  sd t1, 0(t0)\n"
    "  la t0, ai_memblock_xret_status_m_handler_seen\n"
    "  li t1, 1\n"
    "  sd t1, 0(t0)\n"
    "  la t0, ai_memblock_xret_status_after_custom_m_handler\n"
    "  csrw mepc, t0\n"
    "  li t2, 0x1800\n"
    "  csrc mstatus, t2\n"
    "  csrs mstatus, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  addi sp, sp, 32\n"
    "  mret\n"
);

static inline uint64_t ai_memblock_basic_priv_csrr_mstatus(void)
{
    uint64_t value;

    asm volatile("csrr %0, mstatus" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrw_mstatus(uint64_t value)
{
    asm volatile("csrw mstatus, %0" :: "rK"(value) : "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrr_mstatus(void)
{
    asm volatile("csrr t0, mstatus" ::: "t0", "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrw_mstatus(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw mstatus, t0\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline uint64_t ai_memblock_basic_priv_csrr_mepc(void)
{
    uint64_t value;

    asm volatile("csrr %0, mepc" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrw_mepc(uint64_t value)
{
    asm volatile("csrw mepc, %0" :: "rK"(value) : "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrr_mepc(void)
{
    asm volatile("csrr t0, mepc" ::: "t0", "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrw_mepc(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw mepc, t0\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline uint64_t ai_memblock_basic_priv_csrr_mtvec(void)
{
    uint64_t value;

    asm volatile("csrr %0, mtvec" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrw_mtvec(uint64_t value)
{
    asm volatile("csrw mtvec, %0" :: "rK"(value) : "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrr_mtvec(void)
{
    asm volatile("csrr t0, mtvec" ::: "t0", "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrw_mtvec(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw mtvec, t0\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline uint64_t ai_memblock_basic_priv_csrr_mcause(void)
{
    uint64_t value;

    asm volatile("csrr %0, mcause" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrw_mcause(uint64_t value)
{
    asm volatile("csrw mcause, %0" :: "rK"(value) : "memory");
}

static inline uint64_t ai_memblock_basic_priv_csrr_mie(void)
{
    uint64_t value;

    asm volatile("csrr %0, mie" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrw_mie(uint64_t value)
{
    asm volatile("csrw mie, %0" :: "rK"(value) : "memory");
}

static inline uint64_t ai_memblock_basic_priv_csrr_mip(void)
{
    uint64_t value;

    asm volatile("csrr %0, mip" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrs_mip(uint64_t value)
{
    asm volatile("csrs mip, %0" :: "rK"(value) : "memory");
}

static inline uint64_t ai_memblock_basic_priv_csrr_sstatus(void)
{
    uint64_t value;

    asm volatile("csrr %0, sstatus" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrw_sstatus(uint64_t value)
{
    asm volatile("csrw sstatus, %0" :: "rK"(value) : "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrr_sstatus(void)
{
    asm volatile("csrr t0, sstatus" ::: "t0", "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrw_sstatus(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw sstatus, t0\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline uint64_t ai_memblock_basic_priv_csrr_sepc(void)
{
    uint64_t value;

    asm volatile("csrr %0, sepc" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrw_sepc(uint64_t value)
{
    asm volatile("csrw sepc, %0" :: "rK"(value) : "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrr_sepc(void)
{
    asm volatile("csrr t0, sepc" ::: "t0", "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrw_sepc(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw sepc, t0\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline uint64_t ai_memblock_basic_priv_csrr_stvec(void)
{
    uint64_t value;

    asm volatile("csrr %0, stvec" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrw_stvec(uint64_t value)
{
    asm volatile("csrw stvec, %0" :: "rK"(value) : "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrr_stvec(void)
{
    asm volatile("csrr t0, stvec" ::: "t0", "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrw_stvec(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw stvec, t0\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline uint64_t ai_memblock_basic_priv_csrr_scause(void)
{
    uint64_t value;

    asm volatile("csrr %0, scause" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_memblock_basic_priv_csrw_scause(uint64_t value)
{
    asm volatile("csrw scause, %0" :: "rK"(value) : "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrr_scause(void)
{
    asm volatile("csrr t0, scause" ::: "t0", "memory");
}

static inline void ai_memblock_basic_priv_fixed_csrw_scause(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw scause, t0\n\t"
        :: "r"(value)
        : "t0", "memory");
}

static inline uint64_t ai_memblock_basic_priv_ld_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    register uint64_t value asm("a4") = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value)
        : "r"(addr)
        : "memory");
    return value;
}

static inline pte_t *ai_memblock_basic_priv_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_basic_priv_set_mprv_mpp(uint64_t mpp_bits)
{
    uint64_t mstatus = CSRR(CSR_MSTATUS);

    mstatus &= ~(AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK |
        MSTATUS_MPRV);
    mstatus |= MSTATUS_MPRV | mpp_bits;
    CSRW(CSR_MSTATUS, mstatus);
}

static inline void ai_memblock_basic_priv_clear_mprv_mpp(void)
{
    CSRC(CSR_MSTATUS, MSTATUS_MPRV |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK);
}

#define AI_MEMBLOCK_BASIC_MPRV_LOAD_D(name, clear_bits, set_bits) \
static inline uint64_t name(uintptr_t addr) \
{ \
    uint64_t value = 0ULL; \
    const uint64_t clear_mprv_mpp = MSTATUS_MPRV | \
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK | (clear_bits); \
    const uint64_t set_mprv_mpp = MSTATUS_MPRV | (set_bits); \
    asm volatile( \
        "csrc mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        "ld %0, 0(%3)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %1\n\t" \
        : "=&r"(value) \
        : "r"(clear_mprv_mpp), "r"(set_mprv_mpp), "r"(addr) \
        : "memory"); \
    return value; \
}

#define AI_MEMBLOCK_BASIC_MPRV_STORE_D(name, clear_bits, set_bits) \
static inline void name(uintptr_t addr, uint64_t value) \
{ \
    const uint64_t clear_mprv_mpp = MSTATUS_MPRV | \
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK | (clear_bits); \
    const uint64_t set_mprv_mpp = MSTATUS_MPRV | (set_bits); \
    asm volatile( \
        "csrc mstatus, %0\n\t" \
        "csrs mstatus, %1\n\t" \
        "sd %2, 0(%3)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %0\n\t" \
        : \
        : "r"(clear_mprv_mpp), "r"(set_mprv_mpp), "r"(value), "r"(addr) \
        : "memory"); \
}

#define AI_MEMBLOCK_BASIC_MPRV_PREFETCH(name, imm, clear_bits, set_bits) \
static inline void name(uintptr_t addr) \
{ \
    const uint64_t clear_mprv_mpp = MSTATUS_MPRV | \
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK | (clear_bits); \
    const uint64_t set_mprv_mpp = MSTATUS_MPRV | (set_bits); \
    asm volatile( \
        "csrc mstatus, %0\n\t" \
        "csrs mstatus, %1\n\t" \
        ".insn i 0x13, 0x6, x0, %2, " #imm "\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %0\n\t" \
        : \
        : "r"(clear_mprv_mpp), "r"(set_mprv_mpp), "r"(addr) \
        : "memory"); \
}

#define AI_MEMBLOCK_BASIC_MPRV_AMOADD_D(name, clear_bits, set_bits) \
static inline uint64_t name(uintptr_t addr, uint64_t value) \
{ \
    const uint64_t clear_mprv_mpp = MSTATUS_MPRV | \
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK | (clear_bits); \
    const uint64_t set_mprv_mpp = MSTATUS_MPRV | (set_bits); \
    asm volatile( \
        "csrc mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        "amoadd.d %0, %0, 0(%3)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %1\n\t" \
        : "+&r"(value) \
        : "r"(clear_mprv_mpp), "r"(set_mprv_mpp), "r"(addr) \
        : "memory"); \
    return value; \
}

AI_MEMBLOCK_BASIC_MPRV_LOAD_D(ai_memblock_basic_priv_mprv_m_ld,
    0ULL, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_M)
AI_MEMBLOCK_BASIC_MPRV_LOAD_D(ai_memblock_basic_priv_mprv_s_sum0_ld,
    MSTATUS_SUM, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S)
AI_MEMBLOCK_BASIC_MPRV_LOAD_D(ai_memblock_basic_priv_mprv_s_sum0_mxr1_ld,
    MSTATUS_SUM, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_MXR)
AI_MEMBLOCK_BASIC_MPRV_LOAD_D(ai_memblock_basic_priv_mprv_s_sum1_ld,
    0ULL, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_LOAD_D(ai_memblock_basic_priv_mprv_s_sum1_mxr0_ld,
    MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_LOAD_D(ai_memblock_basic_priv_mprv_s_sum1_mxr1_ld,
    0ULL, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM |
    MSTATUS_MXR)
AI_MEMBLOCK_BASIC_MPRV_LOAD_D(ai_memblock_basic_priv_mprv_u_mxr0_ld,
    MSTATUS_MXR | MSTATUS_SUM, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U)
AI_MEMBLOCK_BASIC_MPRV_LOAD_D(ai_memblock_basic_priv_mprv_u_sum1_ld,
    MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_LOAD_D(ai_memblock_basic_priv_mprv_u_mxr1_ld,
    MSTATUS_SUM, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U | MSTATUS_MXR)

AI_MEMBLOCK_BASIC_MPRV_STORE_D(ai_memblock_basic_priv_mprv_s_sum0_sd,
    MSTATUS_SUM, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S)
AI_MEMBLOCK_BASIC_MPRV_STORE_D(ai_memblock_basic_priv_mprv_s_sum1_sd,
    0ULL, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_STORE_D(ai_memblock_basic_priv_mprv_u_sd,
    MSTATUS_SUM | MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U)

AI_MEMBLOCK_BASIC_MPRV_PREFETCH(
    ai_memblock_basic_priv_mprv_s_sum0_prefetch_r,
    1, MSTATUS_SUM | MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S)
AI_MEMBLOCK_BASIC_MPRV_PREFETCH(
    ai_memblock_basic_priv_mprv_s_sum0_prefetch_w,
    3, MSTATUS_SUM | MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S)
AI_MEMBLOCK_BASIC_MPRV_PREFETCH(
    ai_memblock_basic_priv_mprv_s_sum1_mxr0_prefetch_r,
    1, MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_PREFETCH(
    ai_memblock_basic_priv_mprv_u_mxr0_prefetch_r,
    1, MSTATUS_SUM | MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U)

AI_MEMBLOCK_BASIC_MPRV_AMOADD_D(ai_memblock_basic_priv_mprv_s_sum1_amoadd_d,
    0ULL, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_AMOADD_D(ai_memblock_basic_priv_mprv_s_sum0_amoadd_d,
    MSTATUS_SUM, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S)
AI_MEMBLOCK_BASIC_MPRV_AMOADD_D(ai_memblock_basic_priv_mprv_u_amoadd_d,
    MSTATUS_SUM | MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U)

#define AI_MEMBLOCK_BASIC_MPRV_LR_D(name, clear_bits, set_bits) \
static inline uint64_t name(uintptr_t addr) \
{ \
    uint64_t value = 0ULL; \
    const uint64_t clear_mprv_mpp = MSTATUS_MPRV | \
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK | (clear_bits); \
    const uint64_t set_mprv_mpp = MSTATUS_MPRV | (set_bits); \
    asm volatile( \
        "csrc mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        "lr.d %0, 0(%3)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %1\n\t" \
        : "=&r"(value) \
        : "r"(clear_mprv_mpp), "r"(set_mprv_mpp), "r"(addr) \
        : "memory"); \
    return value; \
}

#define AI_MEMBLOCK_BASIC_MPRV_SC_D(name, clear_bits, set_bits) \
static inline uint64_t name(uintptr_t addr, uint64_t value) \
{ \
    uint64_t status = ~0ULL; \
    const uint64_t clear_mprv_mpp = MSTATUS_MPRV | \
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK | (clear_bits); \
    const uint64_t set_mprv_mpp = MSTATUS_MPRV | (set_bits); \
    asm volatile( \
        "csrc mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        "sc.d %0, %4, 0(%3)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %1\n\t" \
        : "=&r"(status) \
        : "r"(clear_mprv_mpp), "r"(set_mprv_mpp), "r"(addr), \
          "r"(value) \
        : "memory"); \
    return status; \
}

#define AI_MEMBLOCK_BASIC_MPRV_LRSC_D_PAIR(name, clear_bits, set_bits) \
static inline uint64_t name(uintptr_t addr, uint64_t new_value, \
    uint64_t *old_value) \
{ \
    uint64_t old_reg = 0ULL; \
    uint64_t status = ~0ULL; \
    const uint64_t clear_mprv_mpp = MSTATUS_MPRV | \
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK | (clear_bits); \
    const uint64_t set_mprv_mpp = MSTATUS_MPRV | (set_bits); \
    asm volatile( \
        "csrc mstatus, %2\n\t" \
        "csrs mstatus, %3\n\t" \
        "lr.d %0, 0(%4)\n\t" \
        "sc.d %1, %5, 0(%4)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %2\n\t" \
        : "=&r"(old_reg), "=&r"(status) \
        : "r"(clear_mprv_mpp), "r"(set_mprv_mpp), "r"(addr), \
          "r"(new_value) \
        : "memory"); \
    *old_value = old_reg; \
    return status; \
}

AI_MEMBLOCK_BASIC_MPRV_LR_D(ai_memblock_basic_priv_mprv_s_sum1_lr_d,
    0ULL, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_LR_D(ai_memblock_basic_priv_mprv_s_sum1_mxr0_lr_d,
    MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_LR_D(ai_memblock_basic_priv_mprv_s_sum1_mxr1_lr_d,
    0ULL, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM |
    MSTATUS_MXR)
AI_MEMBLOCK_BASIC_MPRV_LR_D(ai_memblock_basic_priv_mprv_s_sum0_lr_d,
    MSTATUS_SUM, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S)
AI_MEMBLOCK_BASIC_MPRV_LR_D(ai_memblock_basic_priv_mprv_u_lr_d,
    MSTATUS_SUM | MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U)
AI_MEMBLOCK_BASIC_MPRV_LR_D(ai_memblock_basic_priv_mprv_u_mxr1_lr_d,
    MSTATUS_SUM, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U | MSTATUS_MXR)
AI_MEMBLOCK_BASIC_MPRV_SC_D(ai_memblock_basic_priv_mprv_s_sum1_sc_d,
    0ULL, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_SC_D(ai_memblock_basic_priv_mprv_s_sum0_sc_d,
    MSTATUS_SUM, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S)
AI_MEMBLOCK_BASIC_MPRV_SC_D(ai_memblock_basic_priv_mprv_u_sc_d,
    MSTATUS_SUM | MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U)
AI_MEMBLOCK_BASIC_MPRV_LRSC_D_PAIR(
    ai_memblock_basic_priv_mprv_s_sum1_lrsc_d_pair,
    0ULL, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S | MSTATUS_SUM)
AI_MEMBLOCK_BASIC_MPRV_LRSC_D_PAIR(
    ai_memblock_basic_priv_mprv_u_lrsc_d_pair,
    MSTATUS_SUM | MSTATUS_MXR, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U)

static inline void ai_memblock_basic_priv_mprv_u_fetch_m_phys(uintptr_t addr)
{
    const uint64_t clear_mprv_mpp = MSTATUS_MPRV |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK | MSTATUS_SUM |
        MSTATUS_MXR;
    const uint64_t set_mprv_u = MSTATUS_MPRV |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U;

    asm volatile(
        "addi sp, sp, -16\n\t"
        "sd ra, 0(sp)\n\t"
        "la t0, 2f\n\t"
        "sd t0, %0\n\t"
        "csrc mstatus, %2\n\t"
        "csrs mstatus, %3\n\t"
        "la ra, 1f\n\t"
        "jr %1\n\t"
        "1:\n\t"
        "csrc mstatus, %2\n\t"
        "2:\n\t"
        "csrc mstatus, %2\n\t"
        "ld ra, 0(sp)\n\t"
        "addi sp, sp, 16\n\t"
        "sd zero, %0\n\t"
        : "+m"(excpt.fault_inst)
        : "r"(addr), "r"(clear_mprv_mpp), "r"(set_mprv_u)
        : "t0", "memory");
}

static inline bool ai_memblock_basic_priv_check_illegal_with_inst(
    uint32_t expected_inst)
{
    return excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        (uint32_t)excpt.tval == expected_inst &&
        excpt.epc != 0;
}

static inline bool ai_memblock_basic_priv_check_ebreak(unsigned expected_priv)
{
    uintptr_t auipc_pc = 0;

    TEST_SETUP_EXCEPT();
    asm volatile(
        "1:\n\t"
        "auipc %0, 0\n\t"
        "ebreak\n\t"
        : "=r"(auipc_pc)
        :
        : "memory");

    return excpt.triggered == true &&
        excpt.cause == CAUSE_BREAKPOINT &&
        excpt.epc == auipc_pc + 4U &&
        curr_priv == expected_priv;
}

bool ai_micro_memblock_basic_privilege_csr_and_return_gate_corner()
{
    TEST_START();

    const uint64_t mtvec_before = CSRR(CSR_MTVEC);
    const uint64_t stvec_before = CSRR(CSR_STVEC);
    const uint64_t mepc_probe = 0x80001234ULL;
    const uint64_t mcause_probe = 0x5ULL;
    const uint64_t sepc_probe = 0x80005678ULL;
    const uint64_t scause_probe = 0x9ULL;
    const uint32_t csrr_mstatus_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRR_INST(CSR_MSTATUS);
    const uint32_t csrw_mepc_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRW_INST(CSR_MEPC);
    const uint32_t csrw_mtvec_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRW_INST(CSR_MTVEC);
    const uint32_t csrr_mepc_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRR_INST(CSR_MEPC);
    const uint32_t csrr_sstatus_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRR_INST(CSR_SSTATUS);
    const uint32_t csrw_sstatus_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRW_INST(CSR_SSTATUS);
    const uint32_t csrr_sepc_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRR_INST(CSR_SEPC);
    const uint32_t csrw_sepc_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRW_INST(CSR_SEPC);
    const uint32_t csrr_stvec_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRR_INST(CSR_STVEC);
    const uint32_t csrw_stvec_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRW_INST(CSR_STVEC);
    const uint32_t csrr_scause_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRR_INST(CSR_SCAUSE);
    const uint32_t csrw_scause_inst =
        AI_MEMBLOCK_BASIC_PRIV_CSRW_INST(CSR_SCAUSE);
    const uint32_t mret_inst = 0x30200073U;
    const uint32_t sret_inst = 0x10200073U;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, AI_MEMBLOCK_BASIC_PRIV_TSR |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK |
        MSTATUS_MPRV | MSTATUS_SUM | MSTATUS_MXR);
    CSRW(CSR_MEPC, mepc_probe);
    CSRW(CSR_MCAUSE, mcause_probe);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_SEPC, sepc_probe);
    CSRW(CSR_SCAUSE, scause_probe);

    TEST_SETUP_EXCEPT();
    uint64_t mstatus_read = ai_memblock_basic_priv_csrr_mstatus();
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode can read mstatus as an M-only CSR without trapping",
        mstatus_read == CSRR(CSR_MSTATUS)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_csrw_mstatus(mstatus_read | MSTATUS_MIE);
    uint64_t mstatus_after_write = CSRR(CSR_MSTATUS);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode can write mstatus and observe the WARL-updated value without trapping",
        (mstatus_after_write & MSTATUS_MIE) == MSTATUS_MIE
    );

    TEST_SETUP_EXCEPT();
    uint64_t mepc_read = ai_memblock_basic_priv_csrr_mepc();
    ai_memblock_basic_priv_csrw_mepc(mepc_probe + 4U);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode can read and write mepc without trapping",
        mepc_read == mepc_probe &&
        CSRR(CSR_MEPC) == mepc_probe + 4U
    );

    TEST_SETUP_EXCEPT();
    uint64_t mtvec_read = ai_memblock_basic_priv_csrr_mtvec();
    ai_memblock_basic_priv_csrw_mtvec(mtvec_before);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode can read and write mtvec without trapping",
        mtvec_read == mtvec_before &&
        CSRR(CSR_MTVEC) == mtvec_before
    );

    TEST_SETUP_EXCEPT();
    uint64_t mcause_read = ai_memblock_basic_priv_csrr_mcause();
    ai_memblock_basic_priv_csrw_mcause(mcause_probe + 1U);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode can read and write mcause without trapping",
        mcause_read == mcause_probe &&
        CSRR(CSR_MCAUSE) == mcause_probe + 1U
    );

    TEST_SETUP_EXCEPT();
    uint64_t mie_read = ai_memblock_basic_priv_csrr_mie();
    ai_memblock_basic_priv_csrw_mie(mie_read);
    uint64_t mip_read = ai_memblock_basic_priv_csrr_mip();
    ai_memblock_basic_priv_csrs_mip(0ULL);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode can read/write mie and read mip without trapping",
        CSRR(CSR_MIE) == mie_read &&
        (mip_read & (1ULL << 63)) == 0
    );

    CSRW(CSR_MEPC, mepc_probe);
    CSRW(CSR_MCAUSE, mcause_probe);
    CSRW(CSR_MTVEC, mtvec_before);
    CSRW(CSR_MIE, 0ULL);
    uint64_t mie_before_s_illegal = CSRR(CSR_MIE);
    uint64_t mip_before_s_illegal = CSRR(CSR_MIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrr_mstatus();
    TEST_ASSERT("S-mode reading mstatus reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrr_mstatus_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrw_mepc(mepc_probe + 0x40U);
    TEST_ASSERT("S-mode writing mepc reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrw_mepc_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrw_mtvec(mtvec_before ^ 0x40ULL);
    TEST_ASSERT("S-mode writing mtvec reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrw_mtvec_inst)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("S-mode M-only CSR traps do not commit the requested CSR writes",
        CSRR(CSR_MEPC) != mepc_probe + 0x40U &&
        CSRR(CSR_MTVEC) == mtvec_before &&
        CSRR(CSR_MIE) == mie_before_s_illegal &&
        CSRR(CSR_MIP) == mip_before_s_illegal
    );

    CSRW(CSR_SEPC, sepc_probe);
    CSRW(CSR_STVEC, stvec_before);
    CSRW(CSR_SCAUSE, scause_probe);
    CSRC(CSR_MEDELEG, 1ULL << 8);
    const uint64_t sstatus_before_u_illegal = CSRR(CSR_SSTATUS);
    const uint64_t sepc_before_u_illegal = CSRR(CSR_SEPC);
    const uint64_t stvec_before_u_illegal = CSRR(CSR_STVEC);
    const uint64_t scause_before_u_illegal = CSRR(CSR_SCAUSE);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrr_mepc();
    TEST_ASSERT("U-mode reading mepc reports IllegalInstruction with the trapped M CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrr_mepc_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrw_mstatus(mstatus_after_write);
    TEST_ASSERT("U-mode writing mstatus reports IllegalInstruction with the trapped M CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(
            AI_MEMBLOCK_BASIC_PRIV_CSRW_INST(CSR_MSTATUS))
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrr_sstatus();
    TEST_ASSERT("U-mode reading sstatus reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrr_sstatus_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrw_sstatus(sstatus_before_u_illegal |
        SSTATUS_SIE_BIT);
    TEST_ASSERT("U-mode writing sstatus reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrw_sstatus_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrr_sepc();
    TEST_ASSERT("U-mode reading sepc reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrr_sepc_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrw_sepc(sepc_probe + 0x40U);
    TEST_ASSERT("U-mode writing sepc reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrw_sepc_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrr_stvec();
    TEST_ASSERT("U-mode reading stvec reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrr_stvec_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrw_stvec(stvec_before ^ 0x40ULL);
    TEST_ASSERT("U-mode writing stvec reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrw_stvec_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrr_scause();
    TEST_ASSERT("U-mode reading scause reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrr_scause_inst)
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrw_scause(scause_probe + 1U);
    TEST_ASSERT("U-mode writing scause reports IllegalInstruction with the trapped CSR instruction in tval",
        ai_memblock_basic_priv_check_illegal_with_inst(csrw_scause_inst)
    );

    goto_priv_u_to_m_direct(PRIV_M);
    TEST_ASSERT("U-mode S CSR traps preserve the S CSR state",
        CSRR(CSR_SSTATUS) == sstatus_before_u_illegal &&
        CSRR(CSR_SEPC) == sepc_before_u_illegal &&
        CSRR(CSR_STVEC) == stvec_before_u_illegal &&
        CSRR(CSR_SCAUSE) == scause_before_u_illegal
    );
    CSRS(CSR_MEDELEG, 1ULL << 8);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    asm volatile("mret" ::: "memory");
    TEST_ASSERT("U-mode MRET reports IllegalInstruction and keeps the current privilege in U",
        ai_memblock_basic_priv_check_illegal_with_inst(mret_inst) &&
        curr_priv == PRIV_HU
    );

    TEST_SETUP_EXCEPT();
    asm volatile("sret" ::: "memory");
    TEST_ASSERT("U-mode SRET reports IllegalInstruction and keeps the current privilege in U",
        ai_memblock_basic_priv_check_illegal_with_inst(sret_inst) &&
        curr_priv == PRIV_HU
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    asm volatile("mret" ::: "memory");
    TEST_ASSERT("S-mode MRET reports IllegalInstruction and keeps the current privilege in S",
        ai_memblock_basic_priv_check_illegal_with_inst(mret_inst) &&
        curr_priv == PRIV_HS
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, AI_MEMBLOCK_BASIC_PRIV_TSR);
    goto_priv(PRIV_HS);
    set_prev_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_SRET();
    TEST_ASSERT("S-mode SRET reports IllegalInstruction when mstatus.TSR=1",
        ai_memblock_basic_priv_check_illegal_with_inst(sret_inst) &&
        curr_priv == PRIV_HS
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, AI_MEMBLOCK_BASIC_PRIV_TSR);
    set_prev_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_MRET();
    curr_priv = PRIV_HS;
    real_priv = PRIV_HS;
    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrr_mstatus();
    TEST_ASSERT("MRET with MPP=S immediately returns execution to S-mode, where mstatus becomes illegal again",
        ai_memblock_basic_priv_check_illegal_with_inst(csrr_mstatus_inst)
    );
    goto_priv(PRIV_M);
    TEST_ASSERT("MRET with MPP=S clears the old MPP field after the privilege transition",
        (CSRR(CSR_MSTATUS) & AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK) ==
            AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U
    );

    set_prev_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_MRET();
    curr_priv = PRIV_HU;
    real_priv = PRIV_HU;
    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_fixed_csrr_sstatus();
    TEST_ASSERT("MRET with MPP=U immediately returns execution to U-mode, where sstatus becomes illegal",
        ai_memblock_basic_priv_check_illegal_with_inst(csrr_sstatus_inst)
    );
    goto_priv(PRIV_M);
    TEST_ASSERT("MRET with MPP=U clears the old MPP field after the privilege transition",
        (CSRR(CSR_MSTATUS) & AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK) ==
            AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U
    );

    TEST_ASSERT("M-mode EBREAK reports Breakpoint with epc pointing at the EBREAK instruction",
        ai_memblock_basic_priv_check_ebreak(PRIV_M)
    );

    goto_priv(PRIV_HS);
    TEST_ASSERT("S-mode EBREAK reports Breakpoint with epc pointing at the EBREAK instruction",
        ai_memblock_basic_priv_check_ebreak(PRIV_HS)
    );

    goto_priv(PRIV_HU);
    TEST_ASSERT("U-mode EBREAK reports Breakpoint with epc pointing at the EBREAK instruction",
        ai_memblock_basic_priv_check_ebreak(PRIV_HU)
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, mtvec_before);
    CSRW(CSR_STVEC, stvec_before);
    CSRC(CSR_MSTATUS, AI_MEMBLOCK_BASIC_PRIV_TSR |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK |
        MSTATUS_MPRV | MSTATUS_SUM | MSTATUS_MXR | MSTATUS_MIE);

    TEST_END();
}

bool ai_micro_memblock_mret_hs_data_access_rechecks_context_corner()
{
    TEST_START();

    const enum test_page s_page = VSRWX_GRWX;
    const enum test_page u_page = VSURWX_GURWX;
    const uintptr_t s_load_vaddr = hs_page_base(s_page) + 0x640U;
    const uintptr_t s_store_vaddr = hs_page_base(s_page) + 0x648U;
    const uintptr_t u_load_vaddr = hs_page_base(u_page) + 0x650U;
    const uintptr_t s_load_paddr = phys_page_base(s_page) + 0x640U;
    const uintptr_t s_store_paddr = phys_page_base(s_page) + 0x648U;
    const uintptr_t u_load_paddr = phys_page_base(u_page) + 0x650U;
    const uint64_t s_load_initial = 0x52505249562d4853ULL;
    const uint64_t s_store_initial = 0x52505249562d5350ULL;
    const uint64_t s_store_updated = 0x52505249562d5331ULL;
    const uint64_t u_load_initial = 0x52505249562d4855ULL;
    const uint64_t hs_user_sentinel = 0xd0c0000000000a11ULL;
    const uint64_t satp_before = CSRR(CSR_SATP);
    const uint64_t medeleg_before = CSRR(CSR_MEDELEG);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM | MSTATUS_MPRV | MSTATUS_SUM |
        MSTATUS_MXR | AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_ECU);
    write64(s_load_paddr, s_load_initial);
    write64(s_store_paddr, s_store_initial);
    write64(u_load_paddr, u_load_initial);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    const uint64_t active_satp = CSRR(CSR_SATP);
    const uint64_t s_leaf_before =
        *ai_memblock_basic_priv_hs_leaf_entry(s_page);
    const uint64_t u_leaf_before =
        *ai_memblock_basic_priv_hs_leaf_entry(u_page);
    sfence_vma();
    TEST_ASSERT("the MRET-to-HS memory probe starts from a supervisor leaf and a user leaf under the same HS address space",
        (active_satp >> 60) != 0 &&
        (s_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (s_leaf_before & PTE_U) == 0 &&
        (u_leaf_before & (PTE_V | PTE_U | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_U | PTE_AD | PTE_RWX)
    );

    set_prev_priv(PRIV_HS);
    TEST_EXEC_MRET();
    curr_priv = PRIV_HS;
    real_priv = PRIV_HS;
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t hs_s_load = ld(s_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("immediately after MRET to HS, a supervisor-leaf load is judged by the new HS data privilege",
        hs_s_load == s_load_initial);

    TEST_SETUP_EXCEPT();
    sd(s_store_vaddr, s_store_updated);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("immediately after MRET to HS, a supervisor-leaf store updates only the translated HS target",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t hs_u_denied = ai_memblock_basic_priv_ld_rd_probe(u_load_vaddr,
        hs_user_sentinel);
    TEST_ASSERT("with SUM still clear after the return, HS cannot reuse an old permission snapshot to read a PTE.U=1 user leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == u_load_vaddr &&
        hs_u_denied == hs_user_sentinel);

    CSRS(CSR_SSTATUS, SSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    uint64_t hs_u_allowed = ld(u_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("setting SUM after the HS return makes the following HS load use the updated current permission state",
        hs_u_allowed == u_load_initial);

    goto_priv(PRIV_M);
    TEST_ASSERT("MRET-to-HS data-access checks preserve translation metadata and update only the expected backing words",
        CSRR(CSR_SATP) == active_satp &&
        *ai_memblock_basic_priv_hs_leaf_entry(s_page) == s_leaf_before &&
        *ai_memblock_basic_priv_hs_leaf_entry(u_page) == u_leaf_before &&
        read64(s_load_paddr) == s_load_initial &&
        read64(s_store_paddr) == s_store_updated &&
        read64(u_load_paddr) == u_load_initial
    );

    CSRW(CSR_MEDELEG, medeleg_before);
    CSRW(CSR_SATP, satp_before);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | MSTATUS_SUM | MSTATUS_MXR |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK);
    sfence_vma();

    TEST_END();
}

bool ai_micro_memblock_xret_status_restore_matrix_corner()
{
    TEST_START();

    const uint64_t mstatus_before = CSRR(CSR_MSTATUS);
    const uint64_t mtvec_before = CSRR(CSR_MTVEC);
    const uint64_t medeleg_before = CSRR(CSR_MEDELEG);
    const uint64_t mie_before = CSRR(CSR_MIE);
    const uint64_t mip_before = CSRR(CSR_MIP);
    const uint64_t satp_before = CSRR(CSR_SATP);
    const uint64_t return_after_mret =
        (uintptr_t)ai_memblock_xret_status_hs_after_mret;

    goto_priv(PRIV_M);
    CSRW(CSR_MIE, 0);
    CSRW(CSR_MIP, 0);
    CSRC(CSR_MSTATUS, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK |
        MSTATUS_MIE | AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPIE);
    CSRS(CSR_MSTATUS, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_M |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPIE);

    TEST_EXEC_MRET();
    {
        const uint64_t mstatus_after_mret_to_m = CSRR(CSR_MSTATUS);

        TEST_ASSERT("MRET returning to M restores MIE from MPIE, sets MPIE, and clears MPP to U",
            (mstatus_after_mret_to_m & MSTATUS_MIE) != 0 &&
            (mstatus_after_mret_to_m &
                AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPIE) != 0 &&
            (mstatus_after_mret_to_m &
                AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK) ==
                AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_U,
            "mstatus_after_mret_to_m=0x%llx",
            (unsigned long long)mstatus_after_mret_to_m
        );
    }

    ai_memblock_xret_status_hs_landed = 0;
    ai_memblock_xret_status_hs_fell_through = 0;
    ai_memblock_xret_status_m_landed = 0;
    ai_memblock_xret_status_m_handler_seen = 0;
    ai_memblock_xret_status_m_handler_mstatus = 0;
    ai_memblock_xret_status_m_handler_cause = 0;
    ai_memblock_xret_status_m_handler_epc = 0;

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, (uintptr_t)ai_memblock_xret_status_m_handler);
    CSRW(CSR_MIE, 0);
    CSRW(CSR_MIP, 0);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_ECS);
    CSRW(CSR_MEPC, return_after_mret);
    CSRC(CSR_MSTATUS, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK |
        MSTATUS_MIE | AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPIE |
        MSTATUS_MPRV | MSTATUS_SUM | MSTATUS_MXR);
    CSRS(CSR_MSTATUS, AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_S |
        MSTATUS_MPRV | MSTATUS_SUM | MSTATUS_MXR);

    asm volatile(
        "mret\n\t"
        ".global ai_memblock_xret_status_after_custom_m_handler\n"
        "ai_memblock_xret_status_after_custom_m_handler:\n\t"
        ::: "memory");

    curr_priv = PRIV_M;
    real_priv = PRIV_M;
    ai_memblock_xret_status_m_landed = 1;
    const uint64_t mstatus_after_mret = ai_memblock_xret_status_m_handler_mstatus;

    TEST_ASSERT("MRET returns to the programmed mepc in HS and the following HS ecall is handled by the custom M trap entry",
        ai_memblock_xret_status_hs_landed == 1 &&
        ai_memblock_xret_status_hs_fell_through == 0 &&
        ai_memblock_xret_status_m_landed == 1 &&
        ai_memblock_xret_status_m_handler_seen == 1 &&
        ai_memblock_xret_status_m_handler_cause == CAUSE_ECS &&
        ai_memblock_xret_status_m_handler_epc ==
            (uintptr_t)ai_memblock_xret_status_hs_ecall_site,
        "hs_landed=%llu fell=0x%llx m_landed=%llu seen=%llu cause=0x%llx epc=0x%llx expected_epc=0x%llx",
        (unsigned long long)ai_memblock_xret_status_hs_landed,
        (unsigned long long)ai_memblock_xret_status_hs_fell_through,
        (unsigned long long)ai_memblock_xret_status_m_landed,
        (unsigned long long)ai_memblock_xret_status_m_handler_seen,
        (unsigned long long)ai_memblock_xret_status_m_handler_cause,
        (unsigned long long)ai_memblock_xret_status_m_handler_epc,
        (unsigned long long)(uintptr_t)ai_memblock_xret_status_hs_ecall_site
    );
    TEST_ASSERT("MRET returning below M clears MPRV before the next lower-mode instruction traps back to M",
        (mstatus_after_mret & MSTATUS_MPRV) == 0 &&
        (mstatus_after_mret & MSTATUS_SUM) != 0 &&
        (mstatus_after_mret & MSTATUS_MXR) != 0,
        "mstatus_after_mret=0x%llx",
        (unsigned long long)mstatus_after_mret
    );

    CSRW(CSR_MTVEC, mtvec_before);
    CSRW(CSR_MIE, mie_before);
    CSRW(CSR_MIP, mip_before);
    CSRW(CSR_MEDELEG, medeleg_before);
    CSRW(CSR_MSTATUS, mstatus_before);

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SIE |
        AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SPIE |
        AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SPP | SSTATUS_SUM | SSTATUS_MXR);
    CSRS(CSR_SSTATUS, AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SPIE |
        AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SPP | SSTATUS_SUM | SSTATUS_MXR);
    TEST_EXEC_SRET();
    curr_priv = PRIV_HS;
    real_priv = PRIV_HS;
    const uint64_t sstatus_after_sret = CSRR(CSR_SSTATUS);

    TEST_ASSERT("SRET restores SIE from SPIE, sets SPIE, clears SPP to U, and preserves SUM/MXR control state",
        (sstatus_after_sret & AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SIE) != 0 &&
        (sstatus_after_sret & AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SPIE) != 0 &&
        (sstatus_after_sret & AI_MEMBLOCK_BASIC_PRIV_SSTATUS_SPP) == 0 &&
        (sstatus_after_sret & SSTATUS_SUM) != 0 &&
        (sstatus_after_sret & SSTATUS_MXR) != 0,
        "sstatus_after_sret=0x%llx",
        (unsigned long long)sstatus_after_sret
    );

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, satp_before);
    CSRW(CSR_MSTATUS, mstatus_before);
    sfence_vma();

    TEST_END();
}

bool ai_micro_memblock_mprv_effective_privilege_data_access_matrix_corner()
{
    TEST_START();

    const enum test_page s_page = VSRWX_GRWX;
    const enum test_page u_page = VSURWX_GURWX;
    const enum test_page xonly_page = VSUX_GUX;
    const uintptr_t s_load_vaddr = hs_page_base(s_page) + 0x300U;
    const uintptr_t s_store_vaddr = hs_page_base(s_page) + 0x308U;
    const uintptr_t s_amo_vaddr = hs_page_base(s_page) + 0x310U;
    const uintptr_t s_exec_vaddr = hs_page_base(s_page) + 0x318U;
    const uintptr_t u_load_vaddr = hs_page_base(u_page) + 0x320U;
    const uintptr_t u_store_vaddr = hs_page_base(u_page) + 0x328U;
    const uintptr_t u_amo_vaddr = hs_page_base(u_page) + 0x340U;
    const uintptr_t s_lrsc_vaddr = hs_page_base(s_page) + 0x348U;
    const uintptr_t u_lrsc_vaddr = hs_page_base(u_page) + 0x350U;
    const uintptr_t xonly_load_vaddr = hs_page_base(xonly_page) + 0x330U;
    const uintptr_t xonly_exec_vaddr = hs_page_base(xonly_page) + 0x338U;
    const uintptr_t s_load_paddr = phys_page_base(s_page) + 0x300U;
    const uintptr_t s_store_paddr = phys_page_base(s_page) + 0x308U;
    const uintptr_t s_amo_paddr = phys_page_base(s_page) + 0x310U;
    const uintptr_t s_exec_paddr = phys_page_base(s_page) + 0x318U;
    const uintptr_t u_load_paddr = phys_page_base(u_page) + 0x320U;
    const uintptr_t u_store_paddr = phys_page_base(u_page) + 0x328U;
    const uintptr_t u_amo_paddr = phys_page_base(u_page) + 0x340U;
    const uintptr_t s_lrsc_paddr = phys_page_base(s_page) + 0x348U;
    const uintptr_t u_lrsc_paddr = phys_page_base(u_page) + 0x350U;
    const uintptr_t xonly_load_paddr = phys_page_base(xonly_page) + 0x330U;
    const uintptr_t xonly_exec_paddr = phys_page_base(xonly_page) + 0x338U;
    const uint64_t s_load_initial = 0x4d5052562d532d4cULL;
    const uint64_t s_store_initial = 0x4d5052562d532d53ULL;
    const uint64_t s_store_updated = 0x4d5052562d532d57ULL;
    const uint64_t s_amo_initial = 0x1000200030004000ULL;
    const uint64_t s_amo_addend = 0x0001000100010001ULL;
    const uint64_t s_lrsc_initial = 0x0a0b0c0d0e0f0102ULL;
    const uint64_t s_lrsc_updated = 0x0b0c0d0e0f010203ULL;
    const uint64_t u_load_initial = 0x4d5052562d552d4cULL;
    const uint64_t u_store_initial = 0x4d5052562d552d53ULL;
    const uint64_t u_store_updated = 0x4d5052562d552d57ULL;
    const uint64_t u_amo_initial = 0x2000300040005000ULL;
    const uint64_t u_amo_addend = 0x0002000200020002ULL;
    const uint64_t u_lrsc_initial = 0x01020f0e0d0c0b0aULL;
    const uint64_t u_lrsc_updated = 0x02030f0e0d0c0b0aULL;
    const uint64_t xonly_payload = 0x584f4e4c592d4d58ULL;
    const uint64_t satp_before = CSRR(CSR_SATP);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MEDELEG, (1ULL << CAUSE_LPF) | (1ULL << CAUSE_SPF) |
        (1ULL << CAUSE_IPF));
    ai_memblock_basic_priv_clear_mprv_mpp();
    write64(s_load_paddr, s_load_initial);
    write64(s_store_paddr, s_store_initial);
    write64(s_amo_paddr, s_amo_initial);
    write64(s_lrsc_paddr, s_lrsc_initial);
    write32(s_exec_paddr, 0x00008067U);
    write64(u_load_paddr, u_load_initial);
    write64(u_store_paddr, u_store_initial);
    write64(u_amo_paddr, u_amo_initial);
    write64(u_lrsc_paddr, u_lrsc_initial);
    write64(xonly_load_paddr, xonly_payload);
    write32(xonly_exec_paddr, 0x00008067U);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    const uint64_t s_leaf_before =
        *ai_memblock_basic_priv_hs_leaf_entry(s_page);
    const uint64_t u_leaf_before =
        *ai_memblock_basic_priv_hs_leaf_entry(u_page);
    const uint64_t xonly_leaf_before =
        *ai_memblock_basic_priv_hs_leaf_entry(xonly_page);
    TEST_ASSERT("the MPRV matrix starts with supervisor, user, and user execute-only leaves in ordinary DRAM",
        (s_leaf_before & (PTE_V | PTE_RWX | PTE_A | PTE_D)) ==
            (PTE_V | PTE_RWX | PTE_A | PTE_D) &&
        (s_leaf_before & PTE_U) == 0 &&
        (u_leaf_before & (PTE_V | PTE_U | PTE_RWX | PTE_A | PTE_D)) ==
            (PTE_V | PTE_U | PTE_RWX | PTE_A | PTE_D) &&
        (xonly_leaf_before & (PTE_V | PTE_U | PTE_X | PTE_A | PTE_D)) ==
            (PTE_V | PTE_U | PTE_X | PTE_A | PTE_D) &&
        (xonly_leaf_before & PTE_R) == 0
    );

    CSRC(CSR_MSTATUS, MSTATUS_SUM | MSTATUS_MXR);
    ai_memblock_basic_priv_clear_mprv_mpp();

    TEST_SETUP_EXCEPT();
    uint64_t m_direct_load = ld(s_load_paddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("MPRV=0 M-mode data load uses current M privilege and direct physical addressing, not the stale MPP field",
        m_direct_load == s_load_initial);

    TEST_SETUP_EXCEPT();
    sd(s_store_paddr, s_store_updated);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("MPRV=0 M-mode data store uses current M privilege and updates the direct physical word",
        true);

    TEST_SETUP_EXCEPT();
    m_direct_load = ai_memblock_basic_priv_mprv_m_ld(s_load_paddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("MPRV=1 with MPP=M still keeps the effective privilege at M and bypasses S/U page permission checks for data load",
        m_direct_load == s_load_initial);

    TEST_SETUP_EXCEPT();
    uint64_t mprv_s_load =
        ai_memblock_basic_priv_mprv_s_sum1_ld(s_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode data load with MPRV=1 MPP=S is translated and permitted as effective S on a supervisor leaf",
        mprv_s_load == s_load_initial);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum1_sd(s_store_vaddr, s_store_updated);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode data store with MPRV=1 MPP=S is translated and permitted as effective S on a writable supervisor leaf",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t mprv_s_amo_old =
        ai_memblock_basic_priv_mprv_s_sum1_amoadd_d(s_amo_vaddr,
            s_amo_addend);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode AMO with MPRV=1 MPP=S uses the same effective S translation and returns the old value",
        mprv_s_amo_old == s_amo_initial);

    TEST_SETUP_EXCEPT();
    uint64_t mprv_s_lr_old = 0ULL;
    uint64_t mprv_s_sc_status =
        ai_memblock_basic_priv_mprv_s_sum1_lrsc_d_pair(s_lrsc_vaddr,
            s_lrsc_updated, &mprv_s_lr_old);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode LR/SC pair with MPRV=1 MPP=S uses effective S permission and succeeds without C-side reservation interference",
        mprv_s_lr_old == s_lrsc_initial && mprv_s_sc_status == 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t mprv_s_u_load =
        ai_memblock_basic_priv_mprv_s_sum1_ld(u_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode data load with MPRV=1 MPP=S and SUM=1 can access a PTE.U=1 user leaf",
        mprv_s_u_load == u_load_initial);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum1_sd(u_store_vaddr, u_store_updated);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode data store with MPRV=1 MPP=S and SUM=1 can update a PTE.U=1 user leaf",
        true);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum0_ld(u_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("clearing SUM while keeping MPRV=1 MPP=S makes the same user-leaf load fault as effective S",
        CAUSE_LPF, u_load_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum0_sd(u_store_vaddr,
        0x2222333344445555ULL);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("clearing SUM while keeping MPRV=1 MPP=S also makes a user-leaf store fault without updating memory",
        CAUSE_SPF, u_store_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum0_amoadd_d(u_amo_vaddr,
        0x1111111111111111ULL);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=S with SUM=0 makes a user-leaf AMO fault before any read-modify-write side effect",
        CAUSE_SPF, u_amo_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum0_lr_d(u_lrsc_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=S with SUM=0 makes a user-leaf LR fault and prevents reservation setup",
        CAUSE_LPF, u_lrsc_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum0_sc_d(u_lrsc_vaddr,
        0x3333444455556666ULL);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=S with SUM=0 makes a user-leaf SC fault without reporting a false success",
        CAUSE_SPF, u_lrsc_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t mprv_u_load =
        ai_memblock_basic_priv_mprv_u_mxr0_ld(u_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode data load with MPRV=1 MPP=U is translated and permitted as effective U on a PTE.U=1 leaf",
        mprv_u_load == u_load_initial);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_sd(u_store_vaddr, u_store_updated);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode data store with MPRV=1 MPP=U updates a writable PTE.U=1 leaf",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t mprv_u_amo_old =
        ai_memblock_basic_priv_mprv_u_amoadd_d(u_amo_vaddr,
            u_amo_addend);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode AMO with MPRV=1 MPP=U uses effective U translation on a PTE.U=1 leaf",
        mprv_u_amo_old == u_amo_initial);

    TEST_SETUP_EXCEPT();
    uint64_t mprv_u_lr_old = 0ULL;
    uint64_t mprv_u_sc_status =
        ai_memblock_basic_priv_mprv_u_lrsc_d_pair(u_lrsc_vaddr,
            u_lrsc_updated, &mprv_u_lr_old);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("M-mode LR/SC pair with MPRV=1 MPP=U uses effective U permission and succeeds on a PTE.U=1 leaf",
        mprv_u_lr_old == u_lrsc_initial && mprv_u_sc_status == 0ULL);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_mxr0_ld(s_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("M-mode data load with MPRV=1 MPP=U cannot use the old M privilege to read a supervisor leaf",
        CAUSE_LPF, s_load_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_sd(s_store_vaddr, 0x1111222233334444ULL);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("M-mode data store with MPRV=1 MPP=U cannot use the old M privilege to write a supervisor leaf",
        CAUSE_SPF, s_store_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_sum1_ld(s_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("setting SUM while MPRV=1 MPP=U still does not let effective U read a supervisor leaf",
        CAUSE_LPF, s_load_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_amoadd_d(s_amo_vaddr,
        0x2222222222222222ULL);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=U cannot use M privilege to perform AMO on a supervisor leaf",
        CAUSE_SPF, s_amo_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_lr_d(s_lrsc_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=U cannot establish LR reservation on a supervisor leaf",
        CAUSE_LPF, s_lrsc_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_sc_d(s_lrsc_vaddr,
        0x4444555566667777ULL);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=U makes SC to a supervisor leaf fault without changing memory",
        CAUSE_SPF, s_lrsc_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_mxr0_ld(xonly_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=U with MXR=0 rejects a data load from an execute-only user leaf",
        CAUSE_LPF, xonly_load_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum1_mxr0_ld(xonly_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=S with SUM=1 but MXR=0 still rejects a data load from an execute-only user leaf",
        CAUSE_LPF, xonly_load_vaddr);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum0_mxr1_ld(xonly_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=S with SUM=0 rejects an execute-only PTE.U=1 leaf even when MXR would otherwise widen read permission",
        CAUSE_LPF, xonly_load_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t sum_mxr_load =
        ai_memblock_basic_priv_mprv_s_sum1_mxr1_ld(xonly_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("MPRV=1 MPP=S with SUM=1 and MXR=1 can data-load from an execute-only PTE.U=1 leaf",
        sum_mxr_load == xonly_payload);

    TEST_SETUP_EXCEPT();
    uint64_t mxr_load =
        ai_memblock_basic_priv_mprv_u_mxr1_ld(xonly_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("setting MXR under MPRV=1 MPP=U expands data-load permission for an execute-only user leaf",
        mxr_load == xonly_payload);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum1_mxr0_lr_d(xonly_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("MPRV=1 MPP=S with SUM=1 but MXR=0 rejects LR from an execute-only user leaf and cannot create a reservation",
        CAUSE_LPF, xonly_load_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t lr_mxr_s_value =
        ai_memblock_basic_priv_mprv_s_sum1_mxr1_lr_d(xonly_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("MPRV=1 MPP=S with SUM=1 and MXR=1 lets LR read an execute-only PTE.U=1 leaf",
        lr_mxr_s_value == xonly_payload);

    TEST_SETUP_EXCEPT();
    uint64_t lr_mxr_u_value =
        ai_memblock_basic_priv_mprv_u_mxr1_lr_d(xonly_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("MPRV=1 MPP=U with MXR=1 lets LR read an execute-only user leaf",
        lr_mxr_u_value == xonly_payload);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_fetch_m_phys(s_exec_paddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("instruction fetch from M-mode is not redirected by MPRV/MPP and remains a current-M physical fetch",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t csr_sstatus = ai_memblock_basic_priv_csrr_sstatus();
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("CSR access while current privilege is M is not reclassified by MPRV/MPP effective data privilege",
        curr_priv == PRIV_M && (csr_sstatus & SSTATUS_SPP_BIT) == 0);

    ai_memblock_basic_priv_clear_mprv_mpp();
    CSRS(CSR_MSTATUS, MSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    uint64_t restored_m_load = ld(s_load_paddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("after clearing MPRV, the next M-mode data load returns to direct current-M access",
        restored_m_load == s_load_initial);

    goto_priv(PRIV_M);
    TEST_ASSERT("MPRV matrix preserves the leaf encodings and updates only the intended data words",
        *ai_memblock_basic_priv_hs_leaf_entry(s_page) == s_leaf_before &&
        *ai_memblock_basic_priv_hs_leaf_entry(u_page) == u_leaf_before &&
        *ai_memblock_basic_priv_hs_leaf_entry(xonly_page) ==
            xonly_leaf_before &&
        read64(s_load_paddr) == s_load_initial &&
        read64(s_store_paddr) == s_store_updated &&
        read64(s_amo_paddr) == s_amo_initial + s_amo_addend &&
        read64(s_lrsc_paddr) == s_lrsc_updated &&
        read64(u_load_paddr) == u_load_initial &&
        read64(u_store_paddr) == u_store_updated &&
        read64(u_amo_paddr) == u_amo_initial + u_amo_addend &&
        read64(u_lrsc_paddr) == u_lrsc_updated &&
        read64(xonly_load_paddr) == xonly_payload &&
        read32(s_exec_paddr) == 0x00008067U &&
        read32(xonly_exec_paddr) == 0x00008067U
    );

    CSRW(CSR_SATP, satp_before);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | MSTATUS_SUM | MSTATUS_MXR |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK);
    sfence_vma();

    TEST_END();
}

bool ai_micro_memblock_sum_mxr_pteu_prefetch_silent_no_side_effect_corner()
{
    TEST_START();

    const enum test_page u_page = VSURWX_GURWX;
    const enum test_page xonly_user_page = VSUX_GUX;
    const enum test_page supervisor_page = VSRWX_GRWX;
    const uintptr_t u_load_vaddr = hs_page_base(u_page) + 0x720U;
    const uintptr_t u_store_vaddr = hs_page_base(u_page) + 0x728U;
    const uintptr_t xonly_load_vaddr = hs_page_base(xonly_user_page) + 0x730U;
    const uintptr_t supervisor_load_vaddr =
        hs_page_base(supervisor_page) + 0x738U;
    const uintptr_t u_load_paddr = phys_page_base(u_page) + 0x720U;
    const uintptr_t u_store_paddr = phys_page_base(u_page) + 0x728U;
    const uintptr_t xonly_load_paddr =
        phys_page_base(xonly_user_page) + 0x730U;
    const uintptr_t supervisor_load_paddr =
        phys_page_base(supervisor_page) + 0x738U;
    const uint64_t u_load_initial = 0x5052454653485552ULL;
    const uint64_t u_store_initial = 0x5052454653485557ULL;
    const uint64_t xonly_initial = 0x50524546584f4e4cULL;
    const uint64_t supervisor_initial = 0x5052454653485350ULL;
    const uint64_t u_store_allowed = 0x5052454653485331ULL;
    const uint64_t hs_sum0_sentinel = 0xabcd000000000001ULL;
    const uint64_t hs_xonly_sentinel = 0xabcd000000000002ULL;
    const uint64_t satp_before = CSRR(CSR_SATP);
    const uint64_t medeleg_before = CSRR(CSR_MEDELEG);
    const uint64_t trap_scause_sentinel = 0x5a5ULL;
    const uint64_t trap_mcause_sentinel = 0x6b6ULL;
    const uint64_t trap_tval_zero = 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM | MSTATUS_MPRV | MSTATUS_SUM |
        MSTATUS_MXR | AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK);
    write64(u_load_paddr, u_load_initial);
    write64(u_store_paddr, u_store_initial);
    write64(xonly_load_paddr, xonly_initial);
    write64(supervisor_load_paddr, supervisor_initial);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    const uint64_t active_satp = CSRR(CSR_SATP);
    const uint64_t u_leaf_before =
        *ai_memblock_basic_priv_hs_leaf_entry(u_page);
    const uint64_t xonly_leaf_before =
        *ai_memblock_basic_priv_hs_leaf_entry(xonly_user_page);
    const uint64_t supervisor_leaf_before =
        *ai_memblock_basic_priv_hs_leaf_entry(supervisor_page);
    TEST_ASSERT("the prefetch permission matrix starts from a PTE.U=1 RWX leaf, a PTE.U=1 execute-only leaf, and a supervisor RWX leaf",
        (active_satp >> 60) != 0 &&
        (u_leaf_before & (PTE_V | PTE_U | PTE_RWX | PTE_A | PTE_D)) ==
            (PTE_V | PTE_U | PTE_RWX | PTE_A | PTE_D) &&
        (xonly_leaf_before & (PTE_V | PTE_U | PTE_X | PTE_A |
            PTE_D)) == (PTE_V | PTE_U | PTE_X | PTE_A | PTE_D) &&
        (xonly_leaf_before & (PTE_R | PTE_W)) == 0 &&
        (supervisor_leaf_before & (PTE_V | PTE_RWX | PTE_A |
            PTE_D)) == (PTE_V | PTE_RWX | PTE_A | PTE_D) &&
        (supervisor_leaf_before & PTE_U) == 0
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    CSRW(CSR_SCAUSE, trap_scause_sentinel);
    CSRW(CSR_STVAL, trap_tval_zero);

    TEST_SETUP_EXCEPT();
    prefetch_r(u_load_vaddr);
    TEST_ASSERT("HS prefetch.r to a PTE.U=1 leaf with SUM=0 is a silent hint and does not refresh S trap metadata",
        excpt.triggered == false &&
        CSRR(CSR_SCAUSE) == trap_scause_sentinel &&
        CSRR(CSR_STVAL) == trap_tval_zero
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(u_store_vaddr);
    TEST_ASSERT("HS prefetch.w to the same PTE.U=1 leaf with SUM=0 is also silent and keeps S trap metadata unchanged",
        excpt.triggered == false &&
        CSRR(CSR_SCAUSE) == trap_scause_sentinel &&
        CSRR(CSR_STVAL) == trap_tval_zero
    );

    TEST_SETUP_EXCEPT();
    uint64_t denied_hs_sum0 = ai_memblock_basic_priv_ld_rd_probe(
        u_load_vaddr, hs_sum0_sentinel);
    TEST_ASSERT("a real HS load to the same PTE.U=1 leaf with SUM=0 still reports LPF and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == u_load_vaddr &&
        denied_hs_sum0 == hs_sum0_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(u_store_vaddr, 0x1111222233334444ULL);
    AI_MEMBLOCK_BASIC_ASSERT_EXCEPT("a real HS store to the same PTE.U=1 leaf with SUM=0 still reports SPF and has no write side effect",
        CAUSE_SPF, u_store_vaddr);

    CSRS(CSR_SSTATUS, SSTATUS_SUM);
    CSRC(CSR_SSTATUS, SSTATUS_MXR);
    CSRW(CSR_SCAUSE, trap_scause_sentinel + 1ULL);
    CSRW(CSR_STVAL, trap_tval_zero);

    TEST_SETUP_EXCEPT();
    prefetch_r(xonly_load_vaddr);
    TEST_ASSERT("HS prefetch.r to an execute-only PTE.U=1 leaf with SUM=1/MXR=0 is silent and does not refresh trap metadata",
        excpt.triggered == false &&
        CSRR(CSR_SCAUSE) == trap_scause_sentinel + 1ULL &&
        CSRR(CSR_STVAL) == trap_tval_zero
    );

    TEST_SETUP_EXCEPT();
    uint64_t denied_hs_xonly = ai_memblock_basic_priv_ld_rd_probe(
        xonly_load_vaddr, hs_xonly_sentinel);
    TEST_ASSERT("a real HS load to the execute-only PTE.U=1 leaf with MXR=0 still reports LPF and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == xonly_load_vaddr &&
        denied_hs_xonly == hs_xonly_sentinel
    );

    CSRS(CSR_SSTATUS, SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t hs_mxr_allowed = ld(xonly_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("setting MXR=1 makes the same execute-only PTE.U=1 leaf readable, proving the silent prefetch was on a real MXR-denied path",
        hs_mxr_allowed == xonly_initial);

    TEST_SETUP_EXCEPT();
    uint64_t hs_sum_mxr_allowed = ld(u_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("setting SUM=1 makes the earlier PTE.U=1 RWX leaf readable through the same HS translation",
        hs_sum_mxr_allowed == u_load_initial);

    TEST_SETUP_EXCEPT();
    sd(u_store_vaddr, u_store_allowed);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("setting SUM=1 lets the real HS store update the PTE.U=1 leaf only after the silent prefetch checks",
        true);

    goto_priv(PRIV_M);
    CSRW(CSR_MCAUSE, trap_mcause_sentinel);
    CSRW(CSR_MTVAL, trap_tval_zero);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | MSTATUS_SUM | MSTATUS_MXR |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum0_prefetch_r(u_load_vaddr);
    TEST_ASSERT("M-mode prefetch.r with MPRV=1/MPP=S/SUM=0 to a PTE.U=1 leaf is silent and keeps M trap metadata unchanged",
        excpt.triggered == false &&
        CSRR(CSR_MCAUSE) == trap_mcause_sentinel &&
        CSRR(CSR_MTVAL) == trap_tval_zero
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum0_prefetch_w(u_store_vaddr);
    TEST_ASSERT("M-mode prefetch.w with MPRV=1/MPP=S/SUM=0 to a PTE.U=1 leaf is silent and has no visible side effect",
        excpt.triggered == false &&
        CSRR(CSR_MCAUSE) == trap_mcause_sentinel &&
        CSRR(CSR_MTVAL) == trap_tval_zero
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum0_ld(u_load_vaddr);
    TEST_ASSERT("a real M-mode data load with MPRV=1/MPP=S/SUM=0 to that PTE.U=1 leaf still reports LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == u_load_vaddr
    );

    CSRW(CSR_MCAUSE, trap_mcause_sentinel + 1ULL);
    CSRW(CSR_MTVAL, trap_tval_zero);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum1_mxr0_prefetch_r(xonly_load_vaddr);
    TEST_ASSERT("M-mode prefetch.r with MPRV=1/MPP=S/SUM=1/MXR=0 to an execute-only PTE.U=1 leaf is silent",
        excpt.triggered == false &&
        CSRR(CSR_MCAUSE) == trap_mcause_sentinel + 1ULL &&
        CSRR(CSR_MTVAL) == trap_tval_zero
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_s_sum1_mxr0_ld(xonly_load_vaddr);
    TEST_ASSERT("a real MPRV=1/MPP=S/SUM=1/MXR=0 load to that execute-only user leaf still reports LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == xonly_load_vaddr
    );

    CSRW(CSR_MCAUSE, trap_mcause_sentinel + 2ULL);
    CSRW(CSR_MTVAL, trap_tval_zero);

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_mxr0_prefetch_r(supervisor_load_vaddr);
    TEST_ASSERT("M-mode prefetch.r with MPRV=1/MPP=U to a supervisor leaf is silent and cannot refresh M trap metadata",
        excpt.triggered == false &&
        CSRR(CSR_MCAUSE) == trap_mcause_sentinel + 2ULL &&
        CSRR(CSR_MTVAL) == trap_tval_zero
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_basic_priv_mprv_u_mxr0_ld(supervisor_load_vaddr);
    TEST_ASSERT("a real MPRV=1/MPP=U load to the same supervisor leaf still reports LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == supervisor_load_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t allowed_mprv_u_user =
        ai_memblock_basic_priv_mprv_u_mxr0_ld(u_load_vaddr);
    AI_MEMBLOCK_BASIC_ASSERT_NO_EXCEPT_AND("MPRV=1/MPP=U can still load an ordinary readable PTE.U=1 leaf, so the supervisor-leaf prefetch check used effective U permission",
        allowed_mprv_u_user == u_load_initial);

    goto_priv(PRIV_M);
    TEST_ASSERT("silent prefetch permission rejects preserve leaf encodings and do not update backing memory before the later real allowed store",
        CSRR(CSR_SATP) == active_satp &&
        *ai_memblock_basic_priv_hs_leaf_entry(u_page) == u_leaf_before &&
        *ai_memblock_basic_priv_hs_leaf_entry(xonly_user_page) ==
            xonly_leaf_before &&
        *ai_memblock_basic_priv_hs_leaf_entry(supervisor_page) ==
            supervisor_leaf_before &&
        read64(u_load_paddr) == u_load_initial &&
        read64(u_store_paddr) == u_store_allowed &&
        read64(xonly_load_paddr) == xonly_initial &&
        read64(supervisor_load_paddr) == supervisor_initial
    );

    CSRW(CSR_MEDELEG, medeleg_before);
    CSRW(CSR_SATP, satp_before);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | MSTATUS_SUM | MSTATUS_MXR |
        AI_MEMBLOCK_BASIC_PRIV_MSTATUS_MPP_MASK);
    sfence_vma();

    TEST_END();
}
