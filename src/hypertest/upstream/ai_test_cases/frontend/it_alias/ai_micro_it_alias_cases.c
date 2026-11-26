#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_IT_ALIAS_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_MICRO_IT_ALIAS_MCONTROL6_LOAD 0x1ULL
#define AI_MICRO_IT_ALIAS_MCONTROL6_STORE 0x2ULL
#define AI_MICRO_IT_ALIAS_MCONTROL6_EXECUTE 0x4ULL

enum ai_micro_it_alias_followup_mode {
    AI_MICRO_IT_ALIAS_COMPLETION = 0,
    AI_MICRO_IT_ALIAS_NO_X,
    AI_MICRO_IT_ALIAS_GRANT_X,
    AI_MICRO_IT_ALIAS_NONCANONICAL,
    AI_MICRO_IT_ALIAS_ALL_ONES,
    AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FAULT,
    AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FIX,
};

enum ai_micro_it_alias_trigger_fault_mode {
    AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT = 0,
    AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT,
};

enum ai_micro_it_alias_dual_address_probe_mode {
    AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY = 0,
    AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES,
    AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION,
};

static inline pte_t *ai_micro_it_alias_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_it_alias_set_hs_leaf(enum test_page page,
    uintptr_t paddr,
    uint64_t flags)
{
    *ai_micro_it_alias_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline uintptr_t ai_micro_it_alias_make_noncanonical_vaddr(uintptr_t canonical_vaddr)
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

static inline uintptr_t ai_micro_it_alias_second_half_addr(uintptr_t misaligned_vaddr)
{
    const uintptr_t mask = (uintptr_t)sizeof(uint64_t) - 1U;

    return (misaligned_vaddr + mask) & ~mask;
}

static inline void ai_micro_it_alias_write_bytes(uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_it_alias_bytes_match(uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_micro_it_alias_overlay_u64(uint8_t *bytes, unsigned offset, uint64_t value)
{
    for (unsigned i = 0; i < sizeof(value); ++i) {
        bytes[offset + i] = (uint8_t)(value >> (8U * i));
    }
}

static inline void ai_micro_it_alias_fence_i(void)
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

static inline void ai_micro_it_alias_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline uint64_t ai_micro_it_alias_mcontrol6_tdata1(uint64_t access_bits)
{
    return AI_MICRO_IT_ALIAS_MCONTROL6_MSU_BASE | access_bits;
}

static inline void ai_micro_it_alias_arm_mcontrol6_slot_with_access(size_t slot,
    uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, ai_micro_it_alias_mcontrol6_tdata1(access_bits));
}

static inline void ai_micro_it_alias_arm_mcontrol6_slot(size_t slot, uintptr_t addr)
{
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(slot, addr,
        AI_MICRO_IT_ALIAS_MCONTROL6_EXECUTE);
}

static inline uint64_t ai_micro_it_alias_enable_machine_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static inline void ai_micro_it_alias_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

__attribute__((naked, noinline, aligned(4))) static uint64_t ai_micro_it_alias_execute_load_once(uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld a0, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_it_alias_execute_store_once(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a1, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

static bool ai_micro_it_alias_fetch_after_rewrite(bool use_vector,
    enum ai_micro_it_alias_followup_mode mode)
{
    enum test_page exec_page = VSRWX_GRWX;
    enum test_page alias_page = use_vector ? SWITCH2 : SWITCH1;
    enum test_page source_page = VSRW_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t exec_offset = use_vector ? 0x1a0U : 0x180U;
    const uintptr_t source_offset = use_vector ? 0x1e0U : 0x1c0U;
    const uintptr_t exec_vaddr = hs_page_base(exec_page) + exec_offset;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + exec_offset;
    const uintptr_t exec_phys = phys_page_base(exec_page) + exec_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t sentinel_before_addr = exec_phys - 1U;
    const uintptr_t sentinel_after_addr = exec_phys + sizeof(patched_pattern);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))exec_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;
    uintptr_t bad_vaddr = 0;
    uintptr_t fetch_target = 0;
    uint8_t sentinel_before = 0;
    uint8_t sentinel_after = 0;
    bool test_status = true;

    switch (mode) {
    case AI_MICRO_IT_ALIAS_COMPLETION:
    case AI_MICRO_IT_ALIAS_NO_X:
    case AI_MICRO_IT_ALIAS_GRANT_X:
        sentinel_before = use_vector ? 0x68U : 0x58U;
        sentinel_after = use_vector ? 0x86U : 0x85U;
        break;
    case AI_MICRO_IT_ALIAS_NONCANONICAL:
        sentinel_before = use_vector ? 0x3fU : 0x5dU;
        sentinel_after = use_vector ? 0xf3U : 0xd5U;
        bad_vaddr = ai_micro_it_alias_make_noncanonical_vaddr(exec_vaddr);
        break;
    case AI_MICRO_IT_ALIAS_ALL_ONES:
        sentinel_before = use_vector ? 0x40U : 0x5eU;
        sentinel_after = use_vector ? 0xa4U : 0xe5U;
        bad_vaddr = (uintptr_t)-1;
        fetch_target = bad_vaddr & ~1ULL;
        break;
    case AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FAULT:
        sentinel_before = use_vector ? 0x41U : 0x5fU;
        sentinel_after = use_vector ? 0xb4U : 0xf5U;
        break;
    case AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FIX:
        sentinel_before = use_vector ? 0x42U : 0x6aU;
        sentinel_after = use_vector ? 0xc4U : 0xa6U;
        break;
    }

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(exec_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_it_alias_write_bytes(source_phys, patched_pattern, sizeof(patched_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(exec_page, phys_page_base(exec_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_it_alias_set_hs_leaf(alias_page, phys_page_base(exec_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_it_alias_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the same-paddr alias code-rewrite probe starts, fetching from the executable HS alias still returns the warmed original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    if (use_vector) {
        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)source_vaddr, 8);
        TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the same-paddr writable alias rewrites the executable HS block",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)alias_vaddr, 8);
        TEST_ASSERT("a vector unit-stride store through a writable same-paddr alias can rewrite the executable HS block without touching adjacent bytes",
            excpt.triggered == false
        );
    } else {
        TEST_SETUP_EXCEPT();
        sd(alias_vaddr, 0x0000806700700513ULL);
        TEST_ASSERT("a scalar store through a writable same-paddr alias can rewrite the executable HS block without touching adjacent bytes",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the writable same-paddr alias rewrites the executable HS block, the patched bytes are visible in backing memory and the adjacent sentinels stay unchanged before fence.i",
        ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_it_alias_fence_i();

    if (mode == AI_MICRO_IT_ALIAS_COMPLETION) {
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after fence.i, fetching from the executable HS alias observes the instruction bytes written through the writable same-paddr alias and returns the patched value",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the same-paddr alias rewrite plus fence.i sequence, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();
        return test_status;
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_arm_mcontrol6_slot(0, exec_vaddr);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the same-paddr alias rewrite and fence.i, the first fetch to the executable alias still reports breakpoint when the execute-PC trigger is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the same-paddr alias execute-trigger corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the same-paddr alias rewrite and fence.i, the second fetch to the executable alias still reports breakpoint while the execute-PC trigger remains armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the same-paddr alias execute-trigger corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_it_alias_restore_machine_status(saved_mstatus);
    ai_micro_it_alias_clear_mcontrol6_slot(0);

    switch (mode) {
    case AI_MICRO_IT_ALIAS_NO_X:
    case AI_MICRO_IT_ALIAS_GRANT_X:
        ai_micro_it_alias_set_hs_leaf(exec_page, phys_page_base(exec_page),
            PTE_V | PTE_AD | PTE_RW);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        fetch_func();
        TEST_ASSERT("after clearing the execute-PC trigger, the executable alias first falls back to instruction page fault when execute permission is removed even though the writable same-paddr alias still exists",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == exec_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-X fetch page fault in the same-paddr alias corner still leaves the patched bytes and adjacent sentinels unchanged",
            ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        if (mode == AI_MICRO_IT_ALIAS_GRANT_X) {
            ai_micro_it_alias_set_hs_leaf(exec_page, phys_page_base(exec_page),
                PTE_V | PTE_AD | PTE_RWX);
            sfence_vma();

            goto_priv(PRIV_HS);
            ai_micro_it_alias_fence_i();
            TEST_SETUP_EXCEPT();
            observed = fetch_func();
            TEST_ASSERT("after restoring execute permission on the executable alias, fetching through the same-paddr alias setup completes normally again and returns the patched value",
                excpt.triggered == false &&
                observed == 7ULL
            );

            goto_priv(PRIV_M);
            TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later no-X fetch page fault, and final execute-permission repair in the same-paddr alias corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
                ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after
            );
        }
        break;
    case AI_MICRO_IT_ALIAS_NONCANONICAL:
        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same-paddr alias rewritten executable HS block falls back to instruction page fault for the non-canonical fetch target with the raw VA in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later non-canonical fetch page fault in the same-paddr alias corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the same-paddr alias execute-trigger plus non-canonical fetch fault sequence, a subsequent canonical fetch through the executable alias still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the same-paddr alias execute-trigger plus non-canonical fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_IT_ALIAS_ALL_ONES:
        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same-paddr alias rewritten executable HS block falls back to instruction page fault for the all-ones fetch target with the JALR-cleared target in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == fetch_target
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later all-ones fetch page fault in the same-paddr alias corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the same-paddr alias execute-trigger plus all-ones fetch fault sequence, a subsequent canonical fetch through the executable alias still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the same-paddr alias execute-trigger plus all-ones fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FAULT:
        goto_priv(PRIV_M);
        *ai_micro_it_alias_hs_leaf_entry(exec_page) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        fetch_func();
        TEST_ASSERT("after clearing the execute-PC trigger, the same-paddr alias rewritten executable HS block falls back to instruction access fault when the executable alias leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == exec_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later leaf-overflow fetch access fault in the same-paddr alias corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FIX:
        goto_priv(PRIV_M);
        *ai_micro_it_alias_hs_leaf_entry(exec_page) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        fetch_func();
        TEST_ASSERT("after clearing the execute-PC trigger, the same-paddr alias rewritten executable HS block first falls back to instruction access fault when the executable alias leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == exec_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the leaf-overflow fetch access fault before repair still leaves the patched bytes and adjacent sentinels unchanged in the same-paddr alias execute-trigger corner",
            ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        ai_micro_it_alias_set_hs_leaf(exec_page, phys_page_base(exec_page),
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after repairing the executable alias leaf entry, fetching through the same-paddr alias setup completes normally again and returns the patched value",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later leaf-overflow fetch access fault, and final leaf repair in the same-paddr alias corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(exec_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_IT_ALIAS_COMPLETION:
        break;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(false,
        AI_MICRO_IT_ALIAS_COMPLETION);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(false,
        AI_MICRO_IT_ALIAS_NO_X);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(false,
        AI_MICRO_IT_ALIAS_GRANT_X);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(false,
        AI_MICRO_IT_ALIAS_NONCANONICAL);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(false,
        AI_MICRO_IT_ALIAS_ALL_ONES);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(false,
        AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FAULT);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(false,
        AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FIX);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(true,
        AI_MICRO_IT_ALIAS_COMPLETION);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(true,
        AI_MICRO_IT_ALIAS_NO_X);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(true,
        AI_MICRO_IT_ALIAS_GRANT_X);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(true,
        AI_MICRO_IT_ALIAS_NONCANONICAL);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(true,
        AI_MICRO_IT_ALIAS_ALL_ONES);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(true,
        AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FAULT);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_rewrite(true,
        AI_MICRO_IT_ALIAS_LEAF_OVERFLOW_FIX);
    TEST_END();
}

enum ai_micro_it_alias_mixed_followup_mode {
    AI_MICRO_IT_ALIAS_MIXED_COMPLETION = 0,
    AI_MICRO_IT_ALIAS_MIXED_EXEC_TRIGGER_COMPLETION,
    AI_MICRO_IT_ALIAS_MIXED_NO_X,
    AI_MICRO_IT_ALIAS_MIXED_GRANT_X,
    AI_MICRO_IT_ALIAS_MIXED_NONCANONICAL,
    AI_MICRO_IT_ALIAS_MIXED_ALL_ONES,
    AI_MICRO_IT_ALIAS_MIXED_LEAF_OVERFLOW_FAULT,
    AI_MICRO_IT_ALIAS_MIXED_LEAF_OVERFLOW_FIX,
};

static bool ai_micro_it_alias_fetch_after_scalar_over_vector_rewrite(
    enum ai_micro_it_alias_mixed_followup_mode mode)
{
    enum test_page exec_page = VSRWX_GRWX;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRW_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t exec_offset = boundary_offset + 8U;
    const uintptr_t source_offset = 0x220U;
    const uintptr_t exec_vaddr = hs_page_base(exec_page) + exec_offset;
    const uintptr_t scalar_alias_vaddr = hs_page_base(scalar_alias_page) + exec_offset;
    const uintptr_t vector_alias_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t boundary_phys = phys_page_base(exec_page) + boundary_offset;
    const uintptr_t exec_phys = boundary_phys + 8U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(vector_pattern);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))exec_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;
    uintptr_t bad_vaddr = 0;
    uintptr_t fetch_target = 0;
    uint8_t sentinel_before = 0;
    uint8_t sentinel_after = 0;
    bool test_status = true;

    switch (mode) {
    case AI_MICRO_IT_ALIAS_MIXED_COMPLETION:
        sentinel_before = 0x6aU;
        sentinel_after = 0xa6U;
        break;
    case AI_MICRO_IT_ALIAS_MIXED_EXEC_TRIGGER_COMPLETION:
        sentinel_before = 0x6bU;
        sentinel_after = 0xb6U;
        break;
    case AI_MICRO_IT_ALIAS_MIXED_NO_X:
        sentinel_before = 0x79U;
        sentinel_after = 0xe9U;
        break;
    case AI_MICRO_IT_ALIAS_MIXED_GRANT_X:
        sentinel_before = 0x7aU;
        sentinel_after = 0xfaU;
        break;
    case AI_MICRO_IT_ALIAS_MIXED_NONCANONICAL:
        sentinel_before = 0x5dU;
        sentinel_after = 0xd5U;
        bad_vaddr = ai_micro_it_alias_make_noncanonical_vaddr(exec_vaddr);
        break;
    case AI_MICRO_IT_ALIAS_MIXED_ALL_ONES:
        sentinel_before = 0x5eU;
        sentinel_after = 0xe5U;
        bad_vaddr = (uintptr_t)-1;
        fetch_target = bad_vaddr & ~1ULL;
        break;
    case AI_MICRO_IT_ALIAS_MIXED_LEAF_OVERFLOW_FAULT:
        sentinel_before = 0x5fU;
        sentinel_after = 0xf5U;
        break;
    case AI_MICRO_IT_ALIAS_MIXED_LEAF_OVERFLOW_FIX:
        sentinel_before = 0x6cU;
        sentinel_after = 0xc6U;
        break;
    }

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(exec_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_it_alias_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(exec_page, phys_page_base(exec_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(exec_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(exec_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_it_alias_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar same-paddr alias code rewrite sequence starts, fetching from the executable HS alias still returns the warmed original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first same-paddr alias instruction payload before the mixed vector-then-scalar code rewrite fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_alias_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store through one writable same-paddr alias can rewrite a 16-byte executable HS region before the overlapping scalar alias overwrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector same-paddr alias code rewrite step, the staged boundary bytes are visible in backing memory and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_it_alias_bytes_match(boundary_phys, vector_pattern, sizeof(vector_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(scalar_alias_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar store through another writable same-paddr alias can overwrite the executable HS block to the final instruction stream without fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar same-paddr alias code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_it_alias_fence_i();

    if (mode == AI_MICRO_IT_ALIAS_MIXED_COMPLETION) {
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after fence.i, fetching from the mixed vector-then-scalar same-paddr rewritten executable HS alias returns the final scalar-overwritten value",
            excpt.triggered == false &&
            observed == 9ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the mixed vector-then-scalar same-paddr alias rewrite plus fence.i sequence, the final boundary bytes remain intact and the outer sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();
        return test_status;
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_arm_mcontrol6_slot(0, exec_vaddr);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar same-paddr alias rewrite and fence.i, the first fetch to the executable alias still reports breakpoint when the execute-PC trigger is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar same-paddr alias execute-trigger corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar same-paddr alias rewrite and fence.i, the second fetch to the executable alias still reports breakpoint while the execute-PC trigger remains armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar same-paddr alias execute-trigger corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_it_alias_restore_machine_status(saved_mstatus);
    ai_micro_it_alias_clear_mcontrol6_slot(0);

    switch (mode) {
    case AI_MICRO_IT_ALIAS_MIXED_EXEC_TRIGGER_COMPLETION:
        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after clearing the execute-PC trigger, the mixed vector-then-scalar same-paddr rewritten executable HS alias completes normally again and returns the final scalar-overwritten value",
            excpt.triggered == false &&
            observed == 9ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the mixed vector-then-scalar same-paddr alias rewrite plus repeated execute-trigger fetch sequence, the final boundary bytes remain intact and the outer sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_IT_ALIAS_MIXED_NO_X:
    case AI_MICRO_IT_ALIAS_MIXED_GRANT_X:
        ai_micro_it_alias_set_hs_leaf(exec_page, phys_page_base(exec_page),
            PTE_V | PTE_AD | PTE_RW);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        fetch_func();
        TEST_ASSERT("after clearing the execute-PC trigger, the mixed vector-then-scalar same-paddr rewritten executable HS alias first falls back to instruction page fault when execute permission is removed",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == exec_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-X fetch page fault in the mixed vector-then-scalar same-paddr alias corner still leaves the final boundary bytes and the outer sentinels unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        if (mode == AI_MICRO_IT_ALIAS_MIXED_GRANT_X) {
            ai_micro_it_alias_set_hs_leaf(exec_page, phys_page_base(exec_page),
                PTE_V | PTE_AD | PTE_RWX);
            sfence_vma();

            goto_priv(PRIV_HS);
            ai_micro_it_alias_fence_i();
            TEST_SETUP_EXCEPT();
            observed = fetch_func();
            TEST_ASSERT("after restoring execute permission on the executable alias, fetching through the mixed vector-then-scalar same-paddr alias setup completes normally again and returns the final scalar-overwritten value",
                excpt.triggered == false &&
                observed == 9ULL
            );

            goto_priv(PRIV_M);
            TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later no-X fetch page fault, and final execute-permission repair in the mixed vector-then-scalar same-paddr alias corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
                ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after
            );
        }
        break;
    case AI_MICRO_IT_ALIAS_MIXED_NONCANONICAL:
        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the mixed vector-then-scalar same-paddr rewritten executable HS alias falls back to instruction page fault for the non-canonical fetch target with the raw VA in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later non-canonical fetch page fault in the mixed vector-then-scalar same-paddr alias corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the mixed vector-then-scalar same-paddr alias execute-trigger plus non-canonical fetch fault sequence, a subsequent canonical fetch through the executable alias still executes the final scalar-overwritten instruction stream",
            excpt.triggered == false &&
            observed == 9ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar same-paddr alias execute-trigger plus non-canonical fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_IT_ALIAS_MIXED_ALL_ONES:
        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the mixed vector-then-scalar same-paddr rewritten executable HS alias falls back to instruction page fault for the all-ones fetch target with the JALR-cleared target in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == fetch_target
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later all-ones fetch page fault in the mixed vector-then-scalar same-paddr alias corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the mixed vector-then-scalar same-paddr alias execute-trigger plus all-ones fetch fault sequence, a subsequent canonical fetch through the executable alias still executes the final scalar-overwritten instruction stream",
            excpt.triggered == false &&
            observed == 9ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar same-paddr alias execute-trigger plus all-ones fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_IT_ALIAS_MIXED_LEAF_OVERFLOW_FAULT:
        goto_priv(PRIV_M);
        *ai_micro_it_alias_hs_leaf_entry(exec_page) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        fetch_func();
        TEST_ASSERT("after clearing the execute-PC trigger, the mixed vector-then-scalar same-paddr rewritten executable HS alias falls back to instruction access fault when the executable alias leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == exec_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later leaf-overflow fetch access fault in the mixed vector-then-scalar same-paddr alias corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_IT_ALIAS_MIXED_LEAF_OVERFLOW_FIX:
        goto_priv(PRIV_M);
        *ai_micro_it_alias_hs_leaf_entry(exec_page) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        fetch_func();
        TEST_ASSERT("after clearing the execute-PC trigger, the mixed vector-then-scalar same-paddr rewritten executable HS alias first falls back to instruction access fault when the executable alias leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == exec_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the leaf-overflow fetch access fault before repair still leaves the final boundary bytes and the outer sentinels unchanged in the mixed vector-then-scalar same-paddr alias execute-trigger corner",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        ai_micro_it_alias_set_hs_leaf(exec_page, phys_page_base(exec_page),
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_it_alias_fence_i();
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after repairing the executable alias leaf entry, fetching through the mixed vector-then-scalar same-paddr alias setup completes normally again and returns the final scalar-overwritten value",
            excpt.triggered == false &&
            observed == 9ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later leaf-overflow fetch access fault, and final leaf repair in the mixed vector-then-scalar same-paddr alias corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_IT_ALIAS_MIXED_COMPLETION:
        break;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_scalar_over_vector_rewrite(
        AI_MICRO_IT_ALIAS_MIXED_COMPLETION);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_scalar_over_vector_rewrite(
        AI_MICRO_IT_ALIAS_MIXED_EXEC_TRIGGER_COMPLETION);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_scalar_over_vector_rewrite(
        AI_MICRO_IT_ALIAS_MIXED_NO_X);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_scalar_over_vector_rewrite(
        AI_MICRO_IT_ALIAS_MIXED_GRANT_X);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_scalar_over_vector_rewrite(
        AI_MICRO_IT_ALIAS_MIXED_NONCANONICAL);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_scalar_over_vector_rewrite(
        AI_MICRO_IT_ALIAS_MIXED_ALL_ONES);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_scalar_over_vector_rewrite(
        AI_MICRO_IT_ALIAS_MIXED_LEAF_OVERFLOW_FAULT);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_it_alias_fetch_after_scalar_over_vector_rewrite(
        AI_MICRO_IT_ALIAS_MIXED_LEAF_OVERFLOW_FIX);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_store_stable(bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x220U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = 0x2cU;
    const uint8_t sentinel_after = 0xc2U;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar-load same-paddr alias stable probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload through a writable same-paddr alias before the scalar stable-load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the staged payload before the scalar stable-load probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar stable-load probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector same-paddr alias store touches the shared path, a scalar misaligned load still returns the vector-written payload without sticky alias state",
        excpt.triggered == false &&
        value == 0x2e2d2c2b2a292827ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-store then scalar-load same-paddr alias stable sequence, the shared boundary bytes still match the vector-written payload",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-store then scalar-load same-paddr alias adjacent-preservation sequence, the adjacent sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_load_stable(bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x100U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = 0x4cU;
    const uint8_t sentinel_after = 0xd4U;
    const uint8_t scratch_before = 0x6bU;
    const uint8_t scratch_after = 0xb6U;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the mixed vector-load then scalar-store stable probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar stable-store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar stable-store probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar stable-store probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the vector same-paddr alias load touches the shared path, a scalar misaligned store still commits normally without sticky alias state",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-load then scalar-store same-paddr alias stable sequence, the final boundary bytes reflect the expected overwrite order",
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-load then scalar-store same-paddr alias adjacent-preservation sequence, the shared boundary sentinels and scratch sentinels stay unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-load then scalar-store same-paddr alias stable sequence, a following scalar misaligned load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_stable(false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_stable(true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_stable(false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_stable(true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_alias_stable(bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x80U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the mixed vector-load then scalar-load stable probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar stable-load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar stable-load probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar stable-load probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector same-paddr alias load touches the shared data path, a scalar misaligned load still returns the expected overlapping payload without sticky alias state",
        excpt.triggered == false &&
        value == 0x4e4d4c4b4a494847ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-load then scalar-load same-paddr alias stable sequence, the shared boundary bytes still match their expected values",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-load then scalar-load same-paddr alias adjacent-preservation sequence, the shared boundary sentinels and scratch sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_alias_stable(bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x200U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar-store same-paddr alias stable probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte same-paddr alias payload before the scalar stable-store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the source payload before the scalar stable-store probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar stable-store probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the vector same-paddr alias store touches the shared path, a scalar misaligned store still commits normally without sticky alias state",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-store then scalar-store same-paddr alias stable sequence, the final boundary bytes reflect the expected overwrite order",
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-store then scalar-store same-paddr alias adjacent-preservation sequence, the adjacent sentinels stay unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-store then scalar-store same-paddr alias stable sequence, a following scalar misaligned load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_stable(false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_stable(true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_stable(false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_stable(true);
    TEST_END();
}

static bool ai_micro_it_alias_vector_load_after_scalar_store_stable(bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    static const uint8_t round_patterns[2][16] = {
        {
            0x20, 0x21, 0x22, 0x23,
            0x24, 0x25, 0x26, 0x27,
            0x28, 0x29, 0x2a, 0x2b,
            0x2c, 0x2d, 0x2e, 0x2f
        },
        {
            0x90, 0x91, 0x92, 0x93,
            0x94, 0x95, 0x96, 0x97,
            0x98, 0x99, 0x9a, 0x9b,
            0x9c, 0x9d, 0x9e, 0x9f
        }
    };
    static const uint64_t round_store_values[2] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_base_offset = 0x80U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(round_patterns[0]);
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint8_t expected_rounds[2][16];
    uintptr_t scratch_phys[2];
    uintptr_t scratch_vaddr[2];
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    for (unsigned round = 0; round < 2; ++round) {
        scratch_phys[round] = phys_page_base(scratch_page) + scratch_base_offset + (uintptr_t)(round * 0x20U);
        scratch_vaddr[round] = hs_page_base(scratch_page) + scratch_base_offset + (uintptr_t)(round * 0x20U);

        for (unsigned i = 0; i < sizeof(round_patterns[round]); ++i) {
            expected_rounds[round][i] = round_patterns[round][i];
        }
        ai_micro_it_alias_overlay_u64(expected_rounds[round], 7U, round_store_values[round]);

        goto_priv(PRIV_M);
        ai_micro_it_alias_write_bytes(boundary_phys, round_patterns[round], sizeof(round_patterns[round]));
        ai_micro_it_alias_write_bytes(scratch_phys[round], round_patterns[round], sizeof(round_patterns[round]));
        write8(sentinel_before_addr, sentinel_before);
        write8(sentinel_after_addr, sentinel_after);
        write8(scratch_phys[round] - 1U, scratch_before);
        write8(scratch_phys[round] + sizeof(round_patterns[round]), scratch_after);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(scalar_vaddr, round_store_values[round]);
        TEST_ASSERT("a scalar same-paddr alias store can update the shared boundary before the mixed vector-load readback probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the scalar same-paddr alias store, the shared boundary immediately reflects only the current-round overwrite bytes",
            ai_micro_it_alias_bytes_match(boundary_phys, expected_rounds[round], sizeof(expected_rounds[round]))
        );
        if (check_adjacent) {
            TEST_ASSERT("after the scalar same-paddr alias store, the outer boundary sentinels stay unchanged before the vector same-paddr alias load",
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after
            );
        }

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)vector_vaddr, 16);
        TEST_ASSERT("a vector same-paddr alias load can read back the scalar-updated shared boundary without exception",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)scratch_vaddr[round], 16);
        TEST_ASSERT("the vector same-paddr alias payload can be copied into scratch immediately after the scalar overwrite",
            excpt.triggered == false
        );

        fence_iorw_iorw();
        goto_priv(PRIV_M);
        TEST_ASSERT("the scratch copy after the vector same-paddr alias load matches the exact current-round scalar-overwritten boundary bytes",
            ai_micro_it_alias_bytes_match(scratch_phys[round], expected_rounds[round], sizeof(expected_rounds[round])) &&
            ai_micro_it_alias_bytes_match(boundary_phys, expected_rounds[round], sizeof(expected_rounds[round]))
        );
        if (check_adjacent) {
            TEST_ASSERT("the vector-load-after-scalar-store same-paddr alias round leaves both boundary and scratch sentinels unchanged",
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after &&
                read8(scratch_phys[round] - 1U) == scratch_before &&
                read8(scratch_phys[round] + sizeof(round_patterns[round])) == scratch_after
            );
        }
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after two vector-load-after-scalar-store same-paddr alias rounds, both scratch snapshots preserve their own round-local payloads with no stale carryover",
        ai_micro_it_alias_bytes_match(scratch_phys[0], expected_rounds[0], sizeof(expected_rounds[0])) &&
        ai_micro_it_alias_bytes_match(scratch_phys[1], expected_rounds[1], sizeof(expected_rounds[1]))
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vector_same_paddr_alias_load_after_scalar_same_paddr_alias_store_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_vector_load_after_scalar_store_stable(false);
    TEST_END();
}

bool ai_micro_hs_vector_same_paddr_alias_load_after_scalar_same_paddr_alias_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_vector_load_after_scalar_store_stable(true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_completion(
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x80U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_load_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the mixed vector/scalar execute-and-address-trigger load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar trigger-load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar trigger-load probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );

    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar trigger-load probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector same-paddr alias load touches the shared data path, the first scalar dual-trigger misaligned-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar same-paddr alias trigger-load probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar same-paddr alias trigger-load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector same-paddr alias load, the second scalar dual-trigger misaligned-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar same-paddr alias trigger-load probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar same-paddr alias trigger-load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector same-paddr alias load, clearing only the execute-PC slot still leaves the scalar misaligned-load address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias load probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the scalar same-paddr alias misaligned load still completes normally and returns the expected overlapping payload even though a vector alias load touched the shared path earlier",
        excpt.triggered == false &&
        value == 0x4e4d4c4b4a494847ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector/scalar same-paddr alias trigger-load sequence, the shared boundary bytes still match their expected values",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the mixed vector/scalar same-paddr alias trigger-load sequence, the shared boundary sentinels and scratch sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_completion(
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x200U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_store_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar same-paddr alias trigger-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte same-paddr alias payload before the scalar trigger-store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the source payload before the scalar trigger-store probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar trigger-store probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after a vector same-paddr alias store touches the shared path, the first scalar dual-trigger misaligned-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar same-paddr alias trigger-store probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar same-paddr alias trigger-store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the earlier vector same-paddr alias store, the second scalar dual-trigger misaligned-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar same-paddr alias trigger-store probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar same-paddr alias trigger-store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector same-paddr alias store, clearing only the execute-PC slot still leaves the scalar misaligned-store address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias store probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after clearing both trigger slots, the scalar same-paddr alias misaligned store still commits normally even though a vector alias store touched the shared path earlier",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-then-scalar same-paddr alias trigger-store sequence, the final boundary bytes reflect the expected overwrite order",
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-then-scalar same-paddr alias trigger-store sequence, the adjacent sentinels stay unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-then-scalar same-paddr alias trigger-store sequence, a following scalar misaligned load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_completion(false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_completion(true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_completion(false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_completion(true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x80U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_load_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x6cU : 0x5cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xd6U : 0xc6U;
    const uint8_t scratch_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x7aU : 0x6aU;
    const uint8_t scratch_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xb7U : 0xa7U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_LAF : CAUSE_LPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the mixed vector/scalar repeated-breakpoint fault-recovery load probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the mixed vector/scalar repeated-breakpoint fault-recovery load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar repeated-breakpoint fault-recovery load probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );

    if (check_adjacent) {
        TEST_ASSERT("before the scalar repeated-breakpoint fault-recovery load probe, the shared boundary bytes and adjacent sentinels still match the vector-warmed payload",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector same-paddr alias load touches the shared data path, the first scalar dual-trigger misaligned-load probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar same-paddr alias trigger-load probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar same-paddr alias trigger-load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector same-paddr alias load, the second scalar dual-trigger misaligned-load probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar same-paddr alias trigger-load probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar same-paddr alias trigger-load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector same-paddr alias load, clearing only the execute-PC slot still leaves the scalar misaligned-load address trigger active before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias load probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after clearing both trigger slots, the scalar same-paddr alias misaligned load first falls back to load access fault with the original VA before the alias leaf is repaired" :
        "after clearing both trigger slots, the scalar same-paddr alias misaligned load first falls back to load page fault with the original VA before the alias mapping is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias load access-fault path still leaves the shared boundary bytes unchanged after the earlier vector alias load" :
        "the fallback same-paddr alias load page-fault path still leaves the shared boundary bytes unchanged after the earlier vector alias load"),
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias load access-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged" :
            "the fallback same-paddr alias load page-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf, the mixed vector/scalar misaligned load path completes normally again and returns the expected overlapping payload" :
        "after restoring the scalar same-paddr alias mapping, the mixed vector/scalar misaligned load path completes normally again and returns the expected overlapping payload"),
        excpt.triggered == false &&
        value == 0x4e4d4c4b4a494847ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery load sequence, the shared boundary bytes still match their expected values" :
        "after the repeated-breakpoint plus page-fault recovery load sequence, the shared boundary bytes still match their expected values"),
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the repeated-breakpoint plus access-fault recovery load sequence, the shared boundary sentinels and scratch sentinels still match their expected values" :
            "after the repeated-breakpoint plus page-fault recovery load sequence, the shared boundary sentinels and scratch sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x200U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_store_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x7cU : 0x6cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xe7U : 0xd7U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_SAF : CAUSE_SPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar repeated-breakpoint fault-recovery store probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte same-paddr alias payload before the scalar repeated-breakpoint fault-recovery store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the source payload before the scalar repeated-breakpoint fault-recovery store probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar repeated-breakpoint fault-recovery store probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after a vector same-paddr alias store touches the shared path, the first scalar dual-trigger misaligned-store probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar same-paddr alias trigger-store probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar same-paddr alias trigger-store probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the earlier vector same-paddr alias store, the second scalar dual-trigger misaligned-store probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar same-paddr alias trigger-store probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar same-paddr alias trigger-store probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector same-paddr alias store, clearing only the execute-PC slot still leaves the scalar misaligned-store address trigger active before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias store probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias store probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after clearing both trigger slots, the scalar same-paddr alias misaligned store first falls back to store access fault with the original VA before the alias leaf is repaired" :
        "after clearing both trigger slots, the scalar same-paddr alias misaligned store first falls back to store page fault with the original VA before the alias mapping is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias store access-fault path still leaves the vector-written boundary bytes unchanged" :
        "the fallback same-paddr alias store page-fault path still leaves the vector-written boundary bytes unchanged"),
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias store access-fault path still leaves the adjacent sentinels unchanged" :
            "the fallback same-paddr alias store page-fault path still leaves the adjacent sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf, the mixed vector/scalar misaligned store path commits normally again" :
        "after restoring the scalar same-paddr alias mapping, the mixed vector/scalar misaligned store path commits normally again"),
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery store sequence, the final boundary bytes reflect the expected overwrite order" :
        "after the repeated-breakpoint plus page-fault recovery store sequence, the final boundary bytes reflect the expected overwrite order"),
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the repeated-breakpoint plus access-fault recovery store sequence, the adjacent sentinels stay unchanged" :
            "after the repeated-breakpoint plus page-fault recovery store sequence, the adjacent sentinels stay unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery store sequence, a following scalar misaligned load still sees the latest scalar store payload immediately" :
        "after the repeated-breakpoint plus page-fault recovery store sequence, a following scalar misaligned load still sees the latest scalar store payload immediately"),
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_alias_dual_address_trigger_completion(
    enum ai_micro_it_alias_dual_address_probe_mode mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x80U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the scalar dual-address-trigger load completion probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar dual-address-trigger load completion probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar dual-address-trigger load completion probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar dual-address-trigger load completion probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    if (mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY) {
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, second_half_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    } else {
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, scalar_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    }
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    if (mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY) {
        TEST_ASSERT("on Spike, arming only the true second-half-start load address slot in the same-paddr alias mixed vector-load-then-scalar-load path does not match; the scalar load still completes and returns the expected overlapping payload",
            excpt.triggered == false &&
            value == 0x4e4d4c4b4a494847ULL
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    } else {
        const char *first_breakpoint_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "with both the original misaligned scalar same-paddr alias load slot and the true second-half-start slot armed, Spike still reports breakpoint because only the original address matches in the mixed vector-load-then-scalar-load path" :
            "with both split-load address slots armed in the same-paddr alias mixed vector-load-then-scalar-load path, the first scalar load still reports breakpoint on Spike because only the original misaligned vaddr matches";
        TEST_ASSERT(first_breakpoint_msg,
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);

        goto_priv(PRIV_M);
        const char *first_load_state_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "the trapped same-paddr alias dual-address-trigger load probe still leaves the shared boundary bytes unchanged before the original slot is cleared in the mixed vector-load path" :
            "the first trapped same-paddr alias dual-address-trigger load probe still leaves the shared boundary bytes unchanged before the later completion path in the mixed vector-load path";
        TEST_ASSERT(first_load_state_msg,
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
        );
        if (check_adjacent) {
            const char *first_load_adjacent_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
                "the trapped same-paddr alias dual-address-trigger load probe still leaves the adjacent sentinels and scratch sentinels unchanged before the original slot is cleared in the mixed vector-load path" :
                "the first trapped same-paddr alias dual-address-trigger load probe still leaves the adjacent sentinels and scratch sentinels unchanged before the later completion path in the mixed vector-load path";
            TEST_ASSERT(first_load_adjacent_msg,
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after &&
                read8(scratch_phys - 1U) == scratch_before &&
                read8(scratch_phys + sizeof(pattern)) == scratch_after
            );
        }

        if (mode == AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION) {
            saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            ai_micro_it_alias_execute_load_once(scalar_vaddr);
            TEST_ASSERT("repeating the same scalar same-paddr alias load with both address slots still armed reports breakpoint again and still keeps the second-half-start slot ignored on Spike before completion in the mixed vector-load path",
                excpt.triggered == true &&
                excpt.cause == CAUSE_BKP
            );
            ai_micro_it_alias_restore_machine_status(saved_mstatus);

            goto_priv(PRIV_M);
            TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger load probe still leaves the shared boundary bytes unchanged before completion in the mixed vector-load path",
                ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
            );
            if (check_adjacent) {
                TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger load probe still leaves the adjacent sentinels and scratch sentinels unchanged before completion in the mixed vector-load path",
                    read8(sentinel_before_addr) == sentinel_before &&
                    read8(sentinel_after_addr) == sentinel_after &&
                    read8(scratch_phys - 1U) == scratch_before &&
                    read8(scratch_phys + sizeof(pattern)) == scratch_after
                );
            }
        }

        ai_micro_it_alias_clear_mcontrol6_slot(0);
        saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
        const char *completion_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "after clearing the original misaligned-load slot, the remaining second-half-start slot stays ignored on Spike and the same-paddr alias scalar load completes normally in the mixed vector-load path" :
            "after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the same-paddr alias scalar load completes normally in the mixed vector-load path";
        TEST_ASSERT(completion_msg,
            excpt.triggered == false &&
            value == 0x4e4d4c4b4a494847ULL
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    }

    goto_priv(PRIV_M);
    const char *final_load_state_msg = mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY ?
        "the same-paddr alias scalar load path with only the second-half-start address slot armed still leaves the shared boundary bytes and scratch copy intact after the prior vector load" :
        "after the same-paddr alias dual-address-trigger load completion path in the mixed vector-load path, the shared boundary bytes still match the vector-loaded payload";
    TEST_ASSERT(final_load_state_msg,
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        const char *final_load_adjacent_msg = mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY ?
            "the same-paddr alias scalar load path with only the second-half-start address slot armed still leaves the shared boundary sentinels and scratch sentinels unchanged after the prior vector load" :
            "after the same-paddr alias dual-address-trigger load completion path in the mixed vector-load path, the shared boundary sentinels and scratch sentinels still match their expected values";
        TEST_ASSERT(final_load_adjacent_msg,
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_alias_dual_address_trigger_completion(
    enum ai_micro_it_alias_dual_address_probe_mode mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x200U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the scalar dual-address-trigger store completion probe starts in the mixed vector-store path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte same-paddr alias payload before the scalar dual-address-trigger store completion probe in the mixed vector-store path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the source payload before the scalar dual-address-trigger store completion probe in the mixed vector-store path",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar dual-address-trigger store completion probe in the mixed vector-store path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    if (mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY) {
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, second_half_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    } else {
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, scalar_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    }
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    if (mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY) {
        TEST_ASSERT("on Spike, arming only the true second-half-start store address slot in the same-paddr alias mixed vector-store-then-scalar-store path does not match; the scalar store still commits normally",
            excpt.triggered == false
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    } else {
        const char *first_store_breakpoint_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "with both the original misaligned scalar same-paddr alias store slot and the true second-half-start slot armed, Spike still reports breakpoint before any bytes commit because only the original address matches in the mixed vector-store path" :
            "with both split-store address slots armed in the same-paddr alias mixed vector-store-then-scalar-store path, the first scalar store still reports breakpoint on Spike because only the original misaligned vaddr matches";
        TEST_ASSERT(first_store_breakpoint_msg,
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);

        goto_priv(PRIV_M);
        const char *first_store_state_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "the trapped same-paddr alias dual-address-trigger store probe still leaves the vector-written boundary bytes unchanged before the original slot is cleared in the mixed vector-store path" :
            "the first trapped same-paddr alias dual-address-trigger store probe still leaves the vector-written boundary bytes unchanged before the later completion path in the mixed vector-store path";
        TEST_ASSERT(first_store_state_msg,
            ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
        );
        if (check_adjacent) {
            const char *first_store_adjacent_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
                "the trapped same-paddr alias dual-address-trigger store probe still leaves the adjacent sentinels unchanged before the original slot is cleared in the mixed vector-store path" :
                "the first trapped same-paddr alias dual-address-trigger store probe still leaves the adjacent sentinels unchanged before the later completion path in the mixed vector-store path";
            TEST_ASSERT(first_store_adjacent_msg,
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after
            );
        }

        if (mode == AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION) {
            saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
            TEST_ASSERT("repeating the same scalar same-paddr alias store with both address slots still armed reports breakpoint again and still keeps the second-half-start slot ignored on Spike before completion in the mixed vector-store path",
                excpt.triggered == true &&
                excpt.cause == CAUSE_BKP
            );
            ai_micro_it_alias_restore_machine_status(saved_mstatus);

            goto_priv(PRIV_M);
            TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger store probe still leaves the vector-written boundary bytes unchanged before completion in the mixed vector-store path",
                ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
            );
            if (check_adjacent) {
                TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger store probe still leaves the adjacent sentinels unchanged before completion in the mixed vector-store path",
                    read8(sentinel_before_addr) == sentinel_before &&
                    read8(sentinel_after_addr) == sentinel_after
                );
            }
        }

        ai_micro_it_alias_clear_mcontrol6_slot(0);
        saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
        const char *store_completion_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "after clearing the original misaligned-store slot, the remaining second-half-start slot stays ignored on Spike and the same-paddr alias scalar store commits normally in the mixed vector-store path" :
            "after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the same-paddr alias scalar store commits normally in the mixed vector-store path";
        TEST_ASSERT(store_completion_msg,
            excpt.triggered == false
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    }

    goto_priv(PRIV_M);
    const char *final_store_state_msg = mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY ?
        "the same-paddr alias scalar store path with only the second-half-start address slot armed still reflects the expected overwrite order after the prior vector store" :
        "after the same-paddr alias dual-address-trigger store completion path in the mixed vector-store path, the final boundary bytes reflect the expected overwrite order";
    TEST_ASSERT(final_store_state_msg,
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        const char *final_store_adjacent_msg = mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY ?
            "the same-paddr alias scalar store path with only the second-half-start address slot armed still leaves the adjacent sentinels unchanged after the prior vector store" :
            "after the same-paddr alias dual-address-trigger store completion path in the mixed vector-store path, the adjacent sentinels still match their expected values";
        TEST_ASSERT(final_store_adjacent_msg,
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the same-paddr alias dual-address-trigger store completion path in the mixed vector-store path, a following scalar misaligned load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_second_half_address_trigger_ignored_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_dual_address_trigger_only_original_matches_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_dual_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_second_half_address_trigger_ignored_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_dual_address_trigger_only_original_matches_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_dual_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION, true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_alias_execute_trigger_second_half_address_slot_completion(
    bool repeat_breakpoint,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x80U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_load_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the scalar execute-trigger plus second-half-address-slot load probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar execute-trigger plus second-half-address-slot load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar execute-trigger plus second-half-address-slot load probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar execute-trigger plus second-half-address-slot load probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half-start load address, the mixed vector-load-then-scalar-load same-paddr alias probe still reports breakpoint first on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot load probe still leaves the shared boundary bytes and scratch copy unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot load probe still leaves the shared boundary sentinels and scratch sentinels unchanged after the prior vector load",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    if (repeat_breakpoint) {
        saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_it_alias_execute_load_once(scalar_vaddr);
        TEST_ASSERT("with the same execute-PC slot and true second-half-start load-address slot still armed, the second mixed same-paddr alias load attempt still reports breakpoint on Spike",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot load probe still leaves the shared boundary bytes and scratch copy unchanged after the prior vector load",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern))
        );
        if (check_adjacent) {
            TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot load probe still leaves the shared boundary sentinels and scratch sentinels unchanged after the prior vector load",
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after &&
                read8(scratch_phys - 1U) == scratch_before &&
                read8(scratch_phys + sizeof(pattern)) == scratch_after
            );
        }
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    if (repeat_breakpoint) {
        TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the true second-half-start load-address slot ignored on Spike, so the same-paddr alias scalar load completes normally in the mixed vector-load path",
            excpt.triggered == false &&
            value == 0x4e4d4c4b4a494847ULL
        );
    } else {
        TEST_ASSERT("after clearing the execute-PC slot, the true second-half-start load-address slot does not keep matching on Spike; the same-paddr alias scalar load completes normally in the mixed vector-load path",
            excpt.triggered == false &&
            value == 0x4e4d4c4b4a494847ULL
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot load completion path in the mixed vector-load path, the shared boundary bytes and scratch copy still match the vector-loaded payload",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot load completion path in the mixed vector-load path, the shared boundary sentinels and scratch sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_alias_execute_trigger_second_half_address_slot_completion(
    bool repeat_breakpoint,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x200U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_store_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the scalar execute-trigger plus second-half-address-slot store probe starts in the mixed vector-store path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte same-paddr alias payload before the scalar execute-trigger plus second-half-address-slot store probe in the mixed vector-store path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the source payload before the scalar execute-trigger plus second-half-address-slot store probe in the mixed vector-store path",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar execute-trigger plus second-half-address-slot store probe in the mixed vector-store path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half-start store address, the mixed vector-store-then-scalar-store same-paddr alias probe still reports breakpoint first on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot store probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    if (repeat_breakpoint) {
        saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
        TEST_ASSERT("with the same execute-PC slot and true second-half-start store-address slot still armed, the second mixed same-paddr alias store attempt still reports breakpoint on Spike",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot store probe still leaves the vector-written boundary bytes unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
        );
        if (check_adjacent) {
            TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot store probe still leaves the adjacent sentinels unchanged",
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after
            );
        }
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    if (repeat_breakpoint) {
        TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the true second-half-start store-address slot ignored on Spike, so the same-paddr alias scalar store commits normally in the mixed vector-store path",
            excpt.triggered == false
        );
    } else {
        TEST_ASSERT("after clearing the execute-PC slot, the true second-half-start store-address slot does not keep matching on Spike; the same-paddr alias scalar store commits normally in the mixed vector-store path",
            excpt.triggered == false
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot store completion path in the mixed vector-store path, the final boundary bytes reflect the expected overwrite order",
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot store completion path in the mixed vector-store path, the adjacent sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot store completion path in the mixed vector-store path, a following scalar misaligned load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_ignored_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_trigger_second_half_address_slot_completion(
            false, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_trigger_second_half_address_slot_completion(
            true, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_trigger_second_half_address_slot_completion(
            true, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_ignored_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_trigger_second_half_address_slot_completion(
            false, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_trigger_second_half_address_slot_completion(
            true, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_trigger_second_half_address_slot_completion(
            true, true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x80U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_load_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x4cU : 0x3cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xd3U : 0xc3U;
    const uint8_t scratch_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x6aU : 0x5aU;
    const uint8_t scratch_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xb5U : 0xa5U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_LAF : CAUSE_LPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the scalar execute-trigger plus second-half-address-slot load-recovery probe starts in the mixed vector-load path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar execute-trigger plus second-half-address-slot load-recovery probe in the mixed vector-load path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar execute-trigger plus second-half-address-slot load-recovery probe in the mixed vector-load path",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar execute-trigger plus second-half-address-slot load-recovery probe in the mixed vector-load path",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the true second-half-start load-address slot also armed, the first vector-load-then-scalar-load recovery probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot load-recovery probe still leaves the shared boundary bytes and scratch copy unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot load-recovery probe still leaves the shared boundary sentinels and scratch sentinels unchanged after the prior vector load",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("with the same execute-PC slot and true second-half-start load-address slot still armed, the second vector-load-then-scalar-load recovery probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot load-recovery probe still leaves the shared boundary bytes and scratch copy unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot load-recovery probe still leaves the shared boundary sentinels and scratch sentinels unchanged after the prior vector load",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(0);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after two prior execute-trigger breakpoint hits and then clearing only the execute-PC slot, the remaining second-half-start load-address slot still stays ignored on Spike, so the same-paddr alias load first falls back to load access fault with the original VA before the alias leaf is repaired in the mixed vector-load path" :
        "after two prior execute-trigger breakpoint hits and then clearing only the execute-PC slot, the remaining second-half-start load-address slot still stays ignored on Spike, so the same-paddr alias load first falls back to load page fault with the original VA before the alias mapping is restored in the mixed vector-load path"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias execute-trigger plus second-half-address-slot load access-fault path still leaves the shared boundary bytes and scratch copy unchanged in the mixed vector-load path" :
        "the fallback same-paddr alias execute-trigger plus second-half-address-slot load page-fault path still leaves the shared boundary bytes and scratch copy unchanged in the mixed vector-load path"),
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias execute-trigger plus second-half-address-slot load access-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged in the mixed vector-load path" :
            "the fallback same-paddr alias execute-trigger plus second-half-address-slot load page-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged in the mixed vector-load path"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf while the second-half-start load-address slot remains armed, the vector-load-then-scalar-load path completes normally again on Spike" :
        "after restoring the scalar same-paddr alias mapping while the second-half-start load-address slot remains armed, the vector-load-then-scalar-load path completes normally again on Spike"),
        excpt.triggered == false &&
        value == 0x4e4d4c4b4a494847ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery load sequence in the mixed vector-load path, the shared boundary bytes and scratch copy still match the vector-loaded payload" :
        "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery load sequence in the mixed vector-load path, the shared boundary bytes and scratch copy still match the vector-loaded payload"),
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery load sequence in the mixed vector-load path, the shared boundary sentinels and scratch sentinels still match their expected values" :
            "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery load sequence in the mixed vector-load path, the shared boundary sentinels and scratch sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x200U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_store_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x4cU : 0x3cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xd3U : 0xc3U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_SAF : CAUSE_SPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the scalar execute-trigger plus second-half-address-slot store-recovery probe starts in the mixed vector-store path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte same-paddr alias payload before the scalar execute-trigger plus second-half-address-slot store-recovery probe in the mixed vector-store path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the source payload before the scalar execute-trigger plus second-half-address-slot store-recovery probe in the mixed vector-store path",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar execute-trigger plus second-half-address-slot store-recovery probe in the mixed vector-store path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the true second-half-start store-address slot also armed, the first vector-store-then-scalar-store recovery probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot store-recovery probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot store-recovery probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with the same execute-PC slot and true second-half-start store-address slot still armed, the second vector-store-then-scalar-store recovery probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot store-recovery probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot store-recovery probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(0);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after two prior execute-trigger breakpoint hits and then clearing only the execute-PC slot, the remaining second-half-start store-address slot still stays ignored on Spike, so the same-paddr alias store first falls back to store access fault with the original VA before the alias leaf is repaired in the mixed vector-store path" :
        "after two prior execute-trigger breakpoint hits and then clearing only the execute-PC slot, the remaining second-half-start store-address slot still stays ignored on Spike, so the same-paddr alias store first falls back to store page fault with the original VA before the alias mapping is restored in the mixed vector-store path"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias execute-trigger plus second-half-address-slot store access-fault path still leaves the vector-written boundary bytes unchanged in the mixed vector-store path" :
        "the fallback same-paddr alias execute-trigger plus second-half-address-slot store page-fault path still leaves the vector-written boundary bytes unchanged in the mixed vector-store path"),
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias execute-trigger plus second-half-address-slot store access-fault path still leaves the adjacent sentinels unchanged in the mixed vector-store path" :
            "the fallback same-paddr alias execute-trigger plus second-half-address-slot store page-fault path still leaves the adjacent sentinels unchanged in the mixed vector-store path"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf while the second-half-start store-address slot remains armed, the vector-store-then-scalar-store path commits normally again on Spike" :
        "after restoring the scalar same-paddr alias mapping while the second-half-start store-address slot remains armed, the vector-store-then-scalar-store path commits normally again on Spike"),
        excpt.triggered == false
    );
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery store sequence in the mixed vector-store path, the final boundary bytes reflect the expected overwrite order" :
        "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery store sequence in the mixed vector-store path, the final boundary bytes reflect the expected overwrite order"),
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery store sequence in the mixed vector-store path, the adjacent sentinels still match their expected values" :
            "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery store sequence in the mixed vector-store path, the adjacent sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery store sequence in the mixed vector-store path, a following scalar misaligned load still sees the latest scalar store payload immediately" :
        "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery store sequence in the mixed vector-store path, a following scalar misaligned load still sees the latest scalar store payload immediately"),
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_alias_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_completion(
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x220U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_load_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = 0x2cU;
    const uint8_t sentinel_after = 0xc2U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar-load same-paddr alias trigger probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload through a writable same-paddr alias before the scalar trigger-load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the staged payload before the scalar trigger-load probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar trigger-load probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector same-paddr alias store touches the shared path, the first scalar dual-trigger misaligned-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar same-paddr alias trigger-load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar same-paddr alias trigger-load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector same-paddr alias store, the second scalar dual-trigger misaligned-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar same-paddr alias trigger-load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar same-paddr alias trigger-load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector same-paddr alias store, clearing only the execute-PC slot still leaves the scalar misaligned-load address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the scalar same-paddr alias misaligned load still completes normally and returns the payload written earlier by the vector alias store",
        excpt.triggered == false &&
        value == 0x2e2d2c2b2a292827ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-store then scalar-load same-paddr alias trigger sequence, the shared boundary bytes still match the vector-written payload",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-store then scalar-load same-paddr alias trigger sequence, the adjacent sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_completion(
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x100U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_store_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = 0x4cU;
    const uint8_t sentinel_after = 0xd4U;
    const uint8_t scratch_before = 0x6bU;
    const uint8_t scratch_after = 0xb6U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the mixed vector-load then scalar-store trigger probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar trigger-store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar trigger-store probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar trigger-store probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after a vector same-paddr alias load touches the shared path, the first scalar dual-trigger misaligned-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar same-paddr alias trigger-store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar same-paddr alias trigger-store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the earlier vector same-paddr alias load, the second scalar dual-trigger misaligned-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar same-paddr alias trigger-store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar same-paddr alias trigger-store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector same-paddr alias load, clearing only the execute-PC slot still leaves the scalar misaligned-store address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after clearing both trigger slots, the scalar same-paddr alias misaligned store still commits normally even though a vector alias load touched the shared path earlier",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-load then scalar-store same-paddr alias trigger sequence, the final boundary bytes reflect the expected overwrite order",
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-load then scalar-store same-paddr alias trigger sequence, the shared boundary sentinels and scratch sentinels stay unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-load then scalar-store same-paddr alias trigger sequence, a following scalar misaligned load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_completion(false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_completion(true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_completion(false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_completion(true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x220U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_load_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x3cU : 0x2cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xd2U : 0xc2U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_LAF : CAUSE_LPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar-load repeated-breakpoint fault-recovery probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload through a writable same-paddr alias before the scalar repeated-breakpoint fault-recovery load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the staged payload before the scalar repeated-breakpoint fault-recovery load probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar repeated-breakpoint fault-recovery load probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector same-paddr alias store touches the shared path, the first scalar dual-trigger misaligned-load probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar same-paddr alias trigger-load probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar same-paddr alias trigger-load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector same-paddr alias store, the second scalar dual-trigger misaligned-load probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar same-paddr alias trigger-load probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar same-paddr alias trigger-load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector same-paddr alias store, clearing only the execute-PC slot still leaves the scalar misaligned-load address trigger active before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias load probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after clearing both trigger slots, the scalar same-paddr alias misaligned load first falls back to load access fault with the original VA before the alias leaf is repaired" :
        "after clearing both trigger slots, the scalar same-paddr alias misaligned load first falls back to load page fault with the original VA before the alias mapping is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias load access-fault path still leaves the vector-written boundary bytes unchanged" :
        "the fallback same-paddr alias load page-fault path still leaves the vector-written boundary bytes unchanged"),
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias load access-fault path still leaves the adjacent sentinels unchanged" :
            "the fallback same-paddr alias load page-fault path still leaves the adjacent sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf, the mixed vector-store then scalar-load path completes normally again and returns the expected overlapping payload" :
        "after restoring the scalar same-paddr alias mapping, the mixed vector-store then scalar-load path completes normally again and returns the expected overlapping payload"),
        excpt.triggered == false &&
        value == 0x2e2d2c2b2a292827ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery load sequence in the vector-store then scalar-load corner, the shared boundary bytes still match the vector-written payload" :
        "after the repeated-breakpoint plus page-fault recovery load sequence in the vector-store then scalar-load corner, the shared boundary bytes still match the vector-written payload"),
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the repeated-breakpoint plus access-fault recovery load sequence in the vector-store then scalar-load corner, the adjacent sentinels still match their expected values" :
            "after the repeated-breakpoint plus page-fault recovery load sequence in the vector-store then scalar-load corner, the adjacent sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x100U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_store_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x5cU : 0x4cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xe4U : 0xd4U;
    const uint8_t scratch_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x7bU : 0x6bU;
    const uint8_t scratch_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xc6U : 0xb6U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_SAF : CAUSE_SPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the mixed vector-load then scalar-store repeated-breakpoint fault-recovery probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar repeated-breakpoint fault-recovery store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar repeated-breakpoint fault-recovery store probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar repeated-breakpoint fault-recovery store probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after a vector same-paddr alias load touches the shared path, the first scalar dual-trigger misaligned-store probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar same-paddr alias trigger-store probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar same-paddr alias trigger-store probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the earlier vector same-paddr alias load, the second scalar dual-trigger misaligned-store probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar same-paddr alias trigger-store probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar same-paddr alias trigger-store probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector same-paddr alias load, clearing only the execute-PC slot still leaves the scalar misaligned-store address trigger active before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias store probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar same-paddr alias store probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after clearing both trigger slots, the scalar same-paddr alias misaligned store first falls back to store access fault with the original VA before the alias leaf is repaired" :
        "after clearing both trigger slots, the scalar same-paddr alias misaligned store first falls back to store page fault with the original VA before the alias mapping is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias store access-fault path still leaves the shared boundary bytes unchanged after the earlier vector alias load" :
        "the fallback same-paddr alias store page-fault path still leaves the shared boundary bytes unchanged after the earlier vector alias load"),
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias store access-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged" :
            "the fallback same-paddr alias store page-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf, the mixed vector-load then scalar-store path commits normally again" :
        "after restoring the scalar same-paddr alias mapping, the mixed vector-load then scalar-store path commits normally again"),
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery store sequence in the vector-load then scalar-store corner, the final boundary bytes reflect the expected overwrite order" :
        "after the repeated-breakpoint plus page-fault recovery store sequence in the vector-load then scalar-store corner, the final boundary bytes reflect the expected overwrite order"),
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the repeated-breakpoint plus access-fault recovery store sequence in the vector-load then scalar-store corner, the shared boundary sentinels and scratch sentinels still match their expected values" :
            "after the repeated-breakpoint plus page-fault recovery store sequence in the vector-load then scalar-store corner, the shared boundary sentinels and scratch sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery store sequence in the vector-load then scalar-store corner, a following scalar misaligned load still sees the latest scalar store payload immediately" :
        "after the repeated-breakpoint plus page-fault recovery store sequence in the vector-load then scalar-store corner, a following scalar misaligned load still sees the latest scalar store payload immediately"),
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x220U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x3cU : 0x2cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xd2U : 0xc2U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_LAF : CAUSE_LPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the same-paddr alias dual-address-trigger load-recovery probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload through a writable same-paddr alias before the scalar dual-address-trigger load-recovery probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the staged payload before the scalar dual-address-trigger load-recovery probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar dual-address-trigger load-recovery probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("with both the original misaligned scalar same-paddr alias load slot and the true second-half-start slot armed, the first vector-store-then-scalar-load probe still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias dual-address-trigger load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias dual-address-trigger load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("with the same original misaligned-load slot and second-half-start slot still armed, the second vector-store-then-scalar-load probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(0);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the same-paddr alias load first falls back to load access fault with the original VA before the alias leaf is repaired" :
        "after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the same-paddr alias load first falls back to load page fault with the original VA before the alias mapping is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias dual-address-trigger load access-fault path still leaves the vector-written boundary bytes unchanged" :
        "the fallback same-paddr alias dual-address-trigger load page-fault path still leaves the vector-written boundary bytes unchanged"),
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias dual-address-trigger load access-fault path still leaves the adjacent sentinels unchanged" :
            "the fallback same-paddr alias dual-address-trigger load page-fault path still leaves the adjacent sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf while the second-half-start slot remains armed, the vector-store-then-scalar-load path completes normally again on Spike" :
        "after restoring the scalar same-paddr alias mapping while the second-half-start slot remains armed, the vector-store-then-scalar-load path completes normally again on Spike"),
        excpt.triggered == false &&
        value == 0x2e2d2c2b2a292827ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the same-paddr alias dual-address-trigger access-fault recovery load sequence, the shared boundary bytes still match the vector-written payload" :
        "after the same-paddr alias dual-address-trigger page-fault recovery load sequence, the shared boundary bytes still match the vector-written payload"),
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the same-paddr alias dual-address-trigger access-fault recovery load sequence, the adjacent sentinels still match their expected values" :
            "after the same-paddr alias dual-address-trigger page-fault recovery load sequence, the adjacent sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x100U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x5cU : 0x4cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xe4U : 0xd4U;
    const uint8_t scratch_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x7bU : 0x6bU;
    const uint8_t scratch_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xc6U : 0xb6U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_SAF : CAUSE_SPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the scalar dual-address-trigger store-recovery probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar dual-address-trigger store-recovery probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar dual-address-trigger store-recovery probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar dual-address-trigger store-recovery probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, scalar_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both the original misaligned scalar same-paddr alias store slot and the true second-half-start slot armed, the first vector-load-then-scalar-store probe still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias dual-address-trigger store probe still leaves the shared boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias dual-address-trigger store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with the same original misaligned-store slot and second-half-start slot still armed, the second vector-load-then-scalar-store probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger store probe still leaves the shared boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(0);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the same-paddr alias store first falls back to store access fault with the original VA before the alias leaf is repaired" :
        "after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the same-paddr alias store first falls back to store page fault with the original VA before the alias mapping is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias dual-address-trigger store access-fault path still leaves the shared boundary bytes unchanged" :
        "the fallback same-paddr alias dual-address-trigger store page-fault path still leaves the shared boundary bytes unchanged"),
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias dual-address-trigger store access-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged" :
            "the fallback same-paddr alias dual-address-trigger store page-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf while the second-half-start slot remains armed, the vector-load-then-scalar-store path commits normally again on Spike" :
        "after restoring the scalar same-paddr alias mapping while the second-half-start slot remains armed, the vector-load-then-scalar-store path commits normally again on Spike"),
        excpt.triggered == false
    );
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the same-paddr alias dual-address-trigger access-fault recovery store sequence, the final boundary bytes reflect the expected overwrite order" :
        "after the same-paddr alias dual-address-trigger page-fault recovery store sequence, the final boundary bytes reflect the expected overwrite order"),
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the same-paddr alias dual-address-trigger access-fault recovery store sequence, the shared boundary sentinels and scratch sentinels still match their expected values" :
            "after the same-paddr alias dual-address-trigger page-fault recovery store sequence, the shared boundary sentinels and scratch sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after the same-paddr alias dual-address-trigger access-fault recovery store sequence, a following scalar misaligned load still sees the latest scalar store payload immediately" :
        "after the same-paddr alias dual-address-trigger page-fault recovery store sequence, a following scalar misaligned load still sees the latest scalar store payload immediately"),
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_completion(
    enum ai_micro_it_alias_dual_address_probe_mode mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x220U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = 0x2cU;
    const uint8_t sentinel_after = 0xc2U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the same-paddr alias dual-address-trigger completion probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload through a writable same-paddr alias before the scalar dual-address-trigger completion probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the staged payload before the scalar dual-address-trigger completion probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar dual-address-trigger completion probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    if (mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY) {
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, second_half_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    } else {
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, scalar_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    }
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    if (mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY) {
        TEST_ASSERT("on Spike, arming only the true second-half-start load address slot in the same-paddr alias mixed vector-store-then-scalar-load path does not match; the scalar load still completes and returns the vector-written payload",
            excpt.triggered == false &&
            value == 0x2e2d2c2b2a292827ULL
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    } else {
        const char *first_breakpoint_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "with both the original misaligned scalar same-paddr alias load slot and the true second-half-start slot armed, Spike still reports breakpoint because only the original address matches" :
            "with both split-load address slots armed in the same-paddr alias mixed vector-store-then-scalar-load path, the first scalar load still reports breakpoint on Spike because only the original misaligned vaddr matches";
        TEST_ASSERT(first_breakpoint_msg,
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);

        goto_priv(PRIV_M);
        const char *first_load_state_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "the trapped same-paddr alias dual-address-trigger load probe still leaves the vector-written boundary bytes unchanged before the original slot is cleared" :
            "the first trapped same-paddr alias dual-address-trigger load probe still leaves the vector-written boundary bytes unchanged before the later completion path";
        TEST_ASSERT(first_load_state_msg,
            ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
        );
        if (check_adjacent) {
            const char *first_load_adjacent_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
                "the trapped same-paddr alias dual-address-trigger load probe still leaves the adjacent sentinels unchanged before the original slot is cleared" :
                "the first trapped same-paddr alias dual-address-trigger load probe still leaves the adjacent sentinels unchanged before the later completion path";
            TEST_ASSERT(first_load_adjacent_msg,
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after
            );
        }

        if (mode == AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION) {
            saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            ai_micro_it_alias_execute_load_once(scalar_vaddr);
            TEST_ASSERT("repeating the same scalar same-paddr alias load with both address slots still armed reports breakpoint again and still keeps the second-half-start slot ignored on Spike",
                excpt.triggered == true &&
                excpt.cause == CAUSE_BKP
            );
            ai_micro_it_alias_restore_machine_status(saved_mstatus);

            goto_priv(PRIV_M);
            TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger load probe still leaves the vector-written boundary bytes unchanged before completion",
                ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
            );
            if (check_adjacent) {
                TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger load probe still leaves the adjacent sentinels unchanged before completion",
                    read8(sentinel_before_addr) == sentinel_before &&
                    read8(sentinel_after_addr) == sentinel_after
                );
            }
        }

        ai_micro_it_alias_clear_mcontrol6_slot(0);
        saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
        const char *completion_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "after clearing the original misaligned-load slot, the remaining second-half-start slot stays ignored on Spike and the same-paddr alias scalar load completes normally" :
            "after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the same-paddr alias scalar load completes normally";
        TEST_ASSERT(completion_msg,
            excpt.triggered == false &&
            value == 0x2e2d2c2b2a292827ULL
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    }

    goto_priv(PRIV_M);
    const char *final_load_state_msg = mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY ?
        "the same-paddr alias scalar load path with only the second-half-start address slot armed still leaves the vector-written boundary bytes intact" :
        "after the same-paddr alias dual-address-trigger load completion path, the shared boundary bytes still match the vector-written payload";
    TEST_ASSERT(final_load_state_msg,
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        const char *final_load_adjacent_msg = mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY ?
            "the same-paddr alias scalar load path with only the second-half-start address slot armed still leaves the adjacent sentinels unchanged" :
            "after the same-paddr alias dual-address-trigger load completion path, the adjacent sentinels still match their expected values";
        TEST_ASSERT(final_load_adjacent_msg,
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_completion(
    enum ai_micro_it_alias_dual_address_probe_mode mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x100U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = 0x4cU;
    const uint8_t sentinel_after = 0xd4U;
    const uint8_t scratch_before = 0x6bU;
    const uint8_t scratch_after = 0xb6U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the scalar dual-address-trigger store completion probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar dual-address-trigger store completion probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar dual-address-trigger store completion probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar dual-address-trigger store completion probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    if (mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY) {
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, second_half_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    } else {
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(0, scalar_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
        ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
            AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    }
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    if (mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY) {
        TEST_ASSERT("on Spike, arming only the true second-half-start store address slot in the same-paddr alias mixed vector-load-then-scalar-store path does not match; the scalar store still commits normally",
            excpt.triggered == false
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    } else {
        const char *first_store_breakpoint_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "with both the original misaligned scalar same-paddr alias store slot and the true second-half-start slot armed, Spike still reports breakpoint before any bytes commit because only the original address matches" :
            "with both split-store address slots armed in the same-paddr alias mixed vector-load-then-scalar-store path, the first scalar store still reports breakpoint on Spike because only the original misaligned vaddr matches";
        TEST_ASSERT(first_store_breakpoint_msg,
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);

        goto_priv(PRIV_M);
        const char *first_store_state_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "the trapped same-paddr alias dual-address-trigger store probe still leaves the shared boundary bytes unchanged before the original slot is cleared" :
            "the first trapped same-paddr alias dual-address-trigger store probe still leaves the shared boundary bytes unchanged before the later completion path";
        TEST_ASSERT(first_store_state_msg,
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
        );
        if (check_adjacent) {
            const char *first_store_adjacent_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
                "the trapped same-paddr alias dual-address-trigger store probe still leaves the adjacent sentinels unchanged before the original slot is cleared" :
                "the first trapped same-paddr alias dual-address-trigger store probe still leaves the adjacent sentinels unchanged before the later completion path";
            TEST_ASSERT(first_store_adjacent_msg,
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after
            );
        }

        if (mode == AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION) {
            saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
            TEST_ASSERT("repeating the same scalar same-paddr alias store with both address slots still armed reports breakpoint again and still keeps the second-half-start slot ignored on Spike before any bytes commit",
                excpt.triggered == true &&
                excpt.cause == CAUSE_BKP
            );
            ai_micro_it_alias_restore_machine_status(saved_mstatus);

            goto_priv(PRIV_M);
            TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger store probe still leaves the shared boundary bytes unchanged before completion",
                ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
            );
            if (check_adjacent) {
                TEST_ASSERT("the second trapped same-paddr alias dual-address-trigger store probe still leaves the adjacent sentinels unchanged before completion",
                    read8(sentinel_before_addr) == sentinel_before &&
                    read8(sentinel_after_addr) == sentinel_after
                );
            }
        }

        ai_micro_it_alias_clear_mcontrol6_slot(0);
        saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
        const char *store_completion_msg = mode == AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES ?
            "after clearing the original misaligned-store slot, the remaining second-half-start slot stays ignored on Spike and the same-paddr alias scalar store commits normally" :
            "after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the same-paddr alias scalar store commits normally";
        TEST_ASSERT(store_completion_msg,
            excpt.triggered == false
        );
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    }

    goto_priv(PRIV_M);
    const char *final_store_state_msg = mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY ?
        "the same-paddr alias scalar store path with only the second-half-start address slot armed still reflects the expected overwrite order" :
        "after the same-paddr alias dual-address-trigger store completion path, the final boundary bytes reflect the expected overwrite order";
    TEST_ASSERT(final_store_state_msg,
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        const char *final_store_adjacent_msg = mode == AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY ?
            "the same-paddr alias scalar store path with only the second-half-start address slot armed still leaves the shared boundary sentinels and scratch sentinels unchanged" :
            "after the same-paddr alias dual-address-trigger store completion path, the shared boundary sentinels and scratch sentinels still match their expected values";
        TEST_ASSERT(final_store_adjacent_msg,
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the same-paddr alias dual-address-trigger store completion path, a following scalar misaligned load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_second_half_address_trigger_ignored_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_only_original_matches_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_second_half_address_trigger_ignored_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_SECOND_HALF_SLOT_ONLY, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_only_original_matches_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_ONLY_ORIGINAL_MATCHES, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_dual_address_trigger_completion(
            AI_MICRO_IT_ALIAS_REPEATED_BREAKPOINT_THEN_COMPLETION, true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_store_execute_trigger_second_half_address_slot_completion(
    bool repeat_breakpoint,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x220U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_load_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = 0x2cU;
    const uint8_t sentinel_after = 0xc2U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the same-paddr alias execute-trigger plus second-half-address-slot load probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload through a writable same-paddr alias before the scalar execute-trigger plus second-half-address-slot load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the staged payload before the scalar execute-trigger plus second-half-address-slot load probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar execute-trigger plus second-half-address-slot load probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half-start load address, the mixed vector-store-then-scalar-load same-paddr alias probe still reports breakpoint first on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    if (repeat_breakpoint) {
        saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_it_alias_execute_load_once(scalar_vaddr);
        TEST_ASSERT("with the same execute-PC slot and true second-half-start load-address slot still armed, the second mixed same-paddr alias load attempt still reports breakpoint on Spike",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot load probe still leaves the vector-written boundary bytes unchanged",
            ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
        );
        if (check_adjacent) {
            TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot load probe still leaves the adjacent sentinels unchanged",
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after
            );
        }
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    if (repeat_breakpoint) {
        TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the true second-half-start load-address slot ignored on Spike, so the same-paddr alias scalar load completes normally",
            excpt.triggered == false &&
            value == 0x2e2d2c2b2a292827ULL
        );
    } else {
        TEST_ASSERT("after clearing the execute-PC slot, the true second-half-start load-address slot does not keep matching on Spike; the same-paddr alias scalar load completes normally",
            excpt.triggered == false &&
            value == 0x2e2d2c2b2a292827ULL
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot load completion path, the shared boundary bytes still match the vector-written payload",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot load completion path, the adjacent sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_load_execute_trigger_second_half_address_slot_completion(
    bool repeat_breakpoint,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x100U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_store_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = 0x4cU;
    const uint8_t sentinel_after = 0xd4U;
    const uint8_t scratch_before = 0x6bU;
    const uint8_t scratch_after = 0xb6U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the scalar execute-trigger plus second-half-address-slot store probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar execute-trigger plus second-half-address-slot store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar execute-trigger plus second-half-address-slot store probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar execute-trigger plus second-half-address-slot store probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half-start store address, the mixed vector-load-then-scalar-store same-paddr alias probe still reports breakpoint first on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot store probe still leaves the shared boundary sentinels unchanged after the prior vector load",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    if (repeat_breakpoint) {
        saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
        TEST_ASSERT("with the same execute-PC slot and true second-half-start store-address slot still armed, the second mixed same-paddr alias store attempt still reports breakpoint on Spike",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot store probe still leaves the shared boundary bytes unchanged after the prior vector load",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
        );
        if (check_adjacent) {
            TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot store probe still leaves the shared boundary sentinels unchanged after the prior vector load",
                read8(sentinel_before_addr) == sentinel_before &&
                read8(sentinel_after_addr) == sentinel_after
            );
        }
        ai_micro_it_alias_restore_machine_status(saved_mstatus);
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    if (repeat_breakpoint) {
        TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the true second-half-start store-address slot ignored on Spike, so the same-paddr alias scalar store commits normally",
            excpt.triggered == false
        );
    } else {
        TEST_ASSERT("after clearing the execute-PC slot, the true second-half-start store-address slot does not keep matching on Spike; the same-paddr alias scalar store commits normally",
            excpt.triggered == false
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot store completion path, the final boundary bytes reflect the expected overwrite order",
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot store completion path, the shared boundary sentinels and scratch sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the same-paddr alias execute-trigger plus second-half-address-slot store completion path, a following scalar misaligned load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_ignored_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_trigger_second_half_address_slot_completion(
            false, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_trigger_second_half_address_slot_completion(
            true, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_trigger_second_half_address_slot_completion(
            true, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_ignored_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_trigger_second_half_address_slot_completion(
            false, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_trigger_second_half_address_slot_completion(
            true, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_trigger_second_half_address_slot_completion(
            true, true);
    TEST_END();
}

static bool ai_micro_it_alias_scalar_load_after_vector_store_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page source_page = VSRWX_GRWX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t source_offset = 0x220U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t source_vaddr = hs_page_base(source_page) + source_offset;
    const uintptr_t source_phys = phys_page_base(source_page) + source_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_load_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(source_pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x3cU : 0x2cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xd2U : 0xc2U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_LAF : CAUSE_LPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(source_phys, source_pattern, sizeof(source_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the same-paddr alias execute-trigger plus second-half-address-slot load-recovery probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload through a writable same-paddr alias before the scalar execute-trigger plus second-half-address-slot load-recovery probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector same-paddr alias store, the shared boundary bytes match the staged payload before the scalar execute-trigger plus second-half-address-slot load-recovery probe",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias store, the adjacent sentinels stay unchanged before the scalar execute-trigger plus second-half-address-slot load-recovery probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the true second-half-start load-address slot also armed, the first vector-store-then-scalar-load recovery probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot load-recovery probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot load-recovery probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT("with the same execute-PC slot and true second-half-start load-address slot still armed, the second vector-store-then-scalar-load recovery probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot load-recovery probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot load-recovery probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(0);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after two prior execute-trigger breakpoint hits and then clearing only the execute-PC slot, the remaining second-half-start load-address slot still stays ignored on Spike, so the same-paddr alias load first falls back to load access fault with the original VA before the alias leaf is repaired" :
        "after two prior execute-trigger breakpoint hits and then clearing only the execute-PC slot, the remaining second-half-start load-address slot still stays ignored on Spike, so the same-paddr alias load first falls back to load page fault with the original VA before the alias mapping is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias execute-trigger plus second-half-address-slot load access-fault path still leaves the vector-written boundary bytes unchanged" :
        "the fallback same-paddr alias execute-trigger plus second-half-address-slot load page-fault path still leaves the vector-written boundary bytes unchanged"),
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias execute-trigger plus second-half-address-slot load access-fault path still leaves the adjacent sentinels unchanged" :
            "the fallback same-paddr alias execute-trigger plus second-half-address-slot load page-fault path still leaves the adjacent sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_it_alias_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf while the second-half-start load-address slot remains armed, the vector-store-then-scalar-load path completes normally again on Spike" :
        "after restoring the scalar same-paddr alias mapping while the second-half-start load-address slot remains armed, the vector-store-then-scalar-load path completes normally again on Spike"),
        excpt.triggered == false &&
        value == 0x2e2d2c2b2a292827ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery load sequence, the shared boundary bytes still match the vector-written payload" :
        "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery load sequence, the shared boundary bytes still match the vector-written payload"),
        ai_micro_it_alias_bytes_match(boundary_phys, source_pattern, sizeof(source_pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery load sequence, the adjacent sentinels still match their expected values" :
            "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery load sequence, the adjacent sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_it_alias_scalar_store_after_vector_load_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_it_alias_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    enum test_page shared_page = VSRW_GRW;
    enum test_page scalar_alias_page = SWITCH1;
    enum test_page vector_alias_page = SWITCH2;
    enum test_page scratch_page = VSRWX_GRWX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    const uintptr_t boundary_offset = 0x1b0U;
    const uintptr_t scalar_offset = boundary_offset + 7U;
    const uintptr_t scratch_offset = 0x100U;
    const uintptr_t vector_vaddr = hs_page_base(vector_alias_page) + boundary_offset;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_alias_page) + scalar_offset;
    const uintptr_t second_half_vaddr = ai_micro_it_alias_second_half_addr(scalar_vaddr);
    const uintptr_t boundary_phys = phys_page_base(shared_page) + boundary_offset;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + scratch_offset;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + scratch_offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_it_alias_execute_store_once;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + sizeof(pattern);
    const uint8_t sentinel_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x5cU : 0x4cU;
    const uint8_t sentinel_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xe4U : 0xd4U;
    const uint8_t scratch_before = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0x7bU : 0x6bU;
    const uint8_t scratch_after = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT ? 0xc6U : 0xb6U;
    const bool use_access_fault = fault_mode == AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT;
    const uint64_t expected_cause = use_access_fault ? CAUSE_SAF : CAUSE_SPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_it_alias_write_bytes(boundary_phys, pattern, sizeof(pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + sizeof(pattern), scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte same-paddr alias payload before the scalar execute-trigger plus second-half-address-slot store-recovery probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector same-paddr alias payload into a scratch page preserves the original 16-byte byte sequence before the scalar execute-trigger plus second-half-address-slot store-recovery probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector same-paddr alias load still matches the exact boundary payload before the scalar execute-trigger plus second-half-address-slot store-recovery probe",
        ai_micro_it_alias_bytes_match(scratch_phys, pattern, sizeof(pattern)) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + sizeof(pattern)) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector same-paddr alias load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar execute-trigger plus second-half-address-slot store-recovery probe",
            ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(0);
    ai_micro_it_alias_clear_mcontrol6_slot(1);
    ai_micro_it_alias_arm_mcontrol6_slot(0, trigger_pc);
    ai_micro_it_alias_arm_mcontrol6_slot_with_access(1, second_half_vaddr,
        AI_MICRO_IT_ALIAS_MCONTROL6_STORE);
    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the true second-half-start store-address slot also armed, the first vector-load-then-scalar-store recovery probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot store-recovery probe still leaves the shared boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped same-paddr alias execute-trigger plus second-half-address-slot store-recovery probe still leaves the shared boundary sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with the same execute-PC slot and true second-half-start store-address slot still armed, the second vector-load-then-scalar-store recovery probe still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot store-recovery probe still leaves the shared boundary bytes unchanged",
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped same-paddr alias execute-trigger plus second-half-address-slot store-recovery probe still leaves the shared boundary sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    ai_micro_it_alias_clear_mcontrol6_slot(0);
    if (use_access_fault) {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) |= (1ULL << 46);
    } else {
        *ai_micro_it_alias_hs_leaf_entry(scalar_alias_page) = 0;
    }
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after two prior execute-trigger breakpoint hits and then clearing only the execute-PC slot, the remaining second-half-start store-address slot still stays ignored on Spike, so the same-paddr alias store first falls back to store access fault with the original VA before the alias leaf is repaired" :
        "after two prior execute-trigger breakpoint hits and then clearing only the execute-PC slot, the remaining second-half-start store-address slot still stays ignored on Spike, so the same-paddr alias store first falls back to store page fault with the original VA before the alias mapping is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback same-paddr alias execute-trigger plus second-half-address-slot store access-fault path still leaves the shared boundary bytes unchanged" :
        "the fallback same-paddr alias execute-trigger plus second-half-address-slot store page-fault path still leaves the shared boundary bytes unchanged"),
        ai_micro_it_alias_bytes_match(boundary_phys, pattern, sizeof(pattern))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback same-paddr alias execute-trigger plus second-half-address-slot store access-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged" :
            "the fallback same-paddr alias execute-trigger plus second-half-address-slot store page-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    ai_micro_it_alias_set_hs_leaf(scalar_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_it_alias_set_hs_leaf(vector_alias_page, phys_page_base(shared_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    saved_mstatus = ai_micro_it_alias_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_it_alias_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT((use_access_fault ?
        "after repairing the scalar same-paddr alias leaf while the second-half-start store-address slot remains armed, the vector-load-then-scalar-store path commits normally again on Spike" :
        "after restoring the scalar same-paddr alias mapping while the second-half-start store-address slot remains armed, the vector-load-then-scalar-store path commits normally again on Spike"),
        excpt.triggered == false
    );
    ai_micro_it_alias_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery store sequence, the final boundary bytes reflect the expected overwrite order" :
        "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery store sequence, the final boundary bytes reflect the expected overwrite order"),
        ai_micro_it_alias_bytes_match(boundary_phys, expected_final, sizeof(expected_final))
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery store sequence, the shared boundary sentinels and scratch sentinels still match their expected values" :
            "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery store sequence, the shared boundary sentinels and scratch sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + sizeof(pattern)) == scratch_after
        );
    }

    ai_micro_it_alias_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after the same-paddr alias execute-trigger plus second-half-address-slot access-fault recovery store sequence, a following scalar misaligned load still sees the latest scalar store payload immediately" :
        "after the same-paddr alias execute-trigger plus second-half-address-slot page-fault recovery store sequence, a following scalar misaligned load still sees the latest scalar store payload immediately"),
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_load_after_vector_store_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_it_alias_scalar_store_after_vector_load_execute_trigger_second_half_address_slot_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_IT_ALIAS_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}
