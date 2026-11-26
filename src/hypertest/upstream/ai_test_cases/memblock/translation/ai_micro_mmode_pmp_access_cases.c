#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_M_ACCESS_PAGE VSRWX_GRWX

#define AI_MICRO_M_ACCESS_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_M_ACCESS_PMP_OFF 0x00ULL
#define AI_MICRO_M_ACCESS_PMP_OFF_LOCKED 0x80ULL
#define AI_MICRO_M_ACCESS_PMP_TOR_R 0x09ULL
#define AI_MICRO_M_ACCESS_PMP_TOR_RW 0x0bULL
#define AI_MICRO_M_ACCESS_PMP_TOR_LOCKED 0x88ULL
#define AI_MICRO_M_ACCESS_PMP_TOR_NO_R 0x0eULL
#define AI_MICRO_M_ACCESS_PMP_TOR_NO_W 0x0dULL
#define AI_MICRO_M_ACCESS_PMP_TOR_NO_R_LOCKED 0x8eULL
#define AI_MICRO_M_ACCESS_PMP_TOR_NO_W_LOCKED 0x8dULL
#define AI_MICRO_M_ACCESS_PMP_NAPOT_R 0x19ULL
#define AI_MICRO_M_ACCESS_PMP_NAPOT_RW 0x1bULL
#define AI_MICRO_M_ACCESS_PMP_NAPOT_X 0x1cULL
#define AI_MICRO_M_ACCESS_PMP_NAPOT_RWX 0x1fULL
#define AI_MICRO_M_ACCESS_PMP_NAPOT_NO_R 0x1eULL
#define AI_MICRO_M_ACCESS_PMP_NAPOT_NONE_LOCKED 0x98ULL

#define AI_MICRO_M_ACCESS_LOCKED_LOAD_U_PADDR (TEST_PPAGE_BASE + 0x300000ULL)
#define AI_MICRO_M_ACCESS_LOCKED_LOAD_M_PADDR (TEST_PPAGE_BASE + 0x302000ULL)
#define AI_MICRO_M_ACCESS_LOCKED_STORE_U_PADDR (TEST_PPAGE_BASE + 0x304000ULL)
#define AI_MICRO_M_ACCESS_LOCKED_STORE_M_PADDR (TEST_PPAGE_BASE + 0x306000ULL)
#define AI_MICRO_M_ACCESS_ALT_PAGE SWITCH1
#define AI_MICRO_M_ACCESS_MSTATUS_MPP_MASK (3ULL << 11)
#define AI_MICRO_M_ACCESS_MSTATUS_MPP_S (1ULL << 11)
#define AI_MICRO_M_ACCESS_MSTATUS_MPP_M (3ULL << 11)

#define AI_MICRO_M_ACCESS_CSRR_INST(csr) \
    (((uint32_t)(csr) << 20) | (2U << 12) | (5U << 7) | 0x73U)
#define AI_MICRO_M_ACCESS_CSRW_INST(csr) \
    (((uint32_t)(csr) << 20) | (5U << 15) | (1U << 12) | 0x73U)

static inline void ai_micro_m_access_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_MICRO_M_ACCESS_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline uint64_t ai_micro_m_access_pmpcfg_entry(uint64_t cfg,
    unsigned entry)
{
    return (cfg >> (entry * 8U)) & 0xffULL;
}

static inline void ai_micro_m_access_fixed_csrr_pmpcfg0(void)
{
    asm volatile("csrr t0, pmpcfg0" ::: "t0", "memory");
}

static inline void ai_micro_m_access_fixed_csrw_pmpaddr0(uint64_t value)
{
    asm volatile(
        "mv t0, %0\n\t"
        "csrw pmpaddr0, t0\n\t"
        :
        : "r"(value)
        : "t0", "memory");
}

static inline uint64_t ai_micro_m_access_csrrw_pmpcfg0(uint64_t value)
{
    uint64_t old_value = 0ULL;

    asm volatile(
        "csrrw %0, pmpcfg0, %1\n\t"
        : "=r"(old_value)
        : "r"(value)
        : "memory");
    return old_value;
}

static inline uint64_t ai_micro_m_access_csrrs_pmpcfg0(uint64_t mask)
{
    uint64_t old_value = 0ULL;

    asm volatile(
        "csrrs %0, pmpcfg0, %1\n\t"
        : "=r"(old_value)
        : "r"(mask)
        : "memory");
    return old_value;
}

static inline uint64_t ai_micro_m_access_csrrc_pmpcfg0(uint64_t mask)
{
    uint64_t old_value = 0ULL;

    asm volatile(
        "csrrc %0, pmpcfg0, %1\n\t"
        : "=r"(old_value)
        : "r"(mask)
        : "memory");
    return old_value;
}

static inline void ai_micro_m_access_csrsi_pmpcfg0_bit0(void)
{
    asm volatile("csrsi pmpcfg0, 1" ::: "memory");
}

static inline void ai_micro_m_access_csrci_pmpcfg0_bit2(void)
{
    asm volatile("csrci pmpcfg0, 4" ::: "memory");
}

static inline bool ai_micro_m_access_check_illegal_with_inst(
    uint32_t expected_inst)
{
    return excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        (uint32_t)excpt.tval == expected_inst &&
        excpt.epc != 0;
}

static inline uint64_t ai_micro_m_access_lr_d_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    asm volatile("lr.d %0, 0(%1)" : "+&r"(sentinel) : "r"(addr) : "memory");
    return sentinel;
}

static inline uint64_t ai_micro_m_access_amoadd_d_rd_probe(uintptr_t addr,
    uint64_t sentinel, uint64_t value)
{
    asm volatile("amoadd.d %0, %2, 0(%1)"
        : "+&r"(sentinel)
        : "r"(addr), "r"(value)
        : "memory");
    return sentinel;
}

static inline uint64_t ai_micro_m_access_sc_d_status_probe(uintptr_t addr,
    uint64_t sentinel, uint64_t value)
{
    asm volatile("sc.d %0, %2, 0(%1)"
        : "+&r"(sentinel)
        : "r"(addr), "r"(value)
        : "memory");
    return sentinel;
}

static inline uint64_t ai_micro_m_access_lr_sc_d_pair(uintptr_t addr,
    uint64_t new_value, uint64_t *old_value)
{
    uint64_t old_reg = 0ULL;
    uint64_t status = ~0ULL;

    asm volatile(
        "lr.d %0, 0(%2)\n\t"
        "sc.d %1, %3, 0(%2)\n\t"
        : "=&r"(old_reg), "=&r"(status)
        : "r"(addr), "r"(new_value)
        : "memory");
    *old_value = old_reg;
    return status;
}

