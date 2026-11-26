#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ARCH_AMO_PMA_ALIAS_PAGE   VSRWX_GRWX
#define AI_ARCH_AMO_PMA_BACKING_PAGE SCRATCHPAD
#define AI_ARCH_AMO_MPRV_PAGE        VSRWX_GRWX
#define AI_ARCH_AMO_GUEST_FAULT_PAGE VSRW_GI
#define AI_ARCH_AMO_GUEST_GOOD_PAGE  SWITCH1
#define AI_ARCH_AMO_GUEST_NONLEAF_PT_PAGE SWITCH2
#define AI_ARCH_AMO_PBMT_IO_MEM_BASE    0x30000000000ULL
#define AI_ARCH_AMO_PBMT_IO_MEM_LIMIT   0x40000000000ULL
#define AI_ARCH_AMO_PMA_DEVICE_MEM_BASE 0x30000000000ULL

#define AI_ARCH_AMO_PMP_TOR_RWX  0x0fULL
#define AI_ARCH_AMO_PMP_TOR_NO_W 0x0dULL

#define AI_ARCH_AMO_GUEST_FAULT_OFFSET       0x180U
#define AI_ARCH_AMO_GUEST_FAULT_OFFSET_NEXT  0x2a0U
#define AI_ARCH_AMO_GUEST_GOOD_OFFSET        0x190U
#define AI_ARCH_AMO_TINST_VS_NONLEAF_PTE     0x3000ULL
#define AI_ARCH_AMO_PERMISSION_FAULT_LEAF    (PTE_V | PTE_A | PTE_D | PTE_R)
#define AI_ARCH_AMO_PERMISSION_NO_R_LEAF     (PTE_V | PTE_A | PTE_D | PTE_W)
#define AI_ARCH_AMO_PERMISSION_LEGAL_LEAF    (PTE_V | PTE_A | PTE_D | PTE_RWX)
#define AI_ARCH_AMO_PERMISSION_USER_LEAF     (PTE_V | PTE_A | PTE_D | PTE_RWX | PTE_U)

typedef struct {
    uintptr_t pc;
    uintptr_t next_pc;
} ai_arch_amo_probe_pc_t;

static inline uintptr_t ai_arch_amo_fault_backing_paddr(void)
{
    return phys_page_base(X);
}

static inline uintptr_t ai_arch_amo_guest_fault_vaddr(void)
{
    return vs_page_base(AI_ARCH_AMO_GUEST_FAULT_PAGE) +
        AI_ARCH_AMO_GUEST_FAULT_OFFSET;
}

static inline uintptr_t ai_arch_amo_guest_fault_vaddr_next(void)
{
    return vs_page_base(AI_ARCH_AMO_GUEST_FAULT_PAGE) +
        AI_ARCH_AMO_GUEST_FAULT_OFFSET_NEXT;
}

static inline uintptr_t ai_arch_amo_guest_good_vaddr(void)
{
    return vs_page_base(AI_ARCH_AMO_GUEST_GOOD_PAGE) +
        AI_ARCH_AMO_GUEST_GOOD_OFFSET;
}

static inline uintptr_t ai_arch_amo_guest_good_paddr(void)
{
    return phys_page_base(AI_ARCH_AMO_GUEST_GOOD_PAGE) +
        AI_ARCH_AMO_GUEST_GOOD_OFFSET;
}

static inline uintptr_t ai_arch_amo_make_noncanonical_vaddr(
    uintptr_t canonical_vaddr)
{
#ifdef sv48
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit47 = (canonical_vaddr >> 47) & 1ULL;

    if (bit47) {
        bad_vaddr &= ~(1ULL << 48);
    } else {
        bad_vaddr |= (1ULL << 48);
    }
#else
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit38 = (canonical_vaddr >> 38) & 1ULL;

    if (bit38) {
        bad_vaddr &= ~(1ULL << 39);
    } else {
        bad_vaddr |= (1ULL << 39);
    }
#endif

    return bad_vaddr;
}

/*
 * In the default VS/G-stage test-page layout, VS test pages resolve to guest
 * physical pages with the same TEST_VPAGE_BASE page index and identical
 * intra-page offset, so a guest-page-faulting AMO should report GPA=vaddr here.
 * The architectural htval/mtval2 CSR stores GPA >> 2.
 */
static inline uint64_t ai_arch_amo_guest_fault_expected_tval2(uintptr_t vaddr)
{
    return ((uint64_t)vaddr) >> 2;
}

static inline uintptr_t ai_arch_amo_fault_io_backing_paddr(void)
{
    return phys_page_base_offset(X, AI_ARCH_AMO_PBMT_IO_MEM_BASE);
}

static inline uintptr_t ai_arch_amo_fault_device_backing_paddr(enum test_page page)
{
    return phys_page_base_offset(page, AI_ARCH_AMO_PMA_DEVICE_MEM_BASE);
}

static inline pte_t *ai_arch_amo_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uint64_t ai_arch_amo_hs_leaf_value(enum test_page page,
    uint64_t flags)
{
    return (phys_page_base(page) >> 2) | flags;
}

static inline void ai_arch_amo_set_hs_leaf(enum test_page page, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_arch_amo_hs_leaf_entry(page) =
        ai_arch_amo_hs_leaf_value(page, flags);
    goto_priv(PRIV_HS);
    sfence_vma();
}

