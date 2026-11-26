#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MEMBLOCK_PTE_FAULT_BAD_LVL_MASK (0x3fULL << 54)
#define AI_MEMBLOCK_PTE_FAULT_PMP_TOR_RWX 0x0fULL
#define AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_R 0x0cULL
#define AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_W 0x0dULL
#define AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_X 0x0bULL

#define AI_MEMBLOCK_ASSERT_PAGE_FAULT(message, expected_cause, vaddr) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == (expected_cause) && \
        excpt.tval == (vaddr))

#define AI_MEMBLOCK_ASSERT_NO_FAULT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline pte_t *ai_memblock_pte_fault_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_pte_fault_prepare_hs(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    sfence_vma();
}

static inline void ai_memblock_pte_fault_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}

static inline bool ai_memblock_pte_fault_check_leaf_unchanged(
    enum test_page page, uint64_t expected)
{
    return *ai_memblock_pte_fault_hs_leaf_entry(page) == expected;
}

static inline uintptr_t ai_memblock_pte_fault_ptw_pmp_page(void)
{
#ifdef sv48
    return ((uintptr_t)&hspt[2][0]) & PAGE_ADDR_MSK;
#else
    return ((uintptr_t)&hspt[1][0]) & PAGE_ADDR_MSK;
#endif
}

static inline void ai_memblock_pte_fault_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MEMBLOCK_PTE_FAULT_PMP_TOR_RWX);
}

static inline void ai_memblock_pte_fault_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MEMBLOCK_PTE_FAULT_PMP_TOR_RWX |
        (deny_cfg << 8) | (AI_MEMBLOCK_PTE_FAULT_PMP_TOR_RWX << 16));
}

static inline uintptr_t ai_memblock_pte_fault_cross_page_addr(
    enum test_page low_page)
{
    return hs_page_base(low_page) + PAGE_SIZE - sizeof(uint32_t);
}

static inline uintptr_t ai_memblock_pte_fault_page_end_phys(
    enum test_page page)
{
    return phys_page_base(page) + PAGE_SIZE - sizeof(uint64_t);
}

static inline void ai_memblock_pte_fault_write_bytes(uintptr_t addr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_memblock_pte_fault_bytes_match(uintptr_t addr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

#define AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(name, instruction) \
static inline uint64_t name(uintptr_t addr, uint64_t sentinel) \
{ \
    register uint64_t value asm("a4") = sentinel; \
    asm volatile( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        instruction " %0, 0(%1)\n\t" \
        ".option pop\n\t" \
        : "+r"(value) : "r"(addr) : "memory"); \
    return value; \
}

AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lb_rd_probe, "lb")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lbu_rd_probe, "lbu")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lh_rd_probe, "lh")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lhu_rd_probe, "lhu")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lw_rd_probe, "lw")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lwu_rd_probe, "lwu")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_ld_rd_probe, "ld")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lr_w_rd_probe, "lr.w")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lr_d_rd_probe, "lr.d")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lr_d_aq_rd_probe, "lr.d.aq")
AI_MEMBLOCK_PTE_FAULT_LOAD_RD_PROBE(
    ai_memblock_pte_fault_lr_d_aqrl_rd_probe, "lr.d.aqrl")

static inline uint64_t ai_memblock_pte_fault_read_fflags(void)
{
    return CSRR(CSR_FFLAGS) & 0x1fULL;
}

static inline void ai_memblock_pte_fault_write_fflags(uint64_t flags)
{
    CSRW(CSR_FFLAGS, flags & 0x1fULL);
}

static inline uint64_t ai_memblock_pte_fault_read_fcsr(void)
{
    return CSRR(CSR_FCSR) & 0xffULL;
}

static inline void ai_memblock_pte_fault_write_fcsr(uint64_t flags,
    uint64_t frm)
{
    CSRW(CSR_FCSR, ((frm & 0x7ULL) << 5) | (flags & 0x1fULL));
}

static inline void ai_memblock_pte_fault_seed_non_target_fpr(
    uint64_t sentinel)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft5, %[sentinel]\n\t"
        ".option pop\n\t"
        : : [sentinel] "r"(sentinel) : "ft5");
}

static inline uint64_t ai_memblock_pte_fault_read_non_target_fpr(void)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.x.d %[value], ft5\n\t"
        ".option pop\n\t"
        : [value] "=r"(value) : : "memory");
    return value;
}

static inline uint64_t ai_memblock_pte_fault_flw_frd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.w.x ft2, %[sentinel]\n\t"
        "flw ft2, 0(%[addr])\n\t"
        "fmv.x.w %[value], ft2\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "ft2", "memory");
    return value;
}

static inline uint64_t ai_memblock_pte_fault_fld_frd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft2, %[sentinel]\n\t"
        "fld ft2, 0(%[addr])\n\t"
        "fmv.x.d %[value], ft2\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "ft2", "memory");
    return value;
}

static inline void ai_memblock_pte_fault_fsw_probe(uintptr_t addr,
    uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.w.x ft3, %[bits]\n\t"
        "fsw ft3, 0(%[addr])\n\t"
        ".option pop\n\t"
        :
        : [addr] "r"(addr), [bits] "r"(bits)
        : "ft3", "memory");
}

static inline void ai_memblock_pte_fault_fsd_probe(uintptr_t addr,
    uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft3, %[bits]\n\t"
        "fsd ft3, 0(%[addr])\n\t"
        ".option pop\n\t"
        :
        : [addr] "r"(addr), [bits] "r"(bits)
        : "ft3", "memory");
}

static inline void ai_memblock_pte_fault_seed_ft2(uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft2, %[bits]\n\t"
        ".option pop\n\t"
        : : [bits] "r"(bits) : "ft2");
}

static inline uint64_t ai_memblock_pte_fault_read_ft2(void)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.x.d %[value], ft2\n\t"
        ".option pop\n\t"
        : [value] "=r"(value) : : "memory");

    return value;
}

static inline void ai_memblock_pte_fault_seed_ft3(uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft3, %[bits]\n\t"
        ".option pop\n\t"
        : : [bits] "r"(bits) : "ft3");
}

static inline void ai_memblock_pte_fault_flw_only(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "flw ft2, 0(%[addr])\n\t"
        ".option pop\n\t"
        : : [addr] "r"(addr) : "ft2", "memory");
}

static inline void ai_memblock_pte_fault_fld_only(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fld ft2, 0(%[addr])\n\t"
        ".option pop\n\t"
        : : [addr] "r"(addr) : "ft2", "memory");
}

static inline void ai_memblock_pte_fault_fsw_only(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fsw ft3, 0(%[addr])\n\t"
        ".option pop\n\t"
        : : [addr] "r"(addr) : "memory");
}

static inline void ai_memblock_pte_fault_fsd_only(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fsd ft3, 0(%[addr])\n\t"
        ".option pop\n\t"
        : : [addr] "r"(addr) : "memory");
}

static inline void ai_memblock_pte_fault_set_fs_clean(void)
{
    CSRC(CSR_SSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_SSTATUS, SSTATUS_FS_CLEAN);
}

static inline void ai_memblock_pte_fault_set_fs_dirty(void)
{
    CSRC(CSR_SSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
}

static inline uint64_t ai_memblock_pte_fault_fs_state(void)
{
    return CSRR(CSR_SSTATUS) & SSTATUS_FS_MSK;
}

static inline void ai_memblock_pte_fault_wrong_path_fp_mem_probe(
    uintptr_t load_addr, uintptr_t store_addr, uintptr_t marker_addr,
    uint64_t taken_marker, uint64_t poison_marker)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 1\n\t"
        "bnez t0, 1f\n\t"
        "flw ft2, 0(%[load_addr])\n\t"
        "fsd ft3, 0(%[store_addr])\n\t"
        "sd %[poison_marker], 0(%[marker_addr])\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "sd %[taken_marker], 0(%[marker_addr])\n\t"
        "2:\n\t"
        ".option pop\n\t"
        :
        : [load_addr] "r"(load_addr),
          [store_addr] "r"(store_addr),
          [marker_addr] "r"(marker_addr),
          [taken_marker] "r"(taken_marker),
          [poison_marker] "r"(poison_marker)
        : "t0", "ft2", "ft3", "memory");
}

static inline uint64_t ai_memblock_pte_fault_cross_fld_frd_probe(
    uintptr_t addr, uint64_t sentinel)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft4, %[sentinel]\n\t"
        "fld ft4, 0(%[addr])\n\t"
        "fmv.x.d %[value], ft4\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "ft4", "memory");
    return value;
}

#define AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(message, probe, addr, sentinel) \
    do { \
        TEST_SETUP_EXCEPT(); \
        uint64_t rd_after_fault = (probe)((addr), (sentinel)); \
        TEST_ASSERT((message), \
            excpt.triggered == true && \
            excpt.cause == CAUSE_LPF && \
            excpt.tval == (addr) && \
            rd_after_fault == (sentinel)); \
    } while (0)

#define AI_MEMBLOCK_CHECK_FAULTING_FP_LOAD_FRD(message, probe, addr, \
    sentinel, expected_fflags) \
    do { \
        TEST_SETUP_EXCEPT(); \
        uint64_t frd_after_fault = (probe)((addr), (sentinel)); \
        TEST_ASSERT((message), \
            excpt.triggered == true && \
            excpt.cause == CAUSE_LPF && \
            excpt.tval == (addr) && \
            frd_after_fault == (sentinel) && \
            ai_memblock_pte_fault_read_fflags() == (expected_fflags)); \
    } while (0)

#define AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(name, instruction) \
static inline uint64_t name(uintptr_t addr, uint64_t sentinel, \
    uint64_t operand) \
{ \
    register uint64_t value asm("a4") = sentinel; \
    register uint64_t amo_operand asm("a5") = operand; \
    asm volatile( \
        ".option push\n\t" \
        ".option norvc\n\t" \
        instruction " %0, %2, 0(%1)\n\t" \
        ".option pop\n\t" \
        : "+r"(value) : "r"(addr), "r"(amo_operand) : "memory"); \
    return value; \
}

AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(
    ai_memblock_pte_fault_amoswap_w_rd_probe, "amoswap.w")
AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(
    ai_memblock_pte_fault_amoadd_w_rd_probe, "amoadd.w")
AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(
    ai_memblock_pte_fault_amoswap_d_rd_probe, "amoswap.d")
AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(
    ai_memblock_pte_fault_amoadd_d_rd_probe, "amoadd.d")
AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(
    ai_memblock_pte_fault_amomaxu_d_rd_probe, "amomaxu.d")
AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(
    ai_memblock_pte_fault_sc_w_rd_probe, "sc.w")
AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(
    ai_memblock_pte_fault_sc_d_rd_probe, "sc.d")
AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(
    ai_memblock_pte_fault_sc_d_rl_rd_probe, "sc.d.rl")
AI_MEMBLOCK_PTE_FAULT_AMO_RD_PROBE(
    ai_memblock_pte_fault_sc_d_aqrl_rd_probe, "sc.d.aqrl")

#define AI_MEMBLOCK_CHECK_FAULTING_AMO_RD(message, probe, addr, sentinel, operand) \
    do { \
        TEST_SETUP_EXCEPT(); \
        uint64_t rd_after_fault = (probe)((addr), (sentinel), (operand)); \
        TEST_ASSERT((message), \
            excpt.triggered == true && \
            excpt.cause == CAUSE_SPF && \
            excpt.tval == (addr) && \
            rd_after_fault == (sentinel)); \
    } while (0)

#define AI_MEMBLOCK_CHECK_FAULTING_SC_RD(message, probe, addr, sentinel, operand) \
    do { \
        TEST_SETUP_EXCEPT(); \
        uint64_t rd_after_fault = (probe)((addr), (sentinel), (operand)); \
        TEST_ASSERT((message), \
            excpt.triggered == true && \
            excpt.cause == CAUSE_SPF && \
            excpt.tval == (addr) && \
            rd_after_fault == (sentinel)); \
    } while (0)

typedef struct {
    uint64_t old_value;
    uint64_t status;
} ai_memblock_pte_fault_lrsc_result_t;

static inline ai_memblock_pte_fault_lrsc_result_t
ai_memblock_pte_fault_lr_sc_w_pair(uintptr_t addr, uint64_t payload)
{
    ai_memblock_pte_fault_lrsc_result_t result;
    uint64_t old_value;
    uint64_t status;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.w %0, 0(%3)\n\t"
        "sc.w %1, %2, 0(%3)\n\t"
        ".option pop\n\t"
        : "=&r"(old_value), "=&r"(status)
        : "r"(payload), "r"(addr)
        : "memory");
    result.old_value = old_value;
    result.status = status;
    return result;
}

static inline ai_memblock_pte_fault_lrsc_result_t
ai_memblock_pte_fault_lr_sc_d_pair(uintptr_t addr, uint64_t payload)
{
    ai_memblock_pte_fault_lrsc_result_t result;
    uint64_t old_value;
    uint64_t status;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %0, 0(%3)\n\t"
        "sc.d %1, %2, 0(%3)\n\t"
        ".option pop\n\t"
        : "=&r"(old_value), "=&r"(status)
        : "r"(payload), "r"(addr)
        : "memory");
    result.old_value = old_value;
    result.status = status;
    return result;
}

static inline ai_memblock_pte_fault_lrsc_result_t
ai_memblock_pte_fault_lr_aq_sc_rl_d_pair(uintptr_t addr, uint64_t payload)
{
    ai_memblock_pte_fault_lrsc_result_t result;
    uint64_t old_value;
    uint64_t status;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d.aq %0, 0(%3)\n\t"
        "sc.d.rl %1, %2, 0(%3)\n\t"
        ".option pop\n\t"
        : "=&r"(old_value), "=&r"(status)
        : "r"(payload), "r"(addr)
        : "memory");
    result.old_value = old_value;
    result.status = status;
    return result;
}