static inline void ai_micro_m_access_set_mprv_mpp(uint64_t mpp_bits)
{
    CSRC(CSR_MSTATUS, MSTATUS_MPRV |
        AI_MICRO_M_ACCESS_MSTATUS_MPP_MASK);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV | mpp_bits);
}

static inline uint64_t ai_micro_m_access_make_napot_4k(uintptr_t base)
{
    return (base >> 2) | 0x1ffULL;
}

static inline void ai_micro_m_access_clear_pmp_entries_0_3(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, 0ULL);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPADDR3, 0ULL);
}

static inline void ai_micro_m_access_configure_tor_entry1(uintptr_t start,
    uintptr_t end, uint64_t cfg)
{
    goto_priv(PRIV_M);
    ai_micro_m_access_clear_pmp_entries_0_3();
    CSRW(CSR_PMPADDR0, start >> 2);
    CSRW(CSR_PMPADDR1, end >> 2);
    CSRW(CSR_PMPCFG0, AI_MICRO_M_ACCESS_PMP_TOR_RWX | (cfg << 8));
}

static inline void ai_micro_m_access_configure_prefix_allow_only(
    uintptr_t prefix_end)
{
    goto_priv(PRIV_M);
    ai_micro_m_access_clear_pmp_entries_0_3();
    CSRW(CSR_PMPADDR0, prefix_end >> 2);
    CSRW(CSR_PMPCFG0, AI_MICRO_M_ACCESS_PMP_TOR_RWX);
}

static inline void ai_micro_m_access_configure_napot_entry1(uintptr_t base,
    uint64_t cfg)
{
    goto_priv(PRIV_M);
    ai_micro_m_access_clear_pmp_entries_0_3();
    CSRW(CSR_PMPADDR0, base >> 2);
    CSRW(CSR_PMPADDR1, ai_micro_m_access_make_napot_4k(base));
    CSRW(CSR_PMPCFG0, AI_MICRO_M_ACCESS_PMP_TOR_RWX | (cfg << 8));
}

static inline void ai_micro_m_access_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_M_ACCESS_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_M_ACCESS_PMP_TOR_RWX << 16));
}

static inline void ai_micro_m_access_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_m_access_prepare_user_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();
}

static inline void ai_micro_m_access_enable_mprv_u_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
}

static inline void ai_micro_m_access_fence_i(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".4byte 0x0000100f\n\t"
        ".option pop\n\t"
        :
        :
        : "memory");
}

static inline void ai_micro_m_access_exec_ret_probe(uintptr_t addr)
{
    asm volatile(
        "addi sp, sp, -16\n\t"
        "sd ra, 0(sp)\n\t"
        "la t0, 2f\n\t"
        "sd t0, %0\n\t"
        "la ra, 1f\n\t"
        "jr %1\n\t"
        "1:\n\t"
        "sd zero, %0\n\t"
        "2:\n\t"
        "ld ra, 0(sp)\n\t"
        "addi sp, sp, 16\n\t"
        : "+m"(excpt.fault_inst)
        : "r"(addr)
        : "t0", "memory");
}

