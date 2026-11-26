#include "ai_micro_cross_16b_cacheable_common.h"

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x2e0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x2e0U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x6bU;
    const uint8_t sentinel_after = 0xb6U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B execute-trigger plus grant-X-recovery probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed execute-trigger plus grant-X-recovery code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B execute-trigger plus grant-X-recovery code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-16B execute-trigger plus grant-X-recovery code-rewrite step, the staged boundary bytes are visible and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_16b_bytes_match(boundary_phys, vector_pattern, sizeof(vector_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus grant-X-recovery fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B execute-trigger plus grant-X-recovery code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, code_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the first fetch still reports breakpoint before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus grant-X-recovery corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus grant-X-recovery corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten executable HS block first falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch page fault before execute recovery still leaves the final boundary bytes and the outer sentinels unchanged in the mixed cross-16B execute-trigger corner",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after restoring execute permission, the mixed vector-then-scalar rewritten executable HS block completes normally again and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later no-X fetch page fault, and final execute-permission repair, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x280U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x280U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x71U;
    const uint8_t sentinel_after = 0x97U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B execute-trigger plus leaf-overflow probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed execute-trigger plus leaf-overflow code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B execute-trigger plus leaf-overflow code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-16B execute-trigger plus leaf-overflow code-rewrite step, the staged boundary bytes are visible and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_16b_bytes_match(boundary_phys, vector_pattern, sizeof(vector_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus leaf-overflow fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B execute-trigger plus leaf-overflow code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, code_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the first fetch still reports breakpoint before the later leaf-overflow fetch access fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus leaf-overflow corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later leaf-overflow fetch access fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus leaf-overflow corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    *ai_micro_cross_16b_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten executable HS block falls back to instruction access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later leaf-overflow fetch access fault, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x290U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x290U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x72U;
    const uint8_t sentinel_after = 0xa7U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B execute-trigger plus leaf-repair probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed execute-trigger plus leaf-repair code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B execute-trigger plus leaf-repair code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-16B execute-trigger plus leaf-repair code-rewrite step, the staged boundary bytes are visible and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_16b_bytes_match(boundary_phys, vector_pattern, sizeof(vector_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus leaf-repair fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B execute-trigger plus leaf-repair code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, code_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the first fetch still reports breakpoint before the later leaf repair sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus leaf-repair corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later leaf repair sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus leaf-repair corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    *ai_micro_cross_16b_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten executable HS block first falls back to instruction access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the leaf-overflow fetch access fault before repair still leaves the final boundary bytes and the outer sentinels unchanged in the mixed cross-16B execute-trigger corner",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after repairing the leaf entry, the mixed vector-then-scalar rewritten executable HS block completes normally again and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later leaf-overflow fetch access fault, and final leaf repair, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x2a0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x2a0U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t bad_vaddr = ai_micro_cross_16b_make_noncanonical_vaddr(code_vaddr);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x75U;
    const uint8_t sentinel_after = 0xd7U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B execute-trigger plus non-canonical fetch probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed execute-trigger plus non-canonical code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B execute-trigger plus non-canonical code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-16B execute-trigger plus non-canonical code-rewrite step, the staged boundary bytes are visible and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_16b_bytes_match(boundary_phys, vector_pattern, sizeof(vector_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus non-canonical fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B execute-trigger plus non-canonical code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, code_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the first fetch still reports breakpoint before the later non-canonical fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus non-canonical corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later non-canonical fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus non-canonical corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten executable HS block falls back to instruction page fault for the non-canonical fetch target with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later non-canonical fetch page fault, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B execute-trigger plus non-canonical fetch fault sequence, a subsequent canonical fetch still executes the final scalar-overwritten instruction stream",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar cross-16B execute-trigger plus non-canonical fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x2c0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x2c0U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t bad_vaddr = (uintptr_t)-1;
    const uintptr_t fetch_target = bad_vaddr & ~1ULL;
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x76U;
    const uint8_t sentinel_after = 0xe7U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B execute-trigger plus all-ones fetch probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed execute-trigger plus all-ones code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B execute-trigger plus all-ones code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-16B execute-trigger plus all-ones code-rewrite step, the staged boundary bytes are visible and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_16b_bytes_match(boundary_phys, vector_pattern, sizeof(vector_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus all-ones fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B execute-trigger plus all-ones code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, code_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the first fetch still reports breakpoint before the later all-ones fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus all-ones corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later all-ones fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus all-ones corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten executable HS block falls back to instruction page fault for the all-ones fetch target with the JALR-cleared target in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later all-ones fetch page fault, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B execute-trigger plus all-ones fetch fault sequence, a subsequent canonical fetch still executes the final scalar-overwritten instruction stream",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar cross-16B execute-trigger plus all-ones fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t patched_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x70, 0x00, 0x82, 0x80
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x1c0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x1c0U;
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = code_phys - 1U;
    const uintptr_t sentinel_after_addr = code_phys + sizeof(warm_pattern);
    const uintptr_t trigger_pc = code_vaddr;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, patched_pattern, sizeof(patched_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the vector cross-16B trigger-fetch probe rewrites the code bytes, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, sizeof(patched_pattern));
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-16B execute-trigger fetch-recovery probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)code_vaddr, sizeof(patched_pattern));
    TEST_ASSERT("a vector unit-stride store can rewrite the executable HS block across a 16B boundary before the execute-trigger fetch-recovery probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store rewrites the executable HS block, the patched bytes are visible and the adjacent sentinels stay unchanged before the execute-trigger fetch-recovery probe",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier vector cross-16B store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint when the execute-PC trigger is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch to the vector-rewritten executable HS block still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier vector cross-16B store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint when the execute-PC trigger remains armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch to the vector-rewritten executable HS block still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after clearing the execute-PC trigger, the rewritten executable HS block fetched through the vector cross-16B store path completes normally and returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store plus repeated execute-trigger fetch sequence, the patched bytes remain intact and the adjacent sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t patched_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x70, 0x00, 0x82, 0x80
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x1d0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x1d0U;
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = code_phys - 1U;
    const uintptr_t sentinel_after_addr = code_phys + sizeof(warm_pattern);
    const uintptr_t trigger_pc = code_vaddr;
    const uint8_t sentinel_before = 0x3dU;
    const uint8_t sentinel_after = 0xd3U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, patched_pattern, sizeof(patched_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the vector cross-16B execute-trigger plus no-X probe rewrites the code bytes, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, sizeof(patched_pattern));
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-16B execute-trigger plus no-X fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)code_vaddr, sizeof(patched_pattern));
    TEST_ASSERT("a vector unit-stride store can rewrite the executable HS block across a 16B boundary before the repeated execute-trigger plus no-X fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store rewrites the executable HS block, the patched bytes are visible and the adjacent sentinels stay unchanged before the repeated execute-trigger plus no-X fetch probe",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier vector cross-16B store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the vector cross-16B execute-trigger plus no-X corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier vector cross-16B store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the vector cross-16B execute-trigger plus no-X corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten executable HS block falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later no-X fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-16B corner",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t patched_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x70, 0x00, 0x82, 0x80
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x1e0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x1e0U;
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = code_phys - 1U;
    const uintptr_t sentinel_after_addr = code_phys + sizeof(warm_pattern);
    const uintptr_t trigger_pc = code_vaddr;
    const uint8_t sentinel_before = 0x3eU;
    const uint8_t sentinel_after = 0xe3U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, patched_pattern, sizeof(patched_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the vector cross-16B execute-trigger plus grant-X-recovery probe rewrites the code bytes, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, sizeof(patched_pattern));
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-16B execute-trigger plus grant-X-recovery fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)code_vaddr, sizeof(patched_pattern));
    TEST_ASSERT("a vector unit-stride store can rewrite the executable HS block across a 16B boundary before the repeated execute-trigger plus grant-X-recovery fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store rewrites the executable HS block, the patched bytes are visible and the adjacent sentinels stay unchanged before the repeated execute-trigger plus grant-X-recovery fetch probe",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier vector cross-16B store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the vector cross-16B execute-trigger plus grant-X-recovery corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier vector cross-16B store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the vector cross-16B execute-trigger plus grant-X-recovery corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten executable HS block first falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch page fault before execute recovery still leaves the patched bytes and adjacent sentinels unchanged in the vector cross-16B execute-trigger corner",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after restoring execute permission, the vector-rewritten executable HS block completes normally again and returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later no-X fetch page fault, and final execute-permission repair, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-16B corner",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_16b_vector_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_16B_FOLLOWUP_NONCANONICAL);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_16b_vector_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_16B_FOLLOWUP_ALL_ONES);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_16b_vector_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FAULT);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_16b_vector_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FIX);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    uintptr_t sentinel_before_addr = boundary_phys - 1U;
    uintptr_t sentinel_after_addr = boundary_phys + 16U;
    uintptr_t scratch_vaddr = hs_page_base(VSRW_GRW) + 0x80U;
    uintptr_t scratch_phys = phys_page_base(VSRW_GRW) + 0x80U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(boundary_phys, pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull 16 bytes across the same cacheable 16B boundary before the mixed vector/scalar trigger-load adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B load, the shared boundary bytes and adjacent sentinels remain unchanged before the scalar trigger-load adjacent-preservation probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-16B payload into a scratch page preserves the original 16-byte boundary byte sequence before the scalar trigger-load adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-16B load still matches the exact boundary payload and keeps both scratch and shared-boundary sentinels unchanged before the scalar trigger-load adjacent-preservation probe",
        ai_micro_cross_16b_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, scalar_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector cross-16B load touches the same cacheable boundary, the first scalar dual-trigger split-load probe still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-16B trigger-load probe still leaves the shared boundary bytes and adjacent sentinels unchanged after the prior vector load",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector cross-16B load, the second scalar dual-trigger split-load probe still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-16B trigger-load probe still leaves the shared boundary bytes and adjacent sentinels unchanged after the prior vector load",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-16B load, clearing only the execute-PC slot still leaves the scalar cross-16B load address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B load probe still leaves the shared boundary bytes and adjacent sentinels unchanged after the prior vector load",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-16B load still completes normally and returns the merged payload even though a vector cross-16B load touched the shared path earlier in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0x4e4d4c4b4a494847ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector/scalar cross-16B trigger-load sequence, the shared boundary bytes and adjacent sentinels still match their expected values",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
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
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x180U;
    uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x180U;
    uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    uintptr_t sentinel_before_addr = boundary_phys - 1U;
    uintptr_t sentinel_after_addr = boundary_phys + 16U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar cross-16B trigger-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte boundary payload across the same cacheable 16B boundary before the scalar trigger-store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store, the boundary bytes match the source payload and adjacent sentinels stay unchanged before the scalar trigger-store probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, scalar_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after a vector cross-16B store touches the same cacheable boundary, the first scalar dual-trigger split-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-16B trigger-store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after the earlier vector cross-16B store, the second scalar dual-trigger split-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-16B trigger-store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-16B store, clearing only the execute-PC slot still leaves the scalar cross-16B store address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-16B store still commits normally even though a vector cross-16B store touched the shared path earlier",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-then-scalar trigger-store sequence, the final boundary bytes reflect the expected overwrite order and adjacent sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-then-scalar trigger-store sequence, a following scalar split-load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == AI_MICRO_CROSS_16B_STORE_PATTERN
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
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
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x180U;
    uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x180U;
    uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    uintptr_t sentinel_before_addr = boundary_phys - 1U;
    uintptr_t sentinel_after_addr = boundary_phys + 16U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar cross-16B trigger-store adjacent-preservation probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte boundary payload across the same cacheable 16B boundary before the scalar trigger-store adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store, the boundary bytes match the source payload and adjacent sentinels stay unchanged before the scalar trigger-store adjacent-preservation probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, scalar_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after a vector cross-16B store touches the same cacheable boundary, the first scalar dual-trigger split-store probe still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-16B trigger-store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged in the adjacent-preservation corner",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after the earlier vector cross-16B store, the second scalar dual-trigger split-store probe still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-16B trigger-store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged in the adjacent-preservation corner",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-16B store, clearing only the execute-PC slot still leaves the scalar cross-16B store address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B store probe still leaves the vector-written boundary bytes and adjacent sentinels unchanged in the adjacent-preservation corner",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-16B store still commits normally even though a vector cross-16B store touched the shared path earlier in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-then-scalar trigger-store adjacent-preservation sequence, the final boundary bytes reflect the expected overwrite order and adjacent sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-then-scalar trigger-store adjacent-preservation sequence, a following scalar split-load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == AI_MICRO_CROSS_16B_STORE_PATTERN
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_completion(false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_completion(true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_CROSS_16B_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_CROSS_16B_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_CROSS_16B_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_CROSS_16B_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_7b1b_data_merge_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX,
        AI_MICRO_CROSS_16B_7B1B_OFFSET);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("a 7B+1B cross-16B cacheable load still completes without exception and merges bytes in little-endian order",
        excpt.triggered == false &&
        value == 0x8877665544332211ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_7b1b_data_split_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX,
        AI_MICRO_CROSS_16B_7B1B_OFFSET);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("a 7B+1B cross-16B cacheable store completes without exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a 7B+1B cross-16B store updates the lower aligned doubleword with the leading seven store bytes",
        read64(paddr + 8U) == 0x23456789abcdef00ULL
    );
    TEST_ASSERT("a 7B+1B cross-16B store updates only the first byte of the upper aligned doubleword",
        read64(paddr + 16U) == 0xffeeddccbbaa9901ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_4b4b_3b5b_2b6b_data_merge_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_MID_LD_CASES = 3
    };
    const enum test_page pages[AI_MICRO_CROSS_16B_MID_LD_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t offsets[AI_MICRO_CROSS_16B_MID_LD_CASES] = {
        12U,
        13U,
        14U,
    };
    const unsigned start_bytes[AI_MICRO_CROSS_16B_MID_LD_CASES] = {
        4U,
        5U,
        6U,
    };
    const uint64_t leading_patterns[AI_MICRO_CROSS_16B_MID_LD_CASES] = {
        0x0123456789abcdefULL,
        0x1111222233334444ULL,
        0x5555666677778888ULL,
    };
    const uint64_t low_patterns[AI_MICRO_CROSS_16B_MID_LD_CASES] = {
        0x8070605040302010ULL,
        0xdeadbeefcafef00dULL,
        0xffeeddccbbaa9988ULL,
    };
    const uint64_t high_patterns[AI_MICRO_CROSS_16B_MID_LD_CASES] = {
        0x1122334455667799ULL,
        0x0badf00dfacefeedULL,
        0x66778899aabbccddULL,
    };
    const uint64_t trailing_patterns[AI_MICRO_CROSS_16B_MID_LD_CASES] = {
        0xfedcba9876543210ULL,
        0x99aabbccddeeff00ULL,
        0x123456789abcdef0ULL,
    };
    uintptr_t warm_vaddr[AI_MICRO_CROSS_16B_MID_LD_CASES];
    uintptr_t vaddr[AI_MICRO_CROSS_16B_MID_LD_CASES];
    uintptr_t leading_phys[AI_MICRO_CROSS_16B_MID_LD_CASES];
    uintptr_t low_phys[AI_MICRO_CROSS_16B_MID_LD_CASES];
    uintptr_t high_phys[AI_MICRO_CROSS_16B_MID_LD_CASES];
    uintptr_t trailing_phys[AI_MICRO_CROSS_16B_MID_LD_CASES];
    uint64_t expected[AI_MICRO_CROSS_16B_MID_LD_CASES];
    uint64_t warm0 = 0ULL;
    uint64_t warm1 = 0ULL;
    uint64_t warm2 = 0ULL;
    uint64_t forward0 = 0ULL;
    uint64_t forward1 = 0ULL;
    uint64_t forward2 = 0ULL;
    uint64_t reverse0 = 0ULL;
    uint64_t reverse1 = 0ULL;
    uint64_t reverse2 = 0ULL;

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_MID_LD_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]);
        vaddr[i] = ai_micro_cross_16b_cacheable_addr_with_offset(pages[i], offsets[i]);
        leading_phys[i] = page_phys;
        low_phys[i] = page_phys + 8U;
        high_phys[i] = page_phys + 16U;
        trailing_phys[i] = page_phys + 24U;
        expected[i] = ai_micro_cross_16b_expected_unsigned_window(
            low_patterns[i], high_patterns[i], start_bytes[i], 8U);
    }

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_MID_LD_CASES; ++i) {
        write64(leading_phys[i], leading_patterns[i]);
        write64(low_phys[i], low_patterns[i]);
        write64(high_phys[i], high_patterns[i]);
        write64(trailing_phys[i], trailing_patterns[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warm0 = ld(warm_vaddr[0]);
    warm1 = ld(warm_vaddr[1]);
    warm2 = ld(warm_vaddr[2]);
    TEST_ASSERT("before the middle-ratio cross-16B ld-merge probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
        excpt.triggered == false &&
        warm0 == leading_patterns[0] &&
        warm1 == leading_patterns[1] &&
        warm2 == leading_patterns[2]
    );

    TEST_SETUP_EXCEPT();
    forward0 = ld(vaddr[0]);
    forward1 = ld(vaddr[1]);
    forward2 = ld(vaddr[2]);
    TEST_ASSERT("back-to-back cross-16B ld requests with 4B+4B, 3B+5B, and 2B+6B split shapes each return only their own merged payload instead of inheriting a stale middle-split template",
        excpt.triggered == false &&
        forward0 == expected[0] &&
        forward1 == expected[1] &&
        forward2 == expected[2]
    );

    TEST_SETUP_EXCEPT();
    reverse2 = ld(vaddr[2]);
    reverse1 = ld(vaddr[1]);
    reverse0 = ld(vaddr[0]);
    TEST_ASSERT("reversing the 2B+6B, 3B+5B, and 4B+4B cross-16B ld order still refreshes the split-merge template per request, so no middle-ratio shape inherits stale low-half or high-half ownership",
        excpt.triggered == false &&
        reverse0 == expected[0] &&
        reverse1 == expected[1] &&
        reverse2 == expected[2]
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the middle-ratio cross-16B ld-merge probe leaves every participating boundary doubleword and adjacent sentinel unchanged",
        read64(leading_phys[0]) == leading_patterns[0] &&
        read64(low_phys[0]) == low_patterns[0] &&
        read64(high_phys[0]) == high_patterns[0] &&
        read64(trailing_phys[0]) == trailing_patterns[0] &&
        read64(leading_phys[1]) == leading_patterns[1] &&
        read64(low_phys[1]) == low_patterns[1] &&
        read64(high_phys[1]) == high_patterns[1] &&
        read64(trailing_phys[1]) == trailing_patterns[1] &&
        read64(leading_phys[2]) == leading_patterns[2] &&
        read64(low_phys[2]) == low_patterns[2] &&
        read64(high_phys[2]) == high_patterns[2] &&
        read64(trailing_phys[2]) == trailing_patterns[2]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_4b4b_3b5b_2b6b_data_split_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_MID_SD_CASES = 3
    };
    const enum test_page pages[AI_MICRO_CROSS_16B_MID_SD_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t offsets[AI_MICRO_CROSS_16B_MID_SD_CASES] = {
        12U,
        13U,
        14U,
    };
    const unsigned start_bytes[AI_MICRO_CROSS_16B_MID_SD_CASES] = {
        4U,
        5U,
        6U,
    };
    const uint64_t forward_store_values[AI_MICRO_CROSS_16B_MID_SD_CASES] = {
        0x0123456789abcdefULL,
        0x89abcdef01234567ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t reverse_store_values[AI_MICRO_CROSS_16B_MID_SD_CASES] = {
        0x1021324354657687ULL,
        0x0badf00dfacefeedULL,
        0x3141592653589793ULL,
    };
    const uint64_t leading_patterns[AI_MICRO_CROSS_16B_MID_SD_CASES] = {
        0x1111222233334444ULL,
        0x4444333322221111ULL,
        0x7777666655554444ULL,
    };
    const uint64_t low_patterns[AI_MICRO_CROSS_16B_MID_SD_CASES] = {
        0x7766554433221100ULL,
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t high_patterns[AI_MICRO_CROSS_16B_MID_SD_CASES] = {
        0xffeeddccbbaa9988ULL,
        0x3141592653589793ULL,
        0x1122334455667788ULL,
    };
    const uint64_t trailing_patterns[AI_MICRO_CROSS_16B_MID_SD_CASES] = {
        0xaaaaaaaa55555555ULL,
        0xdddddddd88888888ULL,
        0x123456789abcdef0ULL,
    };
    uintptr_t warm_vaddr[AI_MICRO_CROSS_16B_MID_SD_CASES];
    uintptr_t vaddr[AI_MICRO_CROSS_16B_MID_SD_CASES];
    uintptr_t leading_phys[AI_MICRO_CROSS_16B_MID_SD_CASES];
    uintptr_t low_phys[AI_MICRO_CROSS_16B_MID_SD_CASES];
    uintptr_t high_phys[AI_MICRO_CROSS_16B_MID_SD_CASES];
    uintptr_t trailing_phys[AI_MICRO_CROSS_16B_MID_SD_CASES];
    uint64_t expected_low[AI_MICRO_CROSS_16B_MID_SD_CASES];
    uint64_t expected_high[AI_MICRO_CROSS_16B_MID_SD_CASES];
    uint64_t warm0 = 0ULL;
    uint64_t warm1 = 0ULL;
    uint64_t warm2 = 0ULL;

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_MID_SD_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]);
        vaddr[i] = ai_micro_cross_16b_cacheable_addr_with_offset(pages[i], offsets[i]);
        leading_phys[i] = page_phys;
        low_phys[i] = page_phys + 8U;
        high_phys[i] = page_phys + 16U;
        trailing_phys[i] = page_phys + 24U;
    }

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_MID_SD_CASES; ++i) {
        write64(leading_phys[i], leading_patterns[i]);
        write64(low_phys[i], low_patterns[i]);
        write64(high_phys[i], high_patterns[i]);
        write64(trailing_phys[i], trailing_patterns[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warm0 = ld(warm_vaddr[0]);
    warm1 = ld(warm_vaddr[1]);
    warm2 = ld(warm_vaddr[2]);
    TEST_ASSERT("before the middle-ratio cross-16B sd-split probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
        excpt.triggered == false &&
        warm0 == leading_patterns[0] &&
        warm1 == leading_patterns[1] &&
        warm2 == leading_patterns[2]
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr[0], forward_store_values[0]);
    sd(vaddr[1], forward_store_values[1]);
    sd(vaddr[2], forward_store_values[2]);
    TEST_ASSERT("back-to-back cross-16B sd requests with 4B+4B, 3B+5B, and 2B+6B split shapes all commit without exception in the middle-ratio split-template probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_MID_SD_CASES; ++i) {
        ai_micro_cross_16b_expected_words_after_store(
            low_patterns[i],
            high_patterns[i],
            start_bytes[i],
            8U,
            forward_store_values[i],
            &expected_low[i],
            &expected_high[i]);
    }
    TEST_ASSERT("after the forward-order cross-16B sd sequence with 4B+4B, 3B+5B, and 2B+6B splits, each boundary keeps only its own exact byte image instead of inheriting a stale middle-split store template",
        read64(leading_phys[0]) == leading_patterns[0] &&
        read64(low_phys[0]) == expected_low[0] &&
        read64(high_phys[0]) == expected_high[0] &&
        read64(trailing_phys[0]) == trailing_patterns[0] &&
        read64(leading_phys[1]) == leading_patterns[1] &&
        read64(low_phys[1]) == expected_low[1] &&
        read64(high_phys[1]) == expected_high[1] &&
        read64(trailing_phys[1]) == trailing_patterns[1] &&
        read64(leading_phys[2]) == leading_patterns[2] &&
        read64(low_phys[2]) == expected_low[2] &&
        read64(high_phys[2]) == expected_high[2] &&
        read64(trailing_phys[2]) == trailing_patterns[2]
    );

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_MID_SD_CASES; ++i) {
        write64(leading_phys[i], leading_patterns[i]);
        write64(low_phys[i], low_patterns[i]);
        write64(high_phys[i], high_patterns[i]);
        write64(trailing_phys[i], trailing_patterns[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr[2], reverse_store_values[2]);
    sd(vaddr[1], reverse_store_values[1]);
    sd(vaddr[0], reverse_store_values[0]);
    TEST_ASSERT("reversing the 2B+6B, 3B+5B, and 4B+4B cross-16B sd order still commits cleanly, so no middle-ratio split store inherits stale low-half or high-half ownership from the previous request",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_MID_SD_CASES; ++i) {
        ai_micro_cross_16b_expected_words_after_store(
            low_patterns[i],
            high_patterns[i],
            start_bytes[i],
            8U,
            reverse_store_values[i],
            &expected_low[i],
            &expected_high[i]);
    }
    TEST_ASSERT("after the reverse-order middle-ratio cross-16B sd sequence, every participating boundary doubleword and adjacent sentinel still reflects only that boundary's own split bytes",
        read64(leading_phys[0]) == leading_patterns[0] &&
        read64(low_phys[0]) == expected_low[0] &&
        read64(high_phys[0]) == expected_high[0] &&
        read64(trailing_phys[0]) == trailing_patterns[0] &&
        read64(leading_phys[1]) == leading_patterns[1] &&
        read64(low_phys[1]) == expected_low[1] &&
        read64(high_phys[1]) == expected_high[1] &&
        read64(trailing_phys[1]) == trailing_patterns[1] &&
        read64(leading_phys[2]) == leading_patterns[2] &&
        read64(low_phys[2]) == expected_low[2] &&
        read64(high_phys[2]) == expected_high[2] &&
        read64(trailing_phys[2]) == trailing_patterns[2]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_5b3b_6b2b_data_merge_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES = 2
    };
    const enum test_page pages[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
    };
    const uintptr_t offsets[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES] = {
        11U,
        10U,
    };
    const unsigned start_bytes[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES] = {
        3U,
        2U,
    };
    const uint64_t leading_patterns[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES] = {
        0x0123456789abcdefULL,
        0x1111222233334444ULL,
    };
    const uint64_t low_patterns[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES] = {
        0x8070605040302010ULL,
        0xdeadbeefcafef00dULL,
    };
    const uint64_t high_patterns[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES] = {
        0x1122334455667799ULL,
        0x0badf00dfacefeedULL,
    };
    const uint64_t trailing_patterns[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES] = {
        0xfedcba9876543210ULL,
        0x99aabbccddeeff00ULL,
    };
    uintptr_t warm_vaddr[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES];
    uintptr_t vaddr[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES];
    uintptr_t leading_phys[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES];
    uintptr_t low_phys[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES];
    uintptr_t high_phys[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES];
    uintptr_t trailing_phys[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES];
    uint64_t expected[AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES];
    uint64_t warm0 = 0ULL;
    uint64_t warm1 = 0ULL;
    uint64_t forward0 = 0ULL;
    uint64_t forward1 = 0ULL;
    uint64_t reverse0 = 0ULL;
    uint64_t reverse1 = 0ULL;

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]);
        vaddr[i] = ai_micro_cross_16b_cacheable_addr_with_offset(pages[i], offsets[i]);
        leading_phys[i] = page_phys;
        low_phys[i] = page_phys + 8U;
        high_phys[i] = page_phys + 16U;
        trailing_phys[i] = page_phys + 24U;
        expected[i] = ai_micro_cross_16b_expected_unsigned_window(
            low_patterns[i], high_patterns[i], start_bytes[i], 8U);
    }

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_HIGH_MID_LD_CASES; ++i) {
        write64(leading_phys[i], leading_patterns[i]);
        write64(low_phys[i], low_patterns[i]);
        write64(high_phys[i], high_patterns[i]);
        write64(trailing_phys[i], trailing_patterns[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warm0 = ld(warm_vaddr[0]);
    warm1 = ld(warm_vaddr[1]);
    TEST_ASSERT("before the mirrored middle-ratio cross-16B ld-merge probe, both participating HS pages still warm independently with their own leading sentinel payloads",
        excpt.triggered == false &&
        warm0 == leading_patterns[0] &&
        warm1 == leading_patterns[1]
    );

    TEST_SETUP_EXCEPT();
    forward0 = ld(vaddr[0]);
    forward1 = ld(vaddr[1]);
    TEST_ASSERT("back-to-back cross-16B ld requests with 5B+3B and 6B+2B split shapes each return only their own merged payload instead of inheriting a stale mirrored middle-split template",
        excpt.triggered == false &&
        forward0 == expected[0] &&
        forward1 == expected[1]
    );

    TEST_SETUP_EXCEPT();
    reverse1 = ld(vaddr[1]);
    reverse0 = ld(vaddr[0]);
    TEST_ASSERT("reversing the 6B+2B and 5B+3B cross-16B ld order still refreshes the mirrored split-merge template per request, so no shape inherits stale low-half or high-half ownership",
        excpt.triggered == false &&
        reverse0 == expected[0] &&
        reverse1 == expected[1]
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the mirrored middle-ratio cross-16B ld-merge probe leaves every participating boundary doubleword and adjacent sentinel unchanged",
        read64(leading_phys[0]) == leading_patterns[0] &&
        read64(low_phys[0]) == low_patterns[0] &&
        read64(high_phys[0]) == high_patterns[0] &&
        read64(trailing_phys[0]) == trailing_patterns[0] &&
        read64(leading_phys[1]) == leading_patterns[1] &&
        read64(low_phys[1]) == low_patterns[1] &&
        read64(high_phys[1]) == high_patterns[1] &&
        read64(trailing_phys[1]) == trailing_patterns[1]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_5b3b_6b2b_data_split_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES = 2
    };
    const enum test_page pages[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
    };
    const uintptr_t offsets[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES] = {
        11U,
        10U,
    };
    const unsigned start_bytes[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES] = {
        3U,
        2U,
    };
    const uint64_t forward_store_values[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES] = {
        0x0123456789abcdefULL,
        0x89abcdef01234567ULL,
    };
    const uint64_t reverse_store_values[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES] = {
        0x1021324354657687ULL,
        0x0badf00dfacefeedULL,
    };
    const uint64_t leading_patterns[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES] = {
        0x1111222233334444ULL,
        0x4444333322221111ULL,
    };
    const uint64_t low_patterns[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES] = {
        0x7766554433221100ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t high_patterns[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES] = {
        0xffeeddccbbaa9988ULL,
        0x3141592653589793ULL,
    };
    const uint64_t trailing_patterns[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES] = {
        0xaaaaaaaa55555555ULL,
        0xdddddddd88888888ULL,
    };
    uintptr_t warm_vaddr[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES];
    uintptr_t vaddr[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES];
    uintptr_t leading_phys[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES];
    uintptr_t low_phys[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES];
    uintptr_t high_phys[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES];
    uintptr_t trailing_phys[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES];
    uint64_t expected_low[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES];
    uint64_t expected_high[AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES];
    uint64_t warm0 = 0ULL;
    uint64_t warm1 = 0ULL;

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]);
        vaddr[i] = ai_micro_cross_16b_cacheable_addr_with_offset(pages[i], offsets[i]);
        leading_phys[i] = page_phys;
        low_phys[i] = page_phys + 8U;
        high_phys[i] = page_phys + 16U;
        trailing_phys[i] = page_phys + 24U;
    }

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES; ++i) {
        write64(leading_phys[i], leading_patterns[i]);
        write64(low_phys[i], low_patterns[i]);
        write64(high_phys[i], high_patterns[i]);
        write64(trailing_phys[i], trailing_patterns[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warm0 = ld(warm_vaddr[0]);
    warm1 = ld(warm_vaddr[1]);
    TEST_ASSERT("before the mirrored middle-ratio cross-16B sd-split probe, both participating HS pages still warm independently with their own leading sentinel payloads",
        excpt.triggered == false &&
        warm0 == leading_patterns[0] &&
        warm1 == leading_patterns[1]
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr[0], forward_store_values[0]);
    sd(vaddr[1], forward_store_values[1]);
    TEST_ASSERT("back-to-back cross-16B sd requests with 5B+3B and 6B+2B split shapes both commit without exception in the mirrored middle-ratio split-template probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES; ++i) {
        ai_micro_cross_16b_expected_words_after_store(
            low_patterns[i],
            high_patterns[i],
            start_bytes[i],
            8U,
            forward_store_values[i],
            &expected_low[i],
            &expected_high[i]);
    }
    TEST_ASSERT("after the forward-order cross-16B sd sequence with 5B+3B and 6B+2B splits, each boundary keeps only its own exact byte image instead of inheriting a stale mirrored middle-split store template",
        read64(leading_phys[0]) == leading_patterns[0] &&
        read64(low_phys[0]) == expected_low[0] &&
        read64(high_phys[0]) == expected_high[0] &&
        read64(trailing_phys[0]) == trailing_patterns[0] &&
        read64(leading_phys[1]) == leading_patterns[1] &&
        read64(low_phys[1]) == expected_low[1] &&
        read64(high_phys[1]) == expected_high[1] &&
        read64(trailing_phys[1]) == trailing_patterns[1]
    );

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES; ++i) {
        write64(leading_phys[i], leading_patterns[i]);
        write64(low_phys[i], low_patterns[i]);
        write64(high_phys[i], high_patterns[i]);
        write64(trailing_phys[i], trailing_patterns[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr[1], reverse_store_values[1]);
    sd(vaddr[0], reverse_store_values[0]);
    TEST_ASSERT("reversing the 6B+2B and 5B+3B cross-16B sd order still commits cleanly, so no mirrored middle-ratio split store inherits stale low-half or high-half ownership from the previous request",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_HIGH_MID_SD_CASES; ++i) {
        ai_micro_cross_16b_expected_words_after_store(
            low_patterns[i],
            high_patterns[i],
            start_bytes[i],
            8U,
            reverse_store_values[i],
            &expected_low[i],
            &expected_high[i]);
    }
    TEST_ASSERT("after the reverse-order mirrored middle-ratio cross-16B sd sequence, every participating boundary doubleword and adjacent sentinel still reflects only that boundary's own split bytes",
        read64(leading_phys[0]) == leading_patterns[0] &&
        read64(low_phys[0]) == expected_low[0] &&
        read64(high_phys[0]) == expected_high[0] &&
        read64(trailing_phys[0]) == trailing_patterns[0] &&
        read64(leading_phys[1]) == leading_patterns[1] &&
        read64(low_phys[1]) == expected_low[1] &&
        read64(high_phys[1]) == expected_high[1] &&
        read64(trailing_phys[1]) == trailing_patterns[1]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_offset_width_switch_no_stale_split_template_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_store_offset_width_switch_no_stale_split_template_window(false);
    TEST_ASSERT("back-to-back cross-16B scalar stores with changing offset and width use only the current split mask/data template",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_offset_width_switch_no_stale_split_template_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_store_offset_width_switch_no_stale_split_template_window(true);
    TEST_ASSERT("cross-16B scalar store split-template churn preserves both target fragments and adjacent boundary sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_store_same_16b_offset_width_switch_no_stale_direct_shift_mask_corner()
{
    TEST_START();
    bool ok = ai_micro_same_16b_store_offset_width_switch_no_stale_direct_shift_mask_window(false);
    TEST_ASSERT("same-16B direct misaligned stores with changing offset and width shift only the current payload and mask",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_store_same_16b_offset_width_switch_no_stale_direct_shift_mask_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_same_16b_store_offset_width_switch_no_stale_direct_shift_mask_window(true);
    TEST_ASSERT("same-16B direct store shift/mask churn keeps the target image and outer sentinels bound to the current store",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_same_16b_offset_width_switch_no_stale_direct_data_select_corner()
{
    TEST_START();
    bool ok = ai_micro_same_16b_load_offset_width_switch_no_stale_direct_data_select_window(false);
    TEST_ASSERT("same-16B direct misaligned loads refresh data_select_by_offset across changing offset and width",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_same_16b_offset_width_switch_no_stale_direct_data_select_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_same_16b_load_offset_width_switch_no_stale_direct_data_select_window(true);
    TEST_ASSERT("same-16B direct load offset/width churn preserves current selected data and adjacent guard words",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_same_16b_signedness_switch_no_stale_direct_data_select_corner()
{
    TEST_START();
    bool ok = ai_micro_same_16b_signedness_switch_no_stale_direct_data_select_window(false);
    TEST_ASSERT("same-16B direct narrow loads refresh signed and unsigned final data_select semantics between episodes",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_same_16b_signedness_switch_no_stale_direct_data_select_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_same_16b_signedness_switch_no_stale_direct_data_select_window(true);
    TEST_ASSERT("same-16B signedness-switch load sequence keeps each result current while preserving adjacent sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_same_16b_store_load_overlap_no_wrong_forward_or_stale_merge_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_load_overlap_no_wrong_forward_or_stale_merge_window(false);
    TEST_END();
}

bool ai_micro_hs_same_16b_store_load_overlap_no_wrong_forward_or_stale_merge_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_load_overlap_no_wrong_forward_or_stale_merge_window(true);
    TEST_END();
}

bool ai_micro_hs_same_16b_store_then_signed_unsigned_load_no_stale_data_select_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_then_signed_unsigned_load_no_stale_data_select_window(false);
    TEST_END();
}

bool ai_micro_hs_same_16b_store_then_signed_unsigned_load_no_stale_data_select_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_then_signed_unsigned_load_no_stale_data_select_window(true);
    TEST_END();
}

bool ai_micro_hs_same_16b_store_store_overlap_no_stale_shift_or_byte_age_merge_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_store_overlap_no_stale_shift_or_byte_age_merge_window(false);
    TEST_END();
}

bool ai_micro_hs_same_16b_store_store_overlap_no_stale_shift_or_byte_age_merge_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_store_overlap_no_stale_shift_or_byte_age_merge_window(true);
    TEST_END();
}

bool ai_micro_hs_same_16b_store_path_switch_no_stale_direct_or_aligned_merge_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_path_switch_no_stale_direct_or_aligned_merge_window(false);
    TEST_END();
}

bool ai_micro_hs_same_16b_store_path_switch_no_stale_direct_or_aligned_merge_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_path_switch_no_stale_direct_or_aligned_merge_window(true);
    TEST_END();
}

bool ai_micro_hs_same_16b_load_path_switch_no_stale_direct_or_aligned_select_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_load_path_switch_no_stale_direct_or_aligned_select_window(false);
    TEST_END();
}

bool ai_micro_hs_same_16b_load_path_switch_no_stale_direct_or_aligned_select_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_load_path_switch_no_stale_direct_or_aligned_select_window(true);
    TEST_END();
}

bool ai_micro_hs_same_16b_store_path_switch_then_overlap_load_no_stale_merge_or_select_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_path_switch_then_overlap_load_no_stale_merge_or_select_window(false);
    TEST_END();
}

bool ai_micro_hs_same_16b_store_path_switch_then_overlap_load_no_stale_merge_or_select_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_path_switch_then_overlap_load_no_stale_merge_or_select_window(true);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_offset_width_switch_no_stale_merge_template_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_offset_width_switch_no_stale_merge_template_window(false);
    TEST_ASSERT("back-to-back cross-16B scalar loads with changing offset and width refresh the MAB merge template",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_offset_width_switch_no_stale_merge_template_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_offset_width_switch_no_stale_merge_template_window(true);
    TEST_ASSERT("cross-16B load merge-template churn returns current payloads while preserving adjacent boundary data",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_signedness_switch_no_stale_data_select_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_signedness_switch_no_stale_data_select_window(false);
    TEST_ASSERT("cross-16B scalar loads refresh final signedness data_select between signed and unsigned episodes",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_signedness_switch_no_stale_data_select_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_signedness_switch_no_stale_data_select_window(true);
    TEST_ASSERT("cross-16B signedness-switch loads keep current sign/zero extension and adjacent boundary sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_trigger_breakpoint()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_arm_mcontrol6_0(vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS cross-16B cacheable load reports breakpoint when the misaligned address hits an mcontrol6 load trigger",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("the same HS cross-16B cacheable load completes normally after clearing the trigger",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_second_half_address_trigger_ignored_before_page_fault_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSI_GI);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, second_half_vaddr,
        AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("on Spike, a load-address trigger armed only at the true second-half start does not match a scalar cross-16B page-faulting load, so the original misaligned VA reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second-half-address-triggered page-faulting scalar cross-16B load leaves the backing boundary bytes unchanged before the later fallback fault probe",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    goto_priv(PRIV_M);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing only the second-half-start load trigger, the same scalar cross-16B page-fault path falls back to the underlying load page fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the fallback load page fault, the same backing boundary bytes still remain unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_second_half_address_trigger_ignored_before_access_fault_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, second_half_vaddr,
        AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("on Spike, a load-address trigger armed only at the true second-half start does not match a scalar cross-16B access-faulting load, so the original misaligned VA reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second-half-address-triggered access-faulting scalar cross-16B load leaves the backing boundary bytes unchanged before the later fallback fault probe",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    goto_priv(PRIV_M);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing only the second-half-start load trigger, the same scalar cross-16B access-fault path falls back to the underlying load access fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the fallback load access fault, the same backing boundary bytes still remain unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_second_half_address_trigger_ignored_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("on Spike, arming a cross-16B split-load address trigger at the true second-half start address does not match; the load still completes and returns the merged payload",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    TEST_END();
}
