#include "ai_micro_cross_page_fault_common.h"

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x240U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x240U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x6cU;
    const uint8_t sentinel_after = 0xc6U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_set_hs_leaf_pbmt(low_page, 0);
    ai_micro_set_hs_leaf_pbmt(high_page, 0);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page code rewrite sequence starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed code-rewrite fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-page code rewrite, the staged boundary bytes are visible across both backing pages and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_bytes_match(low_phys_start, vector_pattern, 8U) &&
        ai_micro_bytes_match(high_phys_start, vector_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream without fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after fence.i, fetching from the mixed vector-then-scalar rewritten executable HS block returns the final scalar-overwritten value across the page boundary",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page code rewrite plus fence.i sequence, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x260U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x260U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x4dU;
    const uint8_t sentinel_after = 0xd4U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_set_hs_leaf_pbmt(low_page, 0);
    ai_micro_set_hs_leaf_pbmt(high_page, 0);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page code rewrite no-X probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed no-X code-rewrite fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page no-X code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before execute permission is removed",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before execute permission is removed",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    *ai_micro_hs_leaf_entry(low_page) = (phys_page_base(low_page) >> 2) | PTE_V | PTE_AD | PTE_RW;
    *ai_micro_hs_leaf_entry(high_page) = (phys_page_base(high_page) >> 2) | PTE_V | PTE_AD | PTE_RW;
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite, clearing execute permission forces the next fetch to report instruction page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch fault after the mixed vector-then-scalar cross-page rewrite still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x280U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x280U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x5dU;
    const uint8_t sentinel_after = 0xd5U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_set_hs_leaf_pbmt(low_page, 0);
    ai_micro_set_hs_leaf_pbmt(high_page, 0);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page code rewrite execute-recovery probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed execute-recovery code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page execute-recovery code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before execute permission is toggled",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before execute permission is toggled",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    *ai_micro_hs_leaf_entry(low_page) = (phys_page_base(low_page) >> 2) | PTE_V | PTE_AD | PTE_RW;
    *ai_micro_hs_leaf_entry(high_page) = (phys_page_base(high_page) >> 2) | PTE_V | PTE_AD | PTE_RW;
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite, clearing execute permission first forces instruction page fault before execute recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch fault before execute recovery still leaves the final boundary bytes and the outer sentinels unchanged in the mixed cross-page rewrite corner",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    *ai_micro_hs_leaf_entry(low_page) = (phys_page_base(low_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    *ai_micro_hs_leaf_entry(high_page) = (phys_page_base(high_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after re-granting execute permission, fetching from the mixed vector-then-scalar rewritten two-page executable HS block completes normally and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after execute permission is restored in the mixed vector-then-scalar cross-page rewrite corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x2a0U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x2a0U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t bad_vaddr = ai_micro_cross_page_make_noncanonical_vaddr(code_vaddr);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x8aU;
    const uint8_t sentinel_after = 0xa8U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_set_hs_leaf_pbmt(low_page, 0);
    ai_micro_set_hs_leaf_pbmt(high_page, 0);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page non-canonical fetch probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed non-canonical code-rewrite fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page non-canonical code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the non-canonical fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before the non-canonical fetch probe",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite, a non-canonical fetch still reports instruction page fault with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the non-canonical fetch fault after the mixed vector-then-scalar cross-page rewrite still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page non-canonical fetch fault, a subsequent canonical fetch still executes the final scalar-overwritten instruction stream",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar cross-page non-canonical fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x2c0U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x2c0U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t bad_vaddr = (uintptr_t)-1;
    const uintptr_t fetch_target = bad_vaddr & ~1ULL;
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x8bU;
    const uint8_t sentinel_after = 0xb8U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_set_hs_leaf_pbmt(low_page, 0);
    ai_micro_set_hs_leaf_pbmt(high_page, 0);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page all-ones fetch probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed all-ones code-rewrite fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page all-ones code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the all-ones fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before the all-ones fetch probe",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite, an all-ones fetch still reports instruction page fault with the JALR-cleared target in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the all-ones fetch fault after the mixed vector-then-scalar cross-page rewrite still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page all-ones fetch fault, a subsequent canonical fetch still executes the final scalar-overwritten instruction stream",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar cross-page all-ones fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x2e0U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x2e0U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = code_vaddr + 4U;
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x6eU;
    const uint8_t sentinel_after = 0xe6U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_set_hs_leaf_pbmt(low_page, 0);
    ai_micro_set_hs_leaf_pbmt(high_page, 0);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page leaf-overflow fetch-AF probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed leaf-overflow code-rewrite fetch-AF probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page leaf-overflow code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the leaf-overflow fetch-AF probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before the leaf-overflow fetch-AF probe",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    *ai_micro_hs_leaf_entry(high_page) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite, overflowing only the upper-page leaf PPN forces the next split fetch to report instruction access fault on the true second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the leaf-overflow fetch access fault after the mixed vector-then-scalar cross-page rewrite still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x300U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x300U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = code_vaddr + 4U;
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x6fU;
    const uint8_t sentinel_after = 0xf6U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_set_hs_leaf_pbmt(low_page, 0);
    ai_micro_set_hs_leaf_pbmt(high_page, 0);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page leaf-overflow recovery probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed leaf-overflow recovery code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page leaf-overflow recovery code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the leaf-overflow recovery probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before the leaf-overflow recovery probe",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    *ai_micro_hs_leaf_entry(high_page) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite, overflowing the upper-page leaf PPN first forces instruction access fault before the leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the leaf-overflow fetch access fault before recovery still leaves the final boundary bytes and the outer sentinels unchanged in the mixed cross-page rewrite corner",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_set_hs_leaf_pbmt(low_page, 0);
    ai_micro_set_hs_leaf_pbmt(high_page, 0);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after repairing the upper-page leaf entry, fetching from the mixed vector-then-scalar rewritten two-page executable HS block completes normally and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after leaf repair in the mixed vector-then-scalar cross-page rewrite corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_io_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWXPbmt0_GURWXPbmt1, 4U);

    ai_micro_prepare_cross_page_high_half_io_pbmt(VSRWXPbmt0_GURWXPbmt1,
        VSRWXPbmt0_GURWXPbmt2);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS cross-page misaligned load reports load access fault when the upper half falls into a device page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    ai_micro_reset_cross_page_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_page_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRW_GX, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRX_GRWX);
    uintptr_t high_phys = phys_page_base(VSRX_GRWX);
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("an HS cross-page misaligned store reports the upper-page fault address when only the high half loses write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a high-half store page fault does not modify the upper-page bytes",
        read64(high_phys) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_page_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRW_GX, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRX_GRWX);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRW_GX);
    uintptr_t high_phys = phys_page_base(VSRX_GRWX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("a cross-page store with a high-half page fault reports the second-half address and leaves both participating pages unchanged",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == high_half_vaddr &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_page_fault_then_valid_store_recovers_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_page_addr(VSRW_GX, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRX_GRWX);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRW_GX);
    uintptr_t high_phys = phys_page_base(VSRX_GRWX);
    uintptr_t ok_vaddr = hs_page_base(VSRW_GRWX) + 0x120U;
    uintptr_t ok_phys = phys_page_base(VSRW_GRWX) + 0x120U;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t ok_pattern = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(ok_phys, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("the first faulting split store reports a high-half page fault at the second-half address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting split-store path leaves both participating pages unchanged before the recovery store is issued",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, ok_pattern);
    TEST_ASSERT("after the no-side-effect store page-fault corner, a following aligned HS store to a different valid page still commits normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovery store updates only its target word while the earlier faulting split-store footprint remains unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(ok_phys) == ok_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_page_fault_then_valid_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_page_addr(VSRW_GX, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRX_GRWX);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRW_GX);
    uintptr_t high_phys = phys_page_base(VSRX_GRWX);
    uintptr_t ok_vaddr = hs_page_base(VSRW_GRWX) + 0x120U;
    uintptr_t ok_prev_phys = phys_page_base(VSRW_GRWX) + 0x118U;
    uintptr_t ok_phys = phys_page_base(VSRW_GRWX) + 0x120U;
    uintptr_t ok_next_phys = phys_page_base(VSRW_GRWX) + 0x128U;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t ok_prev_pattern = 0x8877665544332211ULL;
    const uint64_t ok_pattern = 0xfedcba9876543210ULL;
    const uint64_t ok_next_pattern = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(ok_prev_phys, ok_prev_pattern);
    write64(ok_phys, 0ULL);
    write64(ok_next_phys, ok_next_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("the first faulting split store reports a high-half page fault at the second-half address in the adjacent-preservation case",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting split-store path leaves both participating pages and the recovery target plus adjacent doublewords unchanged before the recovery store",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(ok_prev_phys) == ok_prev_pattern &&
        read64(ok_phys) == 0ULL &&
        read64(ok_next_phys) == ok_next_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, ok_pattern);
    TEST_ASSERT("after the no-side-effect store page-fault corner, a following aligned HS store to a different valid page still commits normally in the adjacent-preservation case",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovery store updates only its target word while both its adjacent doublewords and the earlier faulting split-store footprint remain unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(ok_prev_phys) == ok_prev_pattern &&
        read64(ok_phys) == ok_pattern &&
        read64(ok_next_phys) == ok_next_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t high_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = ai_micro_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t second_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_second_fault =
        ai_micro_cross_page_expected_low_after_4b_store(low_pattern, second_store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_A | PTE_R);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("the first split-store probe reports a low-half page fault with tval bound to the original misaligned cross-page store address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first low-half page-faulting split store leaves both participating pages unchanged before the same-window high-half refresh probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store_value);
    TEST_ASSERT("after the earlier low-half page fault has already populated the exception path, the follow-up high-half page fault on the same split-store window must refresh tval to the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refreshed high-half page-fault probe exposes only the low-half fragment from the newer store value while the upper page still keeps its original bytes",
        read64(low_phys) == expected_low_after_second_fault &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_partial_commit_then_late_page_fault_blocks_following_flows_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_vector_store_cross_page_partial_fault_window(false);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_repeated_late_page_fault_then_repaired_full_store_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GX;
    const enum test_page high_page = VSRX_GRWX;
    const enum test_page src_page = SWITCH1;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x200U;
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys - 1U;
    const uintptr_t sentinel_after_addr = high_phys + 8U;
    const uint8_t sentinel_before = 0x69U;
    const uint8_t sentinel_after = 0x96U;
    const uint8_t recovered_pattern[16] = {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7
    };

    test_status = test_status &&
        ai_micro_vector_store_cross_page_partial_fault_window(true);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)src_vaddr, 16);
    TEST_ASSERT("after repairing the blocked upper page, the recovery probe can stage the same 16-byte vector-store payload again",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("after the repeated late page-fault episodes and upper-page repair, the same vector cross-page store completes without exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after recovery, the repaired vector cross-page store commits the full 16-byte payload across both pages with no stale fault-era truncation",
        ai_micro_bytes_match(low_phys, recovered_pattern, 8U) &&
        ai_micro_bytes_match(high_phys, recovered_pattern + 8U, 8U)
    );
    TEST_ASSERT("the repaired vector cross-page store also preserves the outer sentinels around the two-page window",
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_late_fault_vecwriteback_owner_not_rebound_to_younger_vector_store_corner()
{
    TEST_START();

    bool ok = ai_micro_vector_store_late_fault_then_younger_vector_store_owner_bound(false);
    TEST_ASSERT("late vector split-store writeback after page-fault repair updates only its original owner, not the younger vector store",
        ok
    );

    TEST_END();
}

bool ai_micro_hs_vector_store_late_fault_vecwriteback_owner_not_rebound_to_younger_vector_store_adjacent_preserved_corner()
{
    TEST_START();

    bool ok = ai_micro_vector_store_late_fault_then_younger_vector_store_owner_bound(true);
    TEST_ASSERT("late vector store writeback owner binding preserves younger-store payloads and outer page sentinels",
        ok
    );

    TEST_END();
}

bool ai_micro_hs_vector_load_unitstride_split_element_second_flow_page_fault_no_partial_dest_update_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const enum test_page old_page = SWITCH1;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = ai_micro_cross_page_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x200U;
    const uintptr_t old_phys = phys_page_base(old_page) + 0x200U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x240U;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + 0x240U;
    const uintptr_t boundary_before_addr = low_phys - 1U;
    const uintptr_t boundary_after_addr = high_phys + 9U;
    const uintptr_t scratch_before_addr = scratch_phys - 1U;
    const uintptr_t scratch_after_addr = scratch_phys + 16U;
    const uint8_t boundary_before = 0x6dU;
    const uint8_t boundary_after = 0xd6U;
    const uint8_t scratch_before = 0x3aU;
    const uint8_t scratch_after = 0xa3U;
    const uint8_t old_vd_bytes[16] = {
        0xd0, 0xd1, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb,
        0xdc, 0xdd, 0xde, 0xdf
    };
    const uint8_t fault_src_bytes[16] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab,
        0xac, 0xad, 0xae, 0xaf
    };
    uint8_t expected_scratch[16];
    uint64_t observed_vstart = 0;

    memcpy(expected_scratch, old_vd_bytes, sizeof(expected_scratch));
    memcpy(expected_scratch, fault_src_bytes, 4U);

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys, fault_src_bytes, 7U);
    ai_micro_write_bytes(high_phys, fault_src_bytes + 7U, 9U);
    ai_micro_write_bytes(old_phys, old_vd_bytes, 16U);
    ai_micro_write_bytes(scratch_phys, old_vd_bytes, 16U);
    write8(boundary_before_addr, boundary_before);
    write8(boundary_after_addr, boundary_after);
    write8(scratch_before_addr, scratch_before);
    write8(scratch_after_addr, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("the preload vector load seeds v6 with the full old 4x32-bit payload before the split-element late-fault probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a 4x32-bit unit-stride vector load starting at page_end-7 traps on the second flow with the first faulting element reported as element1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 4);
    TEST_ASSERT("after the late page fault is taken, dumping v6 into scratch completes so the destination register footprint can be checked directly",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting split-element vector load updates only element0, while the faulting element1 and all later elements must retain their old bytes",
        ai_micro_bytes_match(scratch_phys, expected_scratch, 16U)
    );
    TEST_ASSERT("the split load probe preserves both the original source bytes and the surrounding boundary and scratch sentinels",
        ai_micro_bytes_match(low_phys, fault_src_bytes, 7U) &&
        ai_micro_bytes_match(high_phys, fault_src_bytes + 7U, 9U) &&
        read8(boundary_before_addr) == boundary_before &&
        read8(boundary_after_addr) == boundary_after &&
        read8(scratch_before_addr) == scratch_before &&
        read8(scratch_after_addr) == scratch_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_unitstride_split_element_second_flow_page_fault_no_partial_faulting_element_commit_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH1;
    const uintptr_t fault_vaddr = ai_micro_cross_page_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x280U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x280U;
    const uintptr_t boundary_before_addr = low_phys - 1U;
    const uintptr_t boundary_after_addr = high_phys + 9U;
    const uint8_t boundary_before = 0x5eU;
    const uint8_t boundary_after = 0xe5U;
    const uint8_t dest_initial[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t src_pattern[16] = {
        0xc0, 0xc1, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb,
        0xcc, 0xcd, 0xce, 0xcf
    };
    uint8_t expected_after_fault[16];
    uint64_t observed_vstart = 0;

    memcpy(expected_after_fault, dest_initial, sizeof(expected_after_fault));
    memcpy(expected_after_fault, src_pattern, 4U);

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys, dest_initial, 7U);
    ai_micro_write_bytes(high_phys, dest_initial + 7U, 9U);
    ai_micro_write_bytes(src_phys, src_pattern, 16U);
    write8(boundary_before_addr, boundary_before);
    write8(boundary_after_addr, boundary_after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the source vector load stages the full 4x32-bit payload before the split-element late-fault store probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a 4x32-bit unit-stride vector store starting at page_end-7 traps on the second flow with the first faulting element reported as element1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the late-fault split-element vector store commits only element0, and must not expose the low 3 bytes of the faulting element1",
        ai_micro_bytes_match(low_phys, expected_after_fault, 7U) &&
        ai_micro_bytes_match(high_phys, expected_after_fault + 7U, 9U)
    );
    TEST_ASSERT("the split store probe also preserves the source payload and the outer sentinels around the 16-byte boundary window",
        ai_micro_bytes_match(src_phys, src_pattern, 16U) &&
        read8(boundary_before_addr) == boundary_before &&
        read8(boundary_after_addr) == boundary_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_unitstride_split_element_third_flow_page_fault_no_partial_faulting_element_commit_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const uintptr_t fault_vaddr = ai_micro_cross_page_addr(low_page, 11U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 11U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x2a0U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x2a0U;
    const uintptr_t boundary_before_addr = low_phys - 1U;
    const uintptr_t boundary_after_addr = high_phys + 5U;
    const uint8_t boundary_before = 0x69U;
    const uint8_t boundary_after = 0x96U;
    const uint8_t dest_initial[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t src_pattern[16] = {
        0xd0, 0xd1, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb,
        0xdc, 0xdd, 0xde, 0xdf
    };
    uint8_t expected_after_fault[16];
    uint64_t observed_vstart = 0;

    memcpy(expected_after_fault, dest_initial, sizeof(expected_after_fault));
    memcpy(expected_after_fault, src_pattern, 8U);

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys, dest_initial, 11U);
    ai_micro_write_bytes(high_phys, dest_initial + 11U, 5U);
    ai_micro_write_bytes(src_phys, src_pattern, 16U);
    write8(boundary_before_addr, boundary_before);
    write8(boundary_after_addr, boundary_after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the source vector load stages the full 4x32-bit payload before the third-element split late-fault store probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a 4x32-bit unit-stride vector store starting at page_end-11 traps on the third element with the first faulting element reported as element2",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 2ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the third-element split page-fault store commits only elements0-1, and must not expose the low 3 bytes of the faulting element2",
        ai_micro_bytes_match(low_phys, expected_after_fault, 11U) &&
        ai_micro_bytes_match(high_phys, expected_after_fault + 11U, 5U)
    );
    TEST_ASSERT("the third-element split page-fault probe also preserves the staged source payload and the outer sentinels around the boundary window",
        ai_micro_bytes_match(src_phys, src_pattern, 16U) &&
        read8(boundary_before_addr) == boundary_before &&
        read8(boundary_after_addr) == boundary_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_unitstride_split_fault_vstart_refreshes_from_second_to_third_flow_page_fault_corner()
{
    TEST_START();

    const enum test_page pair0_low_page = VSRWX_GRWX;
    const enum test_page pair0_high_page = VSRWX_GRW;
    const enum test_page pair1_low_page = VSRW_GRWX;
    const enum test_page pair1_high_page = VSRW_GRW;
    const enum test_page pair0_src_page = SWITCH1;
    const enum test_page pair1_src_page = SWITCH2;
    const uintptr_t pair0_vaddr = ai_micro_cross_page_addr(pair0_low_page, 7U);
    const uintptr_t pair1_vaddr = ai_micro_cross_page_addr(pair1_low_page, 11U);
    const uintptr_t pair0_high_fault_vaddr = hs_page_base(pair0_high_page);
    const uintptr_t pair1_high_fault_vaddr = hs_page_base(pair1_high_page);
    const uintptr_t pair0_low_phys = phys_page_base(pair0_low_page) + PAGE_SIZE - 7U;
    const uintptr_t pair0_high_phys = phys_page_base(pair0_high_page);
    const uintptr_t pair1_low_phys = phys_page_base(pair1_low_page) + PAGE_SIZE - 11U;
    const uintptr_t pair1_high_phys = phys_page_base(pair1_high_page);
    const uintptr_t pair0_src_vaddr = hs_page_base(pair0_src_page) + 0x2c0U;
    const uintptr_t pair1_src_vaddr = hs_page_base(pair1_src_page) + 0x2e0U;
    const uintptr_t pair0_src_phys = phys_page_base(pair0_src_page) + 0x2c0U;
    const uintptr_t pair1_src_phys = phys_page_base(pair1_src_page) + 0x2e0U;
    const uintptr_t pair0_before_addr = pair0_low_phys - 1U;
    const uintptr_t pair0_after_addr = pair0_high_phys + 9U;
    const uintptr_t pair1_before_addr = pair1_low_phys - 1U;
    const uintptr_t pair1_after_addr = pair1_high_phys + 5U;
    const uint8_t pair0_before = 0x16U;
    const uint8_t pair0_after = 0x61U;
    const uint8_t pair1_before = 0x27U;
    const uint8_t pair1_after = 0x72U;
    const uint8_t pair0_initial[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t pair1_initial[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t pair0_src_pattern[16] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab,
        0xac, 0xad, 0xae, 0xaf
    };
    const uint8_t pair1_src_pattern[16] = {
        0xb0, 0xb1, 0xb2, 0xb3,
        0xb4, 0xb5, 0xb6, 0xb7,
        0xb8, 0xb9, 0xba, 0xbb,
        0xbc, 0xbd, 0xbe, 0xbf
    };
    uint8_t pair0_expected_after_fault[16];
    uint8_t pair1_expected_after_fault[16];
    uint64_t first_observed_vstart = 0;
    uint64_t second_observed_vstart = 0;

    memcpy(pair0_expected_after_fault, pair0_initial, sizeof(pair0_expected_after_fault));
    memcpy(pair0_expected_after_fault, pair0_src_pattern, 4U);
    memcpy(pair1_expected_after_fault, pair1_initial, sizeof(pair1_expected_after_fault));
    memcpy(pair1_expected_after_fault, pair1_src_pattern, 8U);

    goto_priv(PRIV_M);
    ai_micro_write_bytes(pair0_low_phys, pair0_initial, 7U);
    ai_micro_write_bytes(pair0_high_phys, pair0_initial + 7U, 9U);
    ai_micro_write_bytes(pair1_low_phys, pair1_initial, 11U);
    ai_micro_write_bytes(pair1_high_phys, pair1_initial + 11U, 5U);
    ai_micro_write_bytes(pair0_src_phys, pair0_src_pattern, 16U);
    ai_micro_write_bytes(pair1_src_phys, pair1_src_pattern, 16U);
    write8(pair0_before_addr, pair0_before);
    write8(pair0_after_addr, pair0_after);
    write8(pair1_before_addr, pair1_before);
    write8(pair1_after_addr, pair1_after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(pair0_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pair0_high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(pair1_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pair1_high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(pair0_src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pair1_src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)pair0_src_vaddr, 4);
    TEST_ASSERT("the first source vector load stages the full 4x32-bit payload before the second-element split page-fault store probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)pair0_vaddr, 4);
    first_observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the first split page-fault probe reports element1 as the first faulting element",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pair0_high_fault_vaddr &&
        first_observed_vstart == 1ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first second-element page fault, only the older pair0 element0 bytes commit while the later pair1 window remains untouched",
        ai_micro_bytes_match(pair0_low_phys, pair0_expected_after_fault, 7U) &&
        ai_micro_bytes_match(pair0_high_phys, pair0_expected_after_fault + 7U, 9U) &&
        ai_micro_bytes_match(pair1_low_phys, pair1_initial, 11U) &&
        ai_micro_bytes_match(pair1_high_phys, pair1_initial + 11U, 5U)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)pair1_src_vaddr, 4);
    TEST_ASSERT("the second source vector load stages a distinct 4x32-bit payload before the third-element split page-fault store probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)pair1_vaddr, 4);
    second_observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the follow-up split page fault must refresh the first faulting element from element1 to element2 instead of reusing stale vstart=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pair1_high_fault_vaddr &&
        second_observed_vstart == 2ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the second split page fault, each destination window reflects only its own strictly earlier committed elements with no faulting-element partial commit",
        ai_micro_bytes_match(pair0_low_phys, pair0_expected_after_fault, 7U) &&
        ai_micro_bytes_match(pair0_high_phys, pair0_expected_after_fault + 7U, 9U) &&
        ai_micro_bytes_match(pair1_low_phys, pair1_expected_after_fault, 11U) &&
        ai_micro_bytes_match(pair1_high_phys, pair1_expected_after_fault + 11U, 5U)
    );
    TEST_ASSERT("the two consecutive split page-fault probes preserve both staged source payloads and all boundary sentinels around the two independent windows",
        ai_micro_bytes_match(pair0_src_phys, pair0_src_pattern, 16U) &&
        ai_micro_bytes_match(pair1_src_phys, pair1_src_pattern, 16U) &&
        read8(pair0_before_addr) == pair0_before &&
        read8(pair0_after_addr) == pair0_after &&
        read8(pair1_before_addr) == pair1_before &&
        read8(pair1_after_addr) == pair1_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_io_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWXPbmt0_GURWXPbmt1, 4U);
    uintptr_t high_phys = ai_micro_cross_page_io_paddr(VSRWXPbmt0_GURWXPbmt2);
    const uint64_t high_pattern = 0x22446688aaccee00ULL;

    goto_priv(PRIV_M);
    write64(high_phys, high_pattern);

    ai_micro_prepare_cross_page_high_half_io_pbmt(VSRWXPbmt0_GURWXPbmt1,
        VSRWXPbmt0_GURWXPbmt2);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("an HS cross-page misaligned store reports store access fault when the upper half falls into a device page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("an upper-half device access fault leaves the device-page bytes unchanged",
        read64(high_phys) == high_pattern
    );

    ai_micro_reset_cross_page_pbmt_env();

    TEST_END();
}
