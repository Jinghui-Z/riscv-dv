#include "ai_micro_memblock_lrsc_trigger_common.h"

bool ai_micro_memblock_amoadd_d_chain_slot0_mismatch_suppresses_sibling_slot1_match_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x540U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x540U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6368316d73333174ULL;
    const uint64_t initial_value = 0x00000000deadbeefULL;
    const uint64_t adjacent_seed = 0xa5a5a5a5a5a5a5a5ULL;
    const uint64_t marker_seed = 0x5a5a5a5a5a5a5a5aULL;
    const uint64_t mismatched_addend = 0x0000000100000001ULL;
    const uint64_t after_first = initial_value + mismatched_addend;
    const uint64_t both_match_addend = 0x0000000200000002ULL;
    const uint64_t cleared_addend = 0x0000000300000003ULL;
    const uint64_t after_cleared = after_first + cleared_addend;

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
    uint64_t first_old = amoadd_d(target_vaddr, mismatched_addend);
    TEST_ASSERT("chained slot0 at guard_vaddr does not match amoadd.d at target_vaddr, so the non-chained slot1 match must stay gated by trigger2ChainOkVec and the RMW must commit without breakpoint",
        excpt.triggered == false &&
        first_old == initial_value &&
        read64(target_vaddr) == after_first &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );
    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);

    ai_memblock_lrsc_trigger_arm_mcontrol6_slot_chain(0, target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD, true);
    saved_mstatus = ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_d(target_vaddr, both_match_addend);
    TEST_ASSERT("after re-arming slot0 chain to target_vaddr so both slots match the same amoadd.d, the chain must close and raise CAUSE_BKP before any 64-bit RMW",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(target_vaddr) == after_first &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );
    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);

    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(0);
    ai_memblock_lrsc_trigger_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t cleared_old = amoadd_d(target_vaddr, cleared_addend);
    TEST_ASSERT("after both trigger slots are cleared, a follow-up amoadd.d observes after_first and commits the full 64-bit update while sentinels stay intact",
        excpt.triggered == false &&
        cleared_old == after_first &&
        read64(target_vaddr) == after_cleared &&
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_seed
    );

    TEST_END(true);
}
