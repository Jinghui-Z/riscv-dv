#include "ai_micro_cmo_common.h"

bool ai_micro_hs_sbuffer_same_entry_update_during_refill_to_mp_no_mixed_epoch_data_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 16U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 16U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_bytes;
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 28U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 29U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 30U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 31U),
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 28U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 29U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 30U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 31U),
    };
    const uint64_t pressure_seeds[] = {
        0x5151515151515151ULL,
        0x6262626262626262ULL,
        0x7373737373737373ULL,
        0x8484848484848484ULL,
    };
    const size_t pressure_count = sizeof(pressure_vaddrs) / sizeof(pressure_vaddrs[0]);
    const uint64_t target_seed = 0x1021324354657687ULL;
    const uint64_t adjacent_seed = 0xdeadbeefcafed00dULL;
    const uint64_t target_word0_initial = target_seed;
    const uint64_t target_word0_after_first =
        (target_word0_initial & ~(0xffffULL << 16)) | (0xbeefULL << 16);
    const uint64_t target_word0_after_second =
        (target_word0_after_first & ~(0xffULL << 48)) | (0xaaULL << 48);
    const uint64_t younger_full_store = 0xa1b2c3d4e5f60718ULL;
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
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the same-entry refill-to-misspipe mixed-epoch corner runs",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block is also readable before the same-entry refill-to-misspipe mixed-epoch corner runs",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_warm_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_warm_ok = pressure_warm_ok &&
            (ld(pressure_vaddrs[i]) == pressure_seeds[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure lines are all readable before the same-entry refill-to-misspipe mixed-epoch corner runs",
        pressure_warm_ok
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache line can be invalidated before the same-entry refill-to-misspipe mixed-epoch corner builds a cold store-miss entry",
        true
    );

    TEST_SETUP_EXCEPT();
    bool pressure_inval_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        cbo_inval(pressure_vaddrs[i]);
        pressure_inval_ok = pressure_inval_ok && !excpt.triggered;
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure lines can all be invalidated before the same-entry refill-to-misspipe mixed-epoch corner",
        pressure_inval_ok
    );

    TEST_SETUP_EXCEPT();
    sh(target_base_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first same-entry partial store retires without exception before the same-entry refill-to-misspipe mixed-epoch snapshot is requested",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(target_base_vaddr + 6U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the second same-entry partial store also retires without exception before the same-entry refill-to-misspipe mixed-epoch snapshot is requested",
        true
    );

    TEST_SETUP_EXCEPT();
    bool pressure_prefetch_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        prefetch_r(pressure_vaddrs[i]);
        pressure_prefetch_ok = pressure_prefetch_ok && !excpt.triggered;
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure-building software prefetches all retire silently before the same-entry refill-to-misspipe mixed-epoch load",
        pressure_prefetch_ok
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_snapshot = ld(target_base_vaddr);
    bool snapshot_no_except = (excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    sd(target_base_vaddr, younger_full_store);
    AI_ASSERT_NO_EXCEPT_AND("the younger same-entry full-width store also retires without exception immediately after the older load request in the same-entry refill-to-misspipe mixed-epoch corner",
        true
    );

    TEST_ASSERT("the older load in the same-entry refill-to-misspipe mixed-epoch corner keeps the earlier cumulative partial-store snapshot and does not leak the younger full-width store payload",
        snapshot_no_except && observed_snapshot == target_word0_after_second
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the same-entry refill-to-misspipe mixed-epoch sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_final_word0 = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the same-entry refill-to-misspipe mixed-epoch sequence, the target word returns the younger full-width store payload",
        observed_final_word0 == younger_full_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word1 = ld(target_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the same-entry refill-to-misspipe mixed-epoch sequence, the untouched next target word still returns its seeded refill value",
        observed_word1 == target_word1_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block remains unchanged across the same-entry refill-to-misspipe mixed-epoch sequence",
        observed_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-entry refill-to-misspipe mixed-epoch corner leaves backing memory with the younger full-width target store while preserving untouched words and the adjacent block",
        read64(target_base_paddr) == younger_full_store &&
        read64(target_base_paddr + sizeof(uint64_t)) == target_word1_initial &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_sbuffer_partial_store_miss_full_overwrite_complement_line_image_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 52U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 52U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_bytes;
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 56U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 57U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 58U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 59U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 60U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 61U),
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 56U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 57U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 58U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 59U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 60U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 61U),
    };
    const uint64_t pressure_initials[] = {
        0x1717171717171701ULL,
        0x2828282828282812ULL,
        0x3939393939393923ULL,
        0x4a4a4a4a4a4a4a34ULL,
        0x5b5b5b5b5b5b5b45ULL,
        0x6c6c6c6c6c6c6c56ULL,
    };
    const uint64_t pressure_updates[] = {
        0xd1e2f3a4b5c60701ULL,
        0xd1e2f3a4b5c60712ULL,
        0xd1e2f3a4b5c60723ULL,
        0xd1e2f3a4b5c60734ULL,
        0xd1e2f3a4b5c60745ULL,
        0xd1e2f3a4b5c60756ULL,
    };
    const size_t pressure_count = sizeof(pressure_vaddrs) / sizeof(pressure_vaddrs[0]);
    const uint64_t target_seed = 0x60718293a4b5c6d7ULL;
    const uint64_t adjacent_seed = 0x1f2e3d4c5b6a7988ULL;
    const uint64_t full_word4_store = 0x0badf00d13579bdfULL;
    uint64_t expected_words[AI_MICRO_CMO_BLOCK_WORDS];

    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        expected_words[i] = target_seed + (i * 0x1111111111111111ULL);
    }
    expected_words[1] =
        (expected_words[1] & ~(0xffffULL << 16)) | (0xbabeULL << 16);
    expected_words[4] = full_word4_store;
    expected_words[7] =
        (expected_words[7] & ~(0xffULL << 48)) | (0x5dULL << 48);

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
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the partial-store-miss complement-line image corner starts",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block is readable before the partial-store-miss complement-line image corner starts",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_warm_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_warm_ok = pressure_warm_ok &&
            (ld(pressure_vaddrs[i]) == pressure_initials[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure blocks are readable before building the partial-store-miss complement-line image window",
        pressure_warm_ok
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache line can be invalidated before the mixed partial/full-overwrite store-miss sequence",
        true
    );

    TEST_SETUP_EXCEPT();
    bool pressure_inval_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        cbo_inval(pressure_vaddrs[i]);
        pressure_inval_ok = pressure_inval_ok && !excpt.triggered;
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure cache lines can be invalidated before the mixed partial/full-overwrite store-miss sequence",
        pressure_inval_ok
    );

    TEST_SETUP_EXCEPT();
    sh(target_base_vaddr + sizeof(uint64_t) + 2U, 0xbabeU);
    AI_ASSERT_NO_EXCEPT_AND("the cold lower-half partial store retires without exception before SBuffer refill complement pressure starts",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = 0; i < pressure_count / 2U; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the first pressure wave retires while the cold target store-miss line may still be refilling",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(target_base_vaddr + (4U * sizeof(uint64_t)), full_word4_store);
    AI_ASSERT_NO_EXCEPT_AND("the upper-half full-overwrite word retires without exception and must not steal refill complement data from partial words",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(target_base_vaddr + (7U * sizeof(uint64_t)) + 6U, 0x5dU);
    AI_ASSERT_NO_EXCEPT_AND("the last-beat high-byte partial store retires without exception in the same cold-line refill complement window",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = pressure_count / 2U; i < pressure_count; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the second pressure wave retires after the same-line full-overwrite and tail-byte partial stores",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the SBuffer refill complement-line sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    bool target_visible_ok = true;
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        target_visible_ok = target_visible_ok &&
            (ld(target_base_vaddr + (i * sizeof(uint64_t))) == expected_words[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the visible target line contains the exact mix of old refill bytes, two partial stores, and one full-overwrite word",
        target_visible_ok
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block remains unchanged across the mixed partial/full-overwrite store-miss refill complement sequence",
        observed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_visible_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_visible_ok = pressure_visible_ok &&
            (ld(pressure_vaddrs[i]) == pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("all pressure stores remain visible after the mixed partial/full-overwrite store-miss refill complement sequence",
        pressure_visible_ok
    );

    goto_priv(PRIV_M);
    bool target_backing_ok = true;
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        target_backing_ok = target_backing_ok &&
            (read64(target_base_paddr + (i * sizeof(uint64_t))) == expected_words[i]);
    }
    bool pressure_backing_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_backing_ok = pressure_backing_ok &&
            (read64(pressure_paddrs[i]) == pressure_updates[i]);
    }
    TEST_ASSERT("the drained backing line preserves old refill bytes in untouched words, keeps both partial-store byte ages, keeps the full-overwrite word exact, and does not corrupt pressure or adjacent blocks",
        target_backing_ok &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed) &&
        pressure_backing_ok
    );
    ai_micro_cmo_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_sbuffer_evict_valid_payload_stable_under_ready_backpressure_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 32U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 32U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_bytes;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 35U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 35U);
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 40U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 41U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 42U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 43U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 44U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 45U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 46U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 47U),
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 40U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 41U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 42U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 43U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 44U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 45U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 46U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 47U),
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
        0x9911223344556601ULL,
        0x9911223344556612ULL,
        0x9911223344556623ULL,
        0x9911223344556634ULL,
        0x9911223344556645ULL,
        0x9911223344556656ULL,
        0x9911223344556667ULL,
        0x9911223344556678ULL,
    };
    const size_t pressure_count = sizeof(pressure_vaddrs) / sizeof(pressure_vaddrs[0]);
    const uint64_t target_seed = 0x8877665544332211ULL;
    const uint64_t adjacent_seed = 0x55aa33cc77ee11ffULL;
    const uint64_t recovery_seed = 0x1021324354657687ULL;
    const uint64_t recovery_update = 0xa1b2c3d4e5f60718ULL;
    const uint64_t target_word0_initial = target_seed;
    const uint64_t target_word0_after_first =
        (target_word0_initial & ~(0xffffULL << 16)) | (0xbeefULL << 16);
    const uint64_t target_word0_final =
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
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the evict-valid ready-backpressure corner builds pressure",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block is also readable before the evict-valid ready-backpressure corner builds pressure",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later recovery location is readable before the evict-valid ready-backpressure corner builds pressure",
        warmed_recovery == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_warm_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_warm_ok = pressure_warm_ok &&
            (ld(pressure_vaddrs[i]) == pressure_initials[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure lines are all readable before the evict-valid ready-backpressure corner builds pressure",
        pressure_warm_ok
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache line can be invalidated before building the cold partial-store entry for the evict-valid ready-backpressure corner",
        true
    );

    TEST_SETUP_EXCEPT();
    bool pressure_inval_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        cbo_inval(pressure_vaddrs[i]);
        pressure_inval_ok = pressure_inval_ok && !excpt.triggered;
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure lines can all be invalidated before the evict-valid ready-backpressure corner",
        pressure_inval_ok
    );

    TEST_SETUP_EXCEPT();
    sh(target_base_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first cold partial store to the target line retires without exception before eviction pressure is applied",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = 0; i < pressure_count / 2U; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the first pressure wave retires without exception while the target eviction request may already be waiting on dcache ready",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_snapshot = ld(target_base_vaddr);
    bool snapshot_no_except = (excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    sb(target_base_vaddr + 6U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the later same-entry byte store also retires without exception after eviction pressure is already present",
        true
    );

    TEST_ASSERT("the older architectural reload in the evict-valid ready-backpressure corner remains bound to the pre-update payload and does not observe a mutated in-flight eviction image",
        snapshot_no_except &&
        observed_snapshot == target_word0_after_first
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
    AI_ASSERT_NO_EXCEPT_AND("the later independent recovery store retires without exception after the evict-valid ready-backpressure sequence",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the evict-valid ready-backpressure sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_final_word0 = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the evict-valid ready-backpressure sequence, the target word returns the cumulative later byte merge instead of a stale or mixed eviction payload",
        observed_final_word0 == target_word0_final
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word1 = ld(target_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the evict-valid ready-backpressure sequence, the untouched next target word still returns its seeded refill value",
        observed_word1 == target_word1_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block remains unchanged across the evict-valid ready-backpressure sequence",
        observed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later recovery store remains visible after the evict-valid ready-backpressure sequence",
        observed_recovery == recovery_update
    );

    TEST_SETUP_EXCEPT();
    bool pressure_visible_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_visible_ok = pressure_visible_ok &&
            (ld(pressure_vaddrs[i]) == pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("every pressure store remains visible after the evict-valid ready-backpressure sequence",
        pressure_visible_ok
    );

    goto_priv(PRIV_M);
    bool pressure_backing_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_backing_ok = pressure_backing_ok &&
            (read64(pressure_paddrs[i]) == pressure_updates[i]);
    }
    TEST_ASSERT("the evict-valid ready-backpressure corner keeps the older snapshot stable while backing memory converges to the later same-entry merge, with untouched words, adjacent data, and all pressure plus recovery stores preserved",
        read64(target_base_paddr) == target_word0_final &&
        read64(target_base_paddr + sizeof(uint64_t)) == target_word1_initial &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed) &&
        read64(recovery_paddr) == recovery_update &&
        pressure_backing_ok
    );
    ai_micro_cmo_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_sbuffer_same_entry_update_after_evict_valid_no_mutated_dcache_payload_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 36U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 36U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_bytes;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_bytes;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 39U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 39U);
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 48U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 49U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 50U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 51U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 52U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 53U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 54U),
        hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 55U),
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 48U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 49U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 50U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 51U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 52U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 53U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 54U),
        phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 55U),
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
    const uint64_t recovery_update = 0x1122334455667788ULL;
    const uint64_t target_word0_initial = target_seed;
    const uint64_t target_word0_after_first =
        (target_word0_initial & ~(0xffffULL << 16)) | (0xbeefULL << 16);
    const uint64_t target_word0_final =
        (target_word0_after_first & ~(0xffULL << 48)) | (0xaaULL << 48);
    const uint64_t younger_word1_full = 0xa1b2c3d4e5f60718ULL;
    const uint64_t target_word2_initial = target_seed + (2U * 0x1111111111111111ULL);

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
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the same-entry post-evict-valid payload-mutation corner builds pressure",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block is also readable before the same-entry post-evict-valid payload-mutation corner builds pressure",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later recovery location is readable before the same-entry post-evict-valid payload-mutation corner builds pressure",
        warmed_recovery == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    bool pressure_warm_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_warm_ok = pressure_warm_ok &&
            (ld(pressure_vaddrs[i]) == pressure_initials[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure lines are all readable before the same-entry post-evict-valid payload-mutation corner builds pressure",
        pressure_warm_ok
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache line can be invalidated before building the cold partial-store entry for the same-entry post-evict-valid payload-mutation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    bool pressure_inval_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        cbo_inval(pressure_vaddrs[i]);
        pressure_inval_ok = pressure_inval_ok && !excpt.triggered;
    }
    AI_ASSERT_NO_EXCEPT_AND("the pressure lines can all be invalidated before the same-entry post-evict-valid payload-mutation corner",
        pressure_inval_ok
    );

    TEST_SETUP_EXCEPT();
    sh(target_base_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first target partial store retires without exception before the eviction-valid pressure wave",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(target_base_vaddr + 6U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the second target partial store also retires without exception before the eviction-valid pressure wave",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = 0; i < pressure_count / 2U; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the first pressure wave retires without exception while the target eviction request may already be waiting on dcache ready",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(target_base_vaddr + sizeof(uint64_t), younger_word1_full);
    AI_ASSERT_NO_EXCEPT_AND("the later same-entry full-width update to the next target word also retires without exception after eviction pressure is already present",
        true
    );

    TEST_SETUP_EXCEPT();
    for (size_t i = pressure_count / 2U; i < pressure_count; ++i) {
        sd(pressure_vaddrs[i], pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("the second pressure wave also retires without exception after the later same-entry word update",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_update);
    AI_ASSERT_NO_EXCEPT_AND("the later independent recovery store retires without exception after the same-entry post-evict-valid payload-mutation sequence",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the same-entry post-evict-valid payload-mutation sequence retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word0 = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the same-entry post-evict-valid payload-mutation sequence, the original target word still returns only its own earlier cumulative byte merges",
        observed_word0 == target_word0_final
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word1 = ld(target_base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the same-entry post-evict-valid payload-mutation sequence, the next target word returns the later full-width update without mutating the already-selected eviction payload of other words",
        observed_word1 == younger_word1_full
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word2 = ld(target_base_vaddr + (2U * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("after the same-entry post-evict-valid payload-mutation sequence, the untouched third target word still returns its seeded refill value",
        observed_word2 == target_word2_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block remains unchanged across the same-entry post-evict-valid payload-mutation sequence",
        observed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later recovery store remains visible after the same-entry post-evict-valid payload-mutation sequence",
        observed_recovery == recovery_update
    );

    TEST_SETUP_EXCEPT();
    bool pressure_visible_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_visible_ok = pressure_visible_ok &&
            (ld(pressure_vaddrs[i]) == pressure_updates[i]);
    }
    AI_ASSERT_NO_EXCEPT_AND("every pressure store remains visible after the same-entry post-evict-valid payload-mutation sequence",
        pressure_visible_ok
    );

    goto_priv(PRIV_M);
    bool pressure_backing_ok = true;
    for (size_t i = 0; i < pressure_count; ++i) {
        pressure_backing_ok = pressure_backing_ok &&
            (read64(pressure_paddrs[i]) == pressure_updates[i]);
    }
    TEST_ASSERT("the same-entry post-evict-valid payload-mutation corner keeps the preexisting target word bytes stable, commits the later neighboring-word update coherently, and preserves untouched, adjacent, pressure, and recovery data in backing memory",
        read64(target_base_paddr) == target_word0_final &&
        read64(target_base_paddr + sizeof(uint64_t)) == younger_word1_full &&
        read64(target_base_paddr + (2U * sizeof(uint64_t))) == target_word2_initial &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed) &&
        read64(recovery_paddr) == recovery_update &&
        pressure_backing_ok
    );
    ai_micro_cmo_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cmo_last_instruction_page_fault_recovers()
{
    TEST_START();

    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + AI_MICRO_CMO_BLOCK_OFFSET;
    const uint64_t zero_seed = 0x1021324354657687ULL;
    const uint64_t recovery_head = 0x89abcdef01234567ULL;
    const uint64_t recovery_next = 0x76543210fedcba98ULL;
    const uint64_t recovery_tail = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    write64(recovery_paddr, recovery_head);
    write64(recovery_paddr + sizeof(uint64_t), recovery_next);
    write64(recovery_paddr + (2U * sizeof(uint64_t)), recovery_tail);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first CMO in the consecutive pair retires successfully before the later faulting CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the successful first CMO also retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the last CMO in the consecutive pair reports store page fault when its translated page is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier cbo.zero still commits its full zeroing effect even though the later consecutive CMO faults",
        ai_micro_cmo_block_all_zero(good_zero_paddr)
    );
    TEST_ASSERT("the separate recovery block remains unchanged before the post-fault recovery probe runs",
        read64(recovery_paddr) == recovery_head &&
        read64(recovery_paddr + sizeof(uint64_t)) == recovery_next &&
        read64(recovery_paddr + (2U * sizeof(uint64_t))) == recovery_tail
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later CBOM probe still retires normally after the consecutive sequence ends with a faulting CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-fault recovery probe leaves the follow-on CBOM target readable",
        recovered_head == recovery_head
    );

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_first_instruction_page_fault_recovers()
{
    TEST_START();

    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + AI_MICRO_CMO_BLOCK_OFFSET;
    const uint64_t zero_seed = 0x13579bdf2468ace0ULL;
    const uint64_t recovery_head = 0x02468ace13579bdfULL;
    const uint64_t recovery_next = 0xfdb97531eca86420ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    write64(recovery_paddr, recovery_head);
    write64(recovery_paddr + sizeof(uint64_t), recovery_next);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the first CMO in the consecutive pair reports store page fault when its translated page is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later CMO in the same consecutive sequence still retires successfully after the first CMO faults",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the later successful CMO retires without exception",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful later cbo.zero still commits its full zeroing effect after the earlier CMO fault",
        ai_micro_cmo_block_all_zero(good_zero_paddr)
    );
    TEST_ASSERT("the separate recovery block keeps its seeded payload before the follow-on CBOM recovery probe",
        read64(recovery_paddr) == recovery_head &&
        read64(recovery_paddr + sizeof(uint64_t)) == recovery_next
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on CBOM probe still retires normally after a consecutive sequence that begins with a faulting CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the follow-on recovery probe leaves its CBOM target readable after the earlier fault-first sequence",
        recovered_head == recovery_head
    );

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_last_instruction_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_vaddr = hs_page_base(VSRWX_GRWX) + AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t zero_seed = 0x5566778899aabbccULL;
    const uint64_t recovery_head = 0x2468ace013579bdfULL;
    const uint64_t recovery_next = 0x1122334455667788ULL;
    const uint64_t bad_seed = 0x0badf00d12345678ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    write64(recovery_paddr, recovery_head);
    write64(recovery_paddr + sizeof(uint64_t), recovery_next);
    write64(bad_paddr + AI_MICRO_CMO_BLOCK_OFFSET, bad_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE | MENVCFG_CBZFE);
    ai_micro_cmo_configure_pmp_window(
        bad_paddr, bad_paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first CMO in the consecutive pair still retires successfully before the later SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the successful first CMO also retires without exception before the later SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the last CMO in the consecutive pair reports store access fault when PMP denies the translated CBOM read side",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier cbo.zero still commits its full zeroing effect even though the later consecutive CMO raises SAF",
        ai_micro_cmo_block_all_zero(good_zero_paddr)
    );
    TEST_ASSERT("the faulting CBOM probe leaves the denied target word unchanged when the later consecutive CMO raises SAF",
        read64(bad_paddr + AI_MICRO_CMO_BLOCK_OFFSET) == bad_seed
    );
    TEST_ASSERT("the separate recovery block remains unchanged before the post-SAF recovery probe runs",
        read64(recovery_paddr) == recovery_head &&
        read64(recovery_paddr + sizeof(uint64_t)) == recovery_next
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later CBOM probe still retires normally after the consecutive sequence ends with a SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-SAF recovery probe leaves the follow-on CBOM target readable",
        recovered_head == recovery_head
    );

    goto_priv(PRIV_M);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_reset_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_first_instruction_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_vaddr = hs_page_base(VSRWX_GRWX) + AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t zero_seed = 0x89abcdef01234567ULL;
    const uint64_t recovery_head = 0x76543210fedcba98ULL;
    const uint64_t recovery_next = 0x13579bdf2468ace0ULL;
    const uint64_t bad_seed = 0xfeedfacecafebeefULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    write64(recovery_paddr, recovery_head);
    write64(recovery_paddr + sizeof(uint64_t), recovery_next);
    write64(bad_paddr + AI_MICRO_CMO_BLOCK_OFFSET, bad_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE | MENVCFG_CBZFE);
    ai_micro_cmo_configure_pmp_window(
        bad_paddr, bad_paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the first CMO in the consecutive pair reports store access fault when PMP denies the translated CBOM read side",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later CMO in the same consecutive sequence still retires successfully after the earlier SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the later successful CMO retires without exception after the earlier SAFing CMO",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting first CBOM probe leaves the denied target word unchanged when the earlier consecutive CMO raises SAF",
        read64(bad_paddr + AI_MICRO_CMO_BLOCK_OFFSET) == bad_seed
    );
    TEST_ASSERT("the successful later cbo.zero still commits its full zeroing effect after the earlier CMO raises SAF",
        ai_micro_cmo_block_all_zero(good_zero_paddr)
    );
    TEST_ASSERT("the separate recovery block keeps its seeded payload before the follow-on post-SAF CBOM probe",
        read64(recovery_paddr) == recovery_head &&
        read64(recovery_paddr + sizeof(uint64_t)) == recovery_next
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on CBOM probe still retires normally after a consecutive sequence that begins with a SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-SAF follow-on CBOM probe leaves its target readable",
        recovered_head == recovery_head
    );

    goto_priv(PRIV_M);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_reset_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_last_instruction_mmio_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uint64_t zero_seed = 0x5566778899aabbccULL;
    const uint64_t recovery_seed = 0x2468ace013579bdfULL;
    const uint64_t bad_seed = 0x0badf00d12345678ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    ai_micro_cmo_seed_block(recovery_paddr, recovery_seed);
    ai_micro_cmo_seed_block(bad_paddr, bad_seed);
    uintptr_t bad_vaddr = ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZE | MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first CMO in the consecutive pair still retires successfully before the later PMA=Device SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the successful first CMO also retires without exception before the later PMA=Device SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the last CMO in the consecutive pair reports store access fault when PMA classifies the translated CBOM target as Device/MMIO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier cbo.zero still commits its full zeroing effect even though the later consecutive CMO hits a PMA=Device SAF",
        ai_micro_cmo_block_all_zero(good_zero_paddr)
    );
    TEST_ASSERT("the faulting PMA=Device CBOM probe leaves the whole faulting backing cache block unchanged when the later consecutive CMO raises SAF",
        ai_micro_cmo_block_matches_seed(bad_paddr, bad_seed)
    );
    TEST_ASSERT("the separate recovery block remains unchanged before the post-PMA-fault recovery probe runs",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later CBOM probe still retires normally after the consecutive sequence ends with a PMA=Device SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-PMA-fault recovery probe leaves the follow-on CBOM target readable",
        recovered_head == recovery_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-PMA-fault recovery probe preserves the recovery backing cache block",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed)
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_first_instruction_mmio_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uint64_t zero_seed = 0x89abcdef01234567ULL;
    const uint64_t recovery_seed = 0x76543210fedcba98ULL;
    const uint64_t bad_seed = 0xfeedfacecafebeefULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    ai_micro_cmo_seed_block(recovery_paddr, recovery_seed);
    ai_micro_cmo_seed_block(bad_paddr, bad_seed);
    uintptr_t bad_vaddr = ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZE | MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the first CMO in the consecutive pair reports store access fault when PMA classifies the translated CBOM target as Device/MMIO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later CMO in the same consecutive sequence still retires successfully after the earlier PMA=Device SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the later successful CMO retires without exception after the earlier PMA=Device SAFing CMO",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting first PMA=Device CBOM probe leaves the whole faulting backing cache block unchanged when the earlier consecutive CMO raises SAF",
        ai_micro_cmo_block_matches_seed(bad_paddr, bad_seed)
    );
    TEST_ASSERT("the successful later cbo.zero still commits its full zeroing effect after the earlier PMA=Device CMO raises SAF",
        ai_micro_cmo_block_all_zero(good_zero_paddr)
    );
    TEST_ASSERT("the separate recovery block keeps its seeded payload before the follow-on post-PMA-fault CBOM probe",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on CBOM probe still retires normally after a consecutive sequence that begins with a PMA=Device SAFing CMO",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-PMA-fault follow-on CBOM probe leaves its target readable",
        recovered_head == recovery_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-PMA-fault follow-on CBOM probe preserves the recovery backing cache block",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed)
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_last_instruction_page_fault_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_adjacent_paddr = good_zero_paddr + block_size;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_adjacent_vaddr = recovery_vaddr + block_size;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + AI_MICRO_CMO_BLOCK_OFFSET;
    const uint64_t zero_seed = 0x1021324354657687ULL;
    const uint64_t zero_adjacent_seed = 0x89abcdef01234567ULL;
    const uint64_t recovery_seed = 0x76543210fedcba98ULL;
    const uint64_t recovery_adjacent_seed = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    ai_micro_cmo_seed_block(good_zero_adjacent_paddr, zero_adjacent_seed);
    ai_micro_cmo_seed_block(recovery_paddr, recovery_seed);
    ai_micro_cmo_seed_block(recovery_adjacent_paddr, recovery_adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first CMO in the consecutive pair still retires successfully before the later page-faulting CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the successful first CMO also retires without exception in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the last CMO in the consecutive pair reports store page fault when its translated page is invalid in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier cbo.zero still commits its full zeroing effect while preserving the adjacent cache block even though the later consecutive CMO faults",
        ai_micro_cmo_block_all_zero(good_zero_paddr) &&
        ai_micro_cmo_block_matches_seed(good_zero_adjacent_paddr, zero_adjacent_seed)
    );
    TEST_ASSERT("the separate recovery block and its adjacent block remain unchanged before the post-page-fault recovery probe runs",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later CBOM probe still retires normally after the consecutive sequence ends with a page-faulting CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-page-fault recovery probe leaves the follow-on CBOM target readable in the adjacent-block-preservation corner",
        recovered_head == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_adjacent = ld(recovery_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovery CBOM target also remains readable with its original payload",
        recovered_adjacent == recovery_adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-page-fault recovery probe preserves both the recovery block and its adjacent backing-memory block",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_first_instruction_page_fault_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t good_zero_adjacent_paddr = good_zero_paddr + block_size;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_adjacent_vaddr = recovery_vaddr + block_size;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uint64_t zero_seed = 0x13579bdf2468ace0ULL;
    const uint64_t zero_adjacent_seed = 0x02468ace13579bdfULL;
    const uint64_t recovery_seed = 0xfdb97531eca86420ULL;
    const uint64_t recovery_adjacent_seed = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    ai_micro_cmo_seed_block(good_zero_adjacent_paddr, zero_adjacent_seed);
    ai_micro_cmo_seed_block(recovery_paddr, recovery_seed);
    ai_micro_cmo_seed_block(recovery_adjacent_paddr, recovery_adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the first CMO in the consecutive pair reports store page fault when its translated page is invalid in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later CMO in the same consecutive sequence still retires successfully after the first CMO faults in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the later successful CMO retires without exception in the adjacent-block-preservation corner",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful later cbo.zero still commits its full zeroing effect while preserving the adjacent cache block after the earlier CMO fault",
        ai_micro_cmo_block_all_zero(good_zero_paddr) &&
        ai_micro_cmo_block_matches_seed(good_zero_adjacent_paddr, zero_adjacent_seed)
    );
    TEST_ASSERT("the separate recovery block and its adjacent block keep their seeded payloads before the follow-on recovery probe",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on CBOM probe still retires normally after a consecutive sequence that begins with a page-faulting CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the follow-on recovery probe leaves its CBOM target readable after the earlier fault-first sequence in the adjacent-block-preservation corner",
        recovered_head == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_adjacent = ld(recovery_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovery CBOM target also remains readable with its original payload after the earlier fault-first sequence",
        recovered_adjacent == recovery_adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the follow-on recovery probe preserves both the recovery block and its adjacent backing-memory block after the earlier page fault",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_last_instruction_access_fault_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_adjacent_paddr = good_zero_paddr + block_size;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_adjacent_vaddr = recovery_vaddr + block_size;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(VSRWX_GRWX) + AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_paddr = phys_page_base(VSRWX_GRWX);
    const uintptr_t bad_block_paddr = bad_paddr + AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_adjacent_paddr = bad_block_paddr + block_size;
    const uint64_t zero_seed = 0x5566778899aabbccULL;
    const uint64_t zero_adjacent_seed = 0x0badc0de12345678ULL;
    const uint64_t recovery_seed = 0x2468ace013579bdfULL;
    const uint64_t recovery_adjacent_seed = 0x1122334455667788ULL;
    const uint64_t bad_seed = 0x0badf00d12345678ULL;
    const uint64_t bad_adjacent_seed = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    ai_micro_cmo_seed_block(good_zero_adjacent_paddr, zero_adjacent_seed);
    ai_micro_cmo_seed_block(recovery_paddr, recovery_seed);
    ai_micro_cmo_seed_block(recovery_adjacent_paddr, recovery_adjacent_seed);
    ai_micro_cmo_seed_block(bad_block_paddr, bad_seed);
    ai_micro_cmo_seed_block(bad_adjacent_paddr, bad_adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE | MENVCFG_CBZFE);
    ai_micro_cmo_configure_pmp_window(
        bad_paddr, bad_paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first CMO in the consecutive pair still retires successfully before the later SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the successful first CMO also retires without exception before the later SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the last CMO in the consecutive pair reports store access fault when PMP denies the translated CBOM read side in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier cbo.zero still commits its full zeroing effect while preserving the adjacent cache block even though the later consecutive CMO raises SAF",
        ai_micro_cmo_block_all_zero(good_zero_paddr) &&
        ai_micro_cmo_block_matches_seed(good_zero_adjacent_paddr, zero_adjacent_seed)
    );
    TEST_ASSERT("the faulting CBOM probe leaves both the denied target block and its adjacent block unchanged when the later consecutive CMO raises SAF",
        ai_micro_cmo_block_matches_seed(bad_block_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    TEST_ASSERT("the separate recovery block and its adjacent block remain unchanged before the post-SAF recovery probe runs",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later CBOM probe still retires normally after the consecutive sequence ends with a SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-SAF recovery probe leaves the follow-on CBOM target readable in the adjacent-block-preservation corner",
        recovered_head == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_adjacent = ld(recovery_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovery CBOM target also remains readable with its original payload after the later SAFing CMO",
        recovered_adjacent == recovery_adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-SAF recovery probe preserves both the recovery block and its adjacent backing-memory block",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_reset_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_last_instruction_mmio_access_fault_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t good_zero_adjacent_paddr = good_zero_paddr + block_size;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_adjacent_vaddr = recovery_vaddr + block_size;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(AI_MICRO_CMO_MMIO_ALIAS_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_adjacent_paddr = bad_paddr + block_size;
    const uint64_t zero_seed = 0x5566778899aabbccULL;
    const uint64_t zero_adjacent_seed = 0x0badc0de12345678ULL;
    const uint64_t recovery_seed = 0x2468ace013579bdfULL;
    const uint64_t recovery_adjacent_seed = 0x1122334455667788ULL;
    const uint64_t bad_seed = 0x0badf00d12345678ULL;
    const uint64_t bad_adjacent_seed = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    ai_micro_cmo_seed_block(good_zero_adjacent_paddr, zero_adjacent_seed);
    ai_micro_cmo_seed_block(recovery_paddr, recovery_seed);
    ai_micro_cmo_seed_block(recovery_adjacent_paddr, recovery_adjacent_seed);
    ai_micro_cmo_seed_block(bad_paddr, bad_seed);
    ai_micro_cmo_seed_block(bad_adjacent_paddr, bad_adjacent_seed);
    ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZE | MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first CMO in the consecutive pair still retires successfully before the later PMA=Device SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the successful first CMO also retires without exception before the later PMA=Device SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the last CMO in the consecutive pair reports store access fault when PMA classifies the translated CBOM target as Device/MMIO in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier cbo.zero still commits its full zeroing effect while preserving the adjacent cache block even though the later consecutive CMO hits a PMA=Device SAF",
        ai_micro_cmo_block_all_zero(good_zero_paddr) &&
        ai_micro_cmo_block_matches_seed(good_zero_adjacent_paddr, zero_adjacent_seed)
    );
    TEST_ASSERT("the PMA=Device CBOM probe leaves both the faulting block and its adjacent block unchanged when the later consecutive CMO raises SAF",
        ai_micro_cmo_block_matches_seed(bad_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    TEST_ASSERT("the separate recovery block and its adjacent block remain unchanged before the post-PMA-fault recovery probe runs",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later CBOM probe still retires normally after the consecutive sequence ends with a PMA=Device SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-PMA-fault recovery probe leaves the follow-on CBOM target readable in the adjacent-block-preservation corner",
        recovered_head == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_adjacent = ld(recovery_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovery CBOM target also remains readable with its original payload after the later PMA=Device SAFing CMO",
        recovered_adjacent == recovery_adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-PMA-fault recovery probe preserves both the recovery block and its adjacent backing-memory block",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_first_instruction_access_fault_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t good_zero_adjacent_paddr = good_zero_paddr + block_size;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_adjacent_vaddr = recovery_vaddr + block_size;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(VSRWX_GRWX) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t bad_paddr = phys_page_base(VSRWX_GRWX);
    const uintptr_t bad_block_paddr = bad_paddr + (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t bad_adjacent_paddr = bad_block_paddr + block_size;
    const uint64_t zero_seed = 0x89abcdef01234567ULL;
    const uint64_t zero_adjacent_seed = 0x55aa33cc77ee11ffULL;
    const uint64_t recovery_seed = 0x76543210fedcba98ULL;
    const uint64_t recovery_adjacent_seed = 0x13579bdf2468ace0ULL;
    const uint64_t bad_seed = 0xfeedfacecafebeefULL;
    const uint64_t bad_adjacent_seed = 0x1029384756abcdefULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    ai_micro_cmo_seed_block(good_zero_adjacent_paddr, zero_adjacent_seed);
    ai_micro_cmo_seed_block(recovery_paddr, recovery_seed);
    ai_micro_cmo_seed_block(recovery_adjacent_paddr, recovery_adjacent_seed);
    ai_micro_cmo_seed_block(bad_block_paddr, bad_seed);
    ai_micro_cmo_seed_block(bad_adjacent_paddr, bad_adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE | MENVCFG_CBZFE);
    ai_micro_cmo_configure_pmp_window(
        bad_paddr, bad_paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the first CMO in the consecutive pair reports store access fault when PMP denies the translated CBOM read side in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later CMO in the same consecutive sequence still retires successfully after the earlier SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the later successful CMO retires without exception after the earlier SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting first CBOM probe leaves both the denied target block and its adjacent block unchanged when the earlier consecutive CMO raises SAF",
        ai_micro_cmo_block_matches_seed(bad_block_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    TEST_ASSERT("the successful later cbo.zero still commits its full zeroing effect while preserving the adjacent cache block after the earlier CMO raises SAF",
        ai_micro_cmo_block_all_zero(good_zero_paddr) &&
        ai_micro_cmo_block_matches_seed(good_zero_adjacent_paddr, zero_adjacent_seed)
    );
    TEST_ASSERT("the separate recovery block and its adjacent block keep their seeded payloads before the follow-on post-SAF CBOM probe",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on CBOM probe still retires normally after a consecutive sequence that begins with a SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-SAF follow-on CBOM probe leaves its target readable in the adjacent-block-preservation corner",
        recovered_head == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_adjacent = ld(recovery_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovery CBOM target also remains readable with its original payload after the earlier SAFing CMO",
        recovered_adjacent == recovery_adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-SAF follow-on CBOM probe preserves both the recovery block and its adjacent backing-memory block",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_reset_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_first_instruction_mmio_access_fault_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t good_zero_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t good_zero_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t good_zero_adjacent_paddr = good_zero_paddr + block_size;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_adjacent_vaddr = recovery_vaddr + block_size;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(AI_MICRO_CMO_MMIO_ALIAS_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_adjacent_paddr = bad_paddr + block_size;
    const uint64_t zero_seed = 0x89abcdef01234567ULL;
    const uint64_t zero_adjacent_seed = 0x55aa33cc77ee11ffULL;
    const uint64_t recovery_seed = 0x76543210fedcba98ULL;
    const uint64_t recovery_adjacent_seed = 0x13579bdf2468ace0ULL;
    const uint64_t bad_seed = 0xfeedfacecafebeefULL;
    const uint64_t bad_adjacent_seed = 0x1029384756abcdefULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(good_zero_paddr, zero_seed);
    ai_micro_cmo_seed_block(good_zero_adjacent_paddr, zero_adjacent_seed);
    ai_micro_cmo_seed_block(recovery_paddr, recovery_seed);
    ai_micro_cmo_seed_block(recovery_adjacent_paddr, recovery_adjacent_seed);
    ai_micro_cmo_seed_block(bad_paddr, bad_seed);
    ai_micro_cmo_seed_block(bad_adjacent_paddr, bad_adjacent_seed);
    ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZE | MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the first CMO in the consecutive pair reports store access fault when PMA classifies the translated CBOM target as Device/MMIO in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(good_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later CMO in the same consecutive sequence still retires successfully after the earlier PMA=Device SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the later successful CMO retires without exception after the earlier PMA=Device SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMA=Device CBOM probe leaves both the faulting block and its adjacent block unchanged when the earlier consecutive CMO raises SAF",
        ai_micro_cmo_block_matches_seed(bad_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    TEST_ASSERT("the successful later cbo.zero still commits its full zeroing effect while preserving the adjacent cache block after the earlier PMA=Device SAF",
        ai_micro_cmo_block_all_zero(good_zero_paddr) &&
        ai_micro_cmo_block_matches_seed(good_zero_adjacent_paddr, zero_adjacent_seed)
    );
    TEST_ASSERT("the separate recovery block and its adjacent block keep their seeded payloads before the follow-on post-PMA-fault CBOM probe",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on CBOM probe still retires normally after a consecutive sequence that begins with a PMA=Device SAFing CMO in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the post-PMA-fault follow-on CBOM probe leaves its target readable in the adjacent-block-preservation corner",
        recovered_head == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_adjacent = ld(recovery_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovery CBOM target also remains readable with its original payload after the earlier PMA=Device SAFing CMO",
        recovered_adjacent == recovery_adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-PMA-fault follow-on CBOM probe preserves both the recovery block and its adjacent backing-memory block",
        ai_micro_cmo_block_matches_seed(recovery_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_sbuffer_hitresp_delayed_mask_clear_reused_entry_no_later_store_drop_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_sbuffer_hitresp_delayed_mask_clear_window(false);
    TEST_ASSERT("SBuffer delayed hit-response mask-clear single-later-store probe completes without losing the later store payload",
        test_status);

    TEST_END();
}

bool ai_micro_hs_sbuffer_hitresp_delayed_mask_clear_reused_entry_no_cross_line_mask_loss_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_sbuffer_hitresp_delayed_mask_clear_window(true);
    TEST_ASSERT("SBuffer delayed hit-response mask-clear dense cross-line probe completes without losing any later store payload",
        test_status);

    TEST_END();
}