bool ai_micro_m_pmp_match_lock_granularity_matrix_corner()
{
    TEST_START();

    const uintptr_t base = phys_page_base(AI_MICRO_M_ACCESS_PAGE);
    const uintptr_t inside = base + 0x100U;
    const uintptr_t edge_low = base;
    const uintptr_t edge_high = base + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t outside = base + PAGE_SIZE;
    const uintptr_t exec_target = base + 0x380U;
    const uint64_t initial = 0x3141592653589793ULL;
    const uint64_t store_value = 0xa5a55a5adeadbeefULL;
    const uint64_t overlap_store_attempt = 0x0badf00ddeadc0deULL;
    const uint64_t rw_store_value = 0x5a5aa5a501234567ULL;
    const uint64_t amo_delta = 0x10ULL;
    const uint32_t ret_inst = 0x00008067U;

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    write64(edge_low, initial);
    write64(inside, initial);
    write64(edge_high, initial);
    write64(outside, initial);
    write32(exec_target, ret_inst);
    ai_micro_m_access_fence_i();

    ai_micro_m_access_configure_prefix_allow_only(base);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(inside);
    TEST_ASSERT("with the target page not matched by any active low PMP entry, a current M-mode direct load uses the no-match M default and succeeds",
        excpt.triggered == false &&
        observed == initial
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_set_mprv_mpp(AI_MICRO_M_ACCESS_MSTATUS_MPP_M);
    TEST_SETUP_EXCEPT();
    observed = ld(inside);
    TEST_ASSERT("MPRV=1/MPP=M keeps PMP no-match behavior on the effective M path and allows the same direct load",
        excpt.triggered == false &&
        observed == initial
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_set_mprv_mpp(AI_MICRO_M_ACCESS_MSTATUS_MPP_S);
    TEST_SETUP_EXCEPT();
    ld(inside);
    TEST_ASSERT("MPRV=1/MPP=S makes a current M-mode data load use the effective S PMP no-match default and report load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == inside
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_set_mprv_mpp(AI_MICRO_M_ACCESS_MSTATUS_MPP_S);
    TEST_SETUP_EXCEPT();
    sd(inside, store_value);
    TEST_ASSERT("MPRV=1/MPP=S makes a current M-mode data store use the effective S PMP no-match default and report store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == inside
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_set_mprv_mpp(0ULL);
    TEST_SETUP_EXCEPT();
    ld(inside);
    TEST_ASSERT("MPRV=1/MPP=U makes a current M-mode data load use the effective U PMP no-match default and report load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == inside
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_set_mprv_mpp(0ULL);
    TEST_SETUP_EXCEPT();
    sd(inside, store_value);
    TEST_ASSERT("MPRV=1/MPP=U makes a current M-mode data store use the effective U PMP no-match default and report store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == inside
    );
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ld(inside);
    TEST_ASSERT("with only the lower prefix allowed by PMP, a current HU direct load to the unmatched target page reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == inside
    );

    ai_micro_m_access_configure_tor_entry1(base, base + PAGE_SIZE,
        AI_MICRO_M_ACCESS_PMP_TOR_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(edge_low);
    TEST_ASSERT("a TOR entry includes its lower bound and allows an HS load at the first byte of the protected page",
        excpt.triggered == false &&
        observed == initial
    );

    TEST_SETUP_EXCEPT();
    observed = ld(edge_high);
    TEST_ASSERT("a TOR entry includes the last naturally aligned doubleword below its upper bound",
        excpt.triggered == false &&
        observed == initial
    );

    TEST_SETUP_EXCEPT();
    ld(outside);
    TEST_ASSERT("a TOR entry excludes the address equal to its upper bound, so the HS load falls through to no-match and faults",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == outside
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_clear_pmp_entries_0_3();
    CSRW(CSR_PMPADDR0, base >> 2);
    CSRW(CSR_PMPADDR1, ai_micro_m_access_make_napot_4k(base));
    CSRW(CSR_PMPADDR2, ai_micro_m_access_make_napot_4k(base));
    CSRW(CSR_PMPCFG0,
        AI_MICRO_M_ACCESS_PMP_TOR_RWX |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_NO_R << 8) |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 16));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(inside);
    TEST_ASSERT("when two NAPOT entries overlap, the lower-numbered deny-read entry wins over the later RWX entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == inside
    );

    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_M_ACCESS_PMP_TOR_RWX |
        (AI_MICRO_M_ACCESS_PMP_OFF << 8) |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 16));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(inside);
    TEST_ASSERT("after disabling the lower-numbered overlapping entry, the later NAPOT RWX entry becomes the selected match and allows the load",
        excpt.triggered == false &&
        observed == initial
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_clear_pmp_entries_0_3();
    CSRW(CSR_PMPADDR0, base >> 2);
    CSRW(CSR_PMPADDR1, ai_micro_m_access_make_napot_4k(base));
    CSRW(CSR_PMPADDR2, ai_micro_m_access_make_napot_4k(base));
    CSRW(CSR_PMPCFG0,
        AI_MICRO_M_ACCESS_PMP_TOR_RWX |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_R << 8) |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 16));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(inside, overlap_store_attempt);
    TEST_ASSERT("when overlapping NAPOT entries both match a store, the lower-numbered R-only entry wins over the later RWX entry and reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == inside
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(inside, amo_delta);
    TEST_ASSERT("when overlapping NAPOT entries both match an AMO, the lower-numbered R-only entry wins over the later RWX entry and reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == inside
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the store and AMO denied by the lower-numbered overlapping entry do not partially update the target doubleword",
        read64(inside) == initial
    );

    CSRW(CSR_PMPCFG0,
        AI_MICRO_M_ACCESS_PMP_TOR_RWX |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RW << 8) |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 16));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_m_access_exec_ret_probe(exec_target);
    TEST_ASSERT("when overlapping NAPOT entries both match a fetch, the lower-numbered no-X entry wins over the later RWX entry and reports instruction access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == exec_target
    );

    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_M_ACCESS_PMP_TOR_RWX |
        (AI_MICRO_M_ACCESS_PMP_OFF << 8) |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 16));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_m_access_exec_ret_probe(exec_target);
    TEST_ASSERT("after disabling the lower-numbered no-X entry, the later NAPOT RWX entry becomes the selected match and allows the fetch",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_napot_entry1(base,
        AI_MICRO_M_ACCESS_PMP_NAPOT_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(inside);
    TEST_ASSERT("a NAPOT R-only entry allows an HS load to the protected page",
        excpt.triggered == false &&
        observed == initial
    );

    TEST_SETUP_EXCEPT();
    sd(inside, store_value);
    TEST_ASSERT("the same NAPOT R-only entry denies an HS store and binds store access fault to the target address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == inside
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied store under the NAPOT R-only entry leaves the target doubleword unchanged",
        read64(inside) == initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(inside, amo_delta);
    TEST_ASSERT("the NAPOT R-only entry also denies AMO because AMO needs write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == inside
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied AMO does not perform a partial RMW update",
        read64(inside) == initial
    );
    ai_micro_m_access_configure_napot_entry1(base,
        AI_MICRO_M_ACCESS_PMP_NAPOT_RW);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(inside);
    TEST_ASSERT("a NAPOT RW entry allows an HS load before the AMO and store permission probes",
        excpt.triggered == false &&
        observed == initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(inside, amo_delta);
    TEST_ASSERT("a NAPOT RW entry allows AMO and returns the old value",
        excpt.triggered == false &&
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the allowed AMO under the NAPOT RW entry writes back the RMW result to the target doubleword only",
        read64(inside) == initial + amo_delta &&
        read64(edge_low) == initial &&
        read64(edge_high) == initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(edge_low, rw_store_value);
    TEST_ASSERT("the same NAPOT RW entry allows an HS store after the AMO permission probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the allowed store under the NAPOT RW entry updates only the addressed doubleword",
        read64(edge_low) == rw_store_value &&
        read64(inside) == initial + amo_delta &&
        read64(edge_high) == initial
    );

    ai_micro_m_access_clear_pmp_entries_0_3();
    CSRW(CSR_PMPADDR0, (base + PAGE_SIZE) >> 2);
    CSRW(CSR_PMPADDR1, base >> 2);
    CSRW(CSR_PMPCFG0, AI_MICRO_M_ACCESS_PMP_TOR_RWX << 8);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(inside);
    TEST_ASSERT("a TOR entry whose upper bound is below its lower bound is empty, so an HS load falls through to no-match and faults",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == inside
    );

    ai_micro_m_access_clear_pmp_entries_0_3();
    CSRW(CSR_PMPADDR1, (base >> 2) | 0x3ffULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_M_ACCESS_PMP_TOR_RWX << 8);
    TEST_ASSERT("on this 4KB-granularity platform, TOR pmpaddr writes clear the low granularity bits instead of preserving a sub-page boundary",
        (CSRR(CSR_PMPADDR1) & 0x3ffULL) == 0
    );

    CSRW(CSR_PMPCFG0, AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 8);
    CSRW(CSR_PMPADDR1, (base >> 2) | 0x111ULL);
    TEST_ASSERT("on this 4KB-granularity platform, NAPOT pmpaddr writes keep the low NAPOT mask bits all ones and do not create a functional NA4-sized region",
        (CSRR(CSR_PMPADDR1) & 0x1ffULL) == 0x1ffULL
    );

    ai_micro_m_access_clear_pmp_entries_0_3();
    const uint64_t entry0_before = AI_MICRO_M_ACCESS_PMP_TOR_RWX;
    const uint64_t entry1_before = AI_MICRO_M_ACCESS_PMP_NAPOT_R;
    const uint64_t entry2_before = AI_MICRO_M_ACCESS_PMP_OFF;
    CSRW(CSR_PMPCFG0, entry0_before | (entry1_before << 8) |
        (entry2_before << 16));
    const uint64_t packed_cfg_before = CSRR(CSR_PMPCFG0);
    CSRW(CSR_PMPCFG0, (packed_cfg_before & ~0xff00ULL) |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RW << 8));
    const uint64_t packed_cfg_after = CSRR(CSR_PMPCFG0);
    TEST_ASSERT("updating a single pmpcfg byte field keeps adjacent entry encodings stable in the packed CSR",
        ai_micro_m_access_pmpcfg_entry(packed_cfg_after, 0) ==
            entry0_before &&
        ai_micro_m_access_pmpcfg_entry(packed_cfg_after, 1) ==
            AI_MICRO_M_ACCESS_PMP_NAPOT_RW &&
        ai_micro_m_access_pmpcfg_entry(packed_cfg_after, 2) ==
            entry2_before
    );

    TEST_SETUP_EXCEPT();
    uint64_t csrrw_old =
        ai_micro_m_access_csrrw_pmpcfg0(packed_cfg_before);
    TEST_ASSERT("csrrw pmpcfg0 returns the old packed value and installs the requested legal PMP configuration",
        excpt.triggered == false &&
        csrrw_old == packed_cfg_after &&
        CSRR(CSR_PMPCFG0) == packed_cfg_before
    );

    TEST_SETUP_EXCEPT();
    uint64_t csrrs_old = ai_micro_m_access_csrrs_pmpcfg0(
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 16));
    TEST_ASSERT("csrrs pmpcfg0 updates only the selected legal bits and returns the prior packed PMP configuration",
        excpt.triggered == false &&
        csrrs_old == packed_cfg_before &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 0) ==
            entry0_before &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 1) ==
            entry1_before &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 2) ==
            AI_MICRO_M_ACCESS_PMP_NAPOT_RWX
    );

    const uint64_t cfg_before_clear = CSRR(CSR_PMPCFG0);
    TEST_SETUP_EXCEPT();
    uint64_t csrrc_old = ai_micro_m_access_csrrc_pmpcfg0(
        AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 16);
    TEST_ASSERT("csrrc pmpcfg0 clears only the selected writable PMP bits and returns the prior packed value",
        excpt.triggered == false &&
        csrrc_old == cfg_before_clear &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 0) ==
            entry0_before &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 1) ==
            entry1_before &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 2) ==
            AI_MICRO_M_ACCESS_PMP_OFF
    );

    TEST_SETUP_EXCEPT();
    ai_micro_m_access_csrsi_pmpcfg0_bit0();
    TEST_ASSERT("csrsi pmpcfg0 immediate writes affect only the requested bit in entry0 and do not touch neighboring PMP entries",
        excpt.triggered == false &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 0) ==
            entry0_before &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 1) ==
            entry1_before &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 2) ==
            AI_MICRO_M_ACCESS_PMP_OFF
    );

    TEST_SETUP_EXCEPT();
    ai_micro_m_access_csrci_pmpcfg0_bit2();
    TEST_ASSERT("csrci pmpcfg0 immediate clears only a legal permission bit in entry0 and leaves neighboring entries stable",
        excpt.triggered == false &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 0) ==
            (entry0_before & ~4ULL) &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 1) ==
            entry1_before &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 2) ==
            AI_MICRO_M_ACCESS_PMP_OFF
    );

    CSRW(CSR_PMPCFG0, packed_cfg_before);
    ai_micro_m_access_clear_pmp_entries_0_3();
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPADDR3, 0x12345ULL);
    CSRW(CSR_PMPCFG0,
        (AI_MICRO_M_ACCESS_PMP_TOR_LOCKED << 16) |
        (AI_MICRO_M_ACCESS_PMP_OFF_LOCKED << 24));
    const uint64_t locked_cfg = CSRR(CSR_PMPCFG0);
    const uint64_t locked_addr1 = CSRR(CSR_PMPADDR1);
    const uint64_t locked_addr2 = CSRR(CSR_PMPADDR2);
    const uint64_t locked_addr3 = CSRR(CSR_PMPADDR3);

    CSRW(CSR_PMPADDR1, (base + PAGE_SIZE) >> 2);
    CSRW(CSR_PMPADDR2, (base + 2 * PAGE_SIZE) >> 2);
    CSRW(CSR_PMPADDR3, ai_micro_m_access_make_napot_4k(base));
    CSRW(CSR_PMPCFG0,
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 16) |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 24));

    TEST_ASSERT("locked PMP entries ignore later pmpcfg writes; a locked TOR entry also protects its pmpaddr[i] and pmpaddr[i-1], and a locked OFF entry protects its own pmpaddr",
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 2) ==
            AI_MICRO_M_ACCESS_PMP_TOR_LOCKED &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 3) ==
            AI_MICRO_M_ACCESS_PMP_OFF_LOCKED &&
        CSRR(CSR_PMPADDR1) == locked_addr1 &&
        CSRR(CSR_PMPADDR2) == locked_addr2 &&
        CSRR(CSR_PMPADDR3) == locked_addr3 &&
        ai_micro_m_access_pmpcfg_entry(locked_cfg, 2) ==
            AI_MICRO_M_ACCESS_PMP_TOR_LOCKED &&
        ai_micro_m_access_pmpcfg_entry(locked_cfg, 3) ==
            AI_MICRO_M_ACCESS_PMP_OFF_LOCKED
    );

    TEST_SETUP_EXCEPT();
    uint64_t locked_csrrw_old =
        ai_micro_m_access_csrrw_pmpcfg0(0ULL);
    TEST_ASSERT("csrrw pmpcfg0 cannot modify locked entries and returns the packed locked value",
        excpt.triggered == false &&
        locked_csrrw_old == CSRR(CSR_PMPCFG0) &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 2) ==
            AI_MICRO_M_ACCESS_PMP_TOR_LOCKED &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 3) ==
            AI_MICRO_M_ACCESS_PMP_OFF_LOCKED
    );

    TEST_SETUP_EXCEPT();
    uint64_t locked_csrrs_old =
        ai_micro_m_access_csrrs_pmpcfg0(
            (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 16) |
            (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 24));
    TEST_ASSERT("csrrs pmpcfg0 cannot set bits inside locked PMP entries",
        excpt.triggered == false &&
        locked_csrrs_old == CSRR(CSR_PMPCFG0) &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 2) ==
            AI_MICRO_M_ACCESS_PMP_TOR_LOCKED &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 3) ==
            AI_MICRO_M_ACCESS_PMP_OFF_LOCKED
    );

    TEST_SETUP_EXCEPT();
    uint64_t locked_csrrc_old =
        ai_micro_m_access_csrrc_pmpcfg0(
            (AI_MICRO_M_ACCESS_PMP_TOR_LOCKED << 16) |
            (AI_MICRO_M_ACCESS_PMP_OFF_LOCKED << 24));
    TEST_ASSERT("csrrc pmpcfg0 cannot clear bits inside locked PMP entries",
        excpt.triggered == false &&
        locked_csrrc_old == CSRR(CSR_PMPCFG0) &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 2) ==
            AI_MICRO_M_ACCESS_PMP_TOR_LOCKED &&
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 3) ==
            AI_MICRO_M_ACCESS_PMP_OFF_LOCKED
    );

    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_M_ACCESS_PMP_TOR_RWX |
        (AI_MICRO_M_ACCESS_PMP_TOR_LOCKED << 16) |
        (AI_MICRO_M_ACCESS_PMP_OFF_LOCKED << 24));
    const uint64_t locked_cfg_after_csr_ops = CSRR(CSR_PMPCFG0);
    const uint64_t locked_addr2_after_csr_ops = CSRR(CSR_PMPADDR2);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_m_access_fixed_csrr_pmpcfg0();
    TEST_ASSERT("HS-mode read of pmpcfg0 reports IllegalInstruction and cannot observe or modify PMP configuration",
        ai_micro_m_access_check_illegal_with_inst(
            AI_MICRO_M_ACCESS_CSRR_INST(CSR_PMPCFG0))
    );

    TEST_SETUP_EXCEPT();
    ai_micro_m_access_fixed_csrw_pmpaddr0((base + 3 * PAGE_SIZE) >> 2);
    TEST_ASSERT("HS-mode write of pmpaddr0 reports IllegalInstruction before any PMP CSR side effect",
        ai_micro_m_access_check_illegal_with_inst(
            AI_MICRO_M_ACCESS_CSRW_INST(CSR_PMPADDR0))
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("illegal lower-privilege PMP CSR probes leave packed pmpcfg and locked address state unchanged",
        CSRR(CSR_PMPCFG0) == locked_cfg_after_csr_ops &&
        CSRR(CSR_PMPADDR2) == locked_addr2_after_csr_ops
    );

    TEST_END();
}

