#include "ai_micro_cross_page_fault_common.h"

bool ai_micro_hs_load_cross_page_data_merge()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWX_GRX, 4U);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("an HS cross-page misaligned load merges data from two readable pages in program order",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_hit_high_fill_data_merge()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWX_GRX, 4U);
    uintptr_t low_page_vaddr = hs_page_base(VSRWX_GRX) + PAGE_SIZE - sizeof(uint64_t);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    uint64_t warmed_low = 0;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed_low = ld(low_page_vaddr);
    TEST_ASSERT("the low-half page is readable before the cross-page mixed warm/cold translation probe",
        excpt.triggered == false &&
        warmed_low == low_pattern
    );

    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("a cross-page HS load still merges data correctly when the low-half page was already warmed and the high-half page is first touched by the split access",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_fill_high_hit_data_merge()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWX_GRX, 4U);
    uintptr_t high_page_vaddr = hs_page_base(VSRWX_GR);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    uint64_t warmed_high = 0;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed_high = ld(high_page_vaddr);
    TEST_ASSERT("the high-half page is readable before the cross-page mixed warm/cold translation probe",
        excpt.triggered == false &&
        warmed_high == high_pattern
    );

    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("a cross-page HS load still merges data correctly when the high-half page was already warmed and the low-half page is first touched by the split access",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_1b7b_data_merge_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWX_GRX, 1U);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("an HS cross-page load still merges bytes correctly when only one low-half byte remains in the first page",
        excpt.triggered == false &&
        value == 0xbbccdd0011223311ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_7b1b_data_merge_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWX_GRX, 7U);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("an HS cross-page load still merges bytes correctly when only one high-half byte is taken from the second page",
        excpt.triggered == false &&
        value == 0x3311223344556677ULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_load_stable_corner()
{
    TEST_START();

    const uint8_t pattern[16] = {
        0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b,
        0x8c, 0x8d, 0x8e, 0x8f
    };
    const uintptr_t vector_vaddr = ai_micro_cross_page_addr(VSRWX_GRX, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_page_addr(VSRWX_GRX, 4U);
    const uintptr_t low_phys_start = phys_page_base(VSRWX_GRX) + PAGE_SIZE - 8U;
    const uintptr_t high_phys_start = phys_page_base(VSRWX_GR);
    const uintptr_t scratch_vaddr = hs_page_base(VSRWX_GRWX) + 0x40U;
    const uintptr_t scratch_phys = phys_page_base(VSRWX_GRWX) + 0x40U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys_start, pattern, 8U);
    ai_micro_write_bytes(high_phys_start, pattern + 8U, 8U);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull 16 bytes across the same HS page boundary before the scalar split-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector boundary payload into a scratch page preserves the original 16-byte cross-page byte sequence",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-page load matches the exact 16-byte boundary payload and keeps adjacent sentinels unchanged",
        ai_micro_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector cross-page load touches the same page pair, a scalar cross-page load still returns the expected merged payload without sticky split state",
        excpt.triggered == false &&
        value == 0x8b8a898887868584ULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_load_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t pattern[16] = {
        0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b,
        0x8c, 0x8d, 0x8e, 0x8f
    };
    const uintptr_t vector_vaddr = ai_micro_cross_page_addr(VSRWX_GRX, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_page_addr(VSRWX_GRX, 4U);
    const uintptr_t low_phys_start = phys_page_base(VSRWX_GRX) + PAGE_SIZE - 8U;
    const uintptr_t high_phys_start = phys_page_base(VSRWX_GR);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uintptr_t scratch_vaddr = hs_page_base(VSRWX_GRWX) + 0x40U;
    const uintptr_t scratch_phys = phys_page_base(VSRWX_GRWX) + 0x40U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys_start, pattern, 8U);
    ai_micro_write_bytes(high_phys_start, pattern + 8U, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull 16 bytes across the same HS page boundary before the scalar split-load adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page load, the shared boundary bytes and adjacent sentinels remain unchanged before the scalar split-load adjacent-preservation probe",
        ai_micro_bytes_match(low_phys_start, pattern, 8U) &&
        ai_micro_bytes_match(high_phys_start, pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector boundary payload into a scratch page preserves the original 16-byte cross-page byte sequence in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-page load matches the exact 16-byte boundary payload and keeps both scratch and shared-boundary sentinels unchanged",
        ai_micro_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector cross-page load touches the same page pair, a scalar cross-page load still returns the expected merged payload without sticky split state in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0x8b8a898887868584ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector/scalar cross-page load sequence, the shared boundary bytes and both sets of sentinels still match their expected values",
        ai_micro_bytes_match(low_phys_start, pattern, 8U) &&
        ai_micro_bytes_match(high_phys_start, pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_load_after_vector_store_stable(false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_load_after_vector_store_stable(true);
    TEST_END();
}

bool ai_micro_hs_vload_mergebuffer_delayed_wb_not_applied_to_reused_mbindex_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_vector_load_fault_then_other_vector_load_no_rebind(false, false);
    TEST_END();
}

bool ai_micro_hs_vload_mergebuffer_inactive_wb_metadata_not_read_from_reused_entry_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_vector_load_fault_then_other_vector_load_no_rebind(true, true);
    TEST_END();
}

bool ai_micro_hs_vector_load_merge_buffer_pressure_no_hang_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_vector_load_merge_pressure(false);
    TEST_END();
}

bool ai_micro_hs_vector_load_merge_buffer_pressure_with_redirect_or_exception_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_vector_load_merge_pressure(true);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_page_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSX_GX, 4U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS cross-page misaligned load reports a low-half page fault when the first page has no read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_nc_misaligned()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWXPbmt0_GURWXPbmt2, 4U);

    ai_micro_prepare_cross_page_pbmt(VSRWXPbmt0_GURWXPbmt2, 0,
        VSRWXPbmt1_GURWXPbmt0, PTE_Pbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS cross-page misaligned load reports load misaligned when only the upper half falls into an NC page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_nc_misaligned()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWXPbmt1_GURWXPbmt0, 4U);

    ai_micro_prepare_cross_page_pbmt(VSRWXPbmt1_GURWXPbmt0, PTE_Pbmt1,
        VSRWXPbmt1_GURWXPbmt1, PTE_Pbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS cross-page misaligned load reports load misaligned when the lower half already falls into an NC page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_page_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS cross-page misaligned load reports the second-half start address when only the high half loses read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_page_fault_then_valid_load_recovers_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_page_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = fault_vaddr + 4U;
    uintptr_t ok_vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX) + 0x180U;
    const uint64_t ok_pattern = 0x0123456789abcdefULL;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(ok_phys, ok_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(fault_vaddr);
    TEST_ASSERT("an HS cross-page load first reports a second-half page fault when only the upper page loses read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after the second-half load page fault, a following aligned HS load to a different valid page still succeeds immediately",
        excpt.triggered == false &&
        value == ok_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_page_fault_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_page_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = fault_vaddr + 4U;
    uintptr_t ok_vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t ok_prev_phys = phys_page_base(VSRWX_GRWX) + 0x178U;
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t ok_next_phys = phys_page_base(VSRWX_GRWX) + 0x188U;
    const uint64_t ok_prev_pattern = 0x8877665544332211ULL;
    const uint64_t ok_pattern = 0x0123456789abcdefULL;
    const uint64_t ok_next_pattern = 0xfedcba9876543210ULL;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(ok_prev_phys, ok_prev_pattern);
    write64(ok_phys, ok_pattern);
    write64(ok_next_phys, ok_next_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(fault_vaddr);
    TEST_ASSERT("an HS cross-page load first reports a second-half page fault when only the upper page loses read permission in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second-half load page-fault path still leaves the recovery target word and its adjacent doublewords unchanged before the follow-up aligned load",
        read64(ok_prev_phys) == ok_prev_pattern &&
        read64(ok_phys) == ok_pattern &&
        read64(ok_next_phys) == ok_next_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after the second-half load page fault, a following aligned HS load to a different valid page still succeeds immediately in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == ok_pattern
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovery load completes, the recovery target word and its adjacent doublewords still match their expected values",
        read64(ok_prev_phys) == ok_prev_pattern &&
        read64(ok_phys) == ok_pattern &&
        read64(ok_next_phys) == ok_next_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_A | PTE_X);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first split-load probe reports a low-half page fault with tval bound to the original misaligned cross-page load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first low-half page-faulting split load leaves both participating pages unchanged before the same-window high-half refresh probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_A | PTE_R);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after the earlier low-half page fault has already populated the exception path, the follow-up high-half page fault on the same split-load window must refresh tval to the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refreshed high-half page-fault probe still leaves both boundary doublewords unchanged because faulting split loads must not create visible memory side effects",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_page_fault_then_access_fault_refreshes_cause_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_A | PTE_R);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first high-half split-load probe reports load page fault with tval at the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first high-half load page fault leaves both participating boundary doublewords unchanged before the follow-up access-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("when the same high-half split-load window later turns into an access fault, MemBlock must refresh the cause from load page fault to load access fault while keeping the same second-half tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the page-fault-then-access-fault split-load cause-refresh probe still leaves both boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_access_fault_then_page_fault_refreshes_cause_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x123456789abcdef0ULL;
    const uint64_t high_pattern = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first high-half split-load probe reports load access fault with tval at the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first high-half load access fault leaves both participating boundary doublewords unchanged before the follow-up page-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("when the same high-half split-load window later turns into a page fault, MemBlock must refresh the cause from load access fault to load page fault while keeping the same second-half tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the access-fault-then-page-fault split-load cause-refresh probe still leaves both boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cross_page_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_store_fault =
        ai_micro_cross_page_expected_low_after_4b_store(low_pattern, store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_A | PTE_X);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first shared cross-page exception probe comes from LoadUnit and reports a low-half load page fault with tval at the original split-load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first low-half load page fault, both participating boundary doublewords remain unchanged before the follow-up shared store-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("when the next shared cross-page exception comes from StoreUnit on the same window, MemBlock must refresh both cause and tval to the newer high-half store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the load-then-store shared-fault switch still shows the newer store-side low-half fragment visibility while the faulting upper page keeps its original bytes",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_page_fault_then_access_fault_refreshes_cause_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0xfedcba9876543210ULL;
    const uint64_t second_store_value = 0x89abcdef01234567ULL;
    const uint64_t expected_low_after_first_fault =
        ai_micro_cross_page_expected_low_after_4b_store(low_pattern, first_store_value);
    const uint64_t expected_low_after_second_fault =
        ai_micro_cross_page_expected_low_after_4b_store(expected_low_after_first_fault, second_store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("the first high-half split-store probe reports store page fault with tval at the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first high-half store page fault exposes only the low-half fragment from the first store value while the upper-page bytes remain unchanged before the access-fault probe",
        read64(low_phys) == expected_low_after_first_fault &&
        read64(high_phys) == high_pattern
    );
    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store_value);
    TEST_ASSERT("when the same high-half split-store window later turns into an access fault, MemBlock must refresh the cause from store page fault to store access fault while keeping the same second-half tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the page-fault-then-access-fault split-store cause-refresh probe updates only the low-half fragment to the newer store value while the upper page still keeps its original bytes",
        read64(low_phys) == expected_low_after_second_fault &&
        read64(high_phys) == high_pattern
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_access_fault_then_page_fault_refreshes_cause_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x8877665544332211ULL;
    const uint64_t high_pattern = 0x1020304050607080ULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t second_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_first_fault =
        ai_micro_cross_page_expected_low_after_4b_store(low_pattern, first_store_value);
    const uint64_t expected_low_after_second_fault =
        ai_micro_cross_page_expected_low_after_4b_store(expected_low_after_first_fault, second_store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("the first high-half split-store probe reports store access fault with tval at the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first high-half store access fault exposes only the low-half fragment from the first store value while the upper-page bytes remain unchanged before the page-fault probe",
        read64(low_phys) == expected_low_after_first_fault &&
        read64(high_phys) == high_pattern
    );
    CSRW(CSR_PMPCFG0, 0ULL);

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store_value);
    TEST_ASSERT("when the same high-half split-store window later turns into a page fault, MemBlock must refresh the cause from store access fault to store page fault while keeping the same second-half tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the access-fault-then-page-fault split-store cause-refresh probe updates only the low-half fragment to the newer store value while the upper page still keeps its original bytes",
        read64(low_phys) == expected_low_after_second_fault &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cross_page_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_store_fault =
        ai_micro_cross_page_expected_low_after_4b_store(low_pattern, store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("the first shared cross-page exception probe comes from StoreUnit and reports a high-half store page fault with tval at the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first high-half store page fault, only the low-half fragment from the store becomes visible while the faulting upper-page bytes remain unchanged",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_A | PTE_X);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("when the next shared cross-page exception comes from LoadUnit on the same window, MemBlock must refresh both cause and tval to the newer low-half load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the store-then-load shared-fault switch keeps the earlier store-side low-half fragment stable while the follow-up load fault adds no new memory side effects",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_page_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSR_GX, 4U);
    uintptr_t low_phys = ai_micro_page_end_phys(VSR_GX);
    uintptr_t high_phys = phys_page_base(VSX_GRWX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS cross-page misaligned store reports a low-half page fault when the first page has no write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a low-half store page fault leaves both participating pages unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_nc_misaligned()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRWXPbmt1_GURWXPbmt0, 4U);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRWXPbmt1_GURWXPbmt0);
    uintptr_t high_phys = phys_page_base(VSRWXPbmt1_GURWXPbmt1);
    const uint64_t low_pattern = 0x1021324354657687ULL;
    const uint64_t high_pattern = 0x98badcfe10325476ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_prepare_cross_page_pbmt(VSRWXPbmt1_GURWXPbmt0, PTE_Pbmt1,
        VSRWXPbmt1_GURWXPbmt1, PTE_Pbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0f1e2d3c4b5a6978ULL);
    TEST_ASSERT("an HS cross-page misaligned store reports store misaligned when the lower half falls into an NC page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a lower-half NC misaligned store leaves the upper-page bytes untouched",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_data_split()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS cross-page misaligned store splits a write cleanly across two writable pages",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a successful split store updates both page fragments with the expected byte lanes",
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_1b7b_data_split_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRW_GRW, 1U);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS cross-page store still splits bytes correctly when only one low-half byte remains in the first page",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a 1B+7B split store updates the low-page tail byte and the upper-page leading seven bytes in order",
        read64(low_phys) == 0xef22334455667788ULL &&
        read64(high_phys) == 0xaa0123456789abcdULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_7b1b_data_split_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRW_GRW, 7U);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS cross-page store still splits bytes correctly when only one high-half byte is written into the second page",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a 7B+1B split store updates the low-page leading seven participating bytes and only the first upper-page byte",
        read64(low_phys) == 0x23456789abcdef88ULL &&
        read64(high_phys) == 0xaabbccdd00112201ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_hit_high_fill_data_split()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    uintptr_t low_page_vaddr = hs_page_base(VSRW_GRW) + PAGE_SIZE - sizeof(uint64_t);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    uint64_t warmed_low = 0;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed_low = ld(low_page_vaddr);
    TEST_ASSERT("the low-half page is readable before the cross-page split-store mixed warm/cold probe",
        excpt.triggered == false &&
        warmed_low == low_pattern
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("a cross-page HS store still splits correctly when the low-half page was already warmed and the high-half page is first touched by the split write",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("warming only the low-half page still leaves the split store bytes committed to both page fragments in order",
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_fill_high_hit_data_split()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    uintptr_t high_page_vaddr = hs_page_base(VSRW_GRX);
    uintptr_t low_phys = ai_micro_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    uint64_t warmed_high = 0;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed_high = ld(high_page_vaddr);
    TEST_ASSERT("the high-half page is readable before the cross-page split-store mixed warm/cold probe",
        excpt.triggered == false &&
        warmed_high == high_pattern
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("a cross-page HS store still splits correctly when the high-half page was already warmed and the low-half page is first touched by the split write",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("warming only the high-half page still leaves the split store bytes committed to both page fragments in order",
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_head_split_upper_half_paddr_and_sqdeq_correct_corner()
{
    TEST_START();

    const uintptr_t split_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    const uintptr_t split_prev_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - 16U;
    const uintptr_t split_low_phys = ai_micro_page_end_phys(VSRW_GRW);
    const uintptr_t split_high_phys = phys_page_base(VSRW_GRX);
    const uintptr_t split_next_phys = phys_page_base(VSRW_GRX) + 8U;
    const uintptr_t high_page_probe_vaddr = hs_page_base(VSRW_GRX);
    const uintptr_t younger_same_high_vaddr = hs_page_base(VSRW_GRX) + 0x100U;
    const uintptr_t younger_same_high_phys = phys_page_base(VSRW_GRX) + 0x100U;
    const uintptr_t younger_other_page_vaddr = hs_page_base(SWITCH1) + 0x188U;
    const uintptr_t younger_other_page_phys = phys_page_base(SWITCH1) + 0x188U;
    const uint64_t split_prev_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t split_low_pattern = 0x1122334455667788ULL;
    const uint64_t split_high_pattern = 0xaabbccdd00112233ULL;
    const uint64_t split_next_pattern = 0x8877665544332211ULL;
    const uint64_t younger_same_high_initial = 0x89abcdef01234567ULL;
    const uint64_t younger_same_high_updated = 0x76543210fedcba98ULL;
    const uint64_t younger_other_page_initial = 0x1021324354657687ULL;
    const uint64_t younger_other_page_updated = 0x8877665544332211ULL;
    uint64_t warmed_high = 0;
    uint64_t warmed_younger_same_high = 0;
    uint64_t warmed_younger_other_page = 0;
    uint64_t split_observed = 0;

    goto_priv(PRIV_M);
    write64(split_prev_phys, split_prev_pattern);
    write64(split_low_phys, split_low_pattern);
    write64(split_high_phys, split_high_pattern);
    write64(split_next_phys, split_next_pattern);
    write64(younger_same_high_phys, younger_same_high_initial);
    write64(younger_other_page_phys, younger_other_page_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed_high = ld(high_page_probe_vaddr);
    TEST_ASSERT("the upper-half HS page is readable before the head cross-page split-store bookkeeping corner",
        excpt.triggered == false &&
        warmed_high == split_high_pattern
    );

    TEST_SETUP_EXCEPT();
    warmed_younger_same_high = ld(younger_same_high_vaddr);
    TEST_ASSERT("the younger aligned target on the split store's upper page is readable before the head cross-page split-store bookkeeping corner",
        excpt.triggered == false &&
        warmed_younger_same_high == younger_same_high_initial
    );

    TEST_SETUP_EXCEPT();
    warmed_younger_other_page = ld(younger_other_page_vaddr);
    TEST_ASSERT("the younger aligned target on another page is also readable before the head cross-page split-store bookkeeping corner",
        excpt.triggered == false &&
        warmed_younger_other_page == younger_other_page_initial
    );

    TEST_SETUP_EXCEPT();
    sd(split_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("the older HS head cross-page unaligned store still retires normally before the younger aligned stores behind it",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(younger_same_high_vaddr, younger_same_high_updated);
    TEST_ASSERT("after the older head cross-page split store retires, the younger aligned HS store on the same upper page still retires normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(younger_other_page_vaddr, younger_other_page_updated);
    TEST_ASSERT("after the older head cross-page split store retires, the younger aligned HS store on another page also retires normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    split_observed = ld(split_vaddr);
    TEST_ASSERT("after the head cross-page split store and both younger aligned stores, a following scalar cross-page load still observes the older split payload exactly once",
        excpt.triggered == false &&
        split_observed == 0x0123456789abcdefULL
    );

    TEST_SETUP_EXCEPT();
    warmed_younger_same_high = ld(younger_same_high_vaddr);
    TEST_ASSERT("the younger aligned store on the split store's upper page is immediately visible after the head cross-page split-store sequence",
        excpt.triggered == false &&
        warmed_younger_same_high == younger_same_high_updated
    );

    TEST_SETUP_EXCEPT();
    warmed_younger_other_page = ld(younger_other_page_vaddr);
    TEST_ASSERT("the younger aligned store on the unrelated page is also immediately visible after the head cross-page split-store sequence",
        excpt.triggered == false &&
        warmed_younger_other_page == younger_other_page_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the head cross-page split-store bookkeeping corner commits only the two participating boundary fragments with the correct upper-half paddr while both younger aligned stores also retire exactly once",
        read64(split_prev_phys) == split_prev_pattern &&
        read64(split_low_phys) == 0x89abcdef55667788ULL &&
        read64(split_high_phys) == 0xaabbccdd01234567ULL &&
        read64(split_next_phys) == split_next_pattern &&
        read64(younger_same_high_phys) == younger_same_high_updated &&
        read64(younger_other_page_phys) == younger_other_page_updated
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_reused_upper_half_paddr_not_rebound_to_next_pair_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_page_reused_upper_half_paddr_window(false);
    TEST_ASSERT("consecutive cross-page scalar split stores keep each upper-half paddr and byte image bound to the current page pair",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_store_cross_page_reused_upper_half_paddr_not_rebound_to_next_pair_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_page_reused_upper_half_paddr_window(true);
    TEST_ASSERT("cross-page split-store resident-state reuse preserves both page-pair fragments and adjacent sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_store_misalign_direct_wb_and_mabuf_wb_same_cycle_no_drop_corner()
{
    TEST_START();

    const uintptr_t direct_vaddr = hs_page_base(VSRWX_GRWX) + 15U;
    const uintptr_t direct_prev_phys = phys_page_base(VSRWX_GRWX);
    const uintptr_t direct_low_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t direct_high_phys = phys_page_base(VSRWX_GRWX) + 16U;
    const uintptr_t direct_next_phys = phys_page_base(VSRWX_GRWX) + 24U;
    const uintptr_t mabuf_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    const uintptr_t mabuf_prev_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - 16U;
    const uintptr_t mabuf_low_phys = ai_micro_page_end_phys(VSRW_GRW);
    const uintptr_t mabuf_high_phys = phys_page_base(VSRW_GRX);
    const uintptr_t mabuf_next_phys = phys_page_base(VSRW_GRX) + 8U;
    const uintptr_t younger_vaddr = hs_page_base(SWITCH1) + 0x200U;
    const uintptr_t younger_phys = phys_page_base(SWITCH1) + 0x200U;
    const uint64_t direct_prev_pattern = 0xfaceb00c1234abcdULL;
    const uint64_t direct_low_pattern = 0x7766554433221100ULL;
    const uint64_t direct_high_pattern = 0xffeeddccbbaa9988ULL;
    const uint64_t direct_next_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t mabuf_prev_pattern = 0x8899aabbccddeeffULL;
    const uint64_t mabuf_low_pattern = 0x1021324354657687ULL;
    const uint64_t mabuf_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t mabuf_next_pattern = 0x0123456789abcdefULL;
    const uint64_t younger_initial = 0x1123581321345589ULL;
    const uint64_t younger_updated = 0x89abcdef76543210ULL;
    uint64_t warmed_direct = 0;
    uint64_t warmed_mabuf_high = 0;
    uint64_t warmed_younger = 0;
    uint64_t observed_direct = 0;
    uint64_t observed_mabuf = 0;
    uint64_t observed_younger = 0;

    goto_priv(PRIV_M);
    write64(direct_prev_phys, direct_prev_pattern);
    write64(direct_low_phys, direct_low_pattern);
    write64(direct_high_phys, direct_high_pattern);
    write64(direct_next_phys, direct_next_pattern);
    write64(mabuf_prev_phys, mabuf_prev_pattern);
    write64(mabuf_low_phys, mabuf_low_pattern);
    write64(mabuf_high_phys, mabuf_high_pattern);
    write64(mabuf_next_phys, mabuf_next_pattern);
    write64(younger_phys, younger_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed_direct = ld(hs_page_base(VSRWX_GRWX));
    TEST_ASSERT("the same-page cacheable region is readable before the mixed direct-writeback and StoreMisalignBuffer-store corner",
        excpt.triggered == false &&
        warmed_direct == direct_prev_pattern
    );

    TEST_SETUP_EXCEPT();
    warmed_mabuf_high = ld(hs_page_base(VSRW_GRX));
    TEST_ASSERT("the cross-page store's upper page is readable before the mixed direct-writeback and StoreMisalignBuffer-store corner",
        excpt.triggered == false &&
        warmed_mabuf_high == mabuf_high_pattern
    );

    TEST_SETUP_EXCEPT();
    warmed_younger = ld(younger_vaddr);
    TEST_ASSERT("the younger aligned store target is readable before the mixed direct-writeback and StoreMisalignBuffer-store corner",
        excpt.triggered == false &&
        warmed_younger == younger_initial
    );

    TEST_SETUP_EXCEPT();
    sd(direct_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("the leading same-page cross-16B HS store still retires normally through the direct misalign-completion path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(mabuf_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("the following cross-page HS store still retires normally through the StoreMisalignBuffer-backed split path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(younger_vaddr, younger_updated);
    TEST_ASSERT("after the mixed direct and StoreMisalignBuffer misaligned stores, the younger aligned HS store still retires normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_direct = ld(direct_vaddr);
    TEST_ASSERT("after the mixed misaligned-store window, the same-page cross-16B split store remains visible exactly as its own payload",
        excpt.triggered == false &&
        observed_direct == 0x0123456789abcdefULL
    );

    TEST_SETUP_EXCEPT();
    observed_mabuf = ld(mabuf_vaddr);
    TEST_ASSERT("after the mixed misaligned-store window, the cross-page split store also remains visible exactly as its own payload",
        excpt.triggered == false &&
        observed_mabuf == 0x8877665544332211ULL
    );

    TEST_SETUP_EXCEPT();
    observed_younger = ld(younger_vaddr);
    TEST_ASSERT("after the mixed misaligned-store window, the younger aligned store payload is immediately readable",
        excpt.triggered == false &&
        observed_younger == younger_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the mixed direct-writeback and StoreMisalignBuffer-writeback corner commits each store exactly once, preserves the untouched adjacent words, and does not leak or drop either misaligned completion",
        read64(direct_prev_phys) == direct_prev_pattern &&
        read64(direct_low_phys) == 0xef66554433221100ULL &&
        read64(direct_high_phys) == 0xff0123456789abcdULL &&
        read64(direct_next_phys) == direct_next_pattern &&
        read64(mabuf_prev_phys) == mabuf_prev_pattern &&
        read64(mabuf_low_phys) == 0x4433221154657687ULL &&
        read64(mabuf_high_phys) == 0x98badcfe88776655ULL &&
        read64(mabuf_next_phys) == mabuf_next_pattern &&
        read64(younger_phys) == younger_updated
    );

    TEST_END();
}

bool ai_micro_hs_vector_store_misalign_direct_feedback_and_mabuf_feedback_no_collision_bug_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vector_store_misalign_direct_and_mabuf_window(false);
    TEST_END();
}

bool ai_micro_hs_vector_store_misalign_direct_feedback_and_mabuf_feedback_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vector_store_misalign_direct_and_mabuf_window(true);
    TEST_END();
}

bool ai_micro_hs_store_wb_port0_scalar_then_sq_mabuf_adjacent_window_corner()
{
    TEST_START();

    const uintptr_t scalar_prev_phys = phys_page_base(VSRWX_GRWX) + 0x1c0U;
    const uintptr_t scalar_vaddr = hs_page_base(VSRWX_GRWX) + 0x1c8U;
    const uintptr_t scalar_phys = phys_page_base(VSRWX_GRWX) + 0x1c8U;
    const uintptr_t scalar_next_phys = phys_page_base(VSRWX_GRWX) + 0x1d0U;
    const uintptr_t mabuf_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    const uintptr_t mabuf_prev_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - 16U;
    const uintptr_t mabuf_low_phys = ai_micro_page_end_phys(VSRW_GRW);
    const uintptr_t mabuf_high_phys = phys_page_base(VSRW_GRX);
    const uintptr_t mabuf_next_phys = phys_page_base(VSRW_GRX) + 8U;
    const uintptr_t younger_vaddr = hs_page_base(SWITCH1) + 0x2c0U;
    const uintptr_t younger_phys = phys_page_base(SWITCH1) + 0x2c0U;
    const uint64_t scalar_prev_pattern = 0x1111222233334444ULL;
    const uint64_t scalar_initial = 0x5555666677778888ULL;
    const uint64_t scalar_next_pattern = 0x9999aaaabbbbccccULL;
    const uint64_t scalar_updated = 0x0123456789abcdefULL;
    const uint64_t mabuf_prev_pattern = 0x8899aabbccddeeffULL;
    const uint64_t mabuf_low_pattern = 0x1021324354657687ULL;
    const uint64_t mabuf_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t mabuf_next_pattern = 0x0123456789abcdefULL;
    const uint64_t mabuf_updated = 0x8877665544332211ULL;
    const uint64_t younger_initial = 0x1123581321345589ULL;
    const uint64_t younger_updated = 0x89abcdef76543210ULL;
    uint64_t warmed_scalar = 0;
    uint64_t warmed_mabuf_high = 0;
    uint64_t warmed_younger = 0;
    uint64_t observed_scalar = 0;
    uint64_t observed_mabuf = 0;
    uint64_t observed_younger = 0;

    goto_priv(PRIV_M);
    write64(scalar_prev_phys, scalar_prev_pattern);
    write64(scalar_phys, scalar_initial);
    write64(scalar_next_phys, scalar_next_pattern);
    write64(mabuf_prev_phys, mabuf_prev_pattern);
    write64(mabuf_low_phys, mabuf_low_pattern);
    write64(mabuf_high_phys, mabuf_high_pattern);
    write64(mabuf_next_phys, mabuf_next_pattern);
    write64(younger_phys, younger_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed_scalar = ld(scalar_vaddr);
    TEST_ASSERT("the aligned scalar-store target is readable before the store-unit0 then sqToStout port0-arbitration approximation",
        excpt.triggered == false &&
        warmed_scalar == scalar_initial
    );

    TEST_SETUP_EXCEPT();
    warmed_mabuf_high = ld(hs_page_base(VSRW_GRX));
    TEST_ASSERT("the cross-page split-store upper page is readable before the store-unit0 then sqToStout port0-arbitration approximation",
        excpt.triggered == false &&
        warmed_mabuf_high == mabuf_high_pattern
    );

    TEST_SETUP_EXCEPT();
    warmed_younger = ld(younger_vaddr);
    TEST_ASSERT("the trailing aligned store target is readable before the store-unit0 then sqToStout port0-arbitration approximation",
        excpt.triggered == false &&
        warmed_younger == younger_initial
    );

    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, scalar_updated);
    TEST_ASSERT("the leading aligned HS store retires normally through the scalar store-unit writeback path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(mabuf_vaddr, mabuf_updated);
    TEST_ASSERT("the following cross-page HS store retires normally through the StoreMisalignBuffer-backed sqToStout path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(younger_vaddr, younger_updated);
    TEST_ASSERT("after the scalar-store and sqToStout near-collision window, a younger aligned HS store still retires normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_scalar = ld(scalar_vaddr);
    TEST_ASSERT("after the store-unit0 then sqToStout window, the aligned scalar-store payload remains visible exactly once",
        excpt.triggered == false &&
        observed_scalar == scalar_updated
    );

    TEST_SETUP_EXCEPT();
    observed_mabuf = ld(mabuf_vaddr);
    TEST_ASSERT("after the store-unit0 then sqToStout window, the cross-page split-store payload also remains visible exactly once",
        excpt.triggered == false &&
        observed_mabuf == mabuf_updated
    );

    TEST_SETUP_EXCEPT();
    observed_younger = ld(younger_vaddr);
    TEST_ASSERT("after the store-unit0 then sqToStout window, the younger aligned store payload is immediately readable",
        excpt.triggered == false &&
        observed_younger == younger_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scalar store-unit writeback and sqToStout writeback adjacent-window probe commits each store exactly once and preserves untouched neighboring words",
        read64(scalar_prev_phys) == scalar_prev_pattern &&
        read64(scalar_phys) == scalar_updated &&
        read64(scalar_next_phys) == scalar_next_pattern &&
        read64(mabuf_prev_phys) == mabuf_prev_pattern &&
        read64(mabuf_low_phys) == 0x4433221154657687ULL &&
        read64(mabuf_high_phys) == 0x98badcfe88776655ULL &&
        read64(mabuf_next_phys) == mabuf_next_pattern &&
        read64(younger_phys) == younger_updated
    );

    TEST_END();
}

bool ai_micro_hs_store_wb_port0_sq_mabuf_then_scalar_no_lost_completion_corner()
{
    TEST_START();

    const uintptr_t mabuf_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    const uintptr_t mabuf_prev_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - 16U;
    const uintptr_t mabuf_low_phys = ai_micro_page_end_phys(VSRW_GRW);
    const uintptr_t mabuf_high_phys = phys_page_base(VSRW_GRX);
    const uintptr_t mabuf_next_phys = phys_page_base(VSRW_GRX) + 8U;
    const uintptr_t scalar_prev_phys = phys_page_base(VSRWX_GRWX) + 0x240U;
    const uintptr_t scalar_vaddr = hs_page_base(VSRWX_GRWX) + 0x248U;
    const uintptr_t scalar_phys = phys_page_base(VSRWX_GRWX) + 0x248U;
    const uintptr_t scalar_next_phys = phys_page_base(VSRWX_GRWX) + 0x250U;
    const uintptr_t tail_vaddr = hs_page_base(SWITCH1) + 0x2e0U;
    const uintptr_t tail_phys = phys_page_base(SWITCH1) + 0x2e0U;
    const uint64_t mabuf_prev_pattern = 0x0a0b0c0d0e0f0102ULL;
    const uint64_t mabuf_low_pattern = 0xa1b2c3d4e5f60718ULL;
    const uint64_t mabuf_high_pattern = 0x1029384756abcdefULL;
    const uint64_t mabuf_next_pattern = 0xfedcba9876543210ULL;
    const uint64_t mabuf_updated = 0x0fedcba987654321ULL;
    const uint64_t scalar_prev_pattern = 0x2233445566778899ULL;
    const uint64_t scalar_initial = 0x0123012301230123ULL;
    const uint64_t scalar_next_pattern = 0xa5a55a5aa5a55a5aULL;
    const uint64_t scalar_updated = 0x13579bdf2468ace0ULL;
    const uint64_t tail_initial = 0x55aa55aa33cc33ccULL;
    const uint64_t tail_updated = 0xcafebabedeadbeefULL;
    uint64_t warmed_mabuf_high = 0;
    uint64_t warmed_scalar = 0;
    uint64_t warmed_tail = 0;
    uint64_t observed_mabuf = 0;
    uint64_t observed_scalar = 0;
    uint64_t observed_tail = 0;

    goto_priv(PRIV_M);
    write64(mabuf_prev_phys, mabuf_prev_pattern);
    write64(mabuf_low_phys, mabuf_low_pattern);
    write64(mabuf_high_phys, mabuf_high_pattern);
    write64(mabuf_next_phys, mabuf_next_pattern);
    write64(scalar_prev_phys, scalar_prev_pattern);
    write64(scalar_phys, scalar_initial);
    write64(scalar_next_phys, scalar_next_pattern);
    write64(tail_phys, tail_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed_mabuf_high = ld(hs_page_base(VSRW_GRX));
    TEST_ASSERT("the cross-page split-store upper page is readable before the sqToStout then scalar store-unit0 port0-arbitration approximation",
        excpt.triggered == false &&
        warmed_mabuf_high == mabuf_high_pattern
    );

    TEST_SETUP_EXCEPT();
    warmed_scalar = ld(scalar_vaddr);
    TEST_ASSERT("the aligned scalar-store target is readable before the sqToStout then scalar store-unit0 port0-arbitration approximation",
        excpt.triggered == false &&
        warmed_scalar == scalar_initial
    );

    TEST_SETUP_EXCEPT();
    warmed_tail = ld(tail_vaddr);
    TEST_ASSERT("the trailing aligned verification store target is readable before the sqToStout then scalar store-unit0 port0-arbitration approximation",
        excpt.triggered == false &&
        warmed_tail == tail_initial
    );

    TEST_SETUP_EXCEPT();
    sd(mabuf_vaddr, mabuf_updated);
    TEST_ASSERT("the leading cross-page HS store retires normally through the StoreMisalignBuffer-backed sqToStout path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, scalar_updated);
    TEST_ASSERT("the following aligned HS store still retires normally through the scalar store-unit writeback path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(tail_vaddr, tail_updated);
    TEST_ASSERT("after the sqToStout then scalar writeback window, a trailing aligned HS store still retires normally with no lost completion",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_mabuf = ld(mabuf_vaddr);
    TEST_ASSERT("after the sqToStout then scalar writeback window, the cross-page split-store payload remains visible exactly once",
        excpt.triggered == false &&
        observed_mabuf == mabuf_updated
    );

    TEST_SETUP_EXCEPT();
    observed_scalar = ld(scalar_vaddr);
    TEST_ASSERT("after the sqToStout then scalar writeback window, the aligned scalar-store payload remains visible exactly once",
        excpt.triggered == false &&
        observed_scalar == scalar_updated
    );

    TEST_SETUP_EXCEPT();
    observed_tail = ld(tail_vaddr);
    TEST_ASSERT("after the sqToStout then scalar writeback window, the trailing aligned store payload is immediately readable",
        excpt.triggered == false &&
        observed_tail == tail_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the sqToStout and scalar store-unit writeback reverse-order probe preserves both payloads, both neighboring guard words, and the trailing aligned store with no dropped completion",
        read64(mabuf_prev_phys) == mabuf_prev_pattern &&
        read64(mabuf_low_phys) == 0x87654321e5f60718ULL &&
        read64(mabuf_high_phys) == 0x102938470fedcba9ULL &&
        read64(mabuf_next_phys) == mabuf_next_pattern &&
        read64(scalar_prev_phys) == scalar_prev_pattern &&
        read64(scalar_phys) == scalar_updated &&
        read64(scalar_next_phys) == scalar_next_pattern &&
        read64(tail_phys) == tail_updated
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_store_stable_corner()
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
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x200U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x200U;
    const uintptr_t vector_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    const uintptr_t low_phys_start = phys_page_base(VSRW_GRW) + PAGE_SIZE - 8U;
    const uintptr_t high_phys_start = phys_page_base(VSRW_GRX);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar cross-page store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte boundary payload across two HS pages without fault",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the vector cross-page store, an overlapping scalar cross-page store still commits normally and overwrites only its own eight-byte window",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the final boundary bytes reflect the expected vector-then-scalar overwrite order while adjacent sentinels stay unchanged",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("the overlapping scalar cross-page store remains immediately visible to a following scalar split-load probe after the earlier vector store",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_store_adjacent_preserved_corner()
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
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x200U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x200U;
    const uintptr_t vector_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    const uintptr_t low_phys_start = phys_page_base(VSRW_GRW) + PAGE_SIZE - 8U;
    const uintptr_t high_phys_start = phys_page_base(VSRW_GRX);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar cross-page store adjacent-preservation probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte boundary payload across two HS pages before the scalar split-store adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store, the shared boundary bytes match the staged payload and adjacent sentinels remain unchanged before the scalar split-store adjacent-preservation probe",
        ai_micro_bytes_match(low_phys_start, source_pattern, 8U) &&
        ai_micro_bytes_match(high_phys_start, source_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the vector cross-page store, an overlapping scalar cross-page store still commits normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the final boundary bytes reflect the expected vector-then-scalar overwrite order while adjacent sentinels stay unchanged in the adjacent-preservation corner",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the mixed vector/scalar cross-page store sequence, a following scalar split-load still observes the latest scalar payload immediately in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_store_after_vector_load_stable(false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_scalar_store_after_vector_load_stable(true);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys_start, warm_pattern, 8U);
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
    TEST_ASSERT("before the scalar cross-page store rewrites the two-page executable HS block, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0000806700700513ULL);
    TEST_ASSERT("a scalar cross-page store can rewrite the two-page executable HS block without fault before the fence.i fetch-recovery probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before fence.i",
        ai_micro_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after fence.i, fetching from the scalar-rewritten two-page executable HS block returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-page store plus fence.i fetch-recovery sequence, the patched instruction bytes remain intact and the adjacent sentinels stay unchanged",
        ai_micro_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x1c0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x1c0U;
    const uintptr_t vaddr = ai_micro_cross_page_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    const uint8_t sentinel_before = 0x5aU;
    const uint8_t sentinel_after = 0xa5U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys_start, warm_pattern, 8U);
    ai_micro_write_bytes(source_phys, patched_pattern, 8U);
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
    TEST_ASSERT("before the vector cross-page store rewrites the two-page executable HS block, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 8);
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-page store rewrites the executable HS block",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vaddr, 8);
    TEST_ASSERT("a vector unit-stride store can rewrite the two-page executable HS block across the page boundary without fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before fence.i",
        ai_micro_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after fence.i, fetching from the vector-rewritten two-page executable HS block returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store plus fence.i fetch-recovery sequence, the patched instruction bytes remain intact and the adjacent sentinels stay unchanged",
        ai_micro_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