static inline bool ai_arch_amo_hs_leaf_matches(enum test_page page,
    uint64_t flags)
{
    return *ai_arch_amo_hs_leaf_entry(page) ==
        ai_arch_amo_hs_leaf_value(page, flags);
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoswap_w_probe_rd(uintptr_t addr, uint32_t value, uint64_t sentinel,
    ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoswap.w %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(operand), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoadd_w_probe_rd(uintptr_t addr, uint32_t value, uint64_t sentinel,
    ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoadd.w %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(operand), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoand_w_probe_rd(uintptr_t addr, uint32_t value, uint64_t sentinel,
    ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoand.w %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(operand), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoand_d_probe_rd(uintptr_t addr, uint64_t value, uint64_t sentinel,
    ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoand.d %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoxor_d_probe_rd(uintptr_t addr, uint64_t value, uint64_t sentinel,
    ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoxor.d %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amominu_d_probe_rd(uintptr_t addr, uint64_t value, uint64_t sentinel,
    ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amominu.d %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amomaxu_d_probe_rd(uintptr_t addr, uint64_t value, uint64_t sentinel,
    ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amomaxu.d %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoor_d_probe_rd(uintptr_t addr, uint64_t value, uint64_t sentinel,
    ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoor.d %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoadd_d_probe_rd(uintptr_t addr, uint64_t value, uint64_t sentinel,
    ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoadd.d %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoadd_d_aq_probe_rd(uintptr_t addr, uint64_t value,
    uint64_t sentinel, ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoadd.d.aq %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoadd_d_rl_probe_rd(uintptr_t addr, uint64_t value,
    uint64_t sentinel, ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoadd.d.rl %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_amoadd_d_aqrl_probe_rd(uintptr_t addr, uint64_t value,
    uint64_t sentinel, ai_arch_amo_probe_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "amoadd.d.aqrl %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline void ai_arch_amo_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_ARCH_AMO_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_arch_amo_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_ARCH_AMO_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_ARCH_AMO_PMP_TOR_RWX << 16));
}

static inline void ai_arch_amo_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
    hspt_init();
    sfence_vma();
}

static inline void ai_arch_amo_prepare_mprv_user_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();
}

static inline void ai_arch_amo_enable_mprv_user_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
}

static inline void ai_arch_reset_amo_fault_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
}

static inline uintptr_t ai_arch_amo_fault_anchor(enum test_page page, uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_arch_prepare_pbmt_amo_alias(enum test_page page)
{
    goto_priv(PRIV_M);
    ai_arch_reset_amo_fault_env();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline uintptr_t ai_arch_prepare_pbmt_amo_io_alias(enum test_page page)
{
    goto_priv(PRIV_M);
    ai_arch_reset_amo_fault_env();
    CSRW(CSR_PMAADDR0, AI_ARCH_AMO_PBMT_IO_MEM_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_ARCH_AMO_PBMT_IO_MEM_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    fence_iorw_iorw();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x_base_paddr(page, AI_ARCH_AMO_PBMT_IO_MEM_BASE);
    sfence_vma();
    return hs_page_base(page);
}

static inline uintptr_t ai_arch_prepare_pma_device_amo_alias(enum test_page alias_page,
    enum test_page backing_page)
{
    uintptr_t backing_paddr = ai_arch_amo_fault_device_backing_paddr(backing_page);

    goto_priv(PRIV_M);
    ai_arch_reset_amo_fault_env();

    CSRW(CSR_PMAADDR0, backing_paddr >> 2);
    CSRW(CSR_PMAADDR1, (backing_paddr + PAGE_SIZE) >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);

    hspt_init();
    hspt_leaf_change_base_paddr(alias_page,
        ai_arch_amo_fault_anchor(alias_page, backing_paddr));
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline uintptr_t ai_arch_prepare_cacheable_amo_alias(enum test_page alias_page,
    uintptr_t backing_paddr)
{
    hspt_leaf_change_base_paddr(alias_page,
        ai_arch_amo_fault_anchor(alias_page, backing_paddr));
    return hs_page_base(alias_page);
}

static inline void ai_arch_amo_prepare_vs_guest_env(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    hpt_init();
    vspt_init();
    sfence_vma();
    hfence();
}

static inline void ai_arch_amo_cleanup_vs_guest_env(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_HGATP, 0ULL);
    CSRW(CSR_VSATP, 0ULL);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();
    hfence();
}

static inline void ai_arch_amo_arm_vs_nonleaf_pt_guest_fault(
    enum test_page nonleaf_pt_page)
{
    goto_priv(PRIV_M);
    /*
     * Force the VS-stage walk for TEST_VPAGE_BASE pages to descend through a
     * guest page that is translated by G-stage as a non-leaf entry. The
     * architectural guest-page fault is then attributed to an implicit VS
     * page-table access and should report tinst/htinst = 0x3000.
     */
    vspt[2][0] = PTE_V | (phys_page_base(nonleaf_pt_page) >> 2);
    hpt[5][nonleaf_pt_page] = PTE_V | (((uintptr_t)&hpt[6][0]) >> 2);
    sfence_vma();
    hfence();
}

static inline uint64_t ai_arch_amo_vs_nonleaf_pt_expected_tval2(
    enum test_page nonleaf_pt_page, uintptr_t fault_vaddr)
{
    uintptr_t implicit_ptw_gpa = phys_page_base(nonleaf_pt_page) +
        ((((uintptr_t)fault_vaddr) >> PAGE_SHIFT) & 0x1ffULL) *
            sizeof(pte_t);
    return ((uint64_t)implicit_ptw_gpa) >> 2;
}

bool ai_arch_amo_io_nc_region_fault()
{
    TEST_START();

    uintptr_t backing_paddr = ai_arch_amo_fault_backing_paddr();
    uintptr_t io_backing_paddr = ai_arch_amo_fault_io_backing_paddr();
    uintptr_t nc_vaddr = ai_arch_prepare_pbmt_amo_alias(VSRWXPbmt1_GURWXPbmt1);

    write64(backing_paddr, 0x1122334455667788ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_w(nc_vaddr, 0x13579bdfU);
    TEST_ASSERT("an aligned AMO on a PBMT=NC alias reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PBMT=NC AMO fault leaves backing memory unchanged",
        read64(backing_paddr) == 0x1122334455667788ULL
    );

    uintptr_t io_vaddr = ai_arch_prepare_pbmt_amo_io_alias(VSRWXPbmt2_GURWXPbmt1);

    write64(io_backing_paddr, 0x8877665544332211ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_w(io_vaddr, 0x2468ace0U);
    TEST_ASSERT("an aligned AMO on a PBMT=IO alias reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PBMT=IO AMO fault also leaves backing memory unchanged",
        read64(io_backing_paddr) == 0x8877665544332211ULL
    );
    ai_arch_reset_amo_fault_env();

    TEST_END();
}

bool ai_arch_amo_pbmt_nc_repeated_fault_then_scalar_load_store_recovers_corner()
{
    TEST_START();

    const uintptr_t vaddr = ai_arch_prepare_pbmt_amo_alias(VSRWXPbmt1_GURWXPbmt1);
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t updated_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent_value = 0x0fedcba987654321ULL;

    /*
     * Seed and check this PBMT=NC recovery probe through the same PBMT=NC
     * alias.  A direct M-mode write/read is a cacheable view on this profile
     * and is not a reliable oracle for later NC alias observations.
     */
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, initial_value);
    TEST_ASSERT("PBMT=NC AMO recovery setup seeds the target doubleword through the same NC alias that the scalar recovery path uses",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(adjacent_vaddr, adjacent_value);
    TEST_ASSERT("PBMT=NC AMO recovery setup seeds the adjacent doubleword through the same NC alias before the guard readback",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr, 0x13579bdfU);
    TEST_ASSERT("the first aligned AMO on a PBMT=NC alias reports store access fault before the scalar recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr, 0x2468ace0U);
    TEST_ASSERT("repeating the same aligned AMO on a PBMT=NC alias still reports store access fault before the scalar recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(vaddr);
    TEST_ASSERT("after the repeated PBMT=NC AMO faults, a normal scalar load through the same alias still returns the original payload",
        excpt.triggered == false &&
        observed_before == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, updated_value);
    TEST_ASSERT("after the repeated PBMT=NC AMO faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(vaddr);
    TEST_ASSERT("after the repeated PBMT=NC AMO faults and the scalar recovery store, a later scalar load through the same alias observes the updated payload immediately",
        excpt.triggered == false &&
        observed_after == updated_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=NC AMO faults and the later scalar recovery store leave the adjacent shared doubleword unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_value
    );
    ai_arch_reset_amo_fault_env();

    TEST_END();
}

bool ai_arch_amo_pbmt_io_repeated_fault_then_scalar_load_store_recovers_corner()
{
    TEST_START();

    const uintptr_t backing_paddr = ai_arch_amo_fault_io_backing_paddr();
    const uintptr_t vaddr = ai_arch_prepare_pbmt_amo_io_alias(VSRWXPbmt2_GURWXPbmt1);
    const uintptr_t adjacent_paddr = backing_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t updated_value = 0x76543210fedcba98ULL;
    const uint64_t adjacent_value = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(backing_paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr, 0x13579bdfU);
    TEST_ASSERT("the first aligned AMO on a PBMT=IO alias reports store access fault before the scalar recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr, 0x2468ace0U);
    TEST_ASSERT("repeating the same aligned AMO on a PBMT=IO alias still reports store access fault before the scalar recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(vaddr);
    TEST_ASSERT("after the repeated PBMT=IO AMO faults, a normal scalar load through the same alias still returns the original payload",
        excpt.triggered == false &&
        observed_before == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, updated_value);
    TEST_ASSERT("after the repeated PBMT=IO AMO faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(vaddr);
    TEST_ASSERT("after the repeated PBMT=IO AMO faults and the scalar recovery store, a later scalar load through the same alias observes the updated payload immediately",
        excpt.triggered == false &&
        observed_after == updated_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=IO AMO faults and the later scalar recovery store leave the adjacent shared doubleword unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=IO AMO faults do not poison the later normal scalar path, so backing memory ends with only the target doubleword updated",
        read64(backing_paddr) == updated_value &&
        read64(adjacent_paddr) == adjacent_value
    );
    ai_arch_reset_amo_fault_env();

    TEST_END();
}

bool ai_arch_amo_pbmt_nc_fault_then_cacheable_alias_amo_recovers_same_pa_corner()
{
    TEST_START();

    const uintptr_t backing_paddr = ai_arch_amo_fault_backing_paddr();
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t adjacent_value = 0x0fedcba987654321ULL;
    const uint64_t amo_mask = 0x00ff0000ff0000ffULL;
    const uint64_t updated_value = initial_value | amo_mask;
    const uintptr_t adjacent_paddr = backing_paddr + sizeof(uint64_t);
    const uintptr_t word_paddr = backing_paddr + 2U * sizeof(uint64_t);
    const uintptr_t word_guard_paddr = backing_paddr + 3U * sizeof(uint64_t);
    const uint64_t word_initial = 0x5566778800000011ULL;
    const uint64_t word_updated = 0x5566778800000033ULL;
    const uint64_t word_guard = 0x5a5aa5a55a5aa5a5ULL;
    const uint32_t word_addend = 0x22U;

    goto_priv(PRIV_M);
    ai_arch_reset_amo_fault_env();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(VSRWXPbmt1_GURWXPbmt1);
    const uintptr_t cacheable_vaddr = ai_arch_prepare_cacheable_amo_alias(
        VSRWX_GRWX, backing_paddr);
    const uintptr_t fault_vaddr = hs_page_base(VSRWXPbmt1_GURWXPbmt1);
    const uintptr_t adjacent_cacheable_vaddr = cacheable_vaddr + sizeof(uint64_t);
    const uintptr_t word_cacheable_vaddr = cacheable_vaddr +
        2U * sizeof(uint64_t);
    const uintptr_t word_guard_cacheable_vaddr = cacheable_vaddr +
        3U * sizeof(uint64_t);
    sfence_vma();

    write64(backing_paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);
    write64(word_paddr, word_initial);
    write64(word_guard_paddr, word_guard);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoor_d(fault_vaddr, amo_mask);
    TEST_ASSERT("the first aligned AMO on the PBMT=NC fault alias reports store access fault with tval at the fault alias before the same-backing-page cacheable-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoor_d(fault_vaddr, amo_mask);
    TEST_ASSERT("repeating the same aligned AMO on the PBMT=NC fault alias still reports store access fault with stable tval before the same-backing-page cacheable-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(cacheable_vaddr);
    TEST_ASSERT("after the repeated PBMT=NC AMO faults, a cacheable alias to the same backing page still reads the original payload",
        excpt.triggered == false &&
        observed_before == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoor_d(cacheable_vaddr, amo_mask);
    TEST_ASSERT("after the repeated PBMT=NC AMO faults, an aligned cacheable AMO through a second alias to the same backing page still succeeds and returns the original value",
        excpt.triggered == false &&
        amo_old == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(cacheable_vaddr);
    TEST_ASSERT("after the same-backing-page cacheable-alias AMO recovery, the cacheable alias immediately observes the AMO-updated payload",
        excpt.triggered == false &&
        observed_after == updated_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t word_old = amoadd_w(word_cacheable_vaddr, word_addend);
    TEST_ASSERT("after the repeated PBMT=NC AMO faults, a cacheable AMOADD.W on a separate word in the same backing page succeeds and returns the old word",
        excpt.triggered == false &&
        word_old == (uint32_t)word_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t word_observed = ld(word_cacheable_vaddr);
    TEST_ASSERT("the cacheable AMOADD.W recovery updates only the target word lane after the PBMT=NC AMO faults",
        excpt.triggered == false &&
        word_observed == word_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t word_guard_observed = ld(word_guard_cacheable_vaddr);
    TEST_ASSERT("the PBMT=NC fault probes plus cacheable W/D AMO recoveries preserve the extra word-lane guard doubleword",
        excpt.triggered == false &&
        word_guard_observed == word_guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_cacheable_vaddr);
    TEST_ASSERT("the repeated PBMT=NC AMO faults and the later same-backing-page cacheable-alias AMO recovery leave the adjacent doubleword unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the PBMT=NC fault-alias attempts and the later cacheable-alias AMO recovery, backing memory also reflects exactly the AMO-updated target while preserving the adjacent doubleword",
        read64(backing_paddr) == updated_value &&
        read64(adjacent_paddr) == adjacent_value &&
        read64(word_paddr) == word_updated &&
        read64(word_guard_paddr) == word_guard
    );

    ai_arch_reset_amo_fault_env();

    TEST_END();
}

bool ai_arch_amo_pbmt_io_fault_then_cacheable_alias_amo_recovers_same_pa_corner()
{
    TEST_START();

    const uintptr_t backing_paddr = ai_arch_amo_fault_io_backing_paddr();
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t adjacent_value = 0x13579bdf2468ace0ULL;
    const uint64_t amo_mask = 0x0f0f00000000f0f0ULL;
    const uint64_t updated_value = initial_value | amo_mask;
    const uint64_t word_initial = 0x6677889900000021ULL;
    const uint64_t word_updated = 0x6677889900000043ULL;
    const uint64_t word_guard = 0xa55a5aa5a55a5aa5ULL;
    const uint32_t word_addend = 0x22U;

    goto_priv(PRIV_M);
    ai_arch_reset_amo_fault_env();
    CSRW(CSR_PMAADDR0, AI_ARCH_AMO_PBMT_IO_MEM_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_ARCH_AMO_PBMT_IO_MEM_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRS(CSR_PMACFG0, 1ULL << 13);
    CSRS(CSR_PMACFG0, 1ULL << 14);
    fence_iorw_iorw();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt2_GURWXPbmt1, AI_ARCH_AMO_PBMT_IO_MEM_BASE);
    const uintptr_t cacheable_vaddr = ai_arch_prepare_cacheable_amo_alias(
        VSRWX_GRWX, backing_paddr);
    const uintptr_t fault_vaddr = hs_page_base(VSRWXPbmt2_GURWXPbmt1);
    const uintptr_t adjacent_cacheable_vaddr = cacheable_vaddr + sizeof(uint64_t);
    const uintptr_t word_cacheable_vaddr = cacheable_vaddr +
        2U * sizeof(uint64_t);
    const uintptr_t word_guard_cacheable_vaddr = cacheable_vaddr +
        3U * sizeof(uint64_t);
    const uintptr_t word_paddr = backing_paddr + 2U * sizeof(uint64_t);
    const uintptr_t word_guard_paddr = backing_paddr + 3U *
        sizeof(uint64_t);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(cacheable_vaddr, initial_value);
    TEST_ASSERT("the PBMT=IO same-PA recovery target is seeded through the cacheable alias so the later AMO recovery path has a cacheable oracle",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(adjacent_cacheable_vaddr, adjacent_value);
    TEST_ASSERT("the PBMT=IO same-PA recovery adjacent guard is seeded through the same cacheable alias before the fault probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(word_cacheable_vaddr, word_initial);
    TEST_ASSERT("the PBMT=IO same-PA recovery word-lane target is seeded through the cacheable alias before the fault probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(word_guard_cacheable_vaddr, word_guard);
    TEST_ASSERT("the PBMT=IO same-PA recovery word-lane guard is seeded through the cacheable alias before the fault probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoor_d(fault_vaddr, amo_mask);
    TEST_ASSERT("the first aligned AMO on the PBMT=IO fault alias reports store access fault with tval at the fault alias before the same-backing-page cacheable-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoor_d(fault_vaddr, amo_mask);
    TEST_ASSERT("repeating the same aligned AMO on the PBMT=IO fault alias still reports store access fault with stable tval before the same-backing-page cacheable-alias recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(cacheable_vaddr);
    TEST_ASSERT("after the repeated PBMT=IO AMO faults, a cacheable alias to the same backing page still reads the original payload",
        excpt.triggered == false &&
        observed_before == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoor_d(cacheable_vaddr, amo_mask);
    TEST_ASSERT("after the repeated PBMT=IO AMO faults, an aligned cacheable AMO through a second alias to the same backing page still succeeds and returns the original value",
        excpt.triggered == false &&
        amo_old == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(cacheable_vaddr);
    TEST_ASSERT("after the same-backing-page cacheable-alias AMO recovery in the PBMT=IO variant, the cacheable alias immediately observes the AMO-updated payload",
        excpt.triggered == false &&
        observed_after == updated_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t word_old = amoadd_w(word_cacheable_vaddr, word_addend);
    TEST_ASSERT("after the repeated PBMT=IO AMO faults, a cacheable AMOADD.W on a separate word in the same backing page succeeds and returns the old word",
        excpt.triggered == false &&
        word_old == (uint32_t)word_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t word_observed = ld(word_cacheable_vaddr);
    TEST_ASSERT("the cacheable AMOADD.W recovery updates only the target word lane after the PBMT=IO AMO faults",
        excpt.triggered == false &&
        word_observed == word_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t word_guard_observed = ld(word_guard_cacheable_vaddr);
    TEST_ASSERT("the PBMT=IO fault probes plus cacheable W/D AMO recoveries preserve the extra word-lane guard doubleword",
        excpt.triggered == false &&
        word_guard_observed == word_guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_cacheable_vaddr);
    TEST_ASSERT("the repeated PBMT=IO AMO faults and the later same-backing-page cacheable-alias AMO recovery leave the adjacent doubleword unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the PBMT=IO fault-alias attempts and cacheable W/D AMO recoveries, backing memory reflects only the intended target updates and preserves both guards",
        read64(backing_paddr) == updated_value &&
        read64(backing_paddr + sizeof(uint64_t)) == adjacent_value &&
        read64(word_paddr) == word_updated &&
        read64(word_guard_paddr) == word_guard
    );

    ai_arch_reset_amo_fault_env();

    TEST_END();
}

bool ai_arch_amo_pma_device_region_fault()
{
    TEST_START();

    uintptr_t backing_paddr = ai_arch_amo_fault_device_backing_paddr(
        AI_ARCH_AMO_PMA_BACKING_PAGE);
    uintptr_t device_vaddr = ai_arch_prepare_pma_device_amo_alias(
        AI_ARCH_AMO_PMA_ALIAS_PAGE, AI_ARCH_AMO_PMA_BACKING_PAGE);
    uintptr_t adjacent_paddr = backing_paddr + sizeof(uint64_t);
    uintptr_t adjacent_device_vaddr = device_vaddr + sizeof(uint64_t);
    uintptr_t misaligned_paddr = backing_paddr + 2U * sizeof(uint64_t);
    uintptr_t misaligned_device_vaddr = device_vaddr +
        2U * sizeof(uint64_t) + 1U;
    uintptr_t misaligned_guard_paddr = backing_paddr + 3U * sizeof(uint64_t);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t adjacent_value = 0x8877665544332211ULL;
    const uint64_t misaligned_value = 0xfeedfacecafebeefULL;
    const uint64_t misaligned_guard = 0x0123456789abcdefULL;
    const uint64_t or_mask = 0x00ff0000ff0000ffULL;
    const uint64_t addend = 0x1111111111111111ULL;
    const uint64_t rd_sentinel_or = 0xface00000000de71ULL;
    const uint64_t rd_sentinel_add = 0xface00000000de72ULL;
    ai_arch_amo_probe_pc_t pc = {0};
    ai_arch_amo_probe_pc_t pc_misaligned = {0};

    goto_priv(PRIV_M);
    write64(backing_paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);
    write64(misaligned_paddr, misaligned_value);
    write64(misaligned_guard_paddr, misaligned_guard);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_w(device_vaddr, 0x13579bdfU);
    TEST_ASSERT("an aligned AMO on a PMA=Device alias reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    uint64_t fault_rd = ai_arch_amoor_d_probe_rd(adjacent_device_vaddr,
        or_mask, rd_sentinel_or, &pc);
    TEST_ASSERT("an aligned AMOOR.D on the PMA=Device/no-atomic alias reports store access fault and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.epc == pc.pc &&
        pc.next_pc == pc.pc + 4U &&
        fault_rd == rd_sentinel_or
    );

    TEST_SETUP_EXCEPT();
    uint64_t misaligned_fault_rd = ai_arch_amoadd_d_probe_rd(
        misaligned_device_vaddr, addend, rd_sentinel_add, &pc_misaligned);
    TEST_ASSERT("a misaligned AMOADD.D on the same PMA=Device/no-atomic alias reports store access fault and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.epc == pc_misaligned.pc &&
        pc_misaligned.next_pc == pc_misaligned.pc + 4U &&
        misaligned_fault_rd == rd_sentinel_add
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMA=Device AMOADD.W, AMOOR.D, and misaligned AMOADD.D faults leave all backing doublewords unchanged",
        read64(backing_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_value &&
        read64(misaligned_paddr) == misaligned_value &&
        read64(misaligned_guard_paddr) == misaligned_guard
    );
    ai_arch_reset_amo_fault_env();

    TEST_END();
}

bool ai_arch_amo_mprv_u_same_page_pmp_repeated_fault_then_restore_recovers_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_ARCH_AMO_MPRV_PAGE) + 0x1c0U;
    const uintptr_t paddr = phys_page_base(AI_ARCH_AMO_MPRV_PAGE) + 0x1c0U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x123456789abcdef0ULL;
    const uint64_t adjacent_value = 0x0fedcba987654321ULL;
    const uint64_t amo_mask = 0x00ff0000ff0000ffULL;
    const uint64_t updated_value = initial_value | amo_mask;
    const uint64_t first_rd_sentinel = 0xface00000000c001ULL;
    const uint64_t second_rd_sentinel = 0xface00000000c002ULL;
    const uint64_t aq_rd_sentinel = 0xface00000000c0a1ULL;
    const uint64_t rl_rd_sentinel = 0xface00000000c0a2ULL;
    const uint64_t xor_rd_sentinel = 0xface00000000c0b1ULL;
    const uint64_t and_w_rd_sentinel = 0xface00000000c0b2ULL;
    ai_arch_amo_probe_pc_t pc0 = {0};
    ai_arch_amo_probe_pc_t pc1 = {0};
    ai_arch_amo_probe_pc_t pc_aq = {0};
    ai_arch_amo_probe_pc_t pc_rl = {0};
    ai_arch_amo_probe_pc_t pc_xor = {0};
    ai_arch_amo_probe_pc_t pc_and_w = {0};

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);
    ai_arch_amo_restore_pmp_full_access();
    ai_arch_amo_prepare_mprv_user_translation();

    goto_priv(PRIV_M);
    ai_arch_amo_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_ARCH_AMO_PMP_TOR_NO_W);
    ai_arch_amo_enable_mprv_user_translation();

    TEST_SETUP_EXCEPT();
    uint64_t first_fault_rd = ai_arch_amoor_d_probe_rd(vaddr, amo_mask,
        first_rd_sentinel, &pc0);
    TEST_ASSERT("a first M-mode translated AMO reports store access fault and suppresses rd writeback when MPRV=1, MPP=U and PMP denies the write side on the selected page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.epc == pc0.pc &&
        pc0.next_pc == pc0.pc + 4U &&
        first_fault_rd == first_rd_sentinel
    );

    ai_arch_amo_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    uint64_t second_fault_rd = ai_arch_amoor_d_probe_rd(vaddr, amo_mask,
        second_rd_sentinel, &pc1);
    TEST_ASSERT("repeating the same M-mode translated AMO while the deny window remains active still reports store access fault with the same translated address and a fresh rd sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.epc == pc1.pc &&
        pc1.next_pc == pc1.pc + 4U &&
        pc1.pc != pc0.pc &&
        second_fault_rd == second_rd_sentinel,
        "triggered=%d cause=0x%llx tval=0x%llx expected_cause=0x%llx expected_tval=0x%llx rd=0x%llx expected_rd=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, (uint64_t)CAUSE_SAF, (uint64_t)vaddr
        , second_fault_rd, second_rd_sentinel
    );

    ai_arch_amo_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    uint64_t aq_fault_rd = ai_arch_amoadd_d_aq_probe_rd(vaddr,
        0x0102030405060708ULL, aq_rd_sentinel, &pc_aq);
    TEST_ASSERT("amoadd.d.aq under the same PMP-denied translated window reports store access fault, keeps rd at the sentinel, and creates no acquire-side RMW",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.epc == pc_aq.pc &&
        pc_aq.next_pc == pc_aq.pc + 4U &&
        aq_fault_rd == aq_rd_sentinel
    );

    ai_arch_amo_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    uint64_t rl_fault_rd = ai_arch_amoadd_d_rl_probe_rd(vaddr,
        0x1111111111111111ULL, rl_rd_sentinel, &pc_rl);
    TEST_ASSERT("amoadd.d.rl under the same PMP-denied translated window reports store access fault, keeps rd at the sentinel, and creates no release-side memory update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.epc == pc_rl.pc &&
        pc_rl.next_pc == pc_rl.pc + 4U &&
        rl_fault_rd == rl_rd_sentinel
    );

    ai_arch_amo_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    uint64_t xor_fault_rd = ai_arch_amoxor_d_probe_rd(vaddr,
        0x0f0f00ff0f0f00ffULL, xor_rd_sentinel, &pc_xor);
    TEST_ASSERT("AMOXOR.D under the same PMP-denied translated window reports store access fault, keeps rd at the sentinel, and creates no logic-xor memory update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.epc == pc_xor.pc &&
        pc_xor.next_pc == pc_xor.pc + 4U &&
        xor_fault_rd == xor_rd_sentinel
    );

    ai_arch_amo_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    uint64_t and_w_fault_rd = ai_arch_amoand_w_probe_rd(vaddr,
        0x0ff00f0fU, and_w_rd_sentinel, &pc_and_w);
    TEST_ASSERT("AMOAND.W under the same PMP-denied translated window reports store access fault, keeps rd at the sentinel, and creates no word-lane memory update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.epc == pc_and_w.pc &&
        pc_and_w.next_pc == pc_and_w.pc + 4U &&
        and_w_fault_rd == and_w_rd_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated denied translated AMO probes, including aq/rl and logic W/D variants, leave both the target and adjacent backing words unchanged before PMP permission is restored",
        read64(paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_value
    );
    ai_arch_amo_restore_pmp_full_access();
    ai_arch_amo_enable_mprv_user_translation();

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoor_d(vaddr, amo_mask);
    TEST_ASSERT("after restoring PMP write permission without rebuilding the translation context, the same translated AMO succeeds immediately and returns the original value",
        excpt.triggered == false &&
        amo_old == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(vaddr);
    TEST_ASSERT("after the repaired translated AMO, a later translated load observes the AMO-updated payload immediately",
        excpt.triggered == false &&
        observed_after == updated_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the same-page translated AMO permission-repair corner leaves the adjacent translated word unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated denied translated AMO probes followed by the repaired AMO update only the target backing word",
        read64(paddr) == updated_value &&
        read64(adjacent_paddr) == adjacent_value
    );
    ai_arch_amo_restore_pmp_full_access();
    ai_arch_amo_clear_machine_access_mode();

    TEST_END();
}

bool ai_arch_amo_mprv_u_same_page_pmp_repeated_fault_then_sfence_restore_recovers_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_ARCH_AMO_MPRV_PAGE) + 0x240U;
    const uintptr_t paddr = phys_page_base(AI_ARCH_AMO_MPRV_PAGE) + 0x240U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x76543210fedcba98ULL;
    const uint64_t adjacent_value = 0x1122334455667788ULL;
    const uint64_t addend = 0x0101010101010101ULL;
    const uint64_t updated_value = initial_value + addend;
    const uint64_t first_rd_sentinel = 0xface00000000d001ULL;
    const uint64_t second_rd_sentinel = 0xface00000000d002ULL;
    ai_arch_amo_probe_pc_t pc0 = {0};
    ai_arch_amo_probe_pc_t pc1 = {0};

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);
    ai_arch_amo_restore_pmp_full_access();
    ai_arch_amo_prepare_mprv_user_translation();

    goto_priv(PRIV_M);
    ai_arch_amo_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_ARCH_AMO_PMP_TOR_NO_W);
    ai_arch_amo_enable_mprv_user_translation();

    TEST_SETUP_EXCEPT();
    uint64_t first_fault_rd = ai_arch_amoadd_d_probe_rd(vaddr, addend,
        first_rd_sentinel, &pc0);
    TEST_ASSERT("a first M-mode translated AMO reports store access fault and suppresses rd writeback before the explicit sfence-based permission-restore probe when PMP denies writes on the selected page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.epc == pc0.pc &&
        pc0.next_pc == pc0.pc + 4U &&
        first_fault_rd == first_rd_sentinel
    );

    ai_arch_amo_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    uint64_t second_fault_rd = ai_arch_amoadd_d_probe_rd(vaddr, addend,
        second_rd_sentinel, &pc1);
    TEST_ASSERT("repeating the same M-mode translated AMO before the explicit sfence-based permission restore still reports store access fault with the same translated address and a fresh rd sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.epc == pc1.pc &&
        pc1.next_pc == pc1.pc + 4U &&
        pc1.pc != pc0.pc &&
        second_fault_rd == second_rd_sentinel,
        "triggered=%d cause=0x%llx tval=0x%llx expected_cause=0x%llx expected_tval=0x%llx rd=0x%llx expected_rd=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, (uint64_t)CAUSE_SAF, (uint64_t)vaddr
        , second_fault_rd, second_rd_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated denied translated AMO probes in the explicit sfence repair corner leave both backing words unchanged before permission restore",
        read64(paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_value
    );
    ai_arch_amo_restore_pmp_full_access();
    ai_arch_amo_enable_mprv_user_translation();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoadd_d(vaddr, addend);
    TEST_ASSERT("after restoring PMP write permission and issuing sfence.vma, the same translated AMO succeeds immediately and returns the original value",
        excpt.triggered == false &&
        amo_old == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(vaddr);
    TEST_ASSERT("after the explicit sfence-based permission repair, a later translated load observes the AMO-updated payload immediately",
        excpt.triggered == false &&
        observed_after == updated_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the explicit sfence-based translated AMO permission-repair corner leaves the adjacent translated word unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated denied translated AMO probes followed by the explicit sfence-based repaired AMO update only the target backing word",
        read64(paddr) == updated_value &&
        read64(adjacent_paddr) == adjacent_value
    );
    ai_arch_amo_restore_pmp_full_access();
    ai_arch_amo_clear_machine_access_mode();

    TEST_END();
}

bool ai_arch_amo_permission_fault_metadata_no_side_effect_matrix_corner()
{
    TEST_START();

    const enum test_page no_w_page = VSRWX_GRWX;
    const enum test_page no_r_page = VSRW_GURWX;
    const enum test_page user_page = VSURWX_GURWX;
    const uintptr_t no_w_vaddr = hs_page_base(no_w_page) + 0x180U;
    const uintptr_t no_w_d_vaddr = hs_page_base(no_w_page) + 0x1c0U;
    const uintptr_t no_r_vaddr = hs_page_base(no_r_page) + 0x1a0U;
    const uintptr_t user_vaddr = hs_page_base(user_page) + 0x1e0U;
    const uintptr_t no_w_paddr = phys_page_base(no_w_page) + 0x180U;
    const uintptr_t no_w_d_paddr = phys_page_base(no_w_page) + 0x1c0U;
    const uintptr_t no_r_paddr = phys_page_base(no_r_page) + 0x1a0U;
    const uintptr_t user_paddr = phys_page_base(user_page) + 0x1e0U;
    const uint64_t no_w_before_guard = 0x1111222233334444ULL;
    const uint64_t no_w_initial = 0x5566778899aabbccULL;
    const uint64_t no_w_after_guard = 0xddddeeeeffff0001ULL;
    const uint64_t no_w_d_before_guard = 0x3141592653589793ULL;
    const uint64_t no_w_d_initial = 0x1122334455667788ULL;
    const uint64_t no_w_d_after_guard = 0x2718281828459045ULL;
    const uint64_t no_w_d_addend = 0x0101010101010101ULL;
    const uint64_t no_w_d_updated = 0x1223344556677889ULL;
    const uint64_t no_r_before_guard = 0x1021324354657687ULL;
    const uint64_t no_r_initial = 0x8877665544332211ULL;
    const uint64_t no_r_after_guard = 0x90a1b2c3d4e5f607ULL;
    const uint64_t user_before_guard = 0x1234432112344321ULL;
    const uint64_t user_initial = 0x2468ace013579bdfULL;
    const uint64_t user_after_guard = 0xfdb97531eca86420ULL;
    const uint32_t no_w_low_word = (uint32_t)no_w_initial;
    const uint64_t sentinel0 = 0xface000000000001ULL;
    const uint64_t sentinel1 = 0xface000000000002ULL;
    const uint64_t sentinel2 = 0xface000000000003ULL;
    const uint64_t sentinel3 = 0xface000000000004ULL;
    const uint64_t sentinel4 = 0xface000000000005ULL;
    const uint64_t sentinel5 = 0xface000000000006ULL;
    const uint64_t sentinel6 = 0xface000000000007ULL;
    const uint64_t sentinel7 = 0xface000000000008ULL;
    const uint64_t sentinel8 = 0xface000000000009ULL;
    const uint64_t sentinel9 = 0xface00000000000aULL;
    const uint64_t no_w_guards[8] = {
        no_w_before_guard, 0x1000000000000002ULL,
        0x1000000000000003ULL, 0x1000000000000004ULL,
        no_w_after_guard, 0x1000000000000006ULL,
        0x1000000000000007ULL, 0x1000000000000008ULL
    };
    const uint64_t no_r_guards[8] = {
        no_r_before_guard, 0x2000000000000002ULL,
        0x2000000000000003ULL, 0x2000000000000004ULL,
        no_r_after_guard, 0x2000000000000006ULL,
        0x2000000000000007ULL, 0x2000000000000008ULL
    };
    const uint64_t user_guards[8] = {
        user_before_guard, 0x3000000000000002ULL,
        0x3000000000000003ULL, 0x3000000000000004ULL,
        user_after_guard, 0x3000000000000006ULL,
        0x3000000000000007ULL, 0x3000000000000008ULL
    };
    ai_arch_amo_probe_pc_t pc0 = {0};
    ai_arch_amo_probe_pc_t pc1 = {0};
    ai_arch_amo_probe_pc_t pc2 = {0};
    ai_arch_amo_probe_pc_t pc3 = {0};
    ai_arch_amo_probe_pc_t pc4 = {0};
    ai_arch_amo_probe_pc_t pc5 = {0};
    ai_arch_amo_probe_pc_t pc7 = {0};
    ai_arch_amo_probe_pc_t pc8 = {0};
    ai_arch_amo_probe_pc_t pc9 = {0};
    ai_arch_amo_probe_pc_t pc10 = {0};

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM | MSTATUS_MPRV | MSTATUS_SUM |
        MSTATUS_MXR);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    ai_arch_amo_restore_pmp_full_access();
    for (unsigned i = 0; i < 4U; ++i) {
        write64(no_w_paddr - ((uintptr_t)(i + 1U) * sizeof(uint64_t)),
            no_w_guards[i]);
        write64(no_w_paddr + ((uintptr_t)(i + 1U) * sizeof(uint64_t)),
            no_w_guards[i + 4U]);
        write64(no_r_paddr - ((uintptr_t)(i + 1U) * sizeof(uint64_t)),
            no_r_guards[i]);
        write64(no_r_paddr + ((uintptr_t)(i + 1U) * sizeof(uint64_t)),
            no_r_guards[i + 4U]);
        write64(user_paddr - ((uintptr_t)(i + 1U) * sizeof(uint64_t)),
            user_guards[i]);
        write64(user_paddr + ((uintptr_t)(i + 1U) * sizeof(uint64_t)),
            user_guards[i + 4U]);
    }
    write64(no_w_paddr - sizeof(uint64_t), no_w_before_guard);
    write64(no_w_paddr, no_w_initial);
    write64(no_w_paddr + sizeof(uint64_t), no_w_after_guard);
    write64(no_r_paddr - sizeof(uint64_t), no_r_before_guard);
    write64(no_r_paddr, no_r_initial);
    write64(no_r_paddr + sizeof(uint64_t), no_r_after_guard);
    write64(user_paddr - sizeof(uint64_t), user_before_guard);
    write64(user_paddr, user_initial);
    write64(user_paddr + sizeof(uint64_t), user_after_guard);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_amo_set_hs_leaf(no_w_page, AI_ARCH_AMO_PERMISSION_LEGAL_LEAF);
    TEST_SETUP_EXCEPT();
    uint32_t reservation_clear_lr = lr_w(no_w_vaddr);
    uint32_t reservation_clear_sc = sc_w(no_w_vaddr, no_w_low_word);
    TEST_ASSERT("a warm-up LR/SC on the no-W target succeeds while the leaf is legal, leaving no stale reservation before the AMO fault matrix",
        excpt.triggered == false &&
        reservation_clear_lr == no_w_low_word &&
        reservation_clear_sc == 0U
    );

    ai_arch_amo_set_hs_leaf(no_w_page, AI_ARCH_AMO_PERMISSION_FAULT_LEAF);
    ai_arch_amo_set_hs_leaf(no_r_page, AI_ARCH_AMO_PERMISSION_NO_R_LEAF);
    ai_arch_amo_set_hs_leaf(user_page, AI_ARCH_AMO_PERMISSION_USER_LEAF);

    TEST_ASSERT("the AMO permission-fault matrix starts from ordinary DRAM pages with no-W, no-R, and user leaves installed",
        ai_arch_amo_hs_leaf_matches(no_w_page,
            AI_ARCH_AMO_PERMISSION_FAULT_LEAF) &&
        ai_arch_amo_hs_leaf_matches(no_r_page,
            AI_ARCH_AMO_PERMISSION_NO_R_LEAF) &&
        ai_arch_amo_hs_leaf_matches(user_page,
            AI_ARCH_AMO_PERMISSION_USER_LEAF)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t amoswap_no_w_rd = ai_arch_amoswap_w_probe_rd(no_w_vaddr,
        0xa5a55a5aU, sentinel0, &pc0);
    TEST_ASSERT("AMOSWAP.W to a PTE.W=0 page reports Store/AMO page fault at the target VA, keeps rd at the sentinel, and records the AMO PC",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == no_w_vaddr &&
        excpt.epc == pc0.pc &&
        pc0.next_pc == pc0.pc + 4U &&
        amoswap_no_w_rd == sentinel0
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_amoadd_no_w_rd = ai_arch_amoadd_w_probe_rd(no_w_vaddr,
        0x00000001U, sentinel1, &pc1);
    TEST_ASSERT("the first AMOADD.W to the same PTE.W=0 page refreshes Store/AMO page-fault metadata independently and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == no_w_vaddr &&
        excpt.epc == pc1.pc &&
        pc1.next_pc == pc1.pc + 4U &&
        first_amoadd_no_w_rd == sentinel1
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_amoadd_no_w_rd = ai_arch_amoadd_w_probe_rd(
        no_w_vaddr + sizeof(uint32_t), 0x00000002U, sentinel2, &pc2);
    TEST_ASSERT("a second AMOADD.W fault at the upper word of the no-W page reports its own PC and faulting VA without inheriting stale metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == no_w_vaddr + sizeof(uint32_t) &&
        excpt.epc == pc2.pc &&
        pc2.next_pc == pc2.pc + 4U &&
        pc2.pc != pc1.pc &&
        second_amoadd_no_w_rd == sentinel2
    );

    TEST_SETUP_EXCEPT();
    uint64_t amoxor_no_r_rd = ai_arch_amoxor_d_probe_rd(no_r_vaddr,
        0x00ff00ff00ff00ffULL, sentinel3, &pc3);
    TEST_ASSERT("AMOXOR.D to an R=0/W=1 illegal leaf is rejected as Store/AMO page fault before any read-modify-write result reaches rd or memory",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == no_r_vaddr &&
        excpt.epc == pc3.pc &&
        pc3.next_pc == pc3.pc + 4U &&
        amoxor_no_r_rd == sentinel3
    );

    TEST_SETUP_EXCEPT();
    uint64_t amominu_no_r_rd = ai_arch_amominu_d_probe_rd(no_r_vaddr,
        0x0101010101010101ULL, sentinel4, &pc4);
    TEST_ASSERT("AMOMINU.D repeats the no-R AMO page-fault path with a min/max-family opcode and still leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == no_r_vaddr &&
        excpt.epc == pc4.pc &&
        pc4.next_pc == pc4.pc + 4U &&
        amominu_no_r_rd == sentinel4
    );

    TEST_SETUP_EXCEPT();
    uint64_t amomaxu_no_r_rd = ai_arch_amomaxu_d_probe_rd(no_r_vaddr,
        0xffffffffffffffffULL, sentinel5, &pc5);
    TEST_ASSERT("AMOMAXU.D also takes the no-R AMO page-fault path for the unsigned max opcode and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == no_r_vaddr &&
        excpt.epc == pc5.pc &&
        pc5.next_pc == pc5.pc + 4U &&
        amomaxu_no_r_rd == sentinel5
    );

    TEST_SETUP_EXCEPT();
    uint64_t amoadd_aq_no_w_rd = ai_arch_amoadd_d_aq_probe_rd(
        no_w_vaddr, 0x0102030405060708ULL, sentinel6, &pc7);
    TEST_ASSERT("AMOADD.D.AQ fault on the read-only leaf reports Store/AMO page fault, keeps rd at its sentinel, and creates no acquire observation point",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == no_w_vaddr &&
        excpt.epc == pc7.pc &&
        pc7.next_pc == pc7.pc + 4U &&
        amoadd_aq_no_w_rd == sentinel6
    );

    TEST_SETUP_EXCEPT();
    uint64_t amoadd_rl_no_w_rd = ai_arch_amoadd_d_rl_probe_rd(
        no_w_vaddr, 0x1111111111111111ULL, sentinel7, &pc8);
    TEST_ASSERT("AMOADD.D.RL fault on the read-only leaf reports Store/AMO page fault, keeps rd at its sentinel, and creates no release-side memory update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == no_w_vaddr &&
        excpt.epc == pc8.pc &&
        pc8.next_pc == pc8.pc + 4U &&
        amoadd_rl_no_w_rd == sentinel7
    );

    TEST_SETUP_EXCEPT();
    uint64_t amoadd_aqrl_no_r_rd = ai_arch_amoadd_d_aqrl_probe_rd(
        no_r_vaddr, 0x2222222222222222ULL, sentinel8, &pc9);
    TEST_ASSERT("AMOADD.D.AQRL fault on the no-R AMO leaf reports its own Store/AMO page fault and does not perform a read-modify-write",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == no_r_vaddr &&
        excpt.epc == pc9.pc &&
        pc9.next_pc == pc9.pc + 4U &&
        amoadd_aqrl_no_r_rd == sentinel8
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    ai_arch_amo_probe_pc_t pc6 = {0};
    TEST_SETUP_EXCEPT();
    uint64_t amoswap_sum0_rd = ai_arch_amoswap_w_probe_rd(user_vaddr,
        0x13579bdfU, sentinel0, &pc6);
    TEST_ASSERT("HS AMOSWAP.W with SUM=0 to a PTE.U=1 page reports Store/AMO page fault and leaves rd at the sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == user_vaddr &&
        excpt.epc == pc6.pc &&
        pc6.next_pc == pc6.pc + 4U &&
        amoswap_sum0_rd == sentinel0
    );

    goto_priv(PRIV_M);
    bool guard_window_unchanged = true;
    for (unsigned i = 0; i < 4U; ++i) {
        guard_window_unchanged = guard_window_unchanged &&
            read64(no_w_paddr - ((uintptr_t)(i + 1U) *
                sizeof(uint64_t))) == no_w_guards[i] &&
            read64(no_w_paddr + ((uintptr_t)(i + 1U) *
                sizeof(uint64_t))) == no_w_guards[i + 4U] &&
            read64(no_r_paddr - ((uintptr_t)(i + 1U) *
                sizeof(uint64_t))) == no_r_guards[i] &&
            read64(no_r_paddr + ((uintptr_t)(i + 1U) *
                sizeof(uint64_t))) == no_r_guards[i + 4U] &&
            read64(user_paddr - ((uintptr_t)(i + 1U) *
                sizeof(uint64_t))) == user_guards[i] &&
            read64(user_paddr + ((uintptr_t)(i + 1U) *
                sizeof(uint64_t))) == user_guards[i + 4U];
    }
    TEST_ASSERT("all page-permission AMO faults leave the target words and the four surrounding guard doublewords on each side unchanged",
        guard_window_unchanged &&
        read64(no_w_paddr - sizeof(uint64_t)) == no_w_before_guard &&
        read64(no_w_paddr) == no_w_initial &&
        read64(no_w_paddr + sizeof(uint64_t)) == no_w_after_guard &&
        read64(no_r_paddr - sizeof(uint64_t)) == no_r_before_guard &&
        read64(no_r_paddr) == no_r_initial &&
        read64(no_r_paddr + sizeof(uint64_t)) == no_r_after_guard &&
        read64(user_paddr - sizeof(uint64_t)) == user_before_guard &&
        read64(user_paddr) == user_initial &&
        read64(user_paddr + sizeof(uint64_t)) == user_after_guard
    );

    goto_priv(PRIV_HS);
    ai_arch_amo_set_hs_leaf(no_w_page, AI_ARCH_AMO_PERMISSION_LEGAL_LEAF);
    TEST_SETUP_EXCEPT();
    uint32_t stale_sc_status = sc_w(no_w_vaddr, 0xfeedbeefU);
    TEST_ASSERT("after the denied AMO probes and permission repair, SC.W without a fresh LR fails, showing the faulting AMOs did not create a reservation",
        excpt.triggered == false &&
        stale_sc_status != 0U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the failed post-fault SC.W leaves the repaired target word unchanged before the recovery AMOADD.W",
        read64(no_w_paddr) == no_w_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t repaired_old = ai_arch_amoadd_w_probe_rd(no_w_vaddr,
        0x01010101U, sentinel1, &pc1);
    TEST_ASSERT("after repairing the no-W leaf and issuing SFENCE.VMA, AMOADD.W re-executes normally and returns the sign-extended old low word",
        excpt.triggered == false &&
        repaired_old == 0xffffffff99aabbccULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired AMOADD.W updates only the target low word while preserving the upper word and both adjacent guards",
        read64(no_w_paddr - sizeof(uint64_t)) == no_w_before_guard &&
        read64(no_w_paddr) == 0x556677889aabbccdULL &&
        read64(no_w_paddr + sizeof(uint64_t)) == no_w_after_guard
    );

    write64(no_w_d_paddr - sizeof(uint64_t), no_w_d_before_guard);
    write64(no_w_d_paddr, no_w_d_initial);
    write64(no_w_d_paddr + sizeof(uint64_t), no_w_d_after_guard);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t repaired_d_old = ai_arch_amoadd_d_probe_rd(no_w_d_vaddr,
        no_w_d_addend, sentinel9, &pc10);
    TEST_ASSERT("after the same permission repair, AMOADD.D on a neighboring aligned doubleword completes normally without inheriting stale AMO fault metadata",
        excpt.triggered == false &&
        repaired_d_old == no_w_d_initial &&
        pc10.next_pc == pc10.pc + 4U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired AMOADD.D updates exactly the aligned doubleword and preserves the adjacent doubleword guards",
        read64(no_w_d_paddr - sizeof(uint64_t)) == no_w_d_before_guard &&
        read64(no_w_d_paddr) == no_w_d_updated &&
        read64(no_w_d_paddr + sizeof(uint64_t)) == no_w_d_after_guard
    );

    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_amoand_d_noncanonical_va_page_fault_no_side_effect_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t canonical_vaddr = hs_page_base(target_page) + 0x280U;
    const uintptr_t bad_vaddr =
        ai_arch_amo_make_noncanonical_vaddr(canonical_vaddr);
    const uintptr_t paddr = phys_page_base(target_page) + 0x280U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0xf0f0cc33aa55ff00ULL;
    const uint64_t adjacent_value = 0x0123456789abcdefULL;
    const uint64_t and_mask = 0x0ff0ffff00ffff0fULL;
    const uint64_t updated_value = initial_value & and_mask;
    const uint64_t rd_sentinel = 0xface00000000a11dULL;
    ai_arch_amo_probe_pc_t pc = {0};

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t fault_rd = ai_arch_amoand_d_probe_rd(bad_vaddr, and_mask,
        rd_sentinel, &pc);
    TEST_ASSERT("AMOAND.D to a non-canonical HS virtual address reports Store/AMO page fault with the raw non-canonical VA and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr &&
        excpt.epc == pc.pc &&
        pc.next_pc == pc.pc + 4U &&
        fault_rd == rd_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the non-canonical AMOAND.D page fault performs no read-modify-write and preserves the adjacent doubleword",
        read64(paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_value
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t repaired_old = ai_arch_amoand_d_probe_rd(canonical_vaddr,
        and_mask, rd_sentinel, &pc);
    TEST_ASSERT("after the non-canonical AMOAND.D fault, the canonical aligned AMOAND.D succeeds and returns the original doubleword",
        excpt.triggered == false &&
        repaired_old == initial_value &&
        pc.next_pc == pc.pc + 4U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired canonical AMOAND.D updates only the target doubleword and keeps the adjacent guard unchanged",
        read64(paddr) == updated_value &&
        read64(adjacent_paddr) == adjacent_value
    );

    TEST_END();
}

bool ai_arch_hyper_vs_lr_guest_page_fault_tval2_matches_faulting_gpa_corner()
{
    TEST_START();

    const uintptr_t fault_vaddr = ai_arch_amo_guest_fault_vaddr();
    const uintptr_t good_vaddr = ai_arch_amo_guest_good_vaddr();
    const uintptr_t good_paddr = ai_arch_amo_guest_good_paddr();
    const uint64_t initial = 0x1122334455667788ULL;
    const uint64_t addend = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    ai_arch_amo_prepare_vs_guest_env();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    lr_d(fault_vaddr);
    TEST_ASSERT("a VS-mode lr.d into a VS-valid but G-stage-invalid page reports load guest page fault with stval at the faulting GVA and htval/mtval2 at the faulting GPA>>2",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == fault_vaddr &&
        excpt.tval2 == ai_arch_amo_guest_fault_expected_tval2(fault_vaddr)
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old = amoadd_d(good_vaddr, addend);
    TEST_ASSERT("after that VS-mode lr.d guest-page-fault probe, a later VS-mode amoadd.d on a valid guest page still succeeds and returns the original payload",
        excpt.triggered == false &&
        good_old == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the VS-mode lr.d guest-page-fault probe leaves the recovery backing word updated only by the later valid amoadd.d",
        read64(good_paddr) == (initial + addend)
    );
    ai_arch_amo_cleanup_vs_guest_env();

    TEST_END();
}

bool ai_arch_hyper_vs_amo_guest_page_fault_tval2_matches_faulting_gpa_corner()
{
    TEST_START();

    const uintptr_t fault_vaddr = ai_arch_amo_guest_fault_vaddr();
    const uintptr_t good_vaddr = ai_arch_amo_guest_good_vaddr();
    const uintptr_t good_paddr = ai_arch_amo_guest_good_paddr();
    const uint64_t initial = 0x8877665544332211ULL;
    const uint64_t addend = 0x1112131415161718ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    ai_arch_amo_prepare_vs_guest_env();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    amoadd_d(fault_vaddr, addend);
    TEST_ASSERT("a VS-mode amoadd.d into a VS-valid but G-stage-invalid page reports store guest page fault with stval at the faulting GVA and htval/mtval2 at the faulting GPA>>2",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SGPF &&
        excpt.tval == fault_vaddr &&
        excpt.tval2 == ai_arch_amo_guest_fault_expected_tval2(fault_vaddr)
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old = amoadd_d(good_vaddr, addend);
    TEST_ASSERT("after that VS-mode guest-page-faulting amoadd.d, a later VS-mode amoadd.d on a valid guest page still succeeds and returns the original payload",
        excpt.triggered == false &&
        good_old == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the VS-mode guest-page-faulting amoadd.d leaves the recovery backing word updated only by the later valid amoadd.d",
        read64(good_paddr) == (initial + addend)
    );
    ai_arch_amo_cleanup_vs_guest_env();

    TEST_END();
}

bool ai_arch_hyper_vs_amo_guest_page_fault_tval2_refreshes_to_next_gpa_corner()
{
    TEST_START();

    const uintptr_t first_fault_vaddr = ai_arch_amo_guest_fault_vaddr();
    const uintptr_t second_fault_vaddr = ai_arch_amo_guest_fault_vaddr_next();
    const uintptr_t good_vaddr = ai_arch_amo_guest_good_vaddr();
    const uintptr_t good_paddr = ai_arch_amo_guest_good_paddr();
    const uint64_t initial = 0x123456789abcdef0ULL;
    const uint64_t addend = 0x0101010101010101ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    ai_arch_amo_prepare_vs_guest_env();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    amoadd_d(first_fault_vaddr, 0x1112131415161718ULL);
    TEST_ASSERT("the first VS-mode guest-page-faulting amoadd.d reports htval/mtval2 from the first faulting guest physical address",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SGPF &&
        excpt.tval == first_fault_vaddr &&
        excpt.tval2 == ai_arch_amo_guest_fault_expected_tval2(first_fault_vaddr)
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(second_fault_vaddr, 0x2122232425262728ULL);
    TEST_ASSERT("the next VS-mode guest-page-faulting amoadd.d at a different byte offset refreshes both stval and htval/mtval2 to the newer faulting address",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SGPF &&
        excpt.tval == second_fault_vaddr &&
        excpt.tval2 == ai_arch_amo_guest_fault_expected_tval2(second_fault_vaddr)
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old = amoadd_d(good_vaddr, addend);
    TEST_ASSERT("after two different VS-mode guest-page-faulting amoadd.d probes, a later valid VS-mode amoadd.d still succeeds and returns the original payload",
        excpt.triggered == false &&
        good_old == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated VS-mode guest-page-faulting amoadd.d probes leave the recovery backing word updated only by the later valid amoadd.d",
        read64(good_paddr) == (initial + addend)
    );
    ai_arch_amo_cleanup_vs_guest_env();

    TEST_END();
}

bool ai_arch_hyper_vs_atomic_guest_fault_switch_lr_then_amo_keeps_latest_cause_and_tval2_corner()
{
    TEST_START();

    const uintptr_t load_fault_vaddr = ai_arch_amo_guest_fault_vaddr();
    const uintptr_t store_fault_vaddr = ai_arch_amo_guest_fault_vaddr_next();
    const uintptr_t good_vaddr = ai_arch_amo_guest_good_vaddr();
    const uintptr_t good_paddr = ai_arch_amo_guest_good_paddr();
    const uint64_t initial = 0xfedcba9876543210ULL;
    const uint64_t addend = 0x0001000100010001ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    ai_arch_amo_prepare_vs_guest_env();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    lr_d(load_fault_vaddr);
    TEST_ASSERT("a VS-mode lr.d guest-page-fault probe first reports load guest page fault with the first faulting GPA encoded in htval/mtval2",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == load_fault_vaddr &&
        excpt.tval2 == ai_arch_amo_guest_fault_expected_tval2(load_fault_vaddr)
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(store_fault_vaddr, 0x3333333333333333ULL);
    TEST_ASSERT("when the following VS-mode atomic guest fault switches from lr.d to amoadd.d, the reported cause and htval/mtval2 both refresh to the newer store guest page fault",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SGPF &&
        excpt.tval == store_fault_vaddr &&
        excpt.tval2 == ai_arch_amo_guest_fault_expected_tval2(store_fault_vaddr)
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old = amoadd_d(good_vaddr, addend);
    TEST_ASSERT("after the VS-mode lr.d then amoadd.d guest-fault switch corner, a later valid VS-mode amoadd.d still succeeds and returns the original payload",
        excpt.triggered == false &&
        good_old == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the VS-mode lr.d then amoadd.d guest-fault switch corner leaves the recovery backing word updated only by the later valid amoadd.d",
        read64(good_paddr) == (initial + addend)
    );
    ai_arch_amo_cleanup_vs_guest_env();

    TEST_END();
}

bool ai_arch_hyper_vs_lr_guest_page_fault_nonleaf_vs_pte_sets_tinst_3000_corner()
{
    TEST_START();

    const uintptr_t fault_vaddr = ai_arch_amo_guest_good_vaddr();
    const uintptr_t good_vaddr = ai_arch_amo_guest_good_vaddr();
    const uintptr_t good_paddr = ai_arch_amo_guest_good_paddr();
    const uint64_t initial = UINT64_C(0x1021324354657687);
    const uint64_t addend = UINT64_C(0x0102030405060708);

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    ai_arch_amo_prepare_vs_guest_env();
    ai_arch_amo_arm_vs_nonleaf_pt_guest_fault(
        AI_ARCH_AMO_GUEST_NONLEAF_PT_PAGE);

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    lr_d(fault_vaddr);
    TEST_ASSERT("a VS-mode lr.d whose guest-page fault comes from an implicit VS non-leaf page-table access reports load guest page fault with stval at the original GVA, tval2 at the implicit PTE GPA, and htinst/mtinst=0x3000",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == fault_vaddr &&
        excpt.tval2 == ai_arch_amo_vs_nonleaf_pt_expected_tval2(
            AI_ARCH_AMO_GUEST_NONLEAF_PT_PAGE, fault_vaddr) &&
        excpt.tinst == AI_ARCH_AMO_TINST_VS_NONLEAF_PTE
    );

    goto_priv(PRIV_M);
    ai_arch_amo_prepare_vs_guest_env();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    uint64_t good_old = amoadd_d(good_vaddr, addend);
    TEST_ASSERT("after that implicit-VS-page-table guest faulting lr.d probe, a later valid VS-mode amoadd.d on the mapped guest page still succeeds and returns the original payload",
        excpt.triggered == false &&
        good_old == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the implicit-VS-page-table guest-faulting lr.d probe leaves the recovery backing word updated only by the later valid amoadd.d",
        read64(good_paddr) == (initial + addend)
    );
    ai_arch_amo_cleanup_vs_guest_env();

    TEST_END();
}

bool ai_arch_hyper_vs_amo_guest_page_fault_nonleaf_vs_pte_sets_tinst_3000_corner()
{
    TEST_START();

    const uintptr_t fault_vaddr = ai_arch_amo_guest_good_vaddr();
    const uintptr_t good_vaddr = ai_arch_amo_guest_good_vaddr();
    const uintptr_t good_paddr = ai_arch_amo_guest_good_paddr();
    const uint64_t initial = UINT64_C(0x8877665544332211);
    const uint64_t addend = UINT64_C(0x1112131415161718);

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    ai_arch_amo_prepare_vs_guest_env();
    ai_arch_amo_arm_vs_nonleaf_pt_guest_fault(
        AI_ARCH_AMO_GUEST_NONLEAF_PT_PAGE);

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    amoadd_d(fault_vaddr, addend);
    TEST_ASSERT("a VS-mode amoadd.d whose guest-page fault comes from an implicit VS non-leaf page-table access reports store guest page fault with stval at the original GVA, tval2 at the implicit PTE GPA, and htinst/mtinst=0x3000",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SGPF &&
        excpt.tval == fault_vaddr &&
        excpt.tval2 == ai_arch_amo_vs_nonleaf_pt_expected_tval2(
            AI_ARCH_AMO_GUEST_NONLEAF_PT_PAGE, fault_vaddr) &&
        excpt.tinst == AI_ARCH_AMO_TINST_VS_NONLEAF_PTE
    );

    goto_priv(PRIV_M);
    ai_arch_amo_prepare_vs_guest_env();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    uint64_t good_old = amoadd_d(good_vaddr, addend);
    TEST_ASSERT("after that implicit-VS-page-table guest-faulting amoadd.d probe, a later valid VS-mode amoadd.d on the mapped guest page still succeeds and returns the original payload",
        excpt.triggered == false &&
        good_old == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the implicit-VS-page-table guest-faulting amoadd.d leaves the recovery backing word updated only by the later valid amoadd.d",
        read64(good_paddr) == (initial + addend)
    );
    ai_arch_amo_cleanup_vs_guest_env();

    TEST_END();
}