bool ai_micro_m_pmp_fetch_xonly_and_locked_deny_matrix_corner()
{
    TEST_START();

    const uintptr_t base = phys_page_base(AI_MICRO_M_ACCESS_PAGE);
    const uintptr_t target = base + 0x100U;
    const uintptr_t atomic_target = base + 0x180U;
    const uint32_t ret_inst = 0x00008067U;
    const uint64_t atomic_initial = 0x13579bdf2468ace0ULL;
    const uint64_t atomic_addend = 0x0102030405060708ULL;
    const uint64_t lrsc_update = 0x1122334455667788ULL;
    const uint64_t store_attempt = 0xfeedfacecafebeefULL;

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    write32(target, ret_inst);
    write64(atomic_target, atomic_initial);
    ai_micro_m_access_fence_i();

    TEST_SETUP_EXCEPT();
    uint64_t current_m_amo_old = amoadd_d(atomic_target, atomic_addend);
    TEST_ASSERT("with full PMP access, a current M-mode AMO is allowed and returns the old target value",
        excpt.triggered == false &&
        current_m_amo_old == atomic_initial &&
        read64(atomic_target) == atomic_initial + atomic_addend
    );

    TEST_SETUP_EXCEPT();
    uint64_t current_m_lr_old = 0ULL;
    uint64_t current_m_sc_status =
        ai_micro_m_access_lr_sc_d_pair(atomic_target, lrsc_update,
            &current_m_lr_old);
    TEST_ASSERT("with full PMP access, a current M-mode LR/SC pair is allowed and updates the target word",
        excpt.triggered == false &&
        current_m_lr_old == atomic_initial + atomic_addend &&
        current_m_sc_status == 0ULL &&
        read64(atomic_target) == lrsc_update
    );

    ai_micro_m_access_configure_napot_entry1(base,
        AI_MICRO_M_ACCESS_PMP_NAPOT_X);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(target);
    TEST_ASSERT("a PMP NAPOT X-only entry denies data load while leaving instruction fetch permission available",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target
    );

    TEST_SETUP_EXCEPT();
    sd(target, store_attempt);
    TEST_ASSERT("the same PMP NAPOT X-only entry denies data store to the executable region",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target
    );

    TEST_SETUP_EXCEPT();
    ai_micro_m_access_exec_ret_probe(target);
    TEST_ASSERT("the same PMP NAPOT X-only entry allows an HS instruction fetch from the target region",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied data store under the X-only PMP entry does not modify the fetched instruction word",
        read32(target) == ret_inst
    );

    ai_micro_m_access_configure_napot_entry1(base,
        AI_MICRO_M_ACCESS_PMP_NAPOT_NONE_LOCKED);
    const uint64_t locked_cfg = CSRR(CSR_PMPCFG0);
    const uint64_t locked_addr1 = CSRR(CSR_PMPADDR1);

    CSRW(CSR_PMPADDR1, ai_micro_m_access_make_napot_4k(base + PAGE_SIZE));
    CSRW(CSR_PMPCFG0,
        AI_MICRO_M_ACCESS_PMP_TOR_RWX |
        (AI_MICRO_M_ACCESS_PMP_NAPOT_RWX << 8));

    TEST_ASSERT("a locked PMP NAPOT entry ignores later pmpcfg and pmpaddr writes before access probes",
        ai_micro_m_access_pmpcfg_entry(CSRR(CSR_PMPCFG0), 1) ==
            AI_MICRO_M_ACCESS_PMP_NAPOT_NONE_LOCKED &&
        CSRR(CSR_PMPADDR1) == locked_addr1 &&
        ai_micro_m_access_pmpcfg_entry(locked_cfg, 1) ==
            AI_MICRO_M_ACCESS_PMP_NAPOT_NONE_LOCKED
    );

    TEST_SETUP_EXCEPT();
    ld(target);
    TEST_ASSERT("the locked PMP no-permission entry also applies to a current M-mode data load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target
    );

    TEST_SETUP_EXCEPT();
    uint64_t locked_amo_value = ai_micro_m_access_amoadd_d_rd_probe(target,
        0xbadc0de000000000ULL, 0x1010101010101010ULL);
    TEST_ASSERT("the locked PMP no-permission entry also applies to a current M-mode AMO as a store/AMO access fault and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target &&
        locked_amo_value == 0xbadc0de000000000ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t locked_lr_value = ai_micro_m_access_lr_d_rd_probe(target,
        0xbadc0de000000001ULL);
    TEST_ASSERT("the locked PMP no-permission entry also applies to a current M-mode LR and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target &&
        locked_lr_value == 0xbadc0de000000001ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t locked_sc_status = ai_micro_m_access_sc_d_status_probe(target,
        0xbadc0de000000002ULL, store_attempt);
    TEST_ASSERT("the locked PMP no-permission entry also applies to a current M-mode SC and does not report a false success",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target &&
        locked_sc_status == 0xbadc0de000000002ULL
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_set_mprv_mpp(AI_MICRO_M_ACCESS_MSTATUS_MPP_S);
    TEST_SETUP_EXCEPT();
    ld(target);
    TEST_ASSERT("the locked PMP no-permission entry also applies when current M uses effective S through MPRV",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_set_mprv_mpp(0ULL);
    TEST_SETUP_EXCEPT();
    ld(target);
    TEST_ASSERT("the locked PMP no-permission entry also applies when current M uses effective U through MPRV",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target
    );
    CSRC(CSR_MSTATUS, MSTATUS_MPRV |
        AI_MICRO_M_ACCESS_MSTATUS_MPP_MASK);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(target);
    TEST_ASSERT("the locked PMP no-permission entry rejects HS data load from the target page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target
    );

    TEST_SETUP_EXCEPT();
    sd(target, store_attempt);
    TEST_ASSERT("the locked PMP no-permission entry rejects HS data store from the target page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target
    );

    TEST_SETUP_EXCEPT();
    ai_micro_m_access_exec_ret_probe(target);
    TEST_ASSERT("the locked PMP no-permission entry rejects HS instruction fetch from the target page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == target
    );

    TEST_END();
}

bool ai_micro_m_load_pmp_allow_data_visible()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE);
    const uint64_t expected = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_m_access_restore_pmp_full_access();
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(paddr);
    TEST_ASSERT("an M-mode load with MPRV=0 and an allowed PMP window reads the backing data without surfacing an exception",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_m_load_unlocked_pmp_u_bypass_succeeds_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x120U;
    const uint64_t expected = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_m_access_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(paddr);
    TEST_ASSERT("an M-mode direct load still succeeds with MPRV=0 and MPP=U when the deny-read PMP entry is not locked, because the access executes with current M privilege",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_m_load_locked_pmp_u_access_fault()
{
    TEST_START();

    uintptr_t paddr = AI_MICRO_M_ACCESS_LOCKED_LOAD_U_PADDR;

    goto_priv(PRIV_M);
    write64(paddr, 0x2233445566778899ULL);
    /* Rebuild the deny window in low PMP slots so it is not shadowed by
     * reset_state()'s permissive full-range TOR entry 0.
     */
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("an M-mode load reports load access fault when a locked PMP entry denies reads and MPP is set to U",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == paddr
    );

    TEST_END();
}

bool ai_micro_m_load_locked_pmp_u_repeated_faults_stable_corner()
{
    TEST_START();

    uintptr_t paddr = AI_MICRO_M_ACCESS_LOCKED_LOAD_U_PADDR;

    goto_priv(PRIV_M);
    write64(paddr, 0x13579bdf2468ace0ULL);
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("a first M-mode load reports load access fault when the locked PMP entry denies reads and MPP is set to U",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == paddr
    );

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("repeating the same M-mode load with the locked deny entry still reports load access fault with the same raw address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == paddr
    );

    TEST_END();
}

bool ai_micro_m_load_locked_pmp_m_access_fault()
{
    TEST_START();

    uintptr_t paddr = AI_MICRO_M_ACCESS_LOCKED_LOAD_M_PADDR;

    goto_priv(PRIV_M);
    write64(paddr, 0x33445566778899aaULL);
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("an M-mode load still reports load access fault when a locked PMP entry denies reads and MPP is set to M",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == paddr
    );

    TEST_END();
}

bool ai_micro_m_load_locked_pmp_m_repeated_faults_stable_corner()
{
    TEST_START();

    uintptr_t paddr = AI_MICRO_M_ACCESS_LOCKED_LOAD_M_PADDR;

    goto_priv(PRIV_M);
    write64(paddr, 0x33445566778899aaULL);
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("a first M-mode load still reports load access fault when the locked PMP entry denies reads and MPP is set to M",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == paddr
    );

    TEST_SETUP_EXCEPT();
    ld(paddr);
    TEST_ASSERT("repeating the same M-mode load with MPP=M and the locked deny entry still reports load access fault with the same raw address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == paddr
    );

    TEST_END();
}

bool ai_micro_m_load_mprv_u_pmp_read_access_fault()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE);
    const uint64_t expected = 0x445566778899aabbULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an M-mode load with MPRV=1 and MPP=U reports load access fault when PMP denies the translated read",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP access, the same translated address loads normally again and returns the original payload",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_END();
}

bool ai_micro_m_load_mprv_u_same_page_fault_then_restore_succeeds_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE) + 0xc0U;
    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0xc0U;
    const uint64_t expected = 0x3141592653589793ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R);
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an M-mode translated load first reports load access fault when PMP denies reads to the selected translated page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP access and re-arming MPRV=1/MPP=U, retrying the same translated load succeeds",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    TEST_END();
}

