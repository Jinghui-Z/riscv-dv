#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_ARCH_LOAD_CANCEL_SLOTS 4U
#define AI_ARCH_LOAD_CANCEL_OBS 4U
#define AI_ARCH_LOAD_CANCEL_SCALAR_ATTR \
    __attribute__((noinline, optimize("no-tree-vectorize", \
                                      "no-tree-loop-distribute-patterns")))

static inline pte_t *ai_arch_load_cancel_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_load_cancel_prepare_hs(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    sfence_vma();
}

static AI_ARCH_LOAD_CANCEL_SCALAR_ATTR void
ai_arch_load_cancel_fill_u64(uintptr_t paddr, unsigned count, uint64_t value)
{
    for (unsigned i = 0; i < count; ++i) {
        write64(paddr + i * sizeof(uint64_t), value ^ ((uint64_t)i << 44));
    }
}

static AI_ARCH_LOAD_CANCEL_SCALAR_ATTR bool
ai_arch_load_cancel_slots_unchanged(uintptr_t paddr, uint64_t seed)
{
    for (unsigned i = 0; i < AI_ARCH_LOAD_CANCEL_SLOTS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) !=
            (seed ^ ((uint64_t)i << 44))) {
            return false;
        }
    }

    return true;
}

static inline void ai_arch_load_cancel_branch_store_episode(
    uintptr_t load_addr, uintptr_t slots_addr, uintptr_t marker_addr,
    uintptr_t obs_addr, uint64_t payload_mask, uint64_t side_marker)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv a1, %[load_addr]\n\t"
        "mv a2, %[slots_addr]\n\t"
        "mv a3, %[marker_addr]\n\t"
        "mv a4, %[obs_addr]\n\t"
        "mv a5, %[payload_mask]\n\t"
        "mv a6, %[side_marker]\n\t"
        "li a0, 0\n\t"
        "ld a0, 0(a1)\n\t"
        "bnez a0, 1f\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "andi t0, a0, 24\n\t"
        "add t1, a2, t0\n\t"
        "xor t2, a0, a5\n\t"
        "sd t2, 0(t1)\n\t"
        "sd a6, 0(a3)\n\t"
        "sd a0, 0(a4)\n\t"
        "sd t0, 8(a4)\n\t"
        "2:\n\t"
        "sd a0, 16(a4)\n\t"
        "ld t3, 0(a3)\n\t"
        "sd t3, 24(a4)\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [load_addr] "r"(load_addr),
          [slots_addr] "r"(slots_addr),
          [marker_addr] "r"(marker_addr),
          [obs_addr] "r"(obs_addr),
          [payload_mask] "r"(payload_mask),
          [side_marker] "r"(side_marker)
        : "a0", "a1", "a2", "a3", "a4", "a5", "a6",
          "t0", "t1", "t2", "t3", "memory");
}

static inline void ai_arch_load_cancel_compressed_episode(
    uintptr_t load_addr, uintptr_t slots_addr, uintptr_t marker_addr,
    uintptr_t obs_addr, uint64_t side_marker)
{
    asm volatile(
        ".option push\n\t"
        ".option arch,+c\n\t"
        "mv a1, %[load_addr]\n\t"
        "mv a2, %[slots_addr]\n\t"
        "mv a3, %[marker_addr]\n\t"
        "mv a4, %[obs_addr]\n\t"
        "mv a5, %[side_marker]\n\t"
        "li a0, 0\n\t"
        "ld a0, 0(a1)\n\t"
        "c.beqz a0, 2f\n\t"
        "c.andi a0, 24\n\t"
        "c.mv a1, a0\n\t"
        "c.add a2, a0\n\t"
        "c.sd a5, 0(a2)\n\t"
        "c.ld a0, 0(a2)\n\t"
        "c.sd a0, 0(a4)\n\t"
        "c.sd a1, 8(a4)\n\t"
        "c.sd a5, 0(a3)\n\t"
        "c.ld a0, 0(a3)\n\t"
        "c.sd a0, 16(a4)\n\t"
        "2:\n\t"
        "c.ld a1, 0(a3)\n\t"
        "c.sd a1, 16(a4)\n\t"
        "c.sd a0, 24(a4)\n\t"
        ".option push\n\t"
        ".option norvc\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        ".option pop\n\t"
        :
        : [load_addr] "r"(load_addr),
          [slots_addr] "r"(slots_addr),
          [marker_addr] "r"(marker_addr),
          [obs_addr] "r"(obs_addr),
          [side_marker] "r"(side_marker)
        : "a0", "a1", "a2", "a3", "a4", "a5", "memory");
}