bool ai_micro_memblock_pte_perm_matrix_faults_corner()
{
    TEST_START();

    const enum test_page load_no_read_page = VSX_GX;
    const enum test_page store_no_write_page = VSR_GR;
    const enum test_page amo_no_write_page = VSR_GRX;
    const enum test_page fetch_no_exec_page = VSR_GR;
    const enum test_page user_read_only_page = VSR_GUR;
    const uintptr_t load_vaddr = hs_page_base(load_no_read_page) + 0x108U;
    const uintptr_t load_paddr = phys_page_base(load_no_read_page) + 0x108U;
    const uintptr_t load_adjacent_paddr = load_paddr + sizeof(uint64_t);
    const uintptr_t store_vaddr = hs_page_base(store_no_write_page) + 0x140U;
    const uintptr_t store_paddr = phys_page_base(store_no_write_page) + 0x140U;
    const uintptr_t store_adjacent_paddr = store_paddr + sizeof(uint64_t);
    const uintptr_t amo_vaddr = hs_page_base(amo_no_write_page) + 0x180U;
    const uintptr_t amo_paddr = phys_page_base(amo_no_write_page) + 0x180U;
    const uintptr_t amo_adjacent_paddr = amo_paddr + sizeof(uint64_t);
    const uintptr_t fetch_vaddr = hs_page_base(fetch_no_exec_page) + 0x1c0U;
    const uintptr_t fetch_paddr = phys_page_base(fetch_no_exec_page) + 0x1c0U;
    const uintptr_t user_ro_vaddr = hs_page_base(user_read_only_page) + 0x208U;
    const uintptr_t user_ro_paddr = phys_page_base(user_read_only_page) + 0x208U;
    const uintptr_t user_ro_adjacent_paddr =
        user_ro_paddr + sizeof(uint64_t);
    const uint64_t load_before = 0x657865632d6f6e6cULL;
    const uint64_t load_adjacent_before = 0x1011121314151617ULL;
    const uint64_t store_before = 0x726f772d64656e79ULL;
    const uint64_t store_adjacent_before = 0x2021222324252627ULL;
    const uint64_t amo_before = 0x61746f6d2d64656eULL;
    const uint64_t amo_adjacent_before = 0x3031323334353637ULL;
    const uint64_t user_ro_before = 0x75726f2d72656164ULL;
    const uint64_t user_ro_adjacent_before = 0x4041424344454647ULL;
    const uint64_t xonly_store_attempt = 0x77726974652d6d7872ULL;
    const uint64_t xonly_amo_sentinel = 0xa11a000000000154ULL;
    const uint64_t user_ro_store_attempt = 0x75726f2d77726974ULL;
    const uint64_t user_ro_amo_sentinel = 0xa11a0000000010adULL;
    const uint64_t store_ro_load_sentinel = 0xa11a00000000b055ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_ECU);
    write64(load_paddr, load_before);
    write64(load_adjacent_paddr, load_adjacent_before);
    write64(store_paddr, store_before);
    write64(store_adjacent_paddr, store_adjacent_before);
    write64(amo_paddr, amo_before);
    write64(amo_adjacent_paddr, amo_adjacent_before);
    write32(fetch_paddr, 0x00008067U);
    write64(user_ro_paddr, user_ro_before);
    write64(user_ro_adjacent_paddr, user_ro_adjacent_before);
    fence_iorw_iorw();
    ai_memblock_pte_fault_fence_i();

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    const uint64_t load_leaf = *ai_memblock_pte_fault_hs_leaf_entry(load_no_read_page);
    const uint64_t store_leaf = *ai_memblock_pte_fault_hs_leaf_entry(store_no_write_page);
    const uint64_t amo_leaf = *ai_memblock_pte_fault_hs_leaf_entry(amo_no_write_page);
    const uint64_t fetch_leaf = *ai_memblock_pte_fault_hs_leaf_entry(fetch_no_exec_page);
    pte_t *user_ro_leaf_entry =
        ai_memblock_pte_fault_hs_leaf_entry(user_read_only_page);
    const uint64_t user_ro_leaf =
        (*user_ro_leaf_entry & ~PTE_FLAGS_MSK) |
        PTE_V | PTE_U | PTE_A | PTE_D | PTE_R;
    *user_ro_leaf_entry = user_ro_leaf;
    sfence_vma();
    TEST_ASSERT("the permission matrix uses explicit V=1 leaves with the denied access bit cleared before probing",
        (load_leaf & (PTE_V | PTE_A | PTE_D | PTE_X)) ==
            (PTE_V | PTE_A | PTE_D | PTE_X) &&
        (load_leaf & PTE_R) == 0 &&
        (store_leaf & (PTE_V | PTE_A | PTE_D | PTE_R)) ==
            (PTE_V | PTE_A | PTE_D | PTE_R) &&
        (store_leaf & PTE_W) == 0 &&
        (amo_leaf & (PTE_V | PTE_A | PTE_D | PTE_R)) ==
            (PTE_V | PTE_A | PTE_D | PTE_R) &&
        (amo_leaf & PTE_W) == 0 &&
        (fetch_leaf & (PTE_V | PTE_A | PTE_D | PTE_R)) ==
            (PTE_V | PTE_A | PTE_D | PTE_R) &&
        (fetch_leaf & PTE_X) == 0 &&
        (user_ro_leaf & (PTE_V | PTE_U | PTE_A | PTE_D | PTE_R)) ==
            (PTE_V | PTE_U | PTE_A | PTE_D | PTE_R) &&
        (user_ro_leaf & (PTE_W | PTE_X)) == 0
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    ld(load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS load from a V=1/X=1/R=0 leaf with MXR=0 reports load page fault and the original VA",
        CAUSE_LPF, load_vaddr);

    CSRS(CSR_SSTATUS, SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    uint64_t mxr_load_value = ld(load_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after setting MXR=1 without changing the PTE or executing SFENCE.VMA, the same X-only leaf permits a load",
        mxr_load_value == load_before);

    CSRC(CSR_SSTATUS, SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    ld(load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("after a successful MXR=1 load, clearing MXR without changing the PTE or executing SFENCE.VMA makes the same X-only leaf fault again",
        CAUSE_LPF, load_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t repeated_mxr_rd_after_fault =
        ai_memblock_pte_fault_ld_rd_probe(load_vaddr,
            store_ro_load_sentinel);
    TEST_ASSERT("a repeated MXR=0 load fault on the same X-only leaf keeps LPF/tval stable and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        repeated_mxr_rd_after_fault == store_ro_load_sentinel
    );

    CSRS(CSR_SSTATUS, SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    mxr_load_value = ld(load_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("setting MXR=1 again lets the same X-only leaf load succeed without rebuilding the mapping",
        mxr_load_value == load_before);

    TEST_SETUP_EXCEPT();
    sd(load_vaddr, xonly_store_attempt);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("MXR=1 does not relax store permission on the same V=1/X=1/R=0/W=0 leaf",
        CAUSE_SPF, load_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t xonly_amo_after_fault = ai_memblock_pte_fault_amoadd_d_rd_probe(
        load_vaddr, xonly_amo_sentinel, 0x33ULL);
    TEST_ASSERT("MXR=1 does not relax AMO write permission on the same X-only leaf and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == load_vaddr &&
        xonly_amo_after_fault == xonly_amo_sentinel
    );

    CSRC(CSR_SSTATUS, SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    uint64_t hs_store_ro_load = ld(store_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS load from the U=0/R=1/W=0/X=0 supervisor leaf succeeds before store-denial probes",
        hs_store_ro_load == store_before);

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, 0x1111222233334444ULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS store from a V=1/R=1/W=0 leaf reports store page fault and the original VA",
        CAUSE_SPF, store_vaddr);

    TEST_SETUP_EXCEPT();
    amoadd_d(amo_vaddr, 0x55ULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS AMO from a V=1/R=1/W=0 leaf reports store page fault and the original VA",
        CAUSE_SPF, amo_vaddr);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(fetch_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS fetch from a V=1/R=1/X=0 leaf reports instruction page fault and the original VA",
        CAUSE_IPF, fetch_vaddr);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t m_store_ro_direct_load = read64(store_paddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("M-mode direct load of the same U=0/R=1/W=0/X=0 physical word is not constrained by the supervisor leaf permission",
        m_store_ro_direct_load == store_before);

    hspt_u_mode_allow_no_leaf();
    *user_ro_leaf_entry = user_ro_leaf;
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t user_ro_load = ld(user_ro_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HU load from a U=1/R=1/W=0/X=0 leaf succeeds before write-denial probes",
        user_ro_load == user_ro_before);

    TEST_SETUP_EXCEPT();
    sd(user_ro_vaddr, user_ro_store_attempt);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HU store to the same U=1/R=1/W=0/X=0 leaf reports store page fault and the original VA",
        CAUSE_SPF, user_ro_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t user_ro_amo_after_fault = ai_memblock_pte_fault_amoadd_d_rd_probe(
        user_ro_vaddr, user_ro_amo_sentinel, 0x44ULL);
    TEST_ASSERT("HU AMO to the U=1/R=1/W=0/X=0 leaf reports store page fault, leaves rd at its sentinel, and cannot perform a partial RMW",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == user_ro_vaddr &&
        user_ro_amo_after_fault == user_ro_amo_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t hu_store_ro_load_after_fault =
        ai_memblock_pte_fault_ld_rd_probe(store_vaddr,
            store_ro_load_sentinel);
    TEST_ASSERT("HU load to the U=0/R=1/W=0/X=0 supervisor leaf reports load page fault and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == store_vaddr &&
        hu_store_ro_load_after_fault == store_ro_load_sentinel
    );

    goto_priv_u_to_m_direct(PRIV_M);
    TEST_ASSERT("permission-denied load/store/fetch/AMO faults leave PTE words, target words, and adjacent words unchanged",
        ai_memblock_pte_fault_check_leaf_unchanged(load_no_read_page, load_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(store_no_write_page, store_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(amo_no_write_page, amo_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(fetch_no_exec_page, fetch_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(user_read_only_page,
            user_ro_leaf) &&
        read64(load_paddr) == load_before &&
        read64(load_adjacent_paddr) == load_adjacent_before &&
        read64(store_paddr) == store_before &&
        read64(store_adjacent_paddr) == store_adjacent_before &&
        read64(amo_paddr) == amo_before &&
        read64(amo_adjacent_paddr) == amo_adjacent_before &&
        read64(user_ro_paddr) == user_ro_before &&
        read64(user_ro_adjacent_paddr) == user_ro_adjacent_before
    );

    TEST_END();
}

bool ai_micro_memblock_pte_invalid_leaf_reserved_faults_corner()
{
    TEST_START();

    const enum test_page invalid_page = VSI_GI;
    const enum test_page illegal_leaf_page = VSW_GI;
    const enum test_page reserved_page = VSR_GRWX;
    const enum test_page illegal_user_leaf_page = VSR_GR;
    const uintptr_t invalid_load_vaddr = hs_page_base(invalid_page) + 0x88U;
    const uintptr_t invalid_fetch_vaddr = hs_page_base(invalid_page) + 0x8cU;
    const uintptr_t invalid_store_vaddr = hs_page_base(invalid_page) + 0x90U;
    const uintptr_t illegal_load_vaddr = hs_page_base(illegal_leaf_page) + 0x120U;
    const uintptr_t illegal_store_vaddr = hs_page_base(illegal_leaf_page) + 0x128U;
    const uintptr_t illegal_user_load_vaddr =
        hs_page_base(illegal_user_leaf_page) + 0x150U;
    const uintptr_t illegal_user_store_vaddr =
        hs_page_base(illegal_user_leaf_page) + 0x158U;
    const uintptr_t reserved_load_vaddr = hs_page_base(reserved_page) + 0x1a0U;
    const uintptr_t reserved_store_vaddr = hs_page_base(reserved_page) + 0x1a8U;
    const uintptr_t reserved_lr_vaddr = hs_page_base(reserved_page) + 0x1b0U;
    const uintptr_t reserved_amo_vaddr = hs_page_base(reserved_page) + 0x1b8U;

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    const uint64_t invalid_leaf = *ai_memblock_pte_fault_hs_leaf_entry(invalid_page);
    const uint64_t illegal_leaf = *ai_memblock_pte_fault_hs_leaf_entry(illegal_leaf_page);
    pte_t *illegal_user_leaf_entry =
        ai_memblock_pte_fault_hs_leaf_entry(illegal_user_leaf_page);
    const uint64_t illegal_user_leaf =
        (*illegal_user_leaf_entry & ~PTE_FLAGS_MSK) |
        PTE_V | PTE_U | PTE_A | PTE_D | PTE_W;
    *illegal_user_leaf_entry = illegal_user_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(reserved_page) |=
        AI_MEMBLOCK_PTE_FAULT_BAD_LVL_MASK;
    const uint64_t reserved_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(reserved_page);
    sfence_vma();
    TEST_ASSERT("invalid, illegal R=0/W=1, and reserved-bit PTE encodings are installed before the page-fault probes",
        (invalid_leaf & PTE_V) == 0 &&
        (illegal_leaf & (PTE_V | PTE_W)) == (PTE_V | PTE_W) &&
        (illegal_leaf & PTE_R) == 0 &&
        (illegal_user_leaf & (PTE_V | PTE_U | PTE_A | PTE_D | PTE_W)) ==
            (PTE_V | PTE_U | PTE_A | PTE_D | PTE_W) &&
        (illegal_user_leaf & PTE_R) == 0 &&
        (reserved_leaf & AI_MEMBLOCK_PTE_FAULT_BAD_LVL_MASK) != 0
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(invalid_load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS load to a PTE.V=0 leaf reports load page fault and the original VA",
        CAUSE_LPF, invalid_load_vaddr);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(invalid_fetch_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS fetch to a PTE.V=0 leaf reports instruction page fault and the original VA",
        CAUSE_IPF, invalid_fetch_vaddr);

    TEST_SETUP_EXCEPT();
    sd(invalid_store_vaddr, 0xaabbccddeeff0011ULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS store to a PTE.V=0 leaf reports store page fault and the original VA",
        CAUSE_SPF, invalid_store_vaddr);

    TEST_SETUP_EXCEPT();
    ld(illegal_load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS load to the reserved R=0/W=1 leaf encoding reports load page fault and the original VA",
        CAUSE_LPF, illegal_load_vaddr);

    TEST_SETUP_EXCEPT();
    sd(illegal_store_vaddr, 0xa5a55a5a11223344ULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS store to the reserved R=0/W=1 leaf encoding reports store page fault and the original VA",
        CAUSE_SPF, illegal_store_vaddr);

    CSRS(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    ld(illegal_user_load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS load with SUM=1/MXR=1 to a PTE.U=1 R=0/W=1 illegal leaf still reports load page fault",
        CAUSE_LPF, illegal_user_load_vaddr);

    TEST_SETUP_EXCEPT();
    sd(illegal_user_store_vaddr, 0x0badc0de12345678ULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS store with SUM=1/MXR=1 to a PTE.U=1 R=0/W=1 illegal leaf still reports store page fault",
        CAUSE_SPF, illegal_user_store_vaddr);

    goto_priv(PRIV_M);
    *illegal_user_leaf_entry = illegal_user_leaf & ~PTE_U;
    sfence_vma();
    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    ld(illegal_user_load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS load with SUM=1/MXR=1 to a PTE.U=0 R=0/W=1 illegal leaf still reports load page fault",
        CAUSE_LPF, illegal_user_load_vaddr);

    TEST_SETUP_EXCEPT();
    ld(reserved_load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS load to a leaf with reserved PTE bits set reports load page fault and the original VA",
        CAUSE_LPF, reserved_load_vaddr);

    TEST_SETUP_EXCEPT();
    sd(reserved_store_vaddr, 0x5566778899aabbccULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS store to a leaf with reserved PTE bits set reports store page fault and the original VA",
        CAUSE_SPF, reserved_store_vaddr);

    TEST_SETUP_EXCEPT();
    lr_d(reserved_lr_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS LR.D to a leaf with reserved PTE bits set reports load page fault and the original VA",
        CAUSE_LPF, reserved_lr_vaddr);

    TEST_SETUP_EXCEPT();
    amoadd_d(reserved_amo_vaddr, 0x99ULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS AMO to a leaf with reserved PTE bits set reports store page fault and the original VA",
        CAUSE_SPF, reserved_amo_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("invalid, illegal-leaf, and reserved-bit faults do not rewrite the PTE words under test",
        ai_memblock_pte_fault_check_leaf_unchanged(invalid_page, invalid_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(illegal_leaf_page, illegal_leaf) &&
        *illegal_user_leaf_entry == (illegal_user_leaf & ~PTE_U) &&
        ai_memblock_pte_fault_check_leaf_unchanged(reserved_page, reserved_leaf)
    );

    TEST_END();
}

bool ai_micro_memblock_pte_u_sum_mode_mismatch_faults_corner()
{
    TEST_START();

    const enum test_page s_access_u_page = VSRWX_GURWX;
    const enum test_page u_access_s_page = VSURWX_GRWX;
    const enum test_page xonly_super_page = VSX_GX;
    const uintptr_t s_load_vaddr = hs_page_base(s_access_u_page) + 0x220U;
    const uintptr_t s_store_vaddr = hs_page_base(s_access_u_page) + 0x228U;
    const uintptr_t s_fetch_vaddr = hs_page_base(s_access_u_page) + 0x230U;
    const uintptr_t u_load_vaddr = hs_page_base(u_access_s_page) + 0x240U;
    const uintptr_t u_store_vaddr = hs_page_base(u_access_s_page) + 0x248U;
    const uintptr_t u_fetch_vaddr = hs_page_base(u_access_s_page) + 0x250U;
    const uintptr_t u_amo_vaddr = hs_page_base(u_access_s_page) + 0x258U;
    const uintptr_t xonly_super_vaddr = hs_page_base(xonly_super_page) + 0x260U;
    const uintptr_t s_page_paddr = phys_page_base(s_access_u_page);
    const uintptr_t u_page_paddr = phys_page_base(u_access_s_page);
    const uintptr_t xonly_super_paddr = phys_page_base(xonly_super_page);
    const uintptr_t m_sum_direct_paddr = s_page_paddr + 0x238U;
    const uint64_t s_page_initial = 0x757365722d706167ULL;
    const uint64_t u_page_initial = 0x73757065722d7067ULL;
    const uint64_t xonly_super_initial = 0x786f6e6c792d7370ULL;
    const uint64_t s_page_sum_store = 0x53554d2d73746f31ULL;
    const uint64_t s_page_denied_store = 0x53554d2d64656e79ULL;
    const uint64_t s_page_sum_store_final = 0x53554d2d73746f32ULL;
    const uint64_t m_sum_direct_initial = 0x4d2d53554d2d7264ULL;
    const uint64_t m_sum_direct_store = 0x4d2d53554d2d7772ULL;
    const uint64_t u_page_super_store = 0x5355504552564953ULL;
    const uint64_t u_page_super_amo_operand = 0x11ULL;
    const uint64_t u_page_super_amomaxu_operand = 0xffffffffffffffffULL;
    const uint64_t u_amo_sentinel = 0xface00000000a111ULL;
    const uint64_t u_amomaxu_sentinel = 0xface00000000a222ULL;
    const uint64_t u_load_sentinel = 0xface00000000f00dULL;
    const uint64_t u_xonly_super_sentinel = 0xface00000000aa55ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_ECU);
    write64(s_page_paddr + 0x220U, s_page_initial);
    write64(s_page_paddr + 0x228U, s_page_initial);
    write64(m_sum_direct_paddr, m_sum_direct_initial);
    write32(s_page_paddr + 0x230U, 0x00008067U);
    write64(u_page_paddr + 0x240U, u_page_initial);
    write64(u_page_paddr + 0x248U, u_page_initial);
    write32(u_page_paddr + 0x250U, 0x00008067U);
    write64(u_page_paddr + 0x258U, u_page_initial);
    write64(xonly_super_paddr + 0x260U, xonly_super_initial);
    fence_iorw_iorw();
    ai_memblock_pte_fault_fence_i();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_memblock_pte_fault_hs_leaf_entry(s_access_u_page) |= PTE_U;
    *ai_memblock_pte_fault_hs_leaf_entry(u_access_s_page) &= ~PTE_U;
    const uint64_t user_leaf = *ai_memblock_pte_fault_hs_leaf_entry(s_access_u_page);
    const uint64_t supervisor_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(u_access_s_page);
    const uint64_t xonly_super_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(xonly_super_page);
    sfence_vma();
    TEST_ASSERT("the U/S mismatch setup has a U leaf for HS SUM=0 probes and a supervisor leaf under U-accessible non-leaf PTEs",
        (user_leaf & (PTE_V | PTE_U | PTE_RWX | PTE_A | PTE_D)) ==
            (PTE_V | PTE_U | PTE_RWX | PTE_A | PTE_D) &&
        (supervisor_leaf & PTE_U) == 0 &&
        (supervisor_leaf & (PTE_V | PTE_RWX | PTE_A | PTE_D)) ==
            (PTE_V | PTE_RWX | PTE_A | PTE_D) &&
        (xonly_super_leaf & (PTE_V | PTE_X | PTE_A | PTE_D)) ==
            (PTE_V | PTE_X | PTE_A | PTE_D) &&
        (xonly_super_leaf & (PTE_U | PTE_R | PTE_W)) == 0
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    ld(s_load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS load with SUM=0 to a PTE.U=1 page reports load page fault and the original VA",
        CAUSE_LPF, s_load_vaddr);

    TEST_SETUP_EXCEPT();
    sd(s_store_vaddr, 0x0102030405060708ULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS store with SUM=0 to a PTE.U=1 page reports store page fault and the original VA",
        CAUSE_SPF, s_store_vaddr);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(s_fetch_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS fetch from a PTE.U=1 page remains disallowed and reports instruction page fault",
        CAUSE_IPF, s_fetch_vaddr);

    CSRS(CSR_SSTATUS, SSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    uint64_t sum_load = ld(s_load_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS load with SUM=1 to the same PTE.U=1 page succeeds, proving the earlier load/store faults came from SUM",
        sum_load == s_page_initial);

    TEST_SETUP_EXCEPT();
    sd(s_store_vaddr, s_page_sum_store);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS store with SUM=1 to the same PTE.U=1 page succeeds through the same leaf",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t sum_store_read = ld(s_store_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the SUM=1 store to the PTE.U=1 page is immediately visible through the same HS translation",
        sum_store_read == s_page_sum_store);

    CSRC(CSR_SSTATUS, SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    ld(s_load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("after a SUM=1 hit, clearing SUM without changing the PTE or executing SFENCE.VMA makes the same U leaf load fault again",
        CAUSE_LPF, s_load_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t repeated_sum_rd_after_fault =
        ai_memblock_pte_fault_ld_rd_probe(s_load_vaddr,
            u_load_sentinel + 1ULL);
    TEST_ASSERT("a repeated SUM=0 load fault on the same PTE.U=1 leaf keeps LPF/tval stable and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == s_load_vaddr &&
        repeated_sum_rd_after_fault == (u_load_sentinel + 1ULL)
    );

    TEST_SETUP_EXCEPT();
    sd(s_store_vaddr, s_page_denied_store);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("after clearing SUM, a store to the same PTE.U=1 leaf faults again and must not overwrite the SUM=1 payload",
        CAUSE_SPF, s_store_vaddr);

    CSRS(CSR_SSTATUS, SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    ld(s_load_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("setting MXR=1 while SUM=0 still does not allow HS data access to the PTE.U=1 leaf",
        CAUSE_LPF, s_load_vaddr);

    CSRC(CSR_SSTATUS, SSTATUS_MXR);
    CSRS(CSR_SSTATUS, SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    sum_load = ld(s_load_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("setting SUM=1 again restores HS load permission to the same PTE.U=1 leaf without rebuilding the mapping",
        sum_load == s_page_initial);

    TEST_SETUP_EXCEPT();
    sd(s_store_vaddr, s_page_sum_store_final);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("setting SUM=1 again restores HS store permission to the same PTE.U=1 leaf",
        true);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    uint64_t m_sum0_direct_read = read64(m_sum_direct_paddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("M-mode direct read of the same PTE.U physical page succeeds while SUM=0",
        m_sum0_direct_read == m_sum_direct_initial);

    CSRS(CSR_MSTATUS, MSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    write64(m_sum_direct_paddr, m_sum_direct_store);
    uint64_t m_sum1_direct_read = read64(m_sum_direct_paddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("M-mode direct write/read of the same PTE.U physical page succeeds while SUM=1",
        m_sum1_direct_read == m_sum_direct_store);

    CSRC(CSR_MSTATUS, MSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t m_mxr0_direct_xonly_read =
        read64(xonly_super_paddr + 0x260U);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("M-mode direct physical read of an execute-only leaf's backing word succeeds with MXR=0 because direct access does not consult PTE.R/X",
        m_mxr0_direct_xonly_read == xonly_super_initial);

    CSRS(CSR_MSTATUS, MSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t m_mxr1_direct_xonly_read =
        read64(xonly_super_paddr + 0x260U);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("M-mode direct physical read of the same execute-only backing word is unchanged by MXR=1",
        m_mxr1_direct_xonly_read == xonly_super_initial);
    CSRC(CSR_MSTATUS, MSTATUS_MXR);

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    uint64_t hs_super_load = ld(u_load_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS load from a PTE.U=0 supervisor page succeeds and proves the supervisor leaf remains legal for S-mode data access",
        hs_super_load == u_page_initial);

    TEST_SETUP_EXCEPT();
    sd(u_store_vaddr, u_page_super_store);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS store to the PTE.U=0 supervisor page succeeds before switching to HU-mode denial probes",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t hs_super_store_read = ld(u_store_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS reload observes the value stored through the PTE.U=0 supervisor page",
        hs_super_store_read == u_page_super_store);

    TEST_SETUP_EXCEPT();
    uint64_t hs_super_amo_old = ai_memblock_pte_fault_amoadd_d_rd_probe(
        u_amo_vaddr, u_amo_sentinel, u_page_super_amo_operand);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS AMO to the PTE.U=0 supervisor page succeeds and returns the old memory value",
        hs_super_amo_old == u_page_initial);

    TEST_SETUP_EXCEPT();
    ((void (*)(void))u_fetch_vaddr)();
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS fetch from the PTE.U=0 supervisor page succeeds before switching to HU-mode denial probes",
        true);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_MXR);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t hs_xonly_super_load = ld(xonly_super_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS load with MXR=1 can read a PTE.U=0 execute-only supervisor leaf",
        hs_xonly_super_load == xonly_super_initial);

    goto_priv(PRIV_M);
    hspt_u_mode_allow_no_leaf();
    *ai_memblock_pte_fault_hs_leaf_entry(s_access_u_page) = user_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(u_access_s_page) = supervisor_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(xonly_super_page) =
        xonly_super_leaf;
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    uint64_t hu_rd_after_fault = ai_memblock_pte_fault_ld_rd_probe(
        u_load_vaddr, u_load_sentinel);
    TEST_ASSERT("HU load to a PTE.U=0 supervisor page reports load page fault, preserves tval, and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == u_load_vaddr &&
        hu_rd_after_fault == u_load_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(u_store_vaddr, 0x1111222233334444ULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HU store to a PTE.U=0 supervisor page reports store page fault and the original VA",
        CAUSE_SPF, u_store_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t hu_amo_after_fault = ai_memblock_pte_fault_amoadd_d_rd_probe(
        u_amo_vaddr, u_amo_sentinel, u_page_super_amo_operand);
    TEST_ASSERT("HU AMO to a PTE.U=0 supervisor page reports store page fault and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == u_amo_vaddr &&
        hu_amo_after_fault == u_amo_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t hu_amomaxu_after_fault =
        ai_memblock_pte_fault_amomaxu_d_rd_probe(u_amo_vaddr,
            u_amomaxu_sentinel, u_page_super_amomaxu_operand);
    TEST_ASSERT("HU AMOMAXU.D to the same PTE.U=0 supervisor page reports store page fault and leaves rd at its sentinel",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == u_amo_vaddr &&
        hu_amomaxu_after_fault == u_amomaxu_sentinel
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(u_fetch_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HU fetch from a PTE.U=0 supervisor page reports instruction page fault and the original VA",
        CAUSE_IPF, u_fetch_vaddr);

    TEST_SETUP_EXCEPT();
    uint64_t hu_xonly_super_rd_after_fault =
        ai_memblock_pte_fault_ld_rd_probe(xonly_super_vaddr,
            u_xonly_super_sentinel);
    TEST_ASSERT("HU load with MXR=1 still cannot cross the PTE.U=0 supervisor boundary on an execute-only leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == xonly_super_vaddr &&
        hu_xonly_super_rd_after_fault == u_xonly_super_sentinel
    );

    goto_priv_u_to_m_direct(PRIV_M);
    TEST_SETUP_EXCEPT();
    TEST_ASSERT("U/S mismatch faults preserve both leaf words and backing data for the denied stores",
        ai_memblock_pte_fault_check_leaf_unchanged(s_access_u_page, user_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(u_access_s_page,
            supervisor_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(xonly_super_page,
            xonly_super_leaf) &&
        read64(s_page_paddr + 0x228U) == s_page_sum_store_final &&
        read64(m_sum_direct_paddr) == m_sum_direct_store &&
        read64(u_page_paddr + 0x248U) == u_page_super_store &&
        read64(u_page_paddr + 0x258U) ==
            u_page_initial + u_page_super_amo_operand &&
        read64(xonly_super_paddr + 0x260U) == xonly_super_initial
    );

    TEST_END();
}

bool ai_micro_memblock_pte_fault_repair_recovers_translation_corner()
{
    TEST_START();

    const enum test_page repair_page = SWITCH2;
    const enum test_page neighbor_page = SWITCH1;
    const uintptr_t vaddr = hs_page_base(repair_page) + 0x2c0U;
    const uintptr_t paddr = phys_page_base(repair_page) + 0x2c0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t neighbor_vaddr = hs_page_base(neighbor_page) + 0x2c0U;
    const uintptr_t neighbor_paddr = phys_page_base(neighbor_page) + 0x2c0U;
    const uint64_t initial = 0x13579bdf2468ace0ULL;
    const uint64_t repaired = 0xf0e1d2c3b4a59687ULL;
    const uint64_t adjacent = 0x55aa66bb77cc88ddULL;
    const uint64_t neighbor_initial = 0x0ddc0ffeec001234ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    write64(neighbor_paddr, neighbor_initial);

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    const uint64_t valid_leaf = *ai_memblock_pte_fault_hs_leaf_entry(repair_page);
    *ai_memblock_pte_fault_hs_leaf_entry(repair_page) = valid_leaf & ~PTE_V;
    const uint64_t invalid_leaf = *ai_memblock_pte_fault_hs_leaf_entry(repair_page);
    sfence_vma();
    TEST_ASSERT("the repair probe first clears PTE.V on a previously legal leaf while keeping the original PPN/permission payload available",
        (valid_leaf & (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D)) ==
            (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D) &&
        (invalid_leaf & PTE_V) == 0
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("before repair, the invalidated leaf reports load page fault and the original VA",
        CAUSE_LPF, vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the load page fault does not mutate the invalid leaf or adjacent backing word",
        ai_memblock_pte_fault_check_leaf_unchanged(repair_page, invalid_leaf) &&
        read64(adjacent_paddr) == adjacent
    );

    *ai_memblock_pte_fault_hs_leaf_entry(repair_page) = valid_leaf;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after restoring the PTE and executing SFENCE.VMA, the same translated load succeeds",
        loaded == initial);

    TEST_SETUP_EXCEPT();
    uint64_t neighbor_loaded = ld(neighbor_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after the target-page repair, the neighboring page still translates and returns its original payload",
        neighbor_loaded == neighbor_initial);

    TEST_SETUP_EXCEPT();
    sd(vaddr, repaired);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the repaired translation also permits a store through the same VA",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("repairing the PTE restores translation for the target page without disturbing the adjacent word",
        ai_memblock_pte_fault_check_leaf_unchanged(repair_page, valid_leaf) &&
        read64(paddr) == repaired &&
        read64(adjacent_paddr) == adjacent &&
        read64(neighbor_paddr) == neighbor_initial
    );

    TEST_END();
}

bool ai_micro_hs_memblock_ptw_pmp_access_fault_repair_rewalks_translation_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x380U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x380U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x7074772d61636366ULL;
    const uint64_t adjacent = 0x13572468ace0bdf9ULL;
    const uint64_t store_payload = 0xfeed1234cafe5678ULL;
    const uint64_t load_sentinel = 0xbadc0de00000a11ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_memblock_pte_fault_restore_pmp_full_access();

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    const uint64_t leaf_before =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    const uintptr_t ptw_page = ai_memblock_pte_fault_ptw_pmp_page();
    TEST_ASSERT("the PTW PMP repair probe starts from a legal HS leaf and a page-aligned intermediate page-table page",
        (leaf_before & (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D)) ==
            (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D) &&
        ((leaf_before << 2) & PAGE_ADDR_MSK) ==
            (phys_page_base(target_page) & PAGE_ADDR_MSK) &&
        (ptw_page & (PAGE_SIZE - 1U)) == 0
    );

    ai_memblock_pte_fault_configure_pmp_window(ptw_page,
        ptw_page + PAGE_SIZE, AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_rd =
        ai_memblock_pte_fault_ld_rd_probe(vaddr, load_sentinel);
    TEST_ASSERT("when PMP denies the PTW read of an intermediate page-table page, an HS load reports load access fault on the original VA and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr &&
        load_rd == load_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PTW access fault leaves the page-table leaf and backing data unchanged before PMP repair",
        ai_memblock_pte_fault_check_leaf_unchanged(target_page,
            leaf_before) &&
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    ai_memblock_pte_fault_restore_pmp_full_access();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after restoring PMP read permission and executing SFENCE.VMA, the same HS load walks again and returns the backing payload",
        loaded == initial);

    goto_priv(PRIV_M);
    ai_memblock_pte_fault_configure_pmp_window(ptw_page,
        ptw_page + PAGE_SIZE, AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_payload);
    TEST_ASSERT("when the same PTW PMP denial is hit by an HS store, the exception is bound to the original store access cause and VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the store-class PTW access fault does not update the backing word or disturb the neighboring word",
        ai_memblock_pte_fault_check_leaf_unchanged(target_page,
            leaf_before) &&
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    ai_memblock_pte_fault_restore_pmp_full_access();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after the second PMP repair and SFENCE.VMA, the same HS store re-walks and commits normally",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t final_load = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the repaired PTW path remains usable for a following HS load through the same VA",
        final_load == store_payload);

    goto_priv(PRIV_M);
    TEST_ASSERT("PTW PMP denial and repair preserve the target leaf and only update the intended backing word after the repaired store",
        ai_memblock_pte_fault_check_leaf_unchanged(target_page,
            leaf_before) &&
        read64(paddr) == store_payload &&
        read64(adjacent_paddr) == adjacent
    );
    ai_memblock_pte_fault_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_memblock_amo_lrsc_ptw_pmp_access_fault_repair_rewalks_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x3c0U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x3c0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x616d6f2d70747731ULL;
    const uint64_t adjacent = 0x2468ace013579bdfULL;
    const uint64_t amo_addend = 0x0001000200030004ULL;
    const uint64_t lr_sc_payload = 0x1122334455667788ULL;
    const uint64_t stale_sc_payload = 0xa5a55a5adeadbeefULL;
    const uint64_t amo_sentinel = 0xbadc0de00000a21ULL;
    const uint64_t lr_sentinel = 0xbadc0de00000a22ULL;
    const uint64_t amo_expected = initial + amo_addend;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_memblock_pte_fault_restore_pmp_full_access();

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    const uint64_t leaf_before =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    const uintptr_t ptw_page = ai_memblock_pte_fault_ptw_pmp_page();
    TEST_ASSERT("the AMO/LRSC PTW PMP probe starts from a legal RWX leaf and a page-aligned intermediate page-table page",
        (leaf_before & (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D)) ==
            (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D) &&
        ((leaf_before << 2) & PAGE_ADDR_MSK) ==
            (phys_page_base(target_page) & PAGE_ADDR_MSK) &&
        (ptw_page & (PAGE_SIZE - 1U)) == 0
    );

    ai_memblock_pte_fault_configure_pmp_window(ptw_page,
        ptw_page + PAGE_SIZE, AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t amo_rd = ai_memblock_pte_fault_amoadd_d_rd_probe(vaddr,
        amo_sentinel, amo_addend);
    TEST_ASSERT("when PMP denies the PTW read for an HS amoadd.d, the fault is reported as store/AMO access fault on the original VA and rd is unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        amo_rd == amo_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting AMO-class PTW access does not update memory or modify the legal target leaf",
        ai_memblock_pte_fault_check_leaf_unchanged(target_page,
            leaf_before) &&
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    ai_memblock_pte_fault_restore_pmp_full_access();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t repaired_amo_old =
        ai_memblock_pte_fault_amoadd_d_rd_probe(vaddr, 0ULL, amo_addend);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after restoring PMP read permission and executing SFENCE.VMA, the same AMO path re-walks and returns the old doubleword",
        repaired_amo_old == initial);

    TEST_SETUP_EXCEPT();
    uint64_t after_amo = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the repaired AMO updates exactly the target doubleword",
        after_amo == amo_expected);

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired AMO leaves the PTW leaf and adjacent backing word unchanged",
        ai_memblock_pte_fault_check_leaf_unchanged(target_page,
            leaf_before) &&
        read64(paddr) == amo_expected &&
        read64(adjacent_paddr) == adjacent
    );

    ai_memblock_pte_fault_configure_pmp_window(ptw_page,
        ptw_page + PAGE_SIZE, AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t lr_rd = ai_memblock_pte_fault_lr_d_rd_probe(vaddr,
        lr_sentinel);
    TEST_ASSERT("when PMP denies the PTW read for an HS lr.d, the fault is reported as load access fault on the original VA and rd is unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr &&
        lr_rd == lr_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting LR-class PTW access does not create a data update or disturb the target leaf",
        ai_memblock_pte_fault_check_leaf_unchanged(target_page,
            leaf_before) &&
        read64(paddr) == amo_expected &&
        read64(adjacent_paddr) == adjacent
    );

    ai_memblock_pte_fault_restore_pmp_full_access();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t stale_sc_status = ai_memblock_pte_fault_sc_d_rd_probe(vaddr,
        0ULL, stale_sc_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the LR PTW access fault, a stale sc.d without a fresh lr.d fails and proves the faulting LR did not create a reservation",
        stale_sc_status != 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t after_stale_sc = ld(vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the stale sc.d after the faulting LR leaves the repaired target value unchanged",
        after_stale_sc == amo_expected);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_lrsc_result_t repaired_pair =
        ai_memblock_pte_fault_lr_sc_d_pair(vaddr, lr_sc_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after PMP repair and SFENCE.VMA, a fresh lr.d/sc.d pair re-walks and commits normally",
        repaired_pair.old_value == amo_expected &&
        repaired_pair.status == 0ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("AMO/LRSC PTW PMP denial and repair preserve the target leaf and only commit the repaired AMO plus fresh LR/SC updates",
        ai_memblock_pte_fault_check_leaf_unchanged(target_page,
            leaf_before) &&
        read64(paddr) == lr_sc_payload &&
        read64(adjacent_paddr) == adjacent
    );
    ai_memblock_pte_fault_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_memblock_fetch_ptw_pmp_access_fault_repair_rewalks_corner()
{
    TEST_START();

    const enum test_page fetch_page = VSRWX_GRWX;
    const uintptr_t fetch_vaddr = hs_page_base(fetch_page) + 0x440U;
    const uintptr_t fetch_paddr = phys_page_base(fetch_page) + 0x440U;
    const uintptr_t guard_paddr = fetch_paddr + sizeof(uint32_t);
    const uint32_t ret_inst = 0x00008067U;
    const uint64_t guard_initial = 0xfeedface13572468ULL;

    goto_priv(PRIV_M);
    write32(fetch_paddr, ret_inst);
    write64(guard_paddr, guard_initial);
    fence_iorw_iorw();
    ai_memblock_pte_fault_fence_i();
    ai_memblock_pte_fault_restore_pmp_full_access();

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    const uint64_t leaf_before =
        *ai_memblock_pte_fault_hs_leaf_entry(fetch_page);
    const uintptr_t ptw_page = ai_memblock_pte_fault_ptw_pmp_page();
    TEST_ASSERT("the fetch PTW PMP probe starts from a legal executable HS leaf and a page-aligned intermediate page-table page",
        (leaf_before & (PTE_V | PTE_X | PTE_A | PTE_D)) ==
            (PTE_V | PTE_X | PTE_A | PTE_D) &&
        ((leaf_before << 2) & PAGE_ADDR_MSK) ==
            (phys_page_base(fetch_page) & PAGE_ADDR_MSK) &&
        (ptw_page & (PAGE_SIZE - 1U)) == 0
    );

    ai_memblock_pte_fault_configure_pmp_window(ptw_page,
        ptw_page + PAGE_SIZE, AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(fetch_vaddr);
    TEST_ASSERT("when PMP denies the PTW read for an HS fetch, the fault is reported as instruction access fault on the original VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == fetch_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting fetch-class PTW access leaves the executable leaf, instruction word, and guard word unchanged before PMP repair",
        ai_memblock_pte_fault_check_leaf_unchanged(fetch_page,
            leaf_before) &&
        read32(fetch_paddr) == ret_inst &&
        read64(guard_paddr) == guard_initial
    );

    ai_memblock_pte_fault_restore_pmp_full_access();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void (*)(void))fetch_vaddr)();
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after restoring PMP read permission and executing SFENCE.VMA, the same HS fetch re-walks and retires the installed return instruction",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("fetch PTW PMP denial and repair preserve the target leaf and never update the instruction or guard word",
        ai_memblock_pte_fault_check_leaf_unchanged(fetch_page,
            leaf_before) &&
        read32(fetch_paddr) == ret_inst &&
        read64(guard_paddr) == guard_initial
    );
    ai_memblock_pte_fault_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_memblock_leaf_g_rsw_pmp_access_faults_preserve_encoding_corner()
{
    TEST_START();

    const enum test_page load_page = VSRWX_GRWX;
    const enum test_page store_page = VSRWX_GRW;
    const enum test_page amo_page = VSRW_GRW;
    const enum test_page fetch_page = VSR_GRWX;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x180U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x1c0U;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0x200U;
    const uintptr_t fetch_vaddr = hs_page_base(fetch_page) + 0x240U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x180U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x1c0U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0x200U;
    const uintptr_t fetch_paddr = phys_page_base(fetch_page) + 0x240U;
    const uintptr_t guard_paddr = phys_page_base(fetch_page) + 0x248U;
    const uint64_t load_initial = 0x47525357504d5031ULL;
    const uint64_t store_initial = 0x47525357504d5032ULL;
    const uint64_t amo_initial = 0x47525357504d5033ULL;
    const uint64_t store_payload = 0x1122334455667788ULL;
    const uint64_t amo_addend = 0x0000000000000101ULL;
    const uint64_t guard_initial = 0x47525357504d5034ULL;
    const uint32_t ret_inst = 0x00008067U;
    const uint64_t grsw_flags = PTE_V | PTE_AD | PTE_RWX | PTE_G |
        PTE_RSW_MSK;

    goto_priv(PRIV_M);
    write64(load_paddr, load_initial);
    write64(store_paddr, store_initial);
    write64(amo_paddr, amo_initial);
    write32(fetch_paddr, ret_inst);
    write64(guard_paddr, guard_initial);
    fence_iorw_iorw();
    ai_memblock_pte_fault_fence_i();
    ai_memblock_pte_fault_restore_pmp_full_access();

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    *ai_memblock_pte_fault_hs_leaf_entry(load_page) =
        (phys_page_base(load_page) >> 2) | grsw_flags;
    *ai_memblock_pte_fault_hs_leaf_entry(store_page) =
        (phys_page_base(store_page) >> 2) | grsw_flags;
    *ai_memblock_pte_fault_hs_leaf_entry(amo_page) =
        (phys_page_base(amo_page) >> 2) | grsw_flags;
    *ai_memblock_pte_fault_hs_leaf_entry(fetch_page) =
        (phys_page_base(fetch_page) >> 2) | grsw_flags;
    const uint64_t load_leaf = *ai_memblock_pte_fault_hs_leaf_entry(load_page);
    const uint64_t store_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_page);
    const uint64_t amo_leaf = *ai_memblock_pte_fault_hs_leaf_entry(amo_page);
    const uint64_t fetch_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(fetch_page);

    TEST_ASSERT("the PMP leaf attribute probe installs 4K leaves with G=1 and both RSW bits set without changing the VA to PA mapping",
        (load_leaf & (PTE_G | PTE_RSW_MSK | PTE_RWX | PTE_AD | PTE_V)) ==
            grsw_flags &&
        (store_leaf & (PTE_G | PTE_RSW_MSK | PTE_RWX | PTE_AD | PTE_V)) ==
            grsw_flags &&
        (amo_leaf & (PTE_G | PTE_RSW_MSK | PTE_RWX | PTE_AD | PTE_V)) ==
            grsw_flags &&
        (fetch_leaf & (PTE_G | PTE_RSW_MSK | PTE_RWX | PTE_AD | PTE_V)) ==
            grsw_flags &&
        ((load_leaf << 2) & PAGE_ADDR_MSK) ==
            phys_page_base(load_page) &&
        ((store_leaf << 2) & PAGE_ADDR_MSK) ==
            phys_page_base(store_page) &&
        ((amo_leaf << 2) & PAGE_ADDR_MSK) ==
            phys_page_base(amo_page) &&
        ((fetch_leaf << 2) & PAGE_ADDR_MSK) ==
            phys_page_base(fetch_page)
    );

    ai_memblock_pte_fault_configure_pmp_window(phys_page_base(load_page),
        phys_page_base(load_page) + PAGE_SIZE,
        AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_R);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_rd = ai_memblock_pte_fault_ld_rd_probe(load_vaddr,
        0xbadc0de00000b11ULL);
    TEST_ASSERT("PTE.G and RSW do not bypass a PMP read denial for HS load and the trap is a load access fault on the original VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == load_vaddr &&
        load_rd == 0xbadc0de00000b11ULL
    );

    goto_priv(PRIV_M);
    ai_memblock_pte_fault_restore_pmp_full_access();
    ai_memblock_pte_fault_configure_pmp_window(phys_page_base(store_page),
        phys_page_base(store_page) + PAGE_SIZE,
        AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_W);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_payload);
    TEST_ASSERT("PTE.G and RSW do not bypass a PMP write denial for HS store and the trap is a store access fault on the original VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == store_vaddr
    );

    goto_priv(PRIV_M);
    ai_memblock_pte_fault_restore_pmp_full_access();
    ai_memblock_pte_fault_configure_pmp_window(phys_page_base(amo_page),
        phys_page_base(amo_page) + PAGE_SIZE,
        AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_W);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t amo_rd = ai_memblock_pte_fault_amoadd_d_rd_probe(amo_vaddr,
        0xbadc0de00000b22ULL, amo_addend);
    TEST_ASSERT("PTE.G and RSW do not bypass a PMP AMO denial and rd remains unchanged on the store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == amo_vaddr &&
        amo_rd == 0xbadc0de00000b22ULL
    );

    goto_priv(PRIV_M);
    ai_memblock_pte_fault_restore_pmp_full_access();
    ai_memblock_pte_fault_configure_pmp_window(phys_page_base(fetch_page),
        phys_page_base(fetch_page) + PAGE_SIZE,
        AI_MEMBLOCK_PTE_FAULT_PMP_TOR_NO_X);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(fetch_vaddr);
    TEST_ASSERT("PTE.G and RSW do not bypass a PMP execute denial for HS fetch and the trap is an instruction access fault on the original VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == fetch_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("all G/RSW PMP access-fault probes preserve the exact leaf encodings and leave data/instruction backing words unchanged",
        ai_memblock_pte_fault_check_leaf_unchanged(load_page, load_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(store_page, store_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(amo_page, amo_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(fetch_page, fetch_leaf) &&
        read64(load_paddr) == load_initial &&
        read64(store_paddr) == store_initial &&
        read64(amo_paddr) == amo_initial &&
        read32(fetch_paddr) == ret_inst &&
        read64(guard_paddr) == guard_initial
    );

    ai_memblock_pte_fault_restore_pmp_full_access();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_after_repair = ld(load_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after restoring PMP access, the G/RSW load leaf remains a normal readable 4K mapping",
        load_after_repair == load_initial);

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after restoring PMP access, the G/RSW store leaf permits the write",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = ai_memblock_pte_fault_amoadd_d_rd_probe(amo_vaddr,
        0ULL, amo_addend);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after restoring PMP access, the G/RSW AMO leaf returns the old value and updates normally",
        amo_old == amo_initial);

    TEST_SETUP_EXCEPT();
    ((void (*)(void))fetch_vaddr)();
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after restoring PMP access, the G/RSW fetch leaf executes the installed return instruction",
        true);

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired G/RSW leaves still preserve their encodings while only the intended store and AMO target words changed",
        ai_memblock_pte_fault_check_leaf_unchanged(load_page, load_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(store_page, store_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(amo_page, amo_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(fetch_page, fetch_leaf) &&
        read64(load_paddr) == load_initial &&
        read64(store_paddr) == store_payload &&
        read64(amo_paddr) == amo_initial + amo_addend &&
        read32(fetch_paddr) == ret_inst &&
        read64(guard_paddr) == guard_initial
    );
    ai_memblock_pte_fault_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_memblock_faulting_load_widths_do_not_clobber_rd_before_repair_corner()
{
    TEST_START();

    const enum test_page target_page = VSR_GRWX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x340U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x340U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x440U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x440U;
    const uint64_t target_pattern = 0x88776655c43382f1ULL;
    const uint64_t guard_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t fault_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t sentinel[] = {
        0xa55a0000000000b1ULL,
        0xa55a0000000000b2ULL,
        0xa55a0000000000b3ULL,
        0xa55a0000000000b4ULL,
        0xa55a0000000000b5ULL,
        0xa55a0000000000b6ULL,
        0xa55a0000000000b7ULL,
    };
    const uint64_t expected_lb = 0xfffffffffffffff1ULL;
    const uint64_t expected_lbu = 0xf1ULL;
    const uint64_t expected_lh = 0xffffffffffff82f1ULL;
    const uint64_t expected_lhu = 0x82f1ULL;
    const uint64_t expected_lw = 0xffffffffc43382f1ULL;
    const uint64_t expected_lwu = 0xc43382f1ULL;

    goto_priv(PRIV_M);
    write64(target_paddr, target_pattern);
    write64(guard_paddr, guard_pattern);

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    *ai_memblock_pte_fault_hs_leaf_entry(target_page) =
        (phys_page_base(target_page) >> 2) | fault_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(guard_page) =
        (phys_page_base(guard_page) >> 2) | legal_leaf;
    const uint64_t fault_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    const uint64_t guard_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the faulting-load rd probe starts with one no-read HS leaf and one legal guard leaf in ordinary cacheable DRAM",
        (fault_leaf_word & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (fault_leaf_word & PTE_R) == 0 &&
        (guard_leaf_word & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lb from the no-read leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_lb_rd_probe, target_vaddr, sentinel[0]);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lbu from the same no-read leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_lbu_rd_probe, target_vaddr, sentinel[1]);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lh from the no-read leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_lh_rd_probe, target_vaddr, sentinel[2]);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lhu from the same no-read leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_lhu_rd_probe, target_vaddr, sentinel[3]);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lw from the no-read leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_lw_rd_probe, target_vaddr, sentinel[4]);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lwu from the same no-read leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_lwu_rd_probe, target_vaddr, sentinel[5]);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS ld from the no-read leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_ld_rd_probe, target_vaddr, sentinel[6]);

    goto_priv(PRIV_M);
    TEST_ASSERT("none of the denied load-width probes mutates the target PTE, guard PTE, target data, or guard data",
        *ai_memblock_pte_fault_hs_leaf_entry(target_page) == fault_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) == guard_leaf_word &&
        read64(target_paddr) == target_pattern &&
        read64(guard_paddr) == guard_pattern
    );

    *ai_memblock_pte_fault_hs_leaf_entry(target_page) =
        (phys_page_base(target_page) >> 2) | legal_leaf;
    const uint64_t repaired_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t lb_value = (uint64_t)(int64_t)(int8_t)lb(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the leaf, lb returns the sign-extended byte from the target word",
        lb_value == expected_lb);

    TEST_SETUP_EXCEPT();
    uint64_t lbu_value = lbu(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the leaf, lbu returns the zero-extended byte from the target word",
        lbu_value == expected_lbu);

    TEST_SETUP_EXCEPT();
    uint64_t lh_value = (uint64_t)(int64_t)(int16_t)lh(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the leaf, lh returns the sign-extended halfword from the target word",
        lh_value == expected_lh);

    TEST_SETUP_EXCEPT();
    uint64_t lhu_value = lhu(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the leaf, lhu returns the zero-extended halfword from the target word",
        lhu_value == expected_lhu);

    TEST_SETUP_EXCEPT();
    uint64_t lw_value = (uint64_t)(int64_t)(int32_t)lw(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the leaf, lw returns the sign-extended word from the target word",
        lw_value == expected_lw);

    TEST_SETUP_EXCEPT();
    uint64_t lwu_value = lwu(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the leaf, lwu returns the zero-extended word from the target word",
        lwu_value == expected_lwu);

    TEST_SETUP_EXCEPT();
    uint64_t ld_value = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the leaf, ld returns the full target doubleword",
        ld_value == target_pattern);

    TEST_SETUP_EXCEPT();
    uint64_t guard_value = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the independent guard leaf remains readable after the fault/recovery width sweep",
        guard_value == guard_pattern);

    goto_priv(PRIV_M);
    TEST_ASSERT("the load-width rd clobber probe ends with the repaired target leaf, stable guard leaf, and unchanged backing data",
        *ai_memblock_pte_fault_hs_leaf_entry(target_page) ==
            repaired_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(target_paddr) == target_pattern &&
        read64(guard_paddr) == guard_pattern
    );

    TEST_END();
}

bool ai_micro_hs_memblock_faulting_fp_loads_do_not_clobber_frd_or_fflags_before_repair_corner()
{
    TEST_START();

    const enum test_page target_page = VSR_GRWX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x580U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x580U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x680U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x680U;
    const uint64_t target_pattern = 0xfff812347fc12345ULL;
    const uint64_t guard_pattern = 0x5a5aa5a50f1e2d3cULL;
    const uint64_t fault_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t flw_sentinel = 0xffffffff8123c0deULL;
    const uint64_t fld_sentinel = 0x7ff80000deadbabeULL;
    const uint64_t non_target_fpr_sentinel = 0x55aa33cc77ee11ffULL;
    const uint64_t initial_fflags = 0x15ULL;
    const uint64_t initial_frm = 0x2ULL;
    const uint64_t initial_fcsr = (initial_frm << 5) | initial_fflags;
    const uint64_t expected_flw = 0x000000007fc12345ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    write64(target_paddr, target_pattern);
    write64(guard_paddr, guard_pattern);

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    *ai_memblock_pte_fault_hs_leaf_entry(target_page) =
        (phys_page_base(target_page) >> 2) | fault_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(guard_page) =
        (phys_page_base(guard_page) >> 2) | legal_leaf;
    const uint64_t fault_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    const uint64_t guard_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the faulting FP-load probe starts with one no-read HS leaf, one legal guard leaf, and FP state enabled",
        (fault_leaf_word & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (fault_leaf_word & PTE_R) == 0 &&
        (guard_leaf_word & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    ai_memblock_pte_fault_write_fcsr(initial_fflags, initial_frm);
    ai_memblock_pte_fault_seed_non_target_fpr(non_target_fpr_sentinel);

    AI_MEMBLOCK_CHECK_FAULTING_FP_LOAD_FRD(
        "faulting HS flw from a no-read leaf reports LPF/tval and leaves frd plus fflags unchanged",
        ai_memblock_pte_fault_flw_frd_probe, target_vaddr, flw_sentinel,
        initial_fflags);
    AI_MEMBLOCK_CHECK_FAULTING_FP_LOAD_FRD(
        "faulting HS fld from the same no-read leaf reports LPF/tval and leaves frd plus fflags unchanged",
        ai_memblock_pte_fault_fld_frd_probe, target_vaddr, fld_sentinel,
        initial_fflags);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("denied FP-load probes leave the target PTE, guard PTE, target data, guard data, and fflags sentinel unchanged",
        *ai_memblock_pte_fault_hs_leaf_entry(target_page) ==
            fault_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(target_paddr) == target_pattern &&
        read64(guard_paddr) == guard_pattern &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr &&
        ai_memblock_pte_fault_read_non_target_fpr() ==
            non_target_fpr_sentinel
    );

    *ai_memblock_pte_fault_hs_leaf_entry(target_page) =
        (phys_page_base(target_page) >> 2) | legal_leaf;
    const uint64_t repaired_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    ai_memblock_pte_fault_write_fcsr(initial_fflags, initial_frm);
    ai_memblock_pte_fault_seed_non_target_fpr(non_target_fpr_sentinel);

    TEST_SETUP_EXCEPT();
    uint64_t flw_value =
        ai_memblock_pte_fault_flw_frd_probe(target_vaddr, flw_sentinel);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the no-read leaf, flw writes the low single-precision payload into frd",
        flw_value == expected_flw &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    uint64_t fld_value =
        ai_memblock_pte_fault_fld_frd_probe(target_vaddr, fld_sentinel);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the no-read leaf, fld writes the complete doubleword payload into frd",
        fld_value == target_pattern &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    uint64_t guard_value = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the independent guard leaf remains readable after the FP fault/recovery sweep",
        guard_value == guard_pattern);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("the FP-load frd clobber probe ends with repaired target leaf, stable guard leaf, unchanged backing data, and unchanged fflags",
        *ai_memblock_pte_fault_hs_leaf_entry(target_page) ==
            repaired_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(target_paddr) == target_pattern &&
        read64(guard_paddr) == guard_pattern &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr &&
        ai_memblock_pte_fault_read_non_target_fpr() ==
            non_target_fpr_sentinel
    );

    TEST_END();
}

bool ai_micro_hs_memblock_faulting_fp_stores_do_not_update_memory_or_forward_after_exception_return_corner()
{
    TEST_START();

    const enum test_page target_page = VSR_GRWX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t fsw_base_vaddr = hs_page_base(target_page) + 0x900U;
    const uintptr_t fsw_store_vaddr = fsw_base_vaddr + sizeof(uint32_t);
    const uintptr_t fsw_base_paddr = phys_page_base(target_page) + 0x900U;
    const uintptr_t fsd_vaddr = hs_page_base(target_page) + 0x940U;
    const uintptr_t fsd_paddr = phys_page_base(target_page) + 0x940U;
    const uintptr_t adjacent_paddr = fsd_paddr + sizeof(uint64_t);
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0xa00U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0xa00U;
    const uint64_t fsw_initial = 0x8877665544332211ULL;
    const uint64_t fsd_initial = 0x0123456789abcdefULL;
    const uint64_t adjacent_initial = 0xfeedfacecafebeefULL;
    const uint64_t guard_initial = 0x5a5aa5a50f1e2d3cULL;
    const uint64_t fsw_killed_bits = 0x000000007fc12345ULL;
    const uint64_t fsw_repaired_bits = 0x000000003f8abcdeULL;
    const uint64_t fsd_killed_bits = 0xfff8123456789abcULL;
    const uint64_t fsd_repaired_bits = 0x400921fb54442d18ULL;
    const uint64_t fault_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t non_target_fpr_sentinel = 0x55aa33cc77ee11ffULL;
    const uint64_t initial_fflags = 0x16ULL;
    const uint64_t initial_frm = 0x3ULL;
    const uint64_t initial_fcsr = (initial_frm << 5) | initial_fflags;
    const uint64_t expected_fsw_old_word = (fsw_initial >> 32) &
        0xffffffffULL;
    const uint64_t expected_fsw_repaired =
        (fsw_initial & 0x00000000ffffffffULL) |
        ((fsw_repaired_bits & 0xffffffffULL) << 32);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    write64(fsw_base_paddr, fsw_initial);
    write64(fsd_paddr, fsd_initial);
    write64(adjacent_paddr, adjacent_initial);
    write64(guard_paddr, guard_initial);

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    *ai_memblock_pte_fault_hs_leaf_entry(target_page) =
        (phys_page_base(target_page) >> 2) | fault_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(guard_page) =
        (phys_page_base(guard_page) >> 2) | legal_leaf;
    const uint64_t fault_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    const uint64_t guard_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the faulting FP-store probe starts with a readable no-write HS leaf, a legal guard leaf, and FP state enabled",
        (fault_leaf_word & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R) &&
        (fault_leaf_word & PTE_W) == 0 &&
        (guard_leaf_word & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    ai_memblock_pte_fault_write_fcsr(initial_fflags, initial_frm);
    ai_memblock_pte_fault_seed_non_target_fpr(non_target_fpr_sentinel);

    TEST_SETUP_EXCEPT();
    uint64_t fsw_warm = ld(fsw_base_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the fsw target doubleword is readable before the store-permission fault probe",
        fsw_warm == fsw_initial);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fsw_probe(fsw_store_vaddr, fsw_killed_bits);
    TEST_ASSERT("faulting HS fsw to the read-only leaf reports SPF/tval and leaves fflags unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fsw_store_vaddr &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    uint64_t fsw_after_fault_word = lwu(fsw_store_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the same-address word load after faulting fsw sees the old high word instead of the FP store payload",
        fsw_after_fault_word == expected_fsw_old_word);

    TEST_SETUP_EXCEPT();
    uint64_t fsw_after_fault_full = ld(fsw_base_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the full doubleword load after faulting fsw does not receive killed FP store data through forwarding",
        fsw_after_fault_full == fsw_initial);

    TEST_SETUP_EXCEPT();
    uint64_t fsd_warm = ld(fsd_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the fsd target doubleword is readable before the store-permission fault probe",
        fsd_warm == fsd_initial);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fsd_probe(fsd_vaddr, fsd_killed_bits);
    TEST_ASSERT("faulting HS fsd to the read-only leaf reports SPF/tval and leaves fflags unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fsd_vaddr &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    uint64_t fsd_after_fault = ld(fsd_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the same-address doubleword load after faulting fsd sees the old image, not the killed FP store payload",
        fsd_after_fault == fsd_initial);

    TEST_SETUP_EXCEPT();
    uint64_t guard_after_fault = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the independent guard leaf remains readable after the faulting FP-store observer sequence",
        guard_after_fault == guard_initial);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("denied FP stores leave target memory, adjacent data, guard data, PTE words, and fflags unchanged before repair",
        *ai_memblock_pte_fault_hs_leaf_entry(target_page) ==
            fault_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(fsw_base_paddr) == fsw_initial &&
        read64(fsd_paddr) == fsd_initial &&
        read64(adjacent_paddr) == adjacent_initial &&
        read64(guard_paddr) == guard_initial &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr &&
        ai_memblock_pte_fault_read_non_target_fpr() ==
            non_target_fpr_sentinel
    );

    *ai_memblock_pte_fault_hs_leaf_entry(target_page) =
        (phys_page_base(target_page) >> 2) | legal_leaf;
    const uint64_t repaired_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    ai_memblock_pte_fault_write_fcsr(initial_fflags, initial_frm);
    ai_memblock_pte_fault_seed_non_target_fpr(non_target_fpr_sentinel);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fsw_probe(fsw_store_vaddr, fsw_repaired_bits);
    fence_iorw_iorw();
    uint64_t fsw_repaired_full = ld(fsw_base_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, fsw updates only the addressed high word lane and keeps fflags stable",
        fsw_repaired_full == expected_fsw_repaired &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fsd_probe(fsd_vaddr, fsd_repaired_bits);
    fence_iorw_iorw();
    uint64_t fsd_repaired_full = ld(fsd_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, fsd updates the complete target doubleword and keeps fflags stable",
        fsd_repaired_full == fsd_repaired_bits &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    uint64_t guard_after_repair = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the independent guard leaf remains readable after repaired FP stores",
        guard_after_repair == guard_initial);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("the FP-store side-effect probe ends with repaired target leaf, stable guard leaf, precise target updates, and stable fflags",
        *ai_memblock_pte_fault_hs_leaf_entry(target_page) ==
            repaired_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(fsw_base_paddr) == expected_fsw_repaired &&
        read64(fsd_paddr) == fsd_repaired_bits &&
        read64(adjacent_paddr) == adjacent_initial &&
        read64(guard_paddr) == guard_initial &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr &&
        ai_memblock_pte_fault_read_non_target_fpr() ==
            non_target_fpr_sentinel
    );

    TEST_END();
}

bool ai_micro_hs_memblock_cross_page_fp_load_store_second_half_fault_keeps_fp_and_precise_partial_memory_corner()
{
    TEST_START();

    const enum test_page load_low_page = VSR_GX;
    const enum test_page load_high_page = VSW_GI;
    const enum test_page store_low_page = VSRW_GX;
    const enum test_page store_high_page = VSRX_GRWX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t load_vaddr =
        ai_memblock_pte_fault_cross_page_addr(load_low_page);
    const uintptr_t load_second_vaddr = load_vaddr + sizeof(uint32_t);
    const uintptr_t store_vaddr =
        ai_memblock_pte_fault_cross_page_addr(store_low_page);
    const uintptr_t store_second_vaddr = store_vaddr + sizeof(uint32_t);
    const uintptr_t load_low_adjacent_paddr =
        phys_page_base(load_low_page) + PAGE_SIZE - (2U * sizeof(uint64_t));
    const uintptr_t load_low_paddr =
        ai_memblock_pte_fault_page_end_phys(load_low_page);
    const uintptr_t load_high_paddr = phys_page_base(load_high_page);
    const uintptr_t load_high_adjacent_paddr =
        load_high_paddr + sizeof(uint64_t);
    const uintptr_t store_low_adjacent_paddr =
        phys_page_base(store_low_page) + PAGE_SIZE - (2U * sizeof(uint64_t));
    const uintptr_t store_low_paddr =
        ai_memblock_pte_fault_page_end_phys(store_low_page);
    const uintptr_t store_high_paddr = phys_page_base(store_high_page);
    const uintptr_t store_high_adjacent_paddr =
        store_high_paddr + sizeof(uint64_t);
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0xb00U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0xb00U;
    const uint64_t load_low_adjacent = 0x1021324354657687ULL;
    const uint64_t load_low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t load_high_pattern = 0x1032547698badcfeULL;
    const uint64_t load_high_adjacent = 0xcc33cc3355aa55aaULL;
    const uint64_t store_low_adjacent = 0xfaceb00c12344321ULL;
    const uint64_t store_low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t store_high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t store_high_adjacent = 0xabcdef0123456789ULL;
    const uint64_t guard_pattern = 0x5a5aa5a50f1e2d3cULL;
    const uint64_t fld_sentinel = 0x7ff80000deadbabeULL;
    const uint64_t fsd_fault_payload = 0xfedcba9876543210ULL;
    const uint64_t fsd_repair_payload = 0x0123456789abcdefULL;
    const uint64_t initial_fflags = 0x13ULL;
    const uint64_t load_fault_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t store_fault_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t expected_fld_repaired =
        ((load_high_pattern & 0xffffffffULL) << 32) |
        (load_low_pattern >> 32);
    const uint64_t expected_store_low_after =
        (store_low_pattern & 0xffffffffULL) |
        ((fsd_repair_payload & 0xffffffffULL) << 32);
    const uint64_t expected_store_high_after =
        (store_high_pattern & 0xffffffff00000000ULL) |
        (fsd_repair_payload >> 32);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    write64(load_low_adjacent_paddr, load_low_adjacent);
    write64(load_low_paddr, load_low_pattern);
    write64(load_high_paddr, load_high_pattern);
    write64(load_high_adjacent_paddr, load_high_adjacent);
    write64(store_low_adjacent_paddr, store_low_adjacent);
    write64(store_low_paddr, store_low_pattern);
    write64(store_high_paddr, store_high_pattern);
    write64(store_high_adjacent_paddr, store_high_adjacent);
    write64(guard_paddr, guard_pattern);

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    *ai_memblock_pte_fault_hs_leaf_entry(load_low_page) =
        (phys_page_base(load_low_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(load_high_page) =
        (phys_page_base(load_high_page) >> 2) | load_fault_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(store_low_page) =
        (phys_page_base(store_low_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(store_high_page) =
        (phys_page_base(store_high_page) >> 2) | store_fault_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(guard_page) =
        (phys_page_base(guard_page) >> 2) | legal_leaf;
    const uint64_t load_low_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_low_page);
    const uint64_t load_high_fault_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_high_page);
    const uint64_t store_low_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_low_page);
    const uint64_t store_high_fault_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_high_page);
    const uint64_t guard_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the cross-page FP fault setup starts with legal low halves, denied high halves, a legal guard leaf, and FP state enabled",
        (load_low_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (load_high_fault_leaf & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (load_high_fault_leaf & PTE_R) == 0 &&
        (store_low_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (store_high_fault_leaf & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R) &&
        (store_high_fault_leaf & PTE_W) == 0 &&
        (guard_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    ai_memblock_pte_fault_write_fflags(initial_fflags);

    TEST_SETUP_EXCEPT();
    uint64_t frd_after_fault =
        ai_memblock_pte_fault_cross_fld_frd_probe(load_vaddr,
            fld_sentinel);
    TEST_ASSERT("cross-page HS fld whose second half has no read permission reports LPF at the high-page start and leaves frd plus fflags unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_second_vaddr &&
        frd_after_fault == fld_sentinel &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fsd_probe(store_vaddr, fsd_fault_payload);
    TEST_ASSERT("cross-page HS fsd whose second half has no write permission reports SPF at the high-page start and leaves fflags unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_second_vaddr &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    uint64_t guard_after_fault = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the independent guard leaf remains readable after the cross-page FP fault probes",
        guard_after_fault == guard_pattern);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("cross-page FP second-half faults preserve all participating PTE words before repair",
        *ai_memblock_pte_fault_hs_leaf_entry(load_low_page) ==
            load_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(load_high_page) ==
            load_high_fault_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(store_low_page) ==
            store_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(store_high_page) ==
            store_high_fault_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) == guard_leaf
    );
    TEST_ASSERT("cross-page FP load second-half fault preserves both load boundary images and adjacent sentinels before repair",
        read64(load_low_adjacent_paddr) == load_low_adjacent &&
        read64(load_low_paddr) == load_low_pattern &&
        read64(load_high_paddr) == load_high_pattern &&
        read64(load_high_adjacent_paddr) == load_high_adjacent
    );
    const uint64_t store_low_after_fault = read64(store_low_paddr);
    const uint64_t store_high_after_fault = read64(store_high_paddr);
    TEST_ASSERT("cross-page FP store second-half fault preserves store-side adjacent sentinels before repair",
        read64(store_low_adjacent_paddr) == store_low_adjacent &&
        read64(store_high_adjacent_paddr) == store_high_adjacent
    );
    TEST_ASSERT("cross-page FP store second-half fault preserves both split boundary images before repair",
        store_low_after_fault == store_low_pattern &&
        store_high_after_fault == store_high_pattern
    );
    TEST_ASSERT("cross-page FP second-half faults preserve guard data and fflags before repair",
        read64(guard_paddr) == guard_pattern &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags
    );

    *ai_memblock_pte_fault_hs_leaf_entry(load_high_page) =
        (phys_page_base(load_high_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(store_high_page) =
        (phys_page_base(store_high_page) >> 2) | legal_leaf;
    const uint64_t load_high_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_high_page);
    const uint64_t store_high_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_high_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    ai_memblock_pte_fault_write_fflags(initial_fflags);

    TEST_SETUP_EXCEPT();
    uint64_t fld_repaired =
        ai_memblock_pte_fault_cross_fld_frd_probe(load_vaddr,
            fld_sentinel);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the high load leaf, cross-page fld writes the merged doubleword and keeps fflags stable",
        fld_repaired == expected_fld_repaired &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fsd_probe(store_vaddr, fsd_repair_payload);
    fence_iorw_iorw();
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the high store leaf, cross-page fsd retires without stale second-half exception state",
        ai_memblock_pte_fault_read_fflags() == initial_fflags);

    TEST_SETUP_EXCEPT();
    uint64_t guard_after_repair = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the independent guard leaf remains readable after repaired cross-page FP load/store",
        guard_after_repair == guard_pattern);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("the repaired cross-page FP store updates exactly the two boundary word halves while preserving adjacent words, load pages, guard data, and fflags",
        *ai_memblock_pte_fault_hs_leaf_entry(load_low_page) ==
            load_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(load_high_page) ==
            load_high_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(store_low_page) ==
            store_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(store_high_page) ==
            store_high_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) == guard_leaf &&
        read64(load_low_adjacent_paddr) == load_low_adjacent &&
        read64(load_low_paddr) == load_low_pattern &&
        read64(load_high_paddr) == load_high_pattern &&
        read64(load_high_adjacent_paddr) == load_high_adjacent &&
        read64(store_low_adjacent_paddr) == store_low_adjacent &&
        read64(store_low_paddr) == expected_store_low_after &&
        read64(store_high_paddr) == expected_store_high_after &&
        read64(store_high_adjacent_paddr) == store_high_adjacent &&
        read64(guard_paddr) == guard_pattern &&
        ai_memblock_pte_fault_read_fflags() == initial_fflags
    );

    TEST_END();
}

bool ai_micro_hs_memblock_fp_memory_clean_fs_faults_no_dirty_until_commit_corner()
{
    TEST_START();

    const enum test_page load_page = VSR_GRWX;
    const enum test_page store_page = VSRW_GRWX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x180U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x180U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x280U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x280U;
    const uintptr_t marker_vaddr = hs_page_base(guard_page) + 0x380U;
    const uintptr_t marker_paddr = phys_page_base(guard_page) + 0x380U;
    const uint64_t load_pattern = 0x400921fb7fc12345ULL;
    const uint64_t store_initial = 0x8877665544332211ULL;
    const uint64_t marker_initial = 0x0badc0de55aa33ccULL;
    const uint64_t taken_marker = 0x13579bdf2468ace0ULL;
    const uint64_t poison_marker = 0xdeadbeefa55a5aa5ULL;
    const uint64_t ft2_sentinel = 0x7ff80000deadbabeULL;
    const uint64_t fsw_fault_bits = 0x000000007fc12345ULL;
    const uint64_t fsd_fault_bits = 0xfff8123456789abcULL;
    const uint64_t wrong_path_store_bits = 0x1122334455667788ULL;
    const uint64_t no_read_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t read_only_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t initial_fflags = 0x12ULL;
    const uint64_t initial_frm = 0x1ULL;
    const uint64_t initial_fcsr = (initial_frm << 5) | initial_fflags;
    const uint64_t expected_flw_boxed =
        0xffffffff00000000ULL | (load_pattern & 0xffffffffULL);
    const uint64_t expected_fsw_old_word =
        (store_initial >> 32) & 0xffffffffULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    write64(load_paddr, load_pattern);
    write64(store_paddr, store_initial);
    write64(marker_paddr, marker_initial);

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    *ai_memblock_pte_fault_hs_leaf_entry(load_page) =
        (phys_page_base(load_page) >> 2) | no_read_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(store_page) =
        (phys_page_base(store_page) >> 2) | read_only_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(guard_page) =
        (phys_page_base(guard_page) >> 2) | legal_leaf;
    const uint64_t load_fault_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_page);
    const uint64_t store_fault_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_page);
    const uint64_t guard_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the FP clean-FS fault setup starts from a no-read load leaf, read-only store leaf, legal marker leaf, and stable backing data",
        (load_fault_leaf & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (load_fault_leaf & PTE_R) == 0 &&
        (store_fault_leaf & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R) &&
        (store_fault_leaf & PTE_W) == 0 &&
        (guard_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        read64(load_paddr) == load_pattern &&
        read64(store_paddr) == store_initial &&
        read64(marker_paddr) == marker_initial
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    ai_memblock_pte_fault_set_fs_dirty();
    ai_memblock_pte_fault_write_fcsr(initial_fflags, initial_frm);
    ai_memblock_pte_fault_seed_ft2(ft2_sentinel);
    ai_memblock_pte_fault_set_fs_clean();

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_flw_only(load_vaddr);
    const uint64_t flw_fault_fs = ai_memblock_pte_fault_fs_state();
    TEST_ASSERT("Clean-FS faulting flw reports LPF and does not dirty FS before any FPR observer runs",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        flw_fault_fs == SSTATUS_FS_CLEAN &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr
    );
    ai_memblock_pte_fault_set_fs_dirty();
    const uint64_t ft2_after_flw_fault = ai_memblock_pte_fault_read_ft2();
    TEST_ASSERT("faulting flw from Clean FS leaves the destination FPR sentinel intact",
        ft2_after_flw_fault == ft2_sentinel);

    ai_memblock_pte_fault_seed_ft2(ft2_sentinel);
    ai_memblock_pte_fault_set_fs_clean();
    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fld_only(load_vaddr);
    const uint64_t fld_fault_fs = ai_memblock_pte_fault_fs_state();
    TEST_ASSERT("Clean-FS faulting fld reports LPF and does not dirty FS",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        fld_fault_fs == SSTATUS_FS_CLEAN &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr
    );
    ai_memblock_pte_fault_set_fs_dirty();
    const uint64_t ft2_after_fld_fault = ai_memblock_pte_fault_read_ft2();
    TEST_ASSERT("faulting fld from Clean FS leaves the destination FPR sentinel intact",
        ft2_after_fld_fault == ft2_sentinel);

    ai_memblock_pte_fault_seed_ft3(fsw_fault_bits);
    ai_memblock_pte_fault_set_fs_clean();
    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fsw_only(store_vaddr + sizeof(uint32_t));
    const uint64_t fsw_fault_fs = ai_memblock_pte_fault_fs_state();
    TEST_ASSERT("Clean-FS faulting fsw reports SPF and does not dirty FS",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr + sizeof(uint32_t) &&
        fsw_fault_fs == SSTATUS_FS_CLEAN &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr
    );
    TEST_SETUP_EXCEPT();
    uint64_t fsw_observed_word = lwu(store_vaddr + sizeof(uint32_t));
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("integer observer after faulting fsw still sees the old store word and keeps FS Clean",
        fsw_observed_word == expected_fsw_old_word &&
        ai_memblock_pte_fault_fs_state() == SSTATUS_FS_CLEAN);

    ai_memblock_pte_fault_set_fs_dirty();
    ai_memblock_pte_fault_seed_ft3(fsd_fault_bits);
    ai_memblock_pte_fault_set_fs_clean();
    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fsd_only(store_vaddr);
    const uint64_t fsd_fault_fs = ai_memblock_pte_fault_fs_state();
    TEST_ASSERT("Clean-FS faulting fsd reports SPF and does not dirty FS",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr &&
        fsd_fault_fs == SSTATUS_FS_CLEAN &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr
    );
    TEST_SETUP_EXCEPT();
    uint64_t fsd_observed_word = ld(store_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("integer observer after faulting fsd still sees the old doubleword image and keeps FS Clean",
        fsd_observed_word == store_initial &&
        ai_memblock_pte_fault_fs_state() == SSTATUS_FS_CLEAN);

    *ai_memblock_pte_fault_hs_leaf_entry(load_page) =
        (phys_page_base(load_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(store_page) =
        (phys_page_base(store_page) >> 2) | legal_leaf;
    const uint64_t load_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_page);
    const uint64_t store_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_page);
    fence_iorw_iorw();
    sfence_vma();

    ai_memblock_pte_fault_set_fs_dirty();
    ai_memblock_pte_fault_seed_ft2(ft2_sentinel);
    ai_memblock_pte_fault_seed_ft3(wrong_path_store_bits);
    ai_memblock_pte_fault_set_fs_clean();
    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_wrong_path_fp_mem_probe(load_vaddr, store_vaddr,
        marker_vaddr, taken_marker, poison_marker);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("taken branch kills wrong-path FP load/store and marker poison while leaving FS Clean",
        ai_memblock_pte_fault_fs_state() == SSTATUS_FS_CLEAN &&
        ld(marker_vaddr) == taken_marker &&
        ld(store_vaddr) == store_initial);
    ai_memblock_pte_fault_set_fs_dirty();
    const uint64_t ft2_after_wrong_path = ai_memblock_pte_fault_read_ft2();
    TEST_ASSERT("wrong-path FP load does not clobber the destination FPR sentinel",
        ft2_after_wrong_path == ft2_sentinel);

    ai_memblock_pte_fault_seed_ft2(ft2_sentinel);
    ai_memblock_pte_fault_set_fs_clean();
    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_flw_only(load_vaddr);
    const uint64_t flw_commit_fs = ai_memblock_pte_fault_fs_state();
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("repaired flw commits from Clean FS and dirties FS with no stale fault state",
        flw_commit_fs == SSTATUS_FS_DIRTY &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr);
    const uint64_t flw_committed_value = ai_memblock_pte_fault_read_ft2();
    TEST_ASSERT("repaired flw writes the expected NaN-boxed single-precision payload",
        flw_committed_value == expected_flw_boxed);

    ai_memblock_pte_fault_seed_ft2(ft2_sentinel);
    ai_memblock_pte_fault_set_fs_clean();
    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_fld_only(load_vaddr);
    const uint64_t fld_commit_fs = ai_memblock_pte_fault_fs_state();
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("repaired fld commits from Clean FS and dirties FS with no stale fault state",
        fld_commit_fs == SSTATUS_FS_DIRTY &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr);
    const uint64_t fld_committed_value = ai_memblock_pte_fault_read_ft2();
    TEST_ASSERT("repaired fld writes the complete doubleword payload",
        fld_committed_value == load_pattern);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("Clean-FS FP memory fault/churn probe leaves repaired PTEs, target data, marker, and FCSR in the expected final state",
        *ai_memblock_pte_fault_hs_leaf_entry(load_page) ==
            load_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(store_page) ==
            store_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) == guard_leaf &&
        read64(load_paddr) == load_pattern &&
        read64(store_paddr) == store_initial &&
        read64(marker_paddr) == taken_marker &&
        ai_memblock_pte_fault_read_fcsr() == initial_fcsr
    );

    TEST_END();
}

bool ai_micro_hs_memblock_faulting_amo_wd_do_not_clobber_rd_or_memory_before_repair_corner()
{
    TEST_START();

    const enum test_page target_page = VSR_GRWX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x580U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x580U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x680U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x680U;
    const uint64_t initial_value = 0x88776655c43382f1ULL;
    const uint64_t guard_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t fault_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t sentinel[] = {
        0xbad00000000000a1ULL,
        0xbad00000000000a2ULL,
        0xbad00000000000a3ULL,
        0xbad00000000000a4ULL,
    };
    const uint64_t operands[] = {
        0x0000000011223344ULL,
        0x0000000001010101ULL,
        0x0102030405060708ULL,
        0x0001000100010001ULL,
    };
    const uint64_t expected_w_old =
        (uint64_t)(int64_t)(int32_t)(uint32_t)initial_value;
    const uint64_t expected_after_swap_w =
        (initial_value & 0xffffffff00000000ULL) |
        (operands[0] & 0xffffffffULL);
    const uint64_t expected_after_add_w =
        (expected_after_swap_w & 0xffffffff00000000ULL) |
        (((uint32_t)expected_after_swap_w + (uint32_t)operands[1]) &
            0xffffffffULL);
    const uint64_t expected_add_w_old =
        (uint64_t)(int64_t)(int32_t)(uint32_t)expected_after_swap_w;
    const uint64_t expected_after_swap_d = operands[2];
    const uint64_t expected_after_add_d = operands[2] + operands[3];

    goto_priv(PRIV_M);
    write64(target_paddr, initial_value);
    write64(guard_paddr, guard_value);

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    *ai_memblock_pte_fault_hs_leaf_entry(target_page) =
        (phys_page_base(target_page) >> 2) | fault_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(guard_page) =
        (phys_page_base(guard_page) >> 2) | legal_leaf;
    const uint64_t fault_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    const uint64_t guard_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the faulting-AMO rd/memory probe starts with a read-only HS leaf and a legal guard leaf",
        (fault_leaf_word & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R) &&
        (fault_leaf_word & PTE_W) == 0 &&
        (guard_leaf_word & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    uint64_t preload = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the read-only AMO target leaf is still readable before the store-permission fault probes",
        preload == initial_value);

    AI_MEMBLOCK_CHECK_FAULTING_AMO_RD(
        "faulting HS amoswap.w on the read-only leaf reports SPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_amoswap_w_rd_probe, target_vaddr,
        sentinel[0], operands[0]);
    AI_MEMBLOCK_CHECK_FAULTING_AMO_RD(
        "faulting HS amoadd.w on the same read-only leaf reports SPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_amoadd_w_rd_probe, target_vaddr,
        sentinel[1], operands[1]);
    AI_MEMBLOCK_CHECK_FAULTING_AMO_RD(
        "faulting HS amoswap.d on the read-only leaf reports SPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_amoswap_d_rd_probe, target_vaddr,
        sentinel[2], operands[2]);
    AI_MEMBLOCK_CHECK_FAULTING_AMO_RD(
        "faulting HS amoadd.d on the same read-only leaf reports SPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_amoadd_d_rd_probe, target_vaddr,
        sentinel[3], operands[3]);

    goto_priv(PRIV_M);
    TEST_ASSERT("none of the denied AMO probes changes the target PTE, guard PTE, target data, or guard data",
        *ai_memblock_pte_fault_hs_leaf_entry(target_page) == fault_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) == guard_leaf_word &&
        read64(target_paddr) == initial_value &&
        read64(guard_paddr) == guard_value
    );

    *ai_memblock_pte_fault_hs_leaf_entry(target_page) =
        (phys_page_base(target_page) >> 2) | legal_leaf;
    const uint64_t repaired_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(target_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_swap_w = ai_memblock_pte_fault_amoswap_w_rd_probe(
        target_vaddr, 0ULL, operands[0]);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, amoswap.w returns the sign-extended old low word",
        old_swap_w == expected_w_old);

    TEST_SETUP_EXCEPT();
    uint64_t after_swap_w = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, amoswap.w updates only the low word lane",
        after_swap_w == expected_after_swap_w);

    TEST_SETUP_EXCEPT();
    uint64_t old_add_w = ai_memblock_pte_fault_amoadd_w_rd_probe(
        target_vaddr, 0ULL, operands[1]);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, amoadd.w returns the old low word left by amoswap.w",
        old_add_w == expected_add_w_old);

    TEST_SETUP_EXCEPT();
    uint64_t after_add_w = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, amoadd.w updates only the low word lane",
        after_add_w == expected_after_add_w);

    TEST_SETUP_EXCEPT();
    uint64_t old_swap_d = amoswap_d(target_vaddr, operands[2]);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, amoswap.d returns the full doubleword left by the word AMO sequence",
        old_swap_d == expected_after_add_w);

    TEST_SETUP_EXCEPT();
    uint64_t after_swap_d = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, amoswap.d replaces the full doubleword",
        after_swap_d == expected_after_swap_d);

    TEST_SETUP_EXCEPT();
    uint64_t old_add_d = amoadd_d(target_vaddr, operands[3]);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, amoadd.d returns the old full doubleword left by amoswap.d",
        old_add_d == expected_after_swap_d);

    TEST_SETUP_EXCEPT();
    uint64_t after_add_d = ld(target_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, amoadd.d updates the full doubleword",
        after_add_d == expected_after_add_d);

    TEST_SETUP_EXCEPT();
    uint64_t guard_after = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the independent guard leaf remains readable after the AMO fault/recovery sequence",
        guard_after == guard_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("the AMO rd/memory clobber probe ends with the repaired target leaf, stable guard leaf, final AMO data, and unchanged guard data",
        *ai_memblock_pte_fault_hs_leaf_entry(target_page) ==
            repaired_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(target_paddr) == expected_after_add_d &&
        read64(guard_paddr) == guard_value
    );

    TEST_END();
}

bool ai_micro_hs_memblock_faulting_lr_sc_do_not_clobber_rd_or_create_side_effects_corner()
{
    TEST_START();

    const enum test_page lr_page = VSR_GRWX;
    const enum test_page sc_page = SWITCH1;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t lr_vaddr = hs_page_base(lr_page) + 0x700U;
    const uintptr_t lr_paddr = phys_page_base(lr_page) + 0x700U;
    const uintptr_t sc_word_vaddr = hs_page_base(sc_page) + 0x780U;
    const uintptr_t sc_word_paddr = phys_page_base(sc_page) + 0x780U;
    const uintptr_t sc_dword_vaddr = hs_page_base(sc_page) + 0x788U;
    const uintptr_t sc_dword_paddr = phys_page_base(sc_page) + 0x788U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x880U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x880U;
    const uint64_t lr_initial = 0x88776655c43382f1ULL;
    const uint64_t sc_word_initial = 0xdeadbeef81234567ULL;
    const uint64_t sc_dword_initial = 0x0123456789abcdefULL;
    const uint64_t guard_initial = 0x1029384756aabbccULL;
    const uint64_t fault_lr_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t fault_sc_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t sentinel_lr_w = 0xbad10000000000a1ULL;
    const uint64_t sentinel_lr_d = 0xbad10000000000a2ULL;
    const uint64_t sentinel_sc_w = 0xbad10000000000a3ULL;
    const uint64_t sentinel_sc_d = 0xbad10000000000a4ULL;
    const uint64_t sentinel_lr_aq = 0xbad10000000000a5ULL;
    const uint64_t sentinel_lr_aqrl = 0xbad10000000000a6ULL;
    const uint64_t sentinel_sc_rl = 0xbad10000000000a7ULL;
    const uint64_t sentinel_sc_aqrl = 0xbad10000000000a8ULL;
    const uint64_t stale_sc_w_payload = 0x0000000011223344ULL;
    const uint64_t stale_sc_d_payload = 0x1122334455667788ULL;
    const uint64_t stale_sc_rl_payload = 0xaabbccddeeff0011ULL;
    const uint64_t lr_sc_payload = 0x99aabbccddeeff00ULL;
    const uint64_t sc_word_payload = 0x0000000010203040ULL;
    const uint64_t sc_dword_payload = 0xfeedfacecafebeefULL;
    const uint64_t sc_release_payload = 0x13579bdf2468ace0ULL;
    const uint64_t expected_lr_w =
        (uint64_t)(int64_t)(int32_t)(uint32_t)lr_initial;
    const uint64_t expected_sc_word_after =
        (sc_word_initial & 0xffffffff00000000ULL) |
        (sc_word_payload & 0xffffffffULL);

    goto_priv(PRIV_M);
    write64(lr_paddr, lr_initial);
    write64(sc_word_paddr, sc_word_initial);
    write64(sc_dword_paddr, sc_dword_initial);
    write64(guard_paddr, guard_initial);

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    *ai_memblock_pte_fault_hs_leaf_entry(lr_page) =
        (phys_page_base(lr_page) >> 2) | fault_lr_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(sc_page) =
        (phys_page_base(sc_page) >> 2) | fault_sc_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(guard_page) =
        (phys_page_base(guard_page) >> 2) | legal_leaf;
    const uint64_t fault_lr_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(lr_page);
    const uint64_t fault_sc_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(sc_page);
    const uint64_t guard_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the LR/SC fault-side-effect probe starts with a no-read LR leaf, a read-only SC leaf, and a legal guard leaf",
        (fault_lr_leaf_word & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (fault_lr_leaf_word & PTE_R) == 0 &&
        (fault_sc_leaf_word & (PTE_V | PTE_AD | PTE_R)) ==
            (PTE_V | PTE_AD | PTE_R) &&
        (fault_sc_leaf_word & PTE_W) == 0 &&
        (guard_leaf_word & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_lrsc_result_t guard_pair =
        ai_memblock_pte_fault_lr_sc_d_pair(guard_vaddr, guard_initial);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("a legal guard lr.d/sc.d pair establishes and consumes a known LR/SC baseline before the fault probes",
        guard_pair.old_value == guard_initial &&
        guard_pair.status == 0ULL);

    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lr.w on the no-read leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_lr_w_rd_probe, lr_vaddr, sentinel_lr_w);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lr.d on the same no-read leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_lr_d_rd_probe, lr_vaddr, sentinel_lr_d);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lr.d.aq on the no-read leaf reports LPF/tval, leaves rd at its sentinel, and creates no acquire reservation",
        ai_memblock_pte_fault_lr_d_aq_rd_probe, lr_vaddr,
        sentinel_lr_aq);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "faulting HS lr.d.aqrl on the no-read leaf reports LPF/tval, leaves rd at its sentinel, and creates no aqrl reservation",
        ai_memblock_pte_fault_lr_d_aqrl_rd_probe, lr_vaddr,
        sentinel_lr_aqrl);

    AI_MEMBLOCK_CHECK_FAULTING_SC_RD(
        "faulting HS sc.w on the read-only leaf reports SPF/tval and leaves the status rd at its sentinel",
        ai_memblock_pte_fault_sc_w_rd_probe, sc_word_vaddr,
        sentinel_sc_w, sc_word_payload);
    AI_MEMBLOCK_CHECK_FAULTING_SC_RD(
        "faulting HS sc.d on the read-only leaf reports SPF/tval and leaves the status rd at its sentinel",
        ai_memblock_pte_fault_sc_d_rd_probe, sc_dword_vaddr,
        sentinel_sc_d, sc_dword_payload);
    AI_MEMBLOCK_CHECK_FAULTING_SC_RD(
        "faulting HS sc.d.rl on the read-only leaf reports SPF/tval, leaves the status rd at its sentinel, and creates no release store",
        ai_memblock_pte_fault_sc_d_rl_rd_probe, sc_dword_vaddr,
        sentinel_sc_rl, sc_release_payload);
    AI_MEMBLOCK_CHECK_FAULTING_SC_RD(
        "faulting HS sc.d.aqrl on the read-only leaf reports SPF/tval, leaves the status rd at its sentinel, and creates no acquire/release side effect",
        ai_memblock_pte_fault_sc_d_aqrl_rd_probe, sc_dword_vaddr,
        sentinel_sc_aqrl, sc_release_payload + 1ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("faulting LR/SC probes do not change PTEs or any target/guard backing data",
        *ai_memblock_pte_fault_hs_leaf_entry(lr_page) ==
            fault_lr_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(sc_page) ==
            fault_sc_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(lr_paddr) == lr_initial &&
        read64(sc_word_paddr) == sc_word_initial &&
        read64(sc_dword_paddr) == sc_dword_initial &&
        read64(guard_paddr) == guard_initial
    );

    *ai_memblock_pte_fault_hs_leaf_entry(lr_page) =
        (phys_page_base(lr_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(sc_page) =
        (phys_page_base(sc_page) >> 2) | legal_leaf;
    const uint64_t repaired_lr_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(lr_page);
    const uint64_t repaired_sc_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(sc_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t stale_sc_w_status = ai_memblock_pte_fault_sc_w_rd_probe(
        lr_vaddr, 0ULL, stale_sc_w_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the LR leaf, a stale sc.w without a fresh lr.w still fails and proves the faulting lr.w did not create a word reservation",
        stale_sc_w_status != 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t after_stale_sc_w = ld(lr_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the stale sc.w after faulting LR does not update the repaired LR target before a fresh word reservation exists",
        after_stale_sc_w == lr_initial);

    TEST_SETUP_EXCEPT();
    uint64_t stale_sc_status = ai_memblock_pte_fault_sc_d_rd_probe(
        lr_vaddr, 0ULL, stale_sc_d_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the LR leaf, a stale sc.d without a fresh lr.d still fails and proves the faulting lr.d did not create a reservation",
        stale_sc_status != 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t after_stale_sc = ld(lr_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the stale sc.d after faulting LR does not update the repaired LR target before a fresh reservation exists",
        after_stale_sc == lr_initial);

    TEST_SETUP_EXCEPT();
    uint64_t stale_sc_rl_status = ai_memblock_pte_fault_sc_d_rl_rd_probe(
        lr_vaddr, 0ULL, stale_sc_rl_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the LR leaf, stale sc.d.rl still fails and proves faulting LR aq/aqrl did not create a release-capable reservation",
        stale_sc_rl_status != 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t after_stale_sc_rl = ld(lr_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the stale sc.d.rl failure does not publish a release-store payload before a fresh LR exists",
        after_stale_sc_rl == lr_initial);

    TEST_SETUP_EXCEPT();
    uint64_t lr_w_value = ai_memblock_pte_fault_lr_w_rd_probe(
        lr_vaddr, 0ULL);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, lr.w returns the sign-extended low word from the target",
        lr_w_value == expected_lr_w);

    TEST_SETUP_EXCEPT();
    uint64_t lr_d_value = ai_memblock_pte_fault_lr_d_rd_probe(
        lr_vaddr, 0ULL);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, lr.d returns the full target doubleword and refreshes the reservation",
        lr_d_value == lr_initial);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_lrsc_result_t lr_pair =
        ai_memblock_pte_fault_lr_sc_d_pair(lr_vaddr, lr_sc_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after a fresh repaired lr.d/sc.d pair, sc.d succeeds and updates the LR target",
        lr_pair.old_value == lr_initial &&
        lr_pair.status == 0ULL);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_lrsc_result_t release_pair =
        ai_memblock_pte_fault_lr_aq_sc_rl_d_pair(lr_vaddr,
            sc_release_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after a fresh repaired lr.d.aq/sc.d.rl pair, the release SC succeeds and is distinguished from the earlier fault/fail paths",
        release_pair.old_value == lr_sc_payload &&
        release_pair.status == 0ULL);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_lrsc_result_t sc_word_pair =
        ai_memblock_pte_fault_lr_sc_w_pair(sc_word_vaddr, sc_word_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, lr.w/sc.w succeeds and updates only the low word lane",
        sc_word_pair.old_value ==
            (uint64_t)(int64_t)(int32_t)(uint32_t)sc_word_initial &&
        sc_word_pair.status == 0ULL);

    TEST_SETUP_EXCEPT();
    ai_memblock_pte_fault_lrsc_result_t sc_dword_pair =
        ai_memblock_pte_fault_lr_sc_d_pair(sc_dword_vaddr,
            sc_dword_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, lr.d/sc.d succeeds and updates the full doubleword",
        sc_dword_pair.old_value == sc_dword_initial &&
        sc_dword_pair.status == 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t guard_after = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the independent guard leaf remains readable after the LR/SC fault and repair sequence",
        guard_after == guard_initial);

    goto_priv(PRIV_M);
    TEST_ASSERT("the LR/SC fault probe ends with repaired leaves, expected LR/SC updates, and unchanged guard data",
        *ai_memblock_pte_fault_hs_leaf_entry(lr_page) ==
            repaired_lr_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(sc_page) ==
            repaired_sc_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(lr_paddr) == sc_release_payload &&
        read64(sc_word_paddr) == expected_sc_word_after &&
        read64(sc_dword_paddr) == sc_dword_payload &&
        read64(guard_paddr) == guard_initial
    );

    TEST_END();
}

bool ai_micro_hs_memblock_ad_faults_do_not_clobber_rd_pte_or_memory_before_repair_corner()
{
    TEST_START();

    const enum test_page load_a_page = SWITCH1;
    const enum test_page store_a_page = SWITCH2;
    const enum test_page fetch_a_page = VSRWX_GRW;
    const enum test_page amo_a_page = VSRWX_GRX;
    const enum test_page store_d_page = VSRW_GRWX;
    const enum test_page amo_d_page = VSRWX_GRWX;
    const enum test_page sum_a_page = VSR_GURWX;
    const enum test_page mxr_a_page = VSX_GX;
    const enum test_page write_d_perm_page = VSR_GRWX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t load_a_vaddr = hs_page_base(load_a_page) + 0x120U;
    const uintptr_t load_a_paddr = phys_page_base(load_a_page) + 0x120U;
    const uintptr_t store_a_vaddr = hs_page_base(store_a_page) + 0x240U;
    const uintptr_t store_a_paddr = phys_page_base(store_a_page) + 0x240U;
    const uintptr_t fetch_a_vaddr = hs_page_base(fetch_a_page) + 0x280U;
    const uintptr_t fetch_a_paddr = phys_page_base(fetch_a_page) + 0x280U;
    const uintptr_t amo_a_vaddr = hs_page_base(amo_a_page) + 0x2c0U;
    const uintptr_t amo_a_paddr = phys_page_base(amo_a_page) + 0x2c0U;
    const uintptr_t store_d_vaddr = hs_page_base(store_d_page) + 0x360U;
    const uintptr_t store_d_paddr = phys_page_base(store_d_page) + 0x360U;
    const uintptr_t amo_d_vaddr = hs_page_base(amo_d_page) + 0x480U;
    const uintptr_t amo_d_paddr = phys_page_base(amo_d_page) + 0x480U;
    const uintptr_t sum_a_load_vaddr = hs_page_base(sum_a_page) + 0x4a0U;
    const uintptr_t sum_a_store_vaddr = hs_page_base(sum_a_page) + 0x4a8U;
    const uintptr_t sum_a_paddr = phys_page_base(sum_a_page) + 0x4a0U;
    const uintptr_t sum_a_store_paddr =
        phys_page_base(sum_a_page) + 0x4a8U;
    const uintptr_t mxr_a_vaddr = hs_page_base(mxr_a_page) + 0x4c0U;
    const uintptr_t mxr_a_paddr = phys_page_base(mxr_a_page) + 0x4c0U;
    const uintptr_t write_d_perm_vaddr =
        hs_page_base(write_d_perm_page) + 0x4e0U;
    const uintptr_t write_d_perm_paddr =
        phys_page_base(write_d_perm_page) + 0x4e0U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x5a0U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x5a0U;
    const uint64_t load_initial = 0x1111222233334444ULL;
    const uint64_t store_a_initial = 0x5555666677778888ULL;
    const uint32_t fetch_a_instruction = 0x00008067U;
    const uint64_t amo_a_initial = 0x13572468abcdef00ULL;
    const uint64_t store_d_initial = 0x9999aaaabbbbccccULL;
    const uint64_t amo_initial = 0x0123456789abcdefULL;
    const uint64_t sum_a_initial = 0x5555aaaacccc3333ULL;
    const uint64_t sum_a_store_initial = 0x13579bdf2468ace0ULL;
    const uint64_t mxr_a_initial = 0x0badf00d12345678ULL;
    const uint64_t write_d_perm_initial = 0x8877aa55cc33ee11ULL;
    const uint64_t guard_initial = 0xfedcba9876543210ULL;
    const uint64_t store_a_payload = 0x0a0b0c0d0e0f1011ULL;
    const uint64_t store_d_payload = 0x2122232425262728ULL;
    const uint64_t sum_a_store_payload = 0x5a5a00000000ad0dULL;
    const uint64_t write_d_perm_payload = 0xd1d0000000000badULL;
    const uint64_t amo_a_operand = 0x0000000000000101ULL;
    const uint64_t amo_operand = 0x0001000100010001ULL;
    const uint64_t load_sentinel = 0xadf00000000000a1ULL;
    const uint64_t amo_a_sentinel = 0xadf00000000000a3ULL;
    const uint64_t amo_sentinel = 0xadf00000000000a2ULL;
    const uint64_t sum_a_sentinel = 0xadf00000000000b1ULL;
    const uint64_t mxr_a_sentinel = 0xadf00000000000b2ULL;
    const uint64_t a0_load_leaf = PTE_V | PTE_D | PTE_R;
    const uint64_t a0_store_leaf = PTE_V | PTE_D | PTE_RW;
    const uint64_t a0_fetch_leaf = PTE_V | PTE_D | PTE_X;
    const uint64_t a0_amo_leaf = PTE_V | PTE_D | PTE_RWX;
    const uint64_t d0_store_leaf = PTE_V | PTE_A | PTE_RWX;
    const uint64_t sum_a_leaf = PTE_V | PTE_U | PTE_D | PTE_RWX;
    const uint64_t mxr_a_leaf = PTE_V | PTE_D | PTE_X;
    const uint64_t d0_no_write_leaf = PTE_V | PTE_A | PTE_R;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    write64(load_a_paddr, load_initial);
    write64(store_a_paddr, store_a_initial);
    write32(fetch_a_paddr, fetch_a_instruction);
    write64(amo_a_paddr, amo_a_initial);
    write64(store_d_paddr, store_d_initial);
    write64(amo_d_paddr, amo_initial);
    write64(sum_a_paddr, sum_a_initial);
    write64(sum_a_store_paddr, sum_a_store_initial);
    write64(mxr_a_paddr, mxr_a_initial);
    write64(write_d_perm_paddr, write_d_perm_initial);
    write64(guard_paddr, guard_initial);
    fence_iorw_iorw();
    ai_memblock_pte_fault_fence_i();

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    *ai_memblock_pte_fault_hs_leaf_entry(load_a_page) =
        (phys_page_base(load_a_page) >> 2) | a0_load_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(store_a_page) =
        (phys_page_base(store_a_page) >> 2) | a0_store_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(fetch_a_page) =
        (phys_page_base(fetch_a_page) >> 2) | a0_fetch_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(amo_a_page) =
        (phys_page_base(amo_a_page) >> 2) | a0_amo_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(store_d_page) =
        (phys_page_base(store_d_page) >> 2) | d0_store_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(amo_d_page) =
        (phys_page_base(amo_d_page) >> 2) | d0_store_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(sum_a_page) =
        (phys_page_base(sum_a_page) >> 2) | sum_a_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(mxr_a_page) =
        (phys_page_base(mxr_a_page) >> 2) | mxr_a_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(write_d_perm_page) =
        (phys_page_base(write_d_perm_page) >> 2) | d0_no_write_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(guard_page) =
        (phys_page_base(guard_page) >> 2) | legal_leaf;
    const uint64_t load_a_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(load_a_page);
    const uint64_t store_a_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(store_a_page);
    const uint64_t fetch_a_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(fetch_a_page);
    const uint64_t amo_a_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(amo_a_page);
    const uint64_t store_d_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(store_d_page);
    const uint64_t amo_d_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(amo_d_page);
    const uint64_t sum_a_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(sum_a_page);
    const uint64_t mxr_a_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_a_page);
    const uint64_t write_d_perm_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(write_d_perm_page);
    const uint64_t guard_leaf_word =
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the A/D fault probe installs A=0 load/store/fetch/AMO leaves, A=0 plus SUM/MXR/PTE.U denial leaves, D=0 store/AMO leaves, a D=0 plus W=0 leaf, and a legal guard leaf",
        (load_a_leaf_word & (PTE_V | PTE_D | PTE_R)) ==
            (PTE_V | PTE_D | PTE_R) &&
        (load_a_leaf_word & PTE_A) == 0 &&
        (store_a_leaf_word & (PTE_V | PTE_D | PTE_RW)) ==
            (PTE_V | PTE_D | PTE_RW) &&
        (store_a_leaf_word & PTE_A) == 0 &&
        (fetch_a_leaf_word & (PTE_V | PTE_D | PTE_X)) ==
            (PTE_V | PTE_D | PTE_X) &&
        (fetch_a_leaf_word & PTE_A) == 0 &&
        (amo_a_leaf_word & (PTE_V | PTE_D | PTE_RWX)) ==
            (PTE_V | PTE_D | PTE_RWX) &&
        (amo_a_leaf_word & PTE_A) == 0 &&
        (store_d_leaf_word & (PTE_V | PTE_A | PTE_RWX)) ==
            (PTE_V | PTE_A | PTE_RWX) &&
        (store_d_leaf_word & PTE_D) == 0 &&
        (amo_d_leaf_word & (PTE_V | PTE_A | PTE_RWX)) ==
            (PTE_V | PTE_A | PTE_RWX) &&
        (amo_d_leaf_word & PTE_D) == 0 &&
        (sum_a_leaf_word & (PTE_V | PTE_U | PTE_D | PTE_RWX)) ==
            (PTE_V | PTE_U | PTE_D | PTE_RWX) &&
        (sum_a_leaf_word & PTE_A) == 0 &&
        (mxr_a_leaf_word & (PTE_V | PTE_D | PTE_X)) ==
            (PTE_V | PTE_D | PTE_X) &&
        (mxr_a_leaf_word & (PTE_A | PTE_R | PTE_W | PTE_U)) == 0 &&
        (write_d_perm_leaf_word & (PTE_V | PTE_A | PTE_R)) ==
            (PTE_V | PTE_A | PTE_R) &&
        (write_d_perm_leaf_word & (PTE_D | PTE_W)) == 0 &&
        (guard_leaf_word & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "HS ld on an A=0 leaf reports LPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_ld_rd_probe, load_a_vaddr, load_sentinel);

    TEST_SETUP_EXCEPT();
    sd(store_a_vaddr, store_a_payload);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS sd on an A=0 leaf reports SPF/tval before any store side effect",
        CAUSE_SPF, store_a_vaddr);

    ai_memblock_pte_fault_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(fetch_a_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS fetch on an A=0 executable leaf reports IPF/tval and does not let the fetch retire",
        CAUSE_IPF, fetch_a_vaddr);

    AI_MEMBLOCK_CHECK_FAULTING_AMO_RD(
        "HS amoadd.d on an A=0 leaf reports SPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_amoadd_d_rd_probe, amo_a_vaddr,
        amo_a_sentinel, amo_a_operand);

    TEST_SETUP_EXCEPT();
    uint64_t d0_load_before_store = ld(store_d_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("HS ld on a D=0 leaf succeeds and proves the dirty bit is not checked for read-only access",
        d0_load_before_store == store_d_initial);

    TEST_SETUP_EXCEPT();
    sd(store_d_vaddr, store_d_payload);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS sd on a D=0 leaf reports SPF/tval and must not dirty memory",
        CAUSE_SPF, store_d_vaddr);

    AI_MEMBLOCK_CHECK_FAULTING_AMO_RD(
        "HS amoadd.d on a D=0 leaf reports SPF/tval and leaves rd at its sentinel",
        ai_memblock_pte_fault_amoadd_d_rd_probe, amo_d_vaddr,
        amo_sentinel, amo_operand);

    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "HS ld on a PTE.U=1 A=0 leaf with SUM=0 reports LPF/tval before any A-bit update and leaves rd at its sentinel",
        ai_memblock_pte_fault_ld_rd_probe, sum_a_load_vaddr,
        sum_a_sentinel);

    TEST_SETUP_EXCEPT();
    sd(sum_a_store_vaddr, sum_a_store_payload);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS sd on the same PTE.U=1 A=0 leaf with SUM=0 reports SPF/tval before any write side effect",
        CAUSE_SPF, sum_a_store_vaddr);

    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "HS ld on an X-only A=0 leaf with MXR=0 reports LPF/tval from the combined no-read and A=0 state and leaves rd at its sentinel",
        ai_memblock_pte_fault_ld_rd_probe, mxr_a_vaddr, mxr_a_sentinel);

    CSRS(CSR_SSTATUS, SSTATUS_MXR);
    AI_MEMBLOCK_CHECK_FAULTING_LOAD_RD(
        "HS ld on the same X-only A=0 leaf still reports LPF/tval with MXR=1, proving the A=0 condition remains a fault after read permission is relaxed",
        ai_memblock_pte_fault_ld_rd_probe, mxr_a_vaddr,
        mxr_a_sentinel + 1ULL);
    CSRC(CSR_SSTATUS, SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    sd(write_d_perm_vaddr, write_d_perm_payload);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("HS sd on a D=0 leaf that also lacks W permission reports SPF/tval and has no partial write",
        CAUSE_SPF, write_d_perm_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("A/D page faults do not set A/D bits, rewrite PTEs, update memory, or disturb the guard page",
        *ai_memblock_pte_fault_hs_leaf_entry(load_a_page) ==
            load_a_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(store_a_page) ==
            store_a_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(fetch_a_page) ==
            fetch_a_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(amo_a_page) ==
            amo_a_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(store_d_page) ==
            store_d_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(amo_d_page) ==
            amo_d_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(sum_a_page) ==
            sum_a_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_a_page) ==
            mxr_a_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(write_d_perm_page) ==
            write_d_perm_leaf_word &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(load_a_paddr) == load_initial &&
        read64(store_a_paddr) == store_a_initial &&
        read32(fetch_a_paddr) == fetch_a_instruction &&
        read64(amo_a_paddr) == amo_a_initial &&
        read64(store_d_paddr) == store_d_initial &&
        read64(amo_d_paddr) == amo_initial &&
        read64(sum_a_paddr) == sum_a_initial &&
        read64(sum_a_store_paddr) == sum_a_store_initial &&
        read64(mxr_a_paddr) == mxr_a_initial &&
        read64(write_d_perm_paddr) == write_d_perm_initial &&
        read64(guard_paddr) == guard_initial
    );

    *ai_memblock_pte_fault_hs_leaf_entry(load_a_page) =
        (phys_page_base(load_a_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(store_a_page) =
        (phys_page_base(store_a_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(fetch_a_page) =
        (phys_page_base(fetch_a_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(amo_a_page) =
        (phys_page_base(amo_a_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(store_d_page) =
        (phys_page_base(store_d_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(amo_d_page) =
        (phys_page_base(amo_d_page) >> 2) | legal_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(sum_a_page) =
        (phys_page_base(sum_a_page) >> 2) | legal_leaf | PTE_U;
    *ai_memblock_pte_fault_hs_leaf_entry(mxr_a_page) =
        (phys_page_base(mxr_a_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    *ai_memblock_pte_fault_hs_leaf_entry(write_d_perm_page) =
        (phys_page_base(write_d_perm_page) >> 2) | legal_leaf;
    const uint64_t load_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_a_page);
    const uint64_t store_a_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_a_page);
    const uint64_t fetch_a_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(fetch_a_page);
    const uint64_t amo_a_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(amo_a_page);
    const uint64_t store_d_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_d_page);
    const uint64_t amo_d_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(amo_d_page);
    const uint64_t sum_a_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(sum_a_page);
    const uint64_t mxr_a_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_a_page);
    const uint64_t write_d_perm_repaired_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(write_d_perm_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_after_repair = ld(load_a_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after software sets A/D and executes SFENCE.VMA, the earlier A=0 load VA reads its original payload",
        load_after_repair == load_initial);

    TEST_SETUP_EXCEPT();
    sd(store_a_vaddr, store_a_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, the earlier A=0 store VA retires without stale page-fault state",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t amo_a_old = ai_memblock_pte_fault_amoadd_d_rd_probe(
        amo_a_vaddr, 0ULL, amo_a_operand);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, the earlier A=0 AMO VA returns the old doubleword and updates memory",
        amo_a_old == amo_a_initial);

    TEST_SETUP_EXCEPT();
    sd(store_d_vaddr, store_d_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, the earlier D=0 store VA retires and updates memory",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = ai_memblock_pte_fault_amoadd_d_rd_probe(
        amo_d_vaddr, 0ULL, amo_operand);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repair, amoadd.d returns the old doubleword and clears the earlier D=0 fault state",
        amo_old == amo_initial);

    CSRS(CSR_SSTATUS, SSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    uint64_t sum_a_after_repair = ld(sum_a_load_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after software sets A and executes SFENCE.VMA, SUM=1 lets the repaired PTE.U=1 leaf load succeed",
        sum_a_after_repair == sum_a_initial);

    TEST_SETUP_EXCEPT();
    sd(sum_a_store_vaddr, sum_a_store_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after software sets A and executes SFENCE.VMA, SUM=1 lets the repaired PTE.U=1 leaf store succeed",
        true);
    CSRC(CSR_SSTATUS, SSTATUS_SUM);

    CSRS(CSR_SSTATUS, SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t mxr_a_after_repair = ld(mxr_a_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after software sets A and executes SFENCE.VMA, MXR=1 can read the repaired execute-only leaf",
        mxr_a_after_repair == mxr_a_initial);
    CSRC(CSR_SSTATUS, SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    sd(write_d_perm_vaddr, write_d_perm_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after software repairs D and W and executes SFENCE.VMA, the earlier D=0 plus no-W store VA retires",
        true);

    TEST_SETUP_EXCEPT();
    uint64_t guard_after = ld(guard_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("the independent guard leaf remains readable after the A/D fault and repair sequence",
        guard_after == guard_initial);

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired A/D sequence updates only the intended store and AMO targets while leaves and guard data remain stable",
        *ai_memblock_pte_fault_hs_leaf_entry(load_a_page) ==
            load_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(store_a_page) ==
            store_a_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(fetch_a_page) ==
            fetch_a_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(amo_a_page) ==
            amo_a_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(store_d_page) ==
            store_d_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(amo_d_page) ==
            amo_d_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(sum_a_page) ==
            sum_a_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_a_page) ==
            mxr_a_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(write_d_perm_page) ==
            write_d_perm_repaired_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(guard_page) ==
            guard_leaf_word &&
        read64(load_a_paddr) == load_initial &&
        read64(store_a_paddr) == store_a_payload &&
        read32(fetch_a_paddr) == fetch_a_instruction &&
        read64(amo_a_paddr) == (amo_a_initial + amo_a_operand) &&
        read64(store_d_paddr) == store_d_payload &&
        read64(amo_d_paddr) == (amo_initial + amo_operand) &&
        read64(sum_a_paddr) == sum_a_initial &&
        read64(sum_a_store_paddr) == sum_a_store_payload &&
        read64(mxr_a_paddr) == mxr_a_initial &&
        read64(write_d_perm_paddr) == write_d_perm_payload &&
        read64(guard_paddr) == guard_initial
    );

    TEST_END();
}

bool ai_micro_memblock_cross_page_low_half_pf_no_side_effect_corner()
{
    TEST_START();

    const enum test_page load_low_page = SWITCH1;
    const enum test_page load_high_page = SWITCH2;
    const enum test_page store_low_page = VSRW_GX;
    const enum test_page store_high_page = VSRX_GRWX;
    const enum test_page fetch_low_page = VSRWX_GRWX;
    const enum test_page fetch_high_page = VSRWX_GRW;
    const uintptr_t load_vaddr =
        ai_memblock_pte_fault_cross_page_addr(load_low_page);
    const uintptr_t store_vaddr =
        ai_memblock_pte_fault_cross_page_addr(store_low_page);
    const uintptr_t fetch_vaddr =
        ai_memblock_pte_fault_cross_page_addr(fetch_low_page);
    const uintptr_t load_low_phys =
        ai_memblock_pte_fault_page_end_phys(load_low_page);
    const uintptr_t load_high_phys = phys_page_base(load_high_page);
    const uintptr_t store_low_phys =
        ai_memblock_pte_fault_page_end_phys(store_low_page);
    const uintptr_t store_high_phys = phys_page_base(store_high_page);
    const uintptr_t fetch_low_phys =
        phys_page_base(fetch_low_page) + PAGE_SIZE - sizeof(uint32_t);
    const uintptr_t fetch_high_phys = phys_page_base(fetch_high_page);
    const uint64_t load_low_pattern = 0x1020304050607080ULL;
    const uint64_t load_high_pattern = 0x8877665544332211ULL;
    const uint64_t store_low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t store_high_pattern = 0x99aabbccddeeff00ULL;
    const uint8_t fetch_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint64_t rd_sentinel = 0xa55a123456789abcULL;
    uint64_t rd_after_fault = 0;

    goto_priv(PRIV_M);
    write64(load_low_phys, load_low_pattern);
    write64(load_high_phys, load_high_pattern);
    write64(store_low_phys, store_low_pattern);
    write64(store_high_phys, store_high_pattern);
    ai_memblock_pte_fault_write_bytes(fetch_low_phys, fetch_pattern,
        sizeof(fetch_pattern));
    fence_iorw_iorw();
    ai_memblock_pte_fault_fence_i();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_memblock_pte_fault_hs_leaf_entry(load_low_page) =
        (phys_page_base(load_low_page) >> 2) | PTE_V | PTE_A | PTE_X;
    *ai_memblock_pte_fault_hs_leaf_entry(load_high_page) =
        (phys_page_base(load_high_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    *ai_memblock_pte_fault_hs_leaf_entry(store_low_page) =
        (phys_page_base(store_low_page) >> 2) | PTE_V | PTE_A | PTE_R;
    *ai_memblock_pte_fault_hs_leaf_entry(store_high_page) =
        (phys_page_base(store_high_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    *ai_memblock_pte_fault_hs_leaf_entry(fetch_low_page) =
        (phys_page_base(fetch_low_page) >> 2) | PTE_V | PTE_AD | PTE_RW;
    *ai_memblock_pte_fault_hs_leaf_entry(fetch_high_page) =
        (phys_page_base(fetch_high_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    const uint64_t load_low_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_low_page);
    const uint64_t load_high_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_high_page);
    const uint64_t store_low_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_low_page);
    const uint64_t store_high_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(store_high_page);
    const uint64_t fetch_low_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(fetch_low_page);
    const uint64_t fetch_high_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(fetch_high_page);
    sfence_vma();
    TEST_ASSERT("the cross-page low-half PF setup denies the first translated half while keeping each second half fully legal",
        (load_low_leaf & (PTE_V | PTE_A | PTE_X)) ==
            (PTE_V | PTE_A | PTE_X) &&
        (load_low_leaf & PTE_R) == 0 &&
        (load_high_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (store_low_leaf & (PTE_V | PTE_A | PTE_R)) ==
            (PTE_V | PTE_A | PTE_R) &&
        (store_low_leaf & PTE_W) == 0 &&
        (store_high_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (fetch_low_leaf & (PTE_V | PTE_AD | PTE_RW)) ==
            (PTE_V | PTE_AD | PTE_RW) &&
        (fetch_low_leaf & PTE_X) == 0 &&
        (fetch_high_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    rd_after_fault = ai_memblock_pte_fault_ld_rd_probe(load_vaddr,
        rd_sentinel);
    TEST_ASSERT("a split load whose first half has no read permission reports load page fault at the original low-half VA and does not write rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        rd_after_fault == rd_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, 0x0123456789abcdefULL);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("a split store whose first half has no write permission reports store page fault at the original low-half VA",
        CAUSE_SPF, store_vaddr);

    ai_memblock_pte_fault_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(fetch_vaddr);
    AI_MEMBLOCK_ASSERT_PAGE_FAULT("a split fetch whose first half lacks execute permission reports instruction page fault at the original low-half VA",
        CAUSE_IPF, fetch_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("low-half cross-page page faults leave all participating PTE words and boundary data unchanged",
        ai_memblock_pte_fault_check_leaf_unchanged(load_low_page,
            load_low_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(load_high_page,
            load_high_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(store_low_page,
            store_low_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(store_high_page,
            store_high_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(fetch_low_page,
            fetch_low_leaf) &&
        ai_memblock_pte_fault_check_leaf_unchanged(fetch_high_page,
            fetch_high_leaf) &&
        read64(load_low_phys) == load_low_pattern &&
        read64(load_high_phys) == load_high_pattern &&
        read64(store_low_phys) == store_low_pattern &&
        read64(store_high_phys) == store_high_pattern &&
        ai_memblock_pte_fault_bytes_match(fetch_low_phys, fetch_pattern,
            sizeof(fetch_pattern))
    );

    TEST_END();
}

bool ai_micro_hs_memblock_cross_page_privilege_load_fault_half_select_no_side_effect_corner()
{
    TEST_START();

    const enum test_page load_low_page = SWITCH1;
    const enum test_page load_high_page = SWITCH2;
    const enum test_page mxr_low_page = VSRW_GRWX;
    const enum test_page mxr_high_page = VSRW_GRW;
    const enum test_page mxr_first_low_page = VSRX_GX;
    const enum test_page mxr_first_high_page = VSR_GRWX;
    const uintptr_t load_vaddr =
        ai_memblock_pte_fault_cross_page_addr(load_low_page);
    const uintptr_t load_fault_vaddr = hs_page_base(load_high_page);
    const uintptr_t mxr_vaddr =
        ai_memblock_pte_fault_cross_page_addr(mxr_low_page);
    const uintptr_t mxr_fault_vaddr = hs_page_base(mxr_high_page);
    const uintptr_t mxr_first_vaddr =
        ai_memblock_pte_fault_cross_page_addr(mxr_first_low_page);
    const uintptr_t load_low_phys =
        ai_memblock_pte_fault_page_end_phys(load_low_page);
    const uintptr_t load_high_phys = phys_page_base(load_high_page);
    const uintptr_t mxr_low_phys =
        ai_memblock_pte_fault_page_end_phys(mxr_low_page);
    const uintptr_t mxr_high_phys = phys_page_base(mxr_high_page);
    const uintptr_t mxr_first_low_phys =
        ai_memblock_pte_fault_page_end_phys(mxr_first_low_page);
    const uintptr_t mxr_first_high_phys =
        phys_page_base(mxr_first_high_page);
    const uint64_t load_low_pattern = 0x1021324354657687ULL;
    const uint64_t load_high_pattern = 0x8877665544332211ULL;
    const uint64_t mxr_low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t mxr_high_pattern = 0x7766554433221100ULL;
    const uint64_t mxr_first_low_pattern = 0x3141592653589793ULL;
    const uint64_t mxr_first_high_pattern = 0x2718281828459045ULL;
    const uint64_t expected_mxr_load =
        ((mxr_low_pattern >> 32) & 0xffffffffULL) |
        ((mxr_high_pattern & 0xffffffffULL) << 32);
    const uint64_t expected_mxr_first_load =
        ((mxr_first_low_pattern >> 32) & 0xffffffffULL) |
        ((mxr_first_high_pattern & 0xffffffffULL) << 32);
    const uint64_t load_sentinel = 0xcbad0000000000a1ULL;
    const uint64_t mxr_sentinel = 0xcbad0000000000a2ULL;
    const uint64_t mxr_first_sentinel = 0xcbad0000000000a3ULL;

    goto_priv(PRIV_M);
    write64(load_low_phys, load_low_pattern);
    write64(load_high_phys, load_high_pattern);
    write64(mxr_low_phys, mxr_low_pattern);
    write64(mxr_high_phys, mxr_high_pattern);
    write64(mxr_first_low_phys, mxr_first_low_pattern);
    write64(mxr_first_high_phys, mxr_first_high_pattern);
    fence_iorw_iorw();

    ai_memblock_pte_fault_prepare_hs();

    goto_priv(PRIV_M);
    *ai_memblock_pte_fault_hs_leaf_entry(load_low_page) =
        (phys_page_base(load_low_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    *ai_memblock_pte_fault_hs_leaf_entry(load_high_page) =
        (phys_page_base(load_high_page) >> 2) |
        PTE_V | PTE_U | PTE_AD | PTE_RWX;
    *ai_memblock_pte_fault_hs_leaf_entry(mxr_low_page) =
        (phys_page_base(mxr_low_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    *ai_memblock_pte_fault_hs_leaf_entry(mxr_high_page) =
        (phys_page_base(mxr_high_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    *ai_memblock_pte_fault_hs_leaf_entry(mxr_first_low_page) =
        (phys_page_base(mxr_first_low_page) >> 2) |
        PTE_V | PTE_AD | PTE_X;
    *ai_memblock_pte_fault_hs_leaf_entry(mxr_first_high_page) =
        (phys_page_base(mxr_first_high_page) >> 2) |
        PTE_V | PTE_AD | PTE_RWX;
    const uint64_t load_low_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_low_page);
    const uint64_t load_high_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(load_high_page);
    const uint64_t mxr_low_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_low_page);
    const uint64_t mxr_high_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_high_page);
    const uint64_t mxr_first_low_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_first_low_page);
    const uint64_t mxr_first_high_leaf =
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_first_high_page);
    sfence_vma();
    TEST_ASSERT("the cross-page privilege-load setup keeps each low half legal while the selected high half is denied by SUM or MXR",
        (load_low_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (load_high_leaf & (PTE_V | PTE_U | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_U | PTE_AD | PTE_RWX) &&
        (mxr_low_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX) &&
        (mxr_high_leaf & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (mxr_high_leaf & PTE_R) == 0 &&
        (mxr_first_low_leaf & (PTE_V | PTE_AD | PTE_X)) ==
            (PTE_V | PTE_AD | PTE_X) &&
        (mxr_first_low_leaf & PTE_R) == 0 &&
        (mxr_first_high_leaf & (PTE_V | PTE_AD | PTE_RWX)) ==
            (PTE_V | PTE_AD | PTE_RWX)
    );

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    TEST_SETUP_EXCEPT();
    uint64_t load_rd_after_fault =
        ai_memblock_pte_fault_ld_rd_probe(load_vaddr, load_sentinel);
    TEST_ASSERT("a cross-page HS load with a legal low half and PTE.U=1 high half reports LPF at the high-half VA with SUM=0 and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_fault_vaddr &&
        load_rd_after_fault == load_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t mxr_rd_after_fault =
        ai_memblock_pte_fault_ld_rd_probe(mxr_vaddr, mxr_sentinel);
    TEST_ASSERT("a cross-page HS load with a legal low half and X-only high half reports LPF at the high-half VA while MXR=0 and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == mxr_fault_vaddr &&
        mxr_rd_after_fault == mxr_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t mxr_first_rd_after_fault =
        ai_memblock_pte_fault_ld_rd_probe(mxr_first_vaddr,
            mxr_first_sentinel);
    TEST_ASSERT("a cross-page HS load whose first half is X-only reports LPF at the original low-half VA while MXR=0 and leaves rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == mxr_first_vaddr &&
        mxr_first_rd_after_fault == mxr_first_sentinel
    );

    CSRS(CSR_SSTATUS, SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    uint64_t mxr_repaired_load = ld(mxr_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("setting MXR=1 lets the same cross-page load read through the X-only high half without changing the PTE",
        mxr_repaired_load == expected_mxr_load);

    TEST_SETUP_EXCEPT();
    uint64_t mxr_first_repaired_load = ld(mxr_first_vaddr);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("setting MXR=1 also lets a cross-page load continue past an X-only first half into a legal high half",
        mxr_first_repaired_load == expected_mxr_first_load);
    CSRC(CSR_SSTATUS, SSTATUS_MXR);

    goto_priv(PRIV_M);
    TEST_ASSERT("cross-page privilege load faults leave all participating PTE words unchanged",
        *ai_memblock_pte_fault_hs_leaf_entry(load_low_page) ==
            load_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(load_high_page) ==
            load_high_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_low_page) ==
            mxr_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_high_page) ==
            mxr_high_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_first_low_page) ==
            mxr_first_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(mxr_first_high_page) ==
            mxr_first_high_leaf
    );

    TEST_ASSERT("cross-page privilege load faults leave SUM-denied and MXR-denied low/high backing halves unchanged",
        read64(load_low_phys) == load_low_pattern &&
        read64(load_high_phys) == load_high_pattern &&
        read64(mxr_low_phys) == mxr_low_pattern &&
        read64(mxr_high_phys) == mxr_high_pattern &&
        read64(mxr_first_low_phys) == mxr_first_low_pattern &&
        read64(mxr_first_high_phys) == mxr_first_high_pattern
    );

    TEST_END();
}

bool ai_micro_memblock_cross_page_store_privilege_xonly_high_half_no_partial_write_matrix_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t store_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - sizeof(uint32_t);
    const uintptr_t high_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys =
        phys_page_base(low_page) + PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_guard_phys = low_phys - sizeof(uint64_t);
    const uintptr_t high_guard_phys = high_phys + sizeof(uint64_t);
    const uintptr_t m_direct_store_addr = low_phys + sizeof(uint32_t);
    const uint64_t low_initial = 0x1021324354657687ULL;
    const uint64_t high_initial = 0x8877665544332211ULL;
    const uint64_t low_guard = 0x0badf00d11223344ULL;
    const uint64_t high_guard = 0x55667788cafebeefULL;
    const uint64_t m_payload = 0xa1a2a3a4b1b2b3b4ULL;
    const uint64_t hs_payload = 0xc1c2c3c4d1d2d3d4ULL;
    const uint64_t hu_payload = 0xe1e2e3e4f1f2f3f4ULL;
    const uint64_t expected_m_low =
        (low_initial & 0x00000000ffffffffULL) |
        ((m_payload & 0xffffffffULL) << 32);
    const uint64_t expected_m_high =
        (high_initial & 0xffffffff00000000ULL) |
        ((m_payload >> 32) & 0xffffffffULL);
    const uint64_t expected_hu_low =
        (low_initial & 0x00000000ffffffffULL) |
        ((hu_payload & 0xffffffffULL) << 32);
    const uint64_t expected_hu_high =
        (high_initial & 0xffffffff00000000ULL) |
        ((hu_payload >> 32) & 0xffffffffULL);
    const uint64_t supervisor_low_leaf =
        (phys_page_base(low_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    const uint64_t supervisor_xonly_high_leaf =
        (phys_page_base(high_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    const uint64_t user_low_leaf =
        supervisor_low_leaf | PTE_U;
    const uint64_t user_xonly_high_leaf =
        supervisor_xonly_high_leaf | PTE_U;
    const uint64_t user_repaired_high_leaf =
        (phys_page_base(high_page) >> 2) |
        PTE_V | PTE_U | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(low_guard_phys, low_guard);
    write64(low_phys, low_initial);
    write64(high_phys, high_initial);
    write64(high_guard_phys, high_guard);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_memblock_pte_fault_hs_leaf_entry(low_page) =
        supervisor_low_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(high_page) =
        supervisor_xonly_high_leaf;
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(m_direct_store_addr, m_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("M-mode direct physical split store ignores the HS PTE.X-only high-half mapping and commits across the page boundary",
        read64(low_phys) == expected_m_low &&
        read64(high_phys) == expected_m_high &&
        read64(low_guard_phys) == low_guard &&
        read64(high_guard_phys) == high_guard);

    write64(low_phys, low_initial);
    write64(high_phys, high_initial);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    TEST_SETUP_EXCEPT();
    sd(store_vaddr, hs_payload);
    TEST_ASSERT("HS split store with a legal supervisor low half and X-only high half reports SPF at the high-half VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the HS high-half X-only store fault does not partially update either participating doubleword or adjacent guard",
        *ai_memblock_pte_fault_hs_leaf_entry(low_page) ==
            supervisor_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(high_page) ==
            supervisor_xonly_high_leaf &&
        read64(low_guard_phys) == low_guard &&
        read64(low_phys) == low_initial &&
        read64(high_phys) == high_initial &&
        read64(high_guard_phys) == high_guard);

    write64(low_phys, low_initial);
    write64(high_phys, high_initial);
    hspt_u_mode_allow_no_leaf();
    *ai_memblock_pte_fault_hs_leaf_entry(low_page) = user_low_leaf;
    *ai_memblock_pte_fault_hs_leaf_entry(high_page) =
        user_xonly_high_leaf;
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    sd(store_vaddr, hu_payload);
    TEST_ASSERT("HU split store with a legal user low half and X-only user high half also reports SPF at the high-half VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_half_vaddr);

    goto_priv_u_to_m_direct(PRIV_M);
    TEST_ASSERT("the HU high-half X-only store fault leaves the same boundary footprint and PTE words unchanged",
        *ai_memblock_pte_fault_hs_leaf_entry(low_page) == user_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(high_page) ==
            user_xonly_high_leaf &&
        read64(low_guard_phys) == low_guard &&
        read64(low_phys) == low_initial &&
        read64(high_phys) == high_initial &&
        read64(high_guard_phys) == high_guard);

    *ai_memblock_pte_fault_hs_leaf_entry(high_page) =
        user_repaired_high_leaf;
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    sd(store_vaddr, hu_payload);
    AI_MEMBLOCK_ASSERT_NO_FAULT_AND("after repairing the user high half to writable, the same HU split store commits normally",
        true);

    goto_priv_u_to_m_direct(PRIV_M);
    TEST_ASSERT("the repaired HU split store updates exactly the two participating doublewords while preserving adjacent guards and final PTEs",
        *ai_memblock_pte_fault_hs_leaf_entry(low_page) == user_low_leaf &&
        *ai_memblock_pte_fault_hs_leaf_entry(high_page) ==
            user_repaired_high_leaf &&
        read64(low_guard_phys) == low_guard &&
        read64(low_phys) == expected_hu_low &&
        read64(high_phys) == expected_hu_high &&
        read64(high_guard_phys) == high_guard);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