bool ai_micro_m_load_mprv_u_same_page_repeated_faults_then_restore_succeeds_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x160U;
    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x160U;
    const uint64_t expected = 0x2718281828459045ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R);
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a first M-mode translated load reports load access fault when PMP denies reads to the selected translated page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    ai_micro_m_access_enable_mprv_u_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("repeating the same M-mode translated load with the deny window still active reports the same load access fault with the same translated address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP access and re-arming MPRV=1/MPP=U, retrying the same translated load succeeds after the repeated faulting probes",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    TEST_END();
}

bool ai_micro_m_load_mprv_u_pmp_fault_then_allowed_translated_load_succeeds_corner()
{
    TEST_START();

    uintptr_t denied_vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x40U;
    uintptr_t denied_paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x40U;
    uintptr_t allowed_vaddr = hs_page_base(AI_MICRO_M_ACCESS_ALT_PAGE) + 0x40U;
    uintptr_t allowed_paddr = phys_page_base(AI_MICRO_M_ACCESS_ALT_PAGE) + 0x40U;
    const uint64_t denied_initial = 0x2233445566778899ULL;
    const uint64_t allowed_expected = 0x99aabbccddeeff00ULL;

    goto_priv(PRIV_M);
    write64(denied_paddr, denied_initial);
    write64(allowed_paddr, allowed_expected);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        denied_paddr & PAGE_ADDR_MSK,
        (denied_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R);
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    ld(denied_vaddr);
    TEST_ASSERT("an M-mode translated load first reports load access fault when PMP denies the selected translated page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == denied_vaddr
    );

    ai_micro_m_access_enable_mprv_u_translation();
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(allowed_vaddr);
    TEST_ASSERT("a later M-mode translated load to a different allowed page succeeds after re-arming MPRV=1/MPP=U",
        excpt.triggered == false &&
        observed == allowed_expected
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    TEST_END();
}

bool ai_micro_m_load_mprv_u_repeated_faults_then_allowed_translated_load_succeeds_corner()
{
    TEST_START();

    uintptr_t denied_vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x140U;
    uintptr_t denied_paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x140U;
    uintptr_t allowed_vaddr = hs_page_base(AI_MICRO_M_ACCESS_ALT_PAGE) + 0x140U;
    uintptr_t allowed_paddr = phys_page_base(AI_MICRO_M_ACCESS_ALT_PAGE) + 0x140U;
    const uint64_t denied_initial = 0x2233445566778899ULL;
    const uint64_t allowed_expected = 0x99aabbccddeeff00ULL;

    goto_priv(PRIV_M);
    write64(denied_paddr, denied_initial);
    write64(allowed_paddr, allowed_expected);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        denied_paddr & PAGE_ADDR_MSK,
        (denied_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_R);
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    ld(denied_vaddr);
    TEST_ASSERT("a first M-mode translated load reports load access fault when PMP denies the selected translated page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == denied_vaddr
    );

    ai_micro_m_access_enable_mprv_u_translation();
    TEST_SETUP_EXCEPT();
    ld(denied_vaddr);
    TEST_ASSERT("repeating the same M-mode translated load to the denied page still reports load access fault with the same translated address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == denied_vaddr
    );

    ai_micro_m_access_enable_mprv_u_translation();
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(allowed_vaddr);
    TEST_ASSERT("a later M-mode translated load to a different allowed page succeeds after re-arming MPRV=1/MPP=U following the repeated denied-page probes",
        excpt.triggered == false &&
        observed == allowed_expected
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    TEST_END();
}

bool ai_micro_m_store_pmp_allow_data_visible()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE);
    const uint64_t initial = 0x5566778899aabbccULL;
    const uint64_t stored = 0xaabbccddeeff0011ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_m_access_restore_pmp_full_access();
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    sd(paddr, stored);
    TEST_ASSERT("an M-mode store with MPRV=0 and an allowed PMP window retires without surfacing an exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same backing location reflects the stored value after the allowed M-mode store path",
        read64(paddr) == stored
    );

    TEST_END();
}

