#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ARCH_ZCB_LOAD_FAULT_LEAF  (PTE_AD | PTE_RWX)
#define AI_ARCH_ZCB_STORE_FAULT_LEAF (PTE_V | PTE_AD | PTE_R)
#define AI_ARCH_ZCB_LEGAL_LEAF       (PTE_V | PTE_AD | PTE_RWX)

typedef struct {
    uintptr_t pc;
    uintptr_t next_pc;
} ai_arch_zcb_probe_pc_t;

static inline pte_t *ai_arch_zcb_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uint64_t ai_arch_zcb_hs_leaf_value(enum test_page page,
    uint64_t flags)
{
    return (phys_page_base(page) >> 2) | flags;
}

static inline void ai_arch_zcb_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_arch_zcb_hs_leaf_entry(page) =
        ai_arch_zcb_hs_leaf_value(page, flags);
    goto_priv(PRIV_HS);
    sfence_vma();
}

static inline bool ai_arch_zcb_hs_leaf_matches(enum test_page page,
    uint64_t flags)
{
    return *ai_arch_zcb_hs_leaf_entry(page) ==
        ai_arch_zcb_hs_leaf_value(page, flags);
}

static inline void ai_arch_zcb_prepare_hs_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    sfence_vma();
}

static inline uint64_t ai_arch_zcb_clh_probe(uintptr_t base,
    uint64_t sentinel, ai_arch_zcb_probe_pc_t *pc_info)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb\n\t"
        "mv a1, %[base]\n\t"
        "mv a0, %[sentinel]\n\t"
        "mv a3, %[pc_info]\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(a3)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(a3)\n\t"
        "1:\n\t"
        "c.lh a0, 0(a1)\n\t"
        "2:\n\t"
        "mv %[value], a0\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [base] "r"(base),
          [sentinel] "r"(sentinel),
          [pc_info] "r"(pc_info)
        : "a0", "a1", "a3", "t0", "memory");

    return value;
}

static inline uint64_t ai_arch_zcb_clhu_probe(uintptr_t base,
    uint64_t sentinel, ai_arch_zcb_probe_pc_t *pc_info)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb\n\t"
        "mv a1, %[base]\n\t"
        "mv a0, %[sentinel]\n\t"
        "mv a3, %[pc_info]\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(a3)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(a3)\n\t"
        "1:\n\t"
        "c.lhu a0, 2(a1)\n\t"
        "2:\n\t"
        "mv %[value], a0\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [base] "r"(base),
          [sentinel] "r"(sentinel),
          [pc_info] "r"(pc_info)
        : "a0", "a1", "a3", "t0", "memory");

    return value;
}

static inline uint64_t ai_arch_zcb_clbu_probe(uintptr_t base,
    uint64_t sentinel, ai_arch_zcb_probe_pc_t *pc_info)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb\n\t"
        "mv a1, %[base]\n\t"
        "mv a0, %[sentinel]\n\t"
        "mv a3, %[pc_info]\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(a3)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(a3)\n\t"
        "1:\n\t"
        "c.lbu a0, 3(a1)\n\t"
        "2:\n\t"
        "mv %[value], a0\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [base] "r"(base),
          [sentinel] "r"(sentinel),
          [pc_info] "r"(pc_info)
        : "a0", "a1", "a3", "t0", "memory");

    return value;
}

static inline void ai_arch_zcb_csb_probe(uintptr_t base, uint64_t payload,
    ai_arch_zcb_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb\n\t"
        "mv a1, %[base]\n\t"
        "mv a2, %[payload]\n\t"
        "mv a3, %[pc_info]\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(a3)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(a3)\n\t"
        "1:\n\t"
        "c.sb a2, 1(a1)\n\t"
        "2:\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [payload] "r"(payload),
          [pc_info] "r"(pc_info)
        : "a1", "a2", "a3", "t0", "memory");
}

