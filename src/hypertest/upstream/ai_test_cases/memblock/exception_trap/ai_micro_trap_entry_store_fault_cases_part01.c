#include "ai_micro_trap_entry_store_fault_common.h"

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_byte_load_sees_old_value_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x180U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x180U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t target_initial = 0x1122334455667788ULL;
    const uint64_t fault_value = 0x89abcdef01234567ULL;
    const uint64_t expected_fault_byte = 0x88ULL;
    const uint64_t pressure_first_seed = 0x13579bdf2468ace0ULL;
    const uint64_t pressure_second_seed = 0x0fedcba987654321ULL;
    const uint64_t pressure_third_seed = 0x55aa55aa33cc33ccULL;
    const uint64_t pressure_fourth_seed = 0xaa55aa55cc33cc33ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    write64(target_phys, target_initial);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(target_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_byte_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed, pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the faulting store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed, pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the faulting store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(target_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate sbuffer-filling translated-store burst, the later read-only-leaf scalar store still reports a precise HS store page fault on the original same-address probe target",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == target_vaddr
    );
    TEST_ASSERT("the case-local trap-entry byte load runs at handler head and sees the old byte on the same fault address instead of any not-yet-committed data from the faulting translated store",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_probe_value == expected_fault_byte
    );
    TEST_ASSERT("after the same-address byte-load probe, the handler also replays the prepared translated-store tail burst so the exception-return window stays under active store pressure",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap",
        fill_visible
    );
    TEST_ASSERT("the faulting translated store itself leaves the target doubleword unchanged even though the trap handler immediately reloads that same address",
        read64(target_phys) == target_initial
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_doubleword_load_sees_old_value_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x1c0U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x1c0U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t target_initial = 0x8877665544332211ULL;
    const uint64_t fault_value = 0x1021324354657687ULL;
    const uint64_t pressure_first_seed = 0x3141592653589793ULL;
    const uint64_t pressure_second_seed = 0x2384626433832795ULL;
    const uint64_t pressure_third_seed = 0xdeadc0de55aa11eeULL;
    const uint64_t pressure_fourth_seed = 0x0badf00d33cc77aaULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    write64(target_phys, target_initial);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF));
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(target_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_doubleword_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed, pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the same-address doubleword-load trap-entry probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed, pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the same-address doubleword-load trap-entry probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(target_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after two full-page dense translated-store pressure bursts, the later read-only-leaf scalar store still reports a precise HS store page fault on the original same-address probe target before the trap-entry doubleword load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == target_vaddr
    );
    TEST_ASSERT("the case-local trap-entry doubleword load runs at handler head and sees the full old committed doubleword on the same fault address instead of any partially forwarded bytes from the faulting translated store",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_probe_value == target_initial
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap before the same-address doubleword load returns",
        fill_visible
    );
    TEST_ASSERT("the faulting translated store itself leaves the target doubleword unchanged even when the trap-entry probe immediately re-reads that same doubleword",
        read64(target_phys) == target_initial
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_vector_load_sees_old_16b_image_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x280U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x280U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0x40U, 0x51U, 0x62U, 0x73U, 0x84U, 0x95U, 0xa6U, 0xb7U,
        0xc8U, 0xd9U, 0xeaU, 0xfbU, 0x1cU, 0x2dU, 0x3eU, 0x4fU
    };
    const uint64_t fault_value = 0x90a0b0c0d0e0f000ULL;
    const uint64_t pressure_first_seed = 0x123456789abcdef0ULL;
    const uint64_t pressure_second_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t pressure_third_seed = 0xa55aa55a33cc33ccULL;
    const uint64_t pressure_fourth_seed = 0x55aa55aacc33cc33ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(target_phys, target_initial,
        AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF));
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(target_vaddr);

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
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the trap-entry same-address vector load probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the trap-entry same-address vector load probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(target_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later read-only-leaf scalar store still reports a precise HS store page fault on the original same-address vector-load probe target",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == target_vaddr
    );
    TEST_ASSERT("the case-local trap-entry vector load runs at handler head and captures the old 16-byte image spanning the fault bytes and the immediately following idx-plus-one bytes",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_probe_bytes_match(target_initial,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES)
    );
    TEST_ASSERT("the same-address vector-load trap-entry path also executes the common post-observation handler pressure burst so this case keeps stressing store-buffer drain immediately after the first probe",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap before the trap-entry vector load returns",
        fill_visible
    );
    TEST_ASSERT("the faulting translated store itself leaves the probed 16-byte region unchanged even though the trap-entry vector load immediately re-reads the fault bytes and the next bytes",
        ai_micro_trap_entry_bytes_match(target_phys, target_initial,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES)
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_flush_preserves_old_image_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t block_vaddr = hs_page_base(target_page) + 0x340U;
    const uintptr_t fault_vaddr = block_vaddr + 8U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t block_phys = phys_page_base(target_page) + 0x340U;
    const uintptr_t fault_phys = block_phys + 8U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t block_seed = 0x6c8e9fb0a1b2c3d4ULL;
    const uint64_t fault_value = 0xfeedfacecafebeefULL;
    const uint64_t pressure_first_seed = 0x1029384756abcdefULL;
    const uint64_t pressure_second_seed = 0xfedcba9876543210ULL;
    const uint64_t pressure_third_seed = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t pressure_fourth_seed = 0x89abcdef76543210ULL;
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
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBZFE);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_flush_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(block_vaddr);
    TEST_ASSERT("the target cache block is readable before the later store page-fault plus trap-entry cbo.flush probe sequence begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the trap-entry same-block cbo.flush probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the trap-entry same-block cbo.flush probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later read-only-leaf scalar store still reports a precise HS store page fault before the trap-entry same-block cbo.flush runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the case-local trap-entry cbo.flush runs at handler head on the faulting cache block address instead of deadlocking behind the earlier store page-fault path",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-block cbo.flush does not let the faulting translated store leak any uncommitted bytes into the backing cache block image",
        ai_micro_trap_entry_block_matches_seed(block_phys, block_seed)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap before the trap-entry cbo.flush returns",
        fill_visible
    );
    TEST_ASSERT("the specific faulting doubleword inside the probed cache block remains unchanged after the trap-entry same-block cbo.flush executes",
        read64(fault_phys) == ai_micro_trap_entry_dense_store_pattern(block_seed, 1U)
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_byte_store_only_handler_byte_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x3c0U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x3c0U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[16] = {
        0x91U, 0x92U, 0x93U, 0x94U, 0x95U, 0x96U, 0x97U, 0x98U,
        0x99U, 0x9aU, 0x9bU, 0x9cU, 0x9dU, 0x9eU, 0x9fU, 0xa0U
    };
    uint8_t expected_final[16] = {0};
    const uint64_t fault_value = 0x7766554433221100ULL;
    const uint64_t handler_byte = 0x3dULL;
    const uint64_t pressure_first_seed = 0x2468ace13579bdf0ULL;
    const uint64_t pressure_second_seed = 0x0fdb97531eca8642ULL;
    const uint64_t pressure_third_seed = 0xcc3355aa77ee1199ULL;
    const uint64_t pressure_fourth_seed = 0x9911ee77aa5533ccULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    memcpy(expected_final, target_initial, sizeof(expected_final));
    expected_final[0] = (uint8_t)handler_byte;

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
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(target_vaddr);
    ai_micro_trap_entry_probe_value = handler_byte;
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
    CSRW(stvec, ai_micro_trap_entry_same_addr_byte_store_with_pte_fix_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the translated-store page-fault plus trap-entry same-address byte-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the translated-store page-fault plus trap-entry same-address byte-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(target_vaddr, fault_value);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    sfence_vma();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later read-only-leaf scalar store still reports a precise HS store page fault before the trap-entry same-address byte store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == target_vaddr
    );
    TEST_ASSERT("the case-local trap-entry same-address byte store is the only architecturally visible writer on the fault byte while the translated store page-fault itself contributes no ghost byte",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_bytes_match(target_phys, expected_final,
            sizeof(expected_final))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap before the same-address byte store returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_vector_store_only_handler_16b_image_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x440U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x440U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0xb1U, 0xb2U, 0xb3U, 0xb4U, 0xb5U, 0xb6U, 0xb7U, 0xb8U,
        0xb9U, 0xbaU, 0xbbU, 0xbcU, 0xbdU, 0xbeU, 0xbfU, 0xc0U
    };
    const uint8_t handler_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0x41U, 0x52U, 0x63U, 0x74U, 0x85U, 0x96U, 0xa7U, 0xb8U,
        0xc9U, 0xdaU, 0xebU, 0xfcU, 0x1dU, 0x2eU, 0x3fU, 0x40U
    };
    const uint64_t fault_value = 0x1122448866aaccf0ULL;
    const uint64_t pressure_first_seed = 0x1020304050607080ULL;
    const uint64_t pressure_second_seed = 0x8070605040302010ULL;
    const uint64_t pressure_third_seed = 0x55aa33cc77ee1199ULL;
    const uint64_t pressure_fourth_seed = 0x9911ee77cc33aa55ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

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
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(target_vaddr);
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
    CSRW(stvec, ai_micro_trap_entry_same_addr_vector_store_with_pte_fix_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the translated-store page-fault plus trap-entry same-address vector-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the translated-store page-fault plus trap-entry same-address vector-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(target_vaddr, fault_value);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    sfence_vma();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later read-only-leaf scalar store still reports a precise HS store page fault before the trap-entry same-address vector store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == target_vaddr
    );
    TEST_ASSERT("the case-local trap-entry same-address vector store is the only architecturally visible writer across the 16-byte fault window while the translated store page-fault itself contributes no ghost write",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_bytes_match(target_phys, handler_image,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap before the same-address vector store returns",
        fill_visible
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_doubleword_store_only_handler_8b_image_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x500U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x500U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[16] = {
        0xc1U, 0xc2U, 0xc3U, 0xc4U, 0xc5U, 0xc6U, 0xc7U, 0xc8U,
        0xc9U, 0xcaU, 0xcbU, 0xccU, 0xcdU, 0xceU, 0xcfU, 0xd0U
    };
    uint8_t expected_final[16] = {0};
    const uint64_t fault_value = 0x1234567890abcdefULL;
    const uint64_t handler_value = 0xfedcba0987654321ULL;
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
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(target_vaddr);
    ai_micro_trap_entry_probe_value = handler_value;
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
    CSRW(stvec, ai_micro_trap_entry_same_addr_doubleword_store_with_pte_fix_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the translated-store page-fault plus trap-entry same-address doubleword-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the translated-store page-fault plus trap-entry same-address doubleword-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(target_vaddr, fault_value);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    sfence_vma();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later read-only-leaf scalar store still reports a precise HS store page fault before the trap-entry same-address doubleword store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == target_vaddr
    );
    TEST_ASSERT("the case-local trap-entry same-address doubleword store is the only architecturally visible writer across the 8-byte fault window while the translated store page-fault itself contributes no ghost write",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_bytes_match(target_phys, expected_final,
            sizeof(expected_final))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap before the same-address doubleword store returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_clean_preserves_old_image_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t block_vaddr = hs_page_base(target_page) + 0x3c0U;
    const uintptr_t fault_vaddr = block_vaddr + 8U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t block_phys = phys_page_base(target_page) + 0x3c0U;
    const uintptr_t fault_phys = block_phys + 8U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t block_seed = 0x4d5e6f708192a3b4ULL;
    const uint64_t fault_value = 0x0fedcba987654321ULL;
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
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBZFE);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_clean_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(block_vaddr);
    TEST_ASSERT("the target cache block is readable before the later store page-fault plus trap-entry cbo.clean probe sequence begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the trap-entry same-block cbo.clean probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the trap-entry same-block cbo.clean probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later read-only-leaf scalar store still reports a precise HS store page fault before the trap-entry same-block cbo.clean runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the case-local trap-entry cbo.clean runs at handler head on the faulting cache block address instead of deadlocking behind the earlier store page-fault path",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-block cbo.clean does not let the faulting translated store leak any uncommitted bytes into the backing cache block image",
        ai_micro_trap_entry_block_matches_seed(block_phys, block_seed)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap before the trap-entry cbo.clean returns",
        fill_visible
    );
    TEST_ASSERT("the specific faulting doubleword inside the probed cache block remains unchanged after the trap-entry same-block cbo.clean executes",
        read64(fault_phys) == ai_micro_trap_entry_dense_store_pattern(block_seed, 1U)
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_inval_preserves_old_image_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t block_vaddr = hs_page_base(target_page) + 0x440U;
    const uintptr_t fault_vaddr = block_vaddr + 8U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t block_phys = phys_page_base(target_page) + 0x440U;
    const uintptr_t fault_phys = block_phys + 8U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t block_seed = 0x7b6a594837261504ULL;
    const uint64_t fault_value = 0x89abcdef01234567ULL;
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
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBIE01);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_inval_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(block_vaddr);
    TEST_ASSERT("the target cache block is readable before the later store page-fault plus trap-entry cbo.inval probe sequence begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the trap-entry same-block cbo.inval probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the trap-entry same-block cbo.inval probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later read-only-leaf scalar store still reports a precise HS store page fault before the trap-entry same-block cbo.inval runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the case-local trap-entry cbo.inval runs at handler head on the faulting cache block address instead of deadlocking behind the earlier store page-fault path",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-block cbo.inval does not let the faulting translated store leak any uncommitted bytes into the backing cache block image",
        ai_micro_trap_entry_block_matches_seed(block_phys, block_seed)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap before the trap-entry cbo.inval returns",
        fill_visible
    );
    TEST_ASSERT("the specific faulting doubleword inside the probed cache block remains unchanged after the trap-entry same-block cbo.inval executes",
        read64(fault_phys) == ai_micro_trap_entry_dense_store_pattern(block_seed, 1U)
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_zero_only_handler_zero_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t block_vaddr = hs_page_base(target_page) + 0x640U;
    const uintptr_t fault_vaddr = block_vaddr + 8U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t block_phys = phys_page_base(target_page) + 0x640U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t block_seed = 0x6f5e4d3c2b1a0918ULL;
    const uint64_t fault_value = 0x123456789abcdef0ULL;
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
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF));
    CSRW(CSR_MENVCFG, old_menvcfg | MENVCFG_CBZE);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_trap_entry_set_hs_leaf(fill_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);
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
    TEST_ASSERT("the target cache block is readable before the later store page-fault plus trap-entry cbo.zero probe sequence begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the trap-entry same-block cbo.zero probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the trap-entry same-block cbo.zero probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_R);
    sfence_vma();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later read-only-leaf scalar store still reports a precise HS store page fault before the trap-entry same-block cbo.zero runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.zero runs at handler head after local PTE repair on the faulting cache block address",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the same-block cbo.zero trap-entry path also executes the common post-observation handler pressure burst so the repaired handler still keeps store-buffer pressure high immediately after the first zeroing action",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.zero is the only architecturally visible writer on the fault block while the original translated store contributes no ghost bytes",
        ai_micro_trap_entry_block_is_zero(block_phys)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later translated store page-fault trap before the trap-entry cbo.zero returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_byte_load_sees_old_value_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWXPbmt1_GURWXPbmt0;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x340U;
    const uintptr_t fault_vaddr = target_base_vaddr + 1U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[18] = {
        0x41U, 0x52U, 0x63U, 0x74U, 0x85U, 0x96U, 0xa7U, 0xb8U, 0xc9U,
        0xdaU, 0xebU, 0xfcU, 0x1dU, 0x2eU, 0x3fU, 0x40U, 0x51U, 0x62U
    };
    const uint64_t fault_value = 0x7766554433221100ULL;
    const uint64_t pressure_first_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t pressure_second_seed = 0x8796a5b4c3d2e1f0ULL;
    const uint64_t pressure_third_seed = 0x1234432112344321ULL;
    const uint64_t pressure_fourth_seed = 0x8765567887655678ULL;
    const uint8_t expected_fault_byte = target_initial[1];
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;
    bool target_visible = false;

    goto_priv(PRIV_M);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SAM));
    ai_micro_trap_entry_prepare_pbmt_nc_alias(target_page);

    ai_micro_trap_entry_reset_local_state(fault_vaddr);

    goto_priv(PRIV_HS);
    ai_micro_trap_entry_write_bytes(target_base_vaddr, target_initial,
        sizeof(target_initial));
    __sync_synchronize();

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
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the misaligned-store plus trap-entry same-address byte-load probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the misaligned-store plus trap-entry same-address byte-load probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);
    __sync_synchronize();
    target_visible = ai_micro_trap_entry_bytes_match(target_base_vaddr,
        target_initial, sizeof(target_initial));

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_reset_pbmt_pbmte_only_env();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later PBMT=NC non-device misaligned scalar store still reports a precise HS store-address-misaligned trap on the original same-address handler-load target",
        ai_micro_trap_entry_trap_cause == CAUSE_SAM &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the case-local trap-entry byte load runs at handler head and sees only the old committed byte at the misaligned fault address instead of any ghost byte from the faulting misaligned store",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_probe_value == expected_fault_byte
    );
    TEST_ASSERT("the misaligned-store trap-entry byte-load path also executes the common post-observation handler pressure burst so the handler continues applying dense pressure immediately after the earliest re-read",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the faulting misaligned scalar store itself leaves the entire probed overlap window unchanged even though the trap-entry byte load immediately re-reads the same fault address",
        target_visible
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later PBMT=NC misaligned store trap before the trap-entry byte load returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_vector_load_retrap_access_fault_no_ghost_image_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWXPbmt1_GURWXPbmt0;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x360U;
    const uintptr_t fault_vaddr = target_base_vaddr + 1U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_base_phys = phys_page_base(target_page) + 0x360U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[18] = {
        0x51U, 0x62U, 0x73U, 0x84U, 0x95U, 0xa6U, 0xb7U, 0xc8U, 0xd9U,
        0xeaU, 0xfbU, 0x1cU, 0x2dU, 0x3eU, 0x4fU, 0x50U, 0x61U, 0x72U
    };
    const uint64_t fault_value = 0x66554433221100ffULL;
    const uint64_t pressure_first_seed = 0x1029384756abcdefULL;
    const uint64_t pressure_second_seed = 0xfedcba9876543210ULL;
    const uint64_t pressure_third_seed = 0x55aa33cc77ee1199ULL;
    const uint64_t pressure_fourth_seed = 0x9911ee77cc33aa55ULL;
    bool fill_visible = false;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_write_bytes(target_base_phys, target_initial,
        sizeof(target_initial));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_LAF) | (1ULL << CAUSE_SAM));
    ai_micro_trap_entry_prepare_pbmt_nc_alias(target_page);

    ai_micro_trap_entry_reset_local_state(fault_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_vector_load_retrap_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the misaligned-store plus trap-entry same-address vector-load probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the misaligned-store plus trap-entry same-address vector-load probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_reset_pbmt_pbmte_only_env();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later PBMT=NC non-device misaligned scalar store still reports a precise HS store-address-misaligned trap before the handler-head same-address vector-load retrap",
        ai_micro_trap_entry_trap_cause == CAUSE_SAM &&
        ai_micro_trap_entry_trap_tval == fault_vaddr,
        "outer_cause=0x%lx expected_sam=0x%x outer_tval=0x%lx expected_tval=0x%lx outer_vstart=%lu",
        ai_micro_trap_entry_trap_cause, CAUSE_SAM,
        ai_micro_trap_entry_trap_tval, fault_vaddr,
        ai_micro_trap_entry_trap_vstart
    );
    TEST_ASSERT("the handler-head same-address PBMT=NC vector load immediately re-traps with a precise HS load-access-fault on the same misaligned fault address instead of acting as an old-image observer",
        ai_micro_trap_entry_probe_fired == 3ULL &&
        ai_micro_trap_entry_retrap_cause == CAUSE_LAF &&
        ai_micro_trap_entry_retrap_tval == fault_vaddr &&
        ai_micro_trap_entry_retrap_vstart == 0ULL,
        "probe_fired=%lu retrap_cause=0x%lx expected_laf=0x%x retrap_tval=0x%lx expected_tval=0x%lx retrap_vstart=%lu",
        ai_micro_trap_entry_probe_fired,
        ai_micro_trap_entry_retrap_cause, CAUSE_LAF,
        ai_micro_trap_entry_retrap_tval, fault_vaddr,
        ai_micro_trap_entry_retrap_vstart
    );
    TEST_ASSERT("the PBMT=NC misaligned-store plus handler-head vector-load access-fault retrap path also executes the common post-observation handler pressure burst before returning",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL,
        "handler_pressure_fired=%lu", ai_micro_trap_entry_handler_pressure_fired
    );
    TEST_ASSERT("the original PBMT=NC misaligned scalar store and the handler-head vector-load access-fault retrap together leave the entire probed overlap window unchanged with no ghost bytes",
        ai_micro_trap_entry_bytes_match(target_base_phys, target_initial,
            sizeof(target_initial))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later PBMT=NC misaligned-store trap before the handler-head vector-load access-fault retrap path returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_byte_store_only_handler_byte_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWXPbmt1_GURWXPbmt0;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x380U;
    const uintptr_t fault_vaddr = target_base_vaddr + 1U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[18] = {
        0x10U, 0x21U, 0x32U, 0x43U, 0x54U, 0x65U, 0x76U, 0x87U, 0x98U,
        0xa9U, 0xbaU, 0xcbU, 0xdcU, 0xedU, 0xfeU, 0x0fU, 0x1eU, 0x2dU
    };
    uint8_t expected_final[18] = {0};
    const uint64_t fault_value = 0x8877665544332211ULL;
    const uint64_t handler_byte = 0x5aULL;
    const uint64_t pressure_first_seed = 0x1111222233334444ULL;
    const uint64_t pressure_second_seed = 0x5555666677778888ULL;
    const uint64_t pressure_third_seed = 0x9999aaaabbbbccccULL;
    const uint64_t pressure_fourth_seed = 0xddddeeeeffff1234ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;
    bool target_visible = false;

    memcpy(expected_final, target_initial, sizeof(expected_final));
    expected_final[1] = (uint8_t)handler_byte;

    goto_priv(PRIV_M);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SAM));
    ai_micro_trap_entry_prepare_pbmt_nc_alias(target_page);

    ai_micro_trap_entry_reset_local_state(fault_vaddr);
    ai_micro_trap_entry_probe_value = handler_byte;

    goto_priv(PRIV_HS);
    ai_micro_trap_entry_write_bytes(target_base_vaddr, target_initial,
        sizeof(target_initial));
    __sync_synchronize();

    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_byte_store_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the misaligned-store plus trap-entry same-address byte-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the misaligned-store plus trap-entry same-address byte-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);
    __sync_synchronize();
    target_visible = ai_micro_trap_entry_bytes_match(target_base_vaddr,
        expected_final, sizeof(expected_final));

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_reset_pbmt_pbmte_only_env();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later PBMT=NC non-device misaligned scalar store still reports a precise HS store-address-misaligned trap on the original same-address handler-store target",
        ai_micro_trap_entry_trap_cause == CAUSE_SAM &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the case-local trap-entry byte store runs at handler head and leaves only the handler byte visible at the fault address while the misaligned scalar store itself contributes no ghost bytes",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        target_visible
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later PBMT=NC misaligned store trap before the trap-entry byte store returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_vector_store_only_handler_16b_image_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWXPbmt1_GURWXPbmt0;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x3c0U;
    const uintptr_t fault_vaddr = target_base_vaddr + 1U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_base_phys = phys_page_base(X) + 0x3c0U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[18] = {
        0xa0U, 0xa1U, 0xa2U, 0xa3U, 0xa4U, 0xa5U, 0xa6U, 0xa7U, 0xa8U,
        0xa9U, 0xaaU, 0xabU, 0xacU, 0xadU, 0xaeU, 0xafU, 0xb0U, 0xb1U
    };
    const uint8_t handler_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0x31U, 0x42U, 0x53U, 0x64U, 0x75U, 0x86U, 0x97U, 0xa8U,
        0xb9U, 0xcaU, 0xdbU, 0xecU, 0xfdU, 0x1eU, 0x2fU, 0x30U
    };
    uint8_t expected_final[18] = {0};
    const uint64_t fault_value = 0x1122334455667788ULL;
    const uint64_t pressure_first_seed = 0x13572468abcdef01ULL;
    const uint64_t pressure_second_seed = 0x24681357fedcba10ULL;
    const uint64_t pressure_third_seed = 0xaa33cc55ee77ff11ULL;
    const uint64_t pressure_fourth_seed = 0x11ff77ee55cc33aaULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    memcpy(expected_final, target_initial, sizeof(expected_final));
    memcpy(&expected_final[1], handler_image, AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_prepare_vector_store_source(handler_image);
    ai_micro_trap_entry_write_bytes(target_base_phys, target_initial,
        sizeof(target_initial));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SAM));
    ai_micro_trap_entry_prepare_pbmt_nc_alias(target_page);

    ai_micro_trap_entry_reset_local_state(fault_vaddr);
    ai_micro_trap_entry_store_fix_pte_addr =
        (uintptr_t)ai_micro_trap_entry_hs_leaf(target_page);
    ai_micro_trap_entry_store_fix_pte_value =
        (phys_page_base(X) >> 2) | (PTE_V | PTE_AD | PTE_RWX);

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
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the misaligned-store plus trap-entry same-address vector-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the misaligned-store plus trap-entry same-address vector-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_reset_pbmt_pbmte_only_env();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the later PBMT=NC non-device misaligned scalar store still reports a precise HS store-address-misaligned trap before the trap-entry same-address vector store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SAM &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("after the handler repairs the PBMT=NC alias to a cacheable same-PA leaf, the case-local trap-entry vector store overwrites exactly the 16-byte same-address image while the misaligned scalar store contributes no ghost write",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_bytes_match(target_base_phys, expected_final,
            sizeof(expected_final))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later PBMT=NC misaligned store trap before the trap-entry vector store returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_doubleword_load_retrap_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWXPbmt1_GURWXPbmt0;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr =
        hs_page_base(target_page) + 0x400U;
    const uintptr_t fault_vaddr = target_base_vaddr + 1U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_base_phys = phys_page_base(target_page) + 0x400U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[18] = {
        0x61U, 0x72U, 0x83U, 0x94U, 0xa5U, 0xb6U, 0xc7U, 0xd8U, 0xe9U,
        0xfaU, 0x1bU, 0x2cU, 0x3dU, 0x4eU, 0x5fU, 0x60U, 0x71U, 0x82U
    };
    const uint64_t fault_value = 0x8877665544332211ULL;
    const uint64_t pressure_first_seed = 0x0123456789abcdefULL;
    const uint64_t pressure_second_seed = 0xfedcba9876543210ULL;
    const uint64_t pressure_third_seed = 0x55aa33cc77ee1199ULL;
    const uint64_t pressure_fourth_seed = 0x99ee77cc33aa5511ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_write_bytes(target_base_phys, target_initial,
        sizeof(target_initial));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_LAM) | (1ULL << CAUSE_SAM));
    ai_micro_trap_entry_prepare_pbmt_nc_alias(target_page);

    ai_micro_trap_entry_reset_local_state(fault_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_doubleword_load_retrap_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the misaligned-store plus handler-head same-address doubleword-load retrap probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the misaligned-store plus handler-head same-address doubleword-load retrap probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_reset_pbmt_pbmte_only_env();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the original PBMT=NC non-device misaligned scalar store still reports a precise HS store-address-misaligned trap before the handler-head same-address doubleword load retraps",
        ai_micro_trap_entry_trap_cause == CAUSE_SAM &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the handler-head same-address doubleword load immediately re-traps with a second precise HS load-address-misaligned exception on that same PBMT=NC fault address instead of deadlocking behind the original misaligned store path",
        ai_micro_trap_entry_probe_fired == 3ULL &&
        ai_micro_trap_entry_retrap_cause == CAUSE_LAM &&
        ai_micro_trap_entry_retrap_tval == fault_vaddr
    );
    TEST_ASSERT("the original PBMT=NC misaligned scalar store and the handler-head same-address doubleword-load retrap together leave the full overlapped observation window architecturally unchanged with no ghost bytes",
        ai_micro_trap_entry_bytes_match(target_base_phys, target_initial,
            sizeof(target_initial))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later PBMT=NC misaligned-store trap before the handler-head same-address doubleword-load retrap path returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_doubleword_store_retrap_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWXPbmt1_GURWXPbmt0;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr =
        hs_page_base(target_page) + 0x420U;
    const uintptr_t fault_vaddr = target_base_vaddr + 1U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_base_phys = phys_page_base(target_page) + 0x420U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[18] = {
        0x81U, 0x92U, 0xa3U, 0xb4U, 0xc5U, 0xd6U, 0xe7U, 0xf8U, 0x19U,
        0x2aU, 0x3bU, 0x4cU, 0x5dU, 0x6eU, 0x7fU, 0x80U, 0x91U, 0xa2U
    };
    const uint64_t fault_value = 0x1021324354657687ULL;
    const uint64_t handler_value = 0xfedcba0987654321ULL;
    const uint64_t pressure_first_seed = 0x1111222233334444ULL;
    const uint64_t pressure_second_seed = 0x5555666677778888ULL;
    const uint64_t pressure_third_seed = 0x9999aaaabbbbccccULL;
    const uint64_t pressure_fourth_seed = 0xddddeeeeffff1234ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    ai_micro_trap_entry_write_bytes(target_base_phys, target_initial,
        sizeof(target_initial));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SAM));
    ai_micro_trap_entry_prepare_pbmt_nc_alias(target_page);

    ai_micro_trap_entry_reset_local_state(fault_vaddr);
    ai_micro_trap_entry_probe_value = handler_value;

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_doubleword_store_retrap_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the misaligned-store plus handler-head same-address doubleword-store retrap probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the misaligned-store plus handler-head same-address doubleword-store retrap probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    sd(fault_vaddr, fault_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_reset_pbmt_pbmte_only_env();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer translated-store burst, the original PBMT=NC non-device misaligned scalar store still reports a precise HS store-address-misaligned trap before the handler-head same-address doubleword store retraps",
        ai_micro_trap_entry_trap_cause == CAUSE_SAM &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the handler-head same-address doubleword store immediately re-traps with a second precise HS store-address-misaligned exception on that same PBMT=NC fault address instead of partially writing any bytes",
        ai_micro_trap_entry_probe_fired == 3ULL &&
        ai_micro_trap_entry_retrap_cause == CAUSE_SAM &&
        ai_micro_trap_entry_retrap_tval == fault_vaddr
    );
    TEST_ASSERT("the original PBMT=NC misaligned scalar store and the handler-head same-address doubleword-store retrap together leave the full overlapped observation window architecturally unchanged with no ghost bytes",
        ai_micro_trap_entry_bytes_match(target_base_phys, target_initial,
            sizeof(target_initial))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later PBMT=NC misaligned-store trap before the handler-head same-address doubleword-store retrap path returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_doubleword_load_sees_latest_committed_value_corner()
{
    TEST_START();

    const enum test_page target_page = SCRATCHPAD;
    const uintptr_t fill_vaddr_base = hs_page_base(target_page);
    const uintptr_t fill_phys_base = phys_page_base(target_page);
    const uintptr_t target_line_index = 17U;
    const uintptr_t target_vaddr = fill_vaddr_base +
        target_line_index * AI_MICRO_TRAP_ENTRY_FILL_STRIDE;
    const uintptr_t target_phys = fill_phys_base +
        target_line_index * AI_MICRO_TRAP_ENTRY_FILL_STRIDE;
    const uint64_t pressure_first_seed = 0x2468ace013579bdfULL;
    const uint64_t pressure_second_seed = 0xfdb97531e0ca8642ULL;
    const uint64_t pressure_third_seed = 0x1122aa55cc3377eeULL;
    const uint64_t pressure_fourth_seed = 0xee7733cc55aa2211ULL;
    const uint64_t expected_committed_value =
        ai_micro_trap_entry_dense_store_pattern(pressure_fourth_seed,
            target_line_index);
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    goto_priv(PRIV_M);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_BKP));
    ai_micro_trap_entry_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(target_vaddr);

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
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later HS breakpoint trap-entry same-address doubleword-load probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later HS breakpoint trap-entry same-address doubleword-load probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    ai_micro_trap_entry_ebreak32();

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-address doubleword load runs at handler head after the breakpoint and immediately observes the latest committed target-line value from the older full-page translated-store pressure burst",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_probe_value == expected_committed_value
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-address doubleword load returns",
        fill_visible
    );
    TEST_ASSERT("the probed target line still matches the final pressure-burst image after the breakpoint trap-entry same-address doubleword load sequence",
        read64(target_phys) == expected_committed_value
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_byte_store_only_handler_byte_visible_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x3e0U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x3e0U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[16] = {
        0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U,
        0x39U, 0x3aU, 0x3bU, 0x3cU, 0x3dU, 0x3eU, 0x3fU, 0x40U
    };
    uint8_t expected_final[16] = {0};
    const uint64_t committed_value = 0x8877665544332211ULL;
    const uint64_t handler_byte = 0x5aULL;
    const uint64_t pressure_first_seed = 0x3141592653589793ULL;
    const uint64_t pressure_second_seed = 0x2718281828459045ULL;
    const uint64_t pressure_third_seed = 0x55aa11ee77cc33ddULL;
    const uint64_t pressure_fourth_seed = 0xdd33cc77ee11aa55ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    memcpy(expected_final, target_initial, sizeof(expected_final));
    memcpy(expected_final, &committed_value, sizeof(committed_value));
    expected_final[0] = (uint8_t)handler_byte;

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
    ai_micro_trap_entry_probe_value = handler_byte;

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_addr_byte_store_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later breakpoint plus trap-entry same-address byte-store probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later breakpoint plus trap-entry same-address byte-store probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, committed_value);
    TEST_ASSERT("the older same-address scalar store commits the latest 8-byte line image before the later breakpoint trap-entry same-address byte-store probe starts",
        excpt.triggered == false
    );

    ai_micro_trap_entry_ebreak32();

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap before the same-address byte store runs",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-address byte store is the only architecturally visible writer on the target byte while preserving the rest of the latest committed target-line image after the breakpoint",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_bytes_match(target_phys, expected_final,
            sizeof(expected_final))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-address byte store returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_vector_load_sees_latest_16b_image_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x480U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x480U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0x41U, 0x42U, 0x43U, 0x44U, 0x45U, 0x46U, 0x47U, 0x48U,
        0x49U, 0x4aU, 0x4bU, 0x4cU, 0x4dU, 0x4eU, 0x4fU, 0x50U
    };
    const uint8_t committed_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES] = {
        0xa1U, 0xa2U, 0xa3U, 0xa4U, 0xa5U, 0xa6U, 0xa7U, 0xa8U,
        0xb1U, 0xb2U, 0xb3U, 0xb4U, 0xb5U, 0xb6U, 0xb7U, 0xb8U
    };
    const uint64_t pressure_first_seed = 0x0123456789abcdefULL;
    const uint64_t pressure_second_seed = 0xfedcba9876543210ULL;
    const uint64_t pressure_third_seed = 0x66778899aabbccddULL;
    const uint64_t pressure_fourth_seed = 0xddccbbaa99887766ULL;
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
    CSRW(stvec, ai_micro_trap_entry_same_addr_vector_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later breakpoint plus trap-entry same-address vector-load probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later breakpoint plus trap-entry same-address vector-load probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, committed_lo);
    sd(target_vaddr + sizeof(committed_lo), committed_hi);
    TEST_ASSERT("the older same-address pair of scalar stores commits the latest 16-byte target window image before the later breakpoint trap-entry same-address vector-load probe starts",
        excpt.triggered == false
    );

    ai_micro_trap_entry_ebreak32();

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap before the same-address vector load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-address vector load runs at handler head after the breakpoint and captures the latest committed 16-byte image across the target window",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_probe_bytes_match(committed_image,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES)
    );
    TEST_ASSERT("the probed 16-byte target window still matches the latest committed image after the breakpoint trap-entry same-address vector-load sequence",
        ai_micro_trap_entry_bytes_match(target_phys, committed_image,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-address vector load returns",
        fill_visible
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_flush_preserves_latest_committed_block_image_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t block_vaddr = hs_page_base(target_page) + 0x540U;
    const uintptr_t probe_vaddr = block_vaddr + 8U;
    const uintptr_t block_phys = phys_page_base(target_page) + 0x540U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t block_seed = 0x5a4b3c2d1e0f9081ULL;
    const uint64_t committed_word1 = 0x1122334455667788ULL;
    const uint64_t committed_word5 = 0x8877665544332211ULL;
    const uint64_t pressure_first_seed = 0x89abcdef01234567ULL;
    const uint64_t pressure_second_seed = 0x76543210fedcba98ULL;
    const uint64_t pressure_third_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t pressure_fourth_seed = 0x78695a4b3c2d1e0fULL;
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
    CSRW(stvec, ai_micro_trap_entry_same_block_cbo_flush_handler);

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(block_vaddr);
    TEST_ASSERT("the target cache block is readable before the later breakpoint plus trap-entry same-block cbo.flush probe sequence begins",
        excpt.triggered == false &&
        warmed == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later breakpoint plus trap-entry same-block cbo.flush probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later breakpoint plus trap-entry same-block cbo.flush probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(block_vaddr + 8U, committed_word1);
    sd(block_vaddr + 40U, committed_word5);
    TEST_ASSERT("the older same-block scalar stores commit the latest visible cache-block payload before the later breakpoint trap-entry same-block cbo.flush probe starts",
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

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap before the same-block cbo.flush runs",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-block cbo.flush runs at handler head on the target block instead of deadlocking behind the earlier translated-store pressure burst",
        ai_micro_trap_entry_probe_fired == 1ULL
    );
    TEST_ASSERT("the trap-entry same-block cbo.flush preserves the latest committed cache-block image rather than rolling back the older committed updates made before the breakpoint",
        read64(block_phys) == ai_micro_trap_entry_dense_store_pattern(block_seed, 0U) &&
        read64(block_phys + 8U) == committed_word1 &&
        read64(block_phys + 40U) == committed_word5 &&
        read64(block_phys + 56U) ==
            ai_micro_trap_entry_dense_store_pattern(block_seed, 7U)
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-block cbo.flush returns",
        fill_visible
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_byte_load_sees_latest_value_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x580U;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t target_phys = phys_page_base(target_page) + 0x580U;
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint8_t target_initial[16] = {
        0x51U, 0x52U, 0x53U, 0x54U, 0x55U, 0x56U, 0x57U, 0x58U,
        0x59U, 0x5aU, 0x5bU, 0x5cU, 0x5dU, 0x5eU, 0x5fU, 0x60U
    };
    uint8_t expected_final[16] = {0};
    const uint64_t committed_value = 0xdecafbad1234567eULL;
    const uint64_t expected_byte = committed_value & 0xffULL;
    const uint64_t pressure_first_seed = 0x1021324354657687ULL;
    const uint64_t pressure_second_seed = 0x7867564534231201ULL;
    const uint64_t pressure_third_seed = 0x11aa22bb33cc44ddULL;
    const uint64_t pressure_fourth_seed = 0xdd44cc33bb22aa11ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    bool fill_visible = false;

    memcpy(expected_final, target_initial, sizeof(expected_final));
    memcpy(expected_final, &committed_value, sizeof(committed_value));

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
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the later breakpoint plus trap-entry same-address byte-load probe and converges to the second seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the later breakpoint plus trap-entry same-address byte-load probe and converges to the fourth seed image",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, committed_value);
    TEST_ASSERT("the older same-address scalar store commits the latest line image before the later breakpoint trap-entry same-address byte-load probe starts",
        excpt.triggered == false
    );

    ai_micro_trap_entry_ebreak32();

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("after the approximate full-sbuffer older translated-store burst, the later HS ebreak is delegated to the case-local trap-entry handler as a precise breakpoint trap before the same-address byte load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_BKP
    );
    TEST_ASSERT("the case-local trap-entry same-address byte load runs at handler head after the breakpoint and captures the latest committed target byte",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_probe_value == expected_byte
    );
    TEST_ASSERT("the probed target window remains architecturally equal to the latest committed line image after the breakpoint trap-entry same-address byte-load sequence",
        ai_micro_trap_entry_bytes_match(target_phys, expected_final,
            sizeof(expected_final))
    );
    TEST_ASSERT("the two full-page dense translated-store pressure bursts remain architecturally visible across the later breakpoint trap before the trap-entry same-address byte load returns",
        fill_visible
    );

    TEST_END();
}
