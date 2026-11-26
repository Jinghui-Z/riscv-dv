#include "ai_micro_memblock_lrsc_trigger_common.h"

bool ai_micro_memblock_sc_d_chain_slot0_mismatch_suppresses_sibling_slot1_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x600U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x600U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7363316d69736d61ULL;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    const uint64_t marker_seed = 0x0102030405060708ULL;
    const uint64_t chained_sc_value = 0xa5a55a5adeadbeefULL;
    const uint64_t naked_sc_value = 0x0badc0ffee0dd00dULL;
    const uint64_t fresh_sc_value = 0x2468ace013579bdfULL;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t chained_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, chained_sc_value);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on sc.d must stay gated and the reserved sc.d must commit once",
        excpt.triggered == false &&
        reserved == initial_value &&
        chained_status == 0U &&
        read64(target_vaddr) == chained_sc_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t naked_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, naked_sc_value);
    TEST_ASSERT("the committed chain-mismatch sc.d consumes the reservation, so a following naked sc.d fails and preserves the committed value",
        excpt.triggered == false &&
        naked_status != 0U &&
        read64(target_vaddr) == chained_sc_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t fresh_reserved = ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t fresh_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, fresh_sc_value);
    TEST_ASSERT("after clearing both trigger slots, a fresh lr.d/sc.d pair observes the committed chain-mismatch value and updates the target normally",
        excpt.triggered == false &&
        fresh_reserved == chained_sc_value &&
        fresh_status == 0U &&
        read64(target_vaddr) == fresh_sc_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the sc.d chain-mismatch sibling-suppression probe contains only the successful SC updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == fresh_sc_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_lr_d_chain_slot0_mismatch_suppresses_sibling_slot1_load_reservation_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x680U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x680U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6c72646d69736d61ULL;
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t adjacent_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_seed = 0x55aa33cc77ee11ffULL;
    const uint64_t chained_sc_value = 0x13579bdf2468ace0ULL;
    const uint64_t naked_sc_value = 0x0ddba11dead0beefULL;
    const uint64_t fresh_sc_value = 0xf00f1234cafe5678ULL;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t chained_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, chained_sc_value);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 load trigger on lr.d must stay gated so lr.d returns data and establishes a reservation for the following sc.d",
        excpt.triggered == false &&
        reserved == initial_value &&
        chained_status == 0U &&
        read64(target_vaddr) == chained_sc_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t naked_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, naked_sc_value);
    TEST_ASSERT("the lr.d chain-mismatch path established and consumed exactly one reservation, so a following naked sc.d fails and preserves the committed value",
        excpt.triggered == false &&
        naked_status != 0U &&
        read64(target_vaddr) == chained_sc_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t fresh_reserved = ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t fresh_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, fresh_sc_value);
    TEST_ASSERT("after clearing both trigger slots, a fresh lr.d/sc.d pair observes the chain-mismatch commit and updates the target normally",
        excpt.triggered == false &&
        fresh_reserved == chained_sc_value &&
        fresh_status == 0U &&
        read64(target_vaddr) == fresh_sc_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the lr.d chain-mismatch sibling-suppression probe contains only the successful SC updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == fresh_sc_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_lane_commit_corner()
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
    const uint64_t guard_seed = 0x61647763686e3030ULL;
    const uint32_t lower_initial = 0xa5a55a5aU;
    const uint32_t upper_initial = 0x1000fff0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    const uint64_t marker_seed = 0x0102030405060708ULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint32_t chained_addend = 0x00010020U;
    const uint32_t final_addend = 0x00000011U;
    const uint32_t after_chain_upper = upper_initial + chained_addend;
    const uint64_t after_chain_value =
        ((uint64_t)after_chain_upper << 32) | lower_initial;
    const uint32_t final_upper = after_chain_upper + final_addend;
    const uint64_t final_value =
        ((uint64_t)final_upper << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoadd_w(upper_word_vaddr, chained_addend);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on upper-word amoadd.w must stay gated and the AMO.W lane add must commit once",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == after_chain_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoadd_w(upper_word_vaddr, final_addend);
    TEST_ASSERT("after clearing both trigger slots, a second upper-word amoadd.w observes the chain-mismatch commit and updates only the upper 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_upper &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the upper-word amoadd.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W upper-lane chain-mismatch sibling-suppression probe contains only the two successful upper-lane adds and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_lr_d_chain_slot0_store_access_mismatch_suppresses_sibling_slot1_load_reservation_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x740U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x740U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6c72646163633030ULL;
    const uint64_t initial_value = 0x89abcdef01234567ULL;
    const uint64_t adjacent_seed = 0x76543210fedcba98ULL;
    const uint64_t marker_seed = 0x1029384756abcdefULL;
    const uint64_t chained_sc_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t naked_sc_value = 0xc001d00dfeedfaceULL;
    const uint64_t fresh_sc_value = 0x1122aabb3344ccddULL;

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
        AI_MEMBLOCK_LRSC_TRIGGER_STORE, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t chained_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, chained_sc_value);
    TEST_ASSERT("when chained slot0 has the same address but only store access enabled, lr.d must not count it as a chain hit; the matching slot1 load trigger stays gated while lr.d returns data and establishes a reservation",
        excpt.triggered == false &&
        reserved == initial_value &&
        chained_status == 0U &&
        read64(target_vaddr) == chained_sc_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t naked_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, naked_sc_value);
    TEST_ASSERT("the access-mismatch chain-miss lr.d established and consumed exactly one reservation, so a following naked sc.d fails and preserves the committed value",
        excpt.triggered == false &&
        naked_status != 0U &&
        read64(target_vaddr) == chained_sc_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t fresh_reserved = ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t fresh_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, fresh_sc_value);
    TEST_ASSERT("after clearing both trigger slots, a fresh lr.d/sc.d pair observes the access-mismatch chain commit and updates the target normally",
        excpt.triggered == false &&
        fresh_reserved == chained_sc_value &&
        fresh_status == 0U &&
        read64(target_vaddr) == fresh_sc_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the same-address access-mismatch chain probe contains only the successful SC updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == fresh_sc_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_sc_d_chain_slot0_load_access_mismatch_suppresses_sibling_slot1_store_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x780U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x780U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x73636c6461633030ULL;
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t adjacent_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_seed = 0xa55a33cc77ee11ddULL;
    const uint64_t chained_sc_value = 0x5a5aa5a512345678ULL;
    const uint64_t naked_sc_value = 0xdeadc0ffee123456ULL;
    const uint64_t fresh_sc_value = 0x24681357ace0bdf9ULL;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t chained_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, chained_sc_value);
    TEST_ASSERT("when chained slot0 has the same address but only load access enabled, sc.d must not count it as a chain hit; the matching slot1 store trigger stays gated while the reserved sc.d commits once",
        excpt.triggered == false &&
        reserved == initial_value &&
        chained_status == 0U &&
        read64(target_vaddr) == chained_sc_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t naked_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, naked_sc_value);
    TEST_ASSERT("the access-mismatch chain-miss sc.d consumed its reservation, so a following naked sc.d fails and preserves the committed value",
        excpt.triggered == false &&
        naked_status != 0U &&
        read64(target_vaddr) == chained_sc_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t fresh_reserved = ai_memblock_lrsc_trigger_lr_d64(target_vaddr);
    uint32_t fresh_status =
        ai_memblock_lrsc_trigger_sc_d_status(target_vaddr, fresh_sc_value);
    TEST_ASSERT("after clearing both trigger slots, a fresh lr.d/sc.d pair observes the access-mismatch chain commit and updates the target normally",
        excpt.triggered == false &&
        fresh_reserved == chained_sc_value &&
        fresh_status == 0U &&
        read64(target_vaddr) == fresh_sc_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the same-address sc.d access-mismatch chain probe contains only the successful SC updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == fresh_sc_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_lr_w_upper_chain_slot0_store_access_mismatch_suppresses_sibling_slot1_load_reservation_corner()
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
    const uint64_t guard_seed = 0x6c72777561636331ULL;
    const uint32_t lower_initial = 0xa5a55a5aU;
    const uint32_t upper_initial = 0x1122ccddU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x76543210fedcba98ULL;
    const uint64_t marker_seed = 0x89abcdef01234567ULL;
    const uint32_t chained_upper_value = 0x3344ee11U;
    const uint32_t naked_upper_value = 0x5566aa99U;
    const uint32_t fresh_upper_value = 0x7788bb22U;
    const uint64_t after_chain_value =
        ((uint64_t)chained_upper_value << 32) | lower_initial;
    const uint64_t final_value =
        ((uint64_t)fresh_upper_value << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_vaddr);
    uint32_t chained_status = sc_w(upper_word_vaddr, chained_upper_value);
    TEST_ASSERT("when chained slot0 has the same upper-word address but only store access enabled, lr.w must not count it as a chain hit; the matching slot1 load trigger stays gated while lr.w reserves the upper lane",
        excpt.triggered == false &&
        reserved == upper_initial &&
        chained_status == 0U &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == chained_upper_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t naked_status = sc_w(upper_word_vaddr, naked_upper_value);
    TEST_ASSERT("the upper-word access-mismatch chain-miss lr.w established and consumed exactly one reservation, so a following naked sc.w fails and preserves both lanes",
        excpt.triggered == false &&
        naked_status != 0U &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == chained_upper_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(upper_word_vaddr);
    uint32_t fresh_status = sc_w(upper_word_vaddr, fresh_upper_value);
    TEST_ASSERT("after clearing both trigger slots, a fresh upper-word lr.w/sc.w pair observes the chain-mismatch lane update and updates only the high 32-bit lane",
        excpt.triggered == false &&
        fresh_reserved == chained_upper_value &&
        fresh_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == fresh_upper_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the upper-word lr.w access-mismatch chain probe contains only the two successful upper-lane SC updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == fresh_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_sc_w_upper_chain_slot0_load_access_mismatch_suppresses_sibling_slot1_store_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x800U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x800U;
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
    const uint64_t guard_seed = 0x7363777561636331ULL;
    const uint32_t lower_initial = 0x13579bdfU;
    const uint32_t upper_initial = 0x2468ace0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t marker_seed = 0x778899aabbccddeeULL;
    const uint32_t chained_upper_value = 0x55aa33ccU;
    const uint32_t naked_upper_value = 0xee11dd22U;
    const uint32_t fresh_upper_value = 0x6699cc00U;
    const uint64_t after_chain_value =
        ((uint64_t)chained_upper_value << 32) | lower_initial;
    const uint64_t final_value =
        ((uint64_t)fresh_upper_value << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_vaddr);
    uint32_t chained_status = sc_w(upper_word_vaddr, chained_upper_value);
    TEST_ASSERT("when chained slot0 has the same upper-word address but only load access enabled, sc.w must not count it as a chain hit; the matching slot1 store trigger stays gated while the reserved sc.w commits the upper lane",
        excpt.triggered == false &&
        reserved == upper_initial &&
        chained_status == 0U &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == chained_upper_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t naked_status = sc_w(upper_word_vaddr, naked_upper_value);
    TEST_ASSERT("the upper-word access-mismatch chain-miss sc.w consumed the reservation, so a following naked sc.w fails and preserves both lanes",
        excpt.triggered == false &&
        naked_status != 0U &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == chained_upper_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(upper_word_vaddr);
    uint32_t fresh_status = sc_w(upper_word_vaddr, fresh_upper_value);
    TEST_ASSERT("after clearing both trigger slots, a fresh upper-word lr.w/sc.w pair observes the chain-mismatch SC commit and updates only the high 32-bit lane",
        excpt.triggered == false &&
        fresh_reserved == chained_upper_value &&
        fresh_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == fresh_upper_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the upper-word sc.w access-mismatch chain probe contains only the two successful upper-lane SC updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == fresh_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_sc_w_lower_chain_slot0_load_access_mismatch_suppresses_sibling_slot1_store_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x840U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x840U;
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
    const uint64_t guard_seed = 0x73636c7761636331ULL;
    const uint32_t lower_initial = 0x89abcdefU;
    const uint32_t upper_initial = 0x76543210U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x1029384756abcdefULL;
    const uint64_t marker_seed = 0x55aa33cc77ee11ddULL;
    const uint32_t chained_lower_value = 0x1122aabbU;
    const uint32_t naked_lower_value = 0xccdd3344U;
    const uint32_t fresh_lower_value = 0x5566ee99U;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | chained_lower_value;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | fresh_lower_value;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(lower_word_vaddr);
    uint32_t chained_status = sc_w(lower_word_vaddr, chained_lower_value);
    TEST_ASSERT("when chained slot0 has the same lower-word address but only load access enabled, sc.w must not count it as a chain hit; the matching slot1 store trigger stays gated while the reserved sc.w commits the lower lane",
        excpt.triggered == false &&
        reserved == lower_initial &&
        chained_status == 0U &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == chained_lower_value &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t naked_status = sc_w(lower_word_vaddr, naked_lower_value);
    TEST_ASSERT("the lower-word access-mismatch chain-miss sc.w consumed the reservation, so a following naked sc.w fails and preserves both lanes",
        excpt.triggered == false &&
        naked_status != 0U &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == chained_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(lower_word_vaddr);
    uint32_t fresh_status = sc_w(lower_word_vaddr, fresh_lower_value);
    TEST_ASSERT("after clearing both trigger slots, a fresh lower-word lr.w/sc.w pair observes the chain-mismatch SC commit and updates only the low 32-bit lane",
        excpt.triggered == false &&
        fresh_reserved == chained_lower_value &&
        fresh_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == fresh_lower_value &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the lower-word sc.w access-mismatch chain probe contains only the two successful lower-lane SC updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == fresh_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_lr_w_lower_chain_slot0_store_access_mismatch_suppresses_sibling_slot1_load_reservation_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x880U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x880U;
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
    const uint64_t guard_seed = 0x6c72776c61636331ULL;
    const uint32_t lower_initial = 0xcafebabeU;
    const uint32_t upper_initial = 0x13572468U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xabcdef0123456789ULL;
    const uint64_t marker_seed = 0x1122ee44cc66aa88ULL;
    const uint32_t chained_lower_value = 0x99aa55ccU;
    const uint32_t naked_lower_value = 0x6677bb22U;
    const uint32_t fresh_lower_value = 0x33dd8844U;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | chained_lower_value;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | fresh_lower_value;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(lower_word_vaddr);
    uint32_t chained_status = sc_w(lower_word_vaddr, chained_lower_value);
    TEST_ASSERT("when chained slot0 has the same lower-word address but only store access enabled, lr.w must not count it as a chain hit; the matching slot1 load trigger stays gated while lr.w reserves the lower lane",
        excpt.triggered == false &&
        reserved == lower_initial &&
        chained_status == 0U &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == chained_lower_value &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t naked_status = sc_w(lower_word_vaddr, naked_lower_value);
    TEST_ASSERT("the lower-word access-mismatch chain-miss lr.w established and consumed exactly one reservation, so a following naked sc.w fails and preserves both lanes",
        excpt.triggered == false &&
        naked_status != 0U &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == chained_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(lower_word_vaddr);
    uint32_t fresh_status = sc_w(lower_word_vaddr, fresh_lower_value);
    TEST_ASSERT("after clearing both trigger slots, a fresh lower-word lr.w/sc.w pair observes the chain-mismatch lane update and updates only the low 32-bit lane",
        excpt.triggered == false &&
        fresh_reserved == chained_lower_value &&
        fresh_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == fresh_lower_value &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the lower-word lr.w access-mismatch chain probe contains only the two successful lower-lane SC updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == fresh_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x8c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x8c0U;
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
    const uint64_t guard_seed = 0x6164776c63683131ULL;
    const uint32_t lower_initial = 0x2000f0f0U;
    const uint32_t upper_initial = 0x7ace1357U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x8877aabbccdd0011ULL;
    const uint64_t marker_seed = 0x1020304050607080ULL;
    const uint64_t marker_final = 0x0f1e2d3c4b5a6978ULL;
    const uint32_t chained_addend = 0x00000f21U;
    const uint32_t final_addend = 0x00000042U;
    const uint32_t after_chain_lower = lower_initial + chained_addend;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | after_chain_lower;
    const uint32_t final_lower = after_chain_lower + final_addend;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoadd_w(lower_word_vaddr, chained_addend);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on lower-word amoadd.w must stay gated and the AMO.W lane add must commit once",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == after_chain_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoadd_w(lower_word_vaddr, final_addend);
    TEST_ASSERT("after clearing both trigger slots, a second lower-word amoadd.w observes the chain-mismatch commit and updates only the lower 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_lower &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the lower-word amoadd.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W lower-lane chain-mismatch sibling-suppression probe contains only the two successful lower-lane adds and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x900U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x900U;
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
    const uint64_t guard_seed = 0x7377706c63683232ULL;
    const uint32_t lower_initial = 0x01234567U;
    const uint32_t upper_initial = 0xfedcba98U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x13579bdf2468ace0ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x1122334455667788ULL;
    const uint32_t chained_lower_value = 0xa55aa55aU;
    const uint32_t final_lower_value = 0x3cc33cc3U;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | chained_lower_value;
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

    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoswap_w(lower_word_vaddr, chained_lower_value);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on lower-word amoswap.w must stay gated and the AMO.W lane replacement must commit once",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == chained_lower_value &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoswap_w(lower_word_vaddr, final_lower_value);
    TEST_ASSERT("after clearing both trigger slots, a second lower-word amoswap.w observes the chain-mismatch replacement and updates only the lower 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == chained_lower_value &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the lower-word amoswap.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W lower-lane replacement chain-mismatch sibling-suppression probe contains only the two successful lower-lane swaps and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x940U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x940U;
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
    const uint64_t guard_seed = 0x7377707563683333ULL;
    const uint32_t lower_initial = 0x89abcdefU;
    const uint32_t upper_initial = 0x02468aceU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t marker_seed = 0x99aa88bb77cc66ddULL;
    const uint64_t marker_final = 0x55aa44bb33cc22ddULL;
    const uint32_t chained_upper_value = 0x13579bdfU;
    const uint32_t final_upper_value = 0x2468ace0U;
    const uint64_t after_chain_value =
        ((uint64_t)chained_upper_value << 32) | lower_initial;
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

    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoswap_w(upper_word_vaddr, chained_upper_value);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on upper-word amoswap.w must stay gated and the AMO.W lane replacement must commit once",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == chained_upper_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoswap_w(upper_word_vaddr, final_upper_value);
    TEST_ASSERT("after clearing both trigger slots, a second upper-word amoswap.w observes the chain-mismatch replacement and updates only the upper 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == chained_upper_value &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the upper-word amoswap.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W upper-lane replacement chain-mismatch sibling-suppression probe contains only the two successful upper-lane swaps and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoand_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_logic_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x980U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x980U;
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
    const uint64_t guard_seed = 0x616e646c63683434ULL;
    const uint32_t lower_initial = 0xf0f35ac3U;
    const uint32_t upper_initial = 0x4b1d2637U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x456789abcdef0123ULL;
    const uint64_t marker_seed = 0x3210fedcba987654ULL;
    const uint64_t marker_final = 0x7e6d5c4b3a291807ULL;
    const uint32_t chained_operand = 0x7cf0ff3cU;
    const uint32_t final_operand = 0x0ff33fcfU;
    const uint32_t after_chain_lower = lower_initial & chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | after_chain_lower;
    const uint32_t final_lower = after_chain_lower & final_operand;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoand_w(lower_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on lower-word amoand.w must stay gated and the AMO.W logic-and lane update must commit once",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == after_chain_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoand_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second lower-word amoand.w observes the chain-mismatch AND result and updates only the lower 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_lower &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the lower-word amoand.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W lower-lane logic-and chain-mismatch sibling-suppression probe contains only the two successful lower-lane AND updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoand_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_logic_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x9c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x9c0U;
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
    const uint64_t guard_seed = 0x616e647563683535ULL;
    const uint32_t lower_initial = 0x6c2d4e90U;
    const uint32_t upper_initial = 0xf3c55a0fU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0123456789abcdefULL;
    const uint64_t marker_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_final = 0x89abcdef01234567ULL;
    const uint32_t chained_operand = 0x7ff00ff3U;
    const uint32_t final_operand = 0x0fcf3ff0U;
    const uint32_t after_chain_upper = upper_initial & chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)after_chain_upper << 32) | lower_initial;
    const uint32_t final_upper = after_chain_upper & final_operand;
    const uint64_t final_value =
        ((uint64_t)final_upper << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoand_w(upper_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on upper-word amoand.w must stay gated and the AMO.W logic-and lane update must commit once",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == after_chain_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoand_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second upper-word amoand.w observes the chain-mismatch AND result and updates only the upper 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_upper &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the upper-word amoand.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W upper-lane logic-and chain-mismatch sibling-suppression probe contains only the two successful upper-lane AND updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_logic_or_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xa00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xa00U;
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
    const uint64_t guard_seed = 0x6f726c6368363630ULL;
    const uint32_t lower_initial = 0x1234500aU;
    const uint32_t upper_initial = 0xc0ffee12U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x76543210fedcba98ULL;
    const uint64_t marker_seed = 0x89abcdef01234567ULL;
    const uint64_t marker_final = 0x5a69788796a5b4c3ULL;
    const uint32_t chained_operand = 0x0c0f0f30U;
    const uint32_t final_operand = 0xa00000c5U;
    const uint32_t after_chain_lower = lower_initial | chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | after_chain_lower;
    const uint32_t final_lower = after_chain_lower | final_operand;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoor_w(lower_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on lower-word amoor.w must stay gated and the AMO.W logic-or lane update must commit once",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == after_chain_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoor_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second lower-word amoor.w observes the chain-mismatch OR result and updates only the lower 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_lower &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the lower-word amoor.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W lower-lane logic-or chain-mismatch sibling-suppression probe contains only the two successful lower-lane OR updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_logic_xor_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xa40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xa40U;
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
    const uint64_t guard_seed = 0x786f726c63683737ULL;
    const uint32_t lower_initial = 0xa5c30f5aU;
    const uint32_t upper_initial = 0x17e4c29bU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t marker_seed = 0x71605f4e3d2c1b0aULL;
    const uint64_t marker_final = 0x13572468ace0bdf9ULL;
    const uint32_t chained_operand = 0x5a0ff0c3U;
    const uint32_t final_operand = 0x0f33cc55U;
    const uint32_t after_chain_lower = lower_initial ^ chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | after_chain_lower;
    const uint32_t final_lower = after_chain_lower ^ final_operand;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoxor_w(lower_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on lower-word amoxor.w must stay gated and the AMO.W logic-xor lane update must commit once",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == after_chain_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoxor_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second lower-word amoxor.w observes the chain-mismatch XOR result and updates only the lower 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_lower &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the lower-word amoxor.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W lower-lane logic-xor chain-mismatch sibling-suppression probe contains only the two successful lower-lane XOR updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_logic_or_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xa80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xa80U;
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
    const uint64_t guard_seed = 0x75707065726f7238ULL;
    const uint32_t lower_initial = 0x6d3c4b2aU;
    const uint32_t upper_initial = 0x2050a00fU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint32_t chained_operand = 0x0f0c5500U;
    const uint32_t final_operand = 0xc00300f0U;
    const uint32_t after_chain_upper = upper_initial | chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)after_chain_upper << 32) | lower_initial;
    const uint32_t final_upper = after_chain_upper | final_operand;
    const uint64_t final_value =
        ((uint64_t)final_upper << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoor_w(upper_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on upper-word amoor.w must stay gated and the AMO.W logic-or lane update must commit once",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == after_chain_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoor_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second upper-word amoor.w observes the chain-mismatch OR result and updates only the upper 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_upper &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the upper-word amoor.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W upper-lane logic-or chain-mismatch sibling-suppression probe contains only the two successful upper-lane OR updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_signed_max_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xac0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xac0U;
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
    const uint64_t guard_seed = 0x6d61786c6f776531ULL;
    const uint32_t lower_initial = 0x80000020U;
    const uint32_t upper_initial = 0x9ace1357U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x22446688aaccef01ULL;
    const uint64_t marker_seed = 0x10fedcba98765432ULL;
    const uint64_t marker_final = 0x0123456789abcdefULL;
    const uint32_t chained_operand = 0x00007f10U;
    const uint32_t final_operand = 0x7fffff00U;
    const uint32_t after_chain_lower = chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | after_chain_lower;
    const uint32_t final_lower = final_operand;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomax_w(lower_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on lower-word amomax.w must stay gated and the AMO.W signed-max lane update must commit once",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == after_chain_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amomax_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second lower-word amomax.w observes the chain-mismatch signed-max result and updates only the lower 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_lower &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the lower-word amomax.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W lower-lane signed-max chain-mismatch sibling-suppression probe contains only the two successful lower-lane MAX updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_min_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xb00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xb00U;
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
    const uint64_t guard_seed = 0x756d696e75703132ULL;
    const uint32_t lower_initial = 0x31415926U;
    const uint32_t upper_initial = 0xfedc7400U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0badc0de12345678ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x55aa33cc77ee1199ULL;
    const uint32_t chained_operand = 0x00004210U;
    const uint32_t final_operand = 0x00001201U;
    const uint32_t after_chain_upper = chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)after_chain_upper << 32) | lower_initial;
    const uint32_t final_upper = final_operand;
    const uint64_t final_value =
        ((uint64_t)final_upper << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amominu_w(upper_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on upper-word amominu.w must stay gated and the AMO.W unsigned-min lane update must commit once",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == after_chain_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amominu_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second upper-word amominu.w observes the chain-mismatch unsigned-min result and updates only the upper 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_upper &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the upper-word amominu.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W upper-lane unsigned-min chain-mismatch sibling-suppression probe contains only the two successful upper-lane MINU updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_max_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xb40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xb40U;
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
    const uint64_t guard_seed = 0x756d61786c6f7732ULL;
    const uint32_t lower_initial = 0x00001234U;
    const uint32_t upper_initial = 0x3579bdf1U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xa55a33cc996655aaULL;
    const uint64_t marker_seed = 0x0101020203030404ULL;
    const uint64_t marker_final = 0x4040030302020101ULL;
    const uint32_t chained_operand = 0x80000030U;
    const uint32_t final_operand = 0x80000080U;
    const uint32_t after_chain_lower = chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | after_chain_lower;
    const uint32_t final_lower = final_operand;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomaxu_w(lower_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on lower-word amomaxu.w must stay gated and the AMO.W unsigned-max lane update must commit once",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == after_chain_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amomaxu_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second lower-word amomaxu.w observes the chain-mismatch unsigned-max result and updates only the lower 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_lower &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the lower-word amomaxu.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W lower-lane unsigned-max chain-mismatch sibling-suppression probe contains only the two successful lower-lane MAXU updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_signed_min_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xb80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xb80U;
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
    const uint64_t guard_seed = 0x736d696e75703334ULL;
    const uint32_t lower_initial = 0x2468ace0U;
    const uint32_t upper_initial = 0x00007000U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x13579bdf2468ace0ULL;
    const uint64_t marker_seed = 0x1122446688aacceeULL;
    const uint64_t marker_final = 0xeeccaa8866442211ULL;
    const uint32_t chained_operand = 0xffff8000U;
    const uint32_t final_operand = 0x80000040U;
    const uint32_t after_chain_upper = chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)after_chain_upper << 32) | lower_initial;
    const uint32_t final_upper = final_operand;
    const uint64_t final_value =
        ((uint64_t)final_upper << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomin_w(upper_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on upper-word amomin.w must stay gated and the AMO.W signed-min lane update must commit once",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == after_chain_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amomin_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second upper-word amomin.w observes the chain-mismatch signed-min result and updates only the upper 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_upper &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the upper-word amomin.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W upper-lane signed-min chain-mismatch sibling-suppression probe contains only the two successful upper-lane MIN updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_d_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_max_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xbc0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xbc0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d617864636831ULL;
    const uint64_t initial_value = 0x0000000012345678ULL;
    const uint64_t adjacent_seed = 0x0102030405060708ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x1122334455667788ULL;
    const uint64_t chained_operand = 0x8000000000000040ULL;
    const uint64_t final_operand = 0x9000000000000080ULL;
    const uint64_t after_chain_value = chained_operand;
    const uint64_t final_value = final_operand;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomaxu_d(target_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on amomaxu.d must stay gated and the full-width unsigned-max update must commit once",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == after_chain_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t post_clear_old = amomaxu_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second amomaxu.d observes the chain-mismatch unsigned-max result and updates the full 64-bit doubleword",
        excpt.triggered == false &&
        post_clear_old == after_chain_value &&
        read64(target_vaddr) == final_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the amomaxu.d chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.D unsigned-max chain-mismatch sibling-suppression probe contains only the two successful full-width MAXU updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_d_chain_slot0_mismatch_suppresses_sibling_slot1_signed_min_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xc00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xc00U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x736d696e64636832ULL;
    const uint64_t initial_value = 0x0000000000007000ULL;
    const uint64_t adjacent_seed = 0x22446688aaccef01ULL;
    const uint64_t marker_seed = 0x0101020203030404ULL;
    const uint64_t marker_final = 0x4040030302020101ULL;
    const uint64_t chained_operand = 0xffffffffffff9000ULL;
    const uint64_t final_operand = 0x8000000000000040ULL;
    const uint64_t after_chain_value = chained_operand;
    const uint64_t final_value = final_operand;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomin_d(target_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on amomin.d must stay gated and the full-width signed-min update must commit once",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == after_chain_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t post_clear_old = amomin_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second amomin.d observes the chain-mismatch signed-min result and updates the full 64-bit doubleword",
        excpt.triggered == false &&
        post_clear_old == after_chain_value &&
        read64(target_vaddr) == final_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the amomin.d chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.D signed-min chain-mismatch sibling-suppression probe contains only the two successful full-width MIN updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_d_chain_slot0_mismatch_suppresses_sibling_slot1_signed_max_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xc40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xc40U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x736d617864636833ULL;
    const uint64_t initial_value = 0xffffffffffff8000ULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x87695a4b3c2d1e0fULL;
    const uint64_t chained_operand = 0x0000000000007000ULL;
    const uint64_t final_operand = 0x0000000000007fffULL;
    const uint64_t after_chain_value = chained_operand;
    const uint64_t final_value = final_operand;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomax_d(target_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on amomax.d must stay gated and the full-width signed-max update must commit once",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == after_chain_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t post_clear_old = amomax_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second amomax.d observes the chain-mismatch signed-max result and updates the full 64-bit doubleword",
        excpt.triggered == false &&
        post_clear_old == after_chain_value &&
        read64(target_vaddr) == final_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the amomax.d chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.D signed-max chain-mismatch sibling-suppression probe contains only the two successful full-width MAX updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_d_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_min_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xc80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xc80U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d696e64636834ULL;
    const uint64_t initial_value = 0xfffffffffffff000ULL;
    const uint64_t adjacent_seed = 0xccbbaa9988776655ULL;
    const uint64_t marker_seed = 0x1234432112344321ULL;
    const uint64_t marker_final = 0x87655a5a3c3c1e1eULL;
    const uint64_t chained_operand = 0x0000000000007000ULL;
    const uint64_t final_operand = 0x0000000000000040ULL;
    const uint64_t after_chain_value = chained_operand;
    const uint64_t final_value = final_operand;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amominu_d(target_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on amominu.d must stay gated and the full-width unsigned-min update must commit once",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == after_chain_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t post_clear_old = amominu_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second amominu.d observes the chain-mismatch unsigned-min result and updates the full 64-bit doubleword",
        excpt.triggered == false &&
        post_clear_old == after_chain_value &&
        read64(target_vaddr) == final_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the amominu.d chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.D unsigned-min chain-mismatch sibling-suppression probe contains only the two successful full-width MINU updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_signed_max_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xcc0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xcc0U;
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
    const uint64_t guard_seed = 0x6d61787570333535ULL;
    const uint32_t lower_initial = 0x13579bdfU;
    const uint32_t upper_initial = 0x80000080U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0123456789abcdefULL;
    const uint64_t marker_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_final = 0x2468ace013579bdfULL;
    const uint32_t chained_operand = 0x00007000U;
    const uint32_t final_operand = 0x7fffff00U;
    const uint32_t after_chain_upper = chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)after_chain_upper << 32) | lower_initial;
    const uint32_t final_upper = final_operand;
    const uint64_t final_value =
        ((uint64_t)final_upper << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomax_w(upper_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on upper-word amomax.w must stay gated and the AMO.W signed-max lane update must commit once",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == after_chain_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amomax_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second upper-word amomax.w observes the chain-mismatch signed-max result and updates only the upper 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_upper &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the upper-word amomax.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W upper-lane signed-max chain-mismatch sibling-suppression probe contains only the two successful upper-lane MAX updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_max_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xd00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xd00U;
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
    const uint64_t guard_seed = 0x756d617875703636ULL;
    const uint32_t lower_initial = 0x89abcdefU;
    const uint32_t upper_initial = 0x00003456U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t marker_seed = 0x1122334455667788ULL;
    const uint64_t marker_final = 0x8877665544332211ULL;
    const uint32_t chained_operand = 0x80000030U;
    const uint32_t final_operand = 0x80000080U;
    const uint32_t after_chain_upper = chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)after_chain_upper << 32) | lower_initial;
    const uint32_t final_upper = final_operand;
    const uint64_t final_value =
        ((uint64_t)final_upper << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomaxu_w(upper_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on upper-word amomaxu.w must stay gated and the AMO.W unsigned-max lane update must commit once",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == after_chain_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amomaxu_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second upper-word amomaxu.w observes the chain-mismatch unsigned-max result and updates only the upper 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_upper &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the upper-word amomaxu.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W upper-lane unsigned-max chain-mismatch sibling-suppression probe contains only the two successful upper-lane MAXU updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_logic_xor_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xd40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xd40U;
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
    const uint64_t guard_seed = 0x786f727570373737ULL;
    const uint32_t lower_initial = 0xc001d00dU;
    const uint32_t upper_initial = 0x5aa55aa5U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xabcdef0101234567ULL;
    const uint64_t marker_seed = 0x89abcdef76543210ULL;
    const uint64_t marker_final = 0x10234567fedcba98ULL;
    const uint32_t chained_operand = 0xa55a0ff0U;
    const uint32_t final_operand = 0x00ff33ccU;
    const uint32_t after_chain_upper = upper_initial ^ chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)after_chain_upper << 32) | lower_initial;
    const uint32_t final_upper = after_chain_upper ^ final_operand;
    const uint64_t final_value =
        ((uint64_t)final_upper << 32) | lower_initial;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoxor_w(upper_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on upper-word amoxor.w must stay gated and the AMO.W logic-xor lane update must commit once",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == after_chain_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amoxor_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second upper-word amoxor.w observes the chain-mismatch XOR result and updates only the upper 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_upper &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the upper-word amoxor.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W upper-lane logic-xor chain-mismatch sibling-suppression probe contains only the two successful upper-lane XOR updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_signed_min_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xd80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xd80U;
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
    const uint64_t guard_seed = 0x736d696e6c6f7731ULL;
    const uint32_t lower_initial = 0x00007000U;
    const uint32_t upper_initial = 0x2468ace0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x02468ace13579bdfULL;
    const uint64_t marker_seed = 0x7766554433221100ULL;
    const uint64_t marker_final = 0x0011223344556677ULL;
    const uint32_t chained_operand = 0xffff8000U;
    const uint32_t final_operand = 0x80000040U;
    const uint32_t after_chain_lower = chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | after_chain_lower;
    const uint32_t final_lower = final_operand;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomin_w(lower_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on lower-word amomin.w must stay gated and the AMO.W signed-min lane update must commit once",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == after_chain_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amomin_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second lower-word amomin.w observes the chain-mismatch signed-min result and updates only the lower 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_lower &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the lower-word amomin.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W lower-lane signed-min chain-mismatch sibling-suppression probe contains only the two successful lower-lane MIN updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_min_lane_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xdc0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xdc0U;
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
    const uint64_t guard_seed = 0x756d696e6c6f7732ULL;
    const uint32_t lower_initial = 0xfedc7400U;
    const uint32_t upper_initial = 0x13579bdfU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x5a5a1234c3c35eedULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x87695a4b3c2d1e0fULL;
    const uint32_t chained_operand = 0x00004210U;
    const uint32_t final_operand = 0x00001201U;
    const uint32_t after_chain_lower = chained_operand;
    const uint64_t after_chain_value =
        ((uint64_t)upper_initial << 32) | after_chain_lower;
    const uint32_t final_lower = final_operand;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amominu_w(lower_word_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on lower-word amominu.w must stay gated and the AMO.W unsigned-min lane update must commit once",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == after_chain_value &&
        read32(lower_word_vaddr) == after_chain_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t post_clear_old = amominu_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second lower-word amominu.w observes the chain-mismatch unsigned-min result and updates only the lower 32-bit lane",
        excpt.triggered == false &&
        post_clear_old == after_chain_lower &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower &&
        read32(upper_word_vaddr) == upper_initial &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the lower-word amominu.w chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.W lower-lane unsigned-min chain-mismatch sibling-suppression probe contains only the two successful lower-lane MINU updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoand_d_chain_slot0_mismatch_suppresses_sibling_slot1_logic_and_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xe00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xe00U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616e64636861696eULL;
    const uint64_t initial_value = 0xf0ff55aac3d2e1f0ULL;
    const uint64_t adjacent_seed = 0x9182736455463728ULL;
    const uint64_t marker_seed = 0x0123456789abcdefULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint64_t chained_operand = 0x00ff00ff00ff00ffULL;
    const uint64_t final_operand = 0x0ff0ff000ff0ff00ULL;
    const uint64_t after_chain_value = initial_value & chained_operand;
    const uint64_t final_value = after_chain_value & final_operand;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoand_d(target_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on amoand.d must stay gated and the full-width logic-and update must commit once",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == after_chain_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t post_clear_old = amoand_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second amoand.d observes the chain-mismatch logic-and result and updates the full 64-bit doubleword",
        excpt.triggered == false &&
        post_clear_old == after_chain_value &&
        read64(target_vaddr) == final_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the amoand.d chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.D logic-and chain-mismatch sibling-suppression probe contains only the two successful full-width AND updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_d_chain_slot0_mismatch_suppresses_sibling_slot1_logic_or_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xe40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xe40U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f72636861696e31ULL;
    const uint64_t initial_value = 0x21003400a5c3e1d0ULL;
    const uint64_t adjacent_seed = 0x6a5b4c3d2e1f8071ULL;
    const uint64_t marker_seed = 0x13579bdf2468ace0ULL;
    const uint64_t marker_final = 0x0eca8642fdb97531ULL;
    const uint64_t chained_operand = 0x00f0004f0000004fULL;
    const uint64_t final_operand = 0x0000c0ff0000c0ffULL;
    const uint64_t after_chain_value = initial_value | chained_operand;
    const uint64_t final_value = after_chain_value | final_operand;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoor_d(target_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on amoor.d must stay gated and the full-width logic-or update must commit once",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == after_chain_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t post_clear_old = amoor_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second amoor.d observes the chain-mismatch logic-or result and updates the full 64-bit doubleword",
        excpt.triggered == false &&
        post_clear_old == after_chain_value &&
        read64(target_vaddr) == final_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the amoor.d chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.D logic-or chain-mismatch sibling-suppression probe contains only the two successful full-width OR updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_d_chain_slot0_mismatch_suppresses_sibling_slot1_logic_xor_commit_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xe80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xe80U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x786f72636861696eULL;
    const uint64_t initial_value = 0xa5a55a5ac3d2e1f0ULL;
    const uint64_t adjacent_seed = 0x7e6d5c4b3a291807ULL;
    const uint64_t marker_seed = 0x02468ace13579bdfULL;
    const uint64_t marker_final = 0xfdb97531eca86420ULL;
    const uint64_t chained_operand = 0x00ff0ff000ff0ff0ULL;
    const uint64_t final_operand = 0xff0000ff00ff0000ULL;
    const uint64_t after_chain_value = initial_value ^ chained_operand;
    const uint64_t final_value = after_chain_value ^ final_operand;

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
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, guard_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    ai_memblock_lrsc_trigger_arm_mcontrol6_slot(1, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);

    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoxor_d(target_vaddr, chained_operand);
    TEST_ASSERT("when slot0 is chained to a nonmatching guard load address, a matching slot1 store trigger on amoxor.d must stay gated and the full-width logic-xor update must commit once",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == after_chain_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t post_clear_old = amoxor_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing both trigger slots, a second amoxor.d observes the chain-mismatch logic-xor result and updates the full 64-bit doubleword",
        excpt.triggered == false &&
        post_clear_old == after_chain_value &&
        read64(target_vaddr) == final_value &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress proves the amoxor.d chain-mismatch path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AMO.D logic-xor chain-mismatch sibling-suppression probe contains only the two successful full-width XOR updates and keeps all sentinels intact",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}