static inline void ai_arch_zcb_csh_probe(uintptr_t base, uint64_t payload,
    ai_arch_zcb_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb\n\t"
        "mv a1, %[base]\n\t"
        "mv a2, %[payload]\n\t"
        "mv a3, %[pc_info]\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(a3)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(a3)\n\t"
        "1:\n\t"
        "c.sh a2, 2(a1)\n\t"
        "2:\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [payload] "r"(payload),
          [pc_info] "r"(pc_info)
        : "a1", "a2", "a3", "t0", "memory");
}

bool ai_arch_zcb_narrow_fault_repair_metadata_corner()
{
    TEST_START();

    const enum test_page load_page = VSRWX_GRWX;
    const enum test_page store_page = VSRW_GRWX;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x180U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x180U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x280U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x280U;
    const uint64_t load_seed = 0x11223344fe808001ULL;
    const uint64_t load_guard = 0x5566778899aabbccULL;
    const uint64_t store_seed = 0x0102030405060708ULL;
    const uint64_t store_guard = 0x8877665544332211ULL;
    const uint64_t clh_sentinel = 0x0bad000000000001ULL;
    const uint64_t clhu_sentinel = 0x0bad000000000002ULL;
    const uint64_t clbu_sentinel = 0x0bad000000000003ULL;
    const uint64_t csb_payload = 0x5aULL;
    const uint64_t csh_payload = 0xc33cULL;
    const uint64_t store_expected = 0x01020304c33c5a08ULL;
    ai_arch_zcb_probe_pc_t pc = {0};
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    write64(load_paddr, load_seed);
    write64(load_paddr + 8U, load_guard);
    write64(store_paddr, store_seed);
    write64(store_paddr + 8U, store_guard);

    ai_arch_zcb_prepare_hs_translation();

    ai_arch_zcb_set_hs_leaf(load_page, AI_ARCH_ZCB_LOAD_FAULT_LEAF);

    TEST_SETUP_EXCEPT();
    observed = ai_arch_zcb_clh_probe(load_vaddr, clh_sentinel, &pc);
    TEST_ASSERT("Zcb C.LH load fault records compressed sepc, tval at byte EA, next_pc=pc+2, and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.epc == pc.pc &&
        excpt.tval == load_vaddr &&
        pc.next_pc == pc.pc + 2U &&
        observed == clh_sentinel,
        "cause=0x%llx epc=0x%llx pc=0x%llx tval=0x%llx expected=0x%llx next=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.epc,
        (unsigned long long)pc.pc,
        (unsigned long long)excpt.tval,
        (unsigned long long)load_vaddr,
        (unsigned long long)pc.next_pc
    );

    TEST_SETUP_EXCEPT();
    observed = ai_arch_zcb_clhu_probe(load_vaddr, clhu_sentinel, &pc);
    TEST_ASSERT("Zcb C.LHU load fault records compressed sepc, tval at halfword EA, next_pc=pc+2, and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.epc == pc.pc &&
        excpt.tval == load_vaddr + 2U &&
        pc.next_pc == pc.pc + 2U &&
        observed == clhu_sentinel
    );

    TEST_SETUP_EXCEPT();
    observed = ai_arch_zcb_clbu_probe(load_vaddr, clbu_sentinel, &pc);
    TEST_ASSERT("Zcb C.LBU load fault records compressed sepc, tval at byte EA, next_pc=pc+2, and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.epc == pc.pc &&
        excpt.tval == load_vaddr + 3U &&
        pc.next_pc == pc.pc + 2U &&
        observed == clbu_sentinel
    );

    ai_arch_zcb_set_hs_leaf(load_page, AI_ARCH_ZCB_LEGAL_LEAF);

    TEST_SETUP_EXCEPT();
    observed = ai_arch_zcb_clh_probe(load_vaddr, clh_sentinel, &pc);
    TEST_ASSERT("after repairing the Zcb load leaf, C.LH sign-extends the repaired halfword and still retires as a 16-bit instruction",
        excpt.triggered == false &&
        observed == 0xffffffffffff8001ULL &&
        pc.next_pc == pc.pc + 2U
    );

    TEST_SETUP_EXCEPT();
    observed = ai_arch_zcb_clhu_probe(load_vaddr, clhu_sentinel, &pc);
    TEST_ASSERT("after repairing the Zcb load leaf, C.LHU zero-extends the repaired halfword and still retires as a 16-bit instruction",
        excpt.triggered == false &&
        observed == 0xfe80ULL &&
        pc.next_pc == pc.pc + 2U
    );

    TEST_SETUP_EXCEPT();
    observed = ai_arch_zcb_clbu_probe(load_vaddr, clbu_sentinel, &pc);
    TEST_ASSERT("after repairing the Zcb load leaf, C.LBU zero-extends the repaired byte and still retires as a 16-bit instruction",
        excpt.triggered == false &&
        observed == 0xfeULL &&
        pc.next_pc == pc.pc + 2U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("Zcb load faults and repairs do not mutate backing load bytes or adjacent load guards",
        read64(load_paddr) == load_seed &&
        read64(load_paddr + 8U) == load_guard &&
        ai_arch_zcb_hs_leaf_matches(load_page, AI_ARCH_ZCB_LEGAL_LEAF)
    );

    ai_arch_zcb_set_hs_leaf(store_page, AI_ARCH_ZCB_STORE_FAULT_LEAF);

    TEST_SETUP_EXCEPT();
    ai_arch_zcb_csb_probe(store_vaddr, csb_payload, &pc);
    TEST_ASSERT("Zcb C.SB store fault records compressed sepc, byte-lane tval, next_pc=pc+2, and performs no partial write",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.epc == pc.pc &&
        excpt.tval == store_vaddr + 1U &&
        pc.next_pc == pc.pc + 2U &&
        ai_arch_zcb_hs_leaf_matches(store_page, AI_ARCH_ZCB_STORE_FAULT_LEAF)
    );

    TEST_SETUP_EXCEPT();
    ai_arch_zcb_csh_probe(store_vaddr, csh_payload, &pc);
    TEST_ASSERT("Zcb C.SH store fault records compressed sepc, halfword-lane tval, next_pc=pc+2, and performs no partial write",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.epc == pc.pc &&
        excpt.tval == store_vaddr + 2U &&
        pc.next_pc == pc.pc + 2U &&
        ai_arch_zcb_hs_leaf_matches(store_page, AI_ARCH_ZCB_STORE_FAULT_LEAF)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("faulting Zcb C.SB/C.SH leave the target doubleword and adjacent guard unchanged before repair",
        read64(store_paddr) == store_seed &&
        read64(store_paddr + 8U) == store_guard
    );

    ai_arch_zcb_set_hs_leaf(store_page, AI_ARCH_ZCB_LEGAL_LEAF);

    TEST_SETUP_EXCEPT();
    ai_arch_zcb_csb_probe(store_vaddr, csb_payload, &pc);
    TEST_ASSERT("after repairing the Zcb store leaf, C.SB completes and still retires as a 16-bit instruction",
        excpt.triggered == false &&
        pc.next_pc == pc.pc + 2U
    );

    TEST_SETUP_EXCEPT();
    ai_arch_zcb_csh_probe(store_vaddr, csh_payload, &pc);
    TEST_ASSERT("after repairing the Zcb store leaf, C.SH completes and still retires as a 16-bit instruction",
        excpt.triggered == false &&
        pc.next_pc == pc.pc + 2U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("Zcb store repair updates exactly the byte and halfword lanes while preserving every guard byte",
        read64(store_paddr) == store_expected &&
        read64(store_paddr + 8U) == store_guard &&
        ai_arch_zcb_hs_leaf_matches(store_page, AI_ARCH_ZCB_LEGAL_LEAF),
        "store_actual=0x%llx expected=0x%llx guard=0x%llx",
        (unsigned long long)read64(store_paddr),
        (unsigned long long)store_expected,
        (unsigned long long)read64(store_paddr + 8U)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
