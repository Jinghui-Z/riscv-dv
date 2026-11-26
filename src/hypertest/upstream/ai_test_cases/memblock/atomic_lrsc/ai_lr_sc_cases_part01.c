#include "ai_lr_sc_common.h"

#define AI_ARCH_LR_SC_FAULT_PAGE        VSRWX_GRWX
#define AI_ARCH_LR_SC_FAULT_OFFSET      0x280U
#define AI_ARCH_LR_SC_LEAF_LEGAL        (PTE_V | PTE_A | PTE_D | PTE_RWX)
#define AI_ARCH_LR_SC_LEAF_NO_R         (PTE_V | PTE_A | PTE_D | PTE_W)
#define AI_ARCH_LR_SC_LEAF_NO_W         (PTE_V | PTE_A | PTE_D | PTE_R)
#define AI_ARCH_LR_SC_PMP_TOR_RWX       0x0fULL
#define AI_ARCH_LR_SC_PMP_TOR_NONE      0x08ULL
#define AI_ARCH_LR_SC_PBMT_IO_BASE      0x30000000000ULL
#define AI_ARCH_LR_SC_PBMT_IO_LIMIT     0x40000000000ULL

typedef struct {
    uintptr_t pc;
    uintptr_t next_pc;
} ai_arch_lr_sc_fault_pc_t;

static inline uint64_t ai_arch_lr_sc_leaf_value(enum test_page page,
    uint64_t flags)
{
    return (phys_page_base(page) >> 2) | flags;
}

static inline void ai_arch_lr_sc_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_lr_sc_hs_leaf_entry(page) = ai_arch_lr_sc_leaf_value(page, flags);
    goto_priv(PRIV_HS);
    sfence_vma();
}

static inline void ai_arch_lr_sc_set_hs_leaf_paddr(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_lr_sc_hs_leaf_entry(page) = ((paddr & PAGE_ADDR_MSK) >> 2) | flags;
    goto_priv(PRIV_HS);
    sfence_vma();
}

static inline void ai_arch_lr_sc_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_ARCH_LR_SC_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_arch_lr_sc_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_ARCH_LR_SC_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_ARCH_LR_SC_PMP_TOR_RWX << 16));
}

static inline void ai_arch_lr_sc_prepare_io_window(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, AI_ARCH_LR_SC_PBMT_IO_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_ARCH_LR_SC_PBMT_IO_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    fence_iorw_iorw();
}

static inline void ai_arch_lr_sc_reset_attr_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    ai_arch_lr_sc_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_arch_lr_sc_lr_w_xlen(uintptr_t addr)
{
    uint64_t value;
    asm volatile(
        "lr.w %0, 0(%1)\n\t"
        : "=r"(value) : "r"(addr) : "memory"
    );
    return value;
}

static inline uint64_t ai_arch_lr_sc_sc_w_x0_zero(uintptr_t addr, uint32_t value)
{
    uint64_t zero_after;
    uintptr_t operand = (uintptr_t)value;

    asm volatile(
        "sc.w x0, %[operand], 0(%[addr])\n\t"
        "mv %[zero_after], x0\n\t"
        : [zero_after] "=r"(zero_after)
        : [addr] "r"(addr), [operand] "r"(operand)
        : "memory"
    );
    return zero_after;
}

static inline uint64_t ai_arch_lr_sc_sc_d_x0_zero(uintptr_t addr, uint64_t value)
{
    uint64_t zero_after;

    asm volatile(
        "sc.d x0, %[value], 0(%[addr])\n\t"
        "mv %[zero_after], x0\n\t"
        : [zero_after] "=r"(zero_after)
        : [addr] "r"(addr), [value] "r"(value)
        : "memory"
    );
    return zero_after;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_lr_sc_lr_w_probe_rd(uintptr_t addr, uint64_t sentinel,
    ai_arch_lr_sc_fault_pc_t *pc_info)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "lr.w %0, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%2)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%2)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(pc_info)
        : "t0", "memory");
    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_lr_sc_sc_w_probe_status(uintptr_t addr, uint32_t value,
    uint64_t sentinel, ai_arch_lr_sc_fault_pc_t *pc_info)
{
    uint64_t status = sentinel;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "sc.w %0, %2, 0(%1)\n\t"
        "2:\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(%3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(%3)\n\t"
        ".option pop\n\t"
        : "+r"(status)
        : "r"(addr), "r"(operand), "r"(pc_info)
        : "t0", "memory");
    return status;
}

