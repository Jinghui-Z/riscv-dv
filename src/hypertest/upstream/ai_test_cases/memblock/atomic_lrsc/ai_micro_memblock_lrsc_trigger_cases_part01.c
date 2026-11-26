#include "ai_micro_memblock_lrsc_trigger_common.h"

bool ai_micro_memblock_lr_load_trigger_breakpoint_does_not_create_reservation_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x300U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x300U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0xfeedfacecafebeefULL;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    const uint64_t marker_seed = 0x0102030405060708ULL;
    const uint64_t no_reservation_value = 0xa5a55a5adeadbeefULL;
    const uint64_t final_value = 0x13579bdf2468ace0ULL;
    const uint64_t stale_value = 0x0badc0ffee0dd00dULL;
    const uint64_t fault_lr_sentinel = 0xf17f17f17f17f17fULL;
    const uint64_t post_fault_sc_value = 0x8182838485868788ULL;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    bool lr_breakpoint_seen =
        excpt.triggered == true && excpt.cause == CAUSE_BKP;
    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool breakpoint_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;

    TEST_ASSERT("lr.d hitting the load trigger reports breakpoint and leaves target, adjacent, guard, and marker words unchanged",
        lr_breakpoint_seen &&
        breakpoint_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t no_reservation_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, no_reservation_value);
    TEST_ASSERT("after clearing the load trigger, a same-address sc.d without a fresh lr.d fails because the breakpoint-suppressed lr.d did not create a reservation",
        excpt.triggered == false &&
        no_reservation_status != 0U &&
        read64(target_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t fresh_reserved =
        ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t final_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, final_value);
    TEST_ASSERT("a fresh lr.d after the breakpoint path captures the unchanged target and the following sc.d succeeds",
        excpt.triggered == false &&
        fresh_reserved == initial_value &&
        final_status == 0U &&
        read64(target_vaddr) == final_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, stale_value);
    TEST_ASSERT("the successful fresh sc.d consumes the reservation, so a second naked sc.d fails and preserves the committed value",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value
    );

    ai_memblock_lrsc_trigger_set_hs_leaf(page, PTE_V | PTE_AD | PTE_X);
    TEST_SETUP_EXCEPT();
    uint64_t fault_lr =
        ai_memblock_lrsc_trigger_lr_d64_seed(target_vaddr, fault_lr_sentinel);
    TEST_ASSERT("after clearing the LR load trigger, removing read permission exposes the underlying LR load page fault with no rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == target_vaddr &&
        fault_lr == fault_lr_sentinel
    );

    ai_memblock_lrsc_trigger_set_hs_leaf(page, PTE_V | PTE_AD | PTE_RWX);
    TEST_SETUP_EXCEPT();
    uint32_t post_fault_sc_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, post_fault_sc_value);
    TEST_ASSERT("the post-clear faulting LR also does not create a reservation, so repairing the page and issuing SC without a fresh LR fails and preserves the committed target",
        excpt.triggered == false &&
        post_fault_sc_status != 0U &&
        read64(target_vaddr) == final_value
    );

    TEST_ASSERT("the lr.d breakpoint no-reservation corner preserves all non-target sentinels",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the fresh post-breakpoint sc.d payload and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_lr_store_trigger_does_not_fire_on_lr_then_fresh_lrsc_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x340U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x340U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6574616d6f746963ULL;
    const uint64_t initial_value = 0x0101020203030404ULL;
    const uint64_t adjacent_seed = 0xfeed1234cafe5678ULL;
    const uint64_t marker_seed = 0x1111222233334444ULL;
    const uint64_t final_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t store_trigger_lr_value =
        ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the LR target does not fire on lr.d and the load still returns the original doubleword",
        excpt.triggered == false &&
        store_trigger_lr_value == initial_value &&
        read64(target_vaddr) == initial_value
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t fresh_lr_value =
        ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t final_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, final_value);
    TEST_ASSERT("after clearing the store-only trigger, a fresh same-address lr.d/sc.d pair succeeds and writes the expected payload",
        excpt.triggered == false &&
        fresh_lr_value == initial_value &&
        final_status == 0U &&
        read64(target_vaddr) == final_value
    );

    TEST_ASSERT("the store-trigger classification probe preserves all non-target sentinels in HS view",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear fresh sc.d update and keeps guard, adjacent, and marker words unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_load_trigger_breakpoint_suppresses_rmw_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x380U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x380U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616d6f6c6f616474ULL;
    const uint64_t initial_value = 0x2024051101020304ULL;
    const uint64_t adjacent_seed = 0xabcd555544443333ULL;
    const uint64_t marker_seed = 0x9999888877776666ULL;
    const uint64_t blocked_addend = 0x1111111111111111ULL;
    const uint64_t final_addend = 0x0102030405060708ULL;
    const uint64_t final_value = initial_value + final_addend;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_d(target_vaddr, blocked_addend);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the AMO target reports breakpoint and suppresses the amoadd.d read-modify-write",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amoadd.d breakpoint leaves target, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoadd_d(target_vaddr, final_addend);
    TEST_ASSERT("after clearing the load-only trigger, amoadd.d returns the original doubleword and commits the expected 64-bit RMW result",
        excpt.triggered == false &&
        amo_old == initial_value &&
        read64(target_vaddr) == final_value
    );

    TEST_ASSERT("the load-trigger AMO breakpoint probe preserves all non-target sentinels in HS view",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoadd.d payload and keeps sentinels unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_store_trigger_breakpoint_suppresses_rmw_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x3c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x3c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x73746f726d61726bULL;
    const uint64_t initial_value = 0x30405060708090a0ULL;
    const uint64_t adjacent_seed = 0x7766554433221100ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t blocked_addend = 0x2222222222222222ULL;
    const uint64_t final_addend = 0x0102040810204080ULL;
    const uint64_t final_value = initial_value + final_addend;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_d(target_vaddr, blocked_addend);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the AMO target reports breakpoint and suppresses the amoadd.d read-modify-write",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amoadd.d breakpoint leaves target, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoadd_d(target_vaddr, final_addend);
    TEST_ASSERT("after clearing the store-only trigger, amoadd.d returns the original doubleword and commits the expected 64-bit RMW result",
        excpt.triggered == false &&
        amo_old == initial_value &&
        read64(target_vaddr) == final_value
    );

    TEST_ASSERT("the store-trigger AMO breakpoint probe preserves all non-target sentinels in HS view",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoadd.d payload and keeps sentinels unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_sc_store_trigger_breakpoint_blocks_commit_then_fresh_lrsc_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x400U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x400U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7363747269676772ULL;
    const uint64_t initial_value = 0x5566778899aabbccULL;
    const uint64_t adjacent_seed = 0x0c0d0e0f10111213ULL;
    const uint64_t marker_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t blocked_sc_value = 0xdeadbeef10203040ULL;
    const uint64_t final_sc_value = 0x2468ace013579bdfULL;
    const uint64_t stale_sc_value = 0x13579bdf02468aceULL;
    const uint64_t fault_sc_value = 0xbad0bad0bad0bad0ULL;
    const uint64_t fault_status_sentinel = 0x5c5c5c5c5c5c5c5cULL;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved =
        ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    TEST_ASSERT("with a store-only mcontrol6 address trigger already armed, lr.d still establishes the reservation and leaves the target unchanged",
        excpt.triggered == false &&
        reserved == initial_value &&
        read64(target_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    (void)ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, blocked_sc_value);
    bool sc_breakpoint_seen =
        excpt.triggered == true && excpt.cause == CAUSE_BKP;
    TEST_ASSERT("a store-only mcontrol6 address trigger reports breakpoint on sc.d before the conditional store can retire",
        sc_breakpoint_seen
    );

    goto_priv(PRIV_M);
    bool breakpoint_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered sc.d breakpoint blocks the conditional store before any sentinel changes",
        breakpoint_preserved_words
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    ai_memblock_lrsc_trigger_set_hs_leaf(page, PTE_V | PTE_AD | PTE_R);
    TEST_SETUP_EXCEPT();
    uint64_t fault_status =
        ai_memblock_lrsc_trigger_sc_d_status_seed(target_vaddr,
            fault_sc_value, fault_status_sentinel);
    TEST_ASSERT("after clearing the SC store trigger, removing write permission exposes the underlying SC store page fault before rd status or memory can change",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == target_vaddr &&
        fault_status == fault_status_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-clear faulting sc.d leaves target, guard, adjacent, and marker words unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_set_hs_leaf(page, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t fresh_reserved =
        ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t final_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, final_sc_value);
    TEST_ASSERT("after clearing the store-only trigger, a fresh same-address lr.d/sc.d pair recovers and commits exactly one final payload",
        excpt.triggered == false &&
        fresh_reserved == initial_value &&
        final_status == 0U &&
        read64(target_vaddr) == final_sc_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, stale_sc_value);
    TEST_ASSERT("the post-clear successful sc.d consumes the reservation, so a following naked sc.d fails and preserves the committed value",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_sc_value
    );

    TEST_ASSERT("the store-triggered sc.d breakpoint probe preserves all non-target sentinels in HS view",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear sc.d payload and keeps sentinels unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_sc_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_sc_load_trigger_does_not_fire_on_sc_then_fresh_lrsc_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = VSRWX_GX;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x440U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x440U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x440U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t target_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6c6f616474726967ULL;
    const uint64_t initial_value = 0x8899aabbccddeeffULL;
    const uint64_t adjacent_seed = 0x1021324354657687ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t armed_sc_value = 0x2468ace013579bdfULL;
    const uint64_t stale_sc_value = 0x55aa33cc77ee11ffULL;
    const uint64_t post_clear_value = 0x13579bdf02468aceULL;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_memblock_lrsc_trigger_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    /* Keep the same-set VA aliases resident before the load-only trigger is armed. */
    (void)read64(target_alias_vaddr);
    (void)read64(target_base_vaddr);

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_base_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved =
        ai_memblock_lrsc_trigger_lr_d64(target_alias_vaddr);
    uint32_t armed_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_base_vaddr, armed_sc_value);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the SC target does not fire on sc.d, so a same-set alias lr.d/base sc.d pair can still consume the reservation and commit",
        excpt.triggered == false &&
        reserved == initial_value &&
        armed_status == 0U &&
        read64(target_alias_vaddr) == armed_sc_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_base_vaddr, stale_sc_value);
    TEST_ASSERT("with the load-only trigger still armed, a second naked sc.d does not report breakpoint and fails after the successful SC consumed the reservation",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_alias_vaddr) == armed_sc_value
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t fresh_reserved =
        ai_memblock_lrsc_trigger_lr_d64(target_base_vaddr);
    uint32_t post_clear_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_base_vaddr, post_clear_value);
    TEST_ASSERT("after clearing the load-only trigger, a fresh same-address lr.d/sc.d pair still captures the committed value and updates it once",
        excpt.triggered == false &&
        fresh_reserved == armed_sc_value &&
        post_clear_status == 0U &&
        read64(target_alias_vaddr) == post_clear_value
    );

    TEST_ASSERT("the sc.d load-trigger negative-classification probe preserves all non-target sentinels in HS view",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the armed sc.d payload followed by the post-clear fresh sc.d payload, with all sentinels unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == post_clear_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_load_store_chain_breakpoint_suppresses_rmw_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x480U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x480U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x636861696e677264ULL;
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t adjacent_seed = 0x8899aabbccddeeffULL;
    const uint64_t marker_seed = 0x0a0b0c0d0e0f1011ULL;
    const uint64_t lone_load_addend = 0x0102030405060708ULL;
    const uint64_t chain_base_value = initial_value + lone_load_addend;
    const uint64_t blocked_addend = 0x1111111111111111ULL;
    const uint64_t repeated_blocked_addend = 0x2222222222222222ULL;
    const uint64_t post_clear_addend = 0x0001000100010001ULL;
    const uint64_t post_clear_value = chain_base_value + post_clear_addend;
    const uint64_t final_addend = 0x0101010101010101ULL;
    const uint64_t final_value = post_clear_value + final_addend;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, adjacent_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t lone_load_old = amoadd_d(target_vaddr, lone_load_addend);
    TEST_ASSERT("with mcontrol6 slot0 load chained to a nonmatching store slot1, amoadd.d must not report breakpoint and must commit exactly one RMW",
        excpt.triggered == false &&
        lone_load_old == initial_value &&
        read64(target_vaddr) == chain_base_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );
    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);

    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    saved_mstatus = ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_d(target_vaddr, blocked_addend);
    TEST_ASSERT("when the same amoadd.d hits both the chained load slot0 and store slot1, breakpoint is reported before the RMW can update memory",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(target_vaddr) == chain_base_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );
    ai_memblock_lrsc_trigger_reenable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_d(target_vaddr, repeated_blocked_addend);
    TEST_ASSERT("a repeated hit on the same load-store mcontrol6 chain still reports breakpoint and leaves target plus sentinels unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(target_vaddr) == chain_base_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );
    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);

    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);
    saved_mstatus = ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t post_clear_old = amoadd_d(target_vaddr, post_clear_addend);
    TEST_ASSERT("after clearing only the store slot, the remaining chained load slot cannot fire alone and the same amoadd.d commits normally",
        excpt.triggered == false &&
        post_clear_old == chain_base_value &&
        read64(target_vaddr) == post_clear_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );
    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);

    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t final_old = amoadd_d(target_vaddr, final_addend);
    TEST_ASSERT("after clearing both mcontrol6 slots, a final amoadd.d observes the post-clear value and commits the expected recovery payload",
        excpt.triggered == false &&
        final_old == post_clear_value &&
        read64(target_vaddr) == final_value
    );

    TEST_ASSERT("the load-store chain AMO breakpoint probe preserves all non-target sentinels in HS view",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the unchained and post-clear amoadd.d updates while guard, adjacent, and marker words stay intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_micro_memblock_amoadd_w_upper_word_store_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x4c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x4c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7570706572777264ULL;
    const uint32_t lower_initial = 0xffff0001U;
    const uint32_t upper_initial = 0x7ffffffeU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0123456789abcdefULL;
    const uint64_t marker_seed = 0x0badf00d12345678ULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint32_t blocked_addend = 0x11111111U;
    const uint32_t final_addend = 0x2U;
    const uint32_t expected_upper = upper_initial + final_addend;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_w(upper_word_vaddr, blocked_addend);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoadd.w and suppresses the lane RMW",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amoadd.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoadd_w(upper_word_vaddr, final_addend);
    TEST_ASSERT("after clearing the store-only trigger, amoadd.w returns the old upper word and updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoadd.w lane update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_lr_w_upper_word_load_trigger_no_reservation_then_fresh_lrsc_updates_lane_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x500U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x500U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6c72777570707264ULL;
    const uint32_t lower_initial = 0x13572468U;
    const uint32_t upper_initial = 0x89abcdefU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_seed = 0x1122334455667788ULL;
    const uint64_t marker_final = 0x8877665544332211ULL;
    const uint32_t no_reservation_value = 0x0badcafeU;
    const uint32_t final_upper_value = 0x2468ace0U;
    const uint32_t stale_upper_value = 0xdeadbeefU;
    const uint64_t final_value =
        ((uint64_t)final_upper_value << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)lr_w(upper_word_vaddr);
    bool lr_breakpoint_seen =
        excpt.triggered == true && excpt.cause == CAUSE_BKP;
    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool breakpoint_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("lr.w hitting the upper-word load trigger reports breakpoint and leaves the full target doubleword plus sentinels unchanged",
        lr_breakpoint_seen &&
        breakpoint_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t no_reservation_status =
        sc_w(upper_word_vaddr, no_reservation_value);
    TEST_ASSERT("after clearing the load trigger, a same upper-word sc.w without a fresh lr.w fails because the breakpoint-suppressed lr.w did not create a reservation",
        excpt.triggered == false &&
        no_reservation_status != 0U &&
        read64(target_vaddr) == initial_value &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(upper_word_vaddr);
    uint32_t final_status = sc_w(upper_word_vaddr, final_upper_value);
    TEST_ASSERT("a fresh upper-word lr.w/sc.w after the breakpoint path succeeds and updates only the high 32-bit lane",
        excpt.triggered == false &&
        fresh_reserved == upper_initial &&
        final_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(upper_word_vaddr, stale_upper_value);
    TEST_ASSERT("the successful upper-word sc.w consumes the reservation, so a second naked sc.w fails and preserves the committed lane value",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value &&
        read32(upper_word_vaddr) == final_upper_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("the lr.w breakpoint no-reservation lane probe preserves guard and adjacent words while marker progress remains possible",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the fresh post-breakpoint upper-word sc.w payload plus marker progress and keeps lower lane plus sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_sc_w_upper_word_load_trigger_does_not_fire_then_fresh_lrsc_updates_lane_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = VSRWX_GX;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x540U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x540U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x540U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t target_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t upper_base_vaddr = target_base_vaddr + sizeof(uint32_t);
    const uintptr_t upper_alias_vaddr = target_alias_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7363776c64747267ULL;
    const uint32_t lower_initial = 0x55667788U;
    const uint32_t upper_initial = 0x10203040U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0102030405060708ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x1122334455667788ULL;
    const uint32_t armed_upper_value = 0xa5a55a5aU;
    const uint32_t stale_upper_value = 0xdeadbeefU;
    const uint32_t post_clear_upper_value = 0x2468ace0U;
    const uint64_t armed_value =
        ((uint64_t)armed_upper_value << 32) | lower_initial;
    const uint64_t post_clear_value =
        ((uint64_t)post_clear_upper_value << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_memblock_lrsc_trigger_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    /* Keep the same-set VA aliases resident before the load-only trigger is armed. */
    (void)read64(target_alias_vaddr);
    (void)read64(target_base_vaddr);

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_base_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_alias_vaddr);
    uint32_t armed_status = sc_w(upper_base_vaddr, armed_upper_value);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper-word SC target does not fire on sc.w, so a same-set alias lr.w/base sc.w can consume the reservation and commit one high-lane update",
        excpt.triggered == false &&
        reserved == upper_initial &&
        armed_status == 0U &&
        read64(target_alias_vaddr) == armed_value &&
        read32(target_alias_vaddr) == lower_initial &&
        read32(upper_alias_vaddr) == armed_upper_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(upper_base_vaddr, stale_upper_value);
    TEST_ASSERT("with the load-only trigger still armed, a second naked upper-word sc.w does not report breakpoint and fails after the successful SC consumed the reservation",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_alias_vaddr) == armed_value &&
        read32(upper_alias_vaddr) == armed_upper_value
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(upper_base_vaddr);
    uint32_t post_clear_status =
        sc_w(upper_base_vaddr, post_clear_upper_value);
    TEST_ASSERT("after clearing the load-only trigger, a fresh upper-word lr.w/sc.w pair still captures the committed high lane and updates only that lane once",
        excpt.triggered == false &&
        fresh_reserved == armed_upper_value &&
        post_clear_status == 0U &&
        read64(target_alias_vaddr) == post_clear_value &&
        read32(target_alias_vaddr) == lower_initial &&
        read32(upper_alias_vaddr) == post_clear_upper_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("the sc.w load-trigger negative-classification lane probe preserves guard and adjacent words while marker progress remains possible",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the two successful upper-word sc.w lane updates plus marker progress and keeps lower lane plus sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == post_clear_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == post_clear_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_lr_w_upper_word_store_trigger_does_not_fire_then_fresh_lrsc_updates_lane_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x580U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x580U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6c72777374726764ULL;
    const uint32_t lower_initial = 0x11224488U;
    const uint32_t upper_initial = 0x7ace1357U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_seed = 0x8877aabbccdd0011ULL;
    const uint64_t marker_final = 0x1100ddccbbaa7788ULL;
    const uint32_t final_upper_value = 0x2468ace0U;
    const uint32_t stale_upper_value = 0xdeadbeefU;
    const uint64_t final_value =
        ((uint64_t)final_upper_value << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t observed_upper = lr_w(upper_word_vaddr);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word does not fire on lr.w and the load returns the original high lane",
        excpt.triggered == false &&
        observed_upper == upper_initial &&
        read64(target_vaddr) == initial_value &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == upper_initial
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(upper_word_vaddr);
    uint32_t final_status = sc_w(upper_word_vaddr, final_upper_value);
    TEST_ASSERT("after clearing the store-only trigger, a fresh upper-word lr.w/sc.w pair still captures the original high lane and updates only that lane",
        excpt.triggered == false &&
        fresh_reserved == upper_initial &&
        final_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(upper_word_vaddr, stale_upper_value);
    TEST_ASSERT("the successful upper-word sc.w consumes the fresh reservation, so a second naked sc.w fails and preserves the committed high lane",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value &&
        read32(upper_word_vaddr) == final_upper_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("the lr.w store-trigger negative-classification lane probe preserves guard and adjacent words while marker progress remains possible",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear fresh upper-word sc.w lane update plus marker progress and keeps lower lane plus sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_w_upper_word_load_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x5c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x5c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616d6f6c64757070ULL;
    const uint32_t lower_initial = 0x13579bdfU;
    const uint32_t upper_initial = 0x6ffffff0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x2468ace013579bdfULL;
    const uint64_t marker_seed = 0x1029384756abcdefULL;
    const uint64_t marker_final = 0xfedcba6547382910ULL;
    const uint32_t blocked_addend = 0x01010101U;
    const uint32_t final_addend = 0x10U;
    const uint32_t expected_upper = upper_initial + final_addend;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_w(upper_word_vaddr, blocked_addend);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoadd.w and suppresses the lane RMW",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amoadd.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoadd_w(upper_word_vaddr, final_addend);
    TEST_ASSERT("after clearing the load-only trigger, amoadd.w returns the old upper word and updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoadd.w lane update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_sc_w_upper_word_store_trigger_breakpoint_blocks_commit_then_fresh_lrsc_updates_lane_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x600U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x600U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7363777374727570ULL;
    const uint32_t lower_initial = 0xa5a55a5aU;
    const uint32_t upper_initial = 0x10203040U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x55aa33cc77ee11ffULL;
    const uint32_t blocked_upper_value = 0xdeadbeefU;
    const uint32_t final_upper_value = 0x2468ace0U;
    const uint32_t stale_upper_value = 0x0badcafeU;
    const uint64_t final_value =
        ((uint64_t)final_upper_value << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_vaddr);
    TEST_ASSERT("with a store-only mcontrol6 address trigger armed on the upper word, lr.w still establishes the reservation and returns the original high lane",
        excpt.triggered == false &&
        reserved == upper_initial &&
        read64(target_vaddr) == initial_value &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    (void)sc_w(upper_word_vaddr, blocked_upper_value);
    bool sc_breakpoint_seen =
        excpt.triggered == true && excpt.cause == CAUSE_BKP;
    TEST_ASSERT("a store-only mcontrol6 address trigger reports breakpoint on upper-word sc.w before the conditional store can commit",
        sc_breakpoint_seen
    );

    goto_priv(PRIV_M);
    bool breakpoint_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered upper-word sc.w breakpoint blocks the lane store before any sentinel changes",
        breakpoint_preserved_words
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(upper_word_vaddr);
    uint32_t final_status = sc_w(upper_word_vaddr, final_upper_value);
    TEST_ASSERT("after clearing the store-only trigger, a fresh upper-word lr.w/sc.w pair recovers and updates only the high 32-bit lane",
        excpt.triggered == false &&
        fresh_reserved == upper_initial &&
        final_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(upper_word_vaddr, stale_upper_value);
    TEST_ASSERT("the post-clear successful upper-word sc.w consumes the reservation, so a following naked sc.w fails and preserves the committed lane",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value &&
        read32(upper_word_vaddr) == final_upper_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("the sc.w store-trigger lane probe preserves guard and adjacent words while marker progress remains possible",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word sc.w payload plus marker progress and keeps lower lane plus sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_w_lower_word_load_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x640U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x640U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616d6f6c6f776572ULL;
    const uint32_t lower_initial = 0x7ffffff0U;
    const uint32_t upper_initial = 0x2468ace0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_seed = 0x1122aabb3344ccddULL;
    const uint64_t marker_final = 0xddcc4433bbaa2211ULL;
    const uint32_t blocked_addend = 0x01020304U;
    const uint32_t final_addend = 0x20U;
    const uint32_t expected_lower = lower_initial + final_addend;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_w(lower_word_vaddr, blocked_addend);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoadd.w and suppresses the lane RMW",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered lower-word amoadd.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoadd_w(lower_word_vaddr, final_addend);
    TEST_ASSERT("after clearing the load-only trigger, amoadd.w returns the old lower word and updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the lower-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoadd.w lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_w_lower_word_store_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x680U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x680U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x73746f6c6f776572ULL;
    const uint32_t lower_initial = 0x7fffff80U;
    const uint32_t upper_initial = 0x13572468U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x89abcdef01234567ULL;
    const uint64_t marker_seed = 0x55aa7711cc8833eeULL;
    const uint64_t marker_final = 0xee3388cc1177aa55ULL;
    const uint32_t blocked_addend = 0x00000100U;
    const uint32_t final_addend = 0x40U;
    const uint32_t expected_lower = lower_initial + final_addend;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_w(lower_word_vaddr, blocked_addend);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoadd.w and suppresses the lane RMW",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered lower-word amoadd.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoadd_w(lower_word_vaddr, final_addend);
    TEST_ASSERT("after clearing the store-only trigger, amoadd.w returns the old lower word and updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the lower-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoadd.w lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_lr_w_lower_word_load_trigger_no_reservation_then_fresh_lrsc_updates_lane_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x6c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x6c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6c72776c6f776572ULL;
    const uint32_t lower_initial = 0x10203040U;
    const uint32_t upper_initial = 0x89abcdefU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0011223344556677ULL;
    const uint64_t marker_seed = 0x8899aabbccddeeffULL;
    const uint64_t marker_final = 0xffeeddccbbaa9988ULL;
    const uint32_t no_reservation_value = 0x0badcafeU;
    const uint32_t final_lower_value = 0x55667788U;
    const uint32_t stale_lower_value = 0xdeadbeefU;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower_value;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)lr_w(lower_word_vaddr);
    bool lr_breakpoint_seen =
        excpt.triggered == true && excpt.cause == CAUSE_BKP;
    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool breakpoint_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("lr.w hitting the lower-word load trigger reports breakpoint and leaves the full target doubleword plus sentinels unchanged",
        lr_breakpoint_seen &&
        breakpoint_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t no_reservation_status =
        sc_w(lower_word_vaddr, no_reservation_value);
    TEST_ASSERT("after clearing the load trigger, a same lower-word sc.w without a fresh lr.w fails because the breakpoint-suppressed lr.w did not create a reservation",
        excpt.triggered == false &&
        no_reservation_status != 0U &&
        read64(target_vaddr) == initial_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(lower_word_vaddr);
    uint32_t final_status = sc_w(lower_word_vaddr, final_lower_value);
    TEST_ASSERT("a fresh lower-word lr.w/sc.w after the breakpoint path succeeds and updates only the low 32-bit lane",
        excpt.triggered == false &&
        fresh_reserved == lower_initial &&
        final_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(lower_word_vaddr, stale_lower_value);
    TEST_ASSERT("the successful lower-word sc.w consumes the reservation, so a second naked sc.w fails and preserves the committed lane value",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("the lower-word lr.w breakpoint no-reservation probe preserves guard and adjacent words while marker progress remains possible",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the fresh post-breakpoint lower-word sc.w payload plus marker progress and keeps upper lane plus sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_lr_w_lower_word_store_trigger_does_not_fire_then_fresh_lrsc_updates_lane_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x700U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x700U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6c7277736c6f7765ULL;
    const uint32_t lower_initial = 0x76543210U;
    const uint32_t upper_initial = 0x89abcdefU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0123fedcba987654ULL;
    const uint64_t marker_seed = 0x55aa996633cc0077ULL;
    const uint64_t marker_final = 0x7700cc336699aa55ULL;
    const uint32_t final_lower_value = 0x13579bdfU;
    const uint32_t stale_lower_value = 0xcafed00dU;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower_value;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t observed_lower = lr_w(lower_word_vaddr);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word does not fire on lr.w and the load returns the original low lane",
        excpt.triggered == false &&
        observed_lower == lower_initial &&
        read64(target_vaddr) == initial_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == upper_initial
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(lower_word_vaddr);
    uint32_t final_status = sc_w(lower_word_vaddr, final_lower_value);
    TEST_ASSERT("after clearing the store-only trigger, a fresh lower-word lr.w/sc.w pair still captures the original low lane and updates only that lane",
        excpt.triggered == false &&
        fresh_reserved == lower_initial &&
        final_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(lower_word_vaddr, stale_lower_value);
    TEST_ASSERT("the successful lower-word sc.w consumes the fresh reservation, so a second naked sc.w fails and preserves the committed low lane",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("the lower-word lr.w store-trigger negative-classification probe preserves guard and adjacent words while marker progress remains possible",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear fresh lower-word sc.w lane update plus marker progress and keeps upper lane plus sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_sc_w_lower_word_load_trigger_does_not_fire_then_fresh_lrsc_updates_lane_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = VSRWX_GX;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x740U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x740U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x740U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t target_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t lower_base_vaddr = target_base_vaddr;
    const uintptr_t lower_alias_vaddr = target_alias_vaddr;
    const uintptr_t upper_alias_vaddr = target_alias_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7363776c6f776572ULL;
    const uint32_t lower_initial = 0x01020304U;
    const uint32_t upper_initial = 0xa5a55a5aU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x123456789abcdef0ULL;
    const uint64_t marker_seed = 0x0f0e0d0c0b0a0908ULL;
    const uint64_t marker_final = 0x8090a0b0c0d0e0f0ULL;
    const uint32_t armed_lower_value = 0x2468ace0U;
    const uint32_t stale_lower_value = 0xdeadbeefU;
    const uint32_t post_clear_lower_value = 0x13579bdfU;
    const uint64_t armed_value =
        ((uint64_t)upper_initial << 32) | armed_lower_value;
    const uint64_t post_clear_value =
        ((uint64_t)upper_initial << 32) | post_clear_lower_value;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_memblock_lrsc_trigger_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    /* Keep the same-set VA aliases resident before the load-only trigger is armed. */
    (void)read64(target_alias_vaddr);
    (void)read64(target_base_vaddr);

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_base_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(lower_alias_vaddr);
    uint32_t armed_status = sc_w(lower_base_vaddr, armed_lower_value);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower-word SC target does not fire on sc.w, so a same-set alias lr.w/base sc.w can consume the reservation and commit one low-lane update",
        excpt.triggered == false &&
        reserved == lower_initial &&
        armed_status == 0U &&
        read64(target_alias_vaddr) == armed_value &&
        read32(lower_alias_vaddr) == armed_lower_value &&
        read32(upper_alias_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(lower_base_vaddr, stale_lower_value);
    TEST_ASSERT("with the load-only trigger still armed, a second naked lower-word sc.w does not report breakpoint and fails after the successful SC consumed the reservation",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_alias_vaddr) == armed_value &&
        read32(lower_alias_vaddr) == armed_lower_value &&
        read32(upper_alias_vaddr) == upper_initial
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(lower_base_vaddr);
    uint32_t post_clear_status =
        sc_w(lower_base_vaddr, post_clear_lower_value);
    TEST_ASSERT("after clearing the load-only trigger, a fresh lower-word lr.w/sc.w pair still captures the committed low lane and updates only that lane once",
        excpt.triggered == false &&
        fresh_reserved == armed_lower_value &&
        post_clear_status == 0U &&
        read64(target_alias_vaddr) == post_clear_value &&
        read32(lower_alias_vaddr) == post_clear_lower_value &&
        read32(upper_alias_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("the lower-word sc.w load-trigger negative-classification probe preserves guard and adjacent words while marker progress remains possible",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the two successful lower-word sc.w lane updates plus marker progress and keeps upper lane plus sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == post_clear_value &&
        read32(lower_word_paddr) == post_clear_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_w_lower_word_load_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x780U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x780U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7377706c6f776572ULL;
    const uint32_t lower_initial = 0x10203040U;
    const uint32_t upper_initial = 0x89abcdefU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x7766554433221100ULL;
    const uint64_t marker_seed = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t marker_final = 0x71605f4e3d2c1b0aULL;
    const uint32_t blocked_lower_value = 0xa5a55a5aU;
    const uint32_t final_lower_value = 0x0badcafeU;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | final_lower_value;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoswap_w(lower_word_vaddr, blocked_lower_value);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoswap.w and suppresses the lane replacement",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered lower-word amoswap.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoswap_w(lower_word_vaddr, final_lower_value);
    TEST_ASSERT("after clearing the load-only trigger, amoswap.w returns the old lower word and replaces only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the lower-word amoswap.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoswap.w lane replacement plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == final_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_w_lower_word_store_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x7c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x7c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x73777073746f7265ULL;
    const uint32_t lower_initial = 0x2468ace0U;
    const uint32_t upper_initial = 0xfedcba98U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x87695a4b3c2d1e0fULL;
    const uint32_t blocked_lower_value = 0xdeadbeefU;
    const uint32_t final_lower_value = 0xc001d00dU;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | final_lower_value;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoswap_w(lower_word_vaddr, blocked_lower_value);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoswap.w and suppresses the lane replacement",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered lower-word amoswap.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoswap_w(lower_word_vaddr, final_lower_value);
    TEST_ASSERT("after clearing the store-only trigger, amoswap.w returns the old lower word and replaces only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the lower-word amoswap.w store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoswap.w lane replacement plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == final_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_w_upper_word_load_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x800U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x800U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7377706c64757070ULL;
    const uint32_t lower_initial = 0x76543210U;
    const uint32_t upper_initial = 0x1234abcdU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    const uint64_t marker_seed = 0x0102030405060708ULL;
    const uint64_t marker_final = 0x8070605040302010ULL;
    const uint32_t blocked_upper_value = 0xfaceb00cU;
    const uint32_t final_upper_value = 0x5eedc0deU;
    const uint64_t expected_final =
        ((uint64_t)final_upper_value << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoswap_w(upper_word_vaddr, blocked_upper_value);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoswap.w and suppresses the lane replacement",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered upper-word amoswap.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoswap_w(upper_word_vaddr, final_upper_value);
    TEST_ASSERT("after clearing the load-only trigger, amoswap.w returns the old upper word and replaces only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the upper-word amoswap.w load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoswap.w lane replacement plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_w_upper_word_store_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x840U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x840U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7377707375707065ULL;
    const uint32_t lower_initial = 0x0f1e2d3cU;
    const uint32_t upper_initial = 0xc3d2e1f0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x1021324354657687ULL;
    const uint64_t marker_seed = 0x1123581321345589ULL;
    const uint64_t marker_final = 0x9855342113582311ULL;
    const uint32_t blocked_upper_value = 0xa11ced09U;
    const uint32_t final_upper_value = 0x6a5a4a3aU;
    const uint64_t expected_final =
        ((uint64_t)final_upper_value << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoswap_w(upper_word_vaddr, blocked_upper_value);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoswap.w and suppresses the lane replacement",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered upper-word amoswap.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoswap_w(upper_word_vaddr, final_upper_value);
    TEST_ASSERT("after clearing the store-only trigger, amoswap.w returns the old upper word and replaces only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the upper-word amoswap.w store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoswap.w lane replacement plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_w_upper_word_load_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x880U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x880U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d696e6c64757070ULL;
    const uint32_t lower_initial = 0x2468ace0U;
    const uint32_t upper_initial = 0x80000010U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x1029384756abcdefULL;
    const uint64_t marker_seed = 0x0123456789abcdefULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint32_t blocked_operand = 0x80000004U;
    const uint32_t final_operand = 0x80000000U;
    const uint32_t expected_upper = final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomin_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper word reports breakpoint on amomin.w before the signed-min lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered upper-word amomin.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomin_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amomin.w returns the old upper word and signed-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomin.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amomin.w signed-min lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_w_upper_word_store_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x8c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x8c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x73746d696e757070ULL;
    const uint32_t lower_initial = 0x13572468U;
    const uint32_t upper_initial = 0x80000020U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x87695a4b3c2d1e0fULL;
    const uint32_t blocked_operand = 0x80000008U;
    const uint32_t final_operand = 0x80000000U;
    const uint32_t expected_upper = final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomin_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word reports breakpoint on amomin.w before the signed-min lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered upper-word amomin.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomin_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amomin.w returns the old upper word and signed-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomin.w store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amomin.w signed-min lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}
