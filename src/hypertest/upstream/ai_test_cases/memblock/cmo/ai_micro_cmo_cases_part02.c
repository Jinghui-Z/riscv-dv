#include "ai_micro_cmo_common.h"

#define AI_MICRO_CMO_HIGH_PPN_OVERFLOW_BIT (1ULL << 46)

static inline pte_t *ai_micro_cmo_gstage_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hpt[5][page];
#else
    return &hpt[2][page];
#endif
}

static inline void ai_micro_cmo_prepare_vs_cbo_env(uint64_t envcfg_bits)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, envcfg_bits);
    CSRS(CSR_HENVCFG, envcfg_bits);
    hspt_init();
    hpt_init();
    vspt_init();
    sfence_vma();
    hfence();
}

static inline void ai_micro_cmo_cleanup_vs_cbo_env(uint64_t envcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, envcfg_bits);
    CSRC(CSR_HENVCFG, envcfg_bits);
    CSRW(CSR_HGATP, 0ULL);
    CSRW(CSR_VSATP, 0ULL);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();
    hfence();
}

bool ai_micro_hs_back_to_back_cbo_zero_busy_sbuffer_no_stuck_or_wrong_line_zero_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t busy_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + 0x1c0U;
    const uintptr_t busy_store_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + 0x1c0U;
    const uintptr_t busy_adjacent_vaddr = busy_store_vaddr + sizeof(uint64_t);
    const uintptr_t busy_adjacent_paddr = busy_store_paddr + sizeof(uint64_t);
    const uintptr_t target0_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t target0_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t target1_base_vaddr = target0_base_vaddr + block_size;
    const uintptr_t target1_base_paddr = target0_base_paddr + block_size;
    const uintptr_t preserved_base_vaddr = target1_base_vaddr + block_size;
    const uintptr_t preserved_base_paddr = target1_base_paddr + block_size;
    const uintptr_t first_zero_vaddr = target0_base_vaddr + block_size - 1U;
    const uintptr_t second_zero_vaddr = target1_base_vaddr + 1U;
    const uint64_t busy_store_initial = 0x2233445566778899ULL;
    const uint64_t busy_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t busy_adjacent = 0x0fedcba987654321ULL;
    const uint64_t target0_seed = 0x1122334455667788ULL;
    const uint64_t target1_seed = 0x99aabbccddeeff00ULL;
    const uint64_t preserved_seed = 0x02468ace13579bdfULL;

    goto_priv(PRIV_M);
    write64(busy_store_paddr, busy_store_initial);
    write64(busy_adjacent_paddr, busy_adjacent);
    ai_micro_cmo_seed_block(target0_base_paddr, target0_seed);
    ai_micro_cmo_seed_block(target1_base_paddr, target1_seed);
    ai_micro_cmo_seed_block(preserved_base_paddr, preserved_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_busy_store = ld(busy_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable busy-store target is readable before the back-to-back cbo.zero plus busy-sbuffer probe runs",
        warmed_busy_store == busy_store_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_target0 = ld(target0_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first cbo.zero target block is readable before the back-to-back cbo.zero plus busy-sbuffer probe runs",
        warmed_target0 == target0_seed
    );

    TEST_SETUP_EXCEPT();
    sd(busy_store_vaddr, busy_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable busy store retires before the back-to-back cbo.zero sequence begins",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(first_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first cbo.zero retires while the sbuffer already holds an earlier cacheable store",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(second_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second cbo.zero to a different cache line also retires while the sbuffer already holds an earlier cacheable store",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the busy-store plus back-to-back cbo.zero sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_busy_store = ld(busy_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the busy-store plus back-to-back cbo.zero sequence, the cacheable busy-store target still reads back with its own stored payload",
        observed_busy_store == busy_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_busy_adjacent = ld(busy_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the word adjacent to the busy store remains unchanged across the busy-store plus back-to-back cbo.zero sequence",
        observed_busy_adjacent == busy_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target0 = ld(target0_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the busy-store plus back-to-back cbo.zero sequence, the first zero target reads back as zero",
        observed_target0 == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target1 = ld(target1_base_vaddr + block_size - sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the busy-store plus back-to-back cbo.zero sequence, the second zero target also reads back as zero at its tail word",
        observed_target1 == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_preserved = ld(preserved_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the two cbo.zero targets remains readable with its original payload across the busy-sbuffer probe",
        observed_preserved == preserved_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the busy-store plus back-to-back cbo.zero regression corner keeps the busy store updated, zeros both target blocks exactly once, and preserves the unrelated adjacent data",
        read64(busy_store_paddr) == busy_store_value &&
        read64(busy_adjacent_paddr) == busy_adjacent &&
        ai_micro_cmo_block_all_zero(target0_base_paddr) &&
        ai_micro_cmo_block_all_zero(target1_base_paddr) &&
        ai_micro_cmo_block_matches_seed(preserved_base_paddr, preserved_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_zero_delayed_sbuffer_fire_not_rebound_to_new_sq_head_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t busy_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t busy_store_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t zero_target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t zero_target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t preserved_base_vaddr = zero_target_base_vaddr + block_size;
    const uintptr_t preserved_base_paddr = zero_target_base_paddr + block_size;
    const uintptr_t head_advance_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 6U);
    const uintptr_t head_advance_store_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 6U);
    const uintptr_t zero_probe_vaddr = zero_target_base_vaddr + 24U;
    const uint64_t busy_store_initial = 0x0102030405060708ULL;
    const uint64_t busy_store_updated = 0x8899aabbccddeeffULL;
    const uint64_t zero_target_seed = 0x1122334455667788ULL;
    const uint64_t preserved_seed = 0x02468ace13579bdfULL;
    const uint64_t head_advance_initial = 0xfedcba9876543210ULL;
    const uint64_t head_advance_updated = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(busy_store_paddr, busy_store_initial);
    ai_micro_cmo_seed_block(zero_target_base_paddr, zero_target_seed);
    ai_micro_cmo_seed_block(preserved_base_paddr, preserved_seed);
    write64(head_advance_store_paddr, head_advance_initial);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_busy_store = ld(busy_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the busy store location is readable before the delayed-sbuffer-fire attribution probe runs",
        warmed_busy_store == busy_store_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_zero_target = ld(zero_target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cbo.zero target block is readable before the delayed-sbuffer-fire attribution probe runs",
        warmed_zero_target == zero_target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_preserved = ld(preserved_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the cbo.zero target is readable before the delayed-sbuffer-fire attribution probe runs",
        warmed_preserved == preserved_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_head_advance_store = ld(head_advance_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger store location that will advance sq head is readable before the delayed-sbuffer-fire attribution probe runs",
        warmed_head_advance_store == head_advance_initial
    );

    TEST_SETUP_EXCEPT();
    sd(busy_store_vaddr, busy_store_updated);
    AI_ASSERT_NO_EXCEPT_AND("the older busy store retires before the cbo.zero request in the delayed-sbuffer-fire attribution corner",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cbo.zero request retires without exception before the younger sq-head-advancing store in the delayed-sbuffer-fire attribution corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(head_advance_store_vaddr, head_advance_updated);
    AI_ASSERT_NO_EXCEPT_AND("the younger store immediately after the cbo.zero also retires without exception while the earlier cbo.zero may already be heading into sbuffer",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the delayed-sbuffer-fire attribution sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_busy_store = ld(busy_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the delayed-sbuffer-fire attribution sequence, the older busy store payload remains visible",
        observed_busy_store == busy_store_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_zero_target_head = ld(zero_target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the delayed-sbuffer-fire attribution sequence, the cbo.zero target head word reads back as zero",
        observed_zero_target_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_zero_target_tail = ld(zero_target_base_vaddr + block_size - sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the delayed-sbuffer-fire attribution sequence, the cbo.zero target tail word also reads back as zero",
        observed_zero_target_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_preserved = ld(preserved_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the cbo.zero target remains unchanged across the delayed-sbuffer-fire attribution sequence",
        observed_preserved == preserved_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_head_advance_store = ld(head_advance_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger sq-head-advancing store still commits its own payload after the delayed-sbuffer-fire attribution sequence",
        observed_head_advance_store == head_advance_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the delayed cbo.zero sbuffer-fire attribution corner still flushes only the real cbo.zero target while preserving the younger sq-head-advancing store and unrelated adjacent data in backing memory",
        read64(busy_store_paddr) == busy_store_updated &&
        ai_micro_cmo_block_all_zero(zero_target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(preserved_base_paddr, preserved_seed) &&
        read64(head_advance_store_paddr) == head_advance_updated
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_sq_head_advance_after_cbo_zero_fire_no_false_or_missed_flush_trigger_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t leading_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t leading_store_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t first_zero_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t first_zero_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t head_advance_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t head_advance_store_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t second_zero_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 5U);
    const uintptr_t second_zero_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 5U);
    const uintptr_t preserved_base_vaddr = second_zero_base_vaddr + block_size;
    const uintptr_t preserved_base_paddr = second_zero_base_paddr + block_size;
    const uintptr_t trailing_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 8U);
    const uintptr_t trailing_store_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 8U);
    const uintptr_t first_zero_probe_vaddr = first_zero_base_vaddr + 8U;
    const uintptr_t second_zero_probe_vaddr = second_zero_base_vaddr + block_size - 1U;
    const uint64_t leading_store_initial = 0x2233445566778899ULL;
    const uint64_t leading_store_updated = 0x76543210fedcba98ULL;
    const uint64_t first_zero_seed = 0x0123456789abcdefULL;
    const uint64_t head_advance_initial = 0x13579bdf2468ace0ULL;
    const uint64_t head_advance_updated = 0x8877665544332211ULL;
    const uint64_t second_zero_seed = 0x99aabbccddeeff00ULL;
    const uint64_t preserved_seed = 0x0fedcba987654321ULL;
    const uint64_t trailing_store_initial = 0x1111222233334444ULL;
    const uint64_t trailing_store_updated = 0xcafebabedeadbeefULL;

    goto_priv(PRIV_M);
    write64(leading_store_paddr, leading_store_initial);
    ai_micro_cmo_seed_block(first_zero_base_paddr, first_zero_seed);
    write64(head_advance_store_paddr, head_advance_initial);
    ai_micro_cmo_seed_block(second_zero_base_paddr, second_zero_seed);
    ai_micro_cmo_seed_block(preserved_base_paddr, preserved_seed);
    write64(trailing_store_paddr, trailing_store_initial);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_leading_store = ld(leading_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the leading busy store location is readable before the sq-head-advance-after-cbo.zero-fire probe runs",
        warmed_leading_store == leading_store_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_first_zero = ld(first_zero_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first cbo.zero target block is readable before the sq-head-advance-after-cbo.zero-fire probe runs",
        warmed_first_zero == first_zero_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_head_advance_store = ld(head_advance_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger store location between the two cbo.zero requests is readable before the sq-head-advance-after-cbo.zero-fire probe runs",
        warmed_head_advance_store == head_advance_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_second_zero = ld(second_zero_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second cbo.zero target block is readable before the sq-head-advance-after-cbo.zero-fire probe runs",
        warmed_second_zero == second_zero_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_preserved = ld(preserved_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the later cbo.zero target is readable before the sq-head-advance-after-cbo.zero-fire probe runs",
        warmed_preserved == preserved_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_trailing_store = ld(trailing_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the trailing store location is readable before the sq-head-advance-after-cbo.zero-fire probe runs",
        warmed_trailing_store == trailing_store_initial
    );

    TEST_SETUP_EXCEPT();
    sd(leading_store_vaddr, leading_store_updated);
    AI_ASSERT_NO_EXCEPT_AND("the leading busy store retires before the first cbo.zero request in the sq-head-advance-after-cbo.zero-fire corner",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(first_zero_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first cbo.zero retires without exception before sq head advances to the intervening store",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(head_advance_store_vaddr, head_advance_updated);
    AI_ASSERT_NO_EXCEPT_AND("the intervening younger store retires without exception while the earlier cbo.zero may already be firing into sbuffer",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(second_zero_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later cbo.zero also retires without exception after sq head advances past the intervening store",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(trailing_store_vaddr, trailing_store_updated);
    AI_ASSERT_NO_EXCEPT_AND("the trailing store after the two cbo.zero requests retires without exception in the sq-head-advance-after-cbo.zero-fire corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the busy-store plus cbo.zero plus store plus cbo.zero sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_leading_store = ld(leading_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the sq-head-advance-after-cbo.zero-fire sequence, the leading busy store payload remains visible",
        observed_leading_store == leading_store_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_zero_head = ld(first_zero_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the sq-head-advance-after-cbo.zero-fire sequence, the first cbo.zero target head word reads back as zero",
        observed_first_zero_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_head_advance_store = ld(head_advance_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the sq-head-advance-after-cbo.zero-fire sequence, the intervening sq-head-advancing store still returns its own payload",
        observed_head_advance_store == head_advance_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_zero_tail = ld(second_zero_base_vaddr + block_size - sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the sq-head-advance-after-cbo.zero-fire sequence, the second cbo.zero target tail word also reads back as zero",
        observed_second_zero_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_preserved = ld(preserved_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the later cbo.zero target remains unchanged across the sq-head-advance-after-cbo.zero-fire sequence",
        observed_preserved == preserved_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_trailing_store = ld(trailing_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the trailing store after the two cbo.zero requests also commits its own payload in the sq-head-advance-after-cbo.zero-fire corner",
        observed_trailing_store == trailing_store_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("advancing sq head immediately after a cbo.zero fires into sbuffer still flushes each real cbo.zero exactly once while preserving the intervening and trailing stores plus adjacent non-target data in backing memory",
        read64(leading_store_paddr) == leading_store_updated &&
        ai_micro_cmo_block_all_zero(first_zero_base_paddr) &&
        read64(head_advance_store_paddr) == head_advance_updated &&
        ai_micro_cmo_block_all_zero(second_zero_base_paddr) &&
        ai_micro_cmo_block_matches_seed(preserved_base_paddr, preserved_seed) &&
        read64(trailing_store_paddr) == trailing_store_updated
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_tlb_access_fault()
{
    TEST_START();

    const uintptr_t block_size =
        AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t block_base_vaddr =
        hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t vaddr = block_base_vaddr + block_size - 1U;

    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    TEST_ASSERT("the AP-131 cbo.zero high-PPN tval probe uses a tail rs1 address distinct from the internal cache-block base",
        vaddr != block_base_vaddr
    );

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(vaddr);
    TEST_ASSERT("an HS cbo.zero reports store access fault with tval equal to tail-offset rs1, not the cache-block base, when the upper-level PTE encoding overflows during translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.tval != block_base_vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(vaddr);
    TEST_ASSERT("an HS cbo.zero also reports store access fault with tval equal to tail-offset rs1, not the cache-block base, when the leaf PTE encoding overflows during translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr &&
        excpt.tval != block_base_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_vs_gstage_cbozero_high_ppn_tval_rs1_manual_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t block_size =
        AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t block_base_vaddr = vs_page_base(page) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t block_base_paddr = phys_page_base(page) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t adjacent_base_vaddr = block_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = block_base_paddr + block_size;
    const uintptr_t tail_rs1_vaddr = block_base_vaddr + block_size - 1U;
    const uint64_t target_seed = 0x61703133312d6362ULL;
    const uint64_t adjacent_seed = 0x61703133312d6164ULL;
    const uint64_t overflow_leaf =
        (phys_page_base(page) >> 2) |
        PTE_V | PTE_U | PTE_AD | PTE_RWX |
        AI_MICRO_CMO_HIGH_PPN_OVERFLOW_BIT;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(block_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_vs_cbo_env(MENVCFG_CBZE);
    *ai_micro_cmo_gstage_leaf_entry(page) = overflow_leaf;
    sfence_vma();
    hfence();

    TEST_ASSERT("the AP-131 VS/G-stage cbo.zero high-PPN probe uses a tail rs1 address distinct from the internal cache-block base",
        tail_rs1_vaddr != block_base_vaddr
    );

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    cbo_zero(tail_rs1_vaddr);
    TEST_ASSERT("a VS cbo.zero whose G-stage leaf has unsupported high PPN bits reports store access fault with stval equal to original tail-offset rs1, not the cache-block base",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SAF &&
        excpt.cause != CAUSE_SGPF &&
        excpt.tval == tail_rs1_vaddr &&
        excpt.tval != block_base_vaddr,
        "cause=0x%llx priv=%u tval=0x%llx rs1=0x%llx block_base=0x%llx tval2=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned)excpt.priv,
        (unsigned long long)excpt.tval,
        (unsigned long long)tail_rs1_vaddr,
        (unsigned long long)block_base_vaddr,
        (unsigned long long)excpt.tval2
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the VS/G-stage CBO.ZERO high-PPN access fault leaves the G-stage leaf and both backing cache blocks unchanged",
        *ai_micro_cmo_gstage_leaf_entry(page) == overflow_leaf &&
        ai_micro_cmo_block_matches_seed(block_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );

    *ai_micro_cmo_gstage_leaf_entry(page) =
        (phys_page_base(page) >> 2) | PTE_V | PTE_U | PTE_AD | PTE_RWX;
    sfence_vma();
    hfence();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    cbo_zero(tail_rs1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after repairing the G-stage high PPN bit, the same VS tail-offset cbo.zero retires normally",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the repaired VS cbo.zero retires without exception",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repairing the G-stage leaf, the VS tail-offset cbo.zero clears exactly the target block and preserves the adjacent block",
        ai_micro_cmo_block_all_zero(block_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_cleanup_vs_cbo_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_tlb_access_fault_then_valid_zero_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t recovery_vaddr = target_base_vaddr + block_size - 1U;
    const uint64_t target_seed = 0x1122334455667788ULL;
    const uint64_t adjacent_seed = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(target_base_vaddr);
    TEST_ASSERT("an upper-level-overflow HS cbo.zero reports store access fault before the later valid zeroing probe in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_base_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the valid cbo.zero target block and its adjacent block remain unchanged after the earlier TLB access-faulting zero probe",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid tail-offset cbo.zero retires normally after translation is restored in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the recovered cbo.zero retires without exception in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zeroed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered cbo.zero target reads back as zero after the earlier TLB access fault",
        zeroed_target == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered cbo.zero target remains readable with its original payload after translation is restored",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cbo.zero clears only the target block and preserves the adjacent backing-memory block after the earlier TLB access fault",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_repeated_tlb_access_fault_stable_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t second_probe_vaddr = target_base_vaddr + 1U;
    const uint64_t target_seed = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent_seed = 0x02468ace13579bdfULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(target_base_vaddr);
    TEST_ASSERT("the first leaf-overflow HS cbo.zero reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_base_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(second_probe_vaddr);
    TEST_ASSERT("the second leaf-overflow HS cbo.zero also reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_probe_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated TLB access-faulting cbo.zero probes leave both the target block and its adjacent block unchanged before translation is restored",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(second_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid subblock-offset cbo.zero still retires normally after repeated TLB access faults in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the recovered cbo.zero retires without exception in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zeroed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered cbo.zero target reads back as zero after repeated TLB access faults",
        zeroed_target == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered cbo.zero target remains readable with its original payload after repeated TLB access faults",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated TLB access-faulting cbo.zero probes stay stable and the later valid cbo.zero still clears only the target backing-memory block",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_pmp_write_access_fault()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE);
    const uint64_t initial = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        write64(paddr + AI_MICRO_CMO_BLOCK_OFFSET + (i * sizeof(uint64_t)),
            initial + i);
    }
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);
    ai_micro_cmo_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(vaddr);
    TEST_ASSERT("an HS cbo.zero reports store access fault when PMP denies writes to the targeted cache block",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    bool unchanged = true;
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        unchanged = unchanged &&
            (read64(paddr + AI_MICRO_CMO_BLOCK_OFFSET + (i * sizeof(uint64_t))) ==
                (initial + i));
    }
    TEST_ASSERT("the failed cbo.zero probe leaves the whole cache block unchanged when PMP denies writes",
        unchanged
    );
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_pmp_write_access_fault_then_restore_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t recovery_vaddr = target_base_vaddr + 1U;
    const uintptr_t deny_page_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE);
    const uint64_t target_seed = 0x0badf00d12345678ULL;
    const uint64_t adjacent_seed = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);
    ai_micro_cmo_configure_pmp_window(
        deny_page_paddr, deny_page_paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(target_base_vaddr);
    TEST_ASSERT("a denied HS cbo.zero reports store access fault before the same page is re-enabled in the adjacent-block-preservation recovery corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_base_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied cbo.zero leaves both the target block and its adjacent block unchanged before PMP write permissions are restored",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a subblock-offset cbo.zero on the same cache block retires normally after PMP write permissions are restored in the adjacent-block-preservation recovery corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the recovered cbo.zero retires without exception in the adjacent-block-preservation recovery corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zeroed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered cbo.zero target reads back as zero after the earlier denied probe",
        zeroed_target == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered cbo.zero target remains readable with its original payload after PMP write permissions are restored",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cbo.zero clears only the target block and preserves the adjacent backing-memory block after the earlier PMP write denial",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_repeated_pmp_write_access_fault_stable_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t second_probe_vaddr = target_base_vaddr + 1U;
    const uintptr_t deny_page_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE);
    const uint64_t target_seed = 0x0badf00d12345678ULL;
    const uint64_t adjacent_seed = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);
    ai_micro_cmo_configure_pmp_window(
        deny_page_paddr, deny_page_paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(target_base_vaddr);
    TEST_ASSERT("the first denied HS cbo.zero reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_base_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(second_probe_vaddr);
    TEST_ASSERT("the second denied HS cbo.zero also reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_probe_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PMP-denied cbo.zero probes leave both the target block and its adjacent block unchanged before permissions are restored",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(second_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid subblock-offset cbo.zero still retires normally after repeated PMP write denials in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the recovered cbo.zero retires without exception in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zeroed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered cbo.zero target reads back as zero after repeated PMP write denials",
        zeroed_target == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered cbo.zero target remains readable with its original payload after repeated PMP write denials",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PMP-denied cbo.zero probes stay stable and the later valid cbo.zero still clears only the target backing-memory block",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_mmio_access_fault()
{
    TEST_START();

    const uintptr_t block_base_paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uint64_t initial_seed = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(block_base_paddr, initial_seed);
    uintptr_t vaddr = ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(vaddr);
    TEST_ASSERT("an HS cbo.zero reports store access fault when PMA classifies the translated target as Device/MMIO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the failed PMA=Device cbo.zero probe leaves the whole backing cache block unchanged",
        ai_micro_cmo_block_matches_seed(block_base_paddr, initial_seed)
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_mmio_access_fault_then_valid_zero_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t bad_base_paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_adjacent_paddr = bad_base_paddr + block_size;
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t recovery_vaddr = target_base_vaddr + block_size - 1U;
    const uint64_t bad_seed = 0x0123456789abcdefULL;
    const uint64_t bad_adjacent_seed = 0xfedcba9876543210ULL;
    const uint64_t target_seed = 0x5566778899aabbccULL;
    const uint64_t adjacent_seed = 0x0badc0de12345678ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(bad_base_paddr, bad_seed);
    ai_micro_cmo_seed_block(bad_adjacent_paddr, bad_adjacent_seed);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    uintptr_t bad_vaddr = ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(bad_vaddr);
    TEST_ASSERT("an HS cbo.zero reports store access fault on a PMA=Device alias before the later valid recovery zero in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMA=Device cbo.zero leaves both the faulting block and its adjacent block unchanged before the later valid recovery zero",
        ai_micro_cmo_block_matches_seed(bad_base_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    TEST_ASSERT("the separate valid cbo.zero target block and its adjacent block also keep their seeded payloads after the earlier PMA=Device fault",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid tail-offset cbo.zero still retires normally after the earlier PMA=Device cbo.zero fault in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the recovered cbo.zero retires without exception in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zeroed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered cbo.zero target reads back as zero after the earlier PMA=Device fault",
        zeroed_target == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered cbo.zero target remains readable with its original payload after the earlier PMA=Device fault",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the later valid cbo.zero clears only the recovery block and preserves its adjacent backing-memory block after the earlier PMA=Device fault",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    TEST_ASSERT("the earlier PMA=Device cbo.zero fault also leaves the faulting backing block pair unchanged after the later valid recovery zero",
        ai_micro_cmo_block_matches_seed(bad_base_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_repeated_mmio_access_fault_stable_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t bad_base_paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_adjacent_paddr = bad_base_paddr + block_size;
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t recovery_vaddr = target_base_vaddr + 1U;
    const uint64_t bad_seed = 0x3141592653589793ULL;
    const uint64_t bad_adjacent_seed = 0x2718281828459045ULL;
    const uint64_t target_seed = 0x2468ace013579bdfULL;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(bad_base_paddr, bad_seed);
    ai_micro_cmo_seed_block(bad_adjacent_paddr, bad_adjacent_seed);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    uintptr_t bad_vaddr = ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZE);
    const uintptr_t second_bad_vaddr = bad_vaddr + 1U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(bad_vaddr);
    TEST_ASSERT("the first PMA=Device HS cbo.zero reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(second_bad_vaddr);
    TEST_ASSERT("the second PMA=Device HS cbo.zero also reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PMA=Device cbo.zero probes leave both the faulting block and its adjacent block unchanged before the later valid recovery zero",
        ai_micro_cmo_block_matches_seed(bad_base_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    TEST_ASSERT("the separate valid cbo.zero target block and its adjacent block also remain unchanged after repeated PMA=Device faults",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid subblock-offset cbo.zero still retires normally after repeated PMA=Device faults in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the recovered cbo.zero retires without exception in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zeroed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered cbo.zero target reads back as zero after repeated PMA=Device faults",
        zeroed_target == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered cbo.zero target remains readable with its original payload after repeated PMA=Device faults",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PMA=Device cbo.zero probes stay stable and the later valid cbo.zero still clears only the recovery backing-memory block",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    TEST_ASSERT("the faulting PMA=Device backing block pair remains unchanged even after the later valid recovery zero",
        ai_micro_cmo_block_matches_seed(bad_base_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbom_storequeue_head_data_visible()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uint64_t initial_head = 0x0123456789abcdefULL;
    const uint64_t initial_next = 0xfedcba9876543210ULL;
    const uint64_t initial_tail = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t updated_head = 0x1122334455667788ULL;
    const uint64_t updated_next = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_head);
    write64(paddr + sizeof(uint64_t), initial_next);
    write64(paddr + (2U * sizeof(uint64_t)), initial_tail);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_head = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the CBOM target block is readable before issuing the storequeue-head maintenance sequence",
        warmed_head == initial_head
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_next = ld(vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("the second word in the same cache block is also readable before the CBOM sequence",
        warmed_next == initial_next
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, updated_head);
    AI_ASSERT_NO_EXCEPT_AND("the first Store before the CBOM head sequence completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr + sizeof(uint64_t), updated_next);
    AI_ASSERT_NO_EXCEPT_AND("the second Store in the same cache block also completes without exception before CBOM",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS CBOM request retires without exception after the prior Stores reach the head of the storequeue",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded_head = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the block after cbo.flush returns the first payload written before the CBOM request",
        reloaded_head == updated_head
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded_next = ld(vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("reloading the adjacent word after cbo.flush returns the second payload written before the CBOM request",
        reloaded_next == updated_next
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the CBOM head sequence leaves both prior Store payloads in backing memory while keeping untouched words intact",
        read64(paddr) == updated_head &&
        read64(paddr + sizeof(uint64_t)) == updated_next &&
        read64(paddr + (2U * sizeof(uint64_t))) == initial_tail
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_storequeue_head_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t adjacent_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t probe_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t)) - 1U;
    const uint64_t target_seed = 0x0123456789abcdefULL;
    const uint64_t adjacent_seed = 0xfedcba9876543210ULL;
    const uint64_t updated_head = 0x1122334455667788ULL;
    const uint64_t updated_next = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the CBOM storequeue-head target block is readable before the adjacent-block-preservation sequence",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the CBOM storequeue-head target is also readable before the adjacent-block-preservation sequence",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    sd(target_base_vaddr, updated_head);
    AI_ASSERT_NO_EXCEPT_AND("the first Store before the CBOM storequeue-head adjacent-block-preservation sequence completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(target_base_vaddr + sizeof(uint64_t), updated_next);
    AI_ASSERT_NO_EXCEPT_AND("the second Store before the CBOM storequeue-head adjacent-block-preservation sequence also completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS CBOM request retires without exception in the storequeue-head adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the target block after the CBOM storequeue-head sequence returns the updated first payload",
        reloaded_head == updated_head
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded_next = ld(target_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("reloading the second target word after the CBOM storequeue-head sequence returns the updated second payload",
        reloaded_next == updated_next
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the CBOM storequeue-head target remains readable with its original payload",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the CBOM storequeue-head sequence leaves both prior Store payloads in the target backing-memory block while preserving the adjacent backing-memory block",
        read64(target_base_paddr) == updated_head &&
        read64(target_base_paddr + sizeof(uint64_t)) == updated_next &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_storequeue_cbom_sqdeq_not_corrupted_by_sbuffer_overlap_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t first_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t first_store_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t first_target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t first_target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t middle_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t middle_store_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t second_target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 5U);
    const uintptr_t second_target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 5U);
    const uintptr_t last_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 7U);
    const uintptr_t last_store_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 7U);
    const uintptr_t first_probe_vaddr = first_target_base_vaddr + block_bytes - 1U;
    const uintptr_t second_probe_vaddr = second_target_base_vaddr + 24U;
    const uint64_t first_store_initial = 0x0102030405060708ULL;
    const uint64_t first_store_updated = 0x8877665544332211ULL;
    const uint64_t first_target_seed = 0x0123456789abcdefULL;
    const uint64_t first_target_updated = 0x1122334455667788ULL;
    const uint64_t first_target_next = first_target_seed + 0x1111111111111111ULL;
    const uint64_t middle_store_initial = 0x89abcdef01234567ULL;
    const uint64_t middle_store_updated = 0x76543210fedcba98ULL;
    const uint64_t second_target_seed = 0x13579bdf2468ace0ULL;
    const uint64_t second_target_next_updated = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_target_head = second_target_seed;
    const uint64_t last_store_initial = 0xfedcba9876543210ULL;
    const uint64_t last_store_updated = 0x02468ace13579bdfULL;

    goto_priv(PRIV_M);
    write64(first_store_paddr, first_store_initial);
    ai_micro_cmo_seed_block(first_target_base_paddr, first_target_seed);
    write64(middle_store_paddr, middle_store_initial);
    ai_micro_cmo_seed_block(second_target_base_paddr, second_target_seed);
    write64(last_store_paddr, last_store_initial);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_first_store = ld(first_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first cacheable Store location is readable before the repeated CBOM overlap window",
        warmed_first_store == first_store_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_first_target = ld(first_target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first CBOM target block is readable before the repeated overlap window",
        warmed_first_target == first_target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_middle_store = ld(middle_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the middle cacheable Store location is readable before the repeated CBOM overlap window",
        warmed_middle_store == middle_store_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_second_target = ld(second_target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second CBOM target block is readable before the repeated overlap window",
        warmed_second_target == second_target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_last_store = ld(last_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the last cacheable Store location is readable before the repeated CBOM overlap window",
        warmed_last_store == last_store_initial
    );

    TEST_SETUP_EXCEPT();
    sd(first_store_vaddr, first_store_updated);
    AI_ASSERT_NO_EXCEPT_AND("the first cacheable Store retires before the first CBOM overlap window without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(first_target_base_vaddr, first_target_updated);
    AI_ASSERT_NO_EXCEPT_AND("the Store feeding the first CBOM target retires without exception before the overlap dequeue window",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(first_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first CBOM request retires without exception in the repeated sbuffer-overlap corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(middle_store_vaddr, middle_store_updated);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable Store immediately after the first CBOM request also retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(second_target_base_vaddr + sizeof(uint64_t), second_target_next_updated);
    AI_ASSERT_NO_EXCEPT_AND("the Store feeding the second CBOM target retires without exception before the second overlap dequeue window",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(second_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second CBOM request also retires without exception in the repeated sbuffer-overlap corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(last_store_vaddr, last_store_updated);
    AI_ASSERT_NO_EXCEPT_AND("the trailing cacheable Store after the repeated CBOM overlap window retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the repeated Store-plus-CBOM overlap windows retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_store = ld(first_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated Store-plus-CBOM overlap windows, the first cacheable Store payload is still visible",
        observed_first_store == first_store_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_target = ld(first_target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated overlap windows, the first CBOM target still returns the payload written before its flush",
        observed_first_target == first_target_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_target_next = ld(first_target_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the repeated overlap windows, the untouched adjacent word in the first CBOM target block remains unchanged",
        observed_first_target_next == first_target_next
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_middle_store = ld(middle_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated overlap windows, the middle cacheable Store payload is still visible",
        observed_middle_store == middle_store_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_target_head = ld(second_target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated overlap windows, the untouched head word in the second CBOM target block remains unchanged",
        observed_second_target_head == second_target_head
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_target_next = ld(second_target_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the repeated overlap windows, the second CBOM target still returns the payload written before its flush",
        observed_second_target_next == second_target_next_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_last_store = ld(last_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated overlap windows, the trailing cacheable Store payload is still visible",
        observed_last_store == last_store_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated Store-plus-CBOM overlap windows keep sqDeq and commit accounting stable so every Store commits once and untouched CBOM target words survive in backing memory",
        read64(first_store_paddr) == first_store_updated &&
        read64(first_target_base_paddr) == first_target_updated &&
        read64(first_target_base_paddr + sizeof(uint64_t)) == first_target_next &&
        read64(middle_store_paddr) == middle_store_updated &&
        read64(second_target_base_paddr) == second_target_head &&
        read64(second_target_base_paddr + sizeof(uint64_t)) == second_target_next_updated &&
        read64(last_store_paddr) == last_store_updated
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_inflight_then_new_store_ordered_correctly_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 6U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 6U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_bytes;
    const uintptr_t cbom_vaddr = target_base_vaddr + 24U;
    const uint64_t target_seed = 0x0102030405060708ULL;
    const uint64_t adjacent_seed = 0x8899aabbccddeeffULL;
    const uint64_t updated_head = 0x1122334455667788ULL;
    const uint64_t updated_middle = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the same-block inflight-then-new-store ordering corner",
        warmed_head == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the same-block inflight target is also readable before the ordering corner",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    sd(target_base_vaddr, updated_head);
    AI_ASSERT_NO_EXCEPT_AND("the first store to the target cache block retires before the same-block CBOM inflight window begins",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(cbom_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same-block CBOM probe retires without exception after the first store in the inflight-ordering corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(target_base_vaddr + (2U * sizeof(uint64_t)), updated_middle);
    AI_ASSERT_NO_EXCEPT_AND("a younger store to the same cache block still retires without exception while the earlier block may already be inflight",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the same-block inflight-plus-new-store sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the same-block inflight-plus-new-store sequence, the target head word reads back the first stored payload",
        observed_head == updated_head
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_middle = ld(target_base_vaddr + (2U * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("after the same-block inflight-plus-new-store sequence, the later same-block store payload is also visible",
        observed_middle == updated_middle
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the same-block inflight target remains unchanged after the sequence",
        observed_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-block inflight-plus-new-store ordering corner keeps both stores in the target backing block while preserving the adjacent backing block",
        read64(target_base_paddr) == updated_head &&
        read64(target_base_paddr + sizeof(uint64_t)) ==
            (target_seed + 0x1111111111111111ULL) &&
        read64(target_base_paddr + (2U * sizeof(uint64_t))) == updated_middle &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_inflight_mask_clears_without_stuck_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 7U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 7U);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 9U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 9U);
    const uintptr_t cbom_head_vaddr = target_base_vaddr + 8U;
    const uintptr_t cbom_tail_vaddr = target_base_vaddr + block_bytes - 1U;
    const uint64_t target_seed = 0x13579bdf2468ace0ULL;
    const uint64_t recovery_seed = 0x02468ace13579bdfULL;
    const uint64_t updated_head = 0x8877665544332211ULL;
    const uint64_t updated_tail = 0x76543210fedcba98ULL;
    const uint64_t recovery_value = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    write64(recovery_paddr, recovery_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the repeated same-block inflight-mask-clear corner",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later independent recovery location is also readable before the repeated same-block inflight-mask-clear corner",
        warmed_recovery == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    sd(target_base_vaddr, updated_head);
    AI_ASSERT_NO_EXCEPT_AND("the first store to the target cache block retires before the first same-block CBOM probe in the inflight-mask-clear corner",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(cbom_head_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first same-block CBOM probe retires without exception in the inflight-mask-clear corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(target_base_vaddr + (3U * sizeof(uint64_t)), updated_tail);
    AI_ASSERT_NO_EXCEPT_AND("a second store to the same cache block retires without exception after the first same-block CBOM probe",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(cbom_tail_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second same-block CBOM probe also retires without exception in the inflight-mask-clear corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_value);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated same-block CBOM probes, a later independent cacheable store still retires normally without stuck state",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the repeated same-block inflight-mask-clear sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated same-block inflight-mask-clear sequence, the first stored target word remains visible",
        observed_head == updated_head
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_tail = ld(target_base_vaddr + (3U * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("after the repeated same-block inflight-mask-clear sequence, the later same-block stored word also remains visible",
        observed_tail == updated_tail
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated same-block inflight-mask-clear sequence, the later independent store payload is also visible so forward progress is preserved",
        observed_recovery == recovery_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated same-block inflight-mask-clear corner preserves forward progress: both same-block stores persist and the later independent store is not blocked or lost",
        read64(target_base_paddr) == updated_head &&
        read64(target_base_paddr + sizeof(uint64_t)) ==
            (target_seed + 0x1111111111111111ULL) &&
        read64(target_base_paddr + (2U * sizeof(uint64_t))) ==
            (target_seed + (2U * 0x1111111111111111ULL)) &&
        read64(target_base_paddr + (3U * sizeof(uint64_t))) == updated_tail &&
        read64(recovery_paddr) == recovery_value
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_sbuffer_sameblock_inflight_replacement_no_hotline_starvation_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t hot_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t hot_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = hot_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = hot_base_paddr + block_bytes;
    const uintptr_t hot_cbom_vaddr = hot_base_vaddr + 24U;
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 6U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 7U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 8U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 9U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 10U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 11U),
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 6U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 7U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 8U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 9U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 10U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 11U),
    };
    const uint64_t pressure_initials[] = {
        0x0101010101010101ULL,
        0x1212121212121212ULL,
        0x2323232323232323ULL,
        0x3434343434343434ULL,
        0x4545454545454545ULL,
        0x5656565656565656ULL,
    };
    const uint64_t pressure_updates[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
        0xd0d1d2d3d4d5d6d7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xf0f1f2f3f4f5f6f7ULL,
    };
    const size_t pressure_count = sizeof(pressure_vaddrs) / sizeof(pressure_vaddrs[0]);
    const uint64_t hot_seed = 0x1122334455667788ULL;
    const uint64_t adjacent_seed = 0x99aabbccddeeff00ULL;
    const uint64_t hot_head_update = 0x8877665544332211ULL;
    const uint64_t hot_middle_update = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(hot_base_paddr, hot_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    for (size_t i = 0; i < pressure_count; ++i) {
        write64(pressure_paddrs[i], pressure_initials[i]);
    }
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_hot = ld(hot_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the hot target block is readable before the same-block-inflight replacement-pressure corner runs",
        warmed_hot == hot_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the hot target is readable before the same-block-inflight replacement-pressure corner runs",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    sd(hot_base_vaddr, hot_head_update);
    AI_ASSERT_NO_EXCEPT_AND("the first hot-block store retires before the same-block CBOM request in the replacement-pressure corner",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(hot_cbom_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same-block CBOM request retires without exception before replacement pressure is applied",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = 0; i < pressure_count; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the burst of unrelated pressure stores all retire without exception while the hot block may still be marked same-block inflight",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(hot_base_vaddr + (2U * sizeof(uint64_t)), hot_middle_update);
    AI_ASSERT_NO_EXCEPT_AND("a later store back into the hot block still retires without exception after replacement pressure",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the same-block-inflight replacement-pressure sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_hot_head = ld(hot_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after replacement pressure, the hot block head word still returns the first hot-store payload",
        observed_hot_head == hot_head_update
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_hot_middle = ld(hot_base_vaddr + (2U * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("after replacement pressure, the later hot-block word still returns the newest payload instead of an evicted stale value",
        observed_hot_middle == hot_middle_update
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the hot target remains unchanged across replacement pressure",
        observed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_visible_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_visible_ok = pressure_visible_ok &&
            (ld(pressure_vaddrs[i]) == pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("every unrelated pressure store remains architecturally visible after the same-block-inflight replacement-pressure sequence",
        pressure_visible_ok
    );

    goto_priv(PRIV_M);
    bool pressure_backing_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_backing_ok = pressure_backing_ok &&
            (read64(pressure_paddrs[i]) == pressure_updates[i]);
    }
    TEST_ASSERT("same-block inflight replacement pressure still makes forward progress: the hot block keeps its newest image and unrelated pressure stores are not starved or lost in backing memory",
        read64(hot_base_paddr) == hot_head_update &&
        read64(hot_base_paddr + sizeof(uint64_t)) ==
            (hot_seed + 0x1111111111111111ULL) &&
        read64(hot_base_paddr + (2U * sizeof(uint64_t))) == hot_middle_update &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed) &&
        pressure_backing_ok
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_sbuffer_sameblock_pressure_no_wrong_victim_or_evict_oscillation_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t hot_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 12U);
    const uintptr_t hot_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 12U);
    const uintptr_t adjacent_base_vaddr = hot_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = hot_base_paddr + block_bytes;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 15U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 15U);
    const uintptr_t cbom_head_vaddr = hot_base_vaddr + 8U;
    const uintptr_t cbom_tail_vaddr = hot_base_vaddr + block_bytes - 1U;
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 2U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 3U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 5U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 6U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 8U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 9U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 10U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 11U),
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 2U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 3U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 5U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 6U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 8U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 9U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 10U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 11U),
    };
    const uint64_t pressure_initials[] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
        0x3132333435363738ULL,
        0x4142434445464748ULL,
        0x5152535455565758ULL,
        0x6162636465666768ULL,
        0x7172737475767778ULL,
    };
    const uint64_t pressure_updates[] = {
        0x8182838485868788ULL,
        0x9192939495969798ULL,
        0xa1a2a3a4a5a6a7a8ULL,
        0xb1b2b3b4b5b6b7b8ULL,
        0xc1c2c3c4c5c6c7c8ULL,
        0xd1d2d3d4d5d6d7d8ULL,
        0xe1e2e3e4e5e6e7e8ULL,
        0xf1f2f3f4f5f6f7f8ULL,
    };
    const size_t pressure_count = sizeof(pressure_vaddrs) / sizeof(pressure_vaddrs[0]);
    const uint64_t hot_seed = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent_seed = 0x02468ace13579bdfULL;
    const uint64_t recovery_seed = 0x5566778899aabbccULL;
    const uint64_t hot_head_update = 0x8877665544332211ULL;
    const uint64_t hot_tail_update = 0x76543210fedcba98ULL;
    const uint64_t hot_next_update = 0xa1b2c3d4e5f60718ULL;
    const uint64_t recovery_update = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(hot_base_paddr, hot_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    write64(recovery_paddr, recovery_seed);
    for (size_t i = 0; i < pressure_count; ++i) {
        write64(pressure_paddrs[i], pressure_initials[i]);
    }
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_hot = ld(hot_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the hot target block is readable before the repeated same-block-inflight pressure corner runs",
        warmed_hot == hot_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later recovery store location is readable before the repeated same-block-inflight pressure corner runs",
        warmed_recovery == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    sd(hot_base_vaddr, hot_head_update);
    AI_ASSERT_NO_EXCEPT_AND("the first hot-block store retires before the first same-block CBOM probe in the repeated pressure corner",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(cbom_head_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first same-block CBOM probe retires without exception in the repeated pressure corner",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = 0; i < pressure_count / 2U; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the first half of the unrelated pressure stores all retire while the hot block remains under same-block inflight pressure",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(hot_base_vaddr + (3U * sizeof(uint64_t)), hot_tail_update);
    AI_ASSERT_NO_EXCEPT_AND("a later store to the hot-block tail retires without exception before the second same-block CBOM probe",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(cbom_tail_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second same-block CBOM probe also retires without exception in the repeated pressure corner",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = pressure_count / 2U; i < pressure_count; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the second half of the unrelated pressure stores all retire after the second same-block CBOM probe",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(hot_base_vaddr + sizeof(uint64_t), hot_next_update);
    AI_ASSERT_NO_EXCEPT_AND("a final same-block store still retires without exception after the repeated pressure waves",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_update);
    AI_ASSERT_NO_EXCEPT_AND("a later recovery store also retires without exception after the repeated same-block pressure waves",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the repeated same-block-inflight pressure sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_hot_head = ld(hot_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated same-block-inflight pressure sequence, the hot block head word still returns the first stored payload",
        observed_hot_head == hot_head_update
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_hot_next = ld(hot_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the repeated same-block-inflight pressure sequence, the later hot-block next word returns the newest payload instead of an oscillating stale image",
        observed_hot_next == hot_next_update
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_hot_tail = ld(hot_base_vaddr + (3U * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("after the repeated same-block-inflight pressure sequence, the hot-block tail word also keeps its stored payload",
        observed_hot_tail == hot_tail_update
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the hot target remains unchanged across the repeated same-block-inflight pressure sequence",
        observed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later recovery store payload is still visible after the repeated same-block-inflight pressure sequence",
        observed_recovery == recovery_update
    );

    TEST_SETUP_EXCEPT();
    bool pressure_visible_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_visible_ok = pressure_visible_ok &&
            (ld(pressure_vaddrs[i]) == pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("every unrelated pressure store remains visible after the repeated same-block-inflight pressure sequence",
        pressure_visible_ok
    );

    goto_priv(PRIV_M);
    bool pressure_backing_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_backing_ok = pressure_backing_ok &&
            (read64(pressure_paddrs[i]) == pressure_updates[i]);
    }
    TEST_ASSERT("repeated same-block inflight pressure does not pick the wrong victim or enter eviction oscillation: the hot block keeps all newer stores, the adjacent block stays seeded, and every pressure store plus the recovery store reaches backing memory",
        read64(hot_base_paddr) == hot_head_update &&
        read64(hot_base_paddr + sizeof(uint64_t)) == hot_next_update &&
        read64(hot_base_paddr + (2U * sizeof(uint64_t))) ==
            (hot_seed + (2U * 0x1111111111111111ULL)) &&
        read64(hot_base_paddr + (3U * sizeof(uint64_t))) == hot_tail_update &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed) &&
        read64(recovery_paddr) == recovery_update &&
        pressure_backing_ok
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_sbuffer_evict_waits_for_same_entry_refill_write_completion_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_bytes;
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 6U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 7U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 8U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 9U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 10U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 11U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 12U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 13U),
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 6U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 7U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 8U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 9U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 10U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 11U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 12U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 13U),
    };
    const uint64_t pressure_initials[] = {
        0x0101010101010101ULL,
        0x1212121212121212ULL,
        0x2323232323232323ULL,
        0x3434343434343434ULL,
        0x4545454545454545ULL,
        0x5656565656565656ULL,
        0x6767676767676767ULL,
        0x7878787878787878ULL,
    };
    const uint64_t pressure_updates[] = {
        0x8899aabbccddee01ULL,
        0x8899aabbccddee12ULL,
        0x8899aabbccddee23ULL,
        0x8899aabbccddee34ULL,
        0x8899aabbccddee45ULL,
        0x8899aabbccddee56ULL,
        0x8899aabbccddee67ULL,
        0x8899aabbccddee78ULL,
    };
    const size_t pressure_count = sizeof(pressure_vaddrs) / sizeof(pressure_vaddrs[0]);
    const uint64_t target_seed = 0x8877665544332211ULL;
    const uint64_t adjacent_seed = 0x55aa33cc77ee11ffULL;
    const uint64_t target_word0_initial = target_seed;
    const uint64_t target_word1_initial = target_seed + 0x1111111111111111ULL;
    const uint64_t target_word2_initial = target_seed + (2U * 0x1111111111111111ULL);
    const uint64_t target_word3_initial = target_seed + (3U * 0x1111111111111111ULL);
    const uint64_t target_word0_expected =
        (target_word0_initial & ~(0xffffULL << 16)) | (0xbeefULL << 16);
    const uint64_t target_word3_expected =
        (target_word3_initial & ~(0xffULL << 40)) | (0xaaULL << 40);

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    for (size_t i = 0; i < pressure_count; ++i) {
        write64(pressure_paddrs[i], pressure_initials[i]);
    }
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the same-entry refill-write-completion eviction corner runs",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the target is also readable before the same-entry refill-write-completion eviction corner runs",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_warm_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_warm_ok = pressure_warm_ok &&
            (ld(pressure_vaddrs[i]) == pressure_initials[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure blocks are all readable before the same-entry refill-write-completion eviction corner runs",
        pressure_warm_ok
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache line can be invalidated before building the cold partial-store miss window for the same-entry refill-write-completion eviction corner",
        true
    );

    TEST_SETUP_EXCEPT();
    bool pressure_inval_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        cbo_inval(pressure_vaddrs[i]);
        pressure_inval_ok = pressure_inval_ok && !excpt.triggered;
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure cache lines can all be invalidated before the same-entry refill-write-completion eviction corner",
        pressure_inval_ok
    );

    TEST_SETUP_EXCEPT();
    sh(target_base_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first cold partial store to the target line retires without exception before eviction pressure begins",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(target_base_vaddr + (3U * sizeof(uint64_t)) + 5U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the second cold partial store to another word in the same target line also retires without exception before eviction pressure begins",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = 0; i < pressure_count; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the unrelated pressure stores all retire without exception while the target line may still be receiving refill writes",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the same-entry refill-write-completion eviction sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target_word0 = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the same-entry refill-write-completion eviction sequence, the first target word keeps the partial-store merge instead of a stale pre-refill image",
        observed_target_word0 == target_word0_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target_word1 = ld(target_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the same-entry refill-write-completion eviction sequence, the untouched second target word still returns its seeded refill value",
        observed_target_word1 == target_word1_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target_word2 = ld(target_base_vaddr + (2U * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("after the same-entry refill-write-completion eviction sequence, the untouched third target word still returns its seeded refill value",
        observed_target_word2 == target_word2_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target_word3 = ld(target_base_vaddr + (3U * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("after the same-entry refill-write-completion eviction sequence, the later target word also keeps its partial-store merge instead of stale data",
        observed_target_word3 == target_word3_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block remains unchanged across the same-entry refill-write-completion eviction sequence",
        observed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_visible_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_visible_ok = pressure_visible_ok &&
            (ld(pressure_vaddrs[i]) == pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("every unrelated pressure store remains visible after the same-entry refill-write-completion eviction sequence",
        pressure_visible_ok
    );

    goto_priv(PRIV_M);
    bool pressure_backing_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_backing_ok = pressure_backing_ok &&
            (read64(pressure_paddrs[i]) == pressure_updates[i]);
    }
    TEST_ASSERT("the same-entry refill-write-completion eviction corner waits for refill writes before eviction so both target partial stores persist, untouched refill words survive, and all pressure stores reach backing memory",
        read64(target_base_paddr) == target_word0_expected &&
        read64(target_base_paddr + sizeof(uint64_t)) == target_word1_initial &&
        read64(target_base_paddr + (2U * sizeof(uint64_t))) == target_word2_initial &&
        read64(target_base_paddr + (3U * sizeof(uint64_t))) == target_word3_expected &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed) &&
        pressure_backing_ok
    );
    ai_micro_cmo_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_sbuffer_refill_and_evict_same_entry_no_stale_mask_or_data_to_dcache_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 18U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 18U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_bytes;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 21U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 21U);
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 24U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 25U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 26U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 27U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 28U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 29U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 30U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 31U),
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 24U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 25U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 26U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 27U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 28U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 29U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 30U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 31U),
    };
    const uint64_t pressure_initials[] = {
        0x1111111111111101ULL,
        0x2222222222222202ULL,
        0x3333333333333303ULL,
        0x4444444444444404ULL,
        0x5555555555555505ULL,
        0x6666666666666606ULL,
        0x7777777777777707ULL,
        0x8888888888888808ULL,
    };
    const uint64_t pressure_updates[] = {
        0xaaaabbbbccccdd01ULL,
        0xaaaabbbbccccdd12ULL,
        0xaaaabbbbccccdd23ULL,
        0xaaaabbbbccccdd34ULL,
        0xaaaabbbbccccdd45ULL,
        0xaaaabbbbccccdd56ULL,
        0xaaaabbbbccccdd67ULL,
        0xaaaabbbbccccdd78ULL,
    };
    const size_t pressure_count = sizeof(pressure_vaddrs) / sizeof(pressure_vaddrs[0]);
    const uint64_t target_seed = 0x1021324354657687ULL;
    const uint64_t adjacent_seed = 0xdeadbeefcafed00dULL;
    const uint64_t recovery_seed = 0x0fedcba987654321ULL;
    const uint64_t recovery_update = 0xa1b2c3d4e5f60718ULL;
    const uint64_t target_word0_initial = target_seed;
    const uint64_t target_word0_after_first =
        (target_word0_initial & ~(0xffffULL << 16)) | (0xbeefULL << 16);
    const uint64_t target_word0_expected =
        (target_word0_after_first & ~(0xffULL << 48)) | (0xaaULL << 48);
    const uint64_t target_word1_initial = target_seed + 0x1111111111111111ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    write64(recovery_paddr, recovery_seed);
    for (size_t i = 0; i < pressure_count; ++i) {
        write64(pressure_paddrs[i], pressure_initials[i]);
    }
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the repeated same-entry refill-and-evict mask/data corner runs",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block is readable before the repeated same-entry refill-and-evict mask/data corner runs",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later recovery location is readable before the repeated same-entry refill-and-evict mask/data corner runs",
        warmed_recovery == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_warm_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_warm_ok = pressure_warm_ok &&
            (ld(pressure_vaddrs[i]) == pressure_initials[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure blocks are all readable before the repeated same-entry refill-and-evict mask/data corner runs",
        pressure_warm_ok
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache line can be invalidated before constructing the repeated same-entry refill-and-evict mask/data corner",
        true
    );

    TEST_SETUP_EXCEPT();
    bool pressure_inval_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        cbo_inval(pressure_vaddrs[i]);
        pressure_inval_ok = pressure_inval_ok && !excpt.triggered;
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure cache lines can all be invalidated before the repeated same-entry refill-and-evict mask/data corner",
        pressure_inval_ok
    );

    TEST_SETUP_EXCEPT();
    sh(target_base_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first cold partial store to the target word retires without exception before the first pressure wave",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = 0; i < pressure_count / 2U; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the first pressure wave retires without exception while the target entry may still be accepting refill writes",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(target_base_vaddr + 6U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the second partial store to the same target word retires without exception between the two pressure waves",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = pressure_count / 2U; i < pressure_count; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the second pressure wave also retires without exception after the later same-entry update",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_update);
    AI_ASSERT_NO_EXCEPT_AND("a later independent recovery store still retires without exception after the repeated same-entry refill-and-evict pressure waves",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the repeated same-entry refill-and-evict mask/data sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target_word0 = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated same-entry refill-and-evict mask/data sequence, the target word keeps the cumulative byte merges rather than a stale mask or stale data image",
        observed_target_word0 == target_word0_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target_word1 = ld(target_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the repeated same-entry refill-and-evict mask/data sequence, the untouched next target word still returns its seeded refill value",
        observed_target_word1 == target_word1_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block remains unchanged across the repeated same-entry refill-and-evict mask/data sequence",
        observed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later independent recovery store is visible after the repeated same-entry refill-and-evict mask/data sequence",
        observed_recovery == recovery_update
    );

    TEST_SETUP_EXCEPT();
    bool pressure_visible_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_visible_ok = pressure_visible_ok &&
            (ld(pressure_vaddrs[i]) == pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("every pressure store remains visible after the repeated same-entry refill-and-evict mask/data sequence",
        pressure_visible_ok
    );

    goto_priv(PRIV_M);
    bool pressure_backing_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_backing_ok = pressure_backing_ok &&
            (read64(pressure_paddrs[i]) == pressure_updates[i]);
    }
    TEST_ASSERT("the repeated same-entry refill-and-evict mask/data corner preserves the cumulative target-byte merges, the untouched refill data, the adjacent block, and every pressure plus recovery store in backing memory",
        read64(target_base_paddr) == target_word0_expected &&
        read64(target_base_paddr + sizeof(uint64_t)) == target_word1_initial &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed) &&
        read64(recovery_paddr) == recovery_update &&
        pressure_backing_ok
    );
    ai_micro_cmo_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_sbuffer_refill_to_mp_resp_uses_coherent_line_snapshot_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 14U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 14U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_bytes;
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 24U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 25U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 26U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 27U),
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 24U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 25U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 26U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 27U),
    };
    const uint64_t pressure_seeds[] = {
        0x1111111111111111ULL,
        0x2222222222222222ULL,
        0x3333333333333333ULL,
        0x4444444444444444ULL,
    };
    const size_t pressure_count = sizeof(pressure_vaddrs) / sizeof(pressure_vaddrs[0]);
    const uint64_t target_seed = 0x8877665544332211ULL;
    const uint64_t adjacent_seed = 0x55aa33cc77ee11ffULL;
    const uint64_t target_word0_initial = target_seed;
    const uint64_t target_word0_after_first =
        (target_word0_initial & ~(0xffffULL << 16)) | (0xbeefULL << 16);
    const uint64_t target_word0_final =
        (target_word0_after_first & ~(0xffULL << 48)) | (0xaaULL << 48);
    const uint64_t target_word1_initial = target_seed + 0x1111111111111111ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    for (size_t i = 0; i < pressure_count; ++i) {
        write64(pressure_paddrs[i], pressure_seeds[i]);
    }
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the refill-to-misspipe coherent-snapshot corner runs",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block is also readable before the refill-to-misspipe coherent-snapshot corner runs",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_warm_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_warm_ok = pressure_warm_ok &&
            (ld(pressure_vaddrs[i]) == pressure_seeds[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure lines are all readable before the refill-to-misspipe coherent-snapshot corner runs",
        pressure_warm_ok
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache line can be invalidated before the refill-to-misspipe coherent-snapshot corner builds a cold store-miss entry",
        true
    );

    TEST_SETUP_EXCEPT();
    bool pressure_inval_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        cbo_inval(pressure_vaddrs[i]);
        pressure_inval_ok = pressure_inval_ok && !excpt.triggered;
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure lines can all be invalidated before the refill-to-misspipe coherent-snapshot corner",
        pressure_inval_ok
    );

    TEST_SETUP_EXCEPT();
    sh(target_base_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first cold partial store to the target line retires without exception before the refill-to-misspipe snapshot is observed",
        true
    );

    TEST_SETUP_EXCEPT();
    bool pressure_prefetch_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        prefetch_r(pressure_vaddrs[i]);
        pressure_prefetch_ok = pressure_prefetch_ok && !excpt.triggered;
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure-building software prefetches all retire silently before the refill-to-misspipe coherent-snapshot load",
        pressure_prefetch_ok
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_snapshot = ld(target_base_vaddr);
    bool snapshot_no_except = (excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    sb(target_base_vaddr + 6U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the younger same-entry byte store also retires without exception immediately after the older load request in the refill-to-misspipe coherent-snapshot corner",
        true
    );

    TEST_ASSERT("the older load in the refill-to-misspipe coherent-snapshot corner keeps the first partial-store snapshot and does not observe a mixed old/new line image",
        snapshot_no_except && observed_snapshot == target_word0_after_first
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the refill-to-misspipe coherent-snapshot sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_final_word0 = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the refill-to-misspipe coherent-snapshot sequence, the final target word returns the younger cumulative byte merge",
        observed_final_word0 == target_word0_final
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word1 = ld(target_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the refill-to-misspipe coherent-snapshot sequence, the untouched next target word still returns its seeded refill value",
        observed_word1 == target_word1_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block remains unchanged across the refill-to-misspipe coherent-snapshot sequence",
        observed_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refill-to-misspipe coherent-snapshot corner leaves backing memory with the final cumulative target-byte merge while preserving untouched words and the adjacent block",
        read64(target_base_paddr) == target_word0_final &&
        read64(target_base_paddr + sizeof(uint64_t)) == target_word1_initial &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBIE01);

    TEST_END();
}
