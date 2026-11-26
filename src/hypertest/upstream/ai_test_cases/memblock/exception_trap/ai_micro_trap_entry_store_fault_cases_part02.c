#include "ai_micro_trap_entry_store_fault_common.h"

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_doubleword_store_only_handler_8b_image_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x600U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x600U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[16] = {
        0x61U, 0x62U, 0x63U, 0x64U, 0x65U, 0x66U, 0x67U, 0x68U,
        0x69U, 0x6aU, 0x6bU, 0x6cU, 0x6dU, 0x6eU, 0x6fU, 0x70U
    };
    uint8_t expected_final[16] = {0};
    const uint64_t committed_value = 0x0123456789abcdefULL;
    const uint64_t handler_value = 0xfedcba9876543210ULL;
    const uint64_t pressure_first_seed = 0x2233445566778899ULL;
    const uint64_t pressure_second_seed = 0x9988776655443322ULL;
    const uint64_t pressure_third_seed = 0x31415926deadbeefULL;
    const uint64_t pressure_fourth_seed = 0xefbeadde26594131ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    memcpy(expected_final, target_initial, sizeof(expected_final));
    memcpy(expected_final, &handler_value, sizeof(handler_value));

    goto_priv(PRIV_M);
    ai_micro_trap_entry_write_bytes(target_phys, target_initial,
        sizeof(target_initial));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_BKP));
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(target_vaddr);
    ai_micro_trap_entry_probe_value = handler_value;

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_doubleword_store_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later breakpoint plus trap-entry same-address doubleword-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later breakpoint plus trap-entry same-address doubleword-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, committed_value);
    TEST_ASSERT("the older same-address scalar store commits the latest line image before the later breakpoint trap-entry same-address doubleword-store probe starts",
        excpt.triggered == false
    );

    ai_micro_trap_entry_ebreak32();

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap before the same-address doubleword store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-address doubleword store is the only architecturally visible post-breakpoint writer across the target 8-byte window",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_bytes_match(target_phys, expected_final,
            sizeof(expected_final))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-address doubleword store returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_vector_store_only_handler_16b_image_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x680U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x680U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0x71U, 0x72U, 0x73U, 0x74U, 0x75U, 0x76U, 0x77U, 0x78U,
        0x79U, 0x7aU, 0x7bU, 0x7cU, 0x7dU, 0x7eU, 0x7fU, 0x80U
    };
    const uint8_t committed_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0x91U, 0x92U, 0x93U, 0x94U, 0x95U, 0x96U, 0x97U, 0x98U,
        0x99U, 0x9aU, 0x9bU, 0x9cU, 0x9dU, 0x9eU, 0x9fU, 0xa0U
    };
    const uint8_t handler_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0x31U, 0x42U, 0x53U, 0x64U, 0x75U, 0x86U, 0x97U, 0xa8U,
        0xb9U, 0xcaU, 0xdbU, 0xecU, 0xfdU, 0x1eU, 0x2fU, 0x40U
    };
    const uint64_t pressure_first_seed = 0x1020304050607080ULL;
    const uint64_t pressure_second_seed = 0x8070605040302010ULL;
    const uint64_t pressure_third_seed = 0x55aa33cc77ee1199ULL;
    const uint64_t pressure_fourth_seed = 0x9911ee77cc33aa55ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uint64_t committed_lo = 0;
    uint64_t committed_hi = 0;
    bool fill_visible = false;

    memcpy(&committed_lo, committed_image, sizeof(committed_lo));
    memcpy(&committed_hi, committed_image + sizeof(committed_lo),
        sizeof(committed_hi));

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_prepare_vector_store_source(handler_image);
    ai_micro_trap_entry_write_bytes(target_phys, target_initial,
        AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_BKP));
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(target_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_vector_store_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later breakpoint plus trap-entry same-address vector-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later breakpoint plus trap-entry same-address vector-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, committed_lo);
    sd(target_vaddr + sizeof(committed_lo), committed_hi);
    TEST_ASSERT("the older same-address pair of scalar stores commits the latest 16-byte target image before the later breakpoint trap-entry same-address vector-store probe starts",
        excpt.triggered == false
    );

    ai_micro_trap_entry_ebreak32();

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap before the same-address vector store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-address vector store is the only architecturally visible post-breakpoint writer across the full 16-byte target window",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_bytes_match(target_phys, handler_image,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-address vector store returns",
        fill_visible
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_clean_preserves_latest_committed_block_image_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t block_vaddr = hs_page_base(target_page) + 0x740U;
    const uintptr_t probe_vaddr = block_vaddr + 8U;
    const uintptr_t block_phys = phys_page_base(target_page) + 0x740U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t block_seed = 0x4d5e6f708192a3b4ULL;
    const uint64_t committed_word1 = 0x0fedcba987654321ULL;
    const uint64_t committed_word5 = 0x123456789abcdef0ULL;
    const uint64_t pressure_first_seed = 0x123456789abcdef0ULL;
    const uint64_t pressure_second_seed = 0x0fedcba987654321ULL;
    const uint64_t pressure_third_seed = 0x55aa77cc33ee1199ULL;
    const uint64_t pressure_fourth_seed = 0x9911ee33cc77aa55ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uintptr_t old_menvcfg = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_seed_block(block_phys, block_seed);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    old_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_BKP));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBZFE);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_clean_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(block_vaddr);
    TEST_ASSERT("the target cache block is readable before the later breakpoint plus trap-entry same-block cbo.clean probe sequence begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later breakpoint plus trap-entry same-block cbo.clean probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later breakpoint plus trap-entry same-block cbo.clean probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(block_vaddr + 8U, committed_word1);
    sd(block_vaddr + 40U, committed_word5);
    TEST_ASSERT("the older same-block scalar stores commit the latest visible cache-block payload before the later breakpoint trap-entry same-block cbo.clean probe starts",
        excpt.triggered == false
    );

    ai_micro_trap_entry_ebreak32();

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap before the same-block cbo.clean runs",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.clean runs at handler head on the target block instead of deadlocking behind the earlier translated-store pressure burst",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-block cbo.clean preserves the latest committed cache-block image rather than rolling back the older committed updates made before the breakpoint",
        read64(block_phys) == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U) &&
        read64(block_phys + 8U) == committed_word1 &&
        read64(block_phys + 40U) == committed_word5 &&
        read64(block_phys + 56U) ==
            ai_micro_trap_entry_dense_store_pattern(block_seed, 7U)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-block cbo.clean returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_inval_preserves_latest_committed_block_image_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t block_vaddr = hs_page_base(target_page) + 0x7c0U;
    const uintptr_t probe_vaddr = block_vaddr + 8U;
    const uintptr_t block_phys = phys_page_base(target_page) + 0x7c0U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t block_seed = 0x7b6a594837261504ULL;
    const uint64_t committed_word1 = 0x89abcdef01234567ULL;
    const uint64_t committed_word5 = 0x76543210fedcba98ULL;
    const uint64_t pressure_first_seed = 0x0a0b0c0d0e0f1011ULL;
    const uint64_t pressure_second_seed = 0x11100f0e0d0c0b0aULL;
    const uint64_t pressure_third_seed = 0x6677445522331100ULL;
    const uint64_t pressure_fourth_seed = 0x0011332255447766ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uintptr_t old_menvcfg = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_seed_block(block_phys, block_seed);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    old_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_BKP));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBIE01);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_inval_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(block_vaddr);
    TEST_ASSERT("the target cache block is readable before the later breakpoint plus trap-entry same-block cbo.inval probe sequence begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later breakpoint plus trap-entry same-block cbo.inval probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later breakpoint plus trap-entry same-block cbo.inval probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(block_vaddr + 8U, committed_word1);
    sd(block_vaddr + 40U, committed_word5);
    TEST_ASSERT("the older same-block scalar stores commit the latest visible cache-block payload before the later breakpoint trap-entry same-block cbo.inval probe starts",
        excpt.triggered == false
    );

    ai_micro_trap_entry_ebreak32();

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap before the same-block cbo.inval runs",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.inval runs at handler head on the target block instead of deadlocking behind the earlier translated-store pressure burst",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-block cbo.inval preserves the latest committed cache-block image rather than rolling back the older committed updates made before the breakpoint",
        read64(block_phys) == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U) &&
        read64(block_phys + 8U) == committed_word1 &&
        read64(block_phys + 40U) == committed_word5 &&
        read64(block_phys + 56U) ==
            ai_micro_trap_entry_dense_store_pattern(block_seed, 7U)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-block cbo.inval returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_zero_only_handler_zero_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t block_vaddr = hs_page_base(target_page) + 0x840U;
    const uintptr_t probe_vaddr = block_vaddr + 8U;
    const uintptr_t block_phys = phys_page_base(target_page) + 0x840U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t block_seed = 0x6f5e4d3c2b1a0918ULL;
    const uint64_t committed_word1 = 0x123456789abcdef0ULL;
    const uint64_t committed_word5 = 0x0fedcba987654321ULL;
    const uint64_t pressure_first_seed = 0x89abcdef76543210ULL;
    const uint64_t pressure_second_seed = 0x0123456789abcdefULL;
    const uint64_t pressure_third_seed = 0x55aa77cc33ee1199ULL;
    const uint64_t pressure_fourth_seed = 0x9911ee33cc77aa55ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uintptr_t old_menvcfg = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_seed_block(block_phys, block_seed);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    old_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_BKP));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBZE);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(probe_vaddr);
    ai_micro_trap_entry_store_fix_pte_addr =
        (uintptr_t)ai_micro_trap_entry_hs_leaf(target_page);
    ai_micro_trap_entry_store_fix_pte_value =
        (phys_page_base(target_page) >> 2) | (PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_zero_with_pte_fix_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(block_vaddr);
    TEST_ASSERT("the target cache block is readable before the later breakpoint plus trap-entry same-block cbo.zero probe sequence begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later breakpoint plus trap-entry same-block cbo.zero probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later breakpoint plus trap-entry same-block cbo.zero probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(block_vaddr + 8U, committed_word1);
    sd(block_vaddr + 40U, committed_word5);
    TEST_ASSERT("the older same-block scalar stores commit the latest visible cache-block payload before the later breakpoint trap-entry same-block cbo.zero probe starts",
        excpt.triggered == false
    );

    ai_micro_trap_entry_ebreak32();

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap before the same-block cbo.zero runs",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.zero runs at handler head on the target block instead of deadlocking behind the earlier translated-store pressure burst",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("after the breakpoint-triggered cbo.zero probe, the handler also replays the prepared translated-store tail burst so the exception-return window stays under active store pressure",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-block cbo.zero is the only architecturally visible post-breakpoint writer across the target cache block",
        ai_micro_trap_entry_block_is_zero(block_phys)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-block cbo.zero returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_byte_load_sees_old_value_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x2e0U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x2e0U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[24] = {
        0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
        0x18U, 0x19U, 0x1aU, 0x1bU, 0x1cU, 0x1dU, 0x1eU, 0x1fU,
        0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U
    };
    const uint8_t src_pattern[16] = {
        0xc0U, 0xc1U, 0xc2U, 0xc3U, 0xc4U, 0xc5U, 0xc6U, 0xc7U,
        0xc8U, 0xc9U, 0xcaU, 0xcbU, 0xccU, 0xcdU, 0xceU, 0xcfU,
    };
    const uint64_t pressure_first_seed = 0x02468ace13579bdfULL;
    const uint64_t pressure_second_seed = 0xfdb97531eca86420ULL;
    const uint64_t pressure_third_seed = 0x55aa33cc77ee1199ULL;
    const uint64_t pressure_fourth_seed = 0x99ee77cc33aa5511ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(low_phys, target_initial, 8U);
    ai_micro_trap_entry_write_bytes(high_phys, target_initial + 8U, 16U);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF));
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_byte_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-fault byte-load probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-fault byte-load probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-fault byte load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-fault byte load runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-fault byte load runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry byte load fires at handler head on the first faulting byte address",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("after the same-fault byte-load probe, the handler also replays the prepared translated-store tail burst so the exception-return window stays under active store pressure",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-fault byte load still sees the old committed byte of the late-fault element instead of any ghost byte from the vector store",
        ai_micro_trap_entry_probe_value == target_initial[8]
    );
    TEST_ASSERT("the older elements before the late fault do commit, but the faulting third element and its idx-plus-one element remain architecturally old across the trap-entry same-fault byte load",
        ai_micro_trap_entry_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_trap_entry_bytes_match(high_phys, target_initial + 8U, 16U)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-fault byte load returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-fault byte-load sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_doubleword_load_sees_old_idx_and_idx_plus_one_8b_image_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x300U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[24] = {
        0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U, 0x28U,
        0x29U, 0x2aU, 0x2bU, 0x2cU, 0x2dU, 0x2eU, 0x2fU, 0x30U,
        0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U
    };
    const uint8_t src_pattern[16] = {
        0xb1U, 0xb2U, 0xb3U, 0xb4U, 0xb5U, 0xb6U, 0xb7U, 0xb8U,
        0xb9U, 0xbaU, 0xbbU, 0xbcU, 0xbdU, 0xbeU, 0xbfU, 0xc0U,
    };
    const uint64_t pressure_first_seed = 0x13579bdf2468ace0ULL;
    const uint64_t pressure_second_seed = 0x0eca8642fdb97531ULL;
    const uint64_t pressure_third_seed = 0x66778899aabbccddULL;
    const uint64_t pressure_fourth_seed = 0xddccbbaa99887766ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uint64_t expected_fault_window = 0;
    bool fill_visible = false;

    memcpy(&expected_fault_window, target_initial + 8U,
        sizeof(expected_fault_window));

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(low_phys, target_initial, 8U);
    ai_micro_trap_entry_write_bytes(high_phys, target_initial + 8U, 16U);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_doubleword_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-fault doubleword-load probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-fault doubleword-load probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store plus trap-entry same-fault doubleword-load probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-fault doubleword load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-fault doubleword load runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-fault doubleword load runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry doubleword load fires at handler head from the first faulting byte address",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-fault doubleword load captures exactly the old committed leading 8-byte image spanning the late-fault element idx and idx-plus-one window",
        ai_micro_trap_entry_probe_value == expected_fault_window
    );
    TEST_ASSERT("the older elements before the late fault do commit, but the faulting third element and its idx-plus-one element remain architecturally old across the trap-entry same-fault doubleword load",
        ai_micro_trap_entry_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_trap_entry_bytes_match(high_phys, target_initial + 8U, 16U)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-fault doubleword load returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-fault doubleword-load sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_vector_load_sees_old_idx_and_idx_plus_one_image_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x320U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x320U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[24] = {
        0x30U, 0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U,
        0x38U, 0x39U, 0x3aU, 0x3bU, 0x3cU, 0x3dU, 0x3eU, 0x3fU,
        0x40U, 0x41U, 0x42U, 0x43U, 0x44U, 0x45U, 0x46U, 0x47U
    };
    const uint8_t src_pattern[16] = {
        0xe0U, 0xe1U, 0xe2U, 0xe3U, 0xe4U, 0xe5U, 0xe6U, 0xe7U,
        0xe8U, 0xe9U, 0xeaU, 0xebU, 0xecU, 0xedU, 0xeeU, 0xefU,
    };
    const uint64_t pressure_first_seed = 0x1123581321345589ULL;
    const uint64_t pressure_second_seed = 0x9985543123128531ULL;
    const uint64_t pressure_third_seed = 0x44cc88ee11aa55ffULL;
    const uint64_t pressure_fourth_seed = 0xff55aa11ee88cc44ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(low_phys, target_initial, 8U);
    ai_micro_trap_entry_write_bytes(high_phys, target_initial + 8U, 16U);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_vector_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-fault vector-load probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-fault vector-load probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store plus trap-entry same-fault vector-load probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-fault vector load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-fault vector load runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-fault vector load runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry vector load fires at handler head from the first faulting byte address",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-fault vector load captures exactly the old 16-byte image spanning the late-fault element idx and idx-plus-one element",
        ai_micro_trap_entry_probe_bytes_match(target_initial + 8U,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES)
    );
    TEST_ASSERT("the late-fault same-parameter vector-load handler also executes the common post-observation handler pressure burst so this vector fault-followup case keeps pressure high after the earliest snapshot",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the older elements before the late fault do commit, but the faulting third element and its idx-plus-one element remain architecturally old across the trap-entry same-fault vector load",
        ai_micro_trap_entry_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_trap_entry_bytes_match(high_phys, target_initial + 8U, 16U)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-fault vector load returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-fault vector-load sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_flush_preserves_old_image_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t block_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x360U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x360U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t low_initial[8] = {
        0x61U, 0x62U, 0x63U, 0x64U, 0x65U, 0x66U, 0x67U, 0x68U
    };
    const uint8_t src_pattern[16] = {
        0x91U, 0x92U, 0x93U, 0x94U, 0x95U, 0x96U, 0x97U, 0x98U,
        0x99U, 0x9aU, 0x9bU, 0x9cU, 0x9dU, 0x9eU, 0x9fU, 0xa0U
    };
    const uint64_t block_seed = 0x7a6b5c4d3e2f1021ULL;
    const uint64_t pressure_first_seed = 0x89abcdef01234567ULL;
    const uint64_t pressure_second_seed = 0x76543210fedcba98ULL;
    const uint64_t pressure_third_seed = 0x0f0e0d0c0b0a0908ULL;
    const uint64_t pressure_fourth_seed = 0x8081828384858687ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uintptr_t old_menvcfg = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(low_phys, low_initial, sizeof(low_initial));
    ai_micro_trap_entry_seed_block(block_phys, block_seed);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    old_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBZFE);
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_flush_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(fault_elem_vaddr);
    TEST_ASSERT("the high-page fault block is readable before the later late-fault vector-store plus trap-entry same-block cbo.flush probe begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-block cbo.flush probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-block cbo.flush probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store plus trap-entry same-block cbo.flush probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-block cbo.flush runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-block cbo.flush runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-block cbo.flush runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.flush runs at handler head on the block that contains the first faulting byte address instead of deadlocking behind the late-fault vector-store path",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the older elements before the late fault do commit, but the faulting third element, its idx-plus-one element, and the rest of the fault block remain architecturally old across the trap-entry same-block cbo.flush",
        ai_micro_trap_entry_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_trap_entry_block_matches_seed(block_phys, block_seed)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-block cbo.flush returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-block cbo.flush sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_clean_preserves_old_image_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t block_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x380U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x380U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t low_initial[8] = {
        0x71U, 0x72U, 0x73U, 0x74U, 0x75U, 0x76U, 0x77U, 0x78U
    };
    const uint8_t src_pattern[16] = {
        0xb1U, 0xb2U, 0xb3U, 0xb4U, 0xb5U, 0xb6U, 0xb7U, 0xb8U,
        0xb9U, 0xbaU, 0xbbU, 0xbcU, 0xbdU, 0xbeU, 0xbfU, 0xc0U
    };
    const uint64_t block_seed = 0x31425364758697a8ULL;
    const uint64_t pressure_first_seed = 0x76543210fedcba98ULL;
    const uint64_t pressure_second_seed = 0x89abcdef01234567ULL;
    const uint64_t pressure_third_seed = 0x1021324354657687ULL;
    const uint64_t pressure_fourth_seed = 0x7867564534231201ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uintptr_t old_menvcfg = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(low_phys, low_initial, sizeof(low_initial));
    ai_micro_trap_entry_seed_block(block_phys, block_seed);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    old_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBZFE);
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_clean_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(fault_elem_vaddr);
    TEST_ASSERT("the high-page fault block is readable before the later late-fault vector-store plus trap-entry same-block cbo.clean probe begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-block cbo.clean probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-block cbo.clean probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store plus trap-entry same-block cbo.clean probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-block cbo.clean runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-block cbo.clean runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-block cbo.clean runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.clean runs at handler head on the block that contains the first faulting byte address instead of deadlocking behind the late-fault vector-store path",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the older elements before the late fault do commit, but the faulting third element, its idx-plus-one element, and the rest of the fault block remain architecturally old across the trap-entry same-block cbo.clean",
        ai_micro_trap_entry_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_trap_entry_block_matches_seed(block_phys, block_seed)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-block cbo.clean returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-block cbo.clean sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_inval_preserves_old_image_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t block_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x3c0U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x3c0U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t low_initial[8] = {
        0x81U, 0x82U, 0x83U, 0x84U, 0x85U, 0x86U, 0x87U, 0x88U
    };
    const uint8_t src_pattern[16] = {
        0xc1U, 0xc2U, 0xc3U, 0xc4U, 0xc5U, 0xc6U, 0xc7U, 0xc8U,
        0xc9U, 0xcaU, 0xcbU, 0xccU, 0xcdU, 0xceU, 0xcfU, 0xd0U
    };
    const uint64_t block_seed = 0x1029384756abcdefULL;
    const uint64_t pressure_first_seed = 0x55aa33cc77ee1199ULL;
    const uint64_t pressure_second_seed = 0x9911ee77cc33aa55ULL;
    const uint64_t pressure_third_seed = 0x0123456789abcdefULL;
    const uint64_t pressure_fourth_seed = 0xfedcba9876543210ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uintptr_t old_menvcfg = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(low_phys, low_initial, sizeof(low_initial));
    ai_micro_trap_entry_seed_block(block_phys, block_seed);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    old_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBIE01);
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_inval_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(fault_elem_vaddr);
    TEST_ASSERT("the high-page fault block is readable before the later late-fault vector-store plus trap-entry same-block cbo.inval probe begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-block cbo.inval probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-block cbo.inval probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store plus trap-entry same-block cbo.inval probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-block cbo.inval runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-block cbo.inval runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-block cbo.inval runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.inval runs at handler head on the block that contains the first faulting byte address instead of deadlocking behind the late-fault vector-store path",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the older elements before the late fault do commit, but the faulting third element, its idx-plus-one element, and the rest of the fault block remain architecturally old across the trap-entry same-block cbo.inval",
        ai_micro_trap_entry_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_trap_entry_block_matches_seed(block_phys, block_seed)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-block cbo.inval returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-block cbo.inval sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_zero_only_handler_zero_visible_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t block_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x400U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x400U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t low_initial[8] = {
        0x91U, 0x92U, 0x93U, 0x94U, 0x95U, 0x96U, 0x97U, 0x98U
    };
    const uint8_t src_pattern[16] = {
        0xd1U, 0xd2U, 0xd3U, 0xd4U, 0xd5U, 0xd6U, 0xd7U, 0xd8U,
        0xd9U, 0xdaU, 0xdbU, 0xdcU, 0xddU, 0xdeU, 0xdfU, 0xe0U
    };
    const uint64_t block_seed = 0x5a4b3c2d1e0f9081ULL;
    const uint64_t pressure_first_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t pressure_second_seed = 0x8796a5b4c3d2e1f0ULL;
    const uint64_t pressure_third_seed = 0x1234432112344321ULL;
    const uint64_t pressure_fourth_seed = 0x8765567887655678ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uintptr_t old_menvcfg = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(low_phys, low_initial, sizeof(low_initial));
    ai_micro_trap_entry_seed_block(block_phys, block_seed);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    old_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBZE);
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);
    ai_micro_trap_entry_store_fix_pte_addr =
        (uintptr_t)ai_micro_trap_entry_hs_leaf(high_page);
    ai_micro_trap_entry_store_fix_pte_value =
        (phys_page_base(high_page) >> 2) | (PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_zero_with_pte_fix_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(fault_elem_vaddr);
    TEST_ASSERT("the high-page fault block is readable before the later late-fault vector-store plus trap-entry same-block cbo.zero probe begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-block cbo.zero probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-block cbo.zero probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store plus trap-entry same-block cbo.zero probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    sfence_vma();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-block cbo.zero runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-block cbo.zero runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-block cbo.zero runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.zero runs at handler head after local high-page leaf repair on the block that contains the first faulting byte address",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the older elements before the late fault do commit, while the local trap-entry same-block cbo.zero is the only architecturally visible writer on the fault block that also contains idx-plus-one",
        ai_micro_trap_entry_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_trap_entry_block_is_zero(block_phys)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-block cbo.zero returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-block cbo.zero sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_byte_store_only_handler_byte_visible_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x3a0U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x3a0U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[24] = {
        0x51U, 0x52U, 0x53U, 0x54U, 0x55U, 0x56U, 0x57U, 0x58U,
        0x59U, 0x5aU, 0x5bU, 0x5cU, 0x5dU, 0x5eU, 0x5fU, 0x60U,
        0x61U, 0x62U, 0x63U, 0x64U, 0x65U, 0x66U, 0x67U, 0x68U
    };
    const uint8_t src_pattern[16] = {
        0xa1U, 0xa2U, 0xa3U, 0xa4U, 0xa5U, 0xa6U, 0xa7U, 0xa8U,
        0xa9U, 0xaaU, 0xabU, 0xacU, 0xadU, 0xaeU, 0xafU, 0xb0U,
    };
    uint8_t expected_final[24] = {0};
    const uint64_t handler_byte = 0x3cU;
    const uint64_t pressure_first_seed = 0x6a5b4c3d2e1f9081ULL;
    const uint64_t pressure_second_seed = 0x81901f2e3d4c5b6aULL;
    const uint64_t pressure_third_seed = 0x1357ace02468bdf1ULL;
    const uint64_t pressure_fourth_seed = 0xf1bd6824e0ac5713ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    memcpy(expected_final, target_initial, sizeof(expected_final));
    memcpy(expected_final, src_pattern, 8U);
    expected_final[8] = (uint8_t)handler_byte;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(low_phys, target_initial, 8U);
    ai_micro_trap_entry_write_bytes(high_phys, target_initial + 8U, 16U);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);
    ai_micro_trap_entry_probe_value = handler_byte;
    ai_micro_trap_entry_store_fix_pte_addr =
        (uintptr_t)ai_micro_trap_entry_hs_leaf(high_page);
    ai_micro_trap_entry_store_fix_pte_value =
        (phys_page_base(high_page) >> 2) | (PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_byte_store_with_pte_fix_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-fault byte-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-fault byte-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store plus trap-entry same-fault byte-store probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    sfence_vma();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-fault byte store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-fault byte store runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-fault byte store runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry byte store fires at handler head on the first faulting byte address",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the case-local trap-entry byte store is the only visible writer on the first faulting byte while the late-fault vector store contributes no ghost byte at the same address",
        ai_micro_trap_entry_bytes_match(low_phys, expected_final, sizeof(expected_final))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-fault byte store returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-fault byte-store sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_vector_store_only_handler_16b_image_visible_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x3e0U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x3e0U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[24] = {
        0x71U, 0x72U, 0x73U, 0x74U, 0x75U, 0x76U, 0x77U, 0x78U,
        0x79U, 0x7aU, 0x7bU, 0x7cU, 0x7dU, 0x7eU, 0x7fU, 0x80U,
        0x81U, 0x82U, 0x83U, 0x84U, 0x85U, 0x86U, 0x87U, 0x88U
    };
    const uint8_t src_pattern[16] = {
        0xd1U, 0xd2U, 0xd3U, 0xd4U, 0xd5U, 0xd6U, 0xd7U, 0xd8U,
        0xd9U, 0xdaU, 0xdbU, 0xdcU, 0xddU, 0xdeU, 0xdfU, 0xe0U,
    };
    const uint8_t handler_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0x21U, 0x32U, 0x43U, 0x54U, 0x65U, 0x76U, 0x87U, 0x98U,
        0xa9U, 0xbaU, 0xcbU, 0xdcU, 0xedU, 0xfeU, 0x1fU, 0x20U
    };
    uint8_t expected_final[24] = {0};
    const uint64_t pressure_first_seed = 0x0a0b0c0d0e0f1011ULL;
    const uint64_t pressure_second_seed = 0x11100f0e0d0c0b0aULL;
    const uint64_t pressure_third_seed = 0x8877665544332211ULL;
    const uint64_t pressure_fourth_seed = 0x1122334455667788ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    memcpy(expected_final, target_initial, sizeof(expected_final));
    memcpy(expected_final, src_pattern, 8U);
    memcpy(expected_final + 8U, handler_image, AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_prepare_vector_store_source(handler_image);
    ai_micro_trap_entry_write_bytes(low_phys, target_initial, 8U);
    ai_micro_trap_entry_write_bytes(high_phys, target_initial + 8U, 16U);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);
    ai_micro_trap_entry_store_fix_pte_addr =
        (uintptr_t)ai_micro_trap_entry_hs_leaf(high_page);
    ai_micro_trap_entry_store_fix_pte_value =
        (phys_page_base(high_page) >> 2) | (PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_vector_store_with_pte_fix_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-fault vector-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-fault vector-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store plus trap-entry same-fault vector-store probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    sfence_vma();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-fault vector store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-fault vector store runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-fault vector store runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry vector store fires at handler head on the first faulting byte address",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the case-local trap-entry vector store is the only visible writer across the 16-byte fault idx and idx-plus-one image while the late-fault vector store contributes no ghost write on the same window",
        ai_micro_trap_entry_bytes_match(low_phys, expected_final, sizeof(expected_final))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-fault vector store returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-fault vector-store sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_doubleword_store_only_handler_8b_image_visible_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t vector_base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x420U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x420U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[24] = {
        0xd1U, 0xd2U, 0xd3U, 0xd4U, 0xd5U, 0xd6U, 0xd7U, 0xd8U,
        0xd9U, 0xdaU, 0xdbU, 0xdcU, 0xddU, 0xdeU, 0xdfU, 0xe0U,
        0xe1U, 0xe2U, 0xe3U, 0xe4U, 0xe5U, 0xe6U, 0xe7U, 0xe8U
    };
    const uint8_t src_pattern[16] = {
        0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U,
        0x39U, 0x3aU, 0x3bU, 0x3cU, 0x3dU, 0x3eU, 0x3fU, 0x40U,
    };
    uint8_t expected_final[24] = {0};
    const uint64_t handler_value = 0x8877665544332211ULL;
    const uint64_t pressure_first_seed = 0x5566778899aabbccULL;
    const uint64_t pressure_second_seed = 0xccbbaa9988776655ULL;
    const uint64_t pressure_third_seed = 0x1029384756abcdefULL;
    const uint64_t pressure_fourth_seed = 0xfedcba5647382910ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    memcpy(expected_final, target_initial, sizeof(expected_final));
    memcpy(expected_final, src_pattern, 8U);
    memcpy(expected_final + 8U, &handler_value, sizeof(handler_value));

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(low_phys, target_initial, 8U);
    ai_micro_trap_entry_write_bytes(high_phys, target_initial + 8U, 16U);
    ai_micro_trap_entry_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_elem_vaddr);
    ai_micro_trap_entry_probe_value = handler_value;
    ai_micro_trap_entry_store_fix_pte_addr =
        (uintptr_t)ai_micro_trap_entry_hs_leaf(high_page);
    ai_micro_trap_entry_store_fix_pte_value =
        (phys_page_base(high_page) >> 2) | (PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_doubleword_store_with_pte_fix_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late-fault vector-store plus trap-entry same-fault doubleword-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late-fault vector-store plus trap-entry same-fault doubleword-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the 4x32-bit source vector load stages the full payload before the later third-element page-fault vector store plus trap-entry same-fault doubleword-store probe",
        excpt.triggered == false
    );

    ai_micro_trap_entry_refresh_final_pressure_tail(fill_vaddr_base,
        pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    vse32_from_v6((uint32_t *)vector_base_vaddr, 4);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_set_hs_leaf(high_page, PTE_V | PTE_A | PTE_RWX);
    sfence_vma();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later 4x32-bit vector store still reports HS translated-store fault before the trap-entry same-fault doubleword store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF ||
        ai_micro_trap_entry_trap_cause == CAUSE_SAF
    );
    TEST_ASSERT("the late-fault third-element vector store reports the first faulting byte address in tval before the trap-entry same-fault doubleword store runs",
        ai_micro_trap_entry_trap_tval == fault_elem_vaddr
    );
    TEST_ASSERT("the late-fault third-element vector store reports vstart=2 before the trap-entry same-fault doubleword store runs",
        ai_micro_trap_entry_trap_vstart == 2ULL
    );
    TEST_ASSERT("the case-local trap-entry same-fault doubleword store fires at handler head on the first faulting byte address",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the case-local trap-entry same-fault doubleword store is the only visible writer across the 8-byte fault idx plus idx-plus-one window while the late-fault vector store contributes no ghost write on that window",
        ai_micro_trap_entry_bytes_match(low_phys, expected_final, sizeof(expected_final))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later late-fault vector-store trap before the trap-entry same-fault doubleword store returns",
        fill_visible
    );
    TEST_ASSERT("the staged 4x32-bit source payload remains intact after the late-fault vector-store plus trap-entry same-fault doubleword-store sequence",
        ai_micro_trap_entry_bytes_match(src_phys, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}