bool ai_arch_lr_sc_fault_suppresses_reservation_and_status_side_effects_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_LR_SC_FAULT_PAGE;
    const uintptr_t vaddr = hs_page_base(page) + AI_ARCH_LR_SC_FAULT_OFFSET;
    const uintptr_t misaligned_vaddr = vaddr + 2U;
    const uintptr_t paddr = phys_page_base(page) + AI_ARCH_LR_SC_FAULT_OFFSET;
    const uintptr_t page_paddr = paddr & PAGE_ADDR_MSK;
    const uint64_t before_guard = 0x1021324354657687ULL;
    const uint64_t initial = 0x8899aabb11223344ULL;
    const uint64_t after_guard = 0xf0e1d2c3b4a59687ULL;
    const uint32_t initial_low = (uint32_t)initial;
    const uint32_t sc_fault_payload = 0x55667788U;
    const uint32_t fresh_payload = 0xa5a55a5aU;
    const uint64_t lr_sentinel0 = 0xface000000001001ULL;
    const uint64_t lr_sentinel1 = 0xface000000001002ULL;
    const uint64_t lr_sentinel2 = 0xface000000001003ULL;
    const uint64_t sc_sentinel0 = 0xface000000002001ULL;
    const uint64_t sc_sentinel1 = 0xface000000002002ULL;
    const uint64_t sc_sentinel2 = 0xface000000002003ULL;
    const uint64_t sc_sentinel3 = 0xface000000002004ULL;
    ai_arch_lr_sc_fault_pc_t lr_misaligned_pc = {0};
    ai_arch_lr_sc_fault_pc_t sc_misaligned_pc = {0};
    ai_arch_lr_sc_fault_pc_t lr_page_fault_pc = {0};
    ai_arch_lr_sc_fault_pc_t stale_sc_pc = {0};
    ai_arch_lr_sc_fault_pc_t sc_page_fault_pc = {0};
    ai_arch_lr_sc_fault_pc_t lr_access_fault_pc = {0};
    ai_arch_lr_sc_fault_pc_t sc_access_fault_pc = {0};

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM | MSTATUS_MPRV | MSTATUS_SUM |
        MSTATUS_MXR);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    ai_arch_lr_sc_restore_pmp_full_access();
    write64(paddr - sizeof(uint64_t), before_guard);
    write64(paddr, initial);
    write64(paddr + sizeof(uint64_t), after_guard);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_lr_sc_set_hs_leaf(page, AI_ARCH_LR_SC_LEAF_LEGAL);

    TEST_SETUP_EXCEPT();
    uint64_t misaligned_lr_rd = ai_arch_lr_sc_lr_w_probe_rd(
        misaligned_vaddr, lr_sentinel0, &lr_misaligned_pc);
    TEST_ASSERT("misaligned lr.w reports load access fault at the raw target VA and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == misaligned_vaddr &&
        excpt.epc == lr_misaligned_pc.pc &&
        lr_misaligned_pc.next_pc == lr_misaligned_pc.pc + 4U &&
        misaligned_lr_rd == lr_sentinel0
    );

    TEST_SETUP_EXCEPT();
    uint64_t misaligned_sc_status = ai_arch_lr_sc_sc_w_probe_status(
        misaligned_vaddr, sc_fault_payload, sc_sentinel0, &sc_misaligned_pc);
    TEST_ASSERT("misaligned sc.w reports store access fault before producing a failure code or touching memory",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr &&
        excpt.epc == sc_misaligned_pc.pc &&
        sc_misaligned_pc.next_pc == sc_misaligned_pc.pc + 4U &&
        misaligned_sc_status == sc_sentinel0
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the LR/SC misalignment probes leave the aligned target and guard doublewords unchanged",
        read64(paddr - sizeof(uint64_t)) == before_guard &&
        read64(paddr) == initial &&
        read64(paddr + sizeof(uint64_t)) == after_guard
    );

    ai_arch_lr_sc_set_hs_leaf(page, AI_ARCH_LR_SC_LEAF_NO_R);
    TEST_SETUP_EXCEPT();
    uint64_t fault_lr_rd = ai_arch_lr_sc_lr_w_probe_rd(vaddr,
        lr_sentinel1, &lr_page_fault_pc);
    TEST_ASSERT("faulting lr.w reports load page fault at the target VA and does not write rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        excpt.epc == lr_page_fault_pc.pc &&
        lr_page_fault_pc.next_pc == lr_page_fault_pc.pc + 4U &&
        fault_lr_rd == lr_sentinel1
    );

    ai_arch_lr_sc_set_hs_leaf(page, AI_ARCH_LR_SC_LEAF_LEGAL);
    TEST_SETUP_EXCEPT();
    uint64_t stale_sc_status = ai_arch_lr_sc_sc_w_probe_status(vaddr,
        sc_fault_payload, sc_sentinel1, &stale_sc_pc);
    AI_ASSERT_NO_EXCEPT_AND("after repairing the page, sc.w without a fresh lr.w returns normal nonzero failure because the faulting lr.w did not create a reservation",
        stale_sc_status != 0U &&
        stale_sc_status != sc_sentinel1
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-fault stale sc.w failure leaves the target and adjacent guard words unchanged",
        read64(paddr - sizeof(uint64_t)) == before_guard &&
        read64(paddr) == initial &&
        read64(paddr + sizeof(uint64_t)) == after_guard
    );

    ai_arch_lr_sc_set_hs_leaf(page, AI_ARCH_LR_SC_LEAF_NO_W);
    TEST_SETUP_EXCEPT();
    uint64_t fault_sc_status = ai_arch_lr_sc_sc_w_probe_status(vaddr,
        sc_fault_payload, sc_sentinel2, &sc_page_fault_pc);
    TEST_ASSERT("faulting sc.w reports store page fault before writing a failure code to rd or storing data",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr &&
        excpt.epc == sc_page_fault_pc.pc &&
        sc_page_fault_pc.next_pc == sc_page_fault_pc.pc + 4U &&
        fault_sc_status == sc_sentinel2
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting sc.w page-fault path leaves target and guards unchanged",
        read64(paddr - sizeof(uint64_t)) == before_guard &&
        read64(paddr) == initial &&
        read64(paddr + sizeof(uint64_t)) == after_guard
    );

    ai_arch_lr_sc_set_hs_leaf(page, AI_ARCH_LR_SC_LEAF_LEGAL);
    ai_arch_lr_sc_configure_pmp_window(page_paddr, page_paddr + PAGE_SIZE,
        AI_ARCH_LR_SC_PMP_TOR_NONE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t access_fault_lr_rd = ai_arch_lr_sc_lr_w_probe_rd(vaddr,
        lr_sentinel2, &lr_access_fault_pc);
    TEST_ASSERT("PMP-denied lr.w reports load access fault at the translated target VA and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr &&
        excpt.epc == lr_access_fault_pc.pc &&
        lr_access_fault_pc.next_pc == lr_access_fault_pc.pc + 4U &&
        access_fault_lr_rd == lr_sentinel2
    );

    TEST_SETUP_EXCEPT();
    uint64_t access_fault_sc_status = ai_arch_lr_sc_sc_w_probe_status(vaddr,
        sc_fault_payload, sc_sentinel3, &sc_access_fault_pc);
    TEST_ASSERT("PMP-denied sc.w reports store access fault before writing a failure code or storing data",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.epc == sc_access_fault_pc.pc &&
        sc_access_fault_pc.next_pc == sc_access_fault_pc.pc + 4U &&
        access_fault_sc_status == sc_sentinel3
    );

    ai_arch_lr_sc_restore_pmp_full_access();
    goto_priv(PRIV_M);
    TEST_ASSERT("the LR/SC access-fault probes leave the target and guards unchanged before the final repair sequence",
        read64(paddr - sizeof(uint64_t)) == before_guard &&
        read64(paddr) == initial &&
        read64(paddr + sizeof(uint64_t)) == after_guard
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t fresh_lr = lr_w(vaddr);
    uint32_t fresh_sc_status = sc_w(vaddr, fresh_payload);
    AI_ASSERT_NO_EXCEPT_AND("after repairing permissions and PMP state, a fresh lr.w/sc.w pair returns zero and writes the target word",
        fresh_lr == initial_low &&
        fresh_sc_status == 0U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired fresh LR/SC sequence updates only the low target word and preserves both adjacent guards",
        read64(paddr - sizeof(uint64_t)) == before_guard &&
        read64(paddr) == ((initial & 0xffffffff00000000ULL) |
            (uint64_t)fresh_payload) &&
        read64(paddr + sizeof(uint64_t)) == after_guard
    );
    ai_arch_lr_sc_restore_pmp_full_access();
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_lr_sc_pbmt_attr_fault_suppresses_side_effects_corner()
{
    TEST_START();

    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page io_page = VSRWXPbmt2_GURWXPbmt1;
    const enum test_page device_page = VSRWX_GRWX;
    const uintptr_t nc_paddr = phys_page_base(SWITCH1) + 0x100U;
    const uintptr_t io_paddr = phys_page_base_offset(
        SWITCH2, AI_ARCH_LR_SC_PBMT_IO_BASE) + 0x100U;
    const uintptr_t device_paddr = phys_page_base_offset(
        SCRATCHPAD, AI_ARCH_LR_SC_PBMT_IO_BASE) + 0x100U;
    const uintptr_t nc_vaddr = hs_page_base(nc_page) + 0x100U;
    const uintptr_t io_vaddr = hs_page_base(io_page) + 0x100U;
    const uintptr_t device_vaddr = hs_page_base(device_page) + 0x100U;
    const uintptr_t cacheable_vaddr = hs_page_base(X) + 0x100U;
    const uintptr_t cacheable_paddr = phys_page_base(X) + 0x100U;
    const uint64_t nc_initial = 0x1122334455667788ULL;
    const uint64_t io_initial = 0x2233445566778899ULL;
    const uint64_t device_initial = 0x33445566778899aaULL;
    const uint64_t cacheable_initial = 0x445566778899aabbULL;
    const uint64_t cacheable_update = 0x5566778899aabbccULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    ai_arch_lr_sc_restore_pmp_full_access();
    ai_arch_lr_sc_prepare_io_window();
    hspt_init();
    ai_arch_lr_sc_set_hs_leaf_paddr(nc_page, nc_paddr,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_lr_sc_set_hs_leaf_paddr(io_page, io_paddr,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt2);
    ai_arch_lr_sc_set_hs_leaf_paddr(device_page, device_paddr,
        PTE_V | PTE_AD | PTE_RWX);
    ai_arch_lr_sc_set_hs_leaf_paddr(X, cacheable_paddr,
        PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_M);
    write64(nc_paddr, nc_initial);
    write64(io_paddr, io_initial);
    write64(device_paddr, device_initial);
    write64(cacheable_paddr, cacheable_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    lr_d(nc_vaddr);
    TEST_ASSERT("lr.d on a PBMT=NC alias reports load access fault and cannot create a reservation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == nc_vaddr);

    TEST_SETUP_EXCEPT();
    uint32_t nc_sc_status = sc_d(nc_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("sc.d on the same PBMT=NC alias reports store access fault before writing status or memory",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == nc_vaddr &&
        nc_sc_status != 0U);

    TEST_SETUP_EXCEPT();
    lr_d(io_vaddr);
    TEST_ASSERT("lr.d on a PBMT=IO alias reports load access fault and cannot create a reservation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == io_vaddr);

    TEST_SETUP_EXCEPT();
    uint32_t io_sc_status = sc_d(io_vaddr, 0xb1b2b3b4b5b6b7b8ULL);
    TEST_ASSERT("sc.d on a PBMT=IO alias reports store access fault before writing status or memory",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == io_vaddr &&
        io_sc_status != 0U);

    TEST_SETUP_EXCEPT();
    lr_d(device_vaddr);
    TEST_ASSERT("lr.d on a PMA=Device alias reports load access fault and cannot create a reservation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == device_vaddr);

    TEST_SETUP_EXCEPT();
    uint32_t device_sc_status = sc_d(device_vaddr, 0xc1c2c3c4c5c6c7c8ULL);
    TEST_ASSERT("sc.d on a PMA=Device alias reports store access fault before writing status or memory",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == device_vaddr &&
        device_sc_status != 0U);

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=NC/PBMT=IO/PMA=Device LR/SC attribute faults leave every target backing doubleword unchanged",
        read64(nc_paddr) == nc_initial &&
        read64(io_paddr) == io_initial &&
        read64(device_paddr) == device_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t stale_lr = ai_lr_sc_lr_d64(cacheable_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a cacheable lr.d establishes the reservation source value before the attribute-switch SC probe",
        stale_lr == cacheable_initial);

    goto_priv(PRIV_M);
    ai_arch_lr_sc_set_hs_leaf_paddr(X, cacheable_paddr,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t stale_sc_status = ai_lr_sc_sc_d_status(cacheable_vaddr,
        cacheable_update);
    TEST_ASSERT("an old cacheable reservation cannot make sc.d succeed after the same VA is rejudged as PBMT=NC",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == cacheable_vaddr &&
        stale_sc_status != 0U);

    goto_priv(PRIV_M);
    TEST_ASSERT("the attribute-switched SC fault leaves the reserved cacheable target unchanged",
        read64(cacheable_paddr) == cacheable_initial);
    ai_arch_lr_sc_set_hs_leaf_paddr(X, cacheable_paddr,
        PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t cacheable_lr = ai_lr_sc_lr_d64(cacheable_vaddr);
    uint32_t cacheable_sc_status = ai_lr_sc_sc_d_status(cacheable_vaddr,
        cacheable_update);
    AI_ASSERT_NO_EXCEPT_AND("after LR/SC attribute faults, a fresh cacheable lr.d/sc.d pair succeeds and is not poisoned by stale reservation state",
        cacheable_lr == cacheable_initial &&
        cacheable_sc_status == 0U);

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired ordinary LR/SC path updates only the cacheable target after the attribute-fault matrix",
        read64(cacheable_paddr) == cacheable_update &&
        read64(nc_paddr) == nc_initial &&
        read64(io_paddr) == io_initial &&
        read64(device_paddr) == device_initial);

    ai_arch_lr_sc_reset_attr_env();

    TEST_END();
}

bool ai_arch_lr_sc_success_status_guard_x0_consumption_corner()
{
    TEST_START();

    const uintptr_t word_addr = ai_lr_sc_addr(0);
    const uint32_t word_initial = 0x81234567U;
    const uint32_t word_guard = 0x55667788U;
    const uint32_t word_commit = 0xa5a55a5aU;
    const uint32_t word_refresh = 0x13579bdfU;
    const uint32_t word_x0_commit = 0x2468ace0U;
    const uint32_t word_fail_payload = 0xdeadbeefU;
    const uint64_t word_lr_expected =
        (uint64_t)(int64_t)(int32_t)word_initial;

    ai_lr_sc_prepare_words(word_initial, word_guard);

    TEST_SETUP_EXCEPT();
    uint64_t word_reserved = ai_arch_lr_sc_lr_w_xlen(word_addr);
    uint32_t word_status = sc_w(word_addr, word_commit);
    TEST_ASSERT("lr.w returns the sign-extended original word and sc.w returns zero on the fresh reservation",
        excpt.triggered == false &&
        word_reserved == word_lr_expected &&
        word_status == 0U
    );
    TEST_ASSERT("successful sc.w writes exactly the reserved word and preserves the adjacent word guard",
        ai_lr_sc_words[0] == word_commit &&
        ai_lr_sc_words[1] == word_guard
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_word_status = sc_w(word_addr, word_fail_payload);
    AI_ASSERT_NO_EXCEPT_AND("a second sc.w without a fresh lr.w fails after the successful sc.w consumed the reservation",
        second_word_status != 0U &&
        ai_lr_sc_words[0] == word_commit &&
        ai_lr_sc_words[1] == word_guard
    );

    TEST_SETUP_EXCEPT();
    uint32_t word_refresh_reserved = lr_w(word_addr);
    uint32_t word_refresh_status = sc_w(word_addr, word_refresh);
    AI_ASSERT_NO_EXCEPT_AND("a fresh lr.w/sc.w pair after the consumed-reservation failure succeeds and updates the word once",
        word_refresh_reserved == word_commit &&
        word_refresh_status == 0U &&
        ai_lr_sc_words[0] == word_refresh &&
        ai_lr_sc_words[1] == word_guard
    );

    TEST_SETUP_EXCEPT();
    uint32_t word_x0_reserved = lr_w(word_addr);
    uint64_t word_x0_after = ai_arch_lr_sc_sc_w_x0_zero(word_addr,
        word_x0_commit);
    AI_ASSERT_NO_EXCEPT_AND("sc.w with rd=x0 discards the zero success code while still committing the reserved word update",
        word_x0_reserved == word_refresh &&
        word_x0_after == 0U &&
        ai_lr_sc_words[0] == word_x0_commit &&
        ai_lr_sc_words[1] == word_guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t word_x0_fail_after = ai_arch_lr_sc_sc_w_x0_zero(word_addr,
        word_fail_payload);
    AI_ASSERT_NO_EXCEPT_AND("a failing sc.w with rd=x0 still leaves x0 as zero and performs no memory write",
        word_x0_fail_after == 0U &&
        ai_lr_sc_words[0] == word_x0_commit &&
        ai_lr_sc_words[1] == word_guard
    );

    TEST_SETUP_EXCEPT();
    uint32_t word_cold_status = sc_w(word_addr, word_fail_payload);
    AI_ASSERT_NO_EXCEPT_AND("a cold sc.w with no fresh lr.w returns nonzero and preserves the target and guard words",
        word_cold_status != 0U &&
        ai_lr_sc_words[0] == word_x0_commit &&
        ai_lr_sc_words[1] == word_guard
    );

    const uintptr_t dword_addr = ai_lr_sc_addr(0);
    const uint64_t dword_initial = 0x0123456789abcdefULL;
    const uint64_t dword_guard = 0xfedcba9876543210ULL;
    const uint64_t dword_commit = 0x1020304050607080ULL;
    const uint64_t dword_refresh = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t dword_x0_commit = 0xa1b2c3d4e5f60718ULL;
    const uint64_t dword_fail_payload = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = dword_initial;
    *(uint64_t *)&ai_lr_sc_words[2] = dword_guard;

    TEST_SETUP_EXCEPT();
    uint64_t dword_reserved = ai_lr_sc_lr_d64(dword_addr);
    uint32_t dword_status = ai_lr_sc_sc_d_status(dword_addr, dword_commit);
    TEST_ASSERT("lr.d returns the original doubleword and sc.d returns zero on the fresh reservation",
        excpt.triggered == false &&
        dword_reserved == dword_initial &&
        dword_status == 0U
    );
    TEST_ASSERT("successful sc.d writes exactly the reserved doubleword and preserves the adjacent doubleword guard",
        *(uint64_t *)&ai_lr_sc_words[0] == dword_commit &&
        *(uint64_t *)&ai_lr_sc_words[2] == dword_guard
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_dword_status =
        ai_lr_sc_sc_d_status(dword_addr, dword_fail_payload);
    TEST_ASSERT("a second sc.d without a fresh lr.d fails after the successful sc.d consumed the reservation",
        excpt.triggered == false &&
        second_dword_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == dword_commit &&
        *(uint64_t *)&ai_lr_sc_words[2] == dword_guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t dword_refresh_reserved = ai_lr_sc_lr_d64(dword_addr);
    uint32_t dword_refresh_status =
        ai_lr_sc_sc_d_status(dword_addr, dword_refresh);
    TEST_ASSERT("a fresh lr.d/sc.d pair after the consumed-reservation failure succeeds and updates the doubleword once",
        excpt.triggered == false &&
        dword_refresh_reserved == dword_commit &&
        dword_refresh_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == dword_refresh &&
        *(uint64_t *)&ai_lr_sc_words[2] == dword_guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t dword_x0_reserved = ai_lr_sc_lr_d64(dword_addr);
    uint64_t dword_x0_after = ai_arch_lr_sc_sc_d_x0_zero(dword_addr,
        dword_x0_commit);
    TEST_ASSERT("sc.d with rd=x0 discards the zero success code while still committing the reserved doubleword update",
        excpt.triggered == false &&
        dword_x0_reserved == dword_refresh &&
        dword_x0_after == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == dword_x0_commit &&
        *(uint64_t *)&ai_lr_sc_words[2] == dword_guard
    );

    TEST_SETUP_EXCEPT();
    uint64_t dword_x0_fail_after = ai_arch_lr_sc_sc_d_x0_zero(dword_addr,
        dword_fail_payload);
    TEST_ASSERT("a failing sc.d with rd=x0 still leaves x0 as zero and performs no memory write",
        excpt.triggered == false &&
        dword_x0_fail_after == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == dword_x0_commit &&
        *(uint64_t *)&ai_lr_sc_words[2] == dword_guard
    );

    TEST_SETUP_EXCEPT();
    uint32_t dword_cold_status =
        ai_lr_sc_sc_d_status(dword_addr, dword_fail_payload);
    TEST_ASSERT("a cold sc.d with no fresh lr.d returns nonzero and preserves the target and guard doublewords",
        excpt.triggered == false &&
        dword_cold_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == dword_x0_commit &&
        *(uint64_t *)&ai_lr_sc_words[2] == dword_guard
    );

    TEST_END();
}

bool ai_arch_lr_sc_mismatch_fail_no_write_matrix_corner()
{
    TEST_START();

    const uintptr_t word_addr = ai_lr_sc_addr(0);
    const uintptr_t other_word_addr = ai_lr_sc_addr(1);
    const uint32_t word_initial = 0x10203040U;
    const uint32_t other_word_initial = 0x55667788U;
    const uint32_t word_refresh = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(word_initial, other_word_initial);

    TEST_SETUP_EXCEPT();
    uint32_t word_reserved = lr_w(word_addr);
    uint32_t other_word_status = sc_w(other_word_addr, 0x13579bdfU);
    uint32_t word_retry_status = sc_w(word_addr, 0x2468ace0U);
    AI_ASSERT_NO_EXCEPT_AND("lr.w reserves one word, while same-width sc.w to a different word fails and performs no write",
        word_reserved == word_initial &&
        other_word_status != 0U &&
        ai_lr_sc_words[0] == word_initial &&
        ai_lr_sc_words[1] == other_word_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("after the different-word failed sc.w, retrying the original word without a fresh lr.w still fails and keeps both words unchanged",
        word_retry_status != 0U &&
        ai_lr_sc_words[0] == word_initial &&
        ai_lr_sc_words[1] == other_word_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t word_refresh_reserved = lr_w(word_addr);
    uint32_t word_refresh_status = sc_w(word_addr, word_refresh);
    AI_ASSERT_NO_EXCEPT_AND("a fresh lr.w/sc.w after the different-word mismatch succeeds on the original word and preserves the other word",
        word_refresh_reserved == word_initial &&
        word_refresh_status == 0U &&
        ai_lr_sc_words[0] == word_refresh &&
        ai_lr_sc_words[1] == other_word_initial
    );

    const uintptr_t dword_addr = ai_lr_sc_addr(0);
    const uintptr_t other_dword_addr = ai_lr_sc_addr(2);
    const uint64_t dword_initial = 0x1122334455667788ULL;
    const uint64_t other_dword_initial = 0x99aabbccddeeff00ULL;
    const uint64_t dword_refresh = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = dword_initial;
    *(uint64_t *)&ai_lr_sc_words[2] = other_dword_initial;

    TEST_SETUP_EXCEPT();
    uint64_t dword_reserved = ai_lr_sc_lr_d64(dword_addr);
    uint32_t other_dword_status =
        ai_lr_sc_sc_d_status(other_dword_addr, 0x0f1e2d3c4b5a6978ULL);
    uint32_t dword_retry_status =
        ai_lr_sc_sc_d_status(dword_addr, 0x8877665544332211ULL);
    TEST_ASSERT("lr.d reserves one doubleword, while same-width sc.d to a different doubleword fails and performs no write",
        excpt.triggered == false &&
        dword_reserved == dword_initial &&
        other_dword_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == dword_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == other_dword_initial
    );
    TEST_ASSERT("after the different-doubleword failed sc.d, retrying the original doubleword without a fresh lr.d still fails and keeps both doublewords unchanged",
        excpt.triggered == false &&
        dword_retry_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == dword_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == other_dword_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t dword_refresh_reserved = ai_lr_sc_lr_d64(dword_addr);
    uint32_t dword_refresh_status =
        ai_lr_sc_sc_d_status(dword_addr, dword_refresh);
    TEST_ASSERT("a fresh lr.d/sc.d after the different-doubleword mismatch succeeds on the original doubleword and preserves the other doubleword",
        excpt.triggered == false &&
        dword_refresh_reserved == dword_initial &&
        dword_refresh_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == dword_refresh &&
        *(uint64_t *)&ai_lr_sc_words[2] == other_dword_initial
    );

    TEST_END();
}

bool ai_arch_lr_sc_repeated_fail_then_success()
{
    TEST_START();

    const uintptr_t target_addr = ai_lr_sc_addr(0);
    const uintptr_t decoy_addr = ai_lr_sc_addr(1);
    const uint32_t initial_target = 0x10203040U;
    const uint32_t initial_decoy = 0x55667788U;
    const uint32_t final_value = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(initial_target, initial_decoy);

    for (uint32_t round = 0; round < 8U; ++round) {
        TEST_SETUP_EXCEPT();
        uint32_t target_value = lr_w(target_addr);
        AI_ASSERT_NO_EXCEPT_AND("each retry round starts by reserving the target word",
            target_value == initial_target
        );

        TEST_SETUP_EXCEPT();
        uint32_t decoy_value = lr_w(decoy_addr);
        AI_ASSERT_NO_EXCEPT_AND("the decoy lr.w replaces the earlier reservation and sets up a deterministic failure",
            decoy_value == initial_decoy
        );

        TEST_SETUP_EXCEPT();
        uint32_t sc_status = sc_w(target_addr, 0x90000000U | round);
        AI_ASSERT_NO_EXCEPT_AND("repeated sc.w attempts fail once another lr.w has replaced the reservation",
            sc_status != 0U &&
            ai_lr_sc_words[0] == initial_target
        );
    }

    /*
     * Keep the final uncontended lr/sc pair back-to-back.
     * Extra test harness work between them can clobber the live reservation.
     */
    TEST_SETUP_EXCEPT();
    uint32_t final_lr = lr_w(target_addr);
    uint32_t final_status = sc_w(target_addr, final_value);
    AI_ASSERT_NO_EXCEPT_AND("after repeated failures a fresh lr.w still obtains a usable reservation",
        final_lr == initial_target
    );
    AI_ASSERT_NO_EXCEPT_AND("an uncontended lr/sc pair still succeeds after many earlier failures",
        final_status == 0U &&
        ai_lr_sc_words[0] == final_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_repeated_doubleword_fail_then_success_corner()
{
    TEST_START();

    const uintptr_t target_addr = ai_lr_sc_addr(0);
    const uintptr_t decoy_addr = ai_lr_sc_addr(2);
    const uint64_t initial_target = 0x1020304050607080ULL;
    const uint64_t initial_decoy = 0x5566778899aabbccULL;
    const uint64_t final_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_target;
    *(uint64_t *)&ai_lr_sc_words[2] = initial_decoy;

    for (uint32_t round = 0; round < 8U; ++round) {
        TEST_SETUP_EXCEPT();
        uint64_t target_value = ai_lr_sc_lr_d64(target_addr);
        TEST_ASSERT("each retry round starts by reserving the target doubleword",
            excpt.triggered == false &&
            target_value == initial_target
        );

        TEST_SETUP_EXCEPT();
        uint64_t decoy_value = ai_lr_sc_lr_d64(decoy_addr);
        TEST_ASSERT("the decoy lr.d replaces the earlier reservation and sets up a deterministic doubleword failure",
            excpt.triggered == false &&
            decoy_value == initial_decoy
        );

        TEST_SETUP_EXCEPT();
        uint32_t sc_status = ai_lr_sc_sc_d_status(target_addr,
            0x9000000000000000ULL | (uint64_t)round);
        TEST_ASSERT("repeated sc.d attempts fail once another lr.d has replaced the reservation",
            excpt.triggered == false &&
            sc_status != 0U &&
            *(uint64_t *)&ai_lr_sc_words[0] == initial_target
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t final_lr = ai_lr_sc_lr_d64(target_addr);
    uint32_t final_status = ai_lr_sc_sc_d_status(target_addr, final_value);
    TEST_ASSERT("after repeated doubleword failures a fresh lr.d still obtains a usable reservation",
        excpt.triggered == false &&
        final_lr == initial_target
    );
    TEST_ASSERT("an uncontended lr.d/sc.d pair still succeeds after many earlier reservation-replacement failures",
        excpt.triggered == false &&
        final_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == final_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_inner_only_succeeds()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(1);
    const uint32_t outer_initial = 0x11112222U;
    const uint32_t inner_initial = 0x33334444U;
    const uint32_t inner_final = 0xdeadbeefu;

    ai_lr_sc_prepare_words(outer_initial, inner_initial);

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(outer_addr);
    AI_ASSERT_NO_EXCEPT_AND("the outer lr.w captures the original outer word before the nested sequence",
        outer_value == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(inner_addr);
    uint32_t inner_status = sc_w(inner_addr, inner_final);
    AI_ASSERT_NO_EXCEPT_AND("the inner lr.w captures the word that replaces the previous reservation",
        inner_value == inner_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("with different reserved addresses, the innermost sc.w succeeds on the latest reservation",
        inner_status == 0U &&
        ai_lr_sc_words[1] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(outer_addr, 0xabcdef01U);
    AI_ASSERT_NO_EXCEPT_AND("after the inner success, the older outer reservation is gone and the outer sc.w fails",
        outer_status != 0U &&
        ai_lr_sc_words[0] == outer_initial
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_inner_success_then_outer_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(1);
    const uint32_t outer_initial = 0x11112222U;
    const uint32_t inner_initial = 0x33334444U;
    const uint32_t inner_final = 0xdeadbeefU;
    const uint32_t outer_refreshed = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(outer_initial, inner_initial);

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(outer_addr);
    AI_ASSERT_NO_EXCEPT_AND("the outer lr.w captures the original outer word before a nested different-address outer-refresh corner begins",
        outer_value == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(inner_addr);
    uint32_t inner_status = sc_w(inner_addr, inner_final);
    AI_ASSERT_NO_EXCEPT_AND("the inner lr.w captures the different-address word that replaces the earlier reservation before the inner sc.w commits",
        inner_value == inner_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the inner sc.w succeeds first on the latest different-address reservation",
        inner_status == 0U &&
        ai_lr_sc_words[1] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(outer_addr, 0xabcdef01U);
    AI_ASSERT_NO_EXCEPT_AND("after the inner success, the older outer sc.w fails because its reservation has been replaced",
        outer_status != 0U &&
        ai_lr_sc_words[0] == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(outer_addr);
    uint32_t refreshed_status = sc_w(outer_addr, outer_refreshed);
    AI_ASSERT_NO_EXCEPT_AND("after that outer failure, a fresh lr.w on the outer address still captures the original outer word",
        refreshed_reserved == outer_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the outer reservation with a new lr.w/sc.w pair, the outer word updates while the inner committed word is preserved",
        refreshed_status == 0U &&
        ai_lr_sc_words[0] == outer_refreshed &&
        ai_lr_sc_words[1] == inner_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_inner_success_then_inner_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(1);
    const uint32_t outer_initial = 0x11112222U;
    const uint32_t inner_initial = 0x33334444U;
    const uint32_t inner_final = 0xdeadbeefU;
    const uint32_t inner_refreshed = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(outer_initial, inner_initial);

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(outer_addr);
    AI_ASSERT_NO_EXCEPT_AND("the outer lr.w captures the original outer word before a nested different-address inner-refresh corner begins",
        outer_value == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(inner_addr);
    uint32_t inner_status = sc_w(inner_addr, inner_final);
    AI_ASSERT_NO_EXCEPT_AND("the inner lr.w captures the different-address word that replaces the earlier reservation before the inner sc.w commits",
        inner_value == inner_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the inner sc.w succeeds first on the latest different-address reservation",
        inner_status == 0U &&
        ai_lr_sc_words[1] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(outer_addr, 0xabcdef01U);
    AI_ASSERT_NO_EXCEPT_AND("after the inner success, the older outer sc.w fails because its reservation has been replaced",
        outer_status != 0U &&
        ai_lr_sc_words[0] == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(inner_addr);
    uint32_t refreshed_status = sc_w(inner_addr, inner_refreshed);
    AI_ASSERT_NO_EXCEPT_AND("after that outer failure, a fresh lr.w on the inner address captures the first committed inner word",
        refreshed_reserved == inner_final
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the inner reservation with a new lr.w/sc.w pair, the inner word updates again while the untouched outer word is preserved",
        refreshed_status == 0U &&
        ai_lr_sc_words[0] == outer_initial &&
        ai_lr_sc_words[1] == inner_refreshed
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_doubleword_inner_only_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(2);
    const uint64_t outer_initial = 0x1111222233334444ULL;
    const uint64_t inner_initial = 0x5555666677778888ULL;
    const uint64_t inner_final = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = outer_initial;
    *(uint64_t *)&ai_lr_sc_words[2] = inner_initial;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(outer_addr);
    TEST_ASSERT("the outer lr.d captures the original outer doubleword before the nested different-address sequence",
        excpt.triggered == false &&
        outer_value == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(inner_addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(inner_addr, inner_final);
    TEST_ASSERT("the inner lr.d captures the different-address doubleword that replaces the previous reservation",
        excpt.triggered == false &&
        inner_value == inner_initial
    );
    TEST_ASSERT("with different reserved doubleword addresses, the innermost sc.d succeeds on the latest reservation",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(outer_addr, 0xabcdef0123456789ULL);
    TEST_ASSERT("after the inner doubleword success, the older outer reservation is gone and the outer sc.d fails",
        excpt.triggered == false &&
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_initial
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_doubleword_inner_success_then_outer_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(2);
    const uint64_t outer_initial = 0x1111222233334444ULL;
    const uint64_t inner_initial = 0x5555666677778888ULL;
    const uint64_t inner_final = 0xdeadbeefcafef00dULL;
    const uint64_t outer_refreshed = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = outer_initial;
    *(uint64_t *)&ai_lr_sc_words[2] = inner_initial;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(outer_addr);
    TEST_ASSERT("the outer lr.d captures the original outer doubleword before a nested different-address doubleword outer-refresh corner begins",
        excpt.triggered == false &&
        outer_value == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(inner_addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(inner_addr, inner_final);
    TEST_ASSERT("the inner lr.d captures the different-address doubleword that replaces the previous reservation before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == inner_initial
    );
    TEST_ASSERT("on Spike, the inner sc.d succeeds first on the latest different-address reservation",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(outer_addr, 0xabcdef0123456789ULL);
    TEST_ASSERT("after the inner doubleword success, the older outer sc.d fails because its reservation has been replaced",
        excpt.triggered == false &&
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(outer_addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(outer_addr, outer_refreshed);
    TEST_ASSERT("after that outer doubleword failure, a fresh lr.d on the outer address still captures the original outer doubleword",
        excpt.triggered == false &&
        refreshed_reserved == outer_initial
    );
    TEST_ASSERT("after refreshing the outer reservation with a new lr.d/sc.d pair, the outer doubleword updates while the inner committed doubleword is preserved",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_refreshed &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_doubleword_inner_success_then_inner_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(2);
    const uint64_t outer_initial = 0x1111222233334444ULL;
    const uint64_t inner_initial = 0x5555666677778888ULL;
    const uint64_t inner_final = 0xdeadbeefcafef00dULL;
    const uint64_t inner_refreshed = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = outer_initial;
    *(uint64_t *)&ai_lr_sc_words[2] = inner_initial;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(outer_addr);
    TEST_ASSERT("the outer lr.d captures the original outer doubleword before a nested different-address doubleword inner-refresh corner begins",
        excpt.triggered == false &&
        outer_value == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(inner_addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(inner_addr, inner_final);
    TEST_ASSERT("the inner lr.d captures the different-address doubleword that replaces the previous reservation before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == inner_initial
    );
    TEST_ASSERT("on Spike, the inner sc.d succeeds first on the latest different-address reservation",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(outer_addr, 0xabcdef0123456789ULL);
    TEST_ASSERT("after the inner doubleword success, the older outer sc.d fails because its reservation has been replaced",
        excpt.triggered == false &&
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(inner_addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(inner_addr, inner_refreshed);
    TEST_ASSERT("after that outer doubleword failure, a fresh lr.d on the inner address captures the first committed inner doubleword",
        excpt.triggered == false &&
        refreshed_reserved == inner_final
    );
    TEST_ASSERT("after refreshing the inner reservation with a new lr.d/sc.d pair, the inner doubleword updates again while the untouched outer doubleword is preserved",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_refreshed
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_different_size_inner_only_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(2);
    const uint64_t outer_initial = 0x1111222233334444ULL;
    const uint32_t inner_initial = 0x55667788U;
    const uint32_t inner_final = 0xdeadbeefU;
    const uint64_t inner_expected =
        ((uint64_t)0x99aabbccU << 32) | (uint64_t)inner_final;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = outer_initial;
    *(uint64_t *)&ai_lr_sc_words[2] =
        ((uint64_t)0x99aabbccU << 32) | (uint64_t)inner_initial;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(outer_addr);
    TEST_ASSERT("the outer lr.d captures the original outer doubleword before a mixed-size nested different-address reservation is created",
        excpt.triggered == false &&
        outer_value == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(inner_addr);
    uint32_t inner_status = sc_w(inner_addr, inner_final);
    AI_ASSERT_NO_EXCEPT_AND("the inner lr.w captures the low word of the different-address second doubleword before the inner sc.w commits",
        inner_value == inner_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("with different reserved addresses and different sizes, the latest inner sc.w still succeeds on the new reservation",
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(outer_addr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("after the mixed-size inner success on a different address, the older outer sc.d fails because its reservation has been replaced",
        excpt.triggered == false &&
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_different_size_inner_success_then_outer_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(2);
    const uint64_t outer_initial = 0x1111222233334444ULL;
    const uint32_t inner_initial = 0x55667788U;
    const uint32_t inner_final = 0xdeadbeefU;
    const uint64_t inner_expected =
        ((uint64_t)0x99aabbccU << 32) | (uint64_t)inner_final;
    const uint64_t outer_refreshed = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = outer_initial;
    *(uint64_t *)&ai_lr_sc_words[2] =
        ((uint64_t)0x99aabbccU << 32) | (uint64_t)inner_initial;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(outer_addr);
    TEST_ASSERT("the outer lr.d captures the original outer doubleword before a mixed-size nested different-address outer-refresh corner begins",
        excpt.triggered == false &&
        outer_value == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(inner_addr);
    uint32_t inner_status = sc_w(inner_addr, inner_final);
    AI_ASSERT_NO_EXCEPT_AND("the inner lr.w captures the low word of the different-address second doubleword before the inner sc.w commits",
        inner_value == inner_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the mixed-size inner sc.w succeeds first on the new different-address reservation",
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(outer_addr, 0x8877665544332211ULL);
    TEST_ASSERT("after the mixed-size inner success on a different address, the older outer sc.d fails because its reservation has been replaced",
        excpt.triggered == false &&
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(outer_addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(outer_addr, outer_refreshed);
    TEST_ASSERT("after that outer failure, a fresh lr.d on the outer address still captures the original outer doubleword",
        excpt.triggered == false &&
        refreshed_reserved == outer_initial
    );
    TEST_ASSERT("after refreshing with a same-size outer lr.d/sc.d pair, the outer doubleword updates while the inner committed word is preserved",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_refreshed &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_different_size_inner_success_then_inner_word_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(2);
    const uint64_t outer_initial = 0x1111222233334444ULL;
    const uint32_t inner_initial = 0x55667788U;
    const uint32_t inner_final = 0xdeadbeefU;
    const uint32_t inner_refreshed = 0x13579bdfU;
    const uint64_t inner_expected =
        ((uint64_t)0x99aabbccU << 32) | (uint64_t)inner_final;
    const uint64_t inner_refreshed_expected =
        ((uint64_t)0x99aabbccU << 32) | (uint64_t)inner_refreshed;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = outer_initial;
    *(uint64_t *)&ai_lr_sc_words[2] =
        ((uint64_t)0x99aabbccU << 32) | (uint64_t)inner_initial;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(outer_addr);
    TEST_ASSERT("the outer lr.d captures the original outer doubleword before a mixed-size nested different-address inner-refresh corner begins",
        excpt.triggered == false &&
        outer_value == outer_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(inner_addr);
    uint32_t inner_status = sc_w(inner_addr, inner_final);
    AI_ASSERT_NO_EXCEPT_AND("the inner lr.w captures the low word of the different-address second doubleword before the inner sc.w commits",
        inner_value == inner_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the mixed-size inner sc.w succeeds first on the new different-address reservation",
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(outer_addr, 0x8877665544332211ULL);
    TEST_ASSERT("after the mixed-size inner success on a different address, the older outer sc.d fails because its reservation has been replaced",
        excpt.triggered == false &&
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(inner_addr);
    uint32_t refreshed_status = sc_w(inner_addr, inner_refreshed);
    AI_ASSERT_NO_EXCEPT_AND("after that outer failure, a fresh lr.w on the inner address still captures the first committed inner word",
        refreshed_reserved == inner_final
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size inner lr.w/sc.w pair, the low word updates again while the untouched upper word is preserved",
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_word_then_doubleword_inner_only_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(2);
    const uint64_t outer_backing = 0x1111222233334444ULL;
    const uint64_t inner_initial = 0x5566778899aabbccULL;
    const uint64_t inner_final = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = outer_backing;
    *(uint64_t *)&ai_lr_sc_words[2] = inner_initial;

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(outer_addr);
    AI_ASSERT_NO_EXCEPT_AND("the outer lr.w captures the low word before a wider nested reservation is created on a different address",
        outer_value == (uint32_t)outer_backing
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(inner_addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(inner_addr, inner_final);
    TEST_ASSERT("the inner lr.d captures the different-address second doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == inner_initial
    );
    TEST_ASSERT("with different reserved addresses and a wider inner reservation, the latest inner sc.d succeeds on the new address",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(outer_addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the wider inner success on a different address, the older outer sc.w fails because its reservation has been replaced",
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_backing &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_word_then_doubleword_inner_success_then_outer_word_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(2);
    const uint64_t outer_backing = 0x1111222233334444ULL;
    const uint64_t inner_initial = 0x5566778899aabbccULL;
    const uint64_t inner_final = 0xdeadbeefcafef00dULL;
    const uint32_t outer_refreshed = 0xa5a55a5aU;
    const uint64_t outer_refreshed_expected =
        0x1111222200000000ULL | (uint64_t)outer_refreshed;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = outer_backing;
    *(uint64_t *)&ai_lr_sc_words[2] = inner_initial;

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(outer_addr);
    AI_ASSERT_NO_EXCEPT_AND("the outer lr.w captures the low word before a wider nested different-address outer-word refresh corner begins",
        outer_value == (uint32_t)outer_backing
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(inner_addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(inner_addr, inner_final);
    TEST_ASSERT("the inner lr.d captures the different-address second doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == inner_initial
    );
    TEST_ASSERT("on Spike, the wider inner sc.d succeeds first on the different-address reservation",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(outer_addr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after the wider inner success on a different address, the older outer sc.w fails because its reservation has been replaced",
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_backing &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(outer_addr);
    uint32_t refreshed_status = sc_w(outer_addr, outer_refreshed);
    AI_ASSERT_NO_EXCEPT_AND("after that outer failure, a fresh lr.w on the outer address still captures the original low word",
        refreshed_reserved == (uint32_t)outer_backing
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size outer lr.w/sc.w pair, the outer low word updates while the inner committed doubleword is preserved",
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_refreshed_expected &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_different_address_word_then_doubleword_inner_success_then_inner_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t outer_addr = ai_lr_sc_addr(0);
    const uintptr_t inner_addr = ai_lr_sc_addr(2);
    const uint64_t outer_backing = 0x1111222233334444ULL;
    const uint64_t inner_initial = 0x5566778899aabbccULL;
    const uint64_t inner_final = 0xdeadbeefcafef00dULL;
    const uint64_t inner_refreshed = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = outer_backing;
    *(uint64_t *)&ai_lr_sc_words[2] = inner_initial;

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(outer_addr);
    AI_ASSERT_NO_EXCEPT_AND("the outer lr.w captures the low word before a wider nested different-address inner-doubleword refresh corner begins",
        outer_value == (uint32_t)outer_backing
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(inner_addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(inner_addr, inner_final);
    TEST_ASSERT("the inner lr.d captures the different-address second doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == inner_initial
    );
    TEST_ASSERT("on Spike, the wider inner sc.d succeeds first on the different-address reservation",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(outer_addr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after the wider inner success on a different address, the older outer sc.w fails because its reservation has been replaced",
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_backing &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(inner_addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(inner_addr, inner_refreshed);
    TEST_ASSERT("after that outer failure, a fresh lr.d on the inner address still captures the first committed inner doubleword",
        excpt.triggered == false &&
        refreshed_reserved == inner_final
    );
    TEST_ASSERT("after refreshing with a same-size inner lr.d/sc.d pair, the inner doubleword updates again while the untouched outer word backing stays preserved",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == outer_backing &&
        *(uint64_t *)&ai_lr_sc_words[2] == inner_refreshed
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_inner_only_succeeds()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t inner_final = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("the first lr.w captures the original word before a same-address nested reservation",
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(addr);
    uint32_t inner_status = sc_w(addr, inner_final);
    AI_ASSERT_NO_EXCEPT_AND("a second lr.w to the same address refreshes the reservation without changing the payload",
        inner_value == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("for a same-address nested pair, the innermost sc.w succeeds on the refreshed reservation",
        inner_status == 0U &&
        ai_lr_sc_words[0] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(addr, 0xf0e1d2c3U);
    AI_ASSERT_NO_EXCEPT_AND("once the inner sc.w commits, the older same-address sc.w fails because its reservation is consumed",
        outer_status != 0U &&
        ai_lr_sc_words[0] == inner_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_inner_success_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t inner_committed = 0x13579bdfU;
    const uint32_t refreshed_value = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("the first lr.w captures the original word before a same-address nested refresh corner begins",
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(addr);
    uint32_t inner_status = sc_w(addr, inner_committed);
    AI_ASSERT_NO_EXCEPT_AND("a second lr.w to the same address refreshes the reservation before the inner sc.w commits",
        inner_value == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("for the same-address nested refresh corner, the inner sc.w first succeeds and updates the shared word",
        inner_status == 0U &&
        ai_lr_sc_words[0] == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(addr, 0xf0e1d2c3U);
    AI_ASSERT_NO_EXCEPT_AND("after that inner success, the older outer sc.w fails because its reservation has already been consumed",
        outer_status != 0U &&
        ai_lr_sc_words[0] == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the outer failure, a fresh lr.w on the same address captures the inner committed word",
        refreshed_reserved == inner_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a new lr.w/sc.w pair on the same address, the shared word is updated again",
        refreshed_status == 0U &&
        ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_doubleword_inner_only_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t inner_final = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(addr);
    TEST_ASSERT("the first lr.d captures the original doubleword before a same-address nested doubleword reservation is created",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(addr, inner_final);
    TEST_ASSERT("a second lr.d to the same address refreshes the reservation without changing the original payload",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("for a same-address nested doubleword pair, the innermost sc.d succeeds on the refreshed reservation",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_final
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(addr, 0x1122334455667788ULL);
    TEST_ASSERT("once the inner sc.d commits, the older same-address sc.d fails because its reservation has already been consumed",
        excpt.triggered == false &&
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_doubleword_inner_success_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refreshed_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(addr);
    TEST_ASSERT("the first lr.d captures the original doubleword before a same-address nested doubleword refresh corner begins",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(addr, inner_committed);
    TEST_ASSERT("a second lr.d to the same address refreshes the reservation before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("for the same-address nested doubleword refresh corner, the inner sc.d first succeeds and updates the shared doubleword",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(addr, 0x1122334455667788ULL);
    TEST_ASSERT("after that inner doubleword success, the older outer sc.d fails because its reservation has already been consumed",
        excpt.triggered == false &&
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after the outer doubleword failure, a fresh lr.d on the same address captures the inner committed payload",
        excpt.triggered == false &&
        refreshed_reserved == inner_committed
    );
    TEST_ASSERT("after refreshing with a new lr.d/sc.d pair on the same address, the shared doubleword is fully updated again",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_different_size_fails()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(addr);
    TEST_ASSERT("a first lr.d captures the original 64-bit word before a same-address nested reservation with a different size",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w to the same address refreshes the reservation at a different size",
        inner_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t word_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after same-address nested reservations with different sizes, sc.w fails instead of committing",
        word_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t double_status = ai_lr_sc_sc_d_status(addr, 0x8877665544332211ULL);
    AI_ASSERT_NO_EXCEPT_AND("the older sc.d also fails because the mixed-size nested reservations leave no valid matching reservation",
        double_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_different_size_fail_then_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t refreshed_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(addr);
    TEST_ASSERT("a first lr.d captures the original 64-bit word before a same-address nested mixed-size recovery corner begins",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w to the same address refreshes the reservation at a different size before the failing mixed-size pair is exercised",
        inner_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t word_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, after same-address nested reservations with different sizes, the newer sc.w still fails and leaves the doubleword unchanged",
        word_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t double_status = ai_lr_sc_sc_d_status(addr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("the older sc.d also fails because the mixed-size nested reservations leave no valid matching reservation",
        excpt.triggered == false &&
        double_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after both mixed-size same-address nested sc operations fail, a fresh lr.d still captures the original doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair, the shared doubleword is fully overwritten",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_different_size_fail_then_word_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        0x1122334400000000ULL | (uint64_t)refreshed_value;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(addr);
    TEST_ASSERT("a first lr.d captures the original 64-bit word before a same-address nested mixed-size word-refresh corner begins",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w to the same address refreshes the reservation at a different size before the failing mixed-size pair is exercised",
        inner_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t word_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, after same-address nested reservations with different sizes, the newer sc.w still fails and leaves the doubleword unchanged",
        word_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t double_status = ai_lr_sc_sc_d_status(addr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("the older sc.d also fails because the mixed-size nested reservations leave no valid matching reservation",
        excpt.triggered == false &&
        double_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after both mixed-size same-address nested sc operations fail, a fresh lr.w still captures the original low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair, only the low word is updated while the original high word is preserved",
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_word_then_doubleword_inner_only_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w to the same address captures the low word before a wider nested reservation is created",
        outer_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(addr, inner_committed);
    TEST_ASSERT("a nested lr.d to the same address captures the full doubleword before the wider sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("on Spike, with same-address nested reservations and a wider inner reservation, the latest sc.d succeeds and updates the whole doubleword",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the inner same-address sc.d commits, the older narrower sc.w fails because its reservation has been consumed",
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_word_then_doubleword_inner_success_then_word_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        0xa1b2c3d400000000ULL | (uint64_t)refreshed_value;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w to the same address captures the low word before a wider same-address nested original-refresh corner begins",
        outer_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(addr, inner_committed);
    TEST_ASSERT("a nested lr.d to the same address captures the full doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("on Spike, with same-address nested reservations and a wider inner reservation, the latest sc.d succeeds and updates the whole doubleword",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the inner same-address sc.d commits, the older narrower sc.w fails because its reservation has been consumed",
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that outer failure, a fresh lr.w on the same address captures the low word of the committed doubleword",
        refreshed_reserved == (uint32_t)inner_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the same address, only the low word is updated while the high word from the earlier sc.d is preserved",
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_nested_same_address_word_then_doubleword_inner_success_then_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refreshed_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w to the same address captures the low word before a wider same-address nested doubleword-refresh corner begins",
        outer_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(addr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(addr, inner_committed);
    TEST_ASSERT("a nested lr.d to the same address captures the full doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("on Spike, with same-address nested reservations and a wider inner reservation, the latest sc.d succeeds and updates the whole doubleword",
        excpt.triggered == false &&
        inner_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the inner same-address sc.d commits, the older narrower sc.w fails because its reservation has been consumed",
        outer_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after that outer failure, a fresh lr.d on the same address captures the fully committed doubleword",
        excpt.triggered == false &&
        refreshed_reserved == inner_committed
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the same address, the shared doubleword is fully overwritten",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_block_different_byte_fails()
{
    TEST_START();

    const uintptr_t lr_addr = ai_lr_sc_addr(0);
    const uintptr_t sc_addr = ai_lr_sc_addr(1);
    const uint32_t first_initial = 0x10203040U;
    const uint32_t second_initial = 0x55667788U;

    ai_lr_sc_prepare_words(first_initial, second_initial);

    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(lr_addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w reserves the first word before an sc.w targets a different word in the same aligned block",
        lr_value == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = sc_w(sc_addr, 0xaabbccddu);
    AI_ASSERT_NO_EXCEPT_AND("sc.w fails when it targets different bytes inside the same block than the preceding lr.w",
        sc_status != 0U &&
        ai_lr_sc_words[0] == first_initial &&
        ai_lr_sc_words[1] == second_initial
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_block_different_byte_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const uintptr_t lr_addr = ai_lr_sc_addr(0);
    const uintptr_t mismatched_sc_addr = ai_lr_sc_addr(1);
    const uint32_t first_initial = 0x10203040U;
    const uint32_t second_initial = 0x55667788U;

    ai_lr_sc_prepare_words(first_initial, second_initial);

    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(lr_addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w reserves the original word before a different-word sc.w first fails inside the same block",
        lr_value == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(mismatched_sc_addr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w fails when it targets a different word inside the same aligned block",
        first_status != 0U &&
        ai_lr_sc_words[0] == first_initial &&
        ai_lr_sc_words[1] == second_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(lr_addr, 0xaabbccddU);
    AI_ASSERT_NO_EXCEPT_AND("after that failed same-block different-word sc.w, retrying sc.w on the original word without a fresh lr.w still fails because the failed sc.w has already invalidated the reservation",
        second_status != 0U &&
        ai_lr_sc_words[0] == first_initial &&
        ai_lr_sc_words[1] == second_initial
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_block_different_byte_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t lr_addr = ai_lr_sc_addr(0);
    const uintptr_t mismatched_sc_addr = ai_lr_sc_addr(1);
    const uint32_t first_initial = 0x10203040U;
    const uint32_t second_initial = 0x55667788U;
    const uint32_t refreshed_value = 0xaabbccddU;

    ai_lr_sc_prepare_words(first_initial, second_initial);

    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(lr_addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w reserves the original word before a different-word sc.w in the same block forces the first failure",
        lr_value == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(mismatched_sc_addr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("sc.w first fails when it targets different bytes inside the same aligned block than the preceding lr.w",
        first_status != 0U &&
        ai_lr_sc_words[0] == first_initial &&
        ai_lr_sc_words[1] == second_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(lr_addr);
    uint32_t refreshed_status = sc_w(lr_addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that same-block different-byte failure, a fresh lr.w still captures the original target word",
        refreshed_reserved == first_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original word, the following sc.w succeeds and leaves the neighboring word unchanged",
        refreshed_status == 0U &&
        ai_lr_sc_words[0] == refreshed_value &&
        ai_lr_sc_words[1] == second_initial
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_block_different_byte_fail_then_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t lr_addr = ai_lr_sc_addr(0);
    const uintptr_t mismatched_sc_addr = ai_lr_sc_addr(1);
    const uint32_t first_initial = 0x10203040U;
    const uint32_t second_initial = 0x55667788U;
    const uint64_t refreshed_value = 0xa1b2c3d4e5f60718ULL;

    ai_lr_sc_prepare_words(first_initial, second_initial);

    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(lr_addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w reserves the original word before a different-word sc.w in the same block forces the first failure",
        lr_value == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(mismatched_sc_addr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("sc.w first fails when it targets different bytes inside the same aligned block than the preceding lr.w",
        first_status != 0U &&
        ai_lr_sc_words[0] == first_initial &&
        ai_lr_sc_words[1] == second_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(lr_addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(lr_addr, refreshed_value);
    TEST_ASSERT("after that same-block different-byte failure, a fresh lr.d still captures the original doubleword spanning both same-block words",
        excpt.triggered == false &&
        refreshed_reserved == (((uint64_t)second_initial << 32) | (uint64_t)first_initial)
    );
    TEST_ASSERT("after refreshing with a wider lr.d/sc.d pair on the original start address, the whole doubleword is overwritten by the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_block_different_doubleword_range_fails_corner()
{
    TEST_START();

    const uintptr_t lr_addr = ai_lr_sc_addr(0);
    const uintptr_t sc_addr = ai_lr_sc_addr(2);
    const uint64_t first_initial = 0x1020304050607080ULL;
    const uint64_t second_initial = 0x5566778899aabbccULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = first_initial;
    *(uint64_t *)&ai_lr_sc_words[2] = second_initial;

    TEST_SETUP_EXCEPT();
    uint64_t lr_value = ai_lr_sc_lr_d64(lr_addr);
    TEST_ASSERT("lr.d reserves the first doubleword before an sc.d targets a different doubleword inside the same aligned LR/SC backing block",
        excpt.triggered == false &&
        lr_value == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = ai_lr_sc_sc_d_status(sc_addr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("on Spike, sc.d fails when it targets a different doubleword range inside the same block than the preceding lr.d",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == second_initial
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_block_different_doubleword_range_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const uintptr_t lr_addr = ai_lr_sc_addr(0);
    const uintptr_t mismatched_sc_addr = ai_lr_sc_addr(2);
    const uint64_t first_initial = 0x1020304050607080ULL;
    const uint64_t second_initial = 0x5566778899aabbccULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = first_initial;
    *(uint64_t *)&ai_lr_sc_words[2] = second_initial;

    TEST_SETUP_EXCEPT();
    uint64_t lr_value = ai_lr_sc_lr_d64(lr_addr);
    TEST_ASSERT("lr.d reserves the original doubleword before a different-range sc.d first fails inside the same block",
        excpt.triggered == false &&
        lr_value == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = ai_lr_sc_sc_d_status(mismatched_sc_addr, 0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("the first sc.d fails when it targets a different doubleword range inside the same aligned block",
        excpt.triggered == false &&
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == second_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(lr_addr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("after that failed same-block different-range sc.d, retrying sc.d on the original doubleword without a fresh lr.d still fails because the failed sc.d has already invalidated the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == second_initial
    );

    TEST_END();
}
