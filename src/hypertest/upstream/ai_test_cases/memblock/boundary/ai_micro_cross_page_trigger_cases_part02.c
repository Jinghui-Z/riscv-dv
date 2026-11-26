#include "ai_micro_cross_page_trigger_common.h"

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t pattern[16] = {
        0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b,
        0x8c, 0x8d, 0x8e, 0x8f
    };
    uintptr_t vector_vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    uintptr_t sentinel_before_addr = low_phys - 1U;
    uintptr_t sentinel_after_addr = high_phys + 8U;
    uintptr_t scratch_vaddr = hs_page_base(VSRWX_GRWX) + 0x40U;
    uintptr_t scratch_phys = phys_page_base(VSRWX_GRWX) + 0x40U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys, pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(high_phys, pattern + 8U, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull 16 bytes across the same HS page boundary before the mixed vector/scalar trigger-load adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page load, the shared boundary bytes and adjacent sentinels remain unchanged before the scalar trigger-load adjacent-preservation probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-page payload into a scratch page preserves the original 16-byte boundary byte sequence before the scalar trigger-load adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-page load still matches the exact boundary payload and keeps both scratch and shared-boundary sentinels unchanged before the scalar trigger-load adjacent-preservation probe",
        ai_micro_cross_page_trigger_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, scalar_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector cross-page load touches the same page pair, the first scalar dual-trigger split-load probe still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-page trigger probe still leaves both boundary fragments and adjacent sentinels unchanged after the prior vector load",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector cross-page load, the second scalar dual-trigger split-load probe still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-page trigger probe still leaves both boundary fragments and adjacent sentinels unchanged after the prior vector load",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-page load, clearing only the execute-PC slot still leaves the scalar cross-page load address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page load probe still leaves both boundary fragments and adjacent sentinels unchanged after the prior vector load",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-page load still completes normally and returns the merged payload even though a vector cross-page load touched the shared path earlier in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0x8b8a898887868584ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector/scalar cross-page trigger-load sequence, the shared boundary bytes and adjacent sentinels still match their expected values",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_completion(false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_completion(true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            false, false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            false, true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            true, false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            true, true);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_execute_and_address_trigger_both_breakpoint_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS cross-page store still reports breakpoint when both execute-PC and address trigger slots match the same no-fault split store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with both trigger slots armed, the trapped HS cross-page no-fault split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page store address trigger still reports breakpoint on the same no-fault split path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped HS cross-page no-fault split store still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page split store commits both fragments normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the low-page fragment reflects the split store bytes",
        read64(low_phys) == 0x89abcdef55667788ULL
    );
    TEST_ASSERT("after clearing both trigger slots, the high-page fragment reflects the split store bytes",
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page no-fault split store, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page no-fault split store still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same cross-page no-fault split store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page no-fault split store still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page no-fault split store still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page split store commits both fragments normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the low-page fragment reflects the split store bytes",
        read64(low_phys) == 0x89abcdef55667788ULL
    );
    TEST_ASSERT("after clearing both trigger slots, the high-page fragment reflects the split store bytes",
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same no-fault cross-page split store, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped no-fault cross-page split-store probe still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same no-fault cross-page split store, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped no-fault cross-page split-store probe still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped no-fault cross-page split-store probe still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after clearing both trigger slots, the same no-fault cross-page split store commits normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered no-fault split-store path with execute-PC and address triggers, only the two boundary doublewords change while the adjacent neighbors remain unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL &&
        read64(high_adjacent_phys) == high_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0xef, 0xcd, 0xab, 0x89,
        0x67, 0x45, 0x23, 0x01,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x200U;
    uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x200U;
    uintptr_t vector_vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t sentinel_before_addr = low_phys - 1U;
    uintptr_t sentinel_after_addr = high_phys + 8U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar cross-page trigger-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte boundary payload across two HS pages before the scalar trigger-store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store, the boundary bytes match the source payload and adjacent sentinels stay unchanged before the scalar trigger-store probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, scalar_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after a vector cross-page store touches the same page pair, the first scalar dual-trigger split-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-page trigger-store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the earlier vector cross-page store, the second scalar dual-trigger split-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-page trigger-store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-page store, clearing only the execute-PC slot still leaves the scalar cross-page store address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-page store still commits normally even though a vector cross-page store touched the shared path earlier",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-then-scalar trigger-store sequence, the final boundary bytes reflect the expected overwrite order and adjacent sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-then-scalar trigger-store sequence, a following scalar split-load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0xef, 0xcd, 0xab, 0x89,
        0x67, 0x45, 0x23, 0x01,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x200U;
    uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x200U;
    uintptr_t vector_vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t sentinel_before_addr = low_phys - 1U;
    uintptr_t sentinel_after_addr = high_phys + 8U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar cross-page trigger-store adjacent-preservation probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte boundary payload across two HS pages before the scalar trigger-store adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store, the boundary bytes match the source payload and adjacent sentinels stay unchanged before the scalar trigger-store adjacent-preservation probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, scalar_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after a vector cross-page store touches the same page pair, the first scalar dual-trigger split-store probe still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-page trigger-store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged in the adjacent-preservation corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the earlier vector cross-page store, the second scalar dual-trigger split-store probe still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-page trigger-store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged in the adjacent-preservation corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-page store, clearing only the execute-PC slot still leaves the scalar cross-page store address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged in the adjacent-preservation corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-page store still commits normally even though a vector cross-page store touched the shared path earlier in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-then-scalar trigger-store adjacent-preservation sequence, the final boundary bytes reflect the expected overwrite order and adjacent sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-then-scalar trigger-store adjacent-preservation sequence, a following scalar split-load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_completion(false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_completion(true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            false, false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            false, true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            true, false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            true, true);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_execute_trigger_with_second_half_address_slot_ignored_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the no-fault cross-page split load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching on Spike; the no-fault cross-page split load completes normally",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first no-fault cross-page split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second no-fault cross-page split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the no-fault cross-page split load completes normally",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRX;
    enum test_page high_page = VSRWX_GR;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first no-fault cross-page split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped no-fault cross-page split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second no-fault cross-page split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped no-fault cross-page split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the no-fault cross-page split load completes normally in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered no-fault split-load path, both boundary doublewords and their adjacent neighbors remain unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_execute_trigger_with_second_half_address_slot_ignored_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    bool trap_triggered;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the no-fault cross-page split store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the execute-PC slot is still armed, the second-half-start address slot does not matter and the trapped no-fault cross-page split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching on Spike; the no-fault cross-page split store commits both fragments normally",
        trap_triggered == false &&
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first no-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped no-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second no-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped no-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the no-fault cross-page split store commits both fragments normally",
        excpt.triggered == false
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated execute-trigger probes, the recovered cross-page split store still commits the expected low-page and high-page fragments",
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x3141592653589793ULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    const uint64_t high_adjacent = 0x2718281828459045ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first no-fault cross-page split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped no-fault cross-page split-store attempt still leaves both page fragments and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second no-fault cross-page split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped no-fault cross-page split-store attempt still leaves both page fragments and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the no-fault cross-page split store commits normally in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered no-fault split-store path, only the two touched page fragments change while their adjacent neighbors remain unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the high-half page-fault split load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching; Spike falls straight back to the underlying high-half load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uint64_t saved_mstatus;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half page-fault cross-page split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half page-fault cross-page split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-page split load falls back to the underlying high-half load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half page-fault cross-page split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half page-fault cross-page split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half page-fault cross-page split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half page-fault cross-page split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-page split load falls back to the underlying high-half load page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback high-half load page-fault path with only the second-half-start slot armed still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half page-fault cross-page split-load attempt still reports breakpoint on Spike before any later repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half page-fault cross-page split-load attempt still reports breakpoint on Spike before any later repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-page split load first falls back to the underlying high-half load page fault before repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr,
        "triggered=%d cause=0x%llx tval=0x%llx expected_tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the upper leaf, repeated execute-trigger hits and the fallback high-half load page fault leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits on the high-half page-fault split load, fixing the upper leaf lets the same split load recover while the second-half-start address slot still stays ignored on Spike",
        excpt.triggered == false &&
        observed == recovery_expected,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, recovery_expected
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = phys_page_base(VSRWX_GR);
    uintptr_t deny_end = deny_start + PAGE_SIZE;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the high-half access-fault split load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching; Spike falls straight back to the underlying high-half load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = phys_page_base(VSRWX_GR);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uint64_t saved_mstatus;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half access-fault cross-page split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half access-fault cross-page split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-page split load falls back to the underlying high-half load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRX;
    enum test_page high_page = VSRWX_GR;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_adjacent = 0x1234432155aa55aaULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0xaa55aa5512344321ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half access-fault cross-page split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half access-fault cross-page split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half access-fault cross-page split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half access-fault cross-page split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-page split load falls back to the underlying high-half load access fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback high-half load access-fault path with only the second-half-start slot armed still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRX;
    enum test_page high_page = VSRWX_GR;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half access-fault cross-page split-load attempt still reports breakpoint on Spike before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half access-fault cross-page split-load attempt still reports breakpoint on Spike before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-page split load first falls back to the underlying high-half load access fault before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before restoring PMP read permission, repeated execute-trigger hits and the fallback high-half load access fault leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_clear_pmp_window();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits on the high-half access-fault split load, restoring PMP read permission lets the same split load recover while the second-half-start address slot still stays ignored on Spike",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GX);
    uintptr_t high_phys = phys_page_base(VSRX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the high-half page-fault split store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the execute-PC slot is still armed, the misprogrammed second-half address slot does not matter and the trapped split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching; Spike falls straight back to the high-half store page-fault corner with no store side effect",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == second_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GX);
    uintptr_t high_phys = phys_page_base(VSRX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half page-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half page-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half page-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half page-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the split store falls back to the high-half store page-fault corner with no store side effect",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == second_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_exec_2nd_addr_slot_page_fault_repeated_bkp_no_side_effect_adjacent_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x3141592653589793ULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t high_adjacent = 0x2718281828459045ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half page-fault cross-page split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half page-fault cross-page split-store attempt still leaves both page fragments and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half page-fault cross-page split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half page-fault cross-page split-store attempt still leaves both page fragments and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the split store falls back to the high-half store page-fault corner in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback high-half store page-fault corner with only the second-half-start slot armed still exposes no low-half store side effect while leaving adjacent neighbors untouched",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half page-fault cross-page split-store attempt still reports breakpoint on Spike before any later repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half page-fault cross-page split-store attempt still leaves both page fragments unchanged before any later repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half page-fault cross-page split-store attempt still reports breakpoint on Spike before any later repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half page-fault cross-page split-store attempt still leaves both page fragments unchanged before any later repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the split store first falls back to the high-half store page-fault corner before repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the upper leaf, repeated execute-trigger hits and the fallback high-half store page fault leave both page fragments unchanged",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits on the high-half page-fault split store, fixing the upper leaf lets the same split store recover while the second-half-start address slot still stays ignored on Spike",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the untouched low-page bytes",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the untouched high-page bytes",
        read64(high_phys) == expected_high_after_store
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t deny_start = phys_page_base(VSRW_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(deny_start, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the high-half access-fault split store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the execute-PC slot is still armed, the misprogrammed second-half address slot does not matter and the trapped split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching; Spike falls straight back to the high-half store access-fault corner with no store side effect",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == second_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(deny_start) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t deny_start = phys_page_base(VSRW_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(deny_start, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half access-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half access-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half access-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half access-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the split store falls back to the high-half store access-fault corner with no store side effect",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == second_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(deny_start) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_exec_2nd_addr_slot_access_fault_repeated_bkp_no_side_effect_adjacent_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x0badc0de55aa1234ULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t high_adjacent = 0x1234fedc5678ba90ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half access-fault cross-page split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half access-fault cross-page split-store attempt still leaves both page fragments and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half access-fault cross-page split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half access-fault cross-page split-store attempt still leaves both page fragments and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the split store falls back to the high-half store access-fault corner in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback high-half store access-fault corner with only the second-half-start slot armed still exposes no low-half store side effect while leaving adjacent neighbors untouched",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x2233445566778899ULL;
    const uint64_t high_pattern = 0x2468ace013579bdfULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x89abcdef01234567ULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first high-half access-fault cross-page split-store attempt still reports breakpoint on Spike before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half access-fault cross-page split-store attempt still leaves both page fragments unchanged before PMP permission is restored",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second high-half access-fault cross-page split-store attempt still reports breakpoint on Spike before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half access-fault cross-page split-store attempt still leaves both page fragments unchanged before PMP permission is restored",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the split store first falls back to the high-half store access-fault corner before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr,
        "triggered=%d cause=0x%llx tval=0x%llx expected_tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before restoring PMP write permission, repeated execute-trigger hits and the fallback high-half store access fault leave both page fragments unchanged",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_clear_pmp_window();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits on the high-half access-fault split store, restoring PMP write permission lets the same split store recover while the second-half-start address slot still stays ignored on Spike",
        excpt.triggered == false,
        "triggered=%d cause=0x%llx tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the untouched low-page bytes",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the untouched high-page bytes",
        read64(high_phys) == expected_high_after_store
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_page_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSX_GX, 4U);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("an HS cross-page load still reports breakpoint when both execute-PC and address trigger slots match a split load whose low half would otherwise raise page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page load address trigger still reports breakpoint on the same low-half page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page load reports low-half load page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSX_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the low-half page-fault split load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching when the low half faults first; Spike falls straight back to the underlying low-half load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSX_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uint64_t saved_mstatus;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first low-half page-fault cross-page split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second low-half page-fault cross-page split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored when the low half faults first on Spike, so the split load falls back to the underlying low-half load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_access_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = phys_page_base(VSRWX_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("an HS cross-page load still reports breakpoint when both execute-PC and address trigger slots match a split load whose low half would otherwise raise access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page load address trigger still reports breakpoint on the same low-half access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page load reports low-half load access fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}