bool ai_micro_m_store_unlocked_pmp_u_bypass_succeeds_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x1a0U;
    const uint64_t initial = 0x0102030405060708ULL;
    const uint64_t stored = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_m_access_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    sd(paddr, stored);
    TEST_ASSERT("an M-mode direct store still succeeds with MPRV=0 and MPP=U when the deny-write PMP entry is not locked, because the access executes with current M privilege",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the direct store commits its payload even though the unlocked PMP entry would deny writes to lower privilege accesses",
        read64(paddr) == stored
    );
    ai_micro_m_access_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_m_store_locked_pmp_u_access_fault()
{
    TEST_START();

    uintptr_t paddr = AI_MICRO_M_ACCESS_LOCKED_STORE_U_PADDR;
    const uint64_t stored = 0xbbccddee11223344ULL;

    goto_priv(PRIV_M);
    write64(paddr, 0x66778899aabbccddULL);
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    sd(paddr, stored);
    TEST_ASSERT("an M-mode store reports store access fault when a locked PMP entry denies writes and MPP is set to U",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == paddr
    );

    TEST_END();
}

bool ai_micro_m_store_locked_pmp_u_repeated_faults_leave_data_unchanged_corner()
{
    TEST_START();

    uintptr_t paddr = AI_MICRO_M_ACCESS_LOCKED_STORE_U_PADDR;
    const uint64_t initial = 0x66778899aabbccddULL;
    const uint64_t first_stored = 0xbbccddee11223344ULL;
    const uint64_t second_stored = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    sd(paddr, first_stored);
    TEST_ASSERT("a first M-mode store reports store access fault when the locked PMP entry denies writes and MPP is set to U",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == paddr
    );

    TEST_SETUP_EXCEPT();
    sd(paddr, second_stored);
    TEST_ASSERT("repeating the same M-mode store with the locked deny entry still reports store access fault with the same raw address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == paddr
    );

    TEST_ASSERT("repeated locked-PMP store faults leave the backing memory unchanged",
        read64(paddr) == initial
    );

    TEST_END();
}

