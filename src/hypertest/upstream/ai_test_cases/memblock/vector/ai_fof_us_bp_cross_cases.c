#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <csrs.h>
#include <stdint.h>

#define AI_FUBP_FULL_VL 16U
#define AI_FUBP_M2_VL (AI_FUBP_FULL_VL * 2U)
#define AI_FUBP_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_FUBP_MCONTROL6_LOAD 0x1ULL
#define AI_FUBP_MCONTROL6_STORE 0x2ULL
#define AI_FUBP_MCONTROL6_AMO \
    (AI_FUBP_MCONTROL6_LOAD | AI_FUBP_MCONTROL6_STORE)
#define AI_FUBP_MCONTROL6_EXECUTE 0x4ULL
#define AI_FUBP_MCONTROL6_U 0x8ULL
#define AI_FUBP_MCONTROL6_S 0x10ULL
#define AI_FUBP_MCONTROL6_M 0x40ULL
#define AI_FUBP_MCONTROL6_ACTION_SHIFT 12U
#define AI_FUBP_MCONTROL6_MSU \
    (AI_FUBP_MCONTROL6_M | AI_FUBP_MCONTROL6_S | AI_FUBP_MCONTROL6_U)
#define AI_FUBP_MSTATUS_MPP_MASK (3ULL << 11)
#define AI_FUBP_MSTATUS_MPP_S (1ULL << 11)
#define AI_FUBP_MSTATUS_VS_MASK (3ULL << 9)
#define AI_FUBP_MSTATUS_VS_DIRTY (3ULL << 9)
#define AI_FUBP_SATP_MODE_SV48 (9ULL << 60)
#define AI_FUBP_SATP_ASID_SHIFT 44
#define AI_FUBP_SATP_ASID_MASK (0xffffULL << AI_FUBP_SATP_ASID_SHIFT)
#define AI_FUBP_SV48_POS_BOUNDARY (1ULL << 47)
#define AI_FUBP_VTYPE_VILL (1ULL << 63)
#define AI_FUBP_PMP_TOR_RWX 0x0fULL
#define AI_FUBP_PMP_TOR_NO_R 0x0eULL
#define AI_FUBP_PMP_TOR_NO_W 0x0dULL

static pte_t ai_fubp_sv48_high_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_fubp_sv48_high_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_fubp_sv48_high_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

static inline void ai_fubp_write_bytes(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline void ai_fubp_fill_bytes(uintptr_t paddr, uint8_t value,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, value);
    }
}

static inline bool ai_fubp_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline bool ai_fubp_split_bytes_match(uintptr_t low_paddr,
    uintptr_t high_paddr, unsigned split, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        uintptr_t paddr = (i < split) ? low_paddr + i :
            high_paddr + i - split;
        if (read8(paddr) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline bool ai_fubp_bytes_old_or_agnostic(uintptr_t paddr,
    const uint8_t *old, unsigned start, unsigned len)
{
    for (unsigned i = start; i < start + len; ++i) {
        uint8_t observed = read8(paddr + i);
        if (observed != old[i] && observed != 0xffU) {
            return false;
        }
    }
    return true;
}

static inline void ai_fubp_fill_pattern(uint8_t *bytes, unsigned len,
    uint8_t seed)
{
    for (unsigned i = 0; i < len; ++i) {
        bytes[i] = (uint8_t)(seed + i * 5U);
    }
}

static inline void ai_fubp_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0ULL;
    }
}

static inline pte_t ai_fubp_next_pt_pte(pte_t *pt)
{
    return PTE_V | (((uintptr_t)pt) >> 2);
}

static inline pte_t ai_fubp_leaf_pte(uintptr_t paddr, uint64_t flags)
{
    return (paddr >> 2) | flags;
}

static inline uintptr_t ai_fubp_sv48_last_page_vaddr(void)
{
    return AI_FUBP_SV48_POS_BOUNDARY - PAGE_SIZE;
}

static inline void ai_fubp_install_sv48_high_page(uintptr_t paddr,
    uint64_t flags)
{
    goto_priv(PRIV_M);
    hspt_init();
    ai_fubp_zero_pt(ai_fubp_sv48_high_l2);
    ai_fubp_zero_pt(ai_fubp_sv48_high_l1);
    ai_fubp_zero_pt(ai_fubp_sv48_high_l0);
    hspt[0][255] = ai_fubp_next_pt_pte(ai_fubp_sv48_high_l2);
    ai_fubp_sv48_high_l2[511] =
        ai_fubp_next_pt_pte(ai_fubp_sv48_high_l1);
    ai_fubp_sv48_high_l1[511] =
        ai_fubp_next_pt_pte(ai_fubp_sv48_high_l0);
    ai_fubp_sv48_high_l0[511] = ai_fubp_leaf_pte(paddr, flags);
    CSRW(satp, (((uintptr_t)hspt) >> 12) | AI_FUBP_SATP_MODE_SV48);
    sfence_vma();
}

static inline pte_t *ai_fubp_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_fubp_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ai_fubp_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_fubp_configure_pmp_window(uintptr_t start,
    uintptr_t end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, start >> 2U);
    CSRW(CSR_PMPADDR1, end >> 2U);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_FUBP_PMP_TOR_RWX | (deny_cfg << 8U) |
        (AI_FUBP_PMP_TOR_RWX << 16U));
}

static inline void ai_fubp_restore_pmp(uint64_t pmpcfg0,
    uint64_t pmpaddr0, uint64_t pmpaddr1, uint64_t pmpaddr2)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, pmpaddr0);
    CSRW(CSR_PMPADDR1, pmpaddr1);
    CSRW(CSR_PMPADDR2, pmpaddr2);
    CSRW(CSR_PMPCFG0, pmpcfg0);
}

static inline void ai_fubp_prepare_three_pages(enum test_page p0,
    uint64_t f0, enum test_page p1, uint64_t f1, enum test_page p2,
    uint64_t f2)
{
    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(p0, f0);
    ai_fubp_set_hs_leaf(p1, f1);
    ai_fubp_set_hs_leaf(p2, f2);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_fubp_cleanup_hs(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();
}

static inline void ai_fubp_set_satp_asid(uint64_t asid)
{
    goto_priv(PRIV_HS);
    uint64_t satp = CSRR(CSR_SATP);

    satp &= ~AI_FUBP_SATP_ASID_MASK;
    satp |= (asid << AI_FUBP_SATP_ASID_SHIFT) &
        AI_FUBP_SATP_ASID_MASK;
    CSRW(CSR_SATP, satp);
    sfence_vma();
}

static inline void ai_fubp_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, AI_FUBP_MSTATUS_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_fubp_disable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    CSRC(CSR_MSTATUS, AI_FUBP_MSTATUS_VS_MASK);
}

static inline void ai_fubp_set_vill_then_restore_e8(unsigned vl,
    uint64_t *vill_vtype, uint64_t *restored_vtype)
{
    uint64_t captured_vill = 0ULL;
    uint64_t captured_restored = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t1, -1\n\t"
        "vsetvl zero, zero, t1\n\t"
        "csrr %[out_vill], vtype\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrr %[out_restored], vtype\n\t"
        "csrw vstart, zero\n\t"
        ".option pop\n\t"
        : [out_vill] "=&r"(captured_vill),
          [out_restored] "=&r"(captured_restored)
        : [vl] "r"(vl)
        : "t0", "t1", "memory");

    *vill_vtype = captured_vill;
    *restored_vtype = captured_restored;
}

static inline void ai_fubp_clear_mcontrol6_slot(unsigned slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0ULL);
}

static inline void ai_fubp_arm_mcontrol6_slot(unsigned slot,
    uintptr_t addr, uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, AI_FUBP_MCONTROL6_MSU_BASE | access_bits);
}

static inline void ai_fubp_arm_mcontrol6_slot_priv(unsigned slot,
    uintptr_t addr, uint64_t access_bits, uint64_t priv_bits)
{
    uint64_t tdata1 = AI_FUBP_MCONTROL6_MSU_BASE | access_bits;

    tdata1 &= ~AI_FUBP_MCONTROL6_MSU;
    tdata1 |= priv_bits & AI_FUBP_MCONTROL6_MSU;

    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, tdata1);
}

static inline void ai_fubp_arm_mcontrol6_slot_action1_attempt(unsigned slot,
    uintptr_t addr, uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, AI_FUBP_MCONTROL6_MSU_BASE | access_bits |
        (1ULL << AI_FUBP_MCONTROL6_ACTION_SHIFT));
}

static inline uint64_t ai_fubp_mcontrol6_action(uint64_t tdata1)
{
    return (tdata1 >> AI_FUBP_MCONTROL6_ACTION_SHIFT) & 0xfULL;
}

static inline void ai_fubp_read_mcontrol6_slot(unsigned slot,
    uint64_t *tdata1, uint64_t *tdata2)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    if (tdata1 != NULL) {
        *tdata1 = CSRR(CSR_TDATA1);
    }
    if (tdata2 != NULL) {
        *tdata2 = CSRR(CSR_TDATA2);
    }
}

__attribute__((naked, noinline, aligned(4)))
static void ai_fubp_execute_marker_store_once(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd a1, 0(a0)\n\t"
        "ret\n\t"
        ".option pop\n\t");
}

static inline uint64_t ai_fubp_enable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
    return saved_mstatus;
}

static inline void ai_fubp_restore_breakpoint_delivery(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline void ai_fubp_set_mprv_s(uint64_t extra_bits)
{
    goto_priv(PRIV_M);
    uint64_t mstatus = CSRR(CSR_MSTATUS);

    mstatus &= ~(AI_FUBP_MSTATUS_MPP_MASK | MSTATUS_MPRV |
        MSTATUS_SUM | MSTATUS_MXR);
    mstatus |= MSTATUS_MPRV | AI_FUBP_MSTATUS_MPP_S | extra_bits;
    CSRW(CSR_MSTATUS, mstatus);
}

static inline uint64_t ai_fubp_lr_d(uintptr_t addr)
{
    uint64_t value = 0ULL;

    asm volatile(
        "lr.d %0, 0(%1)\n\t"
        : "=r"(value)
        : "r"(addr)
        : "memory");
    return value;
}

static inline uint64_t ai_fubp_sc_d(uintptr_t addr, uint64_t value)
{
    uint64_t status = ~0ULL;

    asm volatile(
        "sc.d %0, %2, 0(%1)\n\t"
        : "=&r"(status)
        : "r"(addr), "r"(value)
        : "memory");
    return status;
}

static inline void ai_fubp_vle8_to_v6(uintptr_t src_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vle8_current_to_v6(uintptr_t src_vaddr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vle8.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr)
        : "memory");
}

static inline void ai_fubp_mixed_rvc_setup_vle8_to_v6(uintptr_t old_base,
    uintptr_t current_base, unsigned old_vl, unsigned current_vl,
    unsigned *setup_marker)
{
    unsigned marker = 0U;

    asm volatile(
        ".option push\n\t"
        ".option rvc\n\t"
        "c.li %[mark], 1\n\t"
        "mv t1, %[old_base]\n\t"
        "mv t2, %[old_vl]\n\t"
        "c.addi %[mark], 2\n\t"
        "mv t1, %[current_base]\n\t"
        "mv t2, %[current_vl]\n\t"
        ".option norvc\n\t"
        "vsetvli t0, t2, e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v6, (t1)\n\t"
        ".option pop\n\t"
        : [mark] "=&r"(marker)
        : [old_base] "r"(old_base),
          [current_base] "r"(current_base),
          [old_vl] "r"(old_vl),
          [current_vl] "r"(current_vl)
        : "t0", "t1", "t2", "memory");

    *setup_marker = marker;
}

static inline void ai_fubp_vle8_to_v6_with_vstart(uintptr_t src_vaddr,
    unsigned vl, unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vle8.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [vl] "r"(vl), [start] "r"(start)
        : "t0", "memory");
}

static inline void ai_fubp_masked_vle8_to_v6(uintptr_t src_vaddr,
    uintptr_t mask_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v6, (%[src]), v0.t\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [mask] "r"(mask_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_load_mask_to_v0(uintptr_t mask_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlm.v v0, (%[mask])\n\t"
        ".option pop\n\t"
        :
        : [mask] "r"(mask_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vle8ff_v0_then_masked_vle8_to_v6(
    uintptr_t mask_src_vaddr, uintptr_t load_src_vaddr, unsigned req_vl,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v0, (%[mask_src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v6, (%[load_src]), v0.t\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [mask_src] "r"(mask_src_vaddr),
          [load_src] "r"(load_src_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vse8_from_v6(uintptr_t dst_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vse8.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vse8_from_v8(uintptr_t dst_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vse8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vector_spill_fill_scratch(uintptr_t src_vaddr,
    uintptr_t dst_vaddr, uintptr_t save_vaddr, unsigned half_vl,
    unsigned byte_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[half_vl], e16, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle16.v v12, (%[src])\n\t"
        "vse16.v v12, (%[dst])\n\t"
        "vsetvli t0, %[byte_vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v14, (%[save])\n\t"
        "vse8.v v14, (%[save])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [dst] "r"(dst_vaddr),
          [save] "r"(save_vaddr), [half_vl] "r"(half_vl),
          [byte_vl] "r"(byte_vl)
        : "t0", "memory");
}

static inline void ai_fubp_vlse8_to_v6(uintptr_t src_vaddr, intptr_t stride,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlse8.v v6, (%[src]), %[stride]\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [stride] "r"(stride), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vlseg2e8_to_v8(uintptr_t src_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e8.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vsseg2e8_from_v8(uintptr_t dst_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vsseg2e8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vle16_to_v6(uintptr_t src_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle16.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vse16_from_v6(uintptr_t dst_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vse16.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vle16_m4_to_v8(uintptr_t src_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m4, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle16.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_fubp_vse16_m4_from_v8(uintptr_t dst_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m4, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vse16.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline uint64_t ai_fubp_read_vlenb(void)
{
    uint64_t vlenb = 0ULL;

    asm volatile("csrr %0, vlenb" : "=r"(vlenb));
    return vlenb;
}

static inline void ai_fubp_vl1re8_v12_dump(uintptr_t src_vaddr,
    uintptr_t dump_vaddr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vl1re8.v v12, (%[src])\n\t"
        "vs1r.v v12, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [dump] "r"(dump_vaddr)
        : "memory");
}

static inline void ai_fubp_vle8ff_only(uintptr_t src_vaddr, unsigned req_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [vl] "r"(req_vl)
        : "t0", "memory");
}

static inline void ai_fubp_vle8ff_current_to_v8(uintptr_t src_vaddr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr)
        : "memory");
}

static inline void ai_fubp_vle16ff_m4_only(uintptr_t src_vaddr,
    unsigned req_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m4, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle16ff.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [vl] "r"(req_vl)
        : "t0", "memory");
}

static inline void ai_fubp_vle8ff_only_with_vstart(uintptr_t src_vaddr,
    unsigned req_vl, unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [vl] "r"(req_vl), [start] "r"(start)
        : "t0", "memory");
}

static inline void ai_fubp_vle16ff_only(uintptr_t src_vaddr, unsigned req_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle16ff.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr), [vl] "r"(req_vl)
        : "t0", "memory");
}

static inline void ai_fubp_vle8ff_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t dump_vaddr, unsigned req_vl,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vle8ff_active_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t dump_vaddr, unsigned active_vl,
    unsigned dump_vl, unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[dump_vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[active_vl], e8, m1, tu, mu\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[dump_vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [active_vl] "r"(active_vl),
          [dump_vl] "r"(dump_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vle8ff_seed_capture_no_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, unsigned req_vl, unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vle8ff_m2_group_dump(uintptr_t seed_vaddr,
    uintptr_t neighbor_seed_vaddr, uintptr_t src_vaddr, uintptr_t dump_vaddr,
    uintptr_t neighbor_dump_vaddr, unsigned req_vl, unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m2, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "vsetvli t0, %[vl], e8, m2, tu, mu\n\t"
        "vle8.v v10, (%[neighbor_seed])\n\t"
        "vsetvli t0, %[vl], e8, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m2, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "vsetvli t0, %[vl], e8, m2, tu, mu\n\t"
        "vse8.v v10, (%[neighbor_dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [neighbor_seed] "r"(neighbor_seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [neighbor_dump] "r"(neighbor_dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vle8ff_dump_with_v0_neighbor(
    uintptr_t seed_vaddr, uintptr_t mask_seed_vaddr,
    uintptr_t neighbor_seed_vaddr, uintptr_t src_vaddr,
    uintptr_t dump_vaddr, uintptr_t mask_dump_vaddr,
    uintptr_t neighbor_dump_vaddr, unsigned req_vl, unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "vle8.v v10, (%[neighbor_seed])\n\t"
        "vlm.v v0, (%[mask_seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "vse8.v v10, (%[neighbor_dump])\n\t"
        "vsm.v v0, (%[mask_dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [mask_seed] "r"(mask_seed_vaddr),
          [neighbor_seed] "r"(neighbor_seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [mask_dump] "r"(mask_dump_vaddr),
          [neighbor_dump] "r"(neighbor_dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vlseg2e8ff_full_dump(uintptr_t seed0_vaddr,
    uintptr_t seed1_vaddr, uintptr_t src_vaddr, uintptr_t dump0_vaddr,
    uintptr_t dump1_vaddr, unsigned req_vl, unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed0])\n\t"
        "vle8.v v9, (%[seed1])\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump0])\n\t"
        "vse8.v v9, (%[dump1])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed0] "r"(seed0_vaddr),
          [seed1] "r"(seed1_vaddr),
          [src] "r"(src_vaddr),
          [dump0] "r"(dump0_vaddr),
          [dump1] "r"(dump1_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vlseg2e8ff_full_dump_with_vstart(
    uintptr_t seed0_vaddr, uintptr_t seed1_vaddr, uintptr_t src_vaddr,
    uintptr_t dump0_vaddr, uintptr_t dump1_vaddr, unsigned req_vl,
    unsigned start, unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed0])\n\t"
        "vle8.v v9, (%[seed1])\n\t"
        "csrw vstart, %[start]\n\t"
        "vlseg2e8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump0])\n\t"
        "vse8.v v9, (%[dump1])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed0] "r"(seed0_vaddr),
          [seed1] "r"(seed1_vaddr),
          [src] "r"(src_vaddr),
          [dump0] "r"(dump0_vaddr),
          [dump1] "r"(dump1_vaddr),
          [vl] "r"(req_vl),
          [start] "r"(start)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vle8ff_full_dump_with_vstart(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t dump_vaddr, unsigned req_vl,
    unsigned start, unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, %[start]\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl),
          [start] "r"(start)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vle8ff_csrr_vl_then_vset_us(
    uintptr_t seed_vaddr, uintptr_t fof_src_vaddr, uintptr_t us_src_vaddr,
    unsigned fof_vl, unsigned us_vl, unsigned *fof_observed_vl,
    unsigned *fof_observed_vstart, unsigned *post_vset_vl,
    unsigned *post_us_vl, unsigned *post_us_vstart)
{
    unsigned captured_fof_vl = 0U;
    unsigned captured_fof_vstart = 0U;
    unsigned captured_post_vset_vl = 0U;
    unsigned captured_post_us_vl = 0U;
    unsigned captured_post_us_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[fof_vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[fof_src])\n\t"
        "csrr %[out_fof_vl], vl\n\t"
        "csrr %[out_fof_vstart], vstart\n\t"
        "vsetvli t0, %[us_vl], e8, m1, tu, mu\n\t"
        "csrr %[out_post_vset_vl], vl\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v6, (%[us_src])\n\t"
        "csrr %[out_post_us_vl], vl\n\t"
        "csrr %[out_post_us_vstart], vstart\n\t"
        ".option pop\n\t"
        : [out_fof_vl] "=&r"(captured_fof_vl),
          [out_fof_vstart] "=&r"(captured_fof_vstart),
          [out_post_vset_vl] "=&r"(captured_post_vset_vl),
          [out_post_us_vl] "=&r"(captured_post_us_vl),
          [out_post_us_vstart] "=&r"(captured_post_us_vstart)
        : [seed] "r"(seed_vaddr),
          [fof_src] "r"(fof_src_vaddr),
          [us_src] "r"(us_src_vaddr),
          [fof_vl] "r"(fof_vl),
          [us_vl] "r"(us_vl)
        : "t0", "memory");

    *fof_observed_vl = captured_fof_vl;
    *fof_observed_vstart = captured_fof_vstart;
    *post_vset_vl = captured_post_vset_vl;
    *post_us_vl = captured_post_us_vl;
    *post_us_vstart = captured_post_us_vstart;
}

static inline void ai_fubp_vle16ff_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t dump_vaddr, unsigned req_vl,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "vle16.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle16ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "vse16.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_vle16ff_m4_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t dump_vaddr, unsigned req_vl,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m4, tu, mu\n\t"
        "vle16.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle16ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e16, m4, tu, mu\n\t"
        "vse16.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_masked_vle8ff_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t mask_vaddr, uintptr_t dump_vaddr,
    unsigned req_vl, unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [mask] "r"(mask_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_masked_vle8ff_tail_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t mask_vaddr, uintptr_t dump_vaddr,
    unsigned req_vl, unsigned full_vl, bool agnostic,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    if (agnostic) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
            "vle8.v v8, (%[seed])\n\t"
            "vsetvli t0, %[req_vl], e8, m1, ta, ma\n\t"
            "vlm.v v0, (%[mask])\n\t"
            "csrw vstart, zero\n\t"
            "vle8ff.v v8, (%[src]), v0.t\n\t"
            "csrr %[out_vl], vl\n\t"
            "csrr %[out_vstart], vstart\n\t"
            "csrw vstart, zero\n\t"
            "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
            "vse8.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r"(captured_vl),
              [out_vstart] "=&r"(captured_vstart)
            : [seed] "r"(seed_vaddr),
              [src] "r"(src_vaddr),
              [mask] "r"(mask_vaddr),
              [dump] "r"(dump_vaddr),
              [req_vl] "r"(req_vl),
              [full_vl] "r"(full_vl)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
            "vle8.v v8, (%[seed])\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
            "vlm.v v0, (%[mask])\n\t"
            "csrw vstart, zero\n\t"
            "vle8ff.v v8, (%[src]), v0.t\n\t"
            "csrr %[out_vl], vl\n\t"
            "csrr %[out_vstart], vstart\n\t"
            "csrw vstart, zero\n\t"
            "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
            "vse8.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r"(captured_vl),
              [out_vstart] "=&r"(captured_vstart)
            : [seed] "r"(seed_vaddr),
              [src] "r"(src_vaddr),
              [mask] "r"(mask_vaddr),
              [dump] "r"(dump_vaddr),
              [req_vl] "r"(req_vl),
              [full_vl] "r"(full_vl)
            : "t0", "memory");
    }

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_masked_vle8ff_only(uintptr_t src_vaddr,
    uintptr_t mask_vaddr, unsigned req_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src]), v0.t\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr),
          [mask] "r"(mask_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");
}

static inline void ai_fubp_branch_wrong_us_right_vfof(uintptr_t wrong_src,
    uintptr_t seed_vaddr, uintptr_t right_src, uintptr_t dump_vaddr,
    unsigned req_vl, unsigned take_right, unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li %[out_vl], 0\n\t"
        "li %[out_vstart], 0\n\t"
        "bnez %[take], 2f\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v6, (%[wrong])\n\t"
        "j 3f\n\t"
        "2:\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[right])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        "3:\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [wrong] "r"(wrong_src),
          [seed] "r"(seed_vaddr),
          [right] "r"(right_src),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl),
          [take] "r"(take_right)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_fubp_branch_wrong_vfof_right_us(uintptr_t wrong_src,
    uintptr_t right_src, uintptr_t dump_vaddr, unsigned req_vl,
    unsigned take_right)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "bnez %[take], 2f\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[wrong])\n\t"
        "j 3f\n\t"
        "2:\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v6, (%[right])\n\t"
        "csrw vstart, zero\n\t"
        "vse8.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        "3:\n\t"
        ".option pop\n\t"
        :
        : [wrong] "r"(wrong_src),
          [right] "r"(right_src),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl),
          [take] "r"(take_right)
        : "t0", "memory");
}

static inline void ai_fubp_branch_wrong_store_right_vfof(uintptr_t wrong_src,
    uintptr_t wrong_dst, uintptr_t seed_vaddr, uintptr_t right_src,
    uintptr_t dump_vaddr, unsigned req_vl, unsigned take_right,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li %[out_vl], 0\n\t"
        "li %[out_vstart], 0\n\t"
        "bnez %[take], 2f\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v6, (%[wrong_src])\n\t"
        "vse8.v v6, (%[wrong_dst])\n\t"
        "fence rw, rw\n\t"
        "j 3f\n\t"
        "2:\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[right])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        "3:\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [wrong_src] "r"(wrong_src),
          [wrong_dst] "r"(wrong_dst),
          [seed] "r"(seed_vaddr),
          [right] "r"(right_src),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl),
          [take] "r"(take_right)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

bool ai_micro_fof_us_bp_quarantine_then_refresh_manual_corner()
{
    TEST_START();

    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned us0_vl = 12U;
    const unsigned us0_trigger_index = 4U;
    const unsigned fof_fault_index = 5U;
    const unsigned us1_vl = 12U;
    const unsigned us1_trigger_index = 10U;
    const uintptr_t us0_data_vaddr = hs_page_base(data_page) + 0x100U;
    const uintptr_t us0_old_vaddr = hs_page_base(old_page) + 0x200U;
    const uintptr_t us0_dump_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t us1_data_vaddr = hs_page_base(data_page) + 0x500U;
    const uintptr_t us1_old_vaddr = hs_page_base(old_page) + 0x600U;
    const uintptr_t us1_dump_vaddr = hs_page_base(aux_page) + 0x700U;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x400U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x440U;
    const uintptr_t us0_data_paddr = phys_page_base(data_page) + 0x100U;
    const uintptr_t us0_old_paddr = phys_page_base(old_page) + 0x200U;
    const uintptr_t us0_dump_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t us1_data_paddr = phys_page_base(data_page) + 0x500U;
    const uintptr_t us1_old_paddr = phys_page_base(old_page) + 0x600U;
    const uintptr_t us1_dump_paddr = phys_page_base(aux_page) + 0x700U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x400U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x440U;
    uint8_t us0_data[16] = { 0 };
    uint8_t us0_old[16] = { 0 };
    uint8_t us0_expected[16] = { 0 };
    uint8_t us1_data[16] = { 0 };
    uint8_t us1_old[16] = { 0 };
    uint8_t us1_expected[16] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_full_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us0_data, us0_vl, 0x20U);
    ai_fubp_fill_pattern(us0_old, us0_vl, 0xa0U);
    ai_fubp_fill_pattern(us1_data, us1_vl, 0x50U);
    ai_fubp_fill_pattern(us1_old, us1_vl, 0xc0U);
    ai_fubp_fill_pattern(fof_seed, AI_FUBP_FULL_VL, 0xe0U);
    for (unsigned i = 0; i < us0_vl; ++i) {
        us0_expected[i] = (i < us0_trigger_index) ? us0_data[i] :
            us0_old[i];
    }
    for (unsigned i = 0; i < us1_vl; ++i) {
        us1_expected[i] = (i < us1_trigger_index) ? us1_data[i] :
            us1_old[i];
    }
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        fof_full_expected[i] = (uint8_t)(0x70U + i * 7U);
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us0_data_paddr, us0_data, us0_vl);
    ai_fubp_write_bytes(us0_old_paddr, us0_old, us0_vl);
    ai_fubp_write_bytes(us0_dump_paddr, us0_old, us0_vl);
    ai_fubp_write_bytes(us1_data_paddr, us1_data, us1_vl);
    ai_fubp_write_bytes(us1_old_paddr, us1_old, us1_vl);
    ai_fubp_write_bytes(us1_dump_paddr, us1_old, us1_vl);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x6dU, AI_FUBP_FULL_VL);
    ai_fubp_write_bytes(low_paddr, fof_full_expected, fof_fault_index);
    ai_fubp_write_bytes(high_paddr, fof_full_expected + fof_fault_index,
        AI_FUBP_FULL_VL - fof_fault_index);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us0_old_vaddr, us0_vl);
    TEST_ASSERT("US->FOF quarantine setup preloads old v6 data before the first unit-stride breakpoint episode",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U,
        us0_data_vaddr + us0_trigger_index, AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us0_data_vaddr, us0_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("US->FOF quarantine first episode reports the current US load breakpoint full VA and absolute vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us0_data_vaddr + us0_trigger_index &&
        observed_vstart_csr == us0_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us0_data_vaddr + us0_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us0_dump_vaddr, us0_vl);
    TEST_ASSERT("US->FOF quarantine first episode can dump v6 after clearing trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("US->FOF quarantine first episode leaves only the pre-breakpoint load prefix visible in v6",
        ai_fubp_bytes_match(us0_dump_paddr, us0_expected, us0_vl)
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr, fof_dump_vaddr,
        AI_FUBP_FULL_VL, &observed_vl, &observed_vstart);
    TEST_ASSERT("US->FOF quarantine second episode does not inherit the previous US trigger mask and silently shortens at the current VFOF fault index",
        excpt.triggered == false &&
        observed_vl == fof_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_full_expected,
            fof_fault_index) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_index,
            fof_seed + fof_fault_index, AI_FUBP_FULL_VL - fof_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_fill_bytes(fof_dump_paddr, 0x87U, AI_FUBP_FULL_VL);
    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr, fof_dump_vaddr,
        AI_FUBP_FULL_VL, &observed_vl, &observed_vstart);
    TEST_ASSERT("US->FOF quarantine repair reload clears the old shortened-vl resident state and reloads the whole VFOF payload",
        excpt.triggered == false &&
        observed_vl == AI_FUBP_FULL_VL &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_full_expected,
            AI_FUBP_FULL_VL),
        "vl=%u vstart=%u lane5=0x%x", observed_vl, observed_vstart,
        read8(fof_dump_paddr + fof_fault_index)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us1_old_vaddr, us1_vl);
    TEST_ASSERT("US->FOF quarantine third episode reloads v6 before checking a US breakpoint beyond the old shortened-vl bound",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U,
        us1_data_vaddr + us1_trigger_index, AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us1_data_vaddr, us1_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("US->FOF quarantine third episode refreshes trigger full VA/vstart instead of clipping at the previous VFOF shortened vl",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us1_data_vaddr + us1_trigger_index &&
        observed_vstart_csr == us1_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us1_data_vaddr + us1_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us1_dump_vaddr, us1_vl);
    TEST_ASSERT("US->FOF quarantine third episode can dump the current US prefix after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("US->FOF quarantine third episode keeps the current US prefix and never reuses old VFOF prefix/suffix metadata",
        ai_fubp_bytes_match(us1_dump_paddr, us1_expected, us1_vl) &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_full_expected,
            AI_FUBP_FULL_VL)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_fof_masked_off_trigger_active_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned masked_trigger_index = 2U;
    const unsigned first_active_fault_index = 5U;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        first_active_fault_index;
    const uintptr_t masked_trigger_vaddr = src_vaddr + masked_trigger_index;
    const uintptr_t active_trigger_vaddr = src_vaddr + 1U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xc00U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0xc40U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xc80U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        first_active_fault_index;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xc00U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0xc40U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xc80U;
    const uint8_t mask_bytes[2] = { 0x3bU, 0x00U };
    uint8_t prefix[8] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(prefix, first_active_fault_index, 0x40U);
    ai_fubp_fill_pattern(seed, AI_FUBP_FULL_VL, 0xb0U);

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, prefix, first_active_fault_index);
    ai_fubp_write_bytes(seed_paddr, seed, AI_FUBP_FULL_VL);
    ai_fubp_write_bytes(mask_paddr, mask_bytes, sizeof(mask_bytes));
    ai_fubp_fill_bytes(dump_paddr, 0x69U, AI_FUBP_FULL_VL);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, masked_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_full_dump(seed_vaddr, src_vaddr, mask_vaddr,
        dump_vaddr, req_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("masked VFOF ignores an address trigger match on an inactive lane and still shortens at the later active fault lane",
        excpt.triggered == false &&
        observed_vl == first_active_fault_index &&
        observed_vstart == 0U &&
        read8(dump_paddr) == prefix[0] &&
        read8(dump_paddr + 1U) == prefix[1] &&
        read8(dump_paddr + masked_trigger_index) ==
            seed[masked_trigger_index] &&
        read8(dump_paddr + 3U) == prefix[3] &&
        read8(dump_paddr + 4U) == prefix[4] &&
        ai_fubp_bytes_match(dump_paddr + first_active_fault_index,
            seed + first_active_fault_index,
            req_vl - first_active_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane2=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(dump_paddr + masked_trigger_index)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, active_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_full_dump(seed_vaddr, src_vaddr, mask_vaddr,
        dump_vaddr, req_vl, &observed_vl, &observed_vstart);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("masked VFOF control arm proves the same mask/load path reports a breakpoint when the matched lane is active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == active_trigger_vaddr &&
        observed_vstart_csr == 1U,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, active_trigger_vaddr,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_fof_element0_same_address_bp_priority_manual_corner()
{
    TEST_START();

    const enum test_page fault_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(fault_page) + 0x180U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xd00U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xd40U;
    const uintptr_t src_paddr = phys_page_base(fault_page) + 0x180U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xd00U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xd40U;
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(seed, AI_FUBP_FULL_VL, 0x90U);

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(src_paddr, seed, AI_FUBP_FULL_VL);
    ai_fubp_write_bytes(seed_paddr, seed, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(dump_paddr, 0x55U, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, src_vaddr, AI_FUBP_MCONTROL6_LOAD);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(src_vaddr, AI_FUBP_FULL_VL);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("element0 VFOF same-address breakpoint/fault reports one visible event and does not silently convert element0 into shortened vl",
        excpt.triggered == true &&
        (excpt.cause == CAUSE_BKP || excpt.cause == CAUSE_LPF) &&
        excpt.tval == src_vaddr &&
        observed_vstart_csr == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, src_vaddr, observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(src_vaddr, AI_FUBP_FULL_VL);
    TEST_ASSERT("element0 VFOF retry after clearing trigger exposes the real page fault at element0 with vstart still zero",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == src_vaddr &&
        CSRR(CSR_VSTART) == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, src_vaddr, CSRR(CSR_VSTART)
    );

    ai_fubp_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    ai_fubp_fill_bytes(dump_paddr, 0xa5U, AI_FUBP_FULL_VL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        AI_FUBP_FULL_VL, &observed_vl, &observed_vstart);
    TEST_ASSERT("element0 VFOF repair completes the whole load and proves no stale element0 breakpoint/fault metadata remains resident",
        excpt.triggered == false &&
        observed_vl == AI_FUBP_FULL_VL &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, seed, AI_FUBP_FULL_VL),
        "triggered=%d cause=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_us_nosplit_then_fof_second_flow_fault_manual_corner()
{
    TEST_START();

    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned us_vl = 8U;
    const unsigned fof_fault_index = 10U;
    const uintptr_t data_vaddr = hs_page_base(data_page) + 0x80U;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x80U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x80U;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t data_paddr = phys_page_base(data_page) + 0x80U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x80U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x80U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x140U;
    uint8_t data[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_fubp_fill_pattern(data, us_vl, 0x24U);
    ai_fubp_fill_pattern(old, AI_FUBP_FULL_VL, 0xb4U);
    ai_fubp_fill_pattern(fof_payload, AI_FUBP_FULL_VL, 0x48U);
    ai_fubp_fill_pattern(fof_seed, AI_FUBP_FULL_VL, 0xd8U);
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        us_expected[i] = (i < us_vl) ? data[i] : old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_write_bytes(data_paddr, data, us_vl);
    ai_fubp_write_bytes(old_paddr, old, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(us_dump_paddr, 0x5eU, AI_FUBP_FULL_VL);
    ai_fubp_write_bytes(low_paddr, fof_payload, fof_fault_index);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x9dU, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, AI_FUBP_FULL_VL);
    TEST_ASSERT("US no-split setup preloads all v6 lanes before the short aligned unit-stride load",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(data_vaddr, us_vl);
    TEST_ASSERT("US no-split aligned vl8 load completes without creating a ghost second flow",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, AI_FUBP_FULL_VL);
    TEST_ASSERT("US no-split dump completes so both active prefix and inactive suffix can be checked",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("US no-split vl8 load only updates the requested lanes and leaves lanes 8..15 with their old sentinel",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, AI_FUBP_FULL_VL)
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr, fof_dump_vaddr,
        AI_FUBP_FULL_VL, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("US no-split stale usSecondInv must not suppress the following VFOF second-flow page fault at element10",
        excpt.triggered == false &&
        observed_vl == fof_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload,
            fof_fault_index) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_index,
            fof_seed + fof_fault_index, AI_FUBP_FULL_VL - fof_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_us_store_bp_cutoff_then_fof_flag_isolation_manual_corner()
{
    TEST_START();

    const enum test_page store_page = VSRW_GRWX;
    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned store_vl = 10U;
    const unsigned store_trigger_index = 6U;
    const unsigned fof_fault_index = 7U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0xe00U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xe40U;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0xe80U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xec0U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0xe00U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xe40U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0xe80U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xec0U;
    uint8_t initial[16] = { 0 };
    uint8_t payload[16] = { 0 };
    uint8_t store_expected[16] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(initial, store_vl, 0x32U);
    ai_fubp_fill_pattern(payload, store_vl, 0xa2U);
    ai_fubp_fill_pattern(fof_payload, AI_FUBP_FULL_VL, 0x52U);
    ai_fubp_fill_pattern(fof_seed, AI_FUBP_FULL_VL, 0xc2U);
    for (unsigned i = 0; i < store_vl; ++i) {
        store_expected[i] = (i < store_trigger_index) ? payload[i] :
            initial[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(store_paddr, initial, store_vl);
    ai_fubp_write_bytes(src_paddr, payload, store_vl);
    ai_fubp_write_bytes(low_paddr, fof_payload, fof_fault_index);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x4bU, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, store_vl);
    TEST_ASSERT("store breakpoint cutoff setup loads a distinct source payload before arming the store trigger",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, store_vaddr + store_trigger_index,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, store_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("store breakpoint cutoff reports the current store lane and stops before the trigger lane side effect",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_vaddr + store_trigger_index &&
        observed_vstart_csr == store_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, store_vaddr + store_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("store breakpoint cutoff commits only the prefix once and leaves trigger/suffix bytes with old data",
        ai_fubp_bytes_match(store_paddr, store_expected, store_vl)
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr, fof_dump_vaddr,
        AI_FUBP_FULL_VL, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("store-side breakpoint metadata must not make the next VFOF inherit vecExceptionFlag/finishFlowMask or lose its active prefix",
        excpt.triggered == false &&
        observed_vl == fof_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload,
            fof_fault_index) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_index,
            fof_seed + fof_fault_index, AI_FUBP_FULL_VL - fof_fault_index) &&
        ai_fubp_bytes_match(store_paddr, store_expected, store_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_us_bp_clear_rearm_then_fof_same_va_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page e16_low_page = VSRWX_GRWX;
    const enum test_page e16_high_page = VSRWX_GRW;
    const unsigned e8_req_vl = AI_FUBP_FULL_VL;
    const unsigned e8_us_vl = 12U;
    const unsigned e8_us_trigger_index = 3U;
    const unsigned e8_fof_trigger_index = 8U;
    const unsigned e8_fault_index = 12U;
    const unsigned e16_req_vl = 8U;
    const unsigned e16_us_vl = 8U;
    const unsigned e16_us_trigger_index = 1U;
    const unsigned e16_fof_trigger_index = 4U;
    const unsigned e16_fault_index = 6U;
    const uintptr_t e8_base_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        e8_fault_index;
    const uintptr_t e16_base_vaddr = hs_page_base(e16_low_page) + PAGE_SIZE -
        e16_fault_index * 2U;
    const uintptr_t e8_us_old_vaddr = hs_page_base(aux_page) + 0x180U;
    const uintptr_t e8_us_dump_vaddr = hs_page_base(aux_page) + 0x1c0U;
    const uintptr_t e8_fof_seed_vaddr = hs_page_base(aux_page) + 0x200U;
    const uintptr_t e8_fof_dump_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t e16_us_old_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t e16_us_dump_vaddr = hs_page_base(aux_page) + 0x2c0U;
    const uintptr_t e16_fof_seed_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t e16_fof_dump_vaddr = hs_page_base(aux_page) + 0x340U;
    const uintptr_t e8_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        e8_fault_index;
    const uintptr_t e16_low_paddr = phys_page_base(e16_low_page) + PAGE_SIZE -
        e16_fault_index * 2U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t e16_high_paddr = phys_page_base(e16_high_page);
    const uintptr_t e8_us_old_paddr = phys_page_base(aux_page) + 0x180U;
    const uintptr_t e8_us_dump_paddr = phys_page_base(aux_page) + 0x1c0U;
    const uintptr_t e8_fof_seed_paddr = phys_page_base(aux_page) + 0x200U;
    const uintptr_t e8_fof_dump_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t e16_us_old_paddr = phys_page_base(aux_page) + 0x280U;
    const uintptr_t e16_us_dump_paddr = phys_page_base(aux_page) + 0x2c0U;
    const uintptr_t e16_fof_seed_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t e16_fof_dump_paddr = phys_page_base(aux_page) + 0x340U;
    uint8_t e8_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e8_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e8_prefix_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e8_fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e16_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e16_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e16_prefix_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e16_fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(e8_payload, sizeof(e8_payload), 0x18U);
    ai_fubp_fill_pattern(e8_old, sizeof(e8_old), 0x88U);
    ai_fubp_fill_pattern(e8_fof_seed, sizeof(e8_fof_seed), 0xc8U);
    ai_fubp_fill_pattern(e16_payload, sizeof(e16_payload), 0x31U);
    ai_fubp_fill_pattern(e16_old, sizeof(e16_old), 0xa1U);
    ai_fubp_fill_pattern(e16_fof_seed, sizeof(e16_fof_seed), 0xd1U);
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        e8_prefix_expected[i] = (i < e8_us_trigger_index) ?
            e8_payload[i] : e8_old[i];
        e16_prefix_expected[i] = (i < e16_us_trigger_index * 2U) ?
            e16_payload[i] : e16_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(e8_low_paddr, e8_payload, e8_fault_index);
    ai_fubp_write_bytes(high_paddr, e8_payload + e8_fault_index,
        e8_req_vl - e8_fault_index);
    ai_fubp_write_bytes(e8_us_old_paddr, e8_old, e8_us_vl);
    ai_fubp_fill_bytes(e8_us_dump_paddr, 0x4eU, e8_req_vl);
    ai_fubp_write_bytes(e8_fof_seed_paddr, e8_fof_seed, e8_req_vl);
    ai_fubp_fill_bytes(e8_fof_dump_paddr, 0x7eU, e8_req_vl);
    ai_fubp_write_bytes(e16_low_paddr, e16_payload, e16_fault_index * 2U);
    ai_fubp_write_bytes(e16_high_paddr, e16_payload + e16_fault_index * 2U,
        (e16_req_vl - e16_fault_index) * 2U);
    ai_fubp_write_bytes(e16_us_old_paddr, e16_old, e16_us_vl * 2U);
    ai_fubp_fill_bytes(e16_us_dump_paddr, 0x5fU, e16_req_vl * 2U);
    ai_fubp_write_bytes(e16_fof_seed_paddr, e16_fof_seed, e16_req_vl * 2U);
    ai_fubp_fill_bytes(e16_fof_dump_paddr, 0x8fU, e16_req_vl * 2U);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(e8_us_old_vaddr, e8_us_vl);
    TEST_ASSERT("e8 clear/rearm setup preloads old v6 bytes before the first US breakpoint round",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U,
        e8_base_vaddr + e8_us_trigger_index, AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(e8_base_vaddr, e8_us_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e8 round first reports the US lane breakpoint before any FOF metadata exists",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e8_base_vaddr + e8_us_trigger_index &&
        observed_vstart_csr == e8_us_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, e8_base_vaddr + e8_us_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(e8_us_dump_vaddr, e8_us_vl);
    TEST_ASSERT("e8 US breakpoint leaves only the pre-trigger load prefix before handler clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("e8 US breakpoint prefix uses the current lane index and not stale FOF state",
        ai_fubp_bytes_match(e8_us_dump_paddr, e8_prefix_expected, e8_us_vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(e8_base_vaddr, e8_us_vl);
    TEST_ASSERT("e8 same US load completes after the trigger is cleared",
        excpt.triggered == false
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_arm_mcontrol6_slot(0U,
        e8_base_vaddr + e8_fof_trigger_index, AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(e8_base_vaddr, e8_req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e8 rearm reports the new VFOF lane breakpoint instead of the old US lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e8_base_vaddr + e8_fof_trigger_index &&
        observed_vstart_csr == e8_fof_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, e8_base_vaddr + e8_fof_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(e8_fof_seed_vaddr, e8_base_vaddr,
        e8_fof_dump_vaddr, e8_req_vl, &observed_vl, &observed_vstart);
    goto_priv(PRIV_M);
    TEST_ASSERT("e8 trigger clear exposes the current VFOF nonzero first fault without reusing the US breakpoint lane",
        excpt.triggered == false &&
        observed_vl == e8_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(e8_fof_dump_paddr, e8_payload,
            e8_fault_index) &&
        ai_fubp_bytes_match(e8_fof_dump_paddr + e8_fault_index,
            e8_fof_seed + e8_fault_index, e8_req_vl - e8_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_prepare_three_pages(e16_low_page, PTE_V | PTE_AD | PTE_RWX,
        e16_high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(e16_us_old_vaddr, e16_us_vl);
    TEST_ASSERT("e16 second clear/rearm round preloads old halfword lanes before changing EEW and lane index",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U,
        e16_base_vaddr + e16_us_trigger_index * 2U,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(e16_base_vaddr, e16_us_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e16 second round reports the new US halfword lane after the e8 VFOF round",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e16_base_vaddr + e16_us_trigger_index * 2U &&
        observed_vstart_csr == e16_us_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval,
        e16_base_vaddr + e16_us_trigger_index * 2U,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_from_v6(e16_us_dump_vaddr, e16_us_vl);
    TEST_ASSERT("e16 US breakpoint leaves only the current halfword prefix before retry",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("e16 US prefix uses byte-accurate halfword trigger cutoff and not the prior e8 mask",
        ai_fubp_bytes_match(e16_us_dump_paddr, e16_prefix_expected,
            e16_us_vl * 2U)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(e16_base_vaddr, e16_us_vl);
    TEST_ASSERT("e16 same US load completes after clearing the second-round trigger",
        excpt.triggered == false
    );

    ai_fubp_prepare_three_pages(e16_low_page, PTE_V | PTE_AD | PTE_RWX,
        e16_high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_arm_mcontrol6_slot(0U,
        e16_base_vaddr + e16_fof_trigger_index * 2U,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_only(e16_base_vaddr, e16_req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e16 rearm reports the halfword VFOF breakpoint at the new address and lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e16_base_vaddr + e16_fof_trigger_index * 2U &&
        observed_vstart_csr == e16_fof_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval,
        e16_base_vaddr + e16_fof_trigger_index * 2U,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_full_dump(e16_fof_seed_vaddr, e16_base_vaddr,
        e16_fof_dump_vaddr, e16_req_vl, &observed_vl, &observed_vstart);
    goto_priv(PRIV_M);
    TEST_ASSERT("e16 trigger clear exposes the second-round nonzero first fault with no stale e8 trigger state",
        excpt.triggered == false &&
        observed_vl == e16_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(e16_fof_dump_paddr, e16_payload,
            e16_fault_index * 2U) &&
        ai_fubp_bytes_match(e16_fof_dump_paddr + e16_fault_index * 2U,
            e16_fof_seed + e16_fault_index * 2U,
            (e16_req_vl - e16_fault_index) * 2U),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_nonzero_vstart_us_then_fof_bp_absolute_index_manual_corner()
{
    TEST_START();

    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned us_vl = 12U;
    const unsigned us_start = 3U;
    const unsigned us_trigger_index = 7U;
    const unsigned fof_req_vl = AI_FUBP_FULL_VL;
    const unsigned fof_start = 2U;
    const unsigned fof_trigger_index = 5U;
    const unsigned fof_fault_index = 9U;
    const uintptr_t us_src_vaddr = hs_page_base(us_page) + 0x240U;
    const uintptr_t us_old_vaddr = hs_page_base(old_page) + 0x2c0U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x360U;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x3c0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x400U;
    const uintptr_t us_src_paddr = phys_page_base(us_page) + 0x240U;
    const uintptr_t us_old_paddr = phys_page_base(old_page) + 0x2c0U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x360U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x3c0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x400U;
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, sizeof(us_payload), 0x26U);
    ai_fubp_fill_pattern(us_old, sizeof(us_old), 0xa6U);
    ai_fubp_fill_pattern(fof_payload, sizeof(fof_payload), 0x56U);
    ai_fubp_fill_pattern(fof_seed, sizeof(fof_seed), 0xd6U);
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        us_expected[i] = (i >= us_start && i < us_trigger_index) ?
            us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_src_paddr, us_payload, us_vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, us_vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x47U, us_vl);
    ai_fubp_write_bytes(low_paddr, fof_payload, fof_fault_index);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, fof_req_vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x79U, fof_req_vl);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, us_vl);
    TEST_ASSERT("nonzero-vstart US setup preloads old v6 bytes before the absolute-index breakpoint probe",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_trigger_index,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6_with_vstart(us_src_vaddr, us_vl, us_start);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("nonzero-vstart US breakpoint reports the absolute architectural element index and full VA, not the flow-relative index",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_trigger_index &&
        observed_vstart_csr == us_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + us_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, us_vl);
    TEST_ASSERT("nonzero-vstart US dump completes after the breakpoint is cleared",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("nonzero-vstart US load updates only lanes from vstart through the pre-breakpoint prefix",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, us_vl)
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_arm_mcontrol6_slot(0U,
        fof_src_vaddr + fof_trigger_index, AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only_with_vstart(fof_src_vaddr, fof_req_vl, fof_start);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("nonzero-vstart VFOF breakpoint uses the absolute lane before the later first-fault lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_src_vaddr + fof_trigger_index &&
        observed_vstart_csr == fof_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fof_src_vaddr + fof_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump_with_vstart(fof_seed_vaddr, fof_src_vaddr,
        fof_dump_vaddr, fof_req_vl, fof_start, &observed_vl,
        &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("nonzero-vstart VFOF retry preserves skipped prefix lanes and shortens vl to the absolute first-fault index",
        excpt.triggered == false &&
        observed_vl == fof_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_seed, fof_start) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_start,
            fof_payload + fof_start, fof_fault_index - fof_start) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_index,
            fof_seed + fof_fault_index, fof_req_vl - fof_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane2=0x%x lane9=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(fof_dump_paddr + fof_start),
        read8(fof_dump_paddr + fof_fault_index)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_mask_hole_us_fof_active_only_oldest_manual_corner()
{
    TEST_START();

    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned inactive_hole_index = 2U;
    const unsigned active_trigger_index = 6U;
    const unsigned active_fault_index = 8U;
    const uintptr_t us_src_vaddr = hs_page_base(us_page) + 0x5a0U;
    const uintptr_t us_old_vaddr = hs_page_base(old_page) + 0x620U;
    const uintptr_t us_mask_vaddr = hs_page_base(aux_page) + 0x6a0U;
    const uintptr_t us_mask2_vaddr = hs_page_base(aux_page) + 0x6a8U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x6c0U;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        active_fault_index;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x700U;
    const uintptr_t fof_mask_vaddr = hs_page_base(aux_page) + 0x740U;
    const uintptr_t fof_mask2_vaddr = hs_page_base(aux_page) + 0x748U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x780U;
    const uintptr_t us_src_paddr = phys_page_base(us_page) + 0x5a0U;
    const uintptr_t us_old_paddr = phys_page_base(old_page) + 0x620U;
    const uintptr_t us_mask_paddr = phys_page_base(aux_page) + 0x6a0U;
    const uintptr_t us_mask2_paddr = phys_page_base(aux_page) + 0x6a8U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x6c0U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        active_fault_index;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x700U;
    const uintptr_t fof_mask_paddr = phys_page_base(aux_page) + 0x740U;
    const uintptr_t fof_mask2_paddr = phys_page_base(aux_page) + 0x748U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x780U;
    const uint8_t mask_hole[2] = { 0x7bU, 0x00U };
    const uint8_t mask_lane2_active[2] = { 0x7fU, 0x00U };
    const uint8_t fof_mask_hole[2] = { 0xfbU, 0x01U };
    const uint8_t fof_mask_lane2_active[2] = { 0xffU, 0x01U };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, sizeof(us_payload), 0x38U);
    ai_fubp_fill_pattern(us_old, sizeof(us_old), 0xb8U);
    ai_fubp_fill_pattern(fof_payload, sizeof(fof_payload), 0x68U);
    ai_fubp_fill_pattern(fof_seed, sizeof(fof_seed), 0xe8U);
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        const bool active_before_trigger =
            (i != inactive_hole_index) && (i < active_trigger_index);
        us_expected[i] = active_before_trigger ? us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_src_paddr, us_payload, vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, vl);
    ai_fubp_write_bytes(us_mask_paddr, mask_hole, sizeof(mask_hole));
    ai_fubp_write_bytes(us_mask2_paddr, mask_lane2_active,
        sizeof(mask_lane2_active));
    ai_fubp_fill_bytes(us_dump_paddr, 0x58U, vl);
    ai_fubp_write_bytes(low_paddr, fof_payload, active_fault_index);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_write_bytes(fof_mask_paddr, fof_mask_hole,
        sizeof(fof_mask_hole));
    ai_fubp_write_bytes(fof_mask2_paddr, fof_mask_lane2_active,
        sizeof(fof_mask_lane2_active));
    ai_fubp_fill_bytes(fof_dump_paddr, 0x8aU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("mask-hole active-only setup preloads old v6 bytes before masked US breakpoint arbitration",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U,
        us_src_vaddr + inactive_hole_index, AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8_to_v6(us_src_vaddr, us_mask_vaddr, vl);
    TEST_ASSERT("masked US ignores a trigger match on the earlier inactive hole lane",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("mask-hole active-only reloads old v6 bytes before checking the later active breakpoint cutoff",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U,
        us_src_vaddr + active_trigger_index, AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8_to_v6(us_src_vaddr, us_mask_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("masked US selects the first active trigger lane after the inactive hole",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + active_trigger_index &&
        observed_vstart_csr == active_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + active_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("masked US dump completes after clearing the active trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("masked US keeps inactive-hole and post-breakpoint lanes at the old sentinel",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl)
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U,
        us_src_vaddr + inactive_hole_index, AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8_to_v6(us_src_vaddr, us_mask2_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("masked US reuses the new mask and reports lane2 once it becomes active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + inactive_hole_index &&
        observed_vstart_csr == inactive_hole_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + inactive_hole_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U,
        fof_src_vaddr + inactive_hole_index, AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr,
        fof_mask_vaddr, fof_dump_vaddr, vl, &observed_vl,
        &observed_vstart);
    goto_priv(PRIV_M);
    TEST_ASSERT("masked VFOF ignores the inactive-hole trigger and shortens only at the later active first-fault lane",
        excpt.triggered == false &&
        observed_vl == active_fault_index &&
        observed_vstart == 0U &&
        read8(fof_dump_paddr + inactive_hole_index) ==
            fof_seed[inactive_hole_index] &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, 2U) &&
        ai_fubp_bytes_match(fof_dump_paddr + 3U, fof_payload + 3U,
            active_fault_index - 3U) &&
        ai_fubp_bytes_match(fof_dump_paddr + active_fault_index,
            fof_seed + active_fault_index, vl - active_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane2=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(fof_dump_paddr + inactive_hole_index)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U,
        fof_src_vaddr + inactive_hole_index, AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_only(fof_src_vaddr, fof_mask2_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("masked VFOF reuses the new mask and reports the lane2 breakpoint before the later fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_src_vaddr + inactive_hole_index &&
        observed_vstart_csr == inactive_hole_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fof_src_vaddr + inactive_hole_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U,
        us_src_vaddr + inactive_hole_index, AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8_to_v6(us_src_vaddr, us_mask_vaddr, vl);
    TEST_ASSERT("after VFOF changed the mask to lane2-active, the following US instruction uses its own lane2-inactive mask and reports no stale breakpoint",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_cross_page_second_half_trigger_then_fof_fault_order_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page fof_low_page = VSRWX_GRWX;
    const enum test_page fof_high_page = VSRWX_GRW;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = 2U;
    const unsigned fault_lane = 1U;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 3U;
    const uintptr_t second_half_vaddr = hs_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x840U;
    const uintptr_t load_dump_vaddr = hs_page_base(aux_page) + 0x880U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x8c0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x900U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 3U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x840U;
    const uintptr_t load_dump_paddr = phys_page_base(aux_page) + 0x880U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x8c0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x900U;
    const uint8_t payload[4] = { 0x39U, 0x3aU, 0x3bU, 0x3cU };
    const uint8_t old_bytes[4] = { 0xa9U, 0xaaU, 0xabU, 0xacU };
    const uint8_t load_prefix_expected[4] = {
        0x39U, 0x3aU, 0xabU, 0xacU
    };
    const uint8_t fof_seed[4] = { 0xd9U, 0xdaU, 0xdbU, 0xdcU };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, payload, 3U);
    ai_fubp_write_bytes(high_paddr, payload + 3U, 1U);
    ai_fubp_write_bytes(old_paddr, old_bytes, sizeof(old_bytes));
    ai_fubp_fill_bytes(load_dump_paddr, 0x4dU, sizeof(load_prefix_expected));
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, sizeof(fof_seed));
    ai_fubp_fill_bytes(fof_dump_paddr, 0x7dU, sizeof(fof_seed));

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(old_vaddr, req_vl);
    TEST_ASSERT("cross-page second-half setup preloads old e16 lanes before the unit-stride split-element breakpoint probe",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, second_half_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(base_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("cross-page e16 load reports the high-page second-half address trigger as lane1 before exposing the no-read page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == second_half_vaddr &&
        observed_vstart_csr == fault_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, second_half_vaddr,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_from_v6(load_dump_vaddr, req_vl);
    TEST_ASSERT("cross-page e16 load dump completes after clearing the second-half trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page e16 load trigger commits only the complete prefix element and never writes the split faulting element low half",
        ai_fubp_bytes_match(load_dump_paddr, load_prefix_expected,
            sizeof(load_prefix_expected))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(base_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("cross-page e16 load retry after clearing the trigger exposes the real high-page load fault at the split element",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr &&
        observed_vstart_csr == fault_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, second_half_vaddr,
        observed_vstart_csr
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(base_vaddr, req_vl);
    TEST_ASSERT("cross-page e16 load repair completes after restoring high-page read permission",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_from_v6(load_dump_vaddr, req_vl);
    TEST_ASSERT("cross-page e16 load repair dump completes for full-image checking",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page e16 load repair produces the complete two-lane payload without stale breakpoint or fault metadata",
        ai_fubp_bytes_match(load_dump_paddr, payload, sizeof(payload))
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x8dU, sizeof(fof_seed));
    ai_fubp_arm_mcontrol6_slot(0U, second_half_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_only(base_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("cross-page e16 VFOF reports the high-page second-half breakpoint before the nonzero split-element first fault can silently shorten vl",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == second_half_vaddr &&
        observed_vstart_csr == fault_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, second_half_vaddr,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_full_dump(fof_seed_vaddr, base_vaddr,
        fof_dump_vaddr, req_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page e16 VFOF retry silently shortens at the absolute split-element index and leaves the faulting element bytes untouched",
        excpt.triggered == false &&
        observed_vl == fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, payload, 2U) &&
        ai_fubp_bytes_match(fof_dump_paddr + 2U, fof_seed + 2U, 2U),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane1_lo=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(fof_dump_paddr + 2U)
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x9dU, sizeof(fof_seed));

    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_full_dump(fof_seed_vaddr, base_vaddr,
        fof_dump_vaddr, req_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page e16 VFOF repair completes the full two-lane payload with no stale shortened-vl state",
        excpt.triggered == false &&
        observed_vl == req_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, payload, sizeof(payload)),
        "triggered=%d cause=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_dual_address_trigger_earliest_then_fof_fallback_manual_corner()
{
    TEST_START();

    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_early_index = 3U;
    const unsigned us_late_index = 9U;
    const unsigned fof_early_index = 4U;
    const unsigned fof_late_index = 7U;
    const unsigned fof_fault_index = 11U;
    const uintptr_t us_src_vaddr = hs_page_base(data_page) + 0xa20U;
    const uintptr_t us_old_vaddr = hs_page_base(old_page) + 0xa80U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0xae0U;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0xb40U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xb80U;
    const uintptr_t us_src_paddr = phys_page_base(data_page) + 0xa20U;
    const uintptr_t us_old_paddr = phys_page_base(old_page) + 0xa80U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0xae0U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0xb40U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xb80U;
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected_early[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected_late[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, sizeof(us_payload), 0x2bU);
    ai_fubp_fill_pattern(us_old, sizeof(us_old), 0xabU);
    ai_fubp_fill_pattern(fof_payload, sizeof(fof_payload), 0x5bU);
    ai_fubp_fill_pattern(fof_seed, sizeof(fof_seed), 0xdbU);
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        us_expected_early[i] = (i < us_early_index) ?
            us_payload[i] : us_old[i];
        us_expected_late[i] = (i < us_late_index) ?
            us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_clear_mcontrol6_slot(1U);
    ai_fubp_write_bytes(us_src_paddr, us_payload, vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x4cU, vl);
    ai_fubp_write_bytes(low_paddr, fof_payload, fof_fault_index);
    ai_fubp_write_bytes(high_paddr, fof_payload + fof_fault_index,
        vl - fof_fault_index);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x7cU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("dual address-trigger setup preloads old v6 bytes before checking US earliest-lane arbitration",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_late_index,
        AI_FUBP_MCONTROL6_LOAD);
    ai_fubp_arm_mcontrol6_slot(1U, us_src_vaddr + us_early_index,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("dual address-trigger US load reports the earliest active lane even when it is programmed in slot1 after the later slot0 trigger",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_early_index &&
        observed_vstart_csr == us_early_index,
        "cause=0x%lx tval=0x%lx early=0x%lx late=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + us_early_index,
        us_src_vaddr + us_late_index, observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(1U);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("dual address-trigger reloads old v6 bytes before clearing the early slot and probing the later slot",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("after clearing the early trigger slot, the later US trigger becomes the winner and the old lane is not reported again",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_late_index &&
        observed_vstart_csr == us_late_index,
        "cause=0x%lx tval=0x%lx expected_late=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + us_late_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("dual address-trigger US dump completes after clearing both trigger slots",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("dual address-trigger US prefix after clearing early slot stops at the later trigger and does not reuse the old earliest lane",
        ai_fubp_bytes_match(us_dump_paddr, us_expected_late, vl) &&
        !ai_fubp_bytes_match(us_dump_paddr, us_expected_early, vl)
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_arm_mcontrol6_slot(0U, fof_src_vaddr + fof_late_index,
        AI_FUBP_MCONTROL6_LOAD);
    ai_fubp_arm_mcontrol6_slot(1U, fof_src_vaddr + fof_early_index,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(fof_src_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("dual address-trigger VFOF reports the earliest trigger lane before the later trigger and before the nonzero first-fault lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_src_vaddr + fof_early_index &&
        observed_vstart_csr == fof_early_index,
        "cause=0x%lx tval=0x%lx early=0x%lx late=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fof_src_vaddr + fof_early_index,
        fof_src_vaddr + fof_late_index, observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(1U);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(fof_src_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("after clearing the early VFOF trigger, the later trigger lane becomes visible instead of falling through to shortened-vl",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_src_vaddr + fof_late_index &&
        observed_vstart_csr == fof_late_index,
        "cause=0x%lx tval=0x%lx expected_late=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fof_src_vaddr + fof_late_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x6cU, vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both VFOF triggers, only then does the nonzero first-fault lane shorten vl and preserve the suffix seed",
        excpt.triggered == false &&
        observed_vl == fof_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload,
            fof_fault_index) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_index,
            fof_seed + fof_fault_index, vl - fof_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_store_bp_prefix_to_fof_forward_boundary_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned store_vl = AI_FUBP_FULL_VL;
    const unsigned store_trigger_index = 12U;
    const unsigned fof_req_vl = AI_FUBP_FULL_VL;
    const unsigned fof_fault_index = 11U;
    const uintptr_t target_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xc20U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0xc80U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xcc0U;
    const uintptr_t target_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xc20U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0xc80U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xcc0U;
    uint8_t old_mem[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t expected_memory[AI_FUBP_FULL_VL] = { 0 };
    uint8_t expected_vfof_prefix[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(old_mem, sizeof(old_mem), 0x36U);
    ai_fubp_fill_pattern(store_payload, sizeof(store_payload), 0xb6U);
    ai_fubp_fill_pattern(fof_seed, sizeof(fof_seed), 0xe6U);
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        expected_memory[i] = (i < store_trigger_index) ?
            store_payload[i] : old_mem[i];
        expected_vfof_prefix[i] = (i < store_trigger_index) ?
            store_payload[i] : old_mem[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(target_paddr, old_mem, fof_fault_index);
    ai_fubp_write_bytes(high_paddr, old_mem + fof_fault_index,
        AI_FUBP_FULL_VL - fof_fault_index);
    ai_fubp_write_bytes(src_paddr, store_payload, store_vl);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, fof_req_vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x5cU, fof_req_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, store_vl);
    TEST_ASSERT("store-to-VFOF forwarding setup loads the store payload before arming the cutoff breakpoint",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U,
        target_vaddr + store_trigger_index, AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, store_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("store-to-VFOF forwarding source store reports breakpoint at the first uncommitted suffix lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target_vaddr + store_trigger_index &&
        observed_vstart_csr == store_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, target_vaddr + store_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("store-to-VFOF forwarding memory image contains only committed store prefix and old suffix before the VFOF readback",
        ai_fubp_split_bytes_match(target_paddr, high_paddr,
            fof_fault_index, expected_memory, AI_FUBP_FULL_VL)
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x6cU, fof_req_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, target_vaddr,
        fof_dump_vaddr, fof_req_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF readback sees committed store prefix, old uncommitted suffix, and shortens only at the later first-fault lane",
        excpt.triggered == false &&
        observed_vl == fof_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, expected_vfof_prefix,
            fof_fault_index) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_index,
            fof_seed + fof_fault_index, fof_req_vl - fof_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane10=0x%x old10=0x%x payload10=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(fof_dump_paddr + fof_fault_index - 1U),
        old_mem[fof_fault_index - 1U],
        store_payload[fof_fault_index - 1U]
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, store_vl);
    TEST_ASSERT("store-to-VFOF forwarding retry reloads payload after VFOF has consumed the prefix/old-suffix image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, store_vl);
    TEST_ASSERT("store-to-VFOF forwarding retry completes the full store after clearing trigger and restoring permissions",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("store-to-VFOF forwarding final memory becomes the full store payload exactly once after the explicit retry",
        ai_fubp_split_bytes_match(target_paddr, high_paddr,
            fof_fault_index, store_payload, store_vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_fof_breakpoint_tail_mask_suffix_preserve_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = 10U;
    const unsigned full_vl = AI_FUBP_FULL_VL;
    const unsigned masked_lane = 3U;
    const unsigned trigger_index = 4U;
    const unsigned masked_suffix = 5U;
    const unsigned fault_index = 8U;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fault_index;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xd20U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0xd60U;
    const uintptr_t bp_dump_vaddr = hs_page_base(aux_page) + 0xd80U;
    const uintptr_t tu_dump_vaddr = hs_page_base(aux_page) + 0xdc0U;
    const uintptr_t ta_dump_vaddr = hs_page_base(aux_page) + 0xe00U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fault_index;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xd20U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0xd60U;
    const uintptr_t bp_dump_paddr = phys_page_base(aux_page) + 0xd80U;
    const uintptr_t tu_dump_paddr = phys_page_base(aux_page) + 0xdc0U;
    const uintptr_t ta_dump_paddr = phys_page_base(aux_page) + 0xe00U;
    const uint8_t mask_bytes[2] = { 0x57U, 0x03U };
    uint8_t payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(payload, sizeof(payload), 0x4dU);
    ai_fubp_fill_pattern(seed, sizeof(seed), 0xcdU);

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, payload, fault_index);
    ai_fubp_write_bytes(high_paddr, payload + fault_index,
        full_vl - fault_index);
    ai_fubp_write_bytes(seed_paddr, seed, full_vl);
    ai_fubp_write_bytes(mask_paddr, mask_bytes, sizeof(mask_bytes));
    ai_fubp_fill_bytes(bp_dump_paddr, 0x2dU, full_vl);
    ai_fubp_fill_bytes(tu_dump_paddr, 0x3dU, full_vl);
    ai_fubp_fill_bytes(ta_dump_paddr, 0x4dU, full_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, src_vaddr + trigger_index,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_tail_full_dump(seed_vaddr, src_vaddr,
        mask_vaddr, bp_dump_vaddr, req_vl, full_vl, false,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("masked VFOF breakpoint path reports the active trigger lane without converting it into shortened-vl",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == src_vaddr + trigger_index &&
        observed_vstart == trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu vl=%u captured_vstart=%u",
        excpt.cause, excpt.tval, src_vaddr + trigger_index,
        (uint64_t)observed_vstart, observed_vl, observed_vstart
    );

    TEST_ASSERT("masked VFOF breakpoint full dump preserves masked lanes, trigger lane, later suffix, and architectural tail bytes",
        ai_fubp_bytes_match(bp_dump_paddr, payload, masked_lane) &&
        read8(bp_dump_paddr + masked_lane) == seed[masked_lane] &&
        read8(bp_dump_paddr + trigger_index) == seed[trigger_index] &&
        read8(bp_dump_paddr + masked_suffix) == seed[masked_suffix] &&
        ai_fubp_bytes_match(bp_dump_paddr + 6U, seed + 6U,
            full_vl - 6U),
        "lane3=0x%x lane4=0x%x lane5=0x%x tail10=0x%x seed3=0x%x seed4=0x%x seed5=0x%x seed10=0x%x",
        read8(bp_dump_paddr + masked_lane),
        read8(bp_dump_paddr + trigger_index),
        read8(bp_dump_paddr + masked_suffix),
        read8(bp_dump_paddr + req_vl),
        seed[masked_lane], seed[trigger_index], seed[masked_suffix],
        seed[req_vl]
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_tail_full_dump(seed_vaddr, src_vaddr,
        mask_vaddr, tu_dump_vaddr, req_vl, full_vl, false,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("masked VFOF trigger-clear path shortens only at the active first-fault lane and keeps undisturbed suffix/tail bytes explainable",
        excpt.triggered == false &&
        observed_vl == fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(tu_dump_paddr, payload, masked_lane) &&
        read8(tu_dump_paddr + masked_lane) == seed[masked_lane] &&
        read8(tu_dump_paddr + trigger_index) == payload[trigger_index] &&
        read8(tu_dump_paddr + masked_suffix) == seed[masked_suffix] &&
        ai_fubp_bytes_match(tu_dump_paddr + 6U, payload + 6U,
            fault_index - 6U) &&
        ai_fubp_bytes_match(tu_dump_paddr + fault_index,
            seed + fault_index, full_vl - fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane3=0x%x lane4=0x%x lane5=0x%x lane8=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(tu_dump_paddr + masked_lane),
        read8(tu_dump_paddr + trigger_index),
        read8(tu_dump_paddr + masked_suffix),
        read8(tu_dump_paddr + fault_index)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_tail_full_dump(seed_vaddr, src_vaddr,
        mask_vaddr, ta_dump_vaddr, req_vl, full_vl, true,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("masked VFOF agnostic policy after trigger clear may agnostic-fill inactive/tail bytes but must not leak breakpoint mask into active prefix or fault suffix",
        excpt.triggered == false &&
        observed_vl == fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(ta_dump_paddr, payload, masked_lane) &&
        ai_fubp_bytes_old_or_agnostic(ta_dump_paddr, seed,
            masked_lane, 1U) &&
        read8(ta_dump_paddr + trigger_index) == payload[trigger_index] &&
        ai_fubp_bytes_old_or_agnostic(ta_dump_paddr, seed,
            masked_suffix, 1U) &&
        ai_fubp_bytes_match(ta_dump_paddr + 6U, payload + 6U,
            fault_index - 6U) &&
        ai_fubp_bytes_old_or_agnostic(ta_dump_paddr, seed,
            fault_index, full_vl - fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane3=0x%x lane5=0x%x lane8=0x%x tail10=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(ta_dump_paddr + masked_lane),
        read8(ta_dump_paddr + masked_suffix),
        read8(ta_dump_paddr + fault_index),
        read8(ta_dump_paddr + req_vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_us_bp_pte_flip_then_fof_permission_refresh_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned flip_index = 9U;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        flip_index;
    const uintptr_t flip_vaddr = hs_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0xe40U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0xe80U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0xec0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xf00U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        flip_index;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0xe40U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0xe80U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0xec0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xf00U;
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t repair_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, sizeof(us_payload), 0x23U);
    ai_fubp_fill_pattern(us_old, sizeof(us_old), 0xa3U);
    ai_fubp_fill_pattern(fof_fault_payload, sizeof(fof_fault_payload), 0x53U);
    ai_fubp_fill_pattern(repair_payload, sizeof(repair_payload), 0x83U);
    ai_fubp_fill_pattern(fof_seed, sizeof(fof_seed), 0xd3U);
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        us_expected[i] = (i < flip_index) ? us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, us_payload, flip_index);
    ai_fubp_write_bytes(high_paddr, us_payload + flip_index,
        req_vl - flip_index);
    ai_fubp_write_bytes(old_paddr, us_old, req_vl);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, req_vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x31U, req_vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x41U, req_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, req_vl);
    TEST_ASSERT("PTE-flip setup preloads old v6 bytes before the readable-page US breakpoint round",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, flip_vaddr, AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(base_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("PTE-flip first round reports the same high-page VA as a US breakpoint while the page is readable",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == flip_vaddr &&
        observed_vstart_csr == flip_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, flip_vaddr, observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, req_vl);
    TEST_ASSERT("PTE-flip US breakpoint dump completes after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PTE-flip first round keeps only the old readable-page US prefix before the breakpoint lane",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, req_vl)
    );

    ai_fubp_write_bytes(low_paddr, fof_fault_payload, flip_index);
    ai_fubp_write_bytes(high_paddr, fof_fault_payload + flip_index,
        req_vl - flip_index);
    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, base_vaddr,
        fof_dump_vaddr, req_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("PTE-flip second round uses the new no-read PTE after sfence and shortens VFOF at the same VA instead of reusing the old readable permission",
        excpt.triggered == false &&
        observed_vl == flip_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_payload,
            flip_index) &&
        ai_fubp_bytes_match(fof_dump_paddr + flip_index,
            fof_seed + flip_index, req_vl - flip_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane9=0x%x seed9=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(fof_dump_paddr + flip_index),
        fof_seed[flip_index]
    );

    ai_fubp_write_bytes(low_paddr, repair_payload, flip_index);
    ai_fubp_write_bytes(high_paddr, repair_payload + flip_index,
        req_vl - flip_index);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x51U, req_vl);
    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, base_vaddr,
        fof_dump_vaddr, req_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("PTE-flip repair round reloads the full current payload after restoring high-page read permission and clearing stale VFOF fault metadata",
        excpt.triggered == false &&
        observed_vl == req_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, repair_payload, req_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane9=0x%x expected9=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(fof_dump_paddr + flip_index),
        repair_payload[flip_index]
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_wrong_path_us_fof_bp_kill_correct_path_fresh_manual_corner()
{
    TEST_START();

    const enum test_page wrong_page = VSRWX_GRWX;
    const enum test_page right_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned wrong_trigger_index = 5U;
    const unsigned right_fault_index = 11U;
    const uintptr_t wrong_us_vaddr = hs_page_base(wrong_page) + 0x100U;
    const uintptr_t right_fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        right_fault_index;
    const uintptr_t right_fof_seed_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t right_fof_dump_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t wrong_fof_vaddr = hs_page_base(wrong_page) + 0x180U;
    const uintptr_t right_us_vaddr = hs_page_base(right_page) + 0x1c0U;
    const uintptr_t right_us_dump_vaddr = hs_page_base(aux_page) + 0x200U;
    const uintptr_t wrong_us_paddr = phys_page_base(wrong_page) + 0x100U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        right_fault_index;
    const uintptr_t wrong_fof_paddr = phys_page_base(wrong_page) + 0x180U;
    const uintptr_t right_us_paddr = phys_page_base(right_page) + 0x1c0U;
    const uintptr_t right_fof_seed_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t right_fof_dump_paddr = phys_page_base(aux_page) + 0x140U;
    const uintptr_t right_us_dump_paddr = phys_page_base(aux_page) + 0x200U;
    uint8_t wrong_us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t right_fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t wrong_fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t right_us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t right_fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t wrong_training_dump[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(wrong_us_payload, sizeof(wrong_us_payload), 0x2aU);
    ai_fubp_fill_pattern(right_fof_payload, sizeof(right_fof_payload), 0x5aU);
    ai_fubp_fill_pattern(wrong_fof_payload, sizeof(wrong_fof_payload), 0x7aU);
    ai_fubp_fill_pattern(right_us_payload, sizeof(right_us_payload), 0xaaU);
    ai_fubp_fill_pattern(right_fof_seed, sizeof(right_fof_seed), 0xdaU);
    ai_fubp_fill_pattern(wrong_training_dump, sizeof(wrong_training_dump),
        0xeaU);

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(wrong_us_paddr, wrong_us_payload, vl);
    ai_fubp_write_bytes(low_paddr, right_fof_payload, right_fault_index);
    ai_fubp_write_bytes(wrong_fof_paddr, wrong_fof_payload, vl);
    ai_fubp_write_bytes(right_us_paddr, right_us_payload, vl);
    ai_fubp_write_bytes(right_fof_seed_paddr, right_fof_seed, vl);
    ai_fubp_fill_bytes(right_fof_dump_paddr, 0x34U, vl);
    ai_fubp_fill_bytes(right_us_dump_paddr, 0x45U, vl);

    ai_fubp_prepare_three_pages(wrong_page, PTE_V | PTE_AD | PTE_RWX,
        low_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(right_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_branch_wrong_us_right_vfof(wrong_us_vaddr,
        right_fof_seed_vaddr, right_fof_vaddr, right_fof_dump_vaddr, vl,
        0U, &observed_vl, &observed_vstart);
    TEST_ASSERT("wrong-path US/FOF training executes the not-taken US poison path without trigger armed",
        excpt.triggered == false &&
        observed_vl == 0U &&
        observed_vstart == 0U
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U,
        wrong_us_vaddr + wrong_trigger_index, AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_branch_wrong_us_right_vfof(wrong_us_vaddr,
        right_fof_seed_vaddr, right_fof_vaddr, right_fof_dump_vaddr, vl,
        1U, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("branch redirect kills the wrong-path US address breakpoint while the correct-path VFOF uses its own shortened-vl metadata",
        excpt.triggered == false &&
        observed_vl == right_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(right_fof_dump_paddr, right_fof_payload,
            right_fault_index) &&
        ai_fubp_bytes_match(right_fof_dump_paddr + right_fault_index,
            right_fof_seed + right_fault_index, vl - right_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u lane5=0x%x lane11=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(right_fof_dump_paddr + wrong_trigger_index),
        read8(right_fof_dump_paddr + right_fault_index)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(right_us_dump_paddr, 0x56U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_branch_wrong_vfof_right_us(wrong_fof_vaddr, right_us_vaddr,
        right_us_dump_vaddr, vl, 0U);
    TEST_ASSERT("wrong-path VFOF/US training executes the not-taken VFOF poison path without trigger armed",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U,
        wrong_fof_vaddr + wrong_trigger_index, AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_branch_wrong_vfof_right_us(wrong_fof_vaddr, right_us_vaddr,
        right_us_dump_vaddr, vl, 1U);

    goto_priv(PRIV_M);
    TEST_ASSERT("branch redirect kills the wrong-path VFOF breakpoint and the correct-path US load dumps only its fresh payload",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL &&
        ai_fubp_bytes_match(right_us_dump_paddr, right_us_payload, vl),
        "triggered=%d cause=0x%lx tval=0x%lx lane5=0x%x expected5=0x%x",
        excpt.triggered, excpt.cause, excpt.tval,
        read8(right_us_dump_paddr + wrong_trigger_index),
        right_us_payload[wrong_trigger_index]
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_entry_reuse_pressure_then_us_bp_owner_refresh_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned old_fault_index = 5U;
    const unsigned us_trigger_index = 12U;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        old_fault_index;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x80U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xc0U;
    const uintptr_t pressure0_vaddr = hs_page_base(data_page) + 0x180U;
    const uintptr_t pressure0_seed_vaddr = hs_page_base(aux_page) + 0x120U;
    const uintptr_t pressure0_dump_vaddr = hs_page_base(aux_page) + 0x160U;
    const uintptr_t pressure1_vaddr = hs_page_base(data_page) + 0x240U;
    const uintptr_t pressure1_seed_vaddr = hs_page_base(aux_page) + 0x1a0U;
    const uintptr_t pressure1_dump_vaddr = hs_page_base(aux_page) + 0x1e0U;
    const uintptr_t us_src_vaddr = hs_page_base(data_page) + 0x320U;
    const uintptr_t us_old_vaddr = hs_page_base(old_page) + 0x380U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        old_fault_index;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x80U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xc0U;
    const uintptr_t pressure0_paddr = phys_page_base(data_page) + 0x180U;
    const uintptr_t pressure0_seed_paddr = phys_page_base(aux_page) + 0x120U;
    const uintptr_t pressure0_dump_paddr = phys_page_base(aux_page) + 0x160U;
    const uintptr_t pressure1_paddr = phys_page_base(data_page) + 0x240U;
    const uintptr_t pressure1_seed_paddr = phys_page_base(aux_page) + 0x1a0U;
    const uintptr_t pressure1_dump_paddr = phys_page_base(aux_page) + 0x1e0U;
    const uintptr_t us_src_paddr = phys_page_base(data_page) + 0x320U;
    const uintptr_t us_old_paddr = phys_page_base(old_page) + 0x380U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x240U;
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pressure0_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pressure0_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pressure1_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pressure1_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_prefix_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(fof_payload, req_vl, 0x45U);
    ai_fubp_fill_pattern(fof_seed, req_vl, 0xc5U);
    ai_fubp_fill_pattern(pressure0_payload, req_vl, 0x64U);
    ai_fubp_fill_pattern(pressure0_seed, req_vl, 0xe4U);
    ai_fubp_fill_pattern(pressure1_payload, req_vl, 0x75U);
    ai_fubp_fill_pattern(pressure1_seed, req_vl, 0xf5U);
    ai_fubp_fill_pattern(us_payload, req_vl, 0x28U);
    ai_fubp_fill_pattern(us_old, req_vl, 0xa8U);
    for (unsigned i = 0; i < req_vl; ++i) {
        us_prefix_expected[i] = (i < us_trigger_index) ?
            us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, fof_payload, old_fault_index);
    ai_fubp_write_bytes(high_paddr, fof_payload + old_fault_index,
        req_vl - old_fault_index);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, req_vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x32U, req_vl);
    ai_fubp_write_bytes(pressure0_paddr, pressure0_payload, req_vl);
    ai_fubp_write_bytes(pressure0_seed_paddr, pressure0_seed, req_vl);
    ai_fubp_fill_bytes(pressure0_dump_paddr, 0x42U, req_vl);
    ai_fubp_write_bytes(pressure1_paddr, pressure1_payload, req_vl);
    ai_fubp_write_bytes(pressure1_seed_paddr, pressure1_seed, req_vl);
    ai_fubp_fill_bytes(pressure1_dump_paddr, 0x52U, req_vl);
    ai_fubp_write_bytes(us_src_paddr, us_payload, req_vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, req_vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x62U, req_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr, fof_dump_vaddr,
        req_vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("entry-reuse pressure seed VFOF produces exactly one old shortened-vl before later US breakpoint reuse",
        excpt.triggered == false &&
        observed_vl == old_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload,
            old_fault_index) &&
        ai_fubp_bytes_match(fof_dump_paddr + old_fault_index,
            fof_seed + old_fault_index, req_vl - old_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_fill_bytes(fof_dump_paddr, 0x33U, req_vl);
    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr, fof_dump_vaddr,
        req_vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("entry-reuse pressure repair pass releases the old VFOF shortened-vl owner before pressure allocations",
        excpt.triggered == false &&
        observed_vl == req_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, req_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(pressure0_seed_vaddr, pressure0_vaddr,
        pressure0_dump_vaddr, req_vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("entry-reuse pressure VFOF e8 full pass allocates a fresh owner with distinct payload",
        excpt.triggered == false &&
        observed_vl == req_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(pressure0_dump_paddr, pressure0_payload,
            req_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_full_dump(pressure1_seed_vaddr, pressure1_vaddr,
        pressure1_dump_vaddr, req_vl / 2U, &observed_vl,
        &observed_vstart);
    TEST_ASSERT("entry-reuse pressure VFOF e16 pass changes EEW/vtype so stale e8 owner metadata is observable",
        excpt.triggered == false &&
        observed_vl == req_vl / 2U &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(pressure1_dump_paddr, pressure1_payload,
            req_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, req_vl);
    TEST_ASSERT("entry-reuse pressure US setup preloads old v6 bytes after VFOF entry pressure",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_trigger_index,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("entry-reuse pressure US breakpoint reports the current lane beyond the old VFOF shortened-vl bound",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_trigger_index &&
        observed_vstart_csr == us_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu old_vl=%u",
        excpt.cause, excpt.tval, us_src_vaddr + us_trigger_index,
        observed_vstart_csr, old_fault_index
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, req_vl);
    TEST_ASSERT("entry-reuse pressure can dump the interrupted US vector state after clearing the breakpoint",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("entry-reuse pressure US breakpoint prefix is current payload through lane11, not clipped by old VFOF vl5",
        ai_fubp_bytes_match(us_dump_paddr, us_prefix_expected, req_vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, req_vl);
    TEST_ASSERT("entry-reuse pressure US retry completes after trigger clear with no stale VFOF owner reappearing",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, req_vl);
    TEST_ASSERT("entry-reuse pressure final US dump succeeds after retry",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("entry-reuse pressure final US image is the fresh current payload and old VFOF vl is not replayed",
        ai_fubp_bytes_match(us_dump_paddr, us_payload, req_vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_us_store_bp_suffix_cancel_then_new_store_full_commit_manual_corner()
{
    TEST_START();

    const enum test_page target_page = VSRW_GRWX;
    const enum test_page src_page = VSRWX_GRWX;
    const unsigned vl = 14U;
    const unsigned old_trigger_index = 6U;
    const unsigned new_trigger_index = 11U;
    const uintptr_t target0_vaddr = hs_page_base(target_page) + 0xa00U;
    const uintptr_t target1_vaddr = hs_page_base(target_page) + 0xa80U;
    const uintptr_t src0_vaddr = hs_page_base(src_page) + 0xb00U;
    const uintptr_t src1_vaddr = hs_page_base(src_page) + 0xb40U;
    const uintptr_t src2_vaddr = hs_page_base(src_page) + 0xb80U;
    const uintptr_t src3_vaddr = hs_page_base(src_page) + 0xbc0U;
    const uintptr_t target0_paddr = phys_page_base(target_page) + 0xa00U;
    const uintptr_t target1_paddr = phys_page_base(target_page) + 0xa80U;
    const uintptr_t src0_paddr = phys_page_base(src_page) + 0xb00U;
    const uintptr_t src1_paddr = phys_page_base(src_page) + 0xb40U;
    const uintptr_t src2_paddr = phys_page_base(src_page) + 0xb80U;
    const uintptr_t src3_paddr = phys_page_base(src_page) + 0xbc0U;
    uint8_t initial0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t initial1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t payload0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t payload1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t payload2[AI_FUBP_FULL_VL] = { 0 };
    uint8_t payload3[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_prefix_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t new_prefix_expected[AI_FUBP_FULL_VL] = { 0 };
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(initial0, vl, 0x19U);
    ai_fubp_fill_pattern(initial1, vl, 0x49U);
    ai_fubp_fill_pattern(payload0, vl, 0x89U);
    ai_fubp_fill_pattern(payload1, vl, 0xb9U);
    ai_fubp_fill_pattern(payload2, vl, 0xd9U);
    ai_fubp_fill_pattern(payload3, vl, 0x29U);
    for (unsigned i = 0; i < vl; ++i) {
        old_prefix_expected[i] = (i < old_trigger_index) ?
            payload0[i] : initial0[i];
        new_prefix_expected[i] = (i < new_trigger_index) ?
            payload3[i] : payload2[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(target0_paddr, initial0, vl);
    ai_fubp_write_bytes(target1_paddr, initial1, vl);
    ai_fubp_write_bytes(src0_paddr, payload0, vl);
    ai_fubp_write_bytes(src1_paddr, payload1, vl);
    ai_fubp_write_bytes(src2_paddr, payload2, vl);
    ai_fubp_write_bytes(src3_paddr, payload3, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src0_vaddr, vl);
    TEST_ASSERT("SQ reuse setup loads payload0 before the old store breakpoint episode",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, target0_vaddr + old_trigger_index,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target0_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("SQ reuse old store breakpoint cuts off at the first episode lane6",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target0_vaddr + old_trigger_index &&
        observed_vstart_csr == old_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, target0_vaddr + old_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("SQ reuse old store breakpoint commits only prefix bytes and leaves the old suffix unchanged",
        ai_fubp_bytes_match(target0_paddr, old_prefix_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src1_vaddr, vl);
    TEST_ASSERT("SQ reuse pressure store loads payload1 after releasing the old breakpoint entry",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target1_vaddr, vl);
    TEST_ASSERT("SQ reuse pressure store to a distinct target completes without inheriting old suffix cancel",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("SQ reuse pressure target receives all payload1 bytes exactly once",
        ai_fubp_bytes_match(target1_paddr, payload1, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src2_vaddr, vl);
    TEST_ASSERT("SQ reuse overwrite store loads payload2 for the same target as the old breakpoint",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target0_vaddr, vl);
    TEST_ASSERT("SQ reuse overwrite store covers the old suffix region after the old entry is released",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("SQ reuse overwrite store updates all active bytes, proving old suffix cancel did not survive entry reuse",
        ai_fubp_bytes_match(target0_paddr, payload2, vl)
    );

    ai_fubp_arm_mcontrol6_slot(0U, target0_vaddr + new_trigger_index,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src3_vaddr, vl);
    TEST_ASSERT("SQ reuse new breakpoint round loads payload3 before changing the breakpoint lane",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target0_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("SQ reuse new store breakpoint reports the new lane11 instead of stale old lane6",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target0_vaddr + new_trigger_index &&
        observed_vstart_csr == new_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu old_lane=%u",
        excpt.cause, excpt.tval, target0_vaddr + new_trigger_index,
        observed_vstart_csr, old_trigger_index
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("SQ reuse new breakpoint cuts only at the new lane and keeps suffix from the immediately previous full store",
        ai_fubp_bytes_match(target0_paddr, new_prefix_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src3_vaddr, vl);
    TEST_ASSERT("SQ reuse final retry reloads payload3 after clearing the new store breakpoint",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target0_vaddr, vl);
    TEST_ASSERT("SQ reuse final retry completes all active bytes after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("SQ reuse final target image is payload3 with no stale suffix cancel or old-lane owner",
        ai_fubp_bytes_match(target0_paddr, payload3, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_us_store_bp_sbuffer_pressure_then_fof_freshness_manual_corner()
{
    TEST_START();

    const enum test_page store_page = VSRW_GRWX;
    const enum test_page pressure_page = VSRWX_GRWX;
    const enum test_page src_page = VSRWX_GRW;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const unsigned store_vl = 12U;
    const unsigned store_trigger_index = 7U;
    const unsigned fof_fault_index = 9U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0xc00U;
    const uintptr_t pressure0_vaddr = hs_page_base(pressure_page) + 0xc80U;
    const uintptr_t pressure1_vaddr = hs_page_base(pressure_page) + 0xd00U;
    const uintptr_t src0_vaddr = hs_page_base(src_page) + 0xd80U;
    const uintptr_t src1_vaddr = hs_page_base(src_page) + 0xdc0U;
    const uintptr_t src2_vaddr = hs_page_base(src_page) + 0xe00U;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0xe40U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xe80U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0xc00U;
    const uintptr_t pressure0_paddr = phys_page_base(pressure_page) + 0xc80U;
    const uintptr_t pressure1_paddr = phys_page_base(pressure_page) + 0xd00U;
    const uintptr_t src0_paddr = phys_page_base(src_page) + 0xd80U;
    const uintptr_t src1_paddr = phys_page_base(src_page) + 0xdc0U;
    const uintptr_t src2_paddr = phys_page_base(src_page) + 0xe00U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_index;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0xe40U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xe80U;
    uint8_t store_initial[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pressure_initial0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pressure_initial1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pressure_payload0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pressure_payload1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(store_initial, store_vl, 0x17U);
    ai_fubp_fill_pattern(pressure_initial0, store_vl, 0x47U);
    ai_fubp_fill_pattern(pressure_initial1, store_vl, 0x77U);
    ai_fubp_fill_pattern(store_payload, store_vl, 0xa7U);
    ai_fubp_fill_pattern(pressure_payload0, store_vl, 0xc7U);
    ai_fubp_fill_pattern(pressure_payload1, store_vl, 0xe7U);
    ai_fubp_fill_pattern(fof_payload, AI_FUBP_FULL_VL, 0x58U);
    ai_fubp_fill_pattern(fof_seed, AI_FUBP_FULL_VL, 0xd8U);
    for (unsigned i = 0; i < store_vl; ++i) {
        store_expected[i] = (i < store_trigger_index) ?
            store_payload[i] : store_initial[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(store_paddr, store_initial, store_vl);
    ai_fubp_write_bytes(pressure0_paddr, pressure_initial0, store_vl);
    ai_fubp_write_bytes(pressure1_paddr, pressure_initial1, store_vl);
    ai_fubp_write_bytes(src0_paddr, store_payload, store_vl);
    ai_fubp_write_bytes(src1_paddr, pressure_payload0, store_vl);
    ai_fubp_write_bytes(src2_paddr, pressure_payload1, store_vl);
    ai_fubp_write_bytes(low_paddr, fof_payload, fof_fault_index);
    ai_fubp_write_bytes(high_paddr, fof_payload + fof_fault_index,
        AI_FUBP_FULL_VL - fof_fault_index);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x3aU, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src0_vaddr, store_vl);
    TEST_ASSERT("SBuffer pressure setup loads the breakpoint store payload",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, store_vaddr + store_trigger_index,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, store_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("SBuffer pressure first store reports the precise breakpoint lane before suffix side effects",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_vaddr + store_trigger_index &&
        observed_vstart_csr == store_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, store_vaddr + store_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("SBuffer pressure first store commits prefix exactly once and preserves trigger/suffix bytes",
        ai_fubp_bytes_match(store_paddr, store_expected, store_vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src1_vaddr, store_vl);
    TEST_ASSERT("SBuffer pressure second store loads pressure payload0 after the breakpoint trap",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(pressure0_vaddr, store_vl);
    TEST_ASSERT("SBuffer pressure second store completes on an adjacent store target",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src2_vaddr, store_vl);
    TEST_ASSERT("SBuffer pressure third store loads pressure payload1 to keep the store side busy",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(pressure1_vaddr, store_vl);
    TEST_ASSERT("SBuffer pressure third store completes without reviving the old breakpoint suffix mask",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("SBuffer pressure stores commit their full images while the original prefix image stays stable",
        ai_fubp_bytes_match(store_paddr, store_expected, store_vl) &&
        ai_fubp_bytes_match(pressure0_paddr, pressure_payload0, store_vl) &&
        ai_fubp_bytes_match(pressure1_paddr, pressure_payload1, store_vl)
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr, fof_dump_vaddr,
        AI_FUBP_FULL_VL, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("SBuffer pressure does not leak store-side vecExceptionFlag or finishFlowMask into the following VFOF shortened-vl",
        excpt.triggered == false &&
        observed_vl == fof_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload,
            fof_fault_index) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_index,
            fof_seed + fof_fault_index, AI_FUBP_FULL_VL - fof_fault_index) &&
        ai_fubp_bytes_match(store_paddr, store_expected, store_vl) &&
        ai_fubp_bytes_match(pressure0_paddr, pressure_payload0, store_vl) &&
        ai_fubp_bytes_match(pressure1_paddr, pressure_payload1, store_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_pte_rewalk_then_bp_reevaluate_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned rewalk_index = 8U;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        rewalk_index;
    const uintptr_t trigger_vaddr = src_vaddr + rewalk_index;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xf00U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xf40U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        rewalk_index;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xf00U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xf40U;
    uint8_t gen0_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t gen1_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t gen2_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(gen0_payload, req_vl, 0x3cU);
    ai_fubp_fill_pattern(gen1_payload, req_vl, 0x6cU);
    ai_fubp_fill_pattern(gen2_payload, req_vl, 0x9cU);
    ai_fubp_fill_pattern(seed, req_vl, 0xdcU);

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, gen0_payload, rewalk_index);
    ai_fubp_write_bytes(high_paddr, gen0_payload + rewalk_index,
        req_vl - rewalk_index);
    ai_fubp_write_bytes(seed_paddr, seed, req_vl);
    ai_fubp_fill_bytes(dump_paddr, 0x2cU, req_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        req_vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("VFOF PTE rewalk seed generation silently shortens at the old no-read high-page lane",
        excpt.triggered == false &&
        observed_vl == rewalk_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, gen0_payload, rewalk_index) &&
        ai_fubp_bytes_match(dump_paddr + rewalk_index,
            seed + rewalk_index, req_vl - rewalk_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    goto_priv(PRIV_M);
    ai_fubp_write_bytes(low_paddr, gen1_payload, rewalk_index);
    ai_fubp_write_bytes(high_paddr, gen1_payload + rewalk_index,
        req_vl - rewalk_index);
    ai_fubp_fill_bytes(dump_paddr, 0x3cU, req_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_arm_mcontrol6_slot(0U, trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(src_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("VFOF PTE rewalk must re-evaluate the repaired high-page address trigger instead of replaying the old shortened-vl",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == trigger_vaddr,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu old_vl=%u",
        excpt.cause, excpt.tval, trigger_vaddr, observed_vstart_csr,
        rewalk_index
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        req_vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("VFOF PTE rewalk clear-trigger retry reads the repaired generation as a full success",
        excpt.triggered == false &&
        observed_vl == req_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, gen1_payload, req_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    goto_priv(PRIV_M);
    ai_fubp_write_bytes(low_paddr, gen2_payload, rewalk_index);
    ai_fubp_write_bytes(high_paddr, gen2_payload + rewalk_index,
        req_vl - rewalk_index);
    ai_fubp_fill_bytes(dump_paddr, 0x4cU, req_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        req_vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("VFOF PTE rewalk final no-read generation does not keep the repaired full-success or cleared-trigger state stale",
        excpt.triggered == false &&
        observed_vl == rewalk_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, gen2_payload, rewalk_index) &&
        ai_fubp_bytes_match(dump_paddr + rewalk_index,
            seed + rewalk_index, req_vl - rewalk_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_zero_active_us_fof_then_bp_no_ghost_flow_manual_corner()
{
    TEST_START();

    const enum test_page poison_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned us_trigger_index = 9U;
    const unsigned fof_fault_index = 10U;
    const uintptr_t poison_vaddr = hs_page_base(poison_page) + 0x500U;
    const uintptr_t poison2_vaddr = hs_page_base(high_page) + 0x80U;
    const uintptr_t us_src_vaddr = hs_page_base(data_page) + 0x600U;
    const uintptr_t us_old_vaddr = hs_page_base(old_page) + 0x680U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x700U;
    const uintptr_t mask_zero_vaddr = hs_page_base(aux_page) + 0x740U;
    const uintptr_t zero_fof_seed_vaddr = hs_page_base(aux_page) + 0x780U;
    const uintptr_t zero_fof_dump_vaddr = hs_page_base(aux_page) + 0x7c0U;
    const uintptr_t active_fof_vaddr = hs_page_base(poison_page) +
        PAGE_SIZE - fof_fault_index;
    const uintptr_t active_fof_seed_vaddr = hs_page_base(aux_page) + 0x800U;
    const uintptr_t active_fof_dump_vaddr = hs_page_base(aux_page) + 0x840U;
    const uintptr_t post_fof_seed_vaddr = hs_page_base(aux_page) + 0x880U;
    const uintptr_t post_fof_dump_vaddr = hs_page_base(aux_page) + 0x8c0U;
    const uintptr_t us_src_paddr = phys_page_base(data_page) + 0x600U;
    const uintptr_t us_old_paddr = phys_page_base(old_page) + 0x680U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x700U;
    const uintptr_t mask_zero_paddr = phys_page_base(aux_page) + 0x740U;
    const uintptr_t zero_fof_seed_paddr = phys_page_base(aux_page) + 0x780U;
    const uintptr_t zero_fof_dump_paddr = phys_page_base(aux_page) + 0x7c0U;
    const uintptr_t active_low_paddr = phys_page_base(poison_page) +
        PAGE_SIZE - fof_fault_index;
    const uintptr_t active_high_paddr = phys_page_base(high_page);
    const uintptr_t active_fof_seed_paddr = phys_page_base(aux_page) + 0x800U;
    const uintptr_t active_fof_dump_paddr = phys_page_base(aux_page) + 0x840U;
    const uintptr_t post_fof_seed_paddr = phys_page_base(aux_page) + 0x880U;
    const uintptr_t post_fof_dump_paddr = phys_page_base(aux_page) + 0x8c0U;
    const uint8_t mask_zero[2] = { 0x00U, 0x00U };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_prefix_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t zero_fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t active_fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t active_fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t post_fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, req_vl, 0x2cU);
    ai_fubp_fill_pattern(us_old, req_vl, 0xacU);
    ai_fubp_fill_pattern(zero_fof_seed, req_vl, 0xccU);
    ai_fubp_fill_pattern(active_fof_payload, req_vl, 0x5cU);
    ai_fubp_fill_pattern(active_fof_seed, req_vl, 0xdcU);
    ai_fubp_fill_pattern(post_fof_seed, req_vl, 0xecU);
    for (unsigned i = 0; i < req_vl; ++i) {
        us_prefix_expected[i] = (i < us_trigger_index) ?
            us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_src_paddr, us_payload, req_vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, req_vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x24U, req_vl);
    ai_fubp_write_bytes(mask_zero_paddr, mask_zero, sizeof(mask_zero));
    ai_fubp_write_bytes(zero_fof_seed_paddr, zero_fof_seed, req_vl);
    ai_fubp_fill_bytes(zero_fof_dump_paddr, 0x34U, req_vl);
    ai_fubp_write_bytes(active_low_paddr, active_fof_payload,
        fof_fault_index);
    ai_fubp_write_bytes(active_high_paddr,
        active_fof_payload + fof_fault_index, req_vl - fof_fault_index);
    ai_fubp_write_bytes(active_fof_seed_paddr, active_fof_seed, req_vl);
    ai_fubp_fill_bytes(active_fof_dump_paddr, 0x44U, req_vl);
    ai_fubp_write_bytes(post_fof_seed_paddr, post_fof_seed, req_vl);
    ai_fubp_fill_bytes(post_fof_dump_paddr, 0x54U, req_vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(poison_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(data_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, req_vl);
    TEST_ASSERT("zero-active setup preloads old v6 bytes before inactive US/FOF probes",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, poison_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(poison_vaddr, 0U);
    TEST_ASSERT("vl0 UnitStride load to a no-read trigger VA must not access memory or report a ghost breakpoint",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8_to_v6(poison_vaddr, mask_zero_vaddr, req_vl);
    TEST_ASSERT("all-masked UnitStride load to a no-read trigger VA must not access memory or report a ghost breakpoint",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, req_vl);
    TEST_ASSERT("zero-active UnitStride probes leave the destination vector undisturbed",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("zero-active UnitStride probes preserve the old vector bytes before the real breakpoint",
        ai_fubp_bytes_match(us_dump_paddr, us_old, req_vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_full_dump(zero_fof_seed_vaddr, poison_vaddr,
        mask_zero_vaddr, zero_fof_dump_vaddr, req_vl, &observed_vl,
        &observed_vstart);
    TEST_ASSERT("all-masked VFOF to a no-read trigger VA must not enqueue a shortened-vl or ghost breakpoint",
        excpt.triggered == false &&
        observed_vl == req_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(zero_fof_dump_paddr, zero_fof_seed, req_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, req_vl);
    TEST_ASSERT("zero-active follow-up reloads old v6 before the real active US breakpoint",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_trigger_index,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("real active US breakpoint after zero-active probes reports the fresh active lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_trigger_index &&
        observed_vstart_csr == us_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + us_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, req_vl);
    TEST_ASSERT("real active US breakpoint state can be dumped after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("real active US breakpoint after zero-active probes updates only the current prefix bytes",
        ai_fubp_bytes_match(us_dump_paddr, us_prefix_expected, req_vl)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(poison_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_fubp_fill_bytes(active_fof_dump_paddr, 0x64U, req_vl);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(active_fof_seed_vaddr, active_fof_vaddr,
        active_fof_dump_vaddr, req_vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("real VFOF first-fault before zero-active replay writes one fresh shortened-vl owner",
        excpt.triggered == false &&
        observed_vl == fof_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(active_fof_dump_paddr, active_fof_payload,
            fof_fault_index) &&
        ai_fubp_bytes_match(active_fof_dump_paddr + fof_fault_index,
            active_fof_seed + fof_fault_index, req_vl - fof_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_arm_mcontrol6_slot(0U, poison2_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_full_dump(post_fof_seed_vaddr, poison2_vaddr,
        mask_zero_vaddr, post_fof_dump_vaddr, req_vl, &observed_vl,
        &observed_vstart);
    TEST_ASSERT("all-masked VFOF after a real shortened-vl must not replay a stale fault, vl, or breakpoint",
        excpt.triggered == false &&
        observed_vl == req_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(post_fof_dump_paddr, post_fof_seed, req_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, req_vl);
    TEST_ASSERT("final active UnitStride load still completes after all zero-active and VFOF ghost-flow probes",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, req_vl);
    TEST_ASSERT("final active UnitStride dump completes after zero-active ghost-flow probes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("final active UnitStride image is fresh payload with no leftover zero-active or VFOF ghost effect",
        ai_fubp_bytes_match(us_dump_paddr, us_payload, req_vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_v0_mask_then_us_bp_consumes_fresh_mask_manual_corner()
{
    TEST_START();

    const enum test_page mask_low_page = SWITCH1;
    const enum test_page mask_high_page = SWITCH2;
    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned mask_fault_index = 2U;
    const unsigned stale_lane = 12U;
    const unsigned fresh_lane = 9U;
    const uintptr_t mask_src_vaddr = hs_page_base(mask_low_page) +
        PAGE_SIZE - mask_fault_index;
    const uintptr_t old_mask_vaddr = hs_page_base(aux_page) + 0x900U;
    const uintptr_t load_src_vaddr = hs_page_base(data_page) + 0x940U;
    const uintptr_t load_old_vaddr = hs_page_base(old_page) + 0x980U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x9c0U;
    const uintptr_t mask_src_paddr = phys_page_base(mask_low_page) +
        PAGE_SIZE - mask_fault_index;
    const uintptr_t mask_high_paddr = phys_page_base(mask_high_page);
    const uintptr_t old_mask_paddr = phys_page_base(aux_page) + 0x900U;
    const uintptr_t load_src_paddr = phys_page_base(data_page) + 0x940U;
    const uintptr_t load_old_paddr = phys_page_base(old_page) + 0x980U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x9c0U;
    const uint8_t old_mask[2] = { 0x00U, 0x10U };
    const uint8_t fresh_mask[2] = { 0x00U, 0x02U };
    const uint8_t repaired_mask[2] = { 0x00U, 0x10U };
    uint8_t load_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fresh_lane_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t stale_lane_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t repaired_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(load_payload, req_vl, 0x36U);
    ai_fubp_fill_pattern(old_payload, req_vl, 0xb6U);
    for (unsigned i = 0; i < req_vl; ++i) {
        fresh_lane_expected[i] = old_payload[i];
        stale_lane_expected[i] = old_payload[i];
        repaired_expected[i] = old_payload[i];
    }
    fresh_lane_expected[fresh_lane] = load_payload[fresh_lane];
    repaired_expected[stale_lane] = load_payload[stale_lane];

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(old_mask_paddr, old_mask, sizeof(old_mask));
    ai_fubp_write_bytes(mask_src_paddr, fresh_mask, sizeof(fresh_mask));
    ai_fubp_write_bytes(mask_high_paddr, repaired_mask, sizeof(repaired_mask));
    ai_fubp_write_bytes(load_src_paddr, load_payload, req_vl);
    ai_fubp_write_bytes(load_old_paddr, old_payload, req_vl);
    ai_fubp_fill_bytes(dump_paddr, 0x29U, req_vl);

    ai_fubp_prepare_three_pages(mask_low_page, PTE_V | PTE_AD | PTE_RWX,
        mask_high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(data_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_old_vaddr, req_vl);
    TEST_ASSERT("VFOF-v0 mask setup preloads old vector bytes before consuming the mask producer",
        excpt.triggered == false
    );

    ai_fubp_load_mask_to_v0(old_mask_vaddr, req_vl);
    ai_fubp_arm_mcontrol6_slot(0U, load_src_vaddr + stale_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_v0_then_masked_vle8_to_v6(mask_src_vaddr,
        load_src_vaddr, req_vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("VFOF-v0 shortened mask producer makes the old lane12 mask bit inactive so the stale-lane breakpoint is ignored",
        excpt.triggered == false &&
        observed_vl == mask_fault_index &&
        observed_vstart == 0U,
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, req_vl);
    TEST_ASSERT("VFOF-v0 fresh mask lane9 active only state can be dumped after stale-lane probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF-v0 fresh mask after shortened producer updates only lane9, proving lane12 came from stale v0 if it fires",
        ai_fubp_bytes_match(dump_paddr, fresh_lane_expected, req_vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, load_src_vaddr + fresh_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_old_vaddr, req_vl);
    TEST_ASSERT("VFOF-v0 active-lane check resets destination before probing the fresh mask active lane",
        excpt.triggered == false
    );
    ai_fubp_load_mask_to_v0(old_mask_vaddr, req_vl);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_v0_then_masked_vle8_to_v6(mask_src_vaddr,
        load_src_vaddr, req_vl, &observed_vl, &observed_vstart);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("VFOF-v0 shortened mask producer makes lane9 active and reports the fresh lane9 breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == load_src_vaddr + fresh_lane &&
        observed_vl == mask_fault_index &&
        observed_vstart == 0U &&
        observed_vstart_csr == fresh_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%lu",
        excpt.cause, excpt.tval, load_src_vaddr + fresh_lane,
        observed_vl, observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    ai_fubp_write_bytes(mask_src_paddr, repaired_mask, sizeof(repaired_mask));

    goto_priv(PRIV_HS);
    ai_fubp_prepare_three_pages(mask_low_page, PTE_V | PTE_AD | PTE_RWX,
        mask_high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(data_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_old_vaddr, req_vl);
    TEST_ASSERT("VFOF-v0 repaired-mask round resets destination before the full producer changes mask bits",
        excpt.triggered == false
    );

    ai_fubp_load_mask_to_v0(old_mask_vaddr, req_vl);
    ai_fubp_arm_mcontrol6_slot(0U, load_src_vaddr + stale_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_v0_then_masked_vle8_to_v6(mask_src_vaddr,
        load_src_vaddr, req_vl, &observed_vl, &observed_vstart);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("VFOF-v0 repaired full producer changes v0 so lane12 becomes the active breakpoint lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == load_src_vaddr + stale_lane &&
        observed_vl == req_vl &&
        observed_vstart == 0U &&
        observed_vstart_csr == stale_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%lu",
        excpt.cause, excpt.tval, load_src_vaddr + stale_lane,
        observed_vl, observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_old_vaddr, req_vl);
    TEST_ASSERT("VFOF-v0 final clear path resets destination before a repaired-mask no-trigger load",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_v0_then_masked_vle8_to_v6(mask_src_vaddr,
        load_src_vaddr, req_vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("VFOF-v0 repaired producer completes after clearing trigger and preserves the repaired lane12 mask",
        excpt.triggered == false &&
        observed_vl == req_vl &&
        observed_vstart == 0U,
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, req_vl);
    TEST_ASSERT("VFOF-v0 final dump completes after repaired-mask no-trigger load",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF-v0 final dump follows the repaired mask and updates only lane12",
        ai_fubp_bytes_match(dump_paddr, repaired_expected, req_vl) &&
        !ai_fubp_bytes_match(dump_paddr, stale_lane_expected, req_vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_segment_fof_us_bp_field_owner_resident_manual_corner()
{
    TEST_START();

    const enum test_page seg_low_page = SWITCH1;
    const enum test_page seg_high_page = SWITCH2;
    const enum test_page us_low_page = VSRWX_GRWX;
    const enum test_page us_high_page = VSRWX_GRW;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned us_trigger_index = 11U;
    const unsigned us_fault_index = 7U;
    const unsigned seg_fault_elem = 4U;
    const unsigned seg_trigger_elem = 2U;
    const unsigned seg_late_trigger_elem = 6U;
    const unsigned seg_low_bytes = 2U * seg_fault_elem;
    const uintptr_t us_src_vaddr = hs_page_base(us_low_page) + 0x700U;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0x80U;
    const uintptr_t us_bp_dump_vaddr = hs_page_base(aux_page) + 0xc0U;
    const uintptr_t us_fof_seed0_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t us_fof_dump0_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t us_fof_seed1_vaddr = hs_page_base(aux_page) + 0x180U;
    const uintptr_t us_fof_dump1_vaddr = hs_page_base(aux_page) + 0x1c0U;
    const uintptr_t seg_seed0_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t seg_seed1_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t seg_dump0_vaddr = hs_page_base(aux_page) + 0x2c0U;
    const uintptr_t seg_dump1_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t us_fof_src_vaddr = hs_page_base(us_low_page) +
        PAGE_SIZE - us_fault_index;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_low_page) +
        PAGE_SIZE - seg_low_bytes;
    const uintptr_t seg_field1_trigger_vaddr =
        seg_src_vaddr + 2U * seg_trigger_elem + 1U;
    const uintptr_t seg_late_field1_trigger_vaddr =
        seg_src_vaddr + 2U * seg_late_trigger_elem + 1U;
    const uintptr_t us_src_paddr = phys_page_base(us_low_page) + 0x700U;
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0x80U;
    const uintptr_t us_bp_dump_paddr = phys_page_base(aux_page) + 0xc0U;
    const uintptr_t us_fof_seed0_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t us_fof_dump0_paddr = phys_page_base(aux_page) + 0x140U;
    const uintptr_t us_fof_seed1_paddr = phys_page_base(aux_page) + 0x180U;
    const uintptr_t us_fof_dump1_paddr = phys_page_base(aux_page) + 0x1c0U;
    const uintptr_t seg_seed0_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t seg_seed1_paddr = phys_page_base(aux_page) + 0x280U;
    const uintptr_t seg_dump0_paddr = phys_page_base(aux_page) + 0x2c0U;
    const uintptr_t seg_dump1_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t us_fof_src_paddr = phys_page_base(us_low_page) +
        PAGE_SIZE - us_fault_index;
    const uintptr_t seg_src_paddr = phys_page_base(seg_low_page) +
        PAGE_SIZE - seg_low_bytes;
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_fof_seed0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_fof_seed1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seg_field0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seg_field1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seg_seed0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seg_seed1[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;

    ai_fubp_fill_pattern(us_payload, req_vl, 0x21U);
    ai_fubp_fill_pattern(us_old, req_vl, 0xa1U);
    ai_fubp_fill_pattern(us_fof_payload, req_vl, 0x42U);
    ai_fubp_fill_pattern(us_fof_seed0, req_vl, 0xc2U);
    ai_fubp_fill_pattern(us_fof_seed1, req_vl, 0xd2U);
    ai_fubp_fill_pattern(seg_field0, req_vl, 0x63U);
    ai_fubp_fill_pattern(seg_field1, req_vl, 0x93U);
    ai_fubp_fill_pattern(seg_seed0, req_vl, 0xe3U);
    ai_fubp_fill_pattern(seg_seed1, req_vl, 0xf3U);
    for (unsigned i = 0; i < req_vl; ++i) {
        us_bp_expected[i] = (i < us_trigger_index) ?
            us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_src_paddr, us_payload, req_vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, req_vl);
    ai_fubp_write_bytes(us_fof_src_paddr, us_fof_payload,
        us_fault_index);
    ai_fubp_write_bytes(us_fof_seed0_paddr, us_fof_seed0, req_vl);
    ai_fubp_write_bytes(us_fof_seed1_paddr, us_fof_seed1, req_vl);
    for (unsigned lane = 0; lane < seg_fault_elem; ++lane) {
        write8(seg_src_paddr + 2U * lane, seg_field0[lane]);
        write8(seg_src_paddr + 2U * lane + 1U, seg_field1[lane]);
    }
    ai_fubp_write_bytes(seg_seed0_paddr, seg_seed0, req_vl);
    ai_fubp_write_bytes(seg_seed1_paddr, seg_seed1, req_vl);
    ai_fubp_fill_bytes(us_bp_dump_paddr, 0x31U, req_vl);
    ai_fubp_fill_bytes(us_fof_dump0_paddr, 0x32U, req_vl);
    ai_fubp_fill_bytes(us_fof_dump1_paddr, 0x33U, req_vl);
    ai_fubp_fill_bytes(seg_dump0_paddr, 0x34U, req_vl);
    ai_fubp_fill_bytes(seg_dump1_paddr, 0x35U, req_vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(seg_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(seg_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(us_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, req_vl);
    TEST_ASSERT("Segment-FOF cross setup preloads old UnitStride bytes before the first address breakpoint",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_trigger_index,
        AI_FUBP_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("Segment-FOF cross UnitStride probe reports its own lane before any segment field owner is allocated",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_trigger_index &&
        observed_vstart_csr == us_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + us_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_bp_dump_vaddr, req_vl);
    TEST_ASSERT("Segment-FOF cross UnitStride breakpoint prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF cross UnitStride breakpoint prefix is not overwritten by a stale segment owner",
        ai_fubp_bytes_match(us_bp_dump_paddr, us_bp_expected, req_vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(us_fof_seed0_vaddr, us_fof_src_vaddr,
        us_fof_dump0_vaddr, req_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF cross first UnitStride FOF resident writes one shortened-vl before segment owner reuse",
        excpt.triggered == false &&
        observed_vl == us_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(us_fof_dump0_paddr, us_fof_payload,
            us_fault_index) &&
        ai_fubp_bytes_match(us_fof_dump0_paddr + us_fault_index,
            us_fof_seed0 + us_fault_index, req_vl - us_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_arm_mcontrol6_slot(0U, seg_field1_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump(seg_seed0_vaddr, seg_seed1_vaddr,
        seg_src_vaddr, seg_dump0_vaddr, seg_dump1_vaddr, req_vl,
        &observed_vl, &observed_vstart);
    TEST_ASSERT("Segment-FOF field1 prefix address breakpoint reports the field VA and element vstart, not the prior UnitStride FOF VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == seg_field1_trigger_vaddr &&
        ai_fubp_bytes_match(seg_dump0_paddr, seg_field0,
            seg_trigger_elem) &&
        ai_fubp_bytes_match(seg_dump1_paddr, seg_field1,
            seg_trigger_elem) &&
        (read8(seg_dump0_paddr + seg_trigger_elem) ==
            seg_seed0[seg_trigger_elem] ||
            read8(seg_dump0_paddr + seg_trigger_elem) ==
            seg_field0[seg_trigger_elem]) &&
        read8(seg_dump1_paddr + seg_trigger_elem) ==
            seg_seed1[seg_trigger_elem] &&
        ai_fubp_bytes_match(seg_dump0_paddr + seg_trigger_elem + 1U,
            seg_seed0 + seg_trigger_elem + 1U,
            req_vl - seg_trigger_elem - 1U) &&
        ai_fubp_bytes_match(seg_dump1_paddr + seg_trigger_elem + 1U,
            seg_seed1 + seg_trigger_elem + 1U,
            req_vl - seg_trigger_elem - 1U),
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u old_us_fault_va=0x%lx",
        excpt.cause, excpt.tval, seg_field1_trigger_vaddr,
        observed_vl, observed_vstart, us_fof_src_vaddr + us_fault_index
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, seg_late_field1_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(seg_dump0_paddr, 0x44U, req_vl);
    ai_fubp_fill_bytes(seg_dump1_paddr, 0x45U, req_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump(seg_seed0_vaddr, seg_seed1_vaddr,
        seg_src_vaddr, seg_dump0_vaddr, seg_dump1_vaddr, req_vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF late field trigger beyond the first-fault element is suppressed as a silent shortened-vl",
        excpt.triggered == false &&
        observed_vl == seg_fault_elem &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(seg_dump0_paddr, seg_field0,
            seg_fault_elem) &&
        ai_fubp_bytes_match(seg_dump1_paddr, seg_field1,
            seg_fault_elem) &&
        ai_fubp_bytes_match(seg_dump0_paddr + seg_fault_elem,
            seg_seed0 + seg_fault_elem, req_vl - seg_fault_elem) &&
        ai_fubp_bytes_match(seg_dump1_paddr + seg_fault_elem,
            seg_seed1 + seg_fault_elem, req_vl - seg_fault_elem),
        "triggered=%d cause=0x%lx tval=0x%lx late_trigger=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval,
        seg_late_field1_trigger_vaddr, observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_fill_bytes(us_fof_dump1_paddr, 0x46U, req_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(us_fof_seed1_vaddr, us_fof_src_vaddr,
        us_fof_dump1_vaddr, req_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF cross second UnitStride FOF keeps its own element owner after segment field-owner pressure",
        excpt.triggered == false &&
        observed_vl == us_fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(us_fof_dump1_paddr, us_fof_payload,
            us_fault_index) &&
        ai_fubp_bytes_match(us_fof_dump1_paddr + us_fault_index,
            us_fof_seed1 + us_fault_index, req_vl - us_fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u seg_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, seg_fault_elem
    );

    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_same_pc_loop_us_fof_bp_rearm_iteration_fresh_manual_corner()
{
    TEST_START();

    enum {
        AI_FUBP_LOOP_ITERS = 3
    };
    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned us_trigger_index[AI_FUBP_LOOP_ITERS] = { 3U, 9U, 5U };
    const unsigned fof_trigger_index[AI_FUBP_LOOP_ITERS] = { 2U, 6U, 4U };
    const unsigned fof_fault_index[AI_FUBP_LOOP_ITERS] = { 6U, 11U, 8U };
    const uintptr_t us_offset[AI_FUBP_LOOP_ITERS] = {
        0x640U, 0x780U, 0x8c0U
    };
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x480U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x4c0U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x500U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x540U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0x580U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x480U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x4c0U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x500U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x540U;
    uint8_t us_payload[AI_FUBP_LOOP_ITERS][AI_FUBP_FULL_VL] = { { 0 } };
    uint8_t us_old[AI_FUBP_LOOP_ITERS][AI_FUBP_FULL_VL] = { { 0 } };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_LOOP_ITERS][AI_FUBP_FULL_VL] = { { 0 } };
    uint8_t fof_seed[AI_FUBP_LOOP_ITERS][AI_FUBP_FULL_VL] = { { 0 } };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    for (unsigned iter = 0; iter < AI_FUBP_LOOP_ITERS; ++iter) {
        ai_fubp_fill_pattern(us_payload[iter], req_vl,
            (uint8_t)(0x24U + iter * 0x21U));
        ai_fubp_fill_pattern(us_old[iter], req_vl,
            (uint8_t)(0x94U + iter * 0x17U));
        ai_fubp_fill_pattern(fof_payload[iter], req_vl,
            (uint8_t)(0x54U + iter * 0x1dU));
        ai_fubp_fill_pattern(fof_seed[iter], req_vl,
            (uint8_t)(0xc4U + iter * 0x13U));
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_fill_bytes(marker_paddr, 0x00U, AI_FUBP_LOOP_ITERS);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    for (unsigned iter = 0; iter < AI_FUBP_LOOP_ITERS; ++iter) {
        const uintptr_t us_src_vaddr = hs_page_base(us_page) +
            us_offset[iter];
        const uintptr_t us_src_paddr = phys_page_base(us_page) +
            us_offset[iter];
        const uintptr_t fof_src_vaddr = hs_page_base(low_page) +
            PAGE_SIZE - fof_fault_index[iter];
        const uintptr_t fof_src_paddr = phys_page_base(low_page) +
            PAGE_SIZE - fof_fault_index[iter];

        for (unsigned i = 0; i < req_vl; ++i) {
            us_expected[i] = (i < us_trigger_index[iter]) ?
                us_payload[iter][i] : us_old[iter][i];
        }

        goto_priv(PRIV_M);
        ai_fubp_write_bytes(us_src_paddr, us_payload[iter], req_vl);
        ai_fubp_write_bytes(old_paddr, us_old[iter], req_vl);
        ai_fubp_write_bytes(fof_src_paddr, fof_payload[iter],
            fof_fault_index[iter]);
        ai_fubp_write_bytes(seed_paddr, fof_seed[iter], req_vl);
        ai_fubp_fill_bytes(us_dump_paddr, (uint8_t)(0x30U + iter),
            req_vl);
        ai_fubp_fill_bytes(fof_dump_paddr, (uint8_t)(0x40U + iter),
            req_vl);
        ai_fubp_clear_mcontrol6_slot(0U);
        ai_fubp_arm_mcontrol6_slot(0U,
            us_src_vaddr + us_trigger_index[iter],
            AI_FUBP_MCONTROL6_LOAD);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_fubp_vle8_to_v6(old_vaddr, req_vl);
        TEST_ASSERT("same-PC loop preloads the iteration-specific old vector image before the repeated US breakpoint call-site",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        ai_fubp_vle8_to_v6(us_src_vaddr, req_vl);
        observed_vstart_csr = CSRR(CSR_VSTART);
        TEST_ASSERT("same-PC loop US breakpoint metadata is fresh for this iteration's base and lane",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP &&
            excpt.tval == us_src_vaddr + us_trigger_index[iter] &&
            observed_vstart_csr == us_trigger_index[iter],
            "iter=%u cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
            iter, excpt.cause, excpt.tval,
            us_src_vaddr + us_trigger_index[iter], observed_vstart_csr
        );

        ai_fubp_clear_mcontrol6_slot(0U);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_fubp_vse8_from_v6(us_dump_vaddr, req_vl);
        TEST_ASSERT("same-PC loop US dump completes after per-iteration trigger clear",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("same-PC loop US prefix/suffix image belongs only to the current iteration",
            ai_fubp_bytes_match(us_dump_paddr, us_expected, req_vl),
            "iter=%u trigger=%u lane0=0x%x lane15=0x%x",
            iter, us_trigger_index[iter], read8(us_dump_paddr),
            read8(us_dump_paddr + 15U)
        );

        ai_fubp_arm_mcontrol6_slot(0U,
            fof_src_vaddr + fof_trigger_index[iter],
            AI_FUBP_MCONTROL6_LOAD);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_fubp_vle8ff_only(fof_src_vaddr, req_vl);
        observed_vstart_csr = CSRR(CSR_VSTART);
        TEST_ASSERT("same-PC loop VFOF breakpoint rearm uses this iteration's trigger VA before the later first-fault lane",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP &&
            excpt.tval == fof_src_vaddr + fof_trigger_index[iter] &&
            observed_vstart_csr == fof_trigger_index[iter],
            "iter=%u cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
            iter, excpt.cause, excpt.tval,
            fof_src_vaddr + fof_trigger_index[iter], observed_vstart_csr
        );

        ai_fubp_clear_mcontrol6_slot(0U);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_fubp_vle8ff_full_dump(seed_vaddr, fof_src_vaddr,
            fof_dump_vaddr, req_vl, &observed_vl, &observed_vstart);

        goto_priv(PRIV_M);
        TEST_ASSERT("same-PC loop VFOF shortened-vl and dump are fresh after clearing this iteration's breakpoint",
            excpt.triggered == false &&
            observed_vl == fof_fault_index[iter] &&
            observed_vstart == 0U &&
            ai_fubp_bytes_match(fof_dump_paddr, fof_payload[iter],
                fof_fault_index[iter]) &&
            ai_fubp_bytes_match(fof_dump_paddr + fof_fault_index[iter],
                fof_seed[iter] + fof_fault_index[iter],
                req_vl - fof_fault_index[iter]),
            "iter=%u triggered=%d cause=0x%lx tval=0x%lx vl=%u expected_vl=%u",
            iter, excpt.triggered, excpt.cause, excpt.tval,
            observed_vl, fof_fault_index[iter]
        );
        write8(marker_paddr + iter, (uint8_t)(0x80U + iter));
    }

    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_clear_mcontrol6_slot(0U);

    goto_priv(PRIV_M);
    TEST_ASSERT("same-PC loop completed every metadata-refresh iteration exactly once",
        read8(marker_paddr) == 0x80U &&
        read8(marker_paddr + 1U) == 0x81U &&
        read8(marker_paddr + 2U) == 0x82U
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_high_fullva_vfof_bp_tval_no_truncation_manual_corner()
{
    TEST_START();

    const enum test_page old_page = SWITCH1;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page backing_page = VSRWX_GRWX;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned trigger_index = 10U;
    const uintptr_t high_page_vaddr = ai_fubp_sv48_last_page_vaddr();
    const uintptr_t high_base_vaddr = high_page_vaddr + 0x700U;
    const uintptr_t high_trigger_vaddr = high_base_vaddr + trigger_index;
    const uintptr_t high_fault_vaddr = high_page_vaddr + 0x800U;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x100U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t backing_paddr = phys_page_base(backing_page);
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x100U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x140U;
    uint8_t high_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_prefix_expected[AI_FUBP_FULL_VL] = { 0 };
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(high_payload, req_vl, 0x3eU);
    ai_fubp_fill_pattern(old_payload, req_vl, 0xbeU);
    for (unsigned i = 0; i < req_vl; ++i) {
        us_prefix_expected[i] = (i < trigger_index) ?
            high_payload[i] : old_payload[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(backing_paddr + 0x700U, high_payload, req_vl);
    ai_fubp_write_bytes(old_paddr, old_payload, req_vl);
    ai_fubp_fill_bytes(dump_paddr, 0x5eU, req_vl);

    ai_fubp_install_sv48_high_page(backing_paddr,
        PTE_V | PTE_AD | PTE_RWX);
    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, req_vl);
    TEST_ASSERT("high fullVA setup preloads old vector bytes before high-VA unit-stride trigger probe",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, high_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(high_base_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("high fullVA UnitStride breakpoint preserves the Sv48 high canonical tval instead of truncating to low offset",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == high_trigger_vaddr &&
        observed_vstart_csr == trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, high_trigger_vaddr,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, req_vl);
    TEST_ASSERT("high fullVA UnitStride prefix can be dumped after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("high fullVA UnitStride breakpoint writes only the current high-VA prefix and leaves suffix old",
        ai_fubp_bytes_match(dump_paddr, us_prefix_expected, req_vl)
    );

    ai_fubp_arm_mcontrol6_slot(0U, high_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(high_base_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("high fullVA VFOF breakpoint uses the full high canonical address before any shortened-vl path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == high_trigger_vaddr &&
        observed_vstart_csr == trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, high_trigger_vaddr,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    ai_fubp_install_sv48_high_page(backing_paddr,
        PTE_V | PTE_AD | PTE_X);
    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(high_fault_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("high fullVA VFOF element0 no-read fault reports the current high canonical tval after clearing the lane10 breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart_csr == 0U,
        "triggered=%d cause=0x%lx tval=0x%lx expected=0x%lx old_bp=0x%lx vstart=%lu",
        excpt.triggered, excpt.cause, excpt.tval, high_fault_vaddr,
        high_trigger_vaddr, observed_vstart_csr
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_fof_us_bp_type_filter_nonzero_fault_same_va_manual_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH1;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned fault_index = 6U;
    const unsigned load_trigger_index = 3U;
    const unsigned store_vl = 12U;
    const unsigned store_trigger_index = 9U;
    const uintptr_t target_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fault_index;
    const uintptr_t target_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fault_index;
    const uintptr_t src0_vaddr = hs_page_base(src_page) + 0x180U;
    const uintptr_t src1_vaddr = hs_page_base(src_page) + 0x1c0U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x500U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x540U;
    const uintptr_t src0_paddr = phys_page_base(src_page) + 0x180U;
    const uintptr_t src1_paddr = phys_page_base(src_page) + 0x1c0U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x500U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x540U;
    uint8_t initial[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t expected_store_prefix[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(initial, sizeof(initial), 0x28U);
    ai_fubp_fill_pattern(seed, sizeof(seed), 0xb8U);
    ai_fubp_fill_pattern(store_payload0, sizeof(store_payload0), 0x58U);
    ai_fubp_fill_pattern(store_payload1, sizeof(store_payload1), 0xd8U);
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        expected_store_prefix[i] = (i < store_trigger_index) ?
            store_payload1[i] : store_payload0[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(target_paddr, initial, req_vl);
    ai_fubp_write_bytes(src0_paddr, store_payload0, store_vl);
    ai_fubp_write_bytes(src1_paddr, store_payload1, store_vl);
    ai_fubp_write_bytes(seed_paddr, seed, req_vl);
    ai_fubp_fill_bytes(dump_paddr, 0x61U, req_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_arm_mcontrol6_slot(0U,
        target_vaddr + load_trigger_index, AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, target_vaddr, dump_vaddr,
        req_vl, &observed_vl, &observed_vstart);
    goto_priv(PRIV_M);
    TEST_ASSERT("store-only address trigger must not catch the same-VA VFOF load, which should shorten at the real nonzero fault lane",
        excpt.triggered == false &&
        observed_vl == fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, initial, fault_index) &&
        ai_fubp_bytes_match(dump_paddr + fault_index,
            seed + fault_index, req_vl - fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(dump_paddr, 0x62U, req_vl);
    ai_fubp_arm_mcontrol6_slot(0U,
        target_vaddr + load_trigger_index, AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(target_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("load-only address trigger must catch the same-VA VFOF load before the later nonzero first-fault lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target_vaddr + load_trigger_index &&
        observed_vstart_csr == load_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, target_vaddr + load_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(dump_paddr, 0x63U, req_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, target_vaddr, dump_vaddr,
        req_vl, &observed_vl, &observed_vstart);
    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the load-only trigger, the same-VA VFOF retry exposes the original nonzero first fault and fresh prefix data",
        excpt.triggered == false &&
        observed_vl == fault_index &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, initial, fault_index) &&
        ai_fubp_bytes_match(dump_paddr + fault_index,
            seed + fault_index, req_vl - fault_index),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_arm_mcontrol6_slot(0U,
        target_vaddr + store_trigger_index, AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src0_vaddr, store_vl);
    TEST_ASSERT("same-VA type-filter store setup loads payload0 while a load-only trigger is armed on the destination",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, store_vl);
    TEST_ASSERT("load-only address trigger must not catch the same-VA UnitStride store path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-VA UnitStride store fully updates target bytes when only the load-qualified trigger is armed",
        ai_fubp_bytes_match(target_paddr, store_payload0, store_vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_arm_mcontrol6_slot(0U,
        target_vaddr + store_trigger_index, AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src1_vaddr, store_vl);
    TEST_ASSERT("same-VA type-filter store setup reloads payload1 before the store-qualified trigger round",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, store_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("store-only address trigger catches the same-VA UnitStride store at the new store lane without reusing VFOF load state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target_vaddr + store_trigger_index &&
        observed_vstart_csr == store_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, target_vaddr + store_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("store-only breakpoint commits only payload1 prefix and keeps the suffix from the previous full store",
        ai_fubp_bytes_match(target_paddr, expected_store_prefix, store_vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src1_vaddr, store_vl);
    TEST_ASSERT("same-VA store retry reloads payload1 after clearing the store trigger",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, store_vl);
    TEST_ASSERT("same-VA store retry completes after the store trigger is cleared",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-VA type-filter final image is payload1 with no ghost VFOF suffix or stale store owner",
        ai_fubp_bytes_match(target_paddr, store_payload1, store_vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_fof_us_bp_rearm_same_va_type_filter_manual_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page src_page = SWITCH1;
    const enum test_page dump_page = SCRATCHPAD;
    const unsigned vl = 8U;
    const unsigned load_trigger_index = 3U;
    const unsigned store_trigger_index = 5U;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x900U;
    const uintptr_t store_src0_vaddr = hs_page_base(src_page) + 0xa00U;
    const uintptr_t store_src1_vaddr = hs_page_base(src_page) + 0xa80U;
    const uintptr_t fof_seed_vaddr = hs_page_base(dump_page) + 0xb00U;
    const uintptr_t fof_dump_vaddr = hs_page_base(dump_page) + 0xb40U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x900U;
    const uintptr_t store_src0_paddr = phys_page_base(src_page) + 0xa00U;
    const uintptr_t store_src1_paddr = phys_page_base(src_page) + 0xa80U;
    const uintptr_t fof_seed_paddr = phys_page_base(dump_page) + 0xb00U;
    const uintptr_t fof_dump_paddr = phys_page_base(dump_page) + 0xb40U;
    uint8_t initial_target[16] = { 0 };
    uint8_t store_payload0[16] = { 0 };
    uint8_t store_payload1[16] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t expected_prefix_store[16] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(initial_target, vl, 0x30U);
    ai_fubp_fill_pattern(store_payload0, vl, 0x80U);
    ai_fubp_fill_pattern(store_payload1, vl, 0xd0U);
    ai_fubp_fill_pattern(seed, AI_FUBP_FULL_VL, 0x11U);
    for (unsigned i = 0; i < vl; ++i) {
        expected_prefix_store[i] = (i < store_trigger_index) ?
            store_payload1[i] : store_payload0[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(target_paddr, initial_target, vl);
    ai_fubp_write_bytes(store_src0_paddr, store_payload0, vl);
    ai_fubp_write_bytes(store_src1_paddr, store_payload1, vl);
    ai_fubp_write_bytes(fof_seed_paddr, seed, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x44U, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    ai_fubp_arm_mcontrol6_slot(0U, target_vaddr,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, target_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("same-VA type filter keeps a store-only address trigger from catching the VFOF load path",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, initial_target, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, target_vaddr + load_trigger_index,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(target_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("same-VA rearm makes the load-qualified VFOF breakpoint visible at the newly programmed lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target_vaddr + load_trigger_index &&
        observed_vstart_csr == load_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, target_vaddr + load_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, target_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("same-VA clear after VFOF breakpoint lets the same load complete once without stale trigger metadata",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, initial_target, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl
    );

    ai_fubp_arm_mcontrol6_slot(0U, target_vaddr + store_trigger_index,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src0_vaddr, vl);
    TEST_ASSERT("same-VA type filter store setup loads source data while the load-only trigger is armed on the destination VA",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, vl);
    TEST_ASSERT("same-VA type filter keeps a load-only address trigger from catching the unit-stride store path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-VA type filter store path writes the full payload when only the load trigger type is armed",
        ai_fubp_bytes_match(target_paddr, store_payload0, vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, target_vaddr + store_trigger_index,
        AI_FUBP_MCONTROL6_STORE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src1_vaddr, vl);
    TEST_ASSERT("same-VA rearm store setup reloads a distinct source payload before the store-qualified trigger fires",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("same-VA rearm makes the store-qualified breakpoint visible at the new lane without reusing the load trigger state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target_vaddr + store_trigger_index &&
        observed_vstart_csr == store_trigger_index,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, target_vaddr + store_trigger_index,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("same-VA store breakpoint cuts off precisely at the store trigger lane and preserves the suffix from the previous completed store",
        ai_fubp_bytes_match(target_paddr, expected_prefix_store, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src1_vaddr, vl);
    TEST_ASSERT("same-VA store retry reloads the final source after clearing the store trigger",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, vl);
    TEST_ASSERT("same-VA store retry completes exactly once after the store trigger is cleared",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-VA rearm/clear leaves the final memory image equal to the final store payload with no ghost VFOF load side effect",
        ai_fubp_bytes_match(target_paddr, store_payload1, vl) &&
        ai_fubp_bytes_match(fof_dump_paddr, initial_target, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_same_us_load_store_early_bp_late_fault_priority_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = 16U;
    const unsigned load_trigger_lane = 4U;
    const unsigned store_trigger_lane = 5U;
    const unsigned split_at = 8U;
    const unsigned load_fault_lane = split_at;
    const unsigned store_fault_lane = split_at;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        split_at;
    const uintptr_t base_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        split_at;
    const uintptr_t base_high_paddr = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x600U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x640U;
    const uintptr_t src_vaddr = hs_page_base(aux_page) + 0x680U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x600U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x640U;
    const uintptr_t src_paddr = phys_page_base(aux_page) + 0x680U;
    uint8_t load_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t load_prefix_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t initial_target[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(load_payload, vl, 0x21U);
    ai_fubp_fill_pattern(old_payload, vl, 0x91U);
    ai_fubp_fill_pattern(initial_target, vl, 0x41U);
    ai_fubp_fill_pattern(store_payload, vl, 0xc1U);
    for (unsigned i = 0; i < vl; ++i) {
        load_prefix_expected[i] = (i < load_trigger_lane) ?
            load_payload[i] : old_payload[i];
        store_bp_expected[i] = (i < store_trigger_lane) ?
            store_payload[i] : initial_target[i];
        store_fault_expected[i] = (i < store_fault_lane) ?
            store_payload[i] : initial_target[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(base_low_paddr, load_payload, split_at);
    ai_fubp_write_bytes(base_high_paddr, load_payload + split_at,
        vl - split_at);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(src_paddr, store_payload, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x5cU, vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("same-US early-BP setup preloads old vector bytes before the load priority probe",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, base_vaddr + load_trigger_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("same UnitStride load reports the earlier breakpoint lane before the later no-read page fault lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == base_vaddr + load_trigger_lane &&
        observed_vstart_csr == load_trigger_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu late_fault=0x%lx",
        excpt.cause, excpt.tval, base_vaddr + load_trigger_lane,
        observed_vstart_csr, base_vaddr + load_fault_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("same-US early-BP load dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same UnitStride load breakpoint keeps only the prefix before the trigger and does not consume the later fault lane",
        ai_fubp_bytes_match(dump_paddr, load_prefix_expected, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("same UnitStride load exposes the later page fault after the earlier breakpoint is cleared",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == base_vaddr + split_at &&
        observed_vstart_csr == split_at,
        "cause=0x%lx tval=0x%lx expected_fault=0x%lx vstart=%lu late_fault_lane=%u",
        excpt.cause, excpt.tval, base_vaddr + split_at,
        observed_vstart_csr, load_fault_lane
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(base_vaddr, vl);
    TEST_ASSERT("same-US load repair completes after both the trigger and later fault are removed",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("same-US load repair dump completes for full-image checking",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-US load repair produces the full payload without stale breakpoint/fault priority state",
        ai_fubp_bytes_match(dump_paddr, load_payload, vl)
    );

    ai_fubp_write_bytes(base_low_paddr, initial_target, split_at);
    ai_fubp_write_bytes(base_high_paddr, initial_target + split_at,
        vl - split_at);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, vl);
    TEST_ASSERT("same-US early store-BP setup loads source payload before arming the store trigger",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, base_vaddr + store_trigger_lane,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("same UnitStride store reports the earlier store breakpoint and suppresses the later store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == base_vaddr + store_trigger_lane &&
        observed_vstart_csr == store_trigger_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu late_fault=0x%lx",
        excpt.cause, excpt.tval, base_vaddr + store_trigger_lane,
        observed_vstart_csr, base_vaddr + store_fault_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("same UnitStride store early breakpoint commits only prefix lanes before the trigger and leaves the late-fault suffix old",
        ai_fubp_split_bytes_match(base_low_paddr, base_high_paddr,
            split_at, store_bp_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, vl);
    TEST_ASSERT("same-US store retry reloads source after clearing the early breakpoint",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("same UnitStride store retry exposes the later no-write page fault after clearing the earlier breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == base_vaddr + split_at &&
        observed_vstart_csr == split_at,
        "cause=0x%lx tval=0x%lx expected_fault=0x%lx vstart=%lu late_fault_lane=%u",
        excpt.cause, excpt.tval, base_vaddr + split_at,
        observed_vstart_csr, store_fault_lane
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same UnitStride store late fault commits only prefix lanes before the fault and does not keep the old breakpoint cutoff",
        ai_fubp_split_bytes_match(base_low_paddr, base_high_paddr,
            split_at, store_fault_expected, vl)
    );

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, vl);
    TEST_ASSERT("same-US store final retry reloads source after repairing the late page fault",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(base_vaddr, vl);
    TEST_ASSERT("same-US store final retry completes after trigger clear and page repair",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-US load/store priority final image is the complete store payload with no stale cutoff or fault owner",
        ai_fubp_split_bytes_match(base_low_paddr, base_high_paddr,
            split_at, store_payload, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_wide_eew_partial_byte_trigger_element_index_manual_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = 8U;
    const unsigned bytes_per_elem = 2U;
    const unsigned load_trigger_elem = 3U;
    const unsigned fof_trigger_elem = 4U;
    const unsigned fof_fault_elem = 6U;
    const unsigned store_trigger_elem = 5U;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x900U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x900U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x980U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x9c0U;
    const uintptr_t store_src_vaddr = hs_page_base(aux_page) + 0xa00U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0xa40U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xa80U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x980U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x9c0U;
    const uintptr_t store_src_paddr = phys_page_base(aux_page) + 0xa00U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0xa40U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xa80U;
    const uintptr_t fof_base_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_elem * bytes_per_elem;
    const uintptr_t fof_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_elem * bytes_per_elem;
    const uintptr_t fof_high_paddr = phys_page_base(high_page);
    const unsigned total_bytes = vl * bytes_per_elem;
    uint8_t load_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t load_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_initial[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(load_payload, total_bytes, 0x27U);
    ai_fubp_fill_pattern(old_payload, total_bytes, 0xa7U);
    ai_fubp_fill_pattern(fof_payload, total_bytes, 0x47U);
    ai_fubp_fill_pattern(fof_seed, total_bytes, 0xc7U);
    ai_fubp_fill_pattern(store_initial, total_bytes, 0x67U);
    ai_fubp_fill_pattern(store_payload, total_bytes, 0xe7U);
    for (unsigned i = 0; i < total_bytes; ++i) {
        load_expected[i] = (i < load_trigger_elem * bytes_per_elem) ?
            load_payload[i] : old_payload[i];
        store_expected[i] = (i < store_trigger_elem * bytes_per_elem) ?
            store_payload[i] : store_initial[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(target_paddr, load_payload, total_bytes);
    ai_fubp_write_bytes(old_paddr, old_payload, total_bytes);
    ai_fubp_write_bytes(store_src_paddr, store_payload, total_bytes);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload,
        fof_fault_elem * bytes_per_elem);
    ai_fubp_write_bytes(fof_high_paddr,
        fof_payload + fof_fault_elem * bytes_per_elem,
        total_bytes - fof_fault_elem * bytes_per_elem);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, total_bytes);
    ai_fubp_fill_bytes(dump_paddr, 0x34U, total_bytes);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x74U, total_bytes);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(old_vaddr, vl);
    TEST_ASSERT("wide-EEW setup preloads old e16 vector bytes before the partial-byte load trigger probe",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U,
        target_vaddr + load_trigger_elem * bytes_per_elem + 1U,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(target_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e16 UnitStride load high-byte trigger reports the byte tval but the element vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target_vaddr +
            load_trigger_elem * bytes_per_elem + 1U &&
        observed_vstart_csr == load_trigger_elem,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu elem=%u",
        excpt.cause, excpt.tval,
        target_vaddr + load_trigger_elem * bytes_per_elem + 1U,
        observed_vstart_csr, load_trigger_elem
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_from_v6(dump_vaddr, vl);
    TEST_ASSERT("wide-EEW load dump completes after clearing the high-byte trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("e16 load high-byte trigger cuts off at the whole element rather than at the byte offset",
        ai_fubp_bytes_match(dump_paddr, load_expected, total_bytes)
    );

    ai_fubp_arm_mcontrol6_slot(0U,
        fof_base_vaddr + fof_trigger_elem * bytes_per_elem + 1U,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_only(fof_base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e16 VFOF high-byte trigger reports the partial-byte tval before the later nonzero first-fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_base_vaddr +
            fof_trigger_elem * bytes_per_elem + 1U &&
        observed_vstart_csr == fof_trigger_elem,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu elem=%u",
        excpt.cause, excpt.tval,
        fof_base_vaddr + fof_trigger_elem * bytes_per_elem + 1U,
        observed_vstart_csr, fof_trigger_elem
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_full_dump(fof_seed_vaddr, fof_base_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("e16 VFOF high-byte trigger clear exposes the later first-fault with element-granular vl and byte-accurate prefix",
        excpt.triggered == false &&
        observed_vl == fof_fault_elem &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload,
            fof_fault_elem * bytes_per_elem) &&
        ai_fubp_bytes_match(fof_dump_paddr +
            fof_fault_elem * bytes_per_elem,
            fof_seed + fof_fault_elem * bytes_per_elem,
            total_bytes - fof_fault_elem * bytes_per_elem),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u expected_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        fof_fault_elem
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_full_dump(fof_seed_vaddr, fof_base_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("e16 VFOF repair reloads the full halfword payload after the partial-byte trigger and fault are removed",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, total_bytes)
    );

    ai_fubp_write_bytes(target_paddr, store_initial, total_bytes);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(store_src_vaddr, vl);
    TEST_ASSERT("wide-EEW store setup loads e16 source payload before the partial-byte store trigger",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U,
        target_vaddr + store_trigger_elem * bytes_per_elem + 1U,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_from_v6(target_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e16 UnitStride store high-byte trigger reports the byte tval but cuts off at the element index",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target_vaddr +
            store_trigger_elem * bytes_per_elem + 1U &&
        observed_vstart_csr == store_trigger_elem,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu elem=%u",
        excpt.cause, excpt.tval,
        target_vaddr + store_trigger_elem * bytes_per_elem + 1U,
        observed_vstart_csr, store_trigger_elem
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("e16 store high-byte trigger commits only whole elements before the trigger and leaves the trigger element old",
        ai_fubp_bytes_match(target_paddr, store_expected, total_bytes)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(store_src_vaddr, vl);
    TEST_ASSERT("wide-EEW store retry reloads source after clearing the partial-byte trigger",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_from_v6(target_vaddr, vl);
    TEST_ASSERT("wide-EEW store retry completes after the high-byte trigger is cleared",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("wide-EEW partial-byte trigger final store image has no stale byte-offset or element-index cutoff",
        ai_fubp_bytes_match(target_paddr, store_payload, total_bytes)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_vl_tail_boundary_trigger_active_only_manual_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page seed_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page fault_low_page = VSRWX_GRWX;
    const enum test_page fault_high_page = VSRWX_GRW;
    const unsigned active_vl = 10U;
    const unsigned full_vl = AI_FUBP_FULL_VL;
    const unsigned last_active = active_vl - 1U;
    const unsigned tail_start = active_vl;
    const unsigned fault_lane = 7U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x740U;
    const uintptr_t seed_vaddr = hs_page_base(seed_page) + 0x780U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x7c0U;
    const uintptr_t fault_vaddr = hs_page_base(fault_low_page) +
        PAGE_SIZE - fault_lane;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x740U;
    const uintptr_t seed_paddr = phys_page_base(seed_page) + 0x780U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x7c0U;
    const uintptr_t fault_low_paddr = phys_page_base(fault_low_page) +
        PAGE_SIZE - fault_lane;
    const uintptr_t fault_high_paddr = phys_page_base(fault_high_page);
    uint8_t payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fault_payload[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(payload, full_vl, 0x39U);
    ai_fubp_fill_pattern(seed, full_vl, 0xb9U);
    ai_fubp_fill_pattern(fault_payload, full_vl, 0x59U);
    for (unsigned i = 0; i < full_vl; ++i) {
        bp_expected[i] = (i < last_active) ? payload[i] : seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(src_paddr, payload, full_vl);
    ai_fubp_write_bytes(seed_paddr, seed, full_vl);
    ai_fubp_write_bytes(fault_low_paddr, fault_payload, fault_lane);
    ai_fubp_write_bytes(fault_high_paddr, fault_payload + fault_lane,
        full_vl - fault_lane);
    ai_fubp_fill_bytes(dump_paddr, 0x2aU, full_vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(seed_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fault_high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    ai_fubp_arm_mcontrol6_slot(0U, src_vaddr + tail_start,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_active_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        active_vl, full_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF address trigger at element vl is outside the active range and must not fire or touch tail bytes",
        excpt.triggered == false &&
        observed_vl == active_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, payload, active_vl) &&
        ai_fubp_bytes_match(dump_paddr + active_vl,
            seed + active_vl, full_vl - active_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u active_vl=%u tail=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        active_vl, src_vaddr + tail_start
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, src_vaddr + last_active,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(src_vaddr, active_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("VFOF address trigger at vl-1 is active and reports the last active element instead of tail-start",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == src_vaddr + last_active &&
        observed_vstart_csr == last_active,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, src_vaddr + last_active,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(dump_paddr, 0x3aU, full_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_active_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        active_vl, full_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF clear after vl-1 breakpoint completes the active range and keeps tail-start bytes seeded",
        excpt.triggered == false &&
        observed_vl == active_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, payload, active_vl) &&
        ai_fubp_bytes_match(dump_paddr + active_vl,
            seed + active_vl, full_vl - active_vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u active_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        active_vl
    );

    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(dump_paddr, 0x4aU, full_vl);
    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fault_high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_active_full_dump(seed_vaddr, fault_vaddr, dump_vaddr,
        active_vl, full_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF first-fault before tail-start shortens vl only to the active fault lane and leaves tail bytes seeded",
        excpt.triggered == false &&
        observed_vl == fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, fault_payload, fault_lane) &&
        ai_fubp_bytes_match(dump_paddr + fault_lane,
            seed + fault_lane, full_vl - fault_lane),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u fault_lane=%u lane6=0x%x lane7=0x%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        fault_lane, read8(dump_paddr + fault_lane - 1U),
        read8(dump_paddr + fault_lane)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(seed_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_write_bytes(src_paddr, payload, full_vl);
    ai_fubp_fill_bytes(dump_paddr, 0x5aU, full_vl);
    ai_fubp_arm_mcontrol6_slot(0U, src_vaddr + last_active,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_active_full_dump(seed_vaddr, src_vaddr, dump_vaddr,
        active_vl, full_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF vl-boundary final active breakpoint path preserves prefix and tail seed after page repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == src_vaddr + last_active &&
        observed_vstart == last_active &&
        ai_fubp_bytes_match(dump_paddr, bp_expected, full_vl),
        "triggered=%d cause=0x%lx tval=0x%lx expected=0x%lx vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval,
        src_vaddr + last_active, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_segment_fof_vstart_field_trigger_absolute_index_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned start = 3U;
    const unsigned trigger_seg = 6U;
    const unsigned fault_seg = 9U;
    const unsigned fault_low_bytes = fault_seg * 2U + 1U;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fault_low_bytes;
    const uintptr_t field1_trigger_vaddr = src_vaddr + trigger_seg * 2U +
        1U;
    const uintptr_t seed0_vaddr = hs_page_base(aux_page) + 0x820U;
    const uintptr_t seed1_vaddr = hs_page_base(aux_page) + 0x860U;
    const uintptr_t dump0_vaddr = hs_page_base(aux_page) + 0x8a0U;
    const uintptr_t dump1_vaddr = hs_page_base(aux_page) + 0x8e0U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fault_low_bytes;
    const uintptr_t seed0_paddr = phys_page_base(aux_page) + 0x820U;
    const uintptr_t seed1_paddr = phys_page_base(aux_page) + 0x860U;
    const uintptr_t dump0_paddr = phys_page_base(aux_page) + 0x8a0U;
    const uintptr_t dump1_paddr = phys_page_base(aux_page) + 0x8e0U;
    uint8_t field0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t field1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed1[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(field0, req_vl, 0x4dU);
    ai_fubp_fill_pattern(field1, req_vl, 0x8dU);
    ai_fubp_fill_pattern(seed0, req_vl, 0xcdU);
    ai_fubp_fill_pattern(seed1, req_vl, 0xedU);

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    for (unsigned seg = 0; seg <= fault_seg; ++seg) {
        write8(low_paddr + seg * 2U, field0[seg]);
        if (seg < fault_seg) {
            write8(low_paddr + seg * 2U + 1U, field1[seg]);
        }
    }
    ai_fubp_write_bytes(seed0_paddr, seed0, req_vl);
    ai_fubp_write_bytes(seed1_paddr, seed1, req_vl);
    ai_fubp_fill_bytes(dump0_paddr, 0x6dU, req_vl);
    ai_fubp_fill_bytes(dump1_paddr, 0x7dU, req_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_arm_mcontrol6_slot(0U, field1_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump_with_vstart(seed0_vaddr, seed1_vaddr,
        src_vaddr, dump0_vaddr, dump1_vaddr, req_vl, start,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF field1 address breakpoint after nonzero vstart reports the absolute segment index",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == field1_trigger_vaddr &&
        observed_vstart == trigger_seg &&
        ai_fubp_bytes_match(dump0_paddr, seed0, start) &&
        ai_fubp_bytes_match(dump1_paddr, seed1, start) &&
        ai_fubp_bytes_match(dump0_paddr + start, field0 + start,
            trigger_seg - start) &&
        ai_fubp_bytes_match(dump1_paddr + start, field1 + start,
            trigger_seg - start) &&
        read8(dump0_paddr + trigger_seg) == seed0[trigger_seg] &&
        read8(dump1_paddr + trigger_seg) == seed1[trigger_seg],
        "cause=0x%lx tval=0x%lx expected=0x%lx observed_vl=%u observed_vstart=%u",
        excpt.cause, excpt.tval, field1_trigger_vaddr,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(dump0_paddr, 0x8dU, req_vl);
    ai_fubp_fill_bytes(dump1_paddr, 0x9dU, req_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump_with_vstart(seed0_vaddr, seed1_vaddr,
        src_vaddr, dump0_vaddr, dump1_vaddr, req_vl, start,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF clear-trigger retry shortens vl to the absolute field1 fault segment and preserves prefix before vstart",
        excpt.triggered == false &&
        observed_vl == fault_seg &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump0_paddr, seed0, start) &&
        ai_fubp_bytes_match(dump1_paddr, seed1, start) &&
        ai_fubp_bytes_match(dump0_paddr + start, field0 + start,
            fault_seg - start) &&
        ai_fubp_bytes_match(dump1_paddr + start, field1 + start,
            fault_seg - start) &&
        ai_fubp_bytes_match(dump0_paddr + fault_seg,
            seed0 + fault_seg, req_vl - fault_seg) &&
        ai_fubp_bytes_match(dump1_paddr + fault_seg,
            seed1 + fault_seg, req_vl - fault_seg),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u start=%u fault_seg=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, start, fault_seg
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_high_only_mask_us_trigger_then_fof_split_clean_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page fof_low_page = VSRWX_GRWX;
    const enum test_page fof_high_page = VSRWX_GRW;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned split = 8U;
    const unsigned trigger_lane = 12U;
    const unsigned fof_fault_lane = 11U;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE - split;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x920U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x960U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x9a0U;
    const uintptr_t fof_base_vaddr = hs_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x9c0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xa00U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - split;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x920U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x960U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x9a0U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x9c0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xa00U;
    const uint8_t high_only_mask[2] = { 0x00U, 0xffU };
    uint8_t payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t prefix_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(payload, vl, 0x31U);
    ai_fubp_fill_pattern(old, vl, 0xa1U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xd1U);
    for (unsigned i = 0; i < vl; ++i) {
        prefix_expected[i] = (i >= split && i < trigger_lane) ?
            payload[i] : old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, payload, split);
    ai_fubp_write_bytes(high_paddr, payload + split, vl - split);
    ai_fubp_write_bytes(fof_low_paddr, payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(old_paddr, old, vl);
    ai_fubp_write_bytes(mask_paddr, high_only_mask,
        sizeof(high_only_mask));
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x41U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x51U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("high-only mask setup preloads old v6 before the inactive-low-flow trigger probe",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, base_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8_to_v6(base_vaddr, mask_vaddr, vl);
    TEST_ASSERT("high-only masked US does not access the inactive low flow even when low page is no-read and trigger-armed",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, base_vaddr + trigger_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("high-only mask reloads old v6 before checking that the active high flow is not skipped",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8_to_v6(base_vaddr, mask_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("high-only masked US reports the active high-lane breakpoint after skipping the inactive low flow",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == base_vaddr + trigger_lane &&
        observed_vstart_csr == trigger_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, base_vaddr + trigger_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("high-only masked US dump completes after clearing the active high breakpoint",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("high-only masked US keeps low-flow lanes old and only commits active high prefix before the breakpoint",
        ai_fubp_bytes_match(dump_paddr, prefix_expected, vl)
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_base_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF after high-only masked US observes the current split state and shortens only at its own high-page fault lane",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, payload, fof_fault_lane) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_lane,
            fof_seed + fof_fault_lane, vl - fof_fault_lane),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_lmul_m2_vfof_group_boundary_bp_neighbor_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_M2_VL;
    const unsigned group_boundary = AI_FUBP_FULL_VL;
    const unsigned fault_lane = group_boundary + 4U;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fault_lane;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xa40U;
    const uintptr_t neighbor_seed_vaddr = hs_page_base(aux_page) + 0xa80U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xac0U;
    const uintptr_t neighbor_dump_vaddr = hs_page_base(aux_page) + 0xb00U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fault_lane;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xa40U;
    const uintptr_t neighbor_seed_paddr = phys_page_base(aux_page) + 0xa80U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xac0U;
    const uintptr_t neighbor_dump_paddr = phys_page_base(aux_page) + 0xb00U;
    uint8_t payload[AI_FUBP_M2_VL] = { 0 };
    uint8_t seed[AI_FUBP_M2_VL] = { 0 };
    uint8_t expected_bp[AI_FUBP_M2_VL] = { 0 };
    uint8_t neighbor_seed[AI_FUBP_M2_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(payload, vl, 0x23U);
    ai_fubp_fill_pattern(seed, vl, 0xb3U);
    ai_fubp_fill_pattern(neighbor_seed, vl, 0xe3U);
    for (unsigned i = 0; i < vl; ++i) {
        expected_bp[i] = (i < group_boundary) ? payload[i] : seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, payload, fault_lane);
    ai_fubp_write_bytes(high_paddr, payload + fault_lane,
        vl - fault_lane);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_write_bytes(neighbor_seed_paddr, neighbor_seed, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x33U, vl);
    ai_fubp_fill_bytes(neighbor_dump_paddr, 0x43U, vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_arm_mcontrol6_slot(0U, base_vaddr + group_boundary,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_m2_group_dump(seed_vaddr, neighbor_seed_vaddr,
        base_vaddr, dump_vaddr, neighbor_dump_vaddr, vl, &observed_vl,
        &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("LMUL m2 VFOF breakpoint at the v8/v9 group boundary reports the absolute element and leaves neighbor v10 intact",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == base_vaddr + group_boundary &&
        observed_vstart == group_boundary &&
        ai_fubp_bytes_match(dump_paddr, expected_bp, vl) &&
        ai_fubp_bytes_match(neighbor_dump_paddr, neighbor_seed, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, base_vaddr + group_boundary,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(dump_paddr, 0x53U, vl);
    ai_fubp_fill_bytes(neighbor_dump_paddr, 0x63U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_m2_group_dump(seed_vaddr, neighbor_seed_vaddr,
        base_vaddr, dump_vaddr, neighbor_dump_vaddr, vl, &observed_vl,
        &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("LMUL m2 VFOF first-fault after the group boundary shortens vl without clobbering suffix or neighbor registers",
        excpt.triggered == false &&
        observed_vl == fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, payload, fault_lane) &&
        ai_fubp_bytes_match(dump_paddr + fault_lane,
            seed + fault_lane, vl - fault_lane) &&
        ai_fubp_bytes_match(neighbor_dump_paddr, neighbor_seed, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u fault_lane=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, fault_lane
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_final_vl_then_vsetvli_us_bp_order_manual_corner()
{
    TEST_START();

    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned fof_vl = AI_FUBP_FULL_VL;
    const unsigned fof_fault_lane = 6U;
    const unsigned us_vl = 12U;
    const unsigned us_bp_lane = 10U;
    const uintptr_t fof_src_vaddr = hs_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xf40U;
    const uintptr_t us_src_vaddr = hs_page_base(us_page) + 0xf80U;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0xfc0U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x1000U;
    const uintptr_t fof_src_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xf40U;
    const uintptr_t us_src_paddr = phys_page_base(us_page) + 0xf80U;
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0xfc0U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x1000U;
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_fof_vl = 0U;
    unsigned observed_fof_vstart = 0U;
    unsigned observed_post_vset_vl = 0U;
    unsigned observed_post_us_vl = 0U;
    unsigned observed_post_us_vstart = 0U;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(fof_payload, fof_vl, 0x39U);
    ai_fubp_fill_pattern(seed, fof_vl, 0xb9U);
    ai_fubp_fill_pattern(us_payload, us_vl, 0x49U);
    ai_fubp_fill_pattern(us_old, us_vl, 0xc9U);
    for (unsigned i = 0; i < us_vl; ++i) {
        us_expected[i] = (i < us_bp_lane) ? us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(fof_src_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(seed_paddr, seed, fof_vl);
    ai_fubp_write_bytes(us_src_paddr, us_payload, us_vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, us_vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x59U, us_vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, us_vl);
    TEST_ASSERT("VFOF-final-vl order setup preloads UnitStride old bytes before the ordered CSR probe",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_csrr_vl_then_vset_us(seed_vaddr, fof_src_vaddr,
        us_src_vaddr, fof_vl, us_vl, &observed_fof_vl,
        &observed_fof_vstart, &observed_post_vset_vl,
        &observed_post_us_vl, &observed_post_us_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF final vl is visible to the following csrr and cannot late-overwrite the following vsetvli before US breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_bp_lane &&
        observed_fof_vl == fof_fault_lane &&
        observed_fof_vstart == 0U &&
        observed_post_vset_vl == us_vl &&
        observed_post_us_vl == us_vl &&
        observed_post_us_vstart == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx fof_vl=%u fof_vstart=%u post_vset_vl=%u post_us_vl=%u post_us_vstart=%u",
        excpt.cause, excpt.tval, us_src_vaddr + us_bp_lane,
        observed_fof_vl, observed_fof_vstart, observed_post_vset_vl,
        observed_post_us_vl, observed_post_us_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, us_vl);
    TEST_ASSERT("VFOF-final-vl order dump completes after clearing the ordered US breakpoint",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF final-vl ordering lets the following US use the new vsetvli vl and commit only the breakpoint prefix",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, us_vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_segment_fof_nonzero_vstart_us_neighbor_owner_manual_corner()
{
    TEST_START();

    const enum test_page seg_low_page = SWITCH1;
    const enum test_page seg_high_page = SWITCH2;
    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned start = 3U;
    const unsigned us_pre_trigger = 11U;
    const unsigned us_post_trigger = 5U;
    const unsigned seg_trigger_elem = 7U;
    const unsigned seg_fault_elem = 10U;
    const unsigned seg_low_bytes = seg_fault_elem * 2U + 1U;
    const uintptr_t us_src_vaddr = hs_page_base(us_page) + 0x780U;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0xb40U;
    const uintptr_t us_pre_dump_vaddr = hs_page_base(aux_page) + 0xb80U;
    const uintptr_t us_post_dump_vaddr = hs_page_base(aux_page) + 0xbc0U;
    const uintptr_t seg_src_vaddr = hs_page_base(seg_low_page) +
        PAGE_SIZE - seg_low_bytes;
    const uintptr_t seg_seed0_vaddr = hs_page_base(aux_page) + 0xc00U;
    const uintptr_t seg_seed1_vaddr = hs_page_base(aux_page) + 0xc40U;
    const uintptr_t seg_dump0_vaddr = hs_page_base(aux_page) + 0xc80U;
    const uintptr_t seg_dump1_vaddr = hs_page_base(aux_page) + 0xcc0U;
    const uintptr_t seg_field1_trigger_vaddr =
        seg_src_vaddr + seg_trigger_elem * 2U + 1U;
    const uintptr_t us_src_paddr = phys_page_base(us_page) + 0x780U;
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0xb40U;
    const uintptr_t us_pre_dump_paddr = phys_page_base(aux_page) + 0xb80U;
    const uintptr_t us_post_dump_paddr = phys_page_base(aux_page) + 0xbc0U;
    const uintptr_t seg_src_paddr = phys_page_base(seg_low_page) +
        PAGE_SIZE - seg_low_bytes;
    const uintptr_t seg_seed0_paddr = phys_page_base(aux_page) + 0xc00U;
    const uintptr_t seg_seed1_paddr = phys_page_base(aux_page) + 0xc40U;
    const uintptr_t seg_dump0_paddr = phys_page_base(aux_page) + 0xc80U;
    const uintptr_t seg_dump1_paddr = phys_page_base(aux_page) + 0xcc0U;
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_pre_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_post_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seg_field0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seg_field1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seg_seed0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seg_seed1[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, req_vl, 0x2bU);
    ai_fubp_fill_pattern(us_old, req_vl, 0x9bU);
    ai_fubp_fill_pattern(seg_field0, req_vl, 0x4bU);
    ai_fubp_fill_pattern(seg_field1, req_vl, 0x7bU);
    ai_fubp_fill_pattern(seg_seed0, req_vl, 0xbbU);
    ai_fubp_fill_pattern(seg_seed1, req_vl, 0xdbU);
    for (unsigned i = 0; i < req_vl; ++i) {
        us_pre_expected[i] = (i < us_pre_trigger) ?
            us_payload[i] : us_old[i];
        us_post_expected[i] = (i < us_post_trigger) ?
            us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_src_paddr, us_payload, req_vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, req_vl);
    for (unsigned elem = 0; elem <= seg_fault_elem; ++elem) {
        write8(seg_src_paddr + elem * 2U, seg_field0[elem]);
        if (elem < seg_fault_elem) {
            write8(seg_src_paddr + elem * 2U + 1U,
                seg_field1[elem]);
        }
    }
    ai_fubp_write_bytes(seg_seed0_paddr, seg_seed0, req_vl);
    ai_fubp_write_bytes(seg_seed1_paddr, seg_seed1, req_vl);
    ai_fubp_fill_bytes(us_pre_dump_paddr, 0x2cU, req_vl);
    ai_fubp_fill_bytes(us_post_dump_paddr, 0x3cU, req_vl);
    ai_fubp_fill_bytes(seg_dump0_paddr, 0x4cU, req_vl);
    ai_fubp_fill_bytes(seg_dump1_paddr, 0x5cU, req_vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(seg_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(seg_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, req_vl);
    TEST_ASSERT("Segment vstart neighbor setup preloads UnitStride old bytes before the pre-segment breakpoint",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_pre_trigger,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("pre-segment UnitStride breakpoint reports its own lane and VA before Segment-FOF allocates a field owner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_pre_trigger &&
        observed_vstart_csr == us_pre_trigger,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + us_pre_trigger,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_pre_dump_vaddr, req_vl);
    TEST_ASSERT("pre-segment UnitStride breakpoint leaves only the prefix committed before Segment-FOF pressure",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("pre-segment UnitStride prefix is independent from later Segment-FOF field owner state",
        ai_fubp_bytes_match(us_pre_dump_paddr, us_pre_expected, req_vl)
    );

    ai_fubp_arm_mcontrol6_slot(0U, seg_field1_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump_with_vstart(seg_seed0_vaddr,
        seg_seed1_vaddr, seg_src_vaddr, seg_dump0_vaddr,
        seg_dump1_vaddr, req_vl, start, &observed_vl,
        &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF nonzero vstart field1 breakpoint uses element index and field VA instead of the neighboring US lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == seg_field1_trigger_vaddr &&
        observed_vstart == seg_trigger_elem &&
        ai_fubp_bytes_match(seg_dump0_paddr, seg_seed0, start) &&
        ai_fubp_bytes_match(seg_dump1_paddr, seg_seed1, start) &&
        ai_fubp_bytes_match(seg_dump0_paddr + start,
            seg_field0 + start, seg_trigger_elem - start) &&
        ai_fubp_bytes_match(seg_dump1_paddr + start,
            seg_field1 + start, seg_trigger_elem - start) &&
        read8(seg_dump0_paddr + seg_trigger_elem) ==
            seg_seed0[seg_trigger_elem] &&
        read8(seg_dump1_paddr + seg_trigger_elem) ==
            seg_seed1[seg_trigger_elem],
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u us_pre_va=0x%lx",
        excpt.cause, excpt.tval, seg_field1_trigger_vaddr,
        observed_vl, observed_vstart, us_src_vaddr + us_pre_trigger
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_fill_bytes(seg_dump0_paddr, 0x6cU, req_vl);
    ai_fubp_fill_bytes(seg_dump1_paddr, 0x7cU, req_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump_with_vstart(seg_seed0_vaddr,
        seg_seed1_vaddr, seg_src_vaddr, seg_dump0_vaddr,
        seg_dump1_vaddr, req_vl, start, &observed_vl,
        &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF clear-trigger retry shortens vl at the absolute field1 fault element and preserves prefix before vstart",
        excpt.triggered == false &&
        observed_vl == seg_fault_elem &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(seg_dump0_paddr, seg_seed0, start) &&
        ai_fubp_bytes_match(seg_dump1_paddr, seg_seed1, start) &&
        ai_fubp_bytes_match(seg_dump0_paddr + start,
            seg_field0 + start, seg_fault_elem - start) &&
        ai_fubp_bytes_match(seg_dump1_paddr + start,
            seg_field1 + start, seg_fault_elem - start) &&
        ai_fubp_bytes_match(seg_dump0_paddr + seg_fault_elem,
            seg_seed0 + seg_fault_elem, req_vl - seg_fault_elem) &&
        ai_fubp_bytes_match(seg_dump1_paddr + seg_fault_elem,
            seg_seed1 + seg_fault_elem, req_vl - seg_fault_elem),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u fault_elem=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, seg_fault_elem
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_post_trigger,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, req_vl);
    TEST_ASSERT("post-segment UnitStride owner check resets destination after Segment-FOF retry",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, req_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("post-segment UnitStride breakpoint reports the current US lane and does not inherit Segment-FOF field VA or vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_post_trigger &&
        observed_vstart_csr == us_post_trigger,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu seg_field_va=0x%lx",
        excpt.cause, excpt.tval, us_src_vaddr + us_post_trigger,
        observed_vstart_csr, seg_field1_trigger_vaddr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_post_dump_vaddr, req_vl);
    TEST_ASSERT("post-segment UnitStride breakpoint prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("post-segment UnitStride prefix is still indexed by the US lane, not by Segment-FOF field owner",
        ai_fubp_bytes_match(us_post_dump_paddr, us_post_expected, req_vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_final_vl_wb_mask_no_clobber_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned fault_lane = 9U;
    const unsigned us_bp_lane = 6U;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fault_lane;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xd00U;
    const uintptr_t mask_seed_vaddr = hs_page_base(aux_page) + 0xd40U;
    const uintptr_t neighbor_seed_vaddr = hs_page_base(aux_page) + 0xd80U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xdc0U;
    const uintptr_t mask_dump_vaddr = hs_page_base(aux_page) + 0xe00U;
    const uintptr_t neighbor_dump_vaddr = hs_page_base(aux_page) + 0xe40U;
    const uintptr_t us_src_vaddr = hs_page_base(us_page) + 0xe80U;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0xec0U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0xf00U;
    const uintptr_t fof_src_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fault_lane;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xd00U;
    const uintptr_t mask_seed_paddr = phys_page_base(aux_page) + 0xd40U;
    const uintptr_t neighbor_seed_paddr = phys_page_base(aux_page) + 0xd80U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xdc0U;
    const uintptr_t mask_dump_paddr = phys_page_base(aux_page) + 0xe00U;
    const uintptr_t neighbor_dump_paddr = phys_page_base(aux_page) + 0xe40U;
    const uintptr_t us_src_paddr = phys_page_base(us_page) + 0xe80U;
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0xec0U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0xf00U;
    const uint8_t mask_seed[2] = { 0x5aU, 0xa5U };
    uint8_t payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t neighbor_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(payload, vl, 0x35U);
    ai_fubp_fill_pattern(seed, vl, 0xb5U);
    ai_fubp_fill_pattern(neighbor_seed, vl, 0xe5U);
    ai_fubp_fill_pattern(us_payload, vl, 0x45U);
    ai_fubp_fill_pattern(us_old, vl, 0xc5U);
    for (unsigned i = 0; i < vl; ++i) {
        us_expected[i] = (i < us_bp_lane) ? us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(fof_src_paddr, payload, fault_lane);
    ai_fubp_write_bytes(high_paddr, payload + fault_lane,
        vl - fault_lane);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_write_bytes(mask_seed_paddr, mask_seed, sizeof(mask_seed));
    ai_fubp_write_bytes(neighbor_seed_paddr, neighbor_seed, vl);
    ai_fubp_write_bytes(us_src_paddr, us_payload, vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x55U, vl);
    ai_fubp_fill_bytes(mask_dump_paddr, 0x65U, sizeof(mask_seed));
    ai_fubp_fill_bytes(neighbor_dump_paddr, 0x75U, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x85U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_dump_with_v0_neighbor(seed_vaddr, mask_seed_vaddr,
        neighbor_seed_vaddr, fof_src_vaddr, fof_dump_vaddr,
        mask_dump_vaddr, neighbor_dump_vaddr, vl, &observed_vl,
        &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF final-vl writeback shortens vl without treating the all-ones writeback mask as vector data",
        excpt.triggered == false &&
        observed_vl == fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, payload, fault_lane) &&
        ai_fubp_bytes_match(fof_dump_paddr + fault_lane,
            seed + fault_lane, vl - fault_lane) &&
        ai_fubp_bytes_match(mask_dump_paddr, mask_seed,
            sizeof(mask_seed)) &&
        ai_fubp_bytes_match(neighbor_dump_paddr, neighbor_seed, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u fault_lane=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, fault_lane
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("post-VFOF final-vl neighbor probe preloads UnitStride old bytes",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("post-VFOF UnitStride breakpoint reports its own lane and does not consume the final-vl writeback mask",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("post-VFOF UnitStride breakpoint prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("post-VFOF UnitStride prefix image is not expanded by a stale all-ones VFOF writeback mask",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_redirect_kill_then_correct_path_bp_manual_corner()
{
    TEST_START();

    const enum test_page wrong_page = SWITCH1;
    const enum test_page wrong_high_page = SWITCH2;
    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page right_low_page = VSRW_GRWX;
    const enum test_page right_high_page = VSRW_GRW;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned wrong_fault_lane = 5U;
    const unsigned right_fault_lane = 11U;
    const unsigned us_bp_lane = 9U;
    const uintptr_t wrong_fof_vaddr = hs_page_base(wrong_page) +
        PAGE_SIZE - wrong_fault_lane;
    const uintptr_t us_src_vaddr = hs_page_base(us_page) + 0x340U;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0x380U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x3c0U;
    const uintptr_t right_fof_vaddr = hs_page_base(right_low_page) +
        PAGE_SIZE - right_fault_lane;
    const uintptr_t right_seed_vaddr = hs_page_base(aux_page) + 0x400U;
    const uintptr_t right_dump_vaddr = hs_page_base(aux_page) + 0x440U;
    const uintptr_t wrong_low_paddr = phys_page_base(wrong_page) +
        PAGE_SIZE - wrong_fault_lane;
    const uintptr_t wrong_high_paddr = phys_page_base(wrong_high_page);
    const uintptr_t us_src_paddr = phys_page_base(us_page) + 0x340U;
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0x380U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x3c0U;
    const uintptr_t right_low_paddr = phys_page_base(right_low_page) +
        PAGE_SIZE - right_fault_lane;
    const uintptr_t right_high_paddr = phys_page_base(right_high_page);
    const uintptr_t right_seed_paddr = phys_page_base(aux_page) + 0x400U;
    const uintptr_t right_dump_paddr = phys_page_base(aux_page) + 0x440U;
    uint8_t wrong_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t right_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t right_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(wrong_payload, vl, 0x3dU);
    ai_fubp_fill_pattern(us_payload, vl, 0x5dU);
    ai_fubp_fill_pattern(us_old, vl, 0xbdU);
    ai_fubp_fill_pattern(right_payload, vl, 0x7dU);
    ai_fubp_fill_pattern(right_seed, vl, 0xddU);
    for (unsigned i = 0; i < vl; ++i) {
        us_expected[i] = (i < us_bp_lane) ? us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(wrong_low_paddr, wrong_payload, wrong_fault_lane);
    ai_fubp_write_bytes(wrong_high_paddr, wrong_payload + wrong_fault_lane,
        vl - wrong_fault_lane);
    ai_fubp_write_bytes(us_src_paddr, us_payload, vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x6dU, vl);
    ai_fubp_write_bytes(right_low_paddr, right_payload, right_fault_lane);
    ai_fubp_write_bytes(right_high_paddr, right_payload + right_fault_lane,
        vl - right_fault_lane);
    ai_fubp_write_bytes(right_seed_paddr, right_seed, vl);
    ai_fubp_fill_bytes(right_dump_paddr, 0x8dU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(wrong_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(wrong_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(right_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(right_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_branch_wrong_vfof_right_us(wrong_fof_vaddr, us_src_vaddr,
        us_dump_vaddr, vl, 0U);
    TEST_ASSERT("redirect-kill setup executes the not-taken wrong-path VFOF without creating a resident entry",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("redirect-kill owner check preloads UnitStride old bytes before the correct-path breakpoint",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_branch_wrong_vfof_right_us(wrong_fof_vaddr, us_src_vaddr,
        us_dump_vaddr, vl, 1U);

    goto_priv(PRIV_M);
    TEST_ASSERT("redirect kills wrong-path VFOF before final-vl writeback and correct-path US reports its own breakpoint owner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_bp_lane &&
        CSRR(CSR_VSTART) == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu wrong_fault_lane=%u",
        excpt.cause, excpt.tval, us_src_vaddr + us_bp_lane,
        CSRR(CSR_VSTART), wrong_fault_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("redirect-kill correct-path US breakpoint prefix can be dumped after clearing trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("redirect-kill correct-path US prefix is not clipped by the killed wrong-path VFOF shortened-vl",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(right_seed_vaddr, right_fof_vaddr,
        right_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("redirect-kill follow-up VFOF enqueues a fresh entry and shortens at the correct-path fault lane",
        excpt.triggered == false &&
        observed_vl == right_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(right_dump_paddr, right_payload,
            right_fault_lane) &&
        ai_fubp_bytes_match(right_dump_paddr + right_fault_lane,
            right_seed + right_fault_lane, vl - right_fault_lane),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u killed_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, wrong_fault_lane
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vtype_e8_e16_trigger_scale_refresh_manual_corner()
{
    TEST_START();

    const enum test_page e8_us_page = VSRWX_GRWX;
    const enum test_page e16_fof_low_page = SWITCH1;
    const enum test_page e16_fof_high_page = SWITCH2;
    const enum test_page e16_us_page = VSRW_GRWX;
    const enum test_page e8_fof_low_page = VSRWX_GRW;
    const enum test_page e8_fof_high_page = VSRW_GRW;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned e8_vl = AI_FUBP_FULL_VL;
    const unsigned e16_vl = AI_FUBP_FULL_VL;
    const unsigned same_byte_offset = 8U;
    const unsigned e8_bp_lane = same_byte_offset;
    const unsigned e16_bp_lane = same_byte_offset / 2U;
    const unsigned e16_fof_fault_lane = 10U;
    const unsigned e8_fof_fault_lane = 12U;
    const unsigned e16_total_bytes = e16_vl * 2U;
    const uintptr_t e8_us_vaddr = hs_page_base(e8_us_page) + 0xa80U;
    const uintptr_t e16_fof_vaddr = hs_page_base(e16_fof_low_page) +
        PAGE_SIZE - e16_fof_fault_lane * 2U;
    const uintptr_t e16_us_vaddr = hs_page_base(e16_us_page) + 0xac0U;
    const uintptr_t e8_fof_vaddr = hs_page_base(e8_fof_low_page) +
        PAGE_SIZE - e8_fof_fault_lane;
    const uintptr_t e8_us_old_vaddr = hs_page_base(aux_page) + 0x900U;
    const uintptr_t e8_us_dump_vaddr = hs_page_base(aux_page) + 0x940U;
    const uintptr_t e16_fof_seed_vaddr = hs_page_base(aux_page) + 0x980U;
    const uintptr_t e16_fof_dump_vaddr = hs_page_base(aux_page) + 0x9c0U;
    const uintptr_t e16_us_old_vaddr = hs_page_base(aux_page) + 0xa00U;
    const uintptr_t e16_us_dump_vaddr = hs_page_base(aux_page) + 0xa40U;
    const uintptr_t e8_fof_seed_vaddr = hs_page_base(aux_page) + 0xa80U;
    const uintptr_t e8_fof_dump_vaddr = hs_page_base(aux_page) + 0xac0U;
    const uintptr_t e8_us_paddr = phys_page_base(e8_us_page) + 0xa80U;
    const uintptr_t e16_fof_low_paddr =
        phys_page_base(e16_fof_low_page) + PAGE_SIZE -
        e16_fof_fault_lane * 2U;
    const uintptr_t e16_fof_high_paddr =
        phys_page_base(e16_fof_high_page);
    const uintptr_t e16_us_paddr = phys_page_base(e16_us_page) + 0xac0U;
    const uintptr_t e8_fof_low_paddr =
        phys_page_base(e8_fof_low_page) + PAGE_SIZE - e8_fof_fault_lane;
    const uintptr_t e8_fof_high_paddr = phys_page_base(e8_fof_high_page);
    const uintptr_t e8_us_old_paddr = phys_page_base(aux_page) + 0x900U;
    const uintptr_t e8_us_dump_paddr = phys_page_base(aux_page) + 0x940U;
    const uintptr_t e16_fof_seed_paddr = phys_page_base(aux_page) + 0x980U;
    const uintptr_t e16_fof_dump_paddr = phys_page_base(aux_page) + 0x9c0U;
    const uintptr_t e16_us_old_paddr = phys_page_base(aux_page) + 0xa00U;
    const uintptr_t e16_us_dump_paddr = phys_page_base(aux_page) + 0xa40U;
    const uintptr_t e8_fof_seed_paddr = phys_page_base(aux_page) + 0xa80U;
    const uintptr_t e8_fof_dump_paddr = phys_page_base(aux_page) + 0xac0U;
    uint8_t e8_us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e8_us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e8_us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e16_fof_payload[AI_FUBP_M2_VL] = { 0 };
    uint8_t e16_fof_seed[AI_FUBP_M2_VL] = { 0 };
    uint8_t e16_us_payload[AI_FUBP_M2_VL] = { 0 };
    uint8_t e16_us_old[AI_FUBP_M2_VL] = { 0 };
    uint8_t e16_us_expected[AI_FUBP_M2_VL] = { 0 };
    uint8_t e8_fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e8_fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(e8_us_payload, e8_vl, 0x16U);
    ai_fubp_fill_pattern(e8_us_old, e8_vl, 0x86U);
    ai_fubp_fill_pattern(e16_fof_payload, e16_total_bytes, 0x26U);
    ai_fubp_fill_pattern(e16_fof_seed, e16_total_bytes, 0xa6U);
    ai_fubp_fill_pattern(e16_us_payload, e16_total_bytes, 0x46U);
    ai_fubp_fill_pattern(e16_us_old, e16_total_bytes, 0xc6U);
    ai_fubp_fill_pattern(e8_fof_payload, e8_vl, 0x66U);
    ai_fubp_fill_pattern(e8_fof_seed, e8_vl, 0xe6U);
    for (unsigned i = 0; i < e8_vl; ++i) {
        e8_us_expected[i] = (i < e8_bp_lane) ?
            e8_us_payload[i] : e8_us_old[i];
    }
    for (unsigned i = 0; i < e16_total_bytes; ++i) {
        e16_us_expected[i] = (i < e16_bp_lane * 2U) ?
            e16_us_payload[i] : e16_us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(e8_us_paddr, e8_us_payload, e8_vl);
    ai_fubp_write_bytes(e8_us_old_paddr, e8_us_old, e8_vl);
    ai_fubp_fill_bytes(e8_us_dump_paddr, 0x36U, e8_vl);
    ai_fubp_write_bytes(e16_fof_low_paddr, e16_fof_payload,
        e16_fof_fault_lane * 2U);
    ai_fubp_write_bytes(e16_fof_high_paddr,
        e16_fof_payload + e16_fof_fault_lane * 2U,
        e16_total_bytes - e16_fof_fault_lane * 2U);
    ai_fubp_write_bytes(e16_fof_seed_paddr, e16_fof_seed,
        e16_total_bytes);
    ai_fubp_fill_bytes(e16_fof_dump_paddr, 0x56U, e16_total_bytes);
    ai_fubp_write_bytes(e16_us_paddr, e16_us_payload, e16_total_bytes);
    ai_fubp_write_bytes(e16_us_old_paddr, e16_us_old, e16_total_bytes);
    ai_fubp_fill_bytes(e16_us_dump_paddr, 0x76U, e16_total_bytes);
    ai_fubp_write_bytes(e8_fof_low_paddr, e8_fof_payload,
        e8_fof_fault_lane);
    ai_fubp_write_bytes(e8_fof_high_paddr,
        e8_fof_payload + e8_fof_fault_lane,
        e8_vl - e8_fof_fault_lane);
    ai_fubp_write_bytes(e8_fof_seed_paddr, e8_fof_seed, e8_vl);
    ai_fubp_fill_bytes(e8_fof_dump_paddr, 0x96U, e8_vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(e8_us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(e16_fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(e16_fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(e16_us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(e8_fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(e8_fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(e8_us_old_vaddr, e8_vl);
    TEST_ASSERT("vtype-scale setup preloads e8 UnitStride old bytes",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, e8_us_vaddr + same_byte_offset,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(e8_us_vaddr, e8_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e8 round interprets byte offset 8 as element 8 before switching to e16/m4",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e8_us_vaddr + same_byte_offset &&
        observed_vstart_csr == e8_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, e8_us_vaddr + same_byte_offset,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(e8_us_dump_vaddr, e8_vl);
    TEST_ASSERT("e8 round prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("e8 byte-offset-8 breakpoint leaves an eight-byte prefix, not an e16-scaled four-element prefix",
        ai_fubp_bytes_match(e8_us_dump_paddr, e8_us_expected, e8_vl)
    );

    ai_fubp_arm_mcontrol6_slot(0U, e16_fof_vaddr + same_byte_offset,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_m4_only(e16_fof_vaddr, e16_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e16/m4 VFOF reuses byte offset 8 but reports element 4 with current EEW scale",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e16_fof_vaddr + same_byte_offset &&
        observed_vstart_csr == e16_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu stale_e8_lane=%u",
        excpt.cause, excpt.tval, e16_fof_vaddr + same_byte_offset,
        observed_vstart_csr, e8_bp_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16ff_m4_full_dump(e16_fof_seed_vaddr, e16_fof_vaddr,
        e16_fof_dump_vaddr, e16_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("e16/m4 VFOF clear exposes the later first-fault using halfword element count and byte-accurate suffix",
        excpt.triggered == false &&
        observed_vl == e16_fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(e16_fof_dump_paddr, e16_fof_payload,
            e16_fof_fault_lane * 2U) &&
        ai_fubp_bytes_match(e16_fof_dump_paddr +
            e16_fof_fault_lane * 2U,
            e16_fof_seed + e16_fof_fault_lane * 2U,
            e16_total_bytes - e16_fof_fault_lane * 2U),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_m4_to_v8(e16_us_old_vaddr, e16_vl);
    TEST_ASSERT("reverse vtype-scale setup preloads e16/m4 old lanes before the US breakpoint",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, e16_us_vaddr + same_byte_offset,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_m4_to_v8(e16_us_vaddr, e16_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("reverse e16/m4 US round reports byte offset 8 as element 4 before switching back to e8",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e16_us_vaddr + same_byte_offset &&
        observed_vstart_csr == e16_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, e16_us_vaddr + same_byte_offset,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_m4_from_v8(e16_us_dump_vaddr, e16_vl);
    TEST_ASSERT("reverse e16/m4 prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("reverse e16/m4 breakpoint cuts at four halfword elements instead of stale e8 lane 8",
        ai_fubp_bytes_match(e16_us_dump_paddr, e16_us_expected,
            e16_total_bytes)
    );

    ai_fubp_arm_mcontrol6_slot(0U, e8_fof_vaddr + same_byte_offset,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(e8_fof_vaddr, e8_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e8 VFOF after e16/m4 reinterprets byte offset 8 as element 8 with fresh scale",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e8_fof_vaddr + same_byte_offset &&
        observed_vstart_csr == e8_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu stale_e16_lane=%u",
        excpt.cause, excpt.tval, e8_fof_vaddr + same_byte_offset,
        observed_vstart_csr, e16_bp_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(e8_fof_seed_vaddr, e8_fof_vaddr,
        e8_fof_dump_vaddr, e8_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("e8 VFOF clear exposes the later first-fault and proves the e16/m4 byte mask did not persist",
        excpt.triggered == false &&
        observed_vl == e8_fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(e8_fof_dump_paddr, e8_fof_payload,
            e8_fof_fault_lane) &&
        ai_fubp_bytes_match(e8_fof_dump_paddr + e8_fof_fault_lane,
            e8_fof_seed + e8_fof_fault_lane,
            e8_vl - e8_fof_fault_lane),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_store_vtype_e8_e16_trigger_scale_refresh_manual_corner()
{
    TEST_START();

    const enum test_page e8_store_page = VSRWX_GRWX;
    const enum test_page e16_store_page = VSRW_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned e8_vl = AI_FUBP_FULL_VL;
    const unsigned e16_vl = AI_FUBP_FULL_VL;
    const unsigned e16_total_bytes = e16_vl * 2U;
    const unsigned same_byte_offset = 9U;
    const unsigned e8_bp_lane = same_byte_offset;
    const unsigned e16_bp_lane = same_byte_offset / 2U;
    const unsigned fof_fault_lane = 12U;
    const uintptr_t e8_store_vaddr = hs_page_base(e8_store_page) + 0xb00U;
    const uintptr_t e16_store_vaddr = hs_page_base(e16_store_page) + 0xb80U;
    const uintptr_t e8_src_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t e16_src_vaddr = hs_page_base(aux_page) + 0x180U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t fof_src_vaddr = hs_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t e8_store_paddr = phys_page_base(e8_store_page) + 0xb00U;
    const uintptr_t e16_store_paddr =
        phys_page_base(e16_store_page) + 0xb80U;
    const uintptr_t e8_src_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t e16_src_paddr = phys_page_base(aux_page) + 0x180U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x280U;
    const uintptr_t fof_low_paddr =
        phys_page_base(fof_low_page) + PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    uint8_t e8_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e8_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e8_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t e16_payload[AI_FUBP_M2_VL] = { 0 };
    uint8_t e16_old[AI_FUBP_M2_VL] = { 0 };
    uint8_t e16_expected[AI_FUBP_M2_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(e8_payload, e8_vl, 0x19U);
    ai_fubp_fill_pattern(e8_old, e8_vl, 0x89U);
    ai_fubp_fill_pattern(e16_payload, e16_total_bytes, 0x39U);
    ai_fubp_fill_pattern(e16_old, e16_total_bytes, 0xb9U);
    ai_fubp_fill_pattern(fof_payload, e8_vl, 0x59U);
    ai_fubp_fill_pattern(fof_seed, e8_vl, 0xd9U);
    for (unsigned i = 0; i < e8_vl; ++i) {
        e8_expected[i] = (i < e8_bp_lane) ? e8_payload[i] : e8_old[i];
    }
    for (unsigned i = 0; i < e16_total_bytes; ++i) {
        e16_expected[i] = (i < e16_bp_lane * 2U) ?
            e16_payload[i] : e16_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(e8_store_paddr, e8_old, e8_vl);
    ai_fubp_write_bytes(e16_store_paddr, e16_old, e16_total_bytes);
    ai_fubp_write_bytes(e8_src_paddr, e8_payload, e8_vl);
    ai_fubp_write_bytes(e16_src_paddr, e16_payload, e16_total_bytes);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        e8_vl - fof_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, e8_vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x49U, e8_vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(e8_store_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(e16_store_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(e8_src_vaddr, e8_vl);
    TEST_ASSERT("store-scale setup loads e8 source payload before the byte-offset breakpoint",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, e8_store_vaddr + same_byte_offset,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(e8_store_vaddr, e8_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e8 store byte offset 9 reports element 9 before any e16 store scale is active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e8_store_vaddr + same_byte_offset &&
        observed_vstart_csr == e8_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, e8_store_vaddr + same_byte_offset,
        observed_vstart_csr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("e8 store breakpoint commits byte elements before offset 9 and leaves suffix old",
        ai_fubp_bytes_match(e8_store_paddr, e8_expected, e8_vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr,
        fof_dump_vaddr, e8_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF after e8 store breakpoint ignores the store-side byte mask and shortens at its own fault lane",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, fof_fault_lane) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_lane,
            fof_seed + fof_fault_lane, e8_vl - fof_fault_lane),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(e8_src_vaddr, e8_vl);
    TEST_ASSERT("e8 store retry reloads source after trigger clear",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(e8_store_vaddr, e8_vl);
    TEST_ASSERT("e8 store retry completes after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("e8 store retry writes the full byte payload after the trigger is cleared",
        ai_fubp_bytes_match(e8_store_paddr, e8_payload, e8_vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_m4_to_v8(e16_src_vaddr, e16_vl);
    TEST_ASSERT("store-scale reverse setup loads e16/m4 source payload",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, e16_store_vaddr + same_byte_offset,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_m4_from_v8(e16_store_vaddr, e16_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("e16/m4 store byte offset 9 is inside element 4 and must report element vstart 4",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == e16_store_vaddr + same_byte_offset &&
        observed_vstart_csr == e16_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu stale_e8_lane=%u",
        excpt.cause, excpt.tval, e16_store_vaddr + same_byte_offset,
        observed_vstart_csr, e8_bp_lane
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("e16/m4 store breakpoint commits only full halfword elements before element 4 and never half-writes the trigger element",
        ai_fubp_bytes_match(e16_store_paddr, e16_expected,
            e16_total_bytes)
    );
    ai_fubp_fill_bytes(fof_dump_paddr, 0x69U, e8_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr,
        fof_dump_vaddr, e8_vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF after e16/m4 store breakpoint ignores the halfword store cutoff and still reports its own fault lane",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, fof_fault_lane) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_lane,
            fof_seed + fof_fault_lane, e8_vl - fof_fault_lane),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u stale_e16_lane=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, e16_bp_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_m4_to_v8(e16_src_vaddr, e16_vl);
    TEST_ASSERT("e16/m4 store retry reloads source after trigger clear",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_m4_from_v8(e16_store_vaddr, e16_vl);
    TEST_ASSERT("e16/m4 store retry completes after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("e16/m4 store retry writes the full halfword payload with no stale byte cutoff",
        ai_fubp_bytes_match(e16_store_paddr, e16_payload,
            e16_total_bytes)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_same_pa_alias_fullva_trigger_owner_manual_corner()
{
    TEST_START();

    const enum test_page va0_page = VSRWX_GRWX;
    const enum test_page va1_page = VSRW_GRWX;
    const enum test_page backing_page = SWITCH1;
    const enum test_page remap_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned va0_base_offset = 0x300U;
    const unsigned va1_base_offset = 0x302U;
    const unsigned common_target_offset = 0x306U;
    const unsigned va0_lane = common_target_offset - va0_base_offset;
    const unsigned va1_lane = common_target_offset - va1_base_offset;
    const uintptr_t va0_base_vaddr = hs_page_base(va0_page) +
        va0_base_offset;
    const uintptr_t va1_base_vaddr = hs_page_base(va1_page) +
        va1_base_offset;
    const uintptr_t va0_target_vaddr = hs_page_base(va0_page) +
        common_target_offset;
    const uintptr_t va1_target_vaddr = hs_page_base(va1_page) +
        common_target_offset;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0xb00U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xb40U;
    const uintptr_t backing_paddr = phys_page_base(backing_page);
    const uintptr_t remap_paddr = phys_page_base(remap_page);
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0xb00U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xb40U;
    uint8_t backing_image[32] = { 0 };
    uint8_t remap_image[32] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t va0_full[AI_FUBP_FULL_VL] = { 0 };
    uint8_t va1_full[AI_FUBP_FULL_VL] = { 0 };
    uint8_t remap_va1_full[AI_FUBP_FULL_VL] = { 0 };
    uint8_t va0_prefix[AI_FUBP_FULL_VL] = { 0 };
    uint8_t va1_prefix[AI_FUBP_FULL_VL] = { 0 };
    uint8_t remap_va1_prefix[AI_FUBP_FULL_VL] = { 0 };
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(backing_image, sizeof(backing_image), 0x2bU);
    ai_fubp_fill_pattern(remap_image, sizeof(remap_image), 0x6bU);
    ai_fubp_fill_pattern(old_payload, sizeof(old_payload), 0xabU);
    for (unsigned i = 0; i < vl; ++i) {
        va0_full[i] = backing_image[i];
        va1_full[i] = backing_image[i + (va1_base_offset - va0_base_offset)];
        remap_va1_full[i] =
            remap_image[i + (va1_base_offset - va0_base_offset)];
        va0_prefix[i] = (i < va0_lane) ? va0_full[i] : old_payload[i];
        va1_prefix[i] = (i < va1_lane) ? va1_full[i] : old_payload[i];
        remap_va1_prefix[i] = (i < va1_lane) ?
            remap_va1_full[i] : old_payload[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(backing_paddr + va0_base_offset, backing_image,
        sizeof(backing_image));
    ai_fubp_write_bytes(remap_paddr + va0_base_offset, remap_image,
        sizeof(remap_image));
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x3bU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    *ai_fubp_hs_leaf_entry(va0_page) =
        ai_fubp_leaf_pte(backing_paddr, PTE_V | PTE_AD | PTE_RWX);
    *ai_fubp_hs_leaf_entry(va1_page) =
        ai_fubp_leaf_pte(backing_paddr, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(backing_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(remap_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    ai_fubp_arm_mcontrol6_slot(0U, va0_target_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("same-PA alias setup preloads old bytes before VA0 trigger probes VA1",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(va1_base_vaddr, vl);
    TEST_ASSERT("VA1 load reaches the same PA byte as VA0 trigger but must not break when full VA differs",
        excpt.triggered == false,
        "cause=0x%lx tval=0x%lx va0_target=0x%lx va1_target=0x%lx",
        excpt.cause, excpt.tval, va0_target_vaddr, va1_target_vaddr
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("same-PA VA1 no-break dump completes before VA0 trigger owner check",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VA1 no-break load proves the same backing PA was accessed without matching VA0 fullVA trigger",
        ai_fubp_bytes_match(dump_paddr, va1_full, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("same-PA alias setup reloads old bytes before the VA0 fullVA breakpoint",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(va0_base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("VA0 fullVA trigger reports VA0 tval and VA0 element index even though VA1 maps to the same PA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == va0_target_vaddr &&
        observed_vstart_csr == va0_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu lane=%u",
        excpt.cause, excpt.tval, va0_target_vaddr,
        observed_vstart_csr, va0_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("same-PA VA0 prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VA0 breakpoint prefix uses VA0 base/lane and not VA1 alias element owner",
        ai_fubp_bytes_match(dump_paddr, va0_prefix, vl)
    );

    ai_fubp_arm_mcontrol6_slot(0U, va1_target_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("same-PA reverse setup preloads old bytes before VA1 trigger probes VA0",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(va0_base_vaddr, vl);
    TEST_ASSERT("VA0 load reaches the same PA byte as VA1 trigger but must not break when full VA differs",
        excpt.triggered == false,
        "cause=0x%lx tval=0x%lx va0_target=0x%lx va1_target=0x%lx",
        excpt.cause, excpt.tval, va0_target_vaddr, va1_target_vaddr
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("same-PA VA0 no-break dump completes before VA1 trigger owner check",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VA0 no-break load proves PA aliasing is not enough to satisfy the VA1 fullVA trigger",
        ai_fubp_bytes_match(dump_paddr, va0_full, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("same-PA reverse setup reloads old bytes before the VA1 fullVA breakpoint",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(va1_base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("VA1 fullVA trigger reports VA1 tval and VA1 element index even though VA0 maps to the same PA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == va1_target_vaddr &&
        observed_vstart_csr == va1_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu lane=%u",
        excpt.cause, excpt.tval, va1_target_vaddr,
        observed_vstart_csr, va1_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("same-PA VA1 prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VA1 breakpoint prefix uses VA1 base/lane and not VA0 alias element owner",
        ai_fubp_bytes_match(dump_paddr, va1_prefix, vl)
    );

    goto_priv(PRIV_HS);
    *ai_fubp_hs_leaf_entry(va1_page) =
        ai_fubp_leaf_pte(remap_paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    ai_fubp_arm_mcontrol6_slot(0U, va0_target_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("remap setup preloads old bytes after VA1 moves to a different PA",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(va1_base_vaddr, vl);
    TEST_ASSERT("after sfence remap, VA1 must not inherit a stale VA0 fullVA or stale old PA trigger match",
        excpt.triggered == false,
        "cause=0x%lx tval=0x%lx va0_target=0x%lx va1_target=0x%lx",
        excpt.cause, excpt.tval, va0_target_vaddr, va1_target_vaddr
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("remapped VA1 no-break dump completes under VA0 trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("remapped VA1 no-break load observes the new PA image and no stale same-PA trigger owner",
        ai_fubp_bytes_match(dump_paddr, remap_va1_full, vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_arm_mcontrol6_slot(0U, va1_target_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("remap setup reloads old bytes before checking VA1 trigger on the new PA",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(va1_base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("after sfence remap, VA1 trigger still matches by current fullVA and reports VA1 lane on the new PA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == va1_target_vaddr &&
        observed_vstart_csr == va1_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu lane=%u",
        excpt.cause, excpt.tval, va1_target_vaddr,
        observed_vstart_csr, va1_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("remapped VA1 prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("remapped VA1 breakpoint prefix uses the new PA image but the same VA1 fullVA/lane owner",
        ai_fubp_bytes_match(dump_paddr, remap_va1_prefix, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_mask_whole_neighbor_vl_isolation_manual_corner()
{
    TEST_START();

    const enum test_page whole_page = VSRWX_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page us_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned fault_lane = 11U;
    const unsigned us_bp_lane = 5U;
    const uintptr_t whole_src_vaddr = hs_page_base(whole_page) + 0xc00U;
    const uintptr_t whole_dump_vaddr = hs_page_base(aux_page) + 0xb00U;
    const uintptr_t fof_src_vaddr = hs_page_base(fof_low_page) +
        PAGE_SIZE - fault_lane;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xc00U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xc40U;
    const uintptr_t us_src_vaddr = hs_page_base(us_page) + 0xd00U;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0xd40U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0xd80U;
    const uintptr_t whole_src_paddr = phys_page_base(whole_page) + 0xc00U;
    const uintptr_t whole_dump_paddr = phys_page_base(aux_page) + 0xb00U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xc00U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xc40U;
    const uintptr_t us_src_paddr = phys_page_base(us_page) + 0xd00U;
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0xd40U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0xd80U;
    uint8_t whole_src[128] = { 0 };
    uint8_t whole_guard[2] = { 0x6cU, 0x9cU };
    uint8_t payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint64_t vlenb = 0ULL;
    unsigned active_vlenb = 0U;
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(whole_src, sizeof(whole_src), 0x2cU);
    ai_fubp_fill_pattern(payload, vl, 0x4cU);
    ai_fubp_fill_pattern(seed, vl, 0xacU);
    ai_fubp_fill_pattern(us_payload, vl, 0x6cU);
    ai_fubp_fill_pattern(us_old, vl, 0xccU);
    for (unsigned i = 0; i < vl; ++i) {
        us_expected[i] = (i < us_bp_lane) ? us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    vlenb = ai_fubp_read_vlenb();
    TEST_ASSERT("whole-register neighbor case uses a VLENB that fits the local scratch window",
        vlenb > 0ULL && vlenb <= sizeof(whole_src),
        "vlenb=%lu max=%lu", vlenb, (uint64_t)sizeof(whole_src)
    );
    active_vlenb = (unsigned)vlenb;

    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(whole_src_paddr, whole_src, active_vlenb);
    write8(whole_dump_paddr - 1U, whole_guard[0]);
    ai_fubp_fill_bytes(whole_dump_paddr, 0x3cU, active_vlenb);
    write8(whole_dump_paddr + active_vlenb, whole_guard[1]);
    ai_fubp_write_bytes(fof_low_paddr, payload, fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, payload + fault_lane,
        vl - fault_lane);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x5cU, vl);
    ai_fubp_write_bytes(us_src_paddr, us_payload, vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x7cU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(whole_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vl1re8_v12_dump(whole_src_vaddr, whole_dump_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("vl1re8/vs1r whole-register neighbor copies exactly vlenb bytes and keeps adjacent guards",
        excpt.triggered == false &&
        ai_fubp_bytes_match(whole_dump_paddr, whole_src, active_vlenb) &&
        read8(whole_dump_paddr - 1U) == whole_guard[0] &&
        read8(whole_dump_paddr + active_vlenb) == whole_guard[1],
        "triggered=%d cause=0x%lx tval=0x%lx vlenb=%u",
        excpt.triggered, excpt.cause, excpt.tval, active_vlenb
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_src_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF after whole-register neighbor shortens at its own element lane and not at rawNf/vlenb layout",
        excpt.triggered == false &&
        observed_vl == fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, payload, fault_lane) &&
        ai_fubp_bytes_match(fof_dump_paddr + fault_lane,
            seed + fault_lane, vl - fault_lane),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u vlenb=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, active_vlenb
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("UnitStride breakpoint setup preloads old bytes after whole-register and VFOF neighbors",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("UnitStride breakpoint after whole-register/VFOF neighbors reports its own lane, not mask-byte or rawNf owner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu vlenb=%u",
        excpt.cause, excpt.tval, us_src_vaddr + us_bp_lane,
        observed_vstart_csr, active_vlenb
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("UnitStride breakpoint prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride prefix image is indexed by its own lane and not by whole-register vlenb or mask-register byte count",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_amo_lrsc_trigger_neighbor_vfof_us_owner_manual_corner()
{
    TEST_START();

    const enum test_page atomic_page = VSRWX_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned fof_fault_lane = 7U;
    const unsigned us_bp_lane = 11U;
    const uintptr_t atomic_vaddr = hs_page_base(atomic_page) + 0x180U;
    const uintptr_t fof_src_vaddr = hs_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t us_src_vaddr = hs_page_base(us_page) + 0x200U;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t atomic_paddr = phys_page_base(atomic_page) + 0x180U;
    const uintptr_t fof_src_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x140U;
    const uintptr_t us_src_paddr = phys_page_base(us_page) + 0x200U;
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x280U;
    const uint64_t atomic_initial = 0x1122334455667788ULL;
    const uint64_t atomic_addend = 0x0101010101010101ULL;
    const uint64_t lrsc_new_value = 0x5a5b5c5d5e5f6061ULL;
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(fof_payload, vl, 0x36U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xb6U);
    ai_fubp_fill_pattern(us_payload, vl, 0x46U);
    ai_fubp_fill_pattern(us_old, vl, 0xc6U);
    for (unsigned i = 0; i < vl; ++i) {
        us_expected[i] = (i < us_bp_lane) ? us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    write64(atomic_paddr, atomic_initial);
    ai_fubp_write_bytes(fof_src_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x66U, vl);
    ai_fubp_write_bytes(us_src_paddr, us_payload, vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x76U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(atomic_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    ai_fubp_arm_mcontrol6_slot(0U, atomic_vaddr, AI_FUBP_MCONTROL6_AMO);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(atomic_vaddr, atomic_addend);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("AMO address breakpoint reports scalar atomic metadata and suppresses the RMW before any VFOF owner is allocated",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == atomic_vaddr &&
        observed_vstart_csr == 0ULL &&
        read64(atomic_paddr) == atomic_initial,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu mem=0x%lx",
        excpt.cause, excpt.tval, atomic_vaddr, observed_vstart_csr,
        read64(atomic_paddr)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_src_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF after AMO breakpoint uses its own shortened-vl and is not poisoned by scalar atomic trigger metadata",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload,
            fof_fault_lane) &&
        ai_fubp_bytes_match(fof_dump_paddr + fof_fault_lane,
            fof_seed + fof_fault_lane, vl - fof_fault_lane) &&
        read64(atomic_paddr) == atomic_initial,
        "triggered=%d cause=0x%lx vl=%u vstart=%u atomic=0x%lx",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart,
        read64(atomic_paddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("atomic-neighbor US setup preloads old bytes after VFOF shortened-vl",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("US breakpoint after AMO and VFOF reports the current vector lane instead of stale scalar atomic owner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu atomic_va=0x%lx",
        excpt.cause, excpt.tval, us_src_vaddr + us_bp_lane,
        observed_vstart_csr, atomic_vaddr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("US breakpoint dump completes after clearing the vector trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("US breakpoint prefix is vector-lane indexed and scalar AMO memory remains unchanged after breakpoint suppression",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl) &&
        read64(atomic_paddr) == atomic_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t lr_old = ai_fubp_lr_d(atomic_vaddr);
    uint64_t sc_status = ai_fubp_sc_d(atomic_vaddr, lrsc_new_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("LR/SC after vector breakpoint completes on its own atomic path and proves no vector trigger metadata poisoned reservation state",
        excpt.triggered == false &&
        lr_old == atomic_initial &&
        sc_status == 0U &&
        read64(atomic_paddr) == lrsc_new_value,
        "triggered=%d cause=0x%lx lr=0x%lx sc=%u mem=0x%lx",
        excpt.triggered, excpt.cause, lr_old, sc_status,
        read64(atomic_paddr)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_priv_flip_us_bp_vfof_permission_refresh_manual_corner()
{
    TEST_START();

    const enum test_page u_page = SWITCH1;
    const enum test_page xonly_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned user_bp_lane = 8U;
    const unsigned xonly_fault_lane = 6U;
    const uintptr_t user_vaddr = hs_page_base(u_page) + 0x300U;
    const uintptr_t xonly_vaddr = hs_page_base(u_page) +
        PAGE_SIZE - xonly_fault_lane;
    const uintptr_t user_old_vaddr = hs_page_base(aux_page) + 0x340U;
    const uintptr_t user_dump_vaddr = hs_page_base(aux_page) + 0x380U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x3c0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x400U;
    const uintptr_t user_paddr = phys_page_base(u_page) + 0x300U;
    const uintptr_t xonly_prefix_paddr = phys_page_base(u_page) +
        PAGE_SIZE - xonly_fault_lane;
    const uintptr_t xonly_suffix_paddr = phys_page_base(xonly_page);
    const uintptr_t user_old_paddr = phys_page_base(aux_page) + 0x340U;
    const uintptr_t user_dump_paddr = phys_page_base(aux_page) + 0x380U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x3c0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x400U;
    uint8_t user_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t user_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t user_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t xonly_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(user_payload, vl, 0x58U);
    ai_fubp_fill_pattern(user_old, vl, 0xd8U);
    ai_fubp_fill_pattern(xonly_payload, vl, 0x68U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xe8U);
    for (unsigned i = 0; i < vl; ++i) {
        user_expected[i] = (i < user_bp_lane) ?
            user_payload[i] : user_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(user_paddr, user_payload, vl);
    ai_fubp_write_bytes(user_old_paddr, user_old, vl);
    ai_fubp_write_bytes(xonly_prefix_paddr, xonly_payload,
        xonly_fault_lane);
    ai_fubp_write_bytes(xonly_suffix_paddr,
        xonly_payload + xonly_fault_lane, vl - xonly_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(user_dump_paddr, 0x88U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x98U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(u_page, PTE_V | PTE_AD | PTE_U | PTE_RWX);
    ai_fubp_set_hs_leaf(xonly_page, PTE_V | PTE_AD | PTE_U | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    goto_priv(PRIV_M);
    ai_fubp_set_mprv_s(MSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(user_old_vaddr, vl);
    TEST_ASSERT("priv-flip setup preloads old bytes while MPRV effective-S has SUM set",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, user_vaddr + user_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_M);
    ai_fubp_set_mprv_s(MSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(user_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("MPRV effective-S with SUM set reports a user-page US breakpoint using the current privilege context",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == user_vaddr + user_bp_lane &&
        observed_vstart_csr == user_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, user_vaddr + user_bp_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_M);
    ai_fubp_set_mprv_s(MSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(user_dump_vaddr, vl);
    TEST_ASSERT("priv-flip user breakpoint dump completes while SUM is still set",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("priv-flip user-page breakpoint leaves only the current SUM-enabled prefix in v6",
        ai_fubp_bytes_match(user_dump_paddr, user_expected, vl)
    );

    goto_priv(PRIV_M);
    ai_fubp_set_mprv_s(0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(user_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("clearing SUM before retry exposes the current user-page load fault instead of reusing the old breakpoint permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == user_vaddr &&
        observed_vstart_csr == 0ULL,
        "cause=0x%lx tval=0x%lx expected_fault=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, user_vaddr, observed_vstart_csr
    );

    goto_priv(PRIV_M);
    ai_fubp_set_mprv_s(MSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, xonly_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("with SUM set but MXR clear, VFOF to execute-only user page shortens at the current permission fault lane",
        excpt.triggered == false &&
        observed_vl == xonly_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, xonly_payload,
            xonly_fault_lane) &&
        ai_fubp_bytes_match(fof_dump_paddr + xonly_fault_lane,
            fof_seed + xonly_fault_lane, vl - xonly_fault_lane),
        "triggered=%d cause=0x%lx vl=%u vstart=%u fault_lane=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart,
        xonly_fault_lane
    );

    ai_fubp_fill_bytes(fof_dump_paddr, 0xa8U, vl);

    goto_priv(PRIV_M);
    ai_fubp_set_mprv_s(MSTATUS_SUM | MSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, xonly_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);
    TEST_ASSERT("setting MXR after the VFOF fault makes the same execute-only user page readable without stale shortened-vl metadata",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, xonly_payload, vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u lane6=0x%x",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart,
        read8(fof_dump_paddr + xonly_fault_lane)
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | AI_FUBP_MSTATUS_MPP_MASK |
        MSTATUS_SUM | MSTATUS_MXR);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_us_visible_event_order_owner_manual_corner()
{
    TEST_START();

    const enum test_page vfof_page = VSRWX_GRWX;
    const enum test_page us_low_page = SWITCH1;
    const enum test_page us_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_split_lane = 8U;
    const unsigned us_bp_lane = 4U;
    const uintptr_t vfof_vaddr = hs_page_base(vfof_page) + 0x500U;
    const uintptr_t vfof_seed_vaddr = hs_page_base(aux_page) + 0x540U;
    const uintptr_t vfof_dump_vaddr = hs_page_base(aux_page) + 0x580U;
    const uintptr_t us_vaddr = hs_page_base(us_low_page) + PAGE_SIZE -
        us_split_lane;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0x5c0U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x600U;
    const uintptr_t vfof_paddr = phys_page_base(vfof_page) + 0x500U;
    const uintptr_t vfof_seed_paddr = phys_page_base(aux_page) + 0x540U;
    const uintptr_t vfof_dump_paddr = phys_page_base(aux_page) + 0x580U;
    const uintptr_t us_low_paddr = phys_page_base(us_low_page) + PAGE_SIZE -
        us_split_lane;
    const uintptr_t us_high_paddr = phys_page_base(us_high_page);
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0x5c0U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x600U;
    uint8_t vfof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t vfof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(vfof_payload, vl, 0x5dU);
    ai_fubp_fill_pattern(vfof_seed, vl, 0xddU);
    ai_fubp_fill_pattern(us_payload, vl, 0x6dU);
    ai_fubp_fill_pattern(us_old, vl, 0xedU);
    for (unsigned i = 0; i < vl; ++i) {
        us_bp_expected[i] = (i < us_bp_lane) ?
            us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(vfof_paddr, vfof_payload, vl);
    ai_fubp_write_bytes(vfof_seed_paddr, vfof_seed, vl);
    ai_fubp_fill_bytes(vfof_dump_paddr, 0x7dU, vl);
    ai_fubp_write_bytes(us_low_paddr, us_payload, us_split_lane);
    ai_fubp_write_bytes(us_high_paddr, us_payload + us_split_lane,
        vl - us_split_lane);
    ai_fubp_write_bytes(us_old_paddr, us_old, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x8dU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(vfof_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(us_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, vfof_vaddr, AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(vfof_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("visible-event order first round reports VFOF element0 breakpoint/fault owner before any younger US fault state",
        excpt.triggered == true &&
        (excpt.cause == CAUSE_BKP || excpt.cause == CAUSE_LPF) &&
        excpt.tval == vfof_vaddr &&
        observed_vstart_csr == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, vfof_vaddr, observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("after VFOF visible event is cleared, the neighboring US load exposes its own later page fault metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == us_vaddr + us_split_lane &&
        observed_vstart_csr == us_split_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu vfof_va=0x%lx",
        excpt.cause, excpt.tval, us_vaddr + us_split_lane,
        observed_vstart_csr, vfof_vaddr
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(vfof_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(us_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(vfof_seed_vaddr, vfof_vaddr,
        vfof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("repair after VFOF->US visible events completes the VFOF payload with no stale US fault owner",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(vfof_dump_paddr, vfof_payload, vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("reverse visible-event setup preloads old US bytes before the older US breakpoint",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("reverse visible-event order reports the older US breakpoint owner before the following VFOF element0 fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("reverse visible-event US prefix dump completes after clearing the older breakpoint",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("reverse visible-event US breakpoint prefix is not overwritten by the later VFOF owner",
        ai_fubp_bytes_match(us_dump_paddr, us_bp_expected, vl)
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(vfof_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(vfof_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("following VFOF element0 fault reports its own owner after the older US breakpoint is retired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vfof_vaddr &&
        observed_vstart_csr == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu us_va=0x%lx",
        excpt.cause, excpt.tval, vfof_vaddr, observed_vstart_csr,
        us_vaddr + us_bp_lane
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_dual_nonzero_fault_min_vl_then_us_bp_fresh_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned first_fault_lane = 5U;
    const unsigned second_lane = 10U;
    const unsigned us_bp_lane = 12U;
    const uintptr_t fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        first_fault_lane;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x180U;
    const uintptr_t us_src_vaddr = hs_page_base(us_page) + 0x200U;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t fof_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        first_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x140U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x180U;
    const uintptr_t us_src_paddr = phys_page_base(us_page) + 0x200U;
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x280U;
    const uint8_t second_winner_mask[2] = { 0x1fU, 0x04U };
    uint8_t payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t first_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(payload, vl, 0x2eU);
    ai_fubp_fill_pattern(seed, vl, 0xaeU);
    ai_fubp_fill_pattern(us_payload, vl, 0x4eU);
    ai_fubp_fill_pattern(us_old, vl, 0xceU);
    for (unsigned i = 0; i < vl; ++i) {
        first_expected[i] = (i < first_fault_lane) ?
            payload[i] : seed[i];
        us_expected[i] = (i < us_bp_lane) ? us_payload[i] : us_old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(fof_low_paddr, payload, first_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, payload + first_fault_lane,
        vl - first_fault_lane);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x5eU, vl);
    ai_fubp_write_bytes(mask_paddr, second_winner_mask,
        sizeof(second_winner_mask));
    ai_fubp_write_bytes(us_src_paddr, us_payload, vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x6eU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + second_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, dump_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF with an earlier no-read lane must keep the first shortened-vl even when a later active lane also matches the address trigger",
        excpt.triggered == false &&
        observed_vl == first_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, first_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u later_trigger=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, fof_vaddr + second_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("dual-fault follow-up preloads old UnitStride bytes after clearing the later VFOF trigger",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride breakpoint after the suppressed later VFOF event reports the fresh US lane and not stale second-fault metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu stale_fof_va=0x%lx",
        excpt.cause, excpt.tval, us_src_vaddr + us_bp_lane,
        observed_vstart_csr, fof_vaddr + second_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("UnitStride breakpoint after dual VFOF fault pressure can dump its own prefix after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride prefix after dual VFOF fault pressure is indexed by the US breakpoint lane",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_masked_vle8ff_full_dump(seed_vaddr, fof_vaddr, mask_vaddr,
        dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("masking off the first fault lane lets the second no-read lane become the VFOF shortened-vl winner without inheriting the prior first-fault vl",
        excpt.triggered == false &&
        observed_vl == second_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, first_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u expected_vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        second_lane, observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_samepage_cross16_load_bp_then_vfof_split_fresh_manual_corner()
{
    TEST_START();

    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_cross_lane = 1U;
    const unsigned fof_fault_lane = 11U;
    const uintptr_t us_vaddr = hs_page_base(us_page) + 0x30fU;
    const uintptr_t us_old_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x340U;
    const uintptr_t fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x380U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x3c0U;
    const uintptr_t us_paddr = phys_page_base(us_page) + 0x30fU;
    const uintptr_t us_old_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x340U;
    const uintptr_t fof_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x380U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x3c0U;
    const uintptr_t us_guard_lo_paddr = us_paddr - 1U;
    const uintptr_t us_guard_hi_paddr = us_paddr + vl;
    const uint8_t us_guard_lo = 0x5bU;
    const uint8_t us_guard_hi = 0xb5U;
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, vl, 0x31U);
    ai_fubp_fill_pattern(us_old, vl, 0xb1U);
    ai_fubp_fill_pattern(fof_payload, vl, 0x61U);
    ai_fubp_fill_pattern(seed, vl, 0xe1U);
    for (unsigned i = 0; i < vl; ++i) {
        us_expected[i] = (i < us_cross_lane) ? us_payload[i] : us_old[i];
        fof_expected[i] = (i < fof_fault_lane) ? fof_payload[i] : seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_paddr, us_payload, vl);
    ai_fubp_write_bytes(us_old_paddr, us_old, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x41U, vl);
    write8(us_guard_lo_paddr, us_guard_lo);
    write8(us_guard_hi_paddr, us_guard_hi);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x71U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_old_vaddr, vl);
    TEST_ASSERT("same-page cross16 setup preloads old bytes before the UnitStride load breakpoint allocates split state",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_cross_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("same-page cross16 UnitStride load reports the post-16B-boundary lane as the current breakpoint owner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_cross_lane &&
        observed_vstart_csr == us_cross_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_cross_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("same-page cross16 UnitStride breakpoint prefix can be dumped after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-page cross16 UnitStride breakpoint preserves the current prefix and does not half-write split guards",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl) &&
        read8(us_guard_lo_paddr) == us_guard_lo &&
        read8(us_guard_hi_paddr) == us_guard_hi
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("following cross-page VFOF split uses its own payload and shortened-vl, not the prior same-page cross16 MAB resident bytes",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u us_bp_va=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, us_vaddr + us_cross_lane
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x81U, vl);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("same-page cross16 resident state is fully cleared before VFOF repair reloads the complete current split payload",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_then_scalar_fault_source_refresh_manual_corner()
{
    TEST_START();

    const enum test_page fof_page = VSRWX_GRWX;
    const enum test_page scalar_page = SWITCH1;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned bp_lane = 5U;
    const uintptr_t fof_vaddr = hs_page_base(fof_page) + 0x500U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x600U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x640U;
    const uintptr_t scalar_fault_vaddr = hs_page_base(scalar_page) + 0x700U;
    const uintptr_t fof_paddr = phys_page_base(fof_page) + 0x500U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x600U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x640U;
    const uintptr_t scalar_paddr = phys_page_base(scalar_page) + 0x700U;
    const uint64_t scalar_value = 0xfeedface12345678ULL;
    uint8_t payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t observed_vl_csr = 0ULL;
    uint64_t scalar_sink = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(payload, vl, 0x72U);
    ai_fubp_fill_pattern(seed, vl, 0xd2U);

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(fof_paddr, payload, vl);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x42U, vl);
    write64(scalar_paddr, scalar_value);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(fof_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(scalar_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_only(fof_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF load address breakpoint reports its own nonzero-lane tval before any scalar fault source is present",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fof_vaddr + bp_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, dump_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the VFOF breakpoint, the same VFOF completes with full vl and a fresh payload dump",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, payload, vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[sink], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [sink] "=&r"(scalar_sink)
        : [addr] "r"(scalar_fault_vaddr)
        : "memory");
    observed_vl_csr = CSRR(CSR_VL);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("scalar load fault after a VFOF breakpoint refreshes cause and tval to the scalar source without losing the committed VFOF vl",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == scalar_fault_vaddr &&
        observed_vl_csr == vl &&
        observed_vstart_csr == 0ULL,
        "cause=0x%lx tval=0x%lx scalar=0x%lx stale_vfof=0x%lx vl=%lu vstart=%lu sink=0x%lx",
        excpt.cause, excpt.tval, scalar_fault_vaddr,
        fof_vaddr + bp_lane, observed_vl_csr, observed_vstart_csr,
        scalar_sink
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(scalar_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    scalar_sink = 0ULL;
    TEST_SETUP_EXCEPT();
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[sink], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [sink] "=&r"(scalar_sink)
        : [addr] "r"(scalar_fault_vaddr)
        : "memory");

    goto_priv(PRIV_M);
    TEST_ASSERT("repairing the scalar page lets the scalar observer complete once and proves the prior tval was not stale VFOF metadata",
        excpt.triggered == false &&
        scalar_sink == scalar_value,
        "triggered=%d cause=0x%lx sink=0x%lx expected=0x%lx",
        excpt.triggered, excpt.cause, scalar_sink, scalar_value
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_store_cross16_bp_suffix_mask_repair_manual_corner()
{
    TEST_START();

    const enum test_page target_page = VSRW_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned store_vl = 8U;
    const unsigned fof_vl = AI_FUBP_FULL_VL;
    const unsigned total_bytes = store_vl * 2U;
    const unsigned trigger_elem = 3U;
    const unsigned trigger_byte = trigger_elem * 2U;
    const unsigned fof_fault_lane = 9U;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x30fU;
    const uintptr_t src_vaddr = hs_page_base(aux_page) + 0x700U;
    const uintptr_t fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x780U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x7c0U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x30fU;
    const uintptr_t src_paddr = phys_page_base(aux_page) + 0x700U;
    const uintptr_t fof_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x780U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x7c0U;
    const uintptr_t guard_lo_paddr = target_paddr - 1U;
    const uintptr_t guard_hi_paddr = target_paddr + total_bytes;
    const uint8_t guard_lo = 0xa6U;
    const uint8_t guard_hi = 0x6aU;
    uint8_t old_bytes[AI_FUBP_M2_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_M2_VL] = { 0 };
    uint8_t bp_expected[AI_FUBP_M2_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(old_bytes, total_bytes, 0x26U);
    ai_fubp_fill_pattern(store_payload, total_bytes, 0xc6U);
    ai_fubp_fill_pattern(fof_payload, fof_vl, 0x46U);
    ai_fubp_fill_pattern(seed, fof_vl, 0xe6U);
    for (unsigned i = 0; i < total_bytes; ++i) {
        bp_expected[i] = (i < trigger_byte) ? store_payload[i] :
            old_bytes[i];
    }
    for (unsigned i = 0; i < fof_vl; ++i) {
        fof_expected[i] = (i < fof_fault_lane) ? fof_payload[i] : seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(target_paddr, old_bytes, total_bytes);
    ai_fubp_write_bytes(src_paddr, store_payload, total_bytes);
    write8(guard_lo_paddr, guard_lo);
    write8(guard_hi_paddr, guard_hi);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        fof_vl - fof_fault_lane);
    ai_fubp_write_bytes(seed_paddr, seed, fof_vl);
    ai_fubp_fill_bytes(dump_paddr, 0x56U, fof_vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(src_vaddr, store_vl);
    TEST_ASSERT("cross16 store setup loads the e16 payload before arming the StoreMisalignBuffer breakpoint",
        excpt.triggered == false
    );

    ai_fubp_arm_mcontrol6_slot(0U, target_vaddr + trigger_byte,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_from_v6(target_vaddr, store_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("same-page cross16 e16 store breakpoint reports the element index rather than the raw byte offset",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target_vaddr + trigger_byte &&
        observed_vstart_csr == trigger_elem,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu trigger_elem=%u",
        excpt.cause, excpt.tval, target_vaddr + trigger_byte,
        observed_vstart_csr, trigger_elem
    );

    TEST_ASSERT("same-page cross16 e16 store breakpoint commits complete prefix elements and preserves suffix plus guards",
        ai_fubp_bytes_match(target_paddr, bp_expected, total_bytes) &&
        read8(guard_lo_paddr) == guard_lo &&
        read8(guard_hi_paddr) == guard_hi
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle16_to_v6(src_vaddr, store_vl);
    TEST_ASSERT("cross16 store retry reloads payload after clearing the breakpoint",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse16_from_v6(target_vaddr, store_vl);
    TEST_ASSERT("cross16 store retry completes after the suffix mask is repaired",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross16 store retry writes the complete e16 payload exactly once with guards intact",
        ai_fubp_bytes_match(target_paddr, store_payload, total_bytes) &&
        read8(guard_lo_paddr) == guard_lo &&
        read8(guard_hi_paddr) == guard_hi,
        "target0=0x%x exp0=0x%x target6=0x%x exp6=0x%x target16=0x%x exp16=0x%x target31=0x%x exp31=0x%x guard_lo=0x%x guard_hi=0x%x",
        read8(target_paddr), store_payload[0],
        read8(target_paddr + trigger_byte), store_payload[trigger_byte],
        read8(target_paddr + 16U), store_payload[16],
        read8(target_paddr + total_bytes - 1U),
        store_payload[total_bytes - 1U],
        read8(guard_lo_paddr), read8(guard_hi_paddr)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, dump_vaddr, fof_vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("following VFOF uses its own shortened-vl and is not gated by the repaired cross16 store suffix mask",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, fof_expected, fof_vl) &&
        ai_fubp_bytes_match(target_paddr, store_payload, total_bytes),
        "triggered=%d cause=0x%lx vl=%u vstart=%u trigger_elem=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart,
        trigger_elem
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_strided_neighbor_then_us_fof_mask_owner_manual_corner()
{
    TEST_START();

    const enum test_page strided_page = VSRWX_GRWX;
    const enum test_page us_page = VSRWX_GRW;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = 8U;
    const intptr_t stride = 3;
    const unsigned strided_trigger = 2U;
    const unsigned us_trigger = 5U;
    const unsigned fof_fault_lane = 9U;
    const uintptr_t strided_vaddr = hs_page_base(strided_page) + 0x120U;
    const uintptr_t us_vaddr = hs_page_base(us_page) + 0x240U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t strided_dump_vaddr = hs_page_base(aux_page) + 0x340U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x380U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x3c0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x400U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t strided_paddr = phys_page_base(strided_page) + 0x120U;
    const uintptr_t us_paddr = phys_page_base(us_page) + 0x240U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t strided_dump_paddr = phys_page_base(aux_page) + 0x340U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x380U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x3c0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x400U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    uint8_t strided_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t strided_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(strided_payload, vl, 0x31U);
    ai_fubp_fill_pattern(us_payload, vl, 0x61U);
    ai_fubp_fill_pattern(old_payload, vl, 0xb1U);
    ai_fubp_fill_pattern(fof_payload, AI_FUBP_FULL_VL, 0x41U);
    ai_fubp_fill_pattern(fof_seed, AI_FUBP_FULL_VL, 0xc1U);
    for (unsigned i = 0; i < vl; ++i) {
        strided_expected[i] = (i < strided_trigger) ?
            strided_payload[i] : old_payload[i];
        us_expected[i] = (i < us_trigger) ? us_payload[i] :
            old_payload[i];
    }
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        fof_expected[i] = (i < fof_fault_lane) ? fof_payload[i] :
            fof_seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    for (unsigned i = 0; i < vl; ++i) {
        write8(strided_paddr + (uintptr_t)stride * i,
            strided_payload[i]);
    }
    ai_fubp_write_bytes(us_paddr, us_payload, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        AI_FUBP_FULL_VL - fof_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(strided_dump_paddr, 0x21U, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x22U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x23U, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(strided_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("strided-neighbor setup preloads old destination bytes before any address-trigger episode",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U,
        strided_vaddr + (uintptr_t)stride * strided_trigger,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlse8_to_v6(strided_vaddr, stride, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("strided neighbor breakpoint reports the true strided fullVA and lane before the later UnitStride/VFOF episodes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == strided_vaddr + (uintptr_t)stride *
            strided_trigger &&
        observed_vstart_csr == strided_trigger,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval,
        strided_vaddr + (uintptr_t)stride * strided_trigger,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(strided_dump_vaddr, vl);
    TEST_ASSERT("strided neighbor prefix dump completes after clearing its trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("strided neighbor updates only lanes before its own trigger and leaves later UnitStride lanes unclaimed",
        ai_fubp_bytes_match(strided_dump_paddr, strided_expected, vl)
    );

    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_trigger,
        AI_FUBP_MCONTROL6_LOAD);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride breakpoint after strided neighbor uses the UnitStride lane mask and current fullVA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_trigger &&
        observed_vstart_csr == us_trigger,
        "cause=0x%lx tval=0x%lx expected=0x%lx stale_strided=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_trigger,
        strided_vaddr + (uintptr_t)stride * strided_trigger,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("UnitStride breakpoint prefix dump completes after clearing the UnitStride trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride prefix is cut by its own trigger lane, not by the prior strided index or stride offset",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, AI_FUBP_FULL_VL, &observed_vl,
        &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("following VFOF first-fault uses its own sequential lane count after strided and UnitStride trigger masks",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_expected,
            AI_FUBP_FULL_VL),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u stale_us=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, us_vaddr + us_trigger
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_segment_store_bp_prefix_then_vfof_owner_manual_corner()
{
    TEST_START();

    const enum test_page seg_page = VSRW_GRWX;
    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned seg_vl = 8U;
    const unsigned trigger_elem = 3U;
    const unsigned trigger_byte = trigger_elem * 2U;
    const unsigned fof_bp_lane = 5U;
    const unsigned fof_fault_lane = 9U;
    const uintptr_t seg_vaddr = hs_page_base(seg_page) + 0x510U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x5c0U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x640U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x680U;
    const uintptr_t seg_paddr = phys_page_base(seg_page) + 0x510U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x5c0U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x640U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x680U;
    uint8_t seg_old[AI_FUBP_M2_VL] = { 0 };
    uint8_t seg_src[AI_FUBP_M2_VL] = { 0 };
    uint8_t seg_expected[AI_FUBP_M2_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(seg_old, seg_vl * 2U, 0x25U);
    ai_fubp_fill_pattern(seg_src, seg_vl * 2U, 0xa5U);
    ai_fubp_fill_pattern(fof_payload, AI_FUBP_FULL_VL, 0x45U);
    ai_fubp_fill_pattern(fof_seed, AI_FUBP_FULL_VL, 0xd5U);
    for (unsigned i = 0; i < seg_vl * 2U; ++i) {
        seg_expected[i] = (i < trigger_byte) ? seg_src[i] :
            seg_old[i];
    }
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            fof_seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ? fof_payload[i] :
            fof_seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(seg_paddr, seg_old, seg_vl * 2U);
    ai_fubp_write_bytes(src_paddr, seg_src, seg_vl * 2U);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        AI_FUBP_FULL_VL - fof_fault_lane);
    ai_fubp_write_bytes(seed_paddr, fof_seed, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(dump_paddr, 0x35U, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(seg_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8_to_v8(src_vaddr, seg_vl);
    TEST_ASSERT("segment-store setup loads distinct field0/field1 payload bytes before arming the field0 breakpoint",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, seg_vaddr + trigger_byte,
        AI_FUBP_MCONTROL6_STORE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vsseg2e8_from_v8(seg_vaddr, seg_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("segment-store field0 breakpoint reports the element index and field0 VA before any VFOF owner is allocated",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == seg_vaddr + trigger_byte &&
        observed_vstart_csr == trigger_elem,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, seg_vaddr + trigger_byte,
        observed_vstart_csr
    );

    TEST_ASSERT("segment-store breakpoint commits only complete earlier segment fields and preserves the trigger segment plus suffix",
        ai_fubp_bytes_match(seg_paddr, seg_expected, seg_vl * 2U)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, dump_vaddr,
        AI_FUBP_FULL_VL, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF breakpoint after segment-store prefix reports its own lane and VA, not the old segment field owner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane &&
        ai_fubp_bytes_match(dump_paddr, fof_bp_expected,
            AI_FUBP_FULL_VL),
        "cause=0x%lx tval=0x%lx expected=0x%lx stale_segment=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        seg_vaddr + trigger_byte, observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(dump_paddr, 0x65U, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, dump_vaddr,
        AI_FUBP_FULL_VL, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the VFOF breakpoint exposes the later first-fault lane without changing the segment-store prefix image",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump_paddr, fof_fault_expected,
            AI_FUBP_FULL_VL) &&
        ai_fubp_bytes_match(seg_paddr, seg_expected, seg_vl * 2U),
        "triggered=%d cause=0x%lx vl=%u vstart=%u stale_segment=0x%lx",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart,
        seg_vaddr + trigger_byte
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_fof_bp_trap_csr_logging_no_mixed_state_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned fof_fault_lane = 9U;
    const unsigned fof_bp_lane = 4U;
    const unsigned us_bp_lane = 6U;
    const uintptr_t fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t us_vaddr = hs_page_base(us_page) + 0x720U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x760U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x7a0U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x7e0U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x820U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t us_paddr = phys_page_base(us_page) + 0x720U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x760U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x7a0U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x7e0U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x820U;
    const uint64_t csr_sentinel = 0x5a5a5a5a5a5a5a5aULL;
    const uint64_t tval_sentinel = 0x123456789abcULL;
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vl_csr = 0ULL;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t cycle_before = 0ULL;
    uint64_t cycle_after = 0ULL;
    uint64_t instret_before = 0ULL;
    uint64_t instret_after = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(fof_payload, vl, 0x49U);
    ai_fubp_fill_pattern(seed, vl, 0xc9U);
    ai_fubp_fill_pattern(us_payload, vl, 0x69U);
    ai_fubp_fill_pattern(old_payload, vl, 0xb9U);
    for (unsigned i = 0; i < vl; ++i) {
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ?
            fof_payload[i] : seed[i];
        us_expected[i] = (i < us_bp_lane) ? us_payload[i] :
            old_payload[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(us_paddr, us_payload, vl);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x19U, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x29U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    goto_priv(PRIV_M);
    CSRW(CSR_MCAUSE, csr_sentinel);
    CSRW(CSR_MTVAL, tval_sentinel);
    cycle_before = CSRR(CSR_MCYCLE);
    instret_before = CSRR(CSR_MINSTRET);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, dump_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    cycle_after = CSRR(CSR_MCYCLE);
    instret_after = CSRR(CSR_MINSTRET);
    TEST_ASSERT("silent VFOF first-fault updates vl/dump but does not overwrite visible trap CSR logging state",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        cycle_after >= cycle_before &&
        instret_after >= instret_before &&
        ai_fubp_bytes_match(dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx mtval=0x%lx vl=%u vstart=%u",
        excpt.triggered, CSRR(CSR_MCAUSE), CSRR(CSR_MTVAL),
        observed_vl, observed_vstart
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    CSRW(CSR_MCAUSE, csr_sentinel + 1ULL);
    CSRW(CSR_MTVAL, tval_sentinel + 1ULL);
    ai_fubp_fill_bytes(dump_paddr, 0x39U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, dump_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    observed_vl_csr = CSRR(CSR_VL);
    TEST_ASSERT("VFOF breakpoint trap CSR logging belongs to the breakpoint lane and not the prior silent shortened-vl event",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane &&
        observed_vl_csr == vl &&
        ai_fubp_bytes_match(dump_paddr, fof_bp_expected, vl),
        "cause=0x%lx tval=0x%lx mcause=0x%lx mtval=0x%lx vl_csr=%lu captured_vstart=%u",
        excpt.cause, excpt.tval, CSRR(CSR_MCAUSE), CSRR(CSR_MTVAL),
        observed_vl_csr, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    CSRW(CSR_MCAUSE, csr_sentinel + 2ULL);
    CSRW(CSR_MTVAL, tval_sentinel + 2ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("trap-CSR logging setup preloads old UnitStride bytes before the second visible breakpoint",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride breakpoint after VFOF breakpoint refreshes trap CSR state to the current US VA and lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx mcause=0x%lx mtval=0x%lx stale_fof=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, CSRR(CSR_MCAUSE), CSRR(CSR_MTVAL),
        fof_vaddr + fof_bp_lane, observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("UnitStride dump after trap CSR logging keeps only the current US prefix bytes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("trap CSR logging does not mix the VFOF breakpoint prefix with the following UnitStride breakpoint prefix",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_mixed_rvc_setup_us_fof_current_operands_manual_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page cur_page = VSRW_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned old_vl = 4U;
    const unsigned cur_vl = 12U;
    const unsigned us_bp_lane = 7U;
    const unsigned fof_fault_lane = 10U;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x900U;
    const uintptr_t cur_vaddr = hs_page_base(cur_page) + 0x940U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x980U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x9c0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xa00U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x900U;
    const uintptr_t cur_paddr = phys_page_base(cur_page) + 0x940U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x980U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x9c0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xa00U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t cur_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t expected_prefix[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    unsigned setup_marker = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(old_payload, AI_FUBP_FULL_VL, 0x2aU);
    ai_fubp_fill_pattern(cur_payload, AI_FUBP_FULL_VL, 0x5aU);
    ai_fubp_fill_pattern(seed, AI_FUBP_FULL_VL, 0xbaU);
    ai_fubp_fill_pattern(fof_payload, AI_FUBP_FULL_VL, 0x7aU);
    for (unsigned i = 0; i < AI_FUBP_FULL_VL; ++i) {
        expected_prefix[i] = (i < us_bp_lane) ? cur_payload[i] :
            seed[i];
        fof_expected[i] = (i < fof_fault_lane) ? fof_payload[i] :
            seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(old_paddr, old_payload, AI_FUBP_FULL_VL);
    ai_fubp_write_bytes(cur_paddr, cur_payload, AI_FUBP_FULL_VL);
    ai_fubp_write_bytes(seed_paddr, seed, AI_FUBP_FULL_VL);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        AI_FUBP_FULL_VL - fof_fault_lane);
    ai_fubp_fill_bytes(dump_paddr, 0x3aU, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x4aU, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(cur_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(seed_vaddr, AI_FUBP_FULL_VL);
    TEST_ASSERT("mixed-RVC setup preloads seed bytes before changing base and AVL with compressed setup instructions",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, cur_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_mixed_rvc_setup_vle8_to_v6(old_vaddr, cur_vaddr, old_vl,
        cur_vl, &setup_marker);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("mixed RVC/RVI setup uses the final base and AVL for the UnitStride breakpoint instead of stale setup operands",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == cur_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane &&
        setup_marker == 3U,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu marker=%u old_base=0x%lx",
        excpt.cause, excpt.tval, cur_vaddr + us_bp_lane,
        observed_vstart_csr, setup_marker, old_vaddr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, AI_FUBP_FULL_VL);
    TEST_ASSERT("mixed-RVC setup breakpoint prefix can be dumped after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("mixed-RVC setup prefix reflects the final base and final AVL, not the stale old base or old vl",
        ai_fubp_bytes_match(dump_paddr, expected_prefix,
            AI_FUBP_FULL_VL)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        AI_FUBP_FULL_VL, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("following VFOF uses its own base/vl after the mixed setup breakpoint episode",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_expected,
            AI_FUBP_FULL_VL),
        "triggered=%d cause=0x%lx vl=%u vstart=%u stale_us=0x%lx",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart,
        cur_vaddr + us_bp_lane
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vill_transient_recover_us_fof_retry_manual_corner()
{
    TEST_START();

    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_bp_lane = 6U;
    const unsigned fof_bp_lane = 5U;
    const unsigned fof_fault_lane = 11U;
    const uintptr_t us_vaddr = hs_page_base(us_page) + 0xa80U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0xac0U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xb00U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xb40U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xb80U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t us_paddr = phys_page_base(us_page) + 0xa80U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0xac0U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xb00U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xb40U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xb80U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t vill_vtype = 0ULL;
    uint64_t restored_vtype = 0ULL;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, vl, 0x1bU);
    ai_fubp_fill_pattern(old_payload, vl, 0x8bU);
    ai_fubp_fill_pattern(seed, vl, 0xcbU);
    ai_fubp_fill_pattern(fof_payload, vl, 0x4bU);
    for (unsigned i = 0; i < vl; ++i) {
        us_expected[i] = (i < us_bp_lane) ? us_payload[i] :
            old_payload[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ? fof_payload[i] :
            seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_paddr, us_payload, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_fill_bytes(dump_paddr, 0x2bU, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x3bU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("vill-transient setup preloads old UnitStride bytes before the breakpoint episode",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_set_vill_then_restore_e8(vl, &vill_vtype, &restored_vtype);
    TEST_ASSERT("vill transient is observable and then restored to a legal e8/m1 vector type before memory retry",
        excpt.triggered == false &&
        (vill_vtype & AI_FUBP_VTYPE_VILL) != 0ULL &&
        (restored_vtype & AI_FUBP_VTYPE_VILL) == 0ULL,
        "vill_vtype=0x%lx restored_vtype=0x%lx", vill_vtype,
        restored_vtype
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride breakpoint after a restored vill transient decodes with the restored vtype and current lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane &&
        (CSRR(CSR_VTYPE) & AI_FUBP_VTYPE_VILL) == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu vtype=0x%lx",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr, CSRR(CSR_VTYPE)
    );

    ai_fubp_clear_mcontrol6_slot(0U);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("UnitStride dump after vill transient completes once the trigger is cleared",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride prefix after vill transient is cut by the current breakpoint lane",
        ai_fubp_bytes_match(dump_paddr, us_expected, vl)
    );

    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF breakpoint after vill recovery remains a visible breakpoint instead of a stale illegal/vill decode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u vtype=0x%lx",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart, CSRR(CSR_VTYPE)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x6bU, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the VFOF breakpoint after vill recovery exposes the true first-fault lane with legal vtype state",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        (CSRR(CSR_VTYPE) & AI_FUBP_VTYPE_VILL) == 0ULL &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u vtype=0x%lx",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart,
        CSRR(CSR_VTYPE)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vs_off_illegal_suppresses_us_fof_trigger_manual_corner()
{
    TEST_START();

    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_bp_lane = 4U;
    const unsigned fof_bp_lane = 3U;
    const unsigned fof_fault_lane = 9U;
    const uintptr_t us_vaddr = hs_page_base(us_page) + 0xc20U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0xc60U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xca0U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xce0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xd20U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t us_paddr = phys_page_base(us_page) + 0xc20U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0xc60U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xca0U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xce0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xd20U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, vl, 0x1dU);
    ai_fubp_fill_pattern(old_payload, vl, 0x8dU);
    ai_fubp_fill_pattern(seed, vl, 0xcdU);
    ai_fubp_fill_pattern(fof_payload, vl, 0x4dU);
    for (unsigned i = 0; i < vl; ++i) {
        us_expected[i] = (i < us_bp_lane) ? us_payload[i] :
            old_payload[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ? fof_payload[i] :
            seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_paddr, us_payload, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_fill_bytes(dump_paddr, 0x2dU, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x3dU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("VS-Off setup preloads old UnitStride bytes while vector state is enabled",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    goto_priv(PRIV_HS);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        ".option pop\n\t"
        :
        : [vl] "r"(vl)
        : "t0", "memory");
    ai_fubp_disable_vector_state();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_current_to_v6(us_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("VS-Off UnitStride raises illegal instruction before address-trigger compare or memory access",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        excpt.tval != us_vaddr + us_bp_lane,
        "cause=0x%lx tval=0x%lx trigger_va=0x%lx",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane
    );

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("after VS is re-enabled the same UnitStride address trigger fires with fresh tval and vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("VS-Off UnitStride dump completes after clearing the valid trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VS-Off illegal probe produced no vector-memory side effect before the re-enabled breakpoint prefix",
        ai_fubp_bytes_match(dump_paddr, us_expected, vl)
    );

    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    goto_priv(PRIV_HS);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        ".option pop\n\t"
        :
        : [vl] "r"(vl), [seed] "r"(seed_vaddr)
        : "t0", "memory");
    ai_fubp_disable_vector_state();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_current_to_v8(fof_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("VS-Off VFOF raises illegal instruction before FOF silent-stop or breakpoint metadata is allocated",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        excpt.tval != fof_vaddr + fof_bp_lane,
        "cause=0x%lx tval=0x%lx trigger_va=0x%lx",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane
    );

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x6dU, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("after VS is re-enabled the VFOF address trigger remains visible and is not converted to stale silent shortened-vl",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x7dU, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the re-enabled VFOF trigger exposes the real nonzero first-fault without stale VS-Off metadata",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_trigger_reprogram_old_event_then_segment_fof_fresh_manual_corner()
{
    TEST_START();

    const enum test_page old_us_page = VSRWX_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned old_us_lane = 2U;
    const unsigned new_fof_lane = 6U;
    const unsigned fof_fault_lane = 11U;
    const uintptr_t old_us_vaddr = hs_page_base(old_us_page) + 0xda0U;
    const uintptr_t old_seed_vaddr = hs_page_base(aux_page) + 0xde0U;
    const uintptr_t old_dump_vaddr = hs_page_base(aux_page) + 0xe20U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0xe60U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xea0U;
    const uintptr_t old_us_paddr = phys_page_base(old_us_page) + 0xda0U;
    const uintptr_t old_seed_paddr = phys_page_base(aux_page) + 0xde0U;
    const uintptr_t old_dump_paddr = phys_page_base(aux_page) + 0xe20U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0xe60U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xea0U;
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(old_payload, vl, 0x17U);
    ai_fubp_fill_pattern(old_seed, vl, 0x87U);
    ai_fubp_fill_pattern(fof_payload, vl, 0x37U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xb7U);
    for (unsigned i = 0; i < vl; ++i) {
        old_expected[i] = (i < old_us_lane) ? old_payload[i] :
            old_seed[i];
        fof_bp_expected[i] = (i < new_fof_lane) ? fof_payload[i] :
            fof_seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ? fof_payload[i] :
            fof_seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_clear_mcontrol6_slot(1U);
    ai_fubp_write_bytes(old_us_paddr, old_payload, vl);
    ai_fubp_write_bytes(old_seed_paddr, old_seed, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(old_dump_paddr, 0x27U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x47U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(old_us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_seed_vaddr, vl);
    TEST_ASSERT("trigger-reprogram setup preloads old UnitStride seed before slot0 is armed",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, old_us_vaddr + old_us_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("trigger-reprogram first episode takes the old slot0 UnitStride breakpoint before reprogramming",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == old_us_vaddr + old_us_lane &&
        observed_vstart_csr == old_us_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, old_us_vaddr + old_us_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(1U, fof_vaddr + new_fof_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(old_dump_vaddr, vl);
    TEST_ASSERT("trigger-reprogram old UnitStride prefix dump completes after slot0 is cleared and slot1 is armed",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("trigger-reprogram old response cannot be reclassified with the new slot1 address",
        ai_fubp_bytes_match(old_dump_paddr, old_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("trigger-reprogram next VFOF reports the freshly programmed slot1 full VA and prefix",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + new_fof_lane &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx old_va=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + new_fof_lane,
        old_us_vaddr + old_us_lane, observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(1U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x67U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("trigger-reprogram clearing the new trigger exposes the real later VFOF first-fault without stale old-slot metadata",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_segment_fof_retry_field_owner_fullva_refresh_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned req_vl = AI_FUBP_FULL_VL;
    const unsigned first_fault_elem = 5U;
    const unsigned next_trigger_elem = 6U;
    const unsigned low_bytes = first_fault_elem * 2U + 1U;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        low_bytes;
    const uintptr_t field1_retry_trigger_vaddr =
        src_vaddr + first_fault_elem * 2U + 1U;
    const uintptr_t field0_next_trigger_vaddr =
        src_vaddr + next_trigger_elem * 2U;
    const uintptr_t seed0_vaddr = hs_page_base(aux_page) + 0xf00U;
    const uintptr_t seed1_vaddr = hs_page_base(aux_page) + 0xf40U;
    const uintptr_t dump0_vaddr = hs_page_base(aux_page) + 0xf80U;
    const uintptr_t dump1_vaddr = hs_page_base(aux_page) + 0xfc0U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        low_bytes;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t seed0_paddr = phys_page_base(aux_page) + 0xf00U;
    const uintptr_t seed1_paddr = phys_page_base(aux_page) + 0xf40U;
    const uintptr_t dump0_paddr = phys_page_base(aux_page) + 0xf80U;
    const uintptr_t dump1_paddr = phys_page_base(aux_page) + 0xfc0U;
    uint8_t field0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t field1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed1[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(field0, req_vl, 0x19U);
    ai_fubp_fill_pattern(field1, req_vl, 0x59U);
    ai_fubp_fill_pattern(seed0, req_vl, 0x99U);
    ai_fubp_fill_pattern(seed1, req_vl, 0xd9U);

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    for (unsigned byte = 0; byte < req_vl * 2U; ++byte) {
        const unsigned elem = byte >> 1;
        const uint8_t value = (byte & 1U) ? field1[elem] :
            field0[elem];

        if (byte < low_bytes) {
            write8(low_paddr + byte, value);
        } else {
            write8(high_paddr + byte - low_bytes, value);
        }
    }
    ai_fubp_write_bytes(seed0_paddr, seed0, req_vl);
    ai_fubp_write_bytes(seed1_paddr, seed1, req_vl);
    ai_fubp_fill_bytes(dump0_paddr, 0x29U, req_vl);
    ai_fubp_fill_bytes(dump1_paddr, 0x39U, req_vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump(seed0_vaddr, seed1_vaddr,
        src_vaddr, dump0_vaddr, dump1_vaddr, req_vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF retry setup silently shortens at the field1 page fault before any visible tval is recorded",
        excpt.triggered == false &&
        observed_vl == first_fault_elem &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump0_paddr, field0,
            first_fault_elem) &&
        ai_fubp_bytes_match(dump1_paddr, field1,
            first_fault_elem),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, field1_retry_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(dump0_paddr, 0x49U, req_vl);
    ai_fubp_fill_bytes(dump1_paddr, 0x69U, req_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump(seed0_vaddr, seed1_vaddr,
        src_vaddr, dump0_vaddr, dump1_vaddr, req_vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF repaired retry reports the current field1 full VA and element owner, not the earlier page-fault metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == field1_retry_trigger_vaddr &&
        ai_fubp_bytes_match(dump0_paddr, field0,
            first_fault_elem + 1U) &&
        ai_fubp_bytes_match(dump1_paddr, field1,
            first_fault_elem) &&
        read8(dump1_paddr + first_fault_elem) ==
            seed1[first_fault_elem],
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, field1_retry_trigger_vaddr,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, field0_next_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(dump0_paddr, 0x79U, req_vl);
    ai_fubp_fill_bytes(dump1_paddr, 0x89U, req_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump(seed0_vaddr, seed1_vaddr,
        src_vaddr, dump0_vaddr, dump1_vaddr, req_vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF owner refresh control reports the next field0 VA and advances the element index after retry repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == field0_next_trigger_vaddr &&
        ai_fubp_bytes_match(dump0_paddr, field0,
            next_trigger_elem) &&
        ai_fubp_bytes_match(dump1_paddr, field1,
            next_trigger_elem) &&
        read8(dump0_paddr + next_trigger_elem) ==
            seed0[next_trigger_elem] &&
        read8(dump1_paddr + next_trigger_elem) ==
            seed1[next_trigger_elem],
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, field0_next_trigger_vaddr,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(dump0_paddr, 0xa9U, req_vl);
    ai_fubp_fill_bytes(dump1_paddr, 0xb9U, req_vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump(seed0_vaddr, seed1_vaddr,
        src_vaddr, dump0_vaddr, dump1_vaddr, req_vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF final retry after clearing triggers loads both fields exactly once across the repaired boundary",
        excpt.triggered == false &&
        observed_vl == req_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump0_paddr, field0, req_vl) &&
        ai_fubp_bytes_match(dump1_paddr, field1, req_vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_store_prefix_drained_then_segment_fof_reads_current_bytes_manual_corner()
{
    TEST_START();

    const enum test_page store_page = VSRW_GRWX;
    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned store_vl = AI_FUBP_FULL_VL;
    const unsigned seg_vl = 8U;
    const unsigned store_trigger_byte = 8U;
    const unsigned seg_trigger_elem = 5U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x520U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x560U;
    const uintptr_t seed0_vaddr = hs_page_base(aux_page) + 0x5a0U;
    const uintptr_t seed1_vaddr = hs_page_base(aux_page) + 0x5e0U;
    const uintptr_t dump0_vaddr = hs_page_base(aux_page) + 0x620U;
    const uintptr_t dump1_vaddr = hs_page_base(aux_page) + 0x660U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x520U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x560U;
    const uintptr_t seed0_paddr = phys_page_base(aux_page) + 0x5a0U;
    const uintptr_t seed1_paddr = phys_page_base(aux_page) + 0x5e0U;
    const uintptr_t dump0_paddr = phys_page_base(aux_page) + 0x620U;
    const uintptr_t dump1_paddr = phys_page_base(aux_page) + 0x660U;
    const uintptr_t seg_field1_trigger_vaddr =
        store_vaddr + seg_trigger_elem * 2U + 1U;
    uint8_t old_mem[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t memory_after_store[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t bp_expected0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t bp_expected1[AI_FUBP_FULL_VL] = { 0 };
    uint8_t full_expected0[AI_FUBP_FULL_VL] = { 0 };
    uint8_t full_expected1[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(old_mem, store_vl, 0x22U);
    ai_fubp_fill_pattern(store_payload, store_vl, 0xa2U);
    ai_fubp_fill_pattern(seed0, AI_FUBP_FULL_VL, 0x42U);
    ai_fubp_fill_pattern(seed1, AI_FUBP_FULL_VL, 0x82U);
    for (unsigned byte = 0; byte < store_vl; ++byte) {
        memory_after_store[byte] = (byte < store_trigger_byte) ?
            store_payload[byte] : old_mem[byte];
    }
    for (unsigned elem = 0; elem < AI_FUBP_FULL_VL; ++elem) {
        if (elem < seg_vl) {
            full_expected0[elem] = memory_after_store[elem * 2U];
            full_expected1[elem] = memory_after_store[elem * 2U + 1U];
        } else {
            full_expected0[elem] = seed0[elem];
            full_expected1[elem] = seed1[elem];
        }
        bp_expected0[elem] = (elem <= seg_trigger_elem) ?
            full_expected0[elem] : seed0[elem];
        bp_expected1[elem] = (elem < seg_trigger_elem) ?
            full_expected1[elem] : seed1[elem];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(store_paddr, old_mem, store_vl);
    ai_fubp_write_bytes(src_paddr, store_payload, store_vl);
    ai_fubp_write_bytes(seed0_paddr, seed0, AI_FUBP_FULL_VL);
    ai_fubp_write_bytes(seed1_paddr, seed1, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(dump0_paddr, 0x52U, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(dump1_paddr, 0x62U, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(store_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, store_vl);
    TEST_ASSERT("store-prefix-to-segment setup loads store payload before arming the UnitStride store breakpoint",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, store_vaddr + store_trigger_byte,
        AI_FUBP_MCONTROL6_STORE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, store_vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("older UnitStride store breakpoint drains only the byte prefix that Segment-FOF is allowed to observe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_vaddr + store_trigger_byte &&
        observed_vstart_csr == store_trigger_byte &&
        ai_fubp_bytes_match(store_paddr, memory_after_store, store_vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, store_vaddr + store_trigger_byte,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_arm_mcontrol6_slot(0U, seg_field1_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump(seed0_vaddr, seed1_vaddr,
        store_vaddr, dump0_vaddr, dump1_vaddr, seg_vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF reads the drained store prefix and old suffix before its own field1 breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == seg_field1_trigger_vaddr &&
        ai_fubp_bytes_match(dump0_paddr, bp_expected0, seg_vl) &&
        ai_fubp_bytes_match(dump1_paddr, bp_expected1, seg_vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, seg_field1_trigger_vaddr,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(dump0_paddr, 0x72U, AI_FUBP_FULL_VL);
    ai_fubp_fill_bytes(dump1_paddr, 0x92U, AI_FUBP_FULL_VL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vlseg2e8ff_full_dump(seed0_vaddr, seed1_vaddr,
        store_vaddr, dump0_vaddr, dump1_vaddr, seg_vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("Segment-FOF full retry keeps the committed store prefix and does not invent breakpoint-cutoff suffix bytes",
        excpt.triggered == false &&
        observed_vl == seg_vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(dump0_paddr, full_expected0, seg_vl) &&
        ai_fubp_bytes_match(dump1_paddr, full_expected1, seg_vl) &&
        ai_fubp_bytes_match(store_paddr, memory_after_store, store_vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_us_store_translation_fault_repair_then_bp_rewalk_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned split_lane = 8U;
    const unsigned trigger_lane = 10U;
    const uintptr_t dst_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        split_lane;
    const uintptr_t trigger_vaddr = dst_vaddr + trigger_lane;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x720U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x760U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        split_lane;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x720U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x760U;
    uint8_t initial[AI_FUBP_FULL_VL] = { 0 };
    uint8_t payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fault_expected[AI_FUBP_FULL_VL] = { 0 };
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(initial, vl, 0x24U);
    ai_fubp_fill_pattern(payload, vl, 0xa4U);
    for (unsigned i = 0; i < vl; ++i) {
        bp_expected[i] = (i < trigger_lane) ? payload[i] : initial[i];
        fault_expected[i] = (i < split_lane) ? payload[i] : initial[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(low_paddr, initial, split_lane);
    ai_fubp_write_bytes(high_paddr, initial + split_lane, vl - split_lane);
    ai_fubp_write_bytes(src_paddr, payload, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x34U, vl);

    ai_fubp_prepare_three_pages(low_page, PTE_V | PTE_AD | PTE_RWX,
        high_page, PTE_V | PTE_AD | PTE_X,
        aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, vl);
    TEST_ASSERT("store translation rewalk setup loads source payload before the no-write high-page fault",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dst_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("UnitStride store first pass exposes the high-page translation/permission fault before any trigger is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == dst_vaddr + split_lane &&
        observed_vstart_csr == split_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, dst_vaddr + split_lane,
        observed_vstart_csr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("store translation fault commits only the prefix before the no-write high page and no trigger-side suffix",
        ai_fubp_split_bytes_match(low_paddr, high_paddr, split_lane,
            fault_expected, vl)
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    ai_fubp_arm_mcontrol6_slot(0U, trigger_vaddr,
        AI_FUBP_MCONTROL6_STORE);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, vl);
    TEST_ASSERT("store translation rewalk reloads source payload after page repair before arming breakpoint retry",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dst_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("UnitStride store retry rewalks the repaired translation and reports the current address breakpoint lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == trigger_vaddr &&
        observed_vstart_csr == trigger_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu old_fault=0x%lx",
        excpt.cause, excpt.tval, trigger_vaddr, observed_vstart_csr,
        dst_vaddr + split_lane
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("store translation rewalk breakpoint commits through the current trigger lane and keeps suffix old",
        ai_fubp_split_bytes_match(low_paddr, high_paddr, split_lane,
            bp_expected, vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, vl);
    TEST_ASSERT("store translation rewalk final retry reloads source after clearing trigger",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dst_vaddr, vl);
    TEST_ASSERT("store translation rewalk final retry completes after trigger clear and page repair",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(dst_vaddr, vl);
    TEST_ASSERT("store translation rewalk readback can load the final destination image",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("store translation rewalk readback dump completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("store translation rewalk final image is the full payload with no stale fault or breakpoint mask",
        ai_fubp_bytes_match(dump_paddr, payload, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_same_mbindex_cross16_prefix_and_low_bp_mask_manual_corner()
{
    TEST_START();

    const enum test_page load_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned high_flow_trigger_lane = 2U;
    const unsigned low_flow_trigger_lane = 1U;
    const uintptr_t base_vaddr = hs_page_base(load_page) + 0x30eU;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x7a0U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x7e0U;
    const uintptr_t base_paddr = phys_page_base(load_page) + 0x30eU;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x7a0U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x7e0U;
    uint8_t payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t high_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t low_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(payload, vl, 0x2aU);
    ai_fubp_fill_pattern(old, vl, 0xaaU);
    for (unsigned i = 0; i < vl; ++i) {
        high_bp_expected[i] = (i < high_flow_trigger_lane) ?
            payload[i] : old[i];
        low_bp_expected[i] = (i < low_flow_trigger_lane) ?
            payload[i] : old[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(base_paddr, payload, vl);
    ai_fubp_write_bytes(old_paddr, old, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x4aU, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(load_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("same-mBIndex setup preloads old bytes before high-flow trigger probe",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, base_vaddr + high_flow_trigger_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("same-mBIndex high-flow breakpoint keeps lower-flow data and reports the high-flow lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == base_vaddr + high_flow_trigger_lane &&
        observed_vstart_csr == high_flow_trigger_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, base_vaddr + high_flow_trigger_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("same-mBIndex high-flow breakpoint dump completes after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-mBIndex high-flow trigger preserves only the lower prefix and suppresses trigger/suffix bytes",
        ai_fubp_bytes_match(dump_paddr, high_bp_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("same-mBIndex low-flow probe resets destination before low-lane trigger",
        excpt.triggered == false
    );
    ai_fubp_arm_mcontrol6_slot(0U, base_vaddr + low_flow_trigger_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(base_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("same-mBIndex low-flow breakpoint wins before any higher-flow data can be merged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == base_vaddr + low_flow_trigger_lane &&
        observed_vstart_csr == low_flow_trigger_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, base_vaddr + low_flow_trigger_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(dump_paddr, 0x6aU, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("same-mBIndex low-flow breakpoint dump completes after clearing trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-mBIndex low-flow trigger blocks all younger flow data and leaves destination at old sentinel",
        ai_fubp_bytes_match(dump_paddr, low_bp_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(base_vaddr, vl);
    TEST_ASSERT("same-mBIndex final clear path reloads full payload after both trigger masks are cleared",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(dump_vaddr, vl);
    TEST_ASSERT("same-mBIndex final dump completes for full payload check",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("same-mBIndex final image proves no stale high-flow or low-flow trigger mask remains resident",
        ai_fubp_bytes_match(dump_paddr, payload, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_trigger_priv_filter_us_store_fof_owner_manual_corner()
{
    TEST_START();

    const enum test_page store_page = VSRWX_GRWX;
    const enum test_page src_page = VSRW_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned store_lane = 6U;
    const unsigned fof_bp_lane = 4U;
    const unsigned fof_fault_lane = 10U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x840U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x880U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x8c0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x900U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x840U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x880U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x8c0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x900U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    uint8_t initial[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(initial, vl, 0x36U);
    ai_fubp_fill_pattern(store_payload, vl, 0xb6U);
    ai_fubp_fill_pattern(seed, vl, 0x56U);
    ai_fubp_fill_pattern(fof_payload, vl, 0xd6U);
    for (unsigned i = 0; i < vl; ++i) {
        store_bp_expected[i] = (i < store_lane) ?
            store_payload[i] : initial[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ? fof_payload[i] :
            seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(store_paddr, initial, vl);
    ai_fubp_write_bytes(src_paddr, store_payload, vl);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x46U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(store_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, vl);
    TEST_ASSERT("priv-filter setup loads store source payload before arming an M-only trigger in HS",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot_priv(0U, store_vaddr + store_lane,
        AI_FUBP_MCONTROL6_STORE, AI_FUBP_MCONTROL6_M);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, vl);
    TEST_ASSERT("M-only store address trigger must not fire for the same HS UnitStride store VA",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL &&
        ai_fubp_bytes_match(store_paddr, store_payload, vl)
    );

    goto_priv(PRIV_M);
    ai_fubp_write_bytes(store_paddr, initial, vl);
    ai_fubp_arm_mcontrol6_slot_priv(0U, store_vaddr + store_lane,
        AI_FUBP_MCONTROL6_STORE, AI_FUBP_MCONTROL6_S);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(src_vaddr, vl);
    TEST_ASSERT("priv-filter reloads store source before the S-enabled UnitStride store breakpoint",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("S-enabled store trigger fires in HS and cuts only the current store prefix",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_vaddr + store_lane &&
        observed_vstart_csr == store_lane &&
        ai_fubp_bytes_match(store_paddr, store_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, store_vaddr + store_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x66U, vl);
    ai_fubp_arm_mcontrol6_slot_priv(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD, AI_FUBP_MCONTROL6_M);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("M-only load trigger must not fire for HS VFOF and the real first-fault lane owns vl",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u stale_trigger=0x%lx",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart,
        fof_vaddr + fof_bp_lane
    );

    ai_fubp_fill_bytes(fof_dump_paddr, 0x76U, vl);
    ai_fubp_arm_mcontrol6_slot_priv(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD, AI_FUBP_MCONTROL6_S);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("S-enabled VFOF load trigger reports the HS breakpoint lane instead of stale M-only first-fault metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    ai_fubp_fill_bytes(fof_dump_paddr, 0x86U, vl);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the S-enabled VFOF trigger exposes the current first-fault lane with no privilege-filter residue",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_prefetch_address_trigger_silent_then_us_fof_fresh_manual_corner()
{
    TEST_START();

    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_lane = 5U;
    const unsigned store_lane = 6U;
    const unsigned fof_bp_lane = 3U;
    const unsigned fof_fault_lane = 10U;
    const uintptr_t us_src_vaddr = hs_page_base(data_page) + 0x940U;
    const uintptr_t store_vaddr = hs_page_base(data_page) + 0xa00U;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0xb00U;
    const uintptr_t store_src_vaddr = hs_page_base(aux_page) + 0xb40U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0xb80U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xbc0U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xc00U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t us_src_paddr = phys_page_base(data_page) + 0x940U;
    const uintptr_t store_paddr = phys_page_base(data_page) + 0xa00U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0xb00U;
    const uintptr_t store_src_paddr = phys_page_base(aux_page) + 0xb40U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0xb80U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xbc0U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xc00U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_initial[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, vl, 0x27U);
    ai_fubp_fill_pattern(us_old, vl, 0xa7U);
    ai_fubp_fill_pattern(store_initial, vl, 0x37U);
    ai_fubp_fill_pattern(store_payload, vl, 0xb7U);
    ai_fubp_fill_pattern(seed, vl, 0x57U);
    ai_fubp_fill_pattern(fof_payload, vl, 0xd7U);
    for (unsigned i = 0; i < vl; ++i) {
        us_expected[i] = (i < us_lane) ? us_payload[i] : us_old[i];
        store_bp_expected[i] = (i < store_lane) ?
            store_payload[i] : store_initial[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ?
            fof_payload[i] : seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_src_paddr, us_payload, vl);
    ai_fubp_write_bytes(old_paddr, us_old, vl);
    ai_fubp_write_bytes(store_paddr, store_initial, vl);
    ai_fubp_write_bytes(store_src_paddr, store_payload, vl);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_fill_bytes(us_dump_paddr, 0x47U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x67U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(data_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("prefetch-address setup preloads old v6 bytes before the real US load breakpoint",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_src_vaddr + us_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(us_src_vaddr + us_lane);
    TEST_ASSERT("prefetch.r with a load address trigger armed must retire silently and leave the following US trigger fresh",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_src_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);
    TEST_ASSERT("real UnitStride load after silent prefetch.r reports the current address-trigger lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_src_vaddr + us_lane &&
        observed_vstart_csr == us_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_src_vaddr + us_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("US load prefix dump after silent prefetch.r completes after clearing trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("silent prefetch.r does not consume the US breakpoint or clobber its prefix/suffix state",
        ai_fubp_bytes_match(us_dump_paddr, us_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src_vaddr, vl);
    TEST_ASSERT("prefetch-address setup loads store payload before the prefetch.w probe",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, store_vaddr + store_lane,
        AI_FUBP_MCONTROL6_STORE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(store_vaddr + store_lane);
    TEST_ASSERT("prefetch.w with a store address trigger armed must retire silently before the real US store",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("real UnitStride store after silent prefetch.w reports the current store breakpoint and commits only prefix bytes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_vaddr + store_lane &&
        observed_vstart_csr == store_lane &&
        ai_fubp_bytes_match(store_paddr, store_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, store_vaddr + store_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    ai_fubp_fill_bytes(fof_dump_paddr, 0x77U, vl);
    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    prefetch_r(fof_vaddr + fof_bp_lane);
    TEST_ASSERT("prefetch.r to the VFOF trigger lane must not consume the later VFOF breakpoint",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("real VFOF after silent prefetch.r reports the address breakpoint before the later first-fault lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x87U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the trigger after silent-prefetch VFOF exposes the current first-fault lane without stale trigger state",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_pte_ad_us_vfof_bp_side_effect_boundary_manual_corner()
{
    TEST_START();

    const enum test_page load_low_page = SWITCH1;
    const enum test_page load_high_page = SWITCH2;
    const enum test_page store_low_page = VSRWX_GRWX;
    const enum test_page store_high_page = VSRWX_GRW;
    const enum test_page store_src_page = VSRX_GRWX;
    const enum test_page fof_low_page = VSR_GRWX;
    const enum test_page fof_high_page = VSR_GRW;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned load_lane = 7U;
    const unsigned store_lane = 6U;
    const unsigned fof_fault_lane = 10U;
    const uintptr_t load_vaddr = hs_page_base(load_low_page) + PAGE_SIZE -
        load_lane;
    const uintptr_t load_trigger_vaddr = hs_page_base(load_high_page);
    const uintptr_t store_vaddr = hs_page_base(store_low_page) + PAGE_SIZE -
        store_lane;
    const uintptr_t store_trigger_vaddr = hs_page_base(store_high_page);
    const uintptr_t store_src_vaddr = hs_page_base(store_src_page) + 0x240U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t load_dump_vaddr = hs_page_base(aux_page) + 0x2c0U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x340U;
    const uintptr_t load_low_paddr = phys_page_base(load_low_page) +
        PAGE_SIZE - load_lane;
    const uintptr_t load_high_paddr = phys_page_base(load_high_page);
    const uintptr_t store_low_paddr = phys_page_base(store_low_page) +
        PAGE_SIZE - store_lane;
    const uintptr_t store_high_paddr = phys_page_base(store_high_page);
    const uintptr_t store_src_paddr = phys_page_base(store_src_page) +
        0x240U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x280U;
    const uintptr_t load_dump_paddr = phys_page_base(aux_page) + 0x2c0U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x340U;
    uint8_t load_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t load_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t load_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_initial[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(load_payload, vl, 0x21U);
    ai_fubp_fill_pattern(load_old, vl, 0xa1U);
    ai_fubp_fill_pattern(store_initial, vl, 0x31U);
    ai_fubp_fill_pattern(store_payload, vl, 0xb1U);
    ai_fubp_fill_pattern(fof_payload, vl, 0x51U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xd1U);
    for (unsigned i = 0; i < vl; ++i) {
        load_bp_expected[i] = (i < load_lane) ?
            load_payload[i] : load_old[i];
        store_bp_expected[i] = (i < store_lane) ?
            store_payload[i] : store_initial[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ?
            fof_payload[i] : fof_seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(load_low_paddr, load_payload, load_lane);
    ai_fubp_write_bytes(load_high_paddr, load_payload + load_lane,
        vl - load_lane);
    ai_fubp_write_bytes(old_paddr, load_old, vl);
    ai_fubp_fill_bytes(load_dump_paddr, 0x22U, vl);
    ai_fubp_write_bytes(store_low_paddr, store_initial, store_lane);
    ai_fubp_write_bytes(store_high_paddr, store_initial + store_lane,
        vl - store_lane);
    ai_fubp_write_bytes(store_src_paddr, store_payload, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x52U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(load_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(load_high_page, PTE_V | PTE_D | PTE_RWX);
    ai_fubp_set_hs_leaf(store_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(store_high_page, PTE_V | PTE_A | PTE_RWX);
    ai_fubp_set_hs_leaf(store_src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_D | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("PTE A/D side-effect setup preloads old vector bytes before the A=0 load breakpoint probe",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, load_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    const uint64_t load_high_leaf_after_bp =
        *ai_fubp_hs_leaf_entry(load_high_page);
    TEST_ASSERT("A=0 high-page UnitStride load reports the address breakpoint first and does not set the high leaf A bit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == load_trigger_vaddr &&
        observed_vstart_csr == load_lane &&
        (load_high_leaf_after_bp & PTE_A) == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu leaf=0x%lx",
        excpt.cause, excpt.tval, load_trigger_vaddr,
        observed_vstart_csr, load_high_leaf_after_bp
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(load_dump_vaddr, vl);
    TEST_ASSERT("A=0 load breakpoint prefix can be dumped after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("A=0 load breakpoint updates only the low-page prefix and leaves the trigger lane old",
        ai_fubp_bytes_match(load_dump_paddr, load_bp_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the load trigger exposes the current A=0 page fault at the same high-page VA without changing the leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_trigger_vaddr &&
        observed_vstart_csr == load_lane &&
        (*ai_fubp_hs_leaf_entry(load_high_page) & PTE_A) == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu leaf=0x%lx",
        excpt.cause, excpt.tval, load_trigger_vaddr,
        observed_vstart_csr, *ai_fubp_hs_leaf_entry(load_high_page)
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(load_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(load_dump_paddr, 0x62U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_vaddr, vl);
    TEST_ASSERT("repairing the load A bit lets the same UnitStride load complete as a fresh full image",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(load_dump_vaddr, vl);
    TEST_ASSERT("A-bit repair full-load dump completes after the breakpoint and page-fault rounds",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("A-bit repair full-load image proves no stale breakpoint prefix survived",
        ai_fubp_bytes_match(load_dump_paddr, load_payload, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src_vaddr, vl);
    TEST_ASSERT("D=0 store setup loads source bytes before the store breakpoint probe",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, store_trigger_vaddr,
        AI_FUBP_MCONTROL6_STORE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    const uint64_t store_high_leaf_after_bp =
        *ai_fubp_hs_leaf_entry(store_high_page);
    TEST_ASSERT("D=0 high-page UnitStride store reports the address breakpoint first, writes only the low-page prefix, and leaves D clear",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_trigger_vaddr &&
        observed_vstart_csr == store_lane &&
        (store_high_leaf_after_bp & PTE_D) == 0ULL &&
        ai_fubp_split_bytes_match(store_low_paddr, store_high_paddr,
            store_lane, store_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu leaf=0x%lx high0=0x%x",
        excpt.cause, excpt.tval, store_trigger_vaddr,
        observed_vstart_csr, store_high_leaf_after_bp,
        read8(store_high_paddr)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src_vaddr, vl);
    TEST_ASSERT("D=0 store retry reloads source after clearing the breakpoint",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the store trigger exposes the D=0 page fault and still preserves the high-page suffix",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_trigger_vaddr &&
        observed_vstart_csr == store_lane &&
        (*ai_fubp_hs_leaf_entry(store_high_page) & PTE_D) == 0ULL &&
        ai_fubp_split_bytes_match(store_low_paddr, store_high_paddr,
            store_lane, store_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu leaf=0x%lx",
        excpt.cause, excpt.tval, store_trigger_vaddr,
        observed_vstart_csr, *ai_fubp_hs_leaf_entry(store_high_page)
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(store_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src_vaddr, vl);
    TEST_ASSERT("D-bit repair reloads source for the final full store",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, vl);
    TEST_ASSERT("D-bit repair lets the final UnitStride store complete without stale breakpoint or page-fault state",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("D-bit repair final store image proves the high-page suffix was written exactly after repair",
        ai_fubp_split_bytes_match(store_low_paddr, store_high_paddr,
            store_lane, store_payload, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    const uint64_t fof_high_leaf_after_fault =
        *ai_fubp_hs_leaf_entry(fof_high_page);
    TEST_ASSERT("VFOF nonzero first-fault on an A=0 high leaf silently shortens vl, preserves suffix seed, and leaves the high leaf A bit clear",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        (fof_high_leaf_after_fault & PTE_A) == 0ULL &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u leaf=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, fof_high_leaf_after_fault
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x72U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr, fof_dump_vaddr,
        vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("repairing the VFOF A bit converts the same cross-page FOF load into a full fresh success",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_pmp_us_vfof_bp_priority_matrix_manual_corner()
{
    TEST_START();

    const enum test_page load_low_page = SWITCH1;
    const enum test_page load_high_page = SWITCH2;
    const enum test_page store_low_page = VSRWX_GRWX;
    const enum test_page store_high_page = VSRWX_GRW;
    const enum test_page store_src_page = VSRX_GRWX;
    const enum test_page fof_low_page = VSR_GRWX;
    const enum test_page fof_high_page = VSR_GRW;
    const enum test_page elem0_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned load_lane = 7U;
    const unsigned store_lane = 6U;
    const unsigned fof_fault_lane = 10U;
    const unsigned fof_bp_lane = 4U;
    const uintptr_t load_vaddr = hs_page_base(load_low_page) + PAGE_SIZE -
        load_lane;
    const uintptr_t load_trigger_vaddr = hs_page_base(load_high_page);
    const uintptr_t store_vaddr = hs_page_base(store_low_page) + PAGE_SIZE -
        store_lane;
    const uintptr_t store_trigger_vaddr = hs_page_base(store_high_page);
    const uintptr_t store_src_vaddr = hs_page_base(store_src_page) + 0x440U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x480U;
    const uintptr_t load_dump_vaddr = hs_page_base(aux_page) + 0x4c0U;
    const uintptr_t fof_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t elem0_vaddr = hs_page_base(elem0_page) + 0x540U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x580U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x5c0U;
    const uintptr_t elem0_dump_vaddr = hs_page_base(aux_page) + 0x600U;
    const uintptr_t load_low_paddr = phys_page_base(load_low_page) +
        PAGE_SIZE - load_lane;
    const uintptr_t load_high_paddr = phys_page_base(load_high_page);
    const uintptr_t store_low_paddr = phys_page_base(store_low_page) +
        PAGE_SIZE - store_lane;
    const uintptr_t store_high_paddr = phys_page_base(store_high_page);
    const uintptr_t store_src_paddr = phys_page_base(store_src_page) +
        0x440U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x480U;
    const uintptr_t load_dump_paddr = phys_page_base(aux_page) + 0x4c0U;
    const uintptr_t fof_low_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fof_high_page);
    const uintptr_t elem0_paddr = phys_page_base(elem0_page) + 0x540U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x580U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x5c0U;
    const uintptr_t elem0_dump_paddr = phys_page_base(aux_page) + 0x600U;
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    uint8_t load_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t load_old[AI_FUBP_FULL_VL] = { 0 };
    uint8_t load_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_initial[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t store_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t elem0_payload[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(load_payload, vl, 0x28U);
    ai_fubp_fill_pattern(load_old, vl, 0xa8U);
    ai_fubp_fill_pattern(store_initial, vl, 0x38U);
    ai_fubp_fill_pattern(store_payload, vl, 0xb8U);
    ai_fubp_fill_pattern(fof_payload, vl, 0x58U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xd8U);
    ai_fubp_fill_pattern(elem0_payload, vl, 0x78U);
    for (unsigned i = 0; i < vl; ++i) {
        load_bp_expected[i] = (i < load_lane) ?
            load_payload[i] : load_old[i];
        store_bp_expected[i] = (i < store_lane) ?
            store_payload[i] : store_initial[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ?
            fof_payload[i] : fof_seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ?
            fof_payload[i] : fof_seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(load_low_paddr, load_payload, load_lane);
    ai_fubp_write_bytes(load_high_paddr, load_payload + load_lane,
        vl - load_lane);
    ai_fubp_write_bytes(old_paddr, load_old, vl);
    ai_fubp_fill_bytes(load_dump_paddr, 0x29U, vl);
    ai_fubp_write_bytes(store_low_paddr, store_initial, store_lane);
    ai_fubp_write_bytes(store_high_paddr, store_initial + store_lane,
        vl - store_lane);
    ai_fubp_write_bytes(store_src_paddr, store_payload, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(elem0_paddr, elem0_payload, vl);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x59U, vl);
    ai_fubp_fill_bytes(elem0_dump_paddr, 0x79U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(load_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(load_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(store_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(store_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(store_src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fof_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(elem0_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("PMP priority setup preloads old vector bytes before the high-page load breakpoint/access-fault matrix",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, load_trigger_vaddr,
        AI_FUBP_MCONTROL6_LOAD);
    ai_fubp_configure_pmp_window(phys_page_base(load_high_page),
        phys_page_base(load_high_page) + PAGE_SIZE, AI_FUBP_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("PMP no-read and address trigger on the same high-page load lane report exactly one stable breakpoint owner while armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == load_trigger_vaddr &&
        observed_vstart_csr == load_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, load_trigger_vaddr,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(load_dump_vaddr, vl);
    TEST_ASSERT("PMP load breakpoint prefix dump completes before the fallback access-fault probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PMP load breakpoint keeps only the current low-page prefix before the denied high-page byte",
        ai_fubp_bytes_match(load_dump_paddr, load_bp_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the load trigger exposes the same high-page lane as a PMP load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == load_trigger_vaddr &&
        observed_vstart_csr == load_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, load_trigger_vaddr,
        observed_vstart_csr
    );

    ai_fubp_configure_pmp_window(0ULL, (uintptr_t)-1,
        AI_FUBP_PMP_TOR_RWX);
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(load_vaddr, vl);
    TEST_ASSERT("restoring PMP read permission lets the high-page UnitStride load complete",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(load_dump_vaddr, vl);
    TEST_ASSERT("restored-PMP load dump completes for final full-image check",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restored-PMP load final image proves no stale breakpoint or access-fault metadata survives",
        ai_fubp_bytes_match(load_dump_paddr, load_payload, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src_vaddr, vl);
    TEST_ASSERT("PMP store priority setup loads source payload before the no-write matrix",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, store_trigger_vaddr,
        AI_FUBP_MCONTROL6_STORE);
    ai_fubp_configure_pmp_window(phys_page_base(store_high_page),
        phys_page_base(store_high_page) + PAGE_SIZE, AI_FUBP_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("PMP no-write and store address trigger on the same high-page lane report breakpoint first and commit only prefix bytes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_trigger_vaddr &&
        observed_vstart_csr == store_lane &&
        ai_fubp_split_bytes_match(store_low_paddr, store_high_paddr,
            store_lane, store_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu high0=0x%x",
        excpt.cause, excpt.tval, store_trigger_vaddr,
        observed_vstart_csr, read8(store_high_paddr)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src_vaddr, vl);
    TEST_ASSERT("PMP store fallback reloads source after clearing the store trigger",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the store trigger exposes the same high-page lane as a PMP store access fault without writing suffix bytes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == store_trigger_vaddr &&
        observed_vstart_csr == store_lane &&
        ai_fubp_split_bytes_match(store_low_paddr, store_high_paddr,
            store_lane, store_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu high0=0x%x",
        excpt.cause, excpt.tval, store_trigger_vaddr,
        observed_vstart_csr, read8(store_high_paddr)
    );

    ai_fubp_configure_pmp_window(0ULL, (uintptr_t)-1,
        AI_FUBP_PMP_TOR_RWX);
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(store_src_vaddr, vl);
    TEST_ASSERT("restored-PMP store reloads source for final full-store check",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(store_vaddr, vl);
    TEST_ASSERT("restored-PMP store completes after breakpoint and access-fault rounds",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restored-PMP store final image proves no ghost suffix was forwarded before repair",
        ai_fubp_split_bytes_match(store_low_paddr, store_high_paddr,
            store_lane, store_payload, vl)
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    ai_fubp_configure_pmp_window(phys_page_base(fof_high_page),
        phys_page_base(fof_high_page) + PAGE_SIZE, AI_FUBP_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF prefix address breakpoint remains visible before a later PMP nonzero first-fault lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x69U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the VFOF prefix trigger exposes the PMP nonzero first-fault as a silent shortened-vl",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_configure_pmp_window(phys_page_base(elem0_page),
        phys_page_base(elem0_page) + PAGE_SIZE, AI_FUBP_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, elem0_vaddr,
        elem0_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF element0 PMP no-read fault is visible and must not be converted into a silent nonzero shortened-vl",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == elem0_vaddr &&
        observed_vl == vl &&
        ai_fubp_bytes_match(elem0_dump_paddr, fof_seed, vl),
        "triggered=%d cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, elem0_vaddr,
        observed_vl, observed_vstart
    );

    ai_fubp_configure_pmp_window(0ULL, (uintptr_t)-1,
        AI_FUBP_PMP_TOR_RWX);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x89U, vl);
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring PMP read permission lets the original cross-page VFOF complete as a fresh full image",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_restore_pmp(saved_pmpcfg0, saved_pmpaddr0, saved_pmpaddr1,
        saved_pmpaddr2);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_cbozero_fencei_us_vfof_bp_fresh_owner_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRW_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned cbo_block_bytes = 64U;
    const unsigned us_bp_lane = 7U;
    const unsigned fof_bp_lane = 4U;
    const unsigned fof_fault_lane = 10U;
    const uintptr_t fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t block_vaddr = fof_vaddr & ~((uintptr_t)63U);
    const uintptr_t us_vaddr = block_vaddr + 16U;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x700U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x740U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x780U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x7c0U;
    const uintptr_t block_paddr = phys_page_base(low_page) +
        (block_vaddr - hs_page_base(low_page));
    const uintptr_t us_paddr = block_paddr + 16U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x700U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x740U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x780U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x7c0U;
    const uintptr_t guard_paddr = block_paddr - sizeof(uint64_t);
    const uint64_t guard_value = 0xc0b0feed13572468ULL;
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint8_t block_seed[64] = { 0 };
    uint8_t zero_block[64] = { 0 };
    uint8_t zero_vec[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_vec[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t high_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_full_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(block_seed, cbo_block_bytes, 0x91U);
    ai_fubp_fill_pattern(old_vec, vl, 0xb1U);
    ai_fubp_fill_pattern(high_payload, vl, 0xd1U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xe1U);
    for (unsigned i = 0; i < vl; ++i) {
        us_bp_expected[i] = (i < us_bp_lane) ? 0U : old_vec[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ? 0U : fof_seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ? 0U : fof_seed[i];
        fof_full_expected[i] = (i < fof_fault_lane) ?
            0U : high_payload[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    ai_fubp_write_bytes(block_paddr, block_seed, cbo_block_bytes);
    write64(guard_paddr, guard_value);
    ai_fubp_write_bytes(high_paddr, high_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(old_paddr, old_vec, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x71U, vl);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x72U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    cbo_zero(block_vaddr);
    asm volatile("fence.i" ::: "memory");
    TEST_ASSERT("CBO.ZERO plus fence.i retires on the ordinary DRAM vector block before US/VFOF trigger probes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("CBO.ZERO clears exactly the target data block and preserves the pre-block guard before vector breakpoint probes",
        ai_fubp_bytes_match(block_paddr, zero_block, cbo_block_bytes) &&
        read64(guard_paddr) == guard_value
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("US/CBO setup preloads old vector bytes before the zeroed-block breakpoint probe",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride load breakpoint after CBO.ZERO reports the current data address and only imports zeroed prefix bytes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("US breakpoint dump after CBO.ZERO completes for prefix freshness check",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("US breakpoint after CBO.ZERO keeps zero prefix and old suffix, not the pre-CBO payload",
        ai_fubp_bytes_match(us_dump_paddr, us_bp_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    TEST_ASSERT("clearing the US trigger lets the same zeroed block load complete without stale breakpoint owner",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("full US zeroed-block dump completes after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("full US retry after CBO.ZERO observes all-zero data rather than the old block seed",
        ai_fubp_bytes_match(us_dump_paddr, zero_vec, vl)
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x73U, vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF prefix breakpoint after CBO.ZERO reports current trigger VA before later no-read lane and dumps zeroed prefix",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x74U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing the VFOF trigger exposes the later no-read lane as silent shortened-vl with zeroed prefix",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x75U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("repairing the high page lets VFOF complete with zeroed low-page bytes and fresh high-page payload",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_full_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_handler_vector_spill_between_us_vfof_bp_retry_manual_corner()
{
    TEST_START();

    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_bp_lane = 6U;
    const unsigned fof_bp_lane = 4U;
    const unsigned fof_fault_lane = 10U;
    const uintptr_t us_vaddr = hs_page_base(us_page) + 0x580U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t handler_src_vaddr = hs_page_base(aux_page) + 0x180U;
    const uintptr_t handler_dst_vaddr = hs_page_base(aux_page) + 0x1c0U;
    const uintptr_t handler_save_vaddr = hs_page_base(aux_page) + 0x200U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t us_paddr = phys_page_base(us_page) + 0x580U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x140U;
    const uintptr_t handler_src_paddr = phys_page_base(aux_page) + 0x180U;
    const uintptr_t handler_dst_paddr = phys_page_base(aux_page) + 0x1c0U;
    const uintptr_t handler_save_paddr = phys_page_base(aux_page) + 0x200U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x280U;
    const uintptr_t fof_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(high_page);
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t handler_src[AI_FUBP_FULL_VL] = { 0 };
    uint8_t handler_save[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_full_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(us_payload, vl, 0x26U);
    ai_fubp_fill_pattern(old_payload, vl, 0xa6U);
    ai_fubp_fill_pattern(handler_src, vl, 0x46U);
    ai_fubp_fill_pattern(handler_save, vl, 0x86U);
    ai_fubp_fill_pattern(fof_payload, vl, 0x66U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xc6U);
    for (unsigned i = 0; i < vl; ++i) {
        us_bp_expected[i] = (i < us_bp_lane) ? us_payload[i] :
            old_payload[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            fof_seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ?
            fof_payload[i] : fof_seed[i];
        fof_full_expected[i] = fof_payload[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_paddr, us_payload, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(handler_src_paddr, handler_src, vl);
    ai_fubp_fill_bytes(handler_dst_paddr, 0x36U, vl);
    ai_fubp_write_bytes(handler_save_paddr, handler_save, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x56U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x76U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("handler-vector-spill setup preloads old US destination bytes before the first breakpoint",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("US breakpoint reports its current lane before any handler-style vector spill/fill runs",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vector_spill_fill_scratch(handler_src_vaddr,
        handler_dst_vaddr, handler_save_vaddr, vl / 2U, vl);
    TEST_ASSERT("handler-style vector spill/fill between US breakpoint and retry completes on independent scratch buffers",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("US breakpoint prefix dump survives the intervening handler-style vector spill/fill",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("handler-style vector scratch image is exact and does not corrupt the resident US breakpoint prefix",
        ai_fubp_bytes_match(handler_dst_paddr, handler_src, vl) &&
        ai_fubp_bytes_match(handler_save_paddr, handler_save, vl) &&
        ai_fubp_bytes_match(us_dump_paddr, us_bp_expected, vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    TEST_ASSERT("US retry after handler-style vector spill/fill completes once the trigger is cleared",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("US retry dump after handler-style vector spill/fill completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("US retry after handler-style vector spill/fill loads the full current payload, not handler scratch bytes",
        ai_fubp_bytes_match(us_dump_paddr, us_payload, vl)
    );

    ai_fubp_fill_bytes(handler_dst_paddr, 0x37U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x77U, vl);
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_seed_capture_no_dump(fof_seed_vaddr, fof_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF breakpoint reports the current nonzero lane before handler-style vector context traffic",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vector_spill_fill_scratch(handler_src_vaddr,
        handler_dst_vaddr, handler_save_vaddr, vl / 2U, vl);
    TEST_ASSERT("handler-style vector spill/fill between VFOF breakpoint and dump completes without reusing VFOF metadata",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v8(fof_dump_vaddr, vl);
    TEST_ASSERT("VFOF breakpoint prefix dump completes after handler-style vector spill/fill",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF breakpoint prefix survives handler-style vector traffic and keeps handler scratch isolated",
        ai_fubp_bytes_match(handler_dst_paddr, handler_src, vl) &&
        ai_fubp_bytes_match(handler_save_paddr, handler_save, vl) &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x78U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing VFOF breakpoint after handler-style vector traffic exposes the later nonzero first-fault lane",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x79U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF retry after page repair and handler-style vector traffic completes with the fresh full payload",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_full_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_vfof_elem0_fault_then_cross16_us_bp_mab_owner_manual_corner()
{
    TEST_START();

    const enum test_page fof_page = SWITCH1;
    const enum test_page mab_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned mab_trigger_lane = 2U;
    const uintptr_t fof_vaddr = hs_page_base(fof_page) + 0x480U;
    const uintptr_t mab_vaddr = hs_page_base(mab_page) + 0x30fU;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x340U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x380U;
    const uintptr_t mab_dump_vaddr = hs_page_base(aux_page) + 0x3c0U;
    const uintptr_t fof_paddr = phys_page_base(fof_page) + 0x480U;
    const uintptr_t mab_paddr = phys_page_base(mab_page) + 0x30fU;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x340U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x380U;
    const uintptr_t mab_dump_paddr = phys_page_base(aux_page) + 0x3c0U;
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t mab_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t mab_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(fof_payload, vl, 0x2dU);
    ai_fubp_fill_pattern(fof_seed, vl, 0xadU);
    ai_fubp_fill_pattern(mab_payload, vl, 0x4dU);
    ai_fubp_fill_pattern(old_payload, vl, 0xcdU);
    for (unsigned i = 0; i < vl; ++i) {
        mab_bp_expected[i] = (i < mab_trigger_lane) ?
            mab_payload[i] : old_payload[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(fof_paddr, fof_payload, vl);
    ai_fubp_write_bytes(mab_paddr, mab_payload, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x5dU, vl);
    ai_fubp_fill_bytes(mab_dump_paddr, 0x6dU, vl);

    ai_fubp_prepare_three_pages(fof_page, PTE_V | PTE_AD | PTE_X,
        mab_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("element0 VFOF page fault is visible before the following cross16 UnitStride/MAB breakpoint probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fof_vaddr &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_seed, vl),
        "triggered=%d cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, fof_vaddr,
        observed_vl, observed_vstart
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("cross16 MAB owner setup preloads old destination bytes after the older VFOF element0 fault",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, mab_vaddr + mab_trigger_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(mab_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("cross16 UnitStride load breakpoint after older VFOF fault reports its own MAB-side VA and lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == mab_vaddr + mab_trigger_lane &&
        observed_vstart_csr == mab_trigger_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu old_fof=0x%lx",
        excpt.cause, excpt.tval, mab_vaddr + mab_trigger_lane,
        observed_vstart_csr, fof_vaddr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(mab_dump_vaddr, vl);
    TEST_ASSERT("cross16 UnitStride breakpoint prefix dump completes after clearing the MAB-side trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross16 UnitStride/MAB breakpoint keeps current prefix and does not inherit the older VFOF element0 fault metadata",
        ai_fubp_bytes_match(mab_dump_paddr, mab_bp_expected, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(mab_vaddr, vl);
    TEST_ASSERT("cross16 UnitStride retry after MAB-side trigger clear completes without stale VFOF exception state",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(mab_dump_vaddr, vl);
    TEST_ASSERT("cross16 UnitStride retry dump completes for full-image check",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cross16 UnitStride retry sees the full current payload after the older VFOF exception round",
        ai_fubp_bytes_match(mab_dump_paddr, mab_payload, vl)
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(fof_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x7dU, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("repairing the older VFOF page after the cross16 MAB episode produces a fresh full VFOF image",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_wrong_path_store_bp_kill_then_vfof_no_ghost_forward_manual_corner()
{
    TEST_START();

    const enum test_page target_page = VSRW_GRWX;
    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned wrong_trigger_lane = 6U;
    const unsigned reuse_trigger_lane = 11U;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x680U;
    const uintptr_t wrong_src_vaddr = hs_page_base(src_page) + 0x500U;
    const uintptr_t correct_src_vaddr = hs_page_base(src_page) + 0x540U;
    const uintptr_t reuse_src_vaddr = hs_page_base(src_page) + 0x580U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x600U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x640U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x680U;
    const uintptr_t wrong_src_paddr = phys_page_base(src_page) + 0x500U;
    const uintptr_t correct_src_paddr = phys_page_base(src_page) + 0x540U;
    const uintptr_t reuse_src_paddr = phys_page_base(src_page) + 0x580U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x600U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x640U;
    uint8_t old_mem[AI_FUBP_FULL_VL] = { 0 };
    uint8_t wrong_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t correct_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t reuse_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t reuse_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_fubp_fill_pattern(old_mem, vl, 0x2eU);
    ai_fubp_fill_pattern(wrong_payload, vl, 0xaeU);
    ai_fubp_fill_pattern(correct_payload, vl, 0x4eU);
    ai_fubp_fill_pattern(reuse_payload, vl, 0xceU);
    ai_fubp_fill_pattern(seed, vl, 0x6eU);
    for (unsigned i = 0; i < vl; ++i) {
        reuse_bp_expected[i] = (i < reuse_trigger_lane) ?
            reuse_payload[i] : correct_payload[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(target_paddr, old_mem, vl);
    ai_fubp_write_bytes(wrong_src_paddr, wrong_payload, vl);
    ai_fubp_write_bytes(correct_src_paddr, correct_payload, vl);
    ai_fubp_write_bytes(reuse_src_paddr, reuse_payload, vl);
    ai_fubp_write_bytes(seed_paddr, seed, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x7eU, vl);

    ai_fubp_prepare_three_pages(target_page, PTE_V | PTE_AD | PTE_RWX,
        src_page, PTE_V | PTE_AD | PTE_RWX,
        aux_page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_branch_wrong_store_right_vfof(wrong_src_vaddr, target_vaddr,
        seed_vaddr, target_vaddr, dump_vaddr, vl, 0U, &observed_vl,
        &observed_vstart);
    TEST_ASSERT("wrong-path store helper fallthrough path can commit the poison payload before redirect-kill probing",
        excpt.triggered == false &&
        observed_vl == 0U &&
        observed_vstart == 0U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("wrong-path store helper training writes the poison payload when the branch is not taken",
        ai_fubp_bytes_match(target_paddr, wrong_payload, vl)
    );
    ai_fubp_write_bytes(target_paddr, old_mem, vl);
    ai_fubp_fill_bytes(dump_paddr, 0x7fU, vl);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, target_vaddr + wrong_trigger_lane,
        AI_FUBP_MCONTROL6_STORE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_branch_wrong_store_right_vfof(wrong_src_vaddr, target_vaddr,
        seed_vaddr, target_vaddr, dump_vaddr, vl, 1U, &observed_vl,
        &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("redirect-killed wrong-path UnitStride store breakpoint produces no visible trap and no VFOF ghost-forward bytes",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(target_paddr, old_mem, vl) &&
        ai_fubp_bytes_match(dump_paddr, old_mem, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u target_lane=%x dump_lane=%x",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart, read8(target_paddr + wrong_trigger_lane),
        read8(dump_paddr + wrong_trigger_lane)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(correct_src_vaddr, vl);
    TEST_ASSERT("correct-path committed store setup loads the observer payload after the killed wrong-path SQ round",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, vl);
    TEST_ASSERT("correct-path committed store completes so the following VFOF observer can prove visibility",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(seed_vaddr, target_vaddr, dump_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF observer sees the real correct-path store but not the killed wrong-path store",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(target_paddr, correct_payload, vl) &&
        ai_fubp_bytes_match(dump_paddr, correct_payload, vl),
        "triggered=%d cause=0x%lx vl=%u vstart=%u lane=%x expected=%x",
        excpt.triggered, excpt.cause, observed_vl, observed_vstart,
        read8(dump_paddr + wrong_trigger_lane),
        correct_payload[wrong_trigger_lane]
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, target_vaddr + reuse_trigger_lane,
        AI_FUBP_MCONTROL6_STORE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(reuse_src_vaddr, vl);
    TEST_ASSERT("SQ free-count reuse setup loads a second store payload after killed and committed store episodes",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("later real store breakpoint uses its new lane and leaves suffix from the committed store, not the killed store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == target_vaddr + reuse_trigger_lane &&
        observed_vstart_csr == reuse_trigger_lane &&
        ai_fubp_bytes_match(target_paddr, reuse_bp_expected, vl),
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu old_lane=%u new_lane=%u",
        excpt.cause, excpt.tval, target_vaddr + reuse_trigger_lane,
        observed_vstart_csr, wrong_trigger_lane, reuse_trigger_lane
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(reuse_src_vaddr, vl);
    TEST_ASSERT("SQ free-count final retry reloads payload after clearing the new store breakpoint",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(target_vaddr, vl);
    TEST_ASSERT("SQ free-count final retry completes all bytes after trigger clear",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("final target image proves no killed wrong-path store data or stale suffix mask survived SQ reuse",
        ai_fubp_bytes_match(target_paddr, reuse_payload, vl)
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_execute_slot_then_us_vfof_address_owner_manual_corner()
{
    TEST_START();

    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_bp_lane = 6U;
    const unsigned fof_bp_lane = 5U;
    const unsigned fof_fault_lane = 11U;
    const uintptr_t exec_pc = (uintptr_t)ai_fubp_execute_marker_store_once;
    const uintptr_t us_vaddr = hs_page_base(us_page) + 0x700U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x700U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x740U;
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0x780U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x800U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x840U;
    const uintptr_t fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t us_paddr = phys_page_base(us_page) + 0x700U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x700U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x740U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0x780U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x800U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x840U;
    const uintptr_t fof_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(high_page);
    const uint64_t marker_seed = 0x65786563736c7430ULL;
    const uint64_t marker_us_payload = 0x6578656375736270ULL;
    const uint64_t marker_fof_payload = 0x65786563666f6670ULL;
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    uint64_t slot0_tdata1 = 0ULL;
    uint64_t slot0_tdata2 = 0ULL;
    uint64_t slot1_tdata1 = 0ULL;
    uint64_t slot1_tdata2 = 0ULL;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_fubp_fill_pattern(us_payload, vl, 0x32U);
    ai_fubp_fill_pattern(old_payload, vl, 0xb2U);
    ai_fubp_fill_pattern(fof_payload, vl, 0x52U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xd2U);
    for (unsigned i = 0; i < vl; ++i) {
        us_bp_expected[i] = (i < us_bp_lane) ? us_payload[i] :
            old_payload[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            fof_seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ?
            fof_payload[i] : fof_seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_clear_mcontrol6_slot(1U);
    ai_fubp_write_bytes(us_paddr, us_payload, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x62U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x72U, vl);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("execute/address overlap setup preloads old UnitStride destination bytes",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("slot0 UnitStride address breakpoint reports its own lane before any execute-slot handler probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    ai_fubp_arm_mcontrol6_slot(1U, exec_pc, AI_FUBP_MCONTROL6_EXECUTE);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_execute_marker_store_once(marker_vaddr, marker_us_payload);

    goto_priv(PRIV_M);
    ai_fubp_read_mcontrol6_slot(0U, &slot0_tdata1, &slot0_tdata2);
    ai_fubp_read_mcontrol6_slot(1U, &slot1_tdata1, &slot1_tdata2);
    TEST_ASSERT("handler-like execute trigger fires without clearing or rewriting the resident UnitStride data-address trigger slot",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(marker_paddr) == marker_seed &&
        (slot0_tdata1 & AI_FUBP_MCONTROL6_LOAD) != 0ULL &&
        (slot0_tdata1 & AI_FUBP_MCONTROL6_EXECUTE) == 0ULL &&
        slot0_tdata2 == us_vaddr + us_bp_lane &&
        (slot1_tdata1 & AI_FUBP_MCONTROL6_EXECUTE) != 0ULL &&
        slot1_tdata2 == exec_pc,
        "cause=0x%lx tval=0x%lx slot0_tdata1=0x%lx slot0_tdata2=0x%lx slot1_tdata1=0x%lx slot1_tdata2=0x%lx marker=0x%lx",
        excpt.cause, excpt.tval, slot0_tdata1, slot0_tdata2,
        slot1_tdata1, slot1_tdata2, read64(marker_paddr)
    );

    ai_fubp_clear_mcontrol6_slot(1U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("clearing only the execute slot lets the test reset UnitStride destination while slot0 remains armed",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("after execute-slot clear, the still-armed UnitStride address trigger reports the same data VA and vstart rather than the execute helper PC",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx exec_pc=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane, exec_pc,
        observed_vstart_csr
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("UnitStride prefix dump after execute-slot overlap completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride prefix/suffix image after execute-slot overlap keeps current prefix and old suffix",
        ai_fubp_bytes_match(us_dump_paddr, us_bp_expected, vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    TEST_ASSERT("clearing the UnitStride data slot lets the same vector load complete after the execute-slot probe",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("full UnitStride retry dump after execute/address overlap completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("full UnitStride retry after execute/address overlap observes the current payload exactly once",
        ai_fubp_bytes_match(us_dump_paddr, us_payload, vl)
    );

    ai_fubp_fill_bytes(fof_dump_paddr, 0x73U, vl);
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_seed_capture_no_dump(fof_seed_vaddr, fof_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("slot0 VFOF address breakpoint reports its own nonzero lane before the execute-slot handler probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    ai_fubp_arm_mcontrol6_slot(1U, exec_pc, AI_FUBP_MCONTROL6_EXECUTE);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_execute_marker_store_once(marker_vaddr, marker_fof_payload);

    goto_priv(PRIV_M);
    ai_fubp_read_mcontrol6_slot(0U, &slot0_tdata1, &slot0_tdata2);
    ai_fubp_read_mcontrol6_slot(1U, &slot1_tdata1, &slot1_tdata2);
    TEST_ASSERT("handler-like execute trigger fires without clearing or rewriting the resident VFOF data-address trigger slot",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(marker_paddr) == marker_seed &&
        (slot0_tdata1 & AI_FUBP_MCONTROL6_LOAD) != 0ULL &&
        (slot0_tdata1 & AI_FUBP_MCONTROL6_EXECUTE) == 0ULL &&
        slot0_tdata2 == fof_vaddr + fof_bp_lane &&
        (slot1_tdata1 & AI_FUBP_MCONTROL6_EXECUTE) != 0ULL &&
        slot1_tdata2 == exec_pc,
        "cause=0x%lx tval=0x%lx slot0_tdata1=0x%lx slot0_tdata2=0x%lx slot1_tdata1=0x%lx slot1_tdata2=0x%lx marker=0x%lx",
        excpt.cause, excpt.tval, slot0_tdata1, slot0_tdata2,
        slot1_tdata1, slot1_tdata2, read64(marker_paddr)
    );

    ai_fubp_clear_mcontrol6_slot(1U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_seed_capture_no_dump(fof_seed_vaddr, fof_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("after execute-slot clear, the still-armed VFOF address trigger reports the same data VA rather than the execute helper PC",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx exec_pc=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane, exec_pc,
        observed_vl, observed_vstart
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v8(fof_dump_vaddr, vl);
    TEST_ASSERT("VFOF prefix dump after execute-slot overlap completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF prefix/suffix image after execute-slot overlap keeps current prefix and seed suffix",
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x74U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing both slots exposes the later VFOF no-read lane as shortened-vl rather than a stale execute/data breakpoint",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x75U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("repairing the high page after execute/address overlap lets VFOF complete with the fresh full payload",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_clear_mcontrol6_slot(1U);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_handler_fault_between_us_vfof_bp_retry_manual_corner()
{
    TEST_START();

    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page fault_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_bp_lane = 7U;
    const unsigned fof_bp_lane = 4U;
    const unsigned fof_fault_lane = 10U;
    const uintptr_t us_vaddr = hs_page_base(us_page) + 0x900U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x900U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0x940U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0x980U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0x9c0U;
    const uintptr_t handler_fault_vaddr = hs_page_base(fault_page) + 0x380U;
    const uintptr_t fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t us_paddr = phys_page_base(us_page) + 0x900U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x900U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0x940U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0x980U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0x9c0U;
    const uintptr_t fof_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(fault_page);
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t fault_value = 0ULL;
    uint64_t saved_mstatus = 0ULL;
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_fubp_fill_pattern(us_payload, vl, 0x34U);
    ai_fubp_fill_pattern(old_payload, vl, 0xb4U);
    ai_fubp_fill_pattern(fof_payload, vl, 0x54U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xd4U);
    for (unsigned i = 0; i < vl; ++i) {
        us_bp_expected[i] = (i < us_bp_lane) ? us_payload[i] :
            old_payload[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            fof_seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ?
            fof_payload[i] : fof_seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_paddr, us_payload, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x64U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x74U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("nested-fault setup preloads old UnitStride destination bytes before the first breakpoint",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("first-level UnitStride breakpoint records the original data VA and vstart before handler-like nested fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fault_value = ld(handler_fault_vaddr);
    TEST_ASSERT("handler-like load page fault after UnitStride breakpoint reports its own VA and does not reuse vector tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == handler_fault_vaddr,
        "fault_value=0x%lx cause=0x%lx tval=0x%lx expected=0x%lx old_us=0x%lx",
        fault_value, excpt.cause, excpt.tval, handler_fault_vaddr,
        us_vaddr + us_bp_lane
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("after handler-like fault, UnitStride destination can be reset while original trigger remains armed",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("after handler-like fault, the still-armed UnitStride breakpoint keeps the original data VA and vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx fault_va=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        handler_fault_vaddr, observed_vstart_csr
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("UnitStride prefix dump after handler-like nested fault completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride prefix after handler-like nested fault remains the original breakpoint prefix",
        ai_fubp_bytes_match(us_dump_paddr, us_bp_expected, vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    TEST_ASSERT("clearing the UnitStride trigger after handler-like fault lets the original vector instruction retry",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("full UnitStride retry dump after handler-like fault completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("full UnitStride retry after handler-like fault observes the current payload exactly once",
        ai_fubp_bytes_match(us_dump_paddr, us_payload, vl)
    );

    ai_fubp_fill_bytes(fof_dump_paddr, 0x75U, vl);
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_seed_capture_no_dump(fof_seed_vaddr, fof_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("first-level VFOF breakpoint records the original data VA before handler-like nested fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fault_value = ld(handler_fault_vaddr);
    TEST_ASSERT("handler-like load page fault after VFOF breakpoint reports its own VA and does not overwrite VFOF metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == handler_fault_vaddr,
        "fault_value=0x%lx cause=0x%lx tval=0x%lx expected=0x%lx old_fof=0x%lx",
        fault_value, excpt.cause, excpt.tval, handler_fault_vaddr,
        fof_vaddr + fof_bp_lane
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_seed_capture_no_dump(fof_seed_vaddr, fof_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("after handler-like fault, the still-armed VFOF breakpoint keeps the original data VA rather than the fault VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx fault_va=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        handler_fault_vaddr, observed_vl, observed_vstart
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v8(fof_dump_vaddr, vl);
    TEST_ASSERT("VFOF prefix dump after handler-like nested fault completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF prefix after handler-like nested fault remains the original breakpoint prefix",
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl)
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x76U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("clearing VFOF trigger after handler-like fault exposes the later shortened-vl owner",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    goto_priv(PRIV_HS);
    ai_fubp_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x77U, vl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("repairing the handler-fault/VFOF high page lets the original VFOF retry finish with fresh payload",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_payload, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_same_va_sfence_remap_us_vfof_bp_owner_manual_corner()
{
    TEST_START();

    const enum test_page va_page = VSRWX_GRWX;
    const enum test_page pa_a_page = SWITCH1;
    const enum test_page pa_b_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const uint64_t asid_a = 0x51ULL;
    const uint64_t asid_b = 0x52ULL;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_bp_lane = 6U;
    const unsigned fof_bp_lane = 4U;
    const uintptr_t us_vaddr = hs_page_base(va_page) + 0xa00U;
    const uintptr_t fof_vaddr = hs_page_base(va_page) + 0xa40U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0xa00U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0xa40U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0xa80U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xac0U;
    const uintptr_t pa_a_base = phys_page_base(pa_a_page);
    const uintptr_t pa_b_base = phys_page_base(pa_b_page);
    const uintptr_t pa_a_us_paddr = pa_a_base + 0xa00U;
    const uintptr_t pa_b_us_paddr = pa_b_base + 0xa00U;
    const uintptr_t pa_a_fof_paddr = pa_a_base + 0xa40U;
    const uintptr_t pa_b_fof_paddr = pa_b_base + 0xa40U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0xa00U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0xa40U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0xa80U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xac0U;
    uint8_t pa_a_us[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pa_b_us[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pa_a_fof[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pa_b_fof[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pa_a_us_prefix[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pa_b_us_prefix[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pa_a_fof_prefix[AI_FUBP_FULL_VL] = { 0 };
    uint8_t pa_b_fof_prefix[AI_FUBP_FULL_VL] = { 0 };
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_fubp_fill_pattern(pa_a_us, vl, 0x36U);
    ai_fubp_fill_pattern(pa_b_us, vl, 0x76U);
    ai_fubp_fill_pattern(pa_a_fof, vl, 0x46U);
    ai_fubp_fill_pattern(pa_b_fof, vl, 0x86U);
    ai_fubp_fill_pattern(old_payload, vl, 0xb6U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xd6U);
    for (unsigned i = 0; i < vl; ++i) {
        pa_a_us_prefix[i] = (i < us_bp_lane) ? pa_a_us[i] :
            old_payload[i];
        pa_b_us_prefix[i] = (i < us_bp_lane) ? pa_b_us[i] :
            old_payload[i];
        pa_a_fof_prefix[i] = (i < fof_bp_lane) ? pa_a_fof[i] :
            fof_seed[i];
        pa_b_fof_prefix[i] = (i < fof_bp_lane) ? pa_b_fof[i] :
            fof_seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(pa_a_us_paddr, pa_a_us, vl);
    ai_fubp_write_bytes(pa_b_us_paddr, pa_b_us, vl);
    ai_fubp_write_bytes(pa_a_fof_paddr, pa_a_fof, vl);
    ai_fubp_write_bytes(pa_b_fof_paddr, pa_b_fof, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x66U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x67U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    *ai_fubp_hs_leaf_entry(va_page) =
        ai_fubp_leaf_pte(pa_a_base, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(pa_a_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(pa_b_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    ai_fubp_set_satp_asid(asid_a);
    CSRW(CSR_VSTART, 0ULL);

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("ASID-A same-VA setup preloads old UnitStride destination bytes",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("ASID-A UnitStride breakpoint uses the same GVA trigger and current PA_A payload prefix",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("ASID-A UnitStride breakpoint prefix dump completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("ASID-A UnitStride prefix comes from PA_A rather than any later remap image",
        ai_fubp_bytes_match(us_dump_paddr, pa_a_us_prefix, vl)
    );

    goto_priv(PRIV_HS);
    *ai_fubp_hs_leaf_entry(va_page) =
        ai_fubp_leaf_pte(pa_b_base, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_fubp_set_satp_asid(asid_b);
    CSRW(CSR_VSTART, 0ULL);

    ai_fubp_arm_mcontrol6_slot(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("ASID-B same-VA setup preloads old UnitStride destination bytes after remap",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("ASID-B UnitStride breakpoint keeps the same GVA tval but uses the new address-space generation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("ASID-B UnitStride breakpoint prefix dump completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("ASID-B UnitStride prefix comes from PA_B and not stale PA_A bytes even though tval is the same GVA",
        ai_fubp_bytes_match(us_dump_paddr, pa_b_us_prefix, vl)
    );

    goto_priv(PRIV_HS);
    *ai_fubp_hs_leaf_entry(va_page) =
        ai_fubp_leaf_pte(pa_a_base, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_fubp_set_satp_asid(asid_a);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x68U, vl);
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_seed_capture_no_dump(fof_seed_vaddr, fof_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("ASID-A VFOF breakpoint reports the same GVA lane under the PA_A generation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v8(fof_dump_vaddr, vl);
    TEST_ASSERT("ASID-A VFOF breakpoint prefix dump completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("ASID-A VFOF prefix comes from PA_A and keeps the seed suffix after breakpoint stop",
        ai_fubp_bytes_match(fof_dump_paddr, pa_a_fof_prefix, vl)
    );

    goto_priv(PRIV_HS);
    *ai_fubp_hs_leaf_entry(va_page) =
        ai_fubp_leaf_pte(pa_b_base, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_fubp_set_satp_asid(asid_b);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_M);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x69U, vl);
    ai_fubp_arm_mcontrol6_slot(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_seed_capture_no_dump(fof_seed_vaddr, fof_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("ASID-B VFOF breakpoint keeps the same GVA tval but refreshes translation generation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v8(fof_dump_vaddr, vl);
    TEST_ASSERT("ASID-B VFOF breakpoint prefix dump completes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("ASID-B VFOF prefix comes from PA_B and not stale PA_A bytes even though tval is the same GVA",
        ai_fubp_bytes_match(fof_dump_paddr, pa_b_fof_prefix, vl)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing triggers, ASID-B VFOF full retry observes the PA_B image at the same GVA",
        excpt.triggered == false &&
        observed_vl == vl &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, pa_b_fof, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_cleanup_hs();
    TEST_END();
}

bool ai_micro_debug_action_warl_us_vfof_no_stale_manual_corner()
{
    TEST_START();

    const enum test_page us_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const unsigned vl = AI_FUBP_FULL_VL;
    const unsigned us_bp_lane = 7U;
    const unsigned fof_bp_lane = 4U;
    const unsigned fof_fault_lane = 10U;
    const uintptr_t us_vaddr = hs_page_base(us_page) + 0xb00U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0xb00U;
    const uintptr_t us_dump_vaddr = hs_page_base(aux_page) + 0xb40U;
    const uintptr_t fof_seed_vaddr = hs_page_base(aux_page) + 0xb80U;
    const uintptr_t fof_dump_vaddr = hs_page_base(aux_page) + 0xbc0U;
    const uintptr_t fof_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t us_paddr = phys_page_base(us_page) + 0xb00U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0xb00U;
    const uintptr_t us_dump_paddr = phys_page_base(aux_page) + 0xb40U;
    const uintptr_t fof_seed_paddr = phys_page_base(aux_page) + 0xb80U;
    const uintptr_t fof_dump_paddr = phys_page_base(aux_page) + 0xbc0U;
    const uintptr_t fof_low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        fof_fault_lane;
    const uintptr_t fof_high_paddr = phys_page_base(high_page);
    uint8_t us_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t old_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t us_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_payload[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_seed[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_bp_expected[AI_FUBP_FULL_VL] = { 0 };
    uint8_t fof_fault_expected[AI_FUBP_FULL_VL] = { 0 };
    uint64_t action_readback = 0ULL;
    uint64_t observed_vstart_csr = 0ULL;
    uint64_t saved_mstatus = 0ULL;
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_fubp_fill_pattern(us_payload, vl, 0x38U);
    ai_fubp_fill_pattern(old_payload, vl, 0xb8U);
    ai_fubp_fill_pattern(fof_payload, vl, 0x48U);
    ai_fubp_fill_pattern(fof_seed, vl, 0xd8U);
    for (unsigned i = 0; i < vl; ++i) {
        us_bp_expected[i] = (i < us_bp_lane) ? us_payload[i] :
            old_payload[i];
        fof_bp_expected[i] = (i < fof_bp_lane) ? fof_payload[i] :
            fof_seed[i];
        fof_fault_expected[i] = (i < fof_fault_lane) ?
            fof_payload[i] : fof_seed[i];
    }

    ai_fubp_enable_vector_state();
    goto_priv(PRIV_M);
    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_write_bytes(us_paddr, us_payload, vl);
    ai_fubp_write_bytes(old_paddr, old_payload, vl);
    ai_fubp_write_bytes(fof_low_paddr, fof_payload, fof_fault_lane);
    ai_fubp_write_bytes(fof_high_paddr, fof_payload + fof_fault_lane,
        vl - fof_fault_lane);
    ai_fubp_write_bytes(fof_seed_paddr, fof_seed, vl);
    ai_fubp_fill_bytes(us_dump_paddr, 0x68U, vl);
    ai_fubp_fill_bytes(fof_dump_paddr, 0x78U, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_fubp_set_hs_leaf(us_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_fubp_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_fubp_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(old_vaddr, vl);
    TEST_ASSERT("debug-action WARL setup preloads old UnitStride bytes",
        excpt.triggered == false
    );

    saved_mstatus = ai_fubp_enable_breakpoint_delivery();
    ai_fubp_arm_mcontrol6_slot_action1_attempt(0U, us_vaddr + us_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    ai_fubp_read_mcontrol6_slot(0U, &action_readback, NULL);
    TEST_ASSERT("outside Debug Mode, action=1 write for the UnitStride address trigger normalizes to breakpoint action=0",
        ai_fubp_mcontrol6_action(action_readback) == 0ULL &&
        (action_readback & AI_FUBP_MCONTROL6_LOAD) != 0ULL,
        "tdata1=0x%lx action=%lu",
        action_readback, ai_fubp_mcontrol6_action(action_readback)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8_to_v6(us_vaddr, vl);
    observed_vstart_csr = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride action=1 attempt still reports a normal breakpoint with current data VA and lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == us_vaddr + us_bp_lane &&
        observed_vstart_csr == us_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, us_vaddr + us_bp_lane,
        observed_vstart_csr
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v6(us_dump_vaddr, vl);
    TEST_ASSERT("UnitStride action-WARL breakpoint prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("UnitStride action-WARL prefix contains current bytes before the trigger and old bytes after it",
        ai_fubp_bytes_match(us_dump_paddr, us_bp_expected, vl)
    );

    ai_fubp_arm_mcontrol6_slot_action1_attempt(0U, fof_vaddr + fof_bp_lane,
        AI_FUBP_MCONTROL6_LOAD);
    ai_fubp_read_mcontrol6_slot(0U, &action_readback, NULL);
    TEST_ASSERT("outside Debug Mode, action=1 write for the VFOF address trigger also normalizes to breakpoint action=0",
        ai_fubp_mcontrol6_action(action_readback) == 0ULL &&
        (action_readback & AI_FUBP_MCONTROL6_LOAD) != 0ULL,
        "tdata1=0x%lx action=%lu",
        action_readback, ai_fubp_mcontrol6_action(action_readback)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_seed_capture_no_dump(fof_seed_vaddr, fof_vaddr, vl,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF action=1 attempt is not filtered into silent shortened-vl and reports the current breakpoint lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == fof_vaddr + fof_bp_lane,
        "cause=0x%lx tval=0x%lx expected=0x%lx vl=%u vstart=%u",
        excpt.cause, excpt.tval, fof_vaddr + fof_bp_lane,
        observed_vl, observed_vstart
    );

    ai_fubp_clear_mcontrol6_slot(0U);
    ai_fubp_restore_breakpoint_delivery(saved_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vse8_from_v8(fof_dump_vaddr, vl);
    TEST_ASSERT("VFOF action-WARL breakpoint prefix dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF action-WARL prefix contains current bytes before the trigger and seed bytes after it",
        ai_fubp_bytes_match(fof_dump_paddr, fof_bp_expected, vl)
    );

    ai_fubp_fill_bytes(fof_dump_paddr, 0x79U, vl);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_fubp_vle8ff_full_dump(fof_seed_vaddr, fof_vaddr,
        fof_dump_vaddr, vl, &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the action-WARL trigger is cleared, the later VFOF no-read lane is a silent shortened-vl event with fresh owner",
        excpt.triggered == false &&
        observed_vl == fof_fault_lane &&
        observed_vstart == 0U &&
        ai_fubp_bytes_match(fof_dump_paddr, fof_fault_expected, vl),
        "triggered=%d cause=0x%lx tval=0x%lx vl=%u vstart=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vl,
        observed_vstart
    );

    ai_fubp_cleanup_hs();
    TEST_END();
}
