#include "ai_micro_difftest_cboinvalid_sync_common.h"

bool ai_micro_hs_cbo_inval_prefetch_refill_then_same_block_cbo_zero_zeroes_refilled_block_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x180U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x180U;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uint64_t target_seed = 0x1122334455667788ULL;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_micro_diffsync_seed_block(target_base_paddr, target_seed);
    ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target block is readable before the cbo.invalid + prefetch_r + cbo.zero difftest-sync probe starts",
        warmed == ai_micro_diffsync_seed_word(target_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the warmed target block can be invalidated before the later silent refill plus same-block cbo.zero probe",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("a same-block software prefetch after cbo.invalid retires silently before the later cbo.zero reaches the store queue",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES - 1U);
    AI_ASSERT_NO_EXCEPT_AND("the same-block cbo.zero also retires without exception immediately after the refill-triggering prefetch",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the refill-triggering prefetch and same-block cbo.zero retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the refill-triggering prefetch and same-block cbo.zero, the target block head reads back as zero",
        observed_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_tail = ld(target_base_vaddr +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES - sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the refill-triggering prefetch and same-block cbo.zero, the target block tail also reads back as zero",
        observed_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block still reads its original payload after the target refill plus same-block cbo.zero sequence",
        observed_adjacent == ai_micro_diffsync_seed_word(adjacent_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refill-triggering prefetch plus same-block cbo.zero sequence zeroes exactly the target cache block while preserving the adjacent block backing image",
        ai_micro_diffsync_block_all_zero(target_base_paddr) &&
        ai_micro_diffsync_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_head_cross16b_store_overlays_refilled_block_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x1c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x1c0U;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t misaligned_store_vaddr = target_base_vaddr + 15U;
    const uint64_t target_seed = 0x1021324354657687ULL;
    const uint64_t adjacent_seed = 0x8899aabbccddeeffULL;
    const uint64_t store_value = 0xd1c2b3a495867748ULL;
    const uint64_t initial_word1 =
        ai_micro_diffsync_seed_word(target_seed, 1U);
    const uint64_t initial_word2 =
        ai_micro_diffsync_seed_word(target_seed, 2U);
    const uint64_t expected_word1 =
        (initial_word1 & 0x00ffffffffffffffULL) |
        ((store_value & 0xffULL) << 56);
    const uint64_t expected_word2 =
        ((store_value >> 8) & 0x00ffffffffffffffULL) |
        (initial_word2 & 0xff00000000000000ULL);

    goto_priv(PRIV_M);
    ai_micro_diffsync_seed_block(target_base_paddr, target_seed);
    ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the cross-16B target word is readable before the cbo.invalid + prefetch_r + head-cross16B-store difftest-sync probe starts",
        warmed == initial_word1
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the warmed target block can be invalidated before the later prefetch-hint window plus head cross-16B store overlay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("a same-block software prefetch after cbo.invalid retires silently before the later head cross-16B store reaches the store queue",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(misaligned_store_vaddr, store_value);
    AI_ASSERT_NO_EXCEPT_AND("the head cross-16B translated store itself retires without exception immediately after the cbo.inval plus prefetch hint window",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the cbo.inval plus prefetch hint window and head cross-16B store retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word1 = ld(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the low overlapped word reflects the final head cross-16B overlay byte after the cbo.inval plus prefetch hint window",
        observed_word1 == expected_word1
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word2 = ld(target_base_vaddr + 16U);
    AI_ASSERT_NO_EXCEPT_AND("the high overlapped word reflects the final head cross-16B overlay bytes after the cbo.inval plus prefetch hint window",
        observed_word2 == expected_word2
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block still reads its original payload after the target refill plus head cross-16B store sequence",
        observed_adjacent == ai_micro_diffsync_seed_word(adjacent_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cbo.inval plus prefetch hint window and head cross-16B store sequence overlays exactly the intended target bytes while preserving the rest of the block and the adjacent block backing image",
        ai_micro_diffsync_block_matches_seed_except_two_words(
            target_base_paddr,
            target_seed,
            1U,
            expected_word1,
            2U,
            expected_word2) &&
        ai_micro_diffsync_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_vector_third_element_page_fault_preserves_old_fault_block_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x300U;
    const uint8_t low_initial[8] = {
        0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U
    };
    const uint8_t src_pattern[16] = {
        0xd0U, 0xd1U, 0xd2U, 0xd3U, 0xd4U, 0xd5U, 0xd6U, 0xd7U,
        0xd8U, 0xd9U, 0xdaU, 0xdbU, 0xdcU, 0xddU, 0xdeU, 0xdfU
    };
    const uint64_t high_seed = 0x5a4b3c2d1e0f9081ULL;
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_diffsync_enable_vector_state();
    ai_micro_diffsync_write_bytes(low_phys, low_initial, sizeof(low_initial));
    ai_micro_diffsync_seed_block(high_phys, high_seed);
    ai_micro_diffsync_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);
    ai_micro_diffsync_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_diffsync_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_diffsync_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(fault_elem_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the eventual fault block is readable before the cbo.invalid + prefetch_r + late-fault vector-store difftest-sync probe begins",
        warmed == ai_micro_diffsync_seed_word(high_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(fault_elem_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the eventual fault block can be invalidated before the refill-triggering prefetch and later late-fault vector store",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    AI_ASSERT_NO_EXCEPT_AND("the source vector load stages the full 4x32-bit payload before the refill-triggering prefetch and later late-fault vector store",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(fault_elem_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a same-block software prefetch after cbo.invalid retires silently before the later third-element page-fault vector store reaches the store queue",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the later vector store still reports a third-element page fault with vstart=2 after the refill-triggering prefetch on the same fault block",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_elem_vaddr &&
        observed_vstart == 2ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the refill-triggering prefetch plus late-fault vector store, only the older two elements commit on the low page while the fault block stays architecturally old",
        ai_micro_diffsync_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_diffsync_block_matches_seed(high_phys, high_seed)
    );
    TEST_ASSERT("the staged source vector payload remains intact after the refill-triggering prefetch plus late-fault vector-store sequence",
        ai_micro_diffsync_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_same_block_cbo_zero_zeroes_refilled_block_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0x180U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0x180U;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uint64_t target_seed = 0x31425364758697a8ULL;
    const uint64_t adjacent_seed = 0xa897867564534231ULL;
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;

    goto_priv(PRIV_M);
    ai_micro_diffsync_seed_block(target_base_paddr, target_seed);
    ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr = ai_micro_diffsync_prepare_same_paddr_alias(
        alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0x180U;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base view of the target block is readable before the same-paddr-alias refill plus same-block cbo.zero difftest-sync probe starts",
        warmed == ai_micro_diffsync_seed_word(target_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the warmed base view of the target block can be invalidated before the later alias-triggered refill plus same-block cbo.zero probe",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_alias_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the same-block software prefetch through the same-paddr alias retires silently before the later base-view cbo.zero reaches the store queue",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES - 1U);
    AI_ASSERT_NO_EXCEPT_AND("the base-view same-block cbo.zero also retires without exception immediately after the alias-triggered refill request",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the alias-triggered refill and base-view same-block cbo.zero retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_head = ld(target_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the alias-triggered refill plus base-view same-block cbo.zero, the alias view of the target block head reads back as zero",
        observed_alias_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_tail = ld(target_base_vaddr +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES - sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the alias-triggered refill plus base-view same-block cbo.zero, the base view of the target block tail also reads back as zero",
        observed_base_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block still reads its original payload from the alias view after the target alias-refill plus base-view cbo.zero sequence",
        observed_alias_adjacent == ai_micro_diffsync_seed_word(adjacent_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias-triggered refill plus base-view same-block cbo.zero sequence zeroes exactly the shared target cache block while preserving the adjacent block backing image",
        ai_micro_diffsync_block_all_zero(target_base_paddr) &&
        ai_micro_diffsync_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_head_cross16b_store_overlays_refilled_block_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH2;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0x1c0U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0x1c0U;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t misaligned_store_vaddr = target_base_vaddr + 15U;
    const uint64_t target_seed = 0x7061524334251607ULL;
    const uint64_t adjacent_seed = 0xefdecdbaab998877ULL;
    const uint64_t store_value = 0xc7b6a59483726150ULL;
    const uint64_t initial_word1 =
        ai_micro_diffsync_seed_word(target_seed, 1U);
    const uint64_t initial_word2 =
        ai_micro_diffsync_seed_word(target_seed, 2U);
    const uint64_t expected_word1 =
        (initial_word1 & 0x00ffffffffffffffULL) |
        ((store_value & 0xffULL) << 56);
    const uint64_t expected_word2 =
        ((store_value >> 8) & 0x00ffffffffffffffULL) |
        (initial_word2 & 0xff00000000000000ULL);
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;

    goto_priv(PRIV_M);
    ai_micro_diffsync_seed_block(target_base_paddr, target_seed);
    ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);
    alias_page_base_vaddr = ai_micro_diffsync_prepare_same_paddr_alias(
        alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0x1c0U;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the base view of the cross-16B target word is readable before the same-paddr-alias refill plus head-cross16B-store difftest-sync probe starts",
        warmed == initial_word1
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the warmed base view of the target block can be invalidated before the later alias-view prefetch hint plus head cross-16B store overlay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_alias_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the same-block software prefetch through the same-paddr alias retires silently before the later base-view head cross-16B store reaches the store queue",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(misaligned_store_vaddr, store_value);
    AI_ASSERT_NO_EXCEPT_AND("the base-view head cross-16B translated store itself retires without exception immediately after the alias-view prefetch hint request",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the alias-view prefetch hint and base-view head cross-16B store retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_word1 = ld(target_alias_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the alias view of the low overlapped word reflects the final head cross-16B overlay byte after the alias-view prefetch hint window",
        observed_alias_word1 == expected_word1
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_word2 = ld(target_base_vaddr + 16U);
    AI_ASSERT_NO_EXCEPT_AND("the base view of the high overlapped word reflects the final head cross-16B overlay bytes after the alias-view prefetch hint window",
        observed_base_word2 == expected_word2
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block still reads its original payload from the alias view after the target alias-refill plus base-view head cross-16B store sequence",
        observed_alias_adjacent == ai_micro_diffsync_seed_word(adjacent_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias-view prefetch hint plus base-view head cross-16B store sequence overlays exactly the intended target bytes while preserving the rest of the shared block and the adjacent block backing image",
        ai_micro_diffsync_block_matches_seed_except_two_words(
            target_base_paddr,
            target_seed,
            1U,
            expected_word1,
            2U,
            expected_word2) &&
        ai_micro_diffsync_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_vector_third_element_page_fault_preserves_old_fault_block_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page alias_page = SWITCH1;
    const enum test_page src_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x300U;
    const uint8_t low_initial[8] = {
        0x51U, 0x52U, 0x53U, 0x54U, 0x55U, 0x56U, 0x57U, 0x58U
    };
    const uint8_t src_pattern[16] = {
        0xb0U, 0xb1U, 0xb2U, 0xb3U, 0xb4U, 0xb5U, 0xb6U, 0xb7U,
        0xb8U, 0xb9U, 0xbaU, 0xbbU, 0xbcU, 0xbdU, 0xbeU, 0xbfU
    };
    const uint64_t high_seed = 0x4a3b2c1d0e9f8071ULL;
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t fault_elem_alias_vaddr = 0;
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_diffsync_enable_vector_state();
    ai_micro_diffsync_write_bytes(low_phys, low_initial, sizeof(low_initial));
    ai_micro_diffsync_seed_block(high_phys, high_seed);
    ai_micro_diffsync_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);
    ai_micro_diffsync_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_diffsync_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_diffsync_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    alias_page_base_vaddr = ai_micro_diffsync_prepare_same_paddr_alias(
        alias_page, high_page);
    fault_elem_alias_vaddr = alias_page_base_vaddr;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(fault_elem_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base view of the eventual fault block is readable before the same-paddr-alias refill plus late-fault vector-store difftest-sync probe begins",
        warmed == ai_micro_diffsync_seed_word(high_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(fault_elem_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base view of the eventual fault block can be invalidated before the alias-triggered refill and later late-fault vector store",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    AI_ASSERT_NO_EXCEPT_AND("the source vector load stages the full 4x32-bit payload before the alias-triggered refill and later late-fault vector store",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(fault_elem_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same-block software prefetch through the same-paddr alias retires silently before the later third-element page-fault vector store reaches the store queue",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the later vector store still reports a third-element page fault with vstart=2 after the alias-triggered refill on the same physical fault block",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_elem_vaddr &&
        observed_vstart == 2ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_fault = ld(fault_elem_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the alias-triggered refill plus late-fault vector store, the base view of the fault block head still reads the old image",
        observed_base_fault == ai_micro_diffsync_seed_word(high_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_fault = ld(fault_elem_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the alias-triggered refill plus late-fault vector store, the alias view of the same physical fault block head also reads the old image",
        observed_alias_fault == ai_micro_diffsync_seed_word(high_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias-triggered refill plus late-fault vector store, only the older two elements commit on the low page while the shared fault block stays architecturally old",
        ai_micro_diffsync_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_diffsync_block_matches_seed(high_phys, high_seed)
    );
    TEST_ASSERT("the staged source vector payload remains intact after the alias-triggered refill plus late-fault vector-store sequence",
        ai_micro_diffsync_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_same_paddr_alias_same_block_cbo_zero_zeroes_refilled_block_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0x240U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0x240U;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uint64_t target_seed = 0x2233445566778899ULL;
    const uint64_t adjacent_seed = 0x9988776655443322ULL;
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;

    goto_priv(PRIV_M);
    ai_micro_diffsync_seed_block(target_base_paddr, target_seed);
    ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr = ai_micro_diffsync_prepare_same_paddr_alias(
        alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0x240U;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias view of the target block is readable before the base-triggered refill plus same-paddr-alias same-block cbo.zero difftest-sync probe starts",
        warmed == ai_micro_diffsync_seed_word(target_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_alias_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the warmed alias view of the target block can be invalidated before the later base-triggered refill plus alias-view same-block cbo.zero probe",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the same-block software prefetch through the base view retires silently before the later alias-view cbo.zero reaches the store queue",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES - 1U);
    AI_ASSERT_NO_EXCEPT_AND("the alias-view same-block cbo.zero also retires without exception immediately after the base-triggered refill request",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the base-triggered refill and alias-view same-block cbo.zero retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the base-triggered refill plus alias-view same-block cbo.zero, the base view of the target block head reads back as zero",
        observed_base_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_tail = ld(target_alias_vaddr +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES - sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the base-triggered refill plus alias-view same-block cbo.zero, the alias view of the target block tail also reads back as zero",
        observed_alias_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_adjacent = ld(target_base_vaddr +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block still reads its original payload from the base view after the target base-refill plus alias-view cbo.zero sequence",
        observed_base_adjacent == ai_micro_diffsync_seed_word(adjacent_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the base-triggered refill plus alias-view same-block cbo.zero sequence zeroes exactly the shared target cache block while preserving the adjacent block backing image",
        ai_micro_diffsync_block_all_zero(target_base_paddr) &&
        ai_micro_diffsync_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_same_paddr_alias_head_cross16b_store_overlays_refilled_block_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH2;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0x280U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0x280U;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uint64_t target_seed = 0x2132435465768798ULL;
    const uint64_t adjacent_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t store_value = 0xe6d5c4b3a291807fULL;
    const uint64_t initial_word1 =
        ai_micro_diffsync_seed_word(target_seed, 1U);
    const uint64_t initial_word2 =
        ai_micro_diffsync_seed_word(target_seed, 2U);
    const uint64_t expected_word1 =
        (initial_word1 & 0x00ffffffffffffffULL) |
        ((store_value & 0xffULL) << 56);
    const uint64_t expected_word2 =
        ((store_value >> 8) & 0x00ffffffffffffffULL) |
        (initial_word2 & 0xff00000000000000ULL);
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t misaligned_store_vaddr = 0;

    goto_priv(PRIV_M);
    ai_micro_diffsync_seed_block(target_base_paddr, target_seed);
    ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);
    alias_page_base_vaddr = ai_micro_diffsync_prepare_same_paddr_alias(
        alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0x280U;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    misaligned_store_vaddr = target_alias_vaddr + 15U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_alias_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the alias view of the cross-16B target word is readable before the base-triggered refill plus same-paddr-alias head-cross16B-store difftest-sync probe starts",
        warmed == initial_word1
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_alias_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the warmed alias view of the target block can be invalidated before the later base-view prefetch hint plus alias-view head cross-16B store overlay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the same-block software prefetch through the base view retires silently before the later alias-view head cross-16B store reaches the store queue",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(misaligned_store_vaddr, store_value);
    AI_ASSERT_NO_EXCEPT_AND("the alias-view head cross-16B translated store itself retires without exception immediately after the base-view prefetch hint request",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the base-view prefetch hint and alias-view head cross-16B store retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_word1 = ld(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the base view of the low overlapped word reflects the final head cross-16B overlay byte after the base-view prefetch hint window",
        observed_base_word1 == expected_word1
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_word2 = ld(target_alias_vaddr + 16U);
    AI_ASSERT_NO_EXCEPT_AND("the alias view of the high overlapped word reflects the final head cross-16B overlay bytes after the base-view prefetch hint window",
        observed_alias_word2 == expected_word2
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_adjacent = ld(target_base_vaddr +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block still reads its original payload from the base view after the target base-refill plus alias-view head cross-16B store sequence",
        observed_base_adjacent == ai_micro_diffsync_seed_word(adjacent_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the base-view prefetch hint plus alias-view head cross-16B store sequence overlays exactly the intended target bytes while preserving the rest of the shared block and the adjacent block backing image",
        ai_micro_diffsync_block_matches_seed_except_two_words(
            target_base_paddr,
            target_seed,
            1U,
            expected_word1,
            2U,
            expected_word2) &&
        ai_micro_diffsync_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_same_paddr_alias_vector_third_element_page_fault_preserves_old_fault_block_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page alias_low_page = SWITCH1;
    const enum test_page alias_high_page = SWITCH2;
    const enum test_page src_page = SCRATCHPAD;
    const uintptr_t fault_elem_base_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x340U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x340U;
    const uint8_t low_initial[8] = {
        0x61U, 0x62U, 0x63U, 0x64U, 0x65U, 0x66U, 0x67U, 0x68U
    };
    const uint8_t src_pattern[16] = {
        0xa0U, 0xa1U, 0xa2U, 0xa3U, 0xa4U, 0xa5U, 0xa6U, 0xa7U,
        0xa8U, 0xa9U, 0xaaU, 0xabU, 0xacU, 0xadU, 0xaeU, 0xafU
    };
    const uint64_t high_seed = 0x3a2b1c0d9e8f7061ULL;
    uintptr_t alias_low_page_base_vaddr = 0;
    uintptr_t alias_high_page_base_vaddr = 0;
    uintptr_t vector_alias_base_vaddr = 0;
    uintptr_t fault_elem_alias_vaddr = 0;
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_diffsync_enable_vector_state();
    ai_micro_diffsync_write_bytes(low_phys, low_initial, sizeof(low_initial));
    ai_micro_diffsync_seed_block(high_phys, high_seed);
    ai_micro_diffsync_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);
    ai_micro_diffsync_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_diffsync_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_diffsync_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    alias_low_page_base_vaddr = ai_micro_diffsync_prepare_same_paddr_alias_with_flags(
        alias_low_page, low_page, PTE_V | PTE_AD | PTE_RWX);
    alias_high_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias_with_flags(
            alias_high_page, high_page, PTE_V | PTE_A | PTE_RWX);
    vector_alias_base_vaddr = alias_low_page_base_vaddr + PAGE_SIZE - 8U;
    fault_elem_alias_vaddr = alias_high_page_base_vaddr;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(fault_elem_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias view of the eventual fault block is readable before the base-triggered refill plus same-paddr-alias late-fault vector-store difftest-sync probe begins",
        warmed == ai_micro_diffsync_seed_word(high_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(fault_elem_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias view of the eventual fault block can be invalidated before the base-triggered refill and later alias-view late-fault vector store",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    AI_ASSERT_NO_EXCEPT_AND("the source vector load stages the full 4x32-bit payload before the base-triggered refill and later alias-view late-fault vector store",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(fault_elem_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same-block software prefetch through the base view retires silently before the later third-element page-fault vector store through the alias view reaches the store queue",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)vector_alias_base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the later vector store through the alias view still reports a third-element page fault with vstart=2 after the base-triggered refill on the same physical fault block",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_elem_alias_vaddr &&
        observed_vstart == 2ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_fault = ld(fault_elem_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the base-triggered refill plus alias-view late-fault vector store, the base view of the fault block head still reads the old image",
        observed_base_fault == ai_micro_diffsync_seed_word(high_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_fault = ld(fault_elem_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the base-triggered refill plus alias-view late-fault vector store, the alias view of the same physical fault block head also reads the old image",
        observed_alias_fault == ai_micro_diffsync_seed_word(high_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the base-triggered refill plus alias-view late-fault vector store, only the older two elements commit on the low page while the shared fault block stays architecturally old",
        ai_micro_diffsync_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_diffsync_block_matches_seed(high_phys, high_seed)
    );
    TEST_ASSERT("the staged source vector payload remains intact after the base-triggered refill plus alias-view late-fault vector-store sequence",
        ai_micro_diffsync_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_mshr_pressure_then_same_block_cbo_zero_zeroes_refilled_block_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x180U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x180U;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uint64_t target_seed = 0x2233445566778899ULL;
    const uint64_t adjacent_seed = 0x9988776655443322ULL;
    const uint64_t pressure_seed = 0x4400550066007700ULL;

    goto_priv(PRIV_M);
    ai_micro_diffsync_seed_block(target_base_paddr, target_seed);
    ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_diffsync_seed_pressure_pages(pressure_seed,
        target_page, TEST_PAGE_MAX, TEST_PAGE_MAX);
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target block is readable before the MSHR-pressure cbo.invalid + prefetch_r + cbo.zero difftest-sync probe starts",
        warmed == ai_micro_diffsync_seed_word(target_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the warmed target block can be invalidated before the later MSHR-pressure refill plus same-block cbo.zero probe",
        true
    );

    for (uintptr_t i = 0; i < sizeof(ai_micro_diffsync_pressure_pages) /
        sizeof(ai_micro_diffsync_pressure_pages[0]); ++i) {
        enum test_page page = ai_micro_diffsync_pressure_pages[i];
        uintptr_t vaddr = hs_page_base(page) + AI_MICRO_DIFFSYNC_PRESSURE_OFFSET;

        if (ai_micro_diffsync_page_skipped(page, target_page,
            TEST_PAGE_MAX, TEST_PAGE_MAX)) {
            continue;
        }

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("each HS pressure-building probe page is readable before the MSHR-pressure same-block cbo.zero refill-sync sequence starts",
            probe == ai_micro_diffsync_pressure_word(pressure_seed, i)
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("each warmed HS pressure-building probe line can be invalidated before the MSHR-pressure same-block cbo.zero refill-sync sequence",
            true
        );
    }

    for (uintptr_t i = 0; i < sizeof(ai_micro_diffsync_pressure_pages) /
        sizeof(ai_micro_diffsync_pressure_pages[0]); ++i) {
        enum test_page page = ai_micro_diffsync_pressure_pages[i];
        uintptr_t vaddr = hs_page_base(page) + AI_MICRO_DIFFSYNC_PRESSURE_OFFSET;

        if (ai_micro_diffsync_page_skipped(page, target_page,
            TEST_PAGE_MAX, TEST_PAGE_MAX)) {
            continue;
        }

        TEST_SETUP_EXCEPT();
        prefetch_r(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight before the target refill plus same-block cbo.zero probe",
            true
        );
    }

    TEST_SETUP_EXCEPT();
    prefetch_r(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the target same-block software prefetch remains silent even after miss-machinery pressure is already building before cbo.zero reaches the store queue",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES - 1U);
    AI_ASSERT_NO_EXCEPT_AND("the same-block cbo.zero also retires without exception under the target refill MSHR-pressure window",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the MSHR-pressure target refill and same-block cbo.zero retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the MSHR-pressure refill-triggering prefetch and same-block cbo.zero, the target block head reads back as zero",
        observed_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_tail = ld(target_base_vaddr +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES - sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the MSHR-pressure refill-triggering prefetch and same-block cbo.zero, the target block tail also reads back as zero",
        observed_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block still reads its original payload after the target MSHR-pressure refill plus same-block cbo.zero sequence",
        observed_adjacent == ai_micro_diffsync_seed_word(adjacent_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the MSHR-pressure refill-triggering prefetch plus same-block cbo.zero sequence zeroes exactly the target block while preserving both the adjacent block and all pressure-building backing words",
        ai_micro_diffsync_block_all_zero(target_base_paddr) &&
        ai_micro_diffsync_block_matches_seed(adjacent_base_paddr, adjacent_seed) &&
        ai_micro_diffsync_pressure_pages_match_seed(pressure_seed,
            target_page, TEST_PAGE_MAX, TEST_PAGE_MAX)
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_mshr_pressure_then_head_cross16b_store_overlays_refilled_block_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x1c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x1c0U;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t misaligned_store_vaddr = target_base_vaddr + 15U;
    const uint64_t target_seed = 0x31425364758697a8ULL;
    const uint64_t adjacent_seed = 0xa897867564534231ULL;
    const uint64_t pressure_seed = 0x5500660077008800ULL;
    const uint64_t store_value = 0xe1d2c3b4a5968778ULL;
    const uint64_t initial_word1 =
        ai_micro_diffsync_seed_word(target_seed, 1U);
    const uint64_t initial_word2 =
        ai_micro_diffsync_seed_word(target_seed, 2U);
    const uint64_t expected_word1 =
        (initial_word1 & 0x00ffffffffffffffULL) |
        ((store_value & 0xffULL) << 56);
    const uint64_t expected_word2 =
        ((store_value >> 8) & 0x00ffffffffffffffULL) |
        (initial_word2 & 0xff00000000000000ULL);

    goto_priv(PRIV_M);
    ai_micro_diffsync_seed_block(target_base_paddr, target_seed);
    ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_diffsync_seed_pressure_pages(pressure_seed,
        target_page, TEST_PAGE_MAX, TEST_PAGE_MAX);
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the cross-16B target word is readable before the MSHR-pressure cbo.invalid + prefetch_r + head-cross16B-store difftest-sync probe starts",
        warmed == initial_word1
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the warmed target block can be invalidated before the later MSHR-pressure prefetch-hint window plus head cross-16B store overlay probe",
        true
    );

    for (uintptr_t i = 0; i < sizeof(ai_micro_diffsync_pressure_pages) /
        sizeof(ai_micro_diffsync_pressure_pages[0]); ++i) {
        enum test_page page = ai_micro_diffsync_pressure_pages[i];
        uintptr_t vaddr = hs_page_base(page) + AI_MICRO_DIFFSYNC_PRESSURE_OFFSET;

        if (ai_micro_diffsync_page_skipped(page, target_page,
            TEST_PAGE_MAX, TEST_PAGE_MAX)) {
            continue;
        }

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("each HS pressure-building probe page is readable before the MSHR-pressure head cross-16B refill-sync sequence starts",
            probe == ai_micro_diffsync_pressure_word(pressure_seed, i)
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("each warmed HS pressure-building probe line can be invalidated before the MSHR-pressure head cross-16B refill-sync sequence",
            true
        );
    }

    for (uintptr_t i = 0; i < sizeof(ai_micro_diffsync_pressure_pages) /
        sizeof(ai_micro_diffsync_pressure_pages[0]); ++i) {
        enum test_page page = ai_micro_diffsync_pressure_pages[i];
        uintptr_t vaddr = hs_page_base(page) + AI_MICRO_DIFFSYNC_PRESSURE_OFFSET;

        if (ai_micro_diffsync_page_skipped(page, target_page,
            TEST_PAGE_MAX, TEST_PAGE_MAX)) {
            continue;
        }

        TEST_SETUP_EXCEPT();
        prefetch_r(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight before the target refill plus head cross-16B store probe",
            true
        );
    }

    TEST_SETUP_EXCEPT();
    prefetch_r(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the target same-block software prefetch hint remains silent even after miss-machinery pressure is already building before the head cross-16B store reaches the store queue",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(misaligned_store_vaddr, store_value);
    AI_ASSERT_NO_EXCEPT_AND("the head cross-16B translated store itself retires without exception under the target MSHR-pressure prefetch-hint window",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the MSHR-pressure target prefetch hint and head cross-16B store retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word1 = ld(target_base_vaddr + 8U);
    AI_ASSERT_NO_EXCEPT_AND("the low overlapped word still reflects the final head cross-16B overlay byte after the MSHR-pressure prefetch-hint window",
        observed_word1 == expected_word1
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word2 = ld(target_base_vaddr + 16U);
    AI_ASSERT_NO_EXCEPT_AND("the high overlapped word still reflects the final head cross-16B overlay bytes after the MSHR-pressure prefetch-hint window",
        observed_word2 == expected_word2
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block still reads its original payload after the target MSHR-pressure refill plus head cross-16B store sequence",
        observed_adjacent == ai_micro_diffsync_seed_word(adjacent_seed, 0U)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the MSHR-pressure prefetch-hint window plus head cross-16B store sequence overlays exactly the intended target bytes while preserving the rest of the block, the adjacent block, and all pressure-building backing words",
        ai_micro_diffsync_block_matches_seed_except_two_words(
            target_base_paddr,
            target_seed,
            1U,
            expected_word1,
            2U,
            expected_word2) &&
        ai_micro_diffsync_block_matches_seed(adjacent_base_paddr, adjacent_seed) &&
        ai_micro_diffsync_pressure_pages_match_seed(pressure_seed,
            target_page, TEST_PAGE_MAX, TEST_PAGE_MAX)
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_mshr_pressure_then_vector_third_element_page_fault_preserves_old_fault_block_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x300U;
    const uint8_t low_initial[8] = {
        0x41U, 0x42U, 0x43U, 0x44U, 0x45U, 0x46U, 0x47U, 0x48U
    };
    const uint8_t src_pattern[16] = {
        0xe0U, 0xe1U, 0xe2U, 0xe3U, 0xe4U, 0xe5U, 0xe6U, 0xe7U,
        0xe8U, 0xe9U, 0xeaU, 0xebU, 0xecU, 0xedU, 0xeeU, 0xefU
    };
    const uint64_t high_seed = 0x6b5c4d3e2f109182ULL;
    const uint64_t pressure_seed = 0x6600770088009900ULL;
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_diffsync_enable_vector_state();
    ai_micro_diffsync_write_bytes(low_phys, low_initial, sizeof(low_initial));
    ai_micro_diffsync_seed_block(high_phys, high_seed);
    ai_micro_diffsync_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    ai_micro_diffsync_seed_pressure_pages(pressure_seed,
        low_page, high_page, src_page);
    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);
    ai_micro_diffsync_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_diffsync_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_diffsync_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(fault_elem_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the eventual fault block is readable before the MSHR-pressure cbo.invalid + prefetch_r + late-fault vector-store difftest-sync probe begins",
        warmed == ai_micro_diffsync_seed_word(high_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(fault_elem_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the eventual fault block can be invalidated before the MSHR-pressure refill-triggering prefetch and later late-fault vector store",
        true
    );

    for (uintptr_t i = 0; i < sizeof(ai_micro_diffsync_pressure_pages) /
        sizeof(ai_micro_diffsync_pressure_pages[0]); ++i) {
        enum test_page page = ai_micro_diffsync_pressure_pages[i];
        uintptr_t vaddr = hs_page_base(page) + AI_MICRO_DIFFSYNC_PRESSURE_OFFSET;

        if (ai_micro_diffsync_page_skipped(page, low_page, high_page, src_page)) {
            continue;
        }

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("each HS pressure-building probe page is readable before the MSHR-pressure late-fault vector-store refill-sync sequence starts",
            probe == ai_micro_diffsync_pressure_word(pressure_seed, i)
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("each warmed HS pressure-building probe line can be invalidated before the MSHR-pressure late-fault vector-store refill-sync sequence",
            true
        );
    }

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    AI_ASSERT_NO_EXCEPT_AND("the source vector load stages the full 4x32-bit payload before the MSHR-pressure refill-triggering prefetch and later late-fault vector store",
        true
    );

    for (uintptr_t i = 0; i < sizeof(ai_micro_diffsync_pressure_pages) /
        sizeof(ai_micro_diffsync_pressure_pages[0]); ++i) {
        enum test_page page = ai_micro_diffsync_pressure_pages[i];
        uintptr_t vaddr = hs_page_base(page) + AI_MICRO_DIFFSYNC_PRESSURE_OFFSET;

        if (ai_micro_diffsync_page_skipped(page, low_page, high_page, src_page)) {
            continue;
        }

        TEST_SETUP_EXCEPT();
        prefetch_r(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight before the target late-fault vector-store refill-sync probe",
            true
        );
    }

    TEST_SETUP_EXCEPT();
    prefetch_r(fault_elem_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target same-block software prefetch remains silent even after miss-machinery pressure is already building before the later third-element page-fault vector store reaches the store queue",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the later vector store still reports a third-element page fault with vstart=2 under the target refill MSHR-pressure window on the same fault block",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_elem_vaddr &&
        observed_vstart == 2ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the MSHR-pressure refill-triggering prefetch plus late-fault vector store, only the older two elements commit on the low page while the fault block stays architecturally old",
        ai_micro_diffsync_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_diffsync_block_matches_seed(high_phys, high_seed)
    );
    TEST_ASSERT("the staged source vector payload and all pressure-building backing words remain intact after the MSHR-pressure refill-triggering prefetch plus late-fault vector-store sequence",
        ai_micro_diffsync_bytes_match(src_phys, src_pattern, sizeof(src_pattern)) &&
        ai_micro_diffsync_pressure_pages_match_seed(pressure_seed,
            low_page, high_page, src_page)
    );
    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_lrsc_no_reservation_loss_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x2c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x2c0U;
    const uintptr_t lrsc_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr = target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t tail_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr = target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2c0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x21436587a9cbed0fULL,
        0x1029384756abcdefULL,
        0x89abcdef76543210ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t sc_payloads[] = {
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + LR/SC refill-sync window starts, target block, adjacent block, and marker are readable with seeded values",
            ld(lrsc_vaddr) == block_seeds[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_lrsc_window(inval_vaddr,
            prefetch_vaddr, lrsc_vaddr, neighbor_vaddr, marker_vaddr,
            sc_payloads[round], neighbor_updates[round],
            marker_updates[round], &lr_observed, &sc_status,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval followed by same-block prefetch_r must release refill ownership before LR/SC so LR reads the current seed, SC succeeds, and marker progresses",
            lr_observed == block_seeds[round] &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_vaddr) == sc_payloads[round] &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cbo.inval + prefetch_r + LR/SC refill-sync window preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + LR/SC refill-sync window contains the successful SC payload, neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                sc_payloads[round],
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_lrsc_no_stale_zero_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x340U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x340U;
    const uintptr_t lrsc_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t tail_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr = target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x340U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x340U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x33445566778899aaULL,
        0x456789abcdef0123ULL,
        0xa1b2c3d4e5f60718ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x8899aabbccddeeffULL,
        0x1020304050607080ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t sc_payloads[] = {
        0xcafed00d5a5aa5a5ULL,
        0x0badf00d13579bdfULL,
        0x2468ace0fdb97531ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0102030405060708ULL,
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + cbo.zero + LR/SC window starts, target block, adjacent block, and marker are readable with seeded values",
            ld(lrsc_vaddr) == block_seeds[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_zero_lrsc_window(inval_vaddr,
            prefetch_vaddr, zero_vaddr, lrsc_vaddr, marker_vaddr,
            sc_payloads[round], marker_updates[round], &lr_observed,
            &sc_status, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus prefetch_r must not leave stale refill ownership when cbo.zero hands off to LR/SC, so LR sees zero, SC succeeds, and marker progresses",
            lr_observed == 0ULL &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_vaddr) == sc_payloads[round] &&
            ld(tail_vaddr) == 0ULL &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the refill-to-zero-to-LR/SC owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + cbo.zero + LR/SC window contains one successful SC payload over an otherwise zeroed block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                0U, sc_payloads[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_lrsc_cbo_zero_no_stale_sc_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x3c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x3c0U;
    const uintptr_t lrsc_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t zero_vaddr = target_base_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t tail_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr = target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3c0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1223344556677889ULL,
        0x89abcdef10293847ULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t sc_payloads[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + LR/SC + cbo.zero window starts, target block, adjacent block, and marker are readable with seeded values",
            ld(lrsc_vaddr) == block_seeds[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_lrsc_zero_window(inval_vaddr,
            prefetch_vaddr, lrsc_vaddr, zero_vaddr, marker_vaddr,
            sc_payloads[round], marker_updates[round], &lr_observed,
            &sc_status, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus prefetch_r must not let stale refill ownership disturb LR/SC before the following cbo.zero clears the full block",
            lr_observed == block_seeds[round] &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_vaddr) == 0ULL &&
            ld(tail_vaddr) == 0ULL &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the refill-to-LR/SC-to-zero owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + LR/SC + cbo.zero window contains a fully zeroed target block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero(target_base_paddr) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}