bool ai_micro_m_store_locked_pmp_m_access_fault()
{
    TEST_START();

    uintptr_t paddr = AI_MICRO_M_ACCESS_LOCKED_STORE_M_PADDR;
    const uint64_t stored = 0xccddeeff22334455ULL;

    goto_priv(PRIV_M);
    write64(paddr, 0x778899aabbccddeeULL);
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    sd(paddr, stored);
    TEST_ASSERT("an M-mode store still reports store access fault when a locked PMP entry denies writes and MPP is set to M",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == paddr
    );

    TEST_END();
}

bool ai_micro_m_store_locked_pmp_m_repeated_faults_leave_data_unchanged_corner()
{
    TEST_START();

    uintptr_t paddr = AI_MICRO_M_ACCESS_LOCKED_STORE_M_PADDR;
    const uint64_t initial = 0x778899aabbccddeeULL;
    const uint64_t first_stored = 0xccddeeff22334455ULL;
    const uint64_t second_stored = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W_LOCKED);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, 3ULL << 11);

    TEST_SETUP_EXCEPT();
    sd(paddr, first_stored);
    TEST_ASSERT("a first M-mode store still reports store access fault when the locked PMP entry denies writes and MPP is set to M",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == paddr
    );

    TEST_SETUP_EXCEPT();
    sd(paddr, second_stored);
    TEST_ASSERT("repeating the same M-mode store with MPP=M and the locked deny entry still reports store access fault with the same raw address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == paddr
    );

    TEST_ASSERT("repeated locked-PMP store faults with MPP=M leave the backing memory unchanged",
        read64(paddr) == initial
    );

    TEST_END();
}