bool ai_arch_load_wakeup_cancel_dependent_store_owner_corner()
{
    TEST_START();

    const enum test_page load_page = VSR_GRWX;
    const enum test_page side_page = SCRATCHPAD;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x2c0U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x2c0U;
    const uintptr_t slots_vaddr = hs_page_base(side_page) + 0x500U;
    const uintptr_t slots_paddr = phys_page_base(side_page) + 0x500U;
    const uintptr_t marker_vaddr = hs_page_base(side_page) + 0x580U;
    const uintptr_t marker_paddr = phys_page_base(side_page) + 0x580U;
    const uintptr_t obs_vaddr = hs_page_base(side_page) + 0x5c0U;
    const uintptr_t obs_paddr = phys_page_base(side_page) + 0x5c0U;
    const uint64_t no_read_leaf =
        (phys_page_base(load_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    const uint64_t legal_leaf =
        (phys_page_base(load_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    const uint64_t side_leaf =
        (phys_page_base(side_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    const uint64_t load_value = 0x6d5a4b3c2d1e0f18ULL;
    const uint64_t payload_mask = 0x99aa55aa33cc77ddULL;
    const uint64_t expected_payload = load_value ^ payload_mask;
    const uint64_t fault_slot_seed = 0x3141592600000100ULL;
    const uint64_t repair_slot_seed = 0x2718281800000200ULL;
    const uint64_t marker_seed = 0x123456789abcdef0ULL;
    const uint64_t side_marker = 0xfedcba9876543210ULL;
    const uint64_t obs_seed = 0x0badc0de00000000ULL;

    goto_priv(PRIV_M);
    write64(load_paddr, load_value);
    ai_arch_load_cancel_fill_u64(slots_paddr, AI_ARCH_LOAD_CANCEL_SLOTS,
        fault_slot_seed);
    write64(marker_paddr, marker_seed);
    ai_arch_load_cancel_fill_u64(obs_paddr, AI_ARCH_LOAD_CANCEL_OBS,
        obs_seed);

    ai_arch_load_cancel_prepare_hs();

    goto_priv(PRIV_M);
    *ai_arch_load_cancel_hs_leaf_entry(load_page) = no_read_leaf;
    *ai_arch_load_cancel_hs_leaf_entry(side_page) = side_leaf;
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    ai_arch_load_cancel_branch_store_episode(load_vaddr, slots_vaddr,
        marker_vaddr, obs_vaddr, payload_mask, side_marker);

    TEST_ASSERT("faulting HS load reports LPF/tval while the dependent branch-store window is architecturally skipped with rd still zero",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        read64(obs_vaddr + 16U) == 0ULL &&
        read64(obs_vaddr + 24U) == marker_seed);
    TEST_ASSERT("unsafe load wakeup must not let the dependent store or marker escape before LPF cancellation",
        ai_arch_load_cancel_slots_unchanged(slots_vaddr, fault_slot_seed) &&
        read64(marker_vaddr) == marker_seed &&
        read64(obs_vaddr) == obs_seed &&
        read64(obs_vaddr + 8U) == (obs_seed ^ (1ULL << 44)));

    goto_priv(PRIV_M);
    ai_arch_load_cancel_fill_u64(slots_paddr, AI_ARCH_LOAD_CANCEL_SLOTS,
        repair_slot_seed);
    write64(marker_paddr, marker_seed);
    ai_arch_load_cancel_fill_u64(obs_paddr, AI_ARCH_LOAD_CANCEL_OBS,
        obs_seed);

    *ai_arch_load_cancel_hs_leaf_entry(load_page) = legal_leaf;
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_load_cancel_branch_store_episode(load_vaddr, slots_vaddr,
        marker_vaddr, obs_vaddr, payload_mask, side_marker);

    TEST_ASSERT("after leaf repair the same dependent branch-store window must not be killed by stale ldCancel state",
        excpt.triggered == false &&
        read64(obs_vaddr) == load_value &&
        read64(obs_vaddr + 8U) == 24ULL &&
        read64(obs_vaddr + 16U) == load_value &&
        read64(obs_vaddr + 24U) == side_marker &&
        read64(marker_vaddr) == side_marker);
    TEST_ASSERT("after repair only the selector-chosen dependent store slot is updated exactly once with the load-derived payload",
        read64(slots_vaddr + 0U) == repair_slot_seed &&
        read64(slots_vaddr + 8U) ==
            (repair_slot_seed ^ (1ULL << 44)) &&
        read64(slots_vaddr + 16U) ==
            (repair_slot_seed ^ (2ULL << 44)) &&
        read64(slots_vaddr + 24U) == expected_payload);

    goto_priv(PRIV_M);
    *ai_arch_load_cancel_hs_leaf_entry(load_page) = legal_leaf;
    *ai_arch_load_cancel_hs_leaf_entry(side_page) = side_leaf;
    fence_iorw_iorw();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_wakeup_cancel_compressed_store_owner_corner()
{
    TEST_START();

    const enum test_page load_page = VSR_GRWX;
    const enum test_page side_page = SCRATCHPAD;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x640U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x640U;
    const uintptr_t slots_vaddr = hs_page_base(side_page) + 0x700U;
    const uintptr_t slots_paddr = phys_page_base(side_page) + 0x700U;
    const uintptr_t marker_vaddr = hs_page_base(side_page) + 0x780U;
    const uintptr_t marker_paddr = phys_page_base(side_page) + 0x780U;
    const uintptr_t obs_vaddr = hs_page_base(side_page) + 0x7c0U;
    const uintptr_t obs_paddr = phys_page_base(side_page) + 0x7c0U;
    const uint64_t no_read_leaf =
        (phys_page_base(load_page) >> 2) | PTE_V | PTE_AD | PTE_X;
    const uint64_t legal_leaf =
        (phys_page_base(load_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    const uint64_t side_leaf =
        (phys_page_base(side_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    const uint64_t load_value = 0x0000000000000010ULL;
    const uint64_t fault_slot_seed = 0x6c6f616400000100ULL;
    const uint64_t repair_slot_seed = 0x636d656d00000200ULL;
    const uint64_t marker_seed = 0x1122334455667788ULL;
    const uint64_t side_marker = 0x8877665544332211ULL;
    const uint64_t obs_seed = 0xfacefeed00000000ULL;

    goto_priv(PRIV_M);
    write64(load_paddr, load_value);
    ai_arch_load_cancel_fill_u64(slots_paddr, AI_ARCH_LOAD_CANCEL_SLOTS,
        fault_slot_seed);
    write64(marker_paddr, marker_seed);
    ai_arch_load_cancel_fill_u64(obs_paddr, AI_ARCH_LOAD_CANCEL_OBS,
        obs_seed);

    ai_arch_load_cancel_prepare_hs();

    goto_priv(PRIV_M);
    *ai_arch_load_cancel_hs_leaf_entry(load_page) = no_read_leaf;
    *ai_arch_load_cancel_hs_leaf_entry(side_page) = side_leaf;
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    ai_arch_load_cancel_compressed_episode(load_vaddr, slots_vaddr,
        marker_vaddr, obs_vaddr, side_marker);

    TEST_ASSERT("faulting HS load must cancel the dependent compressed add/sd window before any c.sd side effect is visible",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        ai_arch_load_cancel_slots_unchanged(slots_vaddr, fault_slot_seed) &&
        read64(marker_vaddr) == marker_seed &&
        read64(obs_vaddr) == obs_seed &&
        read64(obs_vaddr + 8U) == (obs_seed ^ (1ULL << 44)) &&
        read64(obs_vaddr + 16U) == marker_seed &&
        read64(obs_vaddr + 24U) == 0ULL);

    goto_priv(PRIV_M);
    ai_arch_load_cancel_fill_u64(slots_paddr, AI_ARCH_LOAD_CANCEL_SLOTS,
        repair_slot_seed);
    write64(marker_paddr, marker_seed);
    ai_arch_load_cancel_fill_u64(obs_paddr, AI_ARCH_LOAD_CANCEL_OBS,
        obs_seed);

    *ai_arch_load_cancel_hs_leaf_entry(load_page) = legal_leaf;
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_load_cancel_compressed_episode(load_vaddr, slots_vaddr,
        marker_vaddr, obs_vaddr, side_marker);

    TEST_ASSERT("after repair the compressed dependent store window executes using the load-selected slot and is not killed by stale cancel",
        excpt.triggered == false &&
        read64(obs_vaddr) == side_marker &&
        read64(obs_vaddr + 8U) == 16ULL &&
        read64(obs_vaddr + 16U) == side_marker &&
        read64(obs_vaddr + 24U) == side_marker &&
        read64(marker_vaddr) == side_marker);
    TEST_ASSERT("only the repaired compressed-store target slot is updated, guarding against stale source or wrong-slot early wakeup",
        read64(slots_vaddr + 0U) == repair_slot_seed &&
        read64(slots_vaddr + 8U) ==
            (repair_slot_seed ^ (1ULL << 44)) &&
        read64(slots_vaddr + 16U) == side_marker &&
        read64(slots_vaddr + 24U) ==
            (repair_slot_seed ^ (3ULL << 44)));

    goto_priv(PRIV_M);
    *ai_arch_load_cancel_hs_leaf_entry(load_page) = legal_leaf;
    *ai_arch_load_cancel_hs_leaf_entry(side_page) = side_leaf;
    fence_iorw_iorw();
    sfence_vma();

    TEST_END();
}