bool ai_micro_m_store_mprv_u_pmp_write_access_fault()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE);
    const uint64_t initial = 0x8899aabbccddeeffULL;
    const uint64_t stored = 0xddeeff3344556677ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("an M-mode store with MPRV=1 and MPP=U reports store access fault when PMP denies the translated write",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after restoring PMP access, the translated location still contains the original value because the denied M-mode store never committed",
        excpt.triggered == false &&
        observed == initial
    );

    TEST_END();
}

bool ai_micro_m_store_mprv_u_same_page_fault_then_restore_succeeds_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x180U;
    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x180U;
    const uint64_t initial = 0x76543210fedcba98ULL;
    const uint64_t stored = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W);
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("an M-mode translated store first reports store access fault when PMP denies writes to the selected translated page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied translated store leaves the backing memory unchanged before PMP permissions are restored",
        read64(paddr) == initial
    );
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after restoring PMP access and re-arming MPRV=1/MPP=U, retrying the same translated store succeeds",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the retried translated store commits to the original backing location once PMP permissions are restored",
        read64(paddr) == stored
    );
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    TEST_END();
}

bool ai_micro_m_store_mprv_u_same_page_repeated_faults_then_restore_succeeds_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x1c0U;
    uintptr_t paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x1c0U;
    const uint64_t initial = 0x3141592653589793ULL;
    const uint64_t stored = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W);
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("a first M-mode translated store reports store access fault when PMP denies writes to the selected translated page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    ai_micro_m_access_enable_mprv_u_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("repeating the same M-mode translated store with the deny window still active reports the same store access fault with the same translated address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated denied translated stores leave the backing memory unchanged before PMP permissions are restored",
        read64(paddr) == initial
    );
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after restoring PMP access and re-arming MPRV=1/MPP=U, retrying the same translated store succeeds after the repeated faulting probes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the retried translated store commits to the original backing location once PMP permissions are restored after the repeated faulting probes",
        read64(paddr) == stored
    );
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    TEST_END();
}

bool ai_micro_m_store_mprv_u_pmp_fault_then_allowed_translated_store_succeeds_corner()
{
    TEST_START();

    uintptr_t denied_vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x80U;
    uintptr_t denied_paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x80U;
    uintptr_t allowed_vaddr = hs_page_base(AI_MICRO_M_ACCESS_ALT_PAGE) + 0x80U;
    uintptr_t allowed_paddr = phys_page_base(AI_MICRO_M_ACCESS_ALT_PAGE) + 0x80U;
    const uint64_t denied_initial = 0x8899aabbccddeeffULL;
    const uint64_t allowed_initial = 0x1021324354657687ULL;
    const uint64_t denied_stored = 0xddeeff3344556677ULL;
    const uint64_t allowed_stored = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(denied_paddr, denied_initial);
    write64(allowed_paddr, allowed_initial);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        denied_paddr & PAGE_ADDR_MSK,
        (denied_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W);
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    sd(denied_vaddr, denied_stored);
    TEST_ASSERT("an M-mode translated store first reports store access fault when PMP denies the selected translated page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == denied_vaddr
    );

    ai_micro_m_access_enable_mprv_u_translation();
    TEST_SETUP_EXCEPT();
    sd(allowed_vaddr, allowed_stored);
    TEST_ASSERT("a later M-mode translated store to a different allowed page succeeds after re-arming MPRV=1/MPP=U",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied translated store leaves its backing memory unchanged while the later allowed translated store commits successfully",
        read64(denied_paddr) == denied_initial &&
        read64(allowed_paddr) == allowed_stored
    );
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    TEST_END();
}

bool ai_micro_m_store_mprv_u_repeated_faults_then_allowed_translated_store_succeeds_corner()
{
    TEST_START();

    uintptr_t denied_vaddr = hs_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x180U;
    uintptr_t denied_paddr = phys_page_base(AI_MICRO_M_ACCESS_PAGE) + 0x180U;
    uintptr_t allowed_vaddr = hs_page_base(AI_MICRO_M_ACCESS_ALT_PAGE) + 0x180U;
    uintptr_t allowed_paddr = phys_page_base(AI_MICRO_M_ACCESS_ALT_PAGE) + 0x180U;
    const uint64_t denied_initial = 0x8899aabbccddeeffULL;
    const uint64_t allowed_initial = 0x1021324354657687ULL;
    const uint64_t denied_stored = 0xddeeff3344556677ULL;
    const uint64_t allowed_stored = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(denied_paddr, denied_initial);
    write64(allowed_paddr, allowed_initial);
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_prepare_user_translation();

    goto_priv(PRIV_M);
    ai_micro_m_access_configure_pmp_window(
        denied_paddr & PAGE_ADDR_MSK,
        (denied_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_M_ACCESS_PMP_TOR_NO_W);
    ai_micro_m_access_enable_mprv_u_translation();

    TEST_SETUP_EXCEPT();
    sd(denied_vaddr, denied_stored);
    TEST_ASSERT("a first M-mode translated store reports store access fault when PMP denies the selected translated page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == denied_vaddr
    );

    ai_micro_m_access_enable_mprv_u_translation();
    TEST_SETUP_EXCEPT();
    sd(denied_vaddr, denied_stored);
    TEST_ASSERT("repeating the same M-mode translated store to the denied page still reports store access fault with the same translated address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == denied_vaddr
    );

    ai_micro_m_access_enable_mprv_u_translation();
    TEST_SETUP_EXCEPT();
    sd(allowed_vaddr, allowed_stored);
    TEST_ASSERT("a later M-mode translated store to a different allowed page succeeds after re-arming MPRV=1/MPP=U following the repeated denied-page probes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated denied translated stores leave their backing memory unchanged while the later allowed translated store commits successfully",
        read64(denied_paddr) == denied_initial &&
        read64(allowed_paddr) == allowed_stored
    );
    ai_micro_m_access_restore_pmp_full_access();
    ai_micro_m_access_clear_machine_access_mode();

    TEST_END();
}
