#include "ai_micro_cmo_common.h"

bool ai_micro_hs_cbom_aligned_no_exception()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uint64_t first_value = 0x1122334455667788ULL;
    const uint64_t second_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, first_value);
    write64(paddr + sizeof(uint64_t), second_value);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the aligned HS cacheable target is readable before issuing the CBOM-style probes",
        warmed == first_value
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an aligned HS cbo.clean retires without surfacing an exception on a translated cacheable page",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an aligned HS cbo.flush also retires without surfacing an exception on the same translated cacheable page",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the CBOM-style maintenance sequence leaves the target payload readable afterwards",
        observed == first_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the CBOM-style maintenance sequence preserves the backing memory contents",
        read64(paddr) == first_value &&
        read64(paddr + sizeof(uint64_t)) == second_value
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cmo_subblock_offset_no_exception()
{
    TEST_START();

    const uintptr_t cbom_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t cbom_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t cbozero_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t cbozero_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t subblock_cbom_vaddr = cbom_base_vaddr + 1U;
    const uintptr_t subblock_cbozero_vaddr = cbozero_base_vaddr + 1U;
    const uint64_t cbom_seed = 0x1122334455667788ULL;
    const uint64_t cbozero_seed = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(cbom_base_paddr, cbom_seed);
    ai_micro_cmo_seed_block(cbozero_base_paddr, cbozero_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE | MENVCFG_CBZE | MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_cbom = ld(cbom_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS cacheable block targeted by sub-block-offset CBOM probes is readable before maintenance",
        warmed_cbom == cbom_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(subblock_cbom_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.clean accepts a translated cache-block sub-block offset without surfacing an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(subblock_cbom_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.flush also accepts the same translated cache-block sub-block offset without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(subblock_cbom_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval also accepts the same translated cache-block sub-block offset without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_head = ld(cbom_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the CBOM-style sub-block-offset sequence keeps the head word readable afterwards",
        preserved_head == cbom_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_tail = ld(cbom_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("the same CBOM-style sub-block-offset sequence preserves the tail word as well",
        preserved_tail == (cbom_seed +
            ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * 0x1111111111111111ULL))
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_cbozero = ld(cbozero_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS cacheable block targeted by the sub-block-offset cbo.zero probe is readable before zeroing",
        warmed_cbozero == cbozero_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(subblock_cbozero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.zero accepts a translated cache-block sub-block offset without surfacing an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the sub-block-offset cbo.zero completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_head = ld(cbozero_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the head word reads back as zero after cbo.zero is issued through a sub-block offset",
        zero_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_tail = ld(cbozero_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("the tail word in the same cache block also reads back as zero after the sub-block-offset cbo.zero",
        zero_tail == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the CBOM-style sub-block-offset sequence preserves the backing memory payload",
        read64(cbom_base_paddr) == cbom_seed &&
        read64(cbom_base_paddr + ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t))) ==
            (cbom_seed + ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * 0x1111111111111111ULL))
    );
    TEST_ASSERT("the sub-block-offset cbo.zero still clears the whole targeted cache block in backing memory",
        ai_micro_cmo_block_all_zero(cbozero_base_paddr)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE | MENVCFG_CBZE | MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbom_tail_subblock_offset_no_exception_corner()
{
    TEST_START();

    const uintptr_t cbom_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t cbom_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t adjacent_base_vaddr = cbom_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t adjacent_base_paddr = cbom_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t tail_subblock_vaddr = cbom_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t)) - 1U;
    const uint64_t cbom_seed = 0x1122334455667788ULL;
    const uint64_t adjacent_seed = 0x99aabbccddeeff00ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(cbom_base_paddr, cbom_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE | MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(cbom_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS cacheable block targeted by the tail-offset CBOM corner is readable before maintenance",
        warmed_target == cbom_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block also reads back its seeded payload before the tail-offset CBOM corner",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(tail_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.clean accepts the last-byte cache-block offset without surfacing an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(tail_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.flush also accepts the same last-byte cache-block offset without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(tail_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval also accepts the same last-byte cache-block offset without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target_tail = ld(cbom_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("the tail word in the target cache block stays readable after the last-byte-offset CBOM sequence",
        preserved_target_tail == (cbom_seed +
            ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * 0x1111111111111111ULL))
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent_head = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block remains unchanged after the last-byte-offset CBOM sequence",
        preserved_adjacent_head == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the last-byte-offset CBOM sequence preserves both the target block and the neighboring block in backing memory",
        ai_micro_cmo_block_matches_seed(cbom_base_paddr, cbom_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE | MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbom_midblock_tail_offset_both_neighbors_preserved_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t previous_base_vaddr = target_base_vaddr -
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t previous_base_paddr = target_base_paddr -
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t next_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t next_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t tail_subblock_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t)) - 1U;
    const uint64_t previous_seed = 0x1021324354657687ULL;
    const uint64_t target_seed = 0x2233445566778899ULL;
    const uint64_t next_seed = 0xaabbccddeeff0011ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(previous_base_paddr, previous_seed);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(next_base_paddr, next_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE | MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_previous = ld(previous_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the previous neighboring block is readable before the mid-block tail-offset CBOM corner",
        warmed_previous == previous_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the middle target block is readable before the mid-block tail-offset CBOM corner",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_next = ld(next_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the next neighboring block is readable before the mid-block tail-offset CBOM corner",
        warmed_next == next_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(tail_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.clean accepts a middle-block last-byte offset without surfacing an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(tail_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.flush also accepts the same middle-block last-byte offset without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(tail_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval also accepts the same middle-block last-byte offset without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_previous = ld(previous_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the previous neighboring block remains unchanged after the mid-block tail-offset CBOM sequence",
        preserved_previous == previous_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target_tail = ld(target_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("the target block itself also remains unchanged after the mid-block tail-offset CBOM sequence",
        preserved_target_tail == (target_seed +
            ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * 0x1111111111111111ULL))
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_next = ld(next_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the next neighboring block remains unchanged after the mid-block tail-offset CBOM sequence",
        preserved_next == next_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the mid-block tail-offset CBOM sequence preserves the previous, target, and next blocks in backing memory",
        ai_micro_cmo_block_matches_seed(previous_base_paddr, previous_seed) &&
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(next_base_paddr, next_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE | MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbozero_data_zeroed()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uint64_t seed = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)), seed + (i * 0x1111111111111111ULL));
    }
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS cache block targeted by cbo.zero is readable before the zeroing operation",
        warmed == seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(vaddr + 24U);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.zero retires without surfacing an exception on a translated cacheable page",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after cbo.zero completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_head = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the head of the cache block reads back as zero after cbo.zero",
        zero_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_tail = ld(vaddr + ((AI_MICRO_CMO_BLOCK_WORDS - 1) * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("the tail of the same cache block also reads back as zero after cbo.zero",
        zero_tail == 0ULL
    );

    goto_priv(PRIV_M);
    bool all_zero = true;
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        all_zero = all_zero &&
            (read64(paddr + (i * sizeof(uint64_t))) == 0ULL);
    }
    TEST_ASSERT("cbo.zero clears the whole targeted cache block in backing memory",
        all_zero
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_whole_line_store_and_partial_store_same_line_forward_by_byte_age_correctly_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t head_vaddr = target_base_vaddr;
    const uintptr_t partial_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t tail_vaddr = target_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t));
    const uintptr_t zero_vaddr = target_base_vaddr + 24U;
    const uint64_t seed = 0x1021324354657687ULL;
    const uint32_t partial_value = 0x89abcdefU;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_head = ld(head_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block head is readable before the partial-Store then cbo.zero same-line forwarding corner",
        warmed_head == seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_partial = ld(partial_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the word later touched by the partial Store is also readable before the same-line forwarding corner",
        warmed_partial == (seed + 0x1111111111111111ULL)
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_tail = ld(tail_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block tail is readable before the partial-Store then cbo.zero same-line forwarding corner",
        warmed_tail == (seed + ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * 0x1111111111111111ULL))
    );

    TEST_SETUP_EXCEPT();
    sw(partial_vaddr, partial_value);
    AI_ASSERT_NO_EXCEPT_AND("the older partial Store in the target cache line retires without exception before the younger whole-line zeroing operation",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger whole-line cbo.zero retires without exception after the older partial Store in the same cache line",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_head = ld(head_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after partial Store then cbo.zero on the same cache line, the head word already reads back as zero before drain",
        observed_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_partial = ld(partial_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after partial Store then cbo.zero on the same cache line, the word that was partially written is also fully zeroed by age order",
        observed_partial == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_tail = ld(tail_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after partial Store then cbo.zero on the same cache line, the tail word also reads back as zero before drain",
        observed_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the partial-Store then cbo.zero same-line forwarding corner retires without exception",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after an older partial Store and a younger whole-line cbo.zero to the same cache line, backing memory is fully zeroed with no stale partial bytes left behind",
        ai_micro_cmo_block_all_zero(target_base_paddr)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_zero_lineflag_wildcard_no_wrong_forward_to_other_word_in_line_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t head_vaddr = target_base_vaddr;
    const uintptr_t partial_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t middle_vaddr = target_base_vaddr + (3U * sizeof(uint64_t));
    const uintptr_t tail_vaddr = target_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t));
    const uintptr_t zero_vaddr = target_base_vaddr + 24U;
    const uint64_t seed = 0x0102030405060708ULL;
    const uint32_t partial_value = 0x89abcdefU;
    const uint64_t expected_partial_word = (uint64_t)partial_value;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_head = ld(head_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block head is readable before the cbo.zero then partial-Store same-line wildcard-forwarding corner",
        warmed_head == seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_partial = ld(partial_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the word later touched by the younger partial Store is readable before the wildcard-forwarding corner",
        warmed_partial == (seed + 0x1111111111111111ULL)
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_tail = ld(tail_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block tail is readable before the cbo.zero then partial-Store wildcard-forwarding corner",
        warmed_tail == (seed + ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * 0x1111111111111111ULL))
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the older whole-line cbo.zero retires without exception before the younger partial Store in the same cache line",
        true
    );

    TEST_SETUP_EXCEPT();
    sw(partial_vaddr, partial_value);
    AI_ASSERT_NO_EXCEPT_AND("the younger partial Store retires without exception after the older whole-line zeroing operation",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_head = ld(head_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after cbo.zero then partial Store on the same cache line, the head word remains zero instead of receiving a wrong wildcard-forwarded partial payload",
        observed_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_partial = ld(partial_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after cbo.zero then partial Store on the same cache line, the touched word reads back only the younger partial payload by byte age",
        observed_partial == expected_partial_word
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_middle = ld(middle_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after cbo.zero then partial Store on the same cache line, another untouched middle word still reads back as zero",
        observed_middle == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_tail = ld(tail_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after cbo.zero then partial Store on the same cache line, the tail word also remains zero instead of picking up the younger partial payload",
        observed_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the cbo.zero then partial-Store wildcard-forwarding corner retires without exception",
        true
    );

    goto_priv(PRIV_M);
    bool backing_ok = true;
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        uint64_t expected = (i == 1U) ? expected_partial_word : 0ULL;
        backing_ok = backing_ok &&
            (read64(target_base_paddr + (i * sizeof(uint64_t))) == expected);
    }
    TEST_ASSERT("after an older whole-line cbo.zero and a younger partial Store to the same cache line, backing memory keeps only the touched word's younger bytes while every other word stays zero",
        backing_ok
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_storequeue_head_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t adjacent_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t zero_vaddr = target_base_vaddr + 24U;
    const uint64_t target_seed = 0x0102030405060708ULL;
    const uint64_t adjacent_seed = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cbo.zero storequeue-head target block is readable before zeroing in the adjacent-block-preservation corner",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the cbo.zero storequeue-head target is also readable before zeroing",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS cbo.zero request retires without exception in the storequeue-head adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the storequeue-head cbo.zero also retires without exception in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zeroed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the storequeue-head cbo.zero target reads back as zero in the adjacent-block-preservation corner",
        zeroed_target == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the storequeue-head cbo.zero target remains readable with its original payload",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the storequeue-head cbo.zero clears only the target backing-memory block and preserves the adjacent backing-memory block",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_zero_partial_store_small_load_mixed_source_no_select_mixup_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_cbo_zero_partial_store_small_load_mixed_source_window(false);
    TEST_ASSERT("cbo.zero plus younger partial store feeds small loads from the current mixed source without stale select",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_cbo_zero_partial_store_small_load_mixed_source_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_cbo_zero_partial_store_small_load_mixed_source_window(true);
    TEST_ASSERT("cbo.zero mixed-source small-load shaping preserves partial-store bytes and adjacent sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_inflight_valid_mixed_load_forward_by_byte_age_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_inflight_valid_mixed_load_window(false);
    TEST_ASSERT("same-block inflight and younger valid SBuffer sources merge load-forward bytes by age",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_inflight_valid_mixed_load_forward_by_byte_age_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_inflight_valid_mixed_load_window(true);
    TEST_ASSERT("same-block inflight plus valid forward keeps mixed-word byte age and adjacent cacheline sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_diff_word_inflight_valid_no_cross_word_forward_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_diff_word_inflight_valid_window(false);
    TEST_ASSERT("same-block inflight and valid sources in different words do not bleed across the queried word",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_diff_word_inflight_valid_no_cross_word_forward_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_diff_word_inflight_valid_window(true);
    TEST_ASSERT("different-word inflight plus valid SBuffer forward preserves each word and adjacent sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_mixed_word_back_to_back_loads_no_stale_query_select_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_mixed_word_back_to_back_load_window(false);
    TEST_ASSERT("back-to-back loads of the same mixed SBuffer word refresh query offset width and signedness",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_mixed_word_back_to_back_loads_no_stale_query_select_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_mixed_word_back_to_back_load_window(true);
    TEST_ASSERT("back-to-back mixed-word SBuffer queries keep current select and preserve adjacent sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_waiting_valid_second_merge_preserves_byte_age_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_waiting_valid_two_merges_window(false);
    TEST_ASSERT("second younger store merges into the waiting valid SBuffer generation with correct byte age",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_waiting_valid_second_merge_preserves_byte_age_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_waiting_valid_two_merges_window(true);
    TEST_ASSERT("waiting valid second-merge sequence preserves byte age and adjacent same-block sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_waiting_valid_multiword_merge_keeps_all_words_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_waiting_valid_multiword_window(false);
    TEST_ASSERT("waiting valid SBuffer generation accumulates younger stores across multiple words",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_same_block_waiting_valid_multiword_merge_keeps_all_words_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_same_block_waiting_valid_multiword_window(true);
    TEST_ASSERT("multiword waiting-valid merge keeps all younger words and adjacent guard words intact",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_reinflight_handoff_then_younger_same_word_merge_preserves_generation_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_reinflight_handoff_same_word_window(false);
    TEST_ASSERT("re-inflight same-block handoff lets the younger same-word store merge into the current generation",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_reinflight_handoff_then_younger_same_word_merge_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cmo_sbuffer_reinflight_handoff_same_word_window(true);
    TEST_ASSERT("re-inflight generation handoff preserves younger same-word merge and adjacent sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_sbuffer_tag_mismatch_decision_stays_on_original_query_epoch_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 5U);
    const uintptr_t alias_base_vaddr = hs_page_base(AI_MICRO_CMO_ALIAS_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 5U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 5U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t adjacent_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t zero_vaddr = target_base_vaddr + 24U;
    const uintptr_t partial_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t alias_partial_vaddr = alias_base_vaddr + sizeof(uint64_t);
    const uint64_t target_seed = 0x1021324354657687ULL;
    const uint64_t adjacent_seed = 0x55aa33cc77ee11ffULL;
    const uint32_t partial_value = 0x89abcdefU;
    const uint64_t expected_partial_word = (uint64_t)partial_value;
    bool backing_ok = true;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);
    ai_micro_cmo_map_alias_to_base(AI_MICRO_CMO_ALIAS_PAGE,
        AI_MICRO_CMO_CBOZERO_PAGE);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base cbo.zero target block is readable before the sbuffer tag-mismatch query-epoch corner",
        warmed_base == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias of the cbo.zero target block is also readable before the sbuffer tag-mismatch query-epoch corner",
        warmed_alias == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the cbo.zero target is readable before the sbuffer tag-mismatch query-epoch corner",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the older whole-line cbo.zero retires without exception before the alias-query epoch is sampled",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_epoch = ld(alias_base_vaddr);
    bool alias_epoch_no_except = (excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    sw(partial_vaddr, partial_value);
    AI_ASSERT_NO_EXCEPT_AND("the next-cycle base-VA partial Store retires without exception while the earlier alias load may still be resolving its sbuffer tag-mismatch decision",
        true
    );

    TEST_ASSERT("the alias load stays bound to the original query epoch and reads back the all-zero cbo.zero image instead of losing or inventing matchInvalid after the later same-entry mutation",
        alias_epoch_no_except &&
        observed_alias_epoch == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the later same-entry partial Store, the base head word still reads back as zero in the sbuffer tag-mismatch query-epoch corner",
        observed_base == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_partial = ld(partial_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the later same-entry partial Store, the touched word reads back only the younger partial payload in the sbuffer tag-mismatch query-epoch corner",
        observed_partial == expected_partial_word
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_partial = ld(alias_partial_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias of the touched word also observes only the younger partial payload after the sbuffer tag-mismatch query-epoch corner",
        observed_alias_partial == expected_partial_word
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the cbo.zero target remains unchanged across the sbuffer tag-mismatch query-epoch corner",
        observed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the sbuffer tag-mismatch query-epoch corner retires without exception",
        true
    );

    goto_priv(PRIV_M);
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        uint64_t expected = (i == 1U) ? expected_partial_word : 0ULL;
        backing_ok = backing_ok &&
            (read64(target_base_paddr + (i * sizeof(uint64_t))) == expected);
    }
    TEST_ASSERT("the sbuffer tag-mismatch query-epoch corner keeps the alias query bound to the original cbo.zero image while backing memory ends with only the later partial update and the adjacent block preserved",
        backing_ok &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_alias_matchinvalid_not_created_or_lost_by_one_cycle_entry_retag_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 6U);
    const uintptr_t alias_base_vaddr = hs_page_base(AI_MICRO_CMO_ALIAS_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 6U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 6U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t adjacent_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t zero_vaddr = target_base_vaddr + 24U;
    const uintptr_t partial_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t alias_partial_vaddr = alias_base_vaddr + sizeof(uint64_t);
    const uint64_t target_seed = 0x0102030405060708ULL;
    const uint64_t adjacent_seed = 0x8899aabbccddeeffULL;
    const uint32_t partial_value = 0x76543210U;
    const uint64_t expected_partial_word = (uint64_t)partial_value;
    bool backing_ok = true;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);
    ai_micro_cmo_map_alias_to_base(AI_MICRO_CMO_ALIAS_PAGE,
        AI_MICRO_CMO_CBOZERO_PAGE);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base cbo.zero target block is readable before the one-cycle retag matchInvalid corner",
        warmed_base == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias of the cbo.zero target block is also readable before the one-cycle retag matchInvalid corner",
        warmed_alias == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the cbo.zero target is readable before the one-cycle retag matchInvalid corner",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the older whole-line cbo.zero retires without exception before the base-query epoch is sampled",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_epoch = ld(target_base_vaddr);
    bool base_epoch_no_except = (excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    sw(alias_partial_vaddr, partial_value);
    AI_ASSERT_NO_EXCEPT_AND("the next-cycle alias partial Store retires without exception while the earlier base load may still be resolving its sbuffer tag-mismatch decision",
        true
    );

    TEST_ASSERT("the base-VA load must not invent a new alias matchInvalid because the same sbuffer entry is retagged one cycle later through an alias partial Store",
        base_epoch_no_except &&
        observed_base_epoch == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_head = ld(alias_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the later alias partial Store, the alias head word still reads back as zero in the one-cycle retag matchInvalid corner",
        observed_alias_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_partial = ld(partial_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the later alias partial Store, the touched word reads back only the younger partial payload in the one-cycle retag matchInvalid corner",
        observed_partial == expected_partial_word
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_partial = ld(alias_partial_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias of the touched word also observes only the younger partial payload after the one-cycle retag matchInvalid corner",
        observed_alias_partial == expected_partial_word
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the cbo.zero target remains unchanged across the one-cycle retag matchInvalid corner",
        observed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the one-cycle retag matchInvalid corner retires without exception",
        true
    );

    goto_priv(PRIV_M);
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        uint64_t expected = (i == 1U) ? expected_partial_word : 0ULL;
        backing_ok = backing_ok &&
            (read64(target_base_paddr + (i * sizeof(uint64_t))) == expected);
    }
    TEST_ASSERT("the one-cycle retag matchInvalid corner keeps the base query bound to the original cbo.zero image while backing memory ends with only the later alias partial update and the adjacent block preserved",
        backing_ok &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_storequeue_cbozero_sbuffer_adjacent_cycle_collision_corner()
{
    TEST_START();

    const uintptr_t block_bytes = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t before_store_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t zero_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t zero_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t preserved_base_vaddr = zero_base_vaddr + block_bytes;
    const uintptr_t preserved_base_paddr = zero_base_paddr + block_bytes;
    const uintptr_t after_store_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t after_store_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 4U);
    const uintptr_t zero_probe_vaddr = zero_base_vaddr + 24U;
    const uint64_t before_initial = 0x1021324354657687ULL;
    const uint64_t before_updated = 0x8877665544332211ULL;
    const uint64_t zero_seed = 0x0102030405060708ULL;
    const uint64_t preserved_seed = 0xfedcba9876543210ULL;
    const uint64_t after_initial = 0x89abcdef01234567ULL;
    const uint64_t after_updated = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(before_store_paddr, before_initial);
    ai_micro_cmo_seed_block(zero_base_paddr, zero_seed);
    ai_micro_cmo_seed_block(preserved_base_paddr, preserved_seed);
    write64(after_store_paddr, after_initial);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_before = ld(before_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable store immediately before the cbo.zero overlap window is readable before the adjacent-cycle collision probe",
        warmed_before == before_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_zero = ld(zero_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cbo.zero target block is readable before the adjacent-cycle collision probe",
        warmed_zero == zero_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_preserved = ld(preserved_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the cbo.zero target is also readable before the adjacent-cycle collision probe",
        warmed_preserved == preserved_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_after = ld(after_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable store immediately after the cbo.zero overlap window is readable before the adjacent-cycle collision probe",
        warmed_after == after_initial
    );

    TEST_SETUP_EXCEPT();
    sd(before_store_vaddr, before_updated);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable Store immediately before the cbo.zero request retires without exception in the adjacent-cycle collision corner",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cbo.zero request retires without exception while sandwiched between adjacent cacheable Stores in the collision corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(after_store_vaddr, after_updated);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable Store immediately after the cbo.zero request also retires without exception in the adjacent-cycle collision corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the cacheable-Store plus cbo.zero plus cacheable-Store overlap window retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(before_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the adjacent-cycle collision window, the earlier cacheable Store payload is still immediately visible",
        observed_before == before_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_zero = ld(zero_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the adjacent-cycle collision window, the cbo.zero target block reads back as zero",
        observed_zero == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_preserved = ld(preserved_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the adjacent-cycle collision window, the cache block adjacent to the cbo.zero target remains unchanged",
        observed_preserved == preserved_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(after_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the adjacent-cycle collision window, the later cacheable Store payload is also immediately visible",
        observed_after == after_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cacheable-Store plus cbo.zero plus cacheable-Store adjacent-cycle collision commits both Stores, zeros only the target block, and preserves the adjacent block in backing memory",
        read64(before_store_paddr) == before_updated &&
        ai_micro_cmo_block_all_zero(zero_base_paddr) &&
        ai_micro_cmo_block_matches_seed(preserved_base_paddr, preserved_seed) &&
        read64(after_store_paddr) == after_updated
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_tail_subblock_offset_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t adjacent_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t adjacent_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t tail_subblock_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t)) - 1U;
    const uint64_t target_seed = 0x0102030405060708ULL;
    const uint64_t adjacent_seed = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the tail-offset cbo.zero corner runs",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block is also readable before the tail-offset cbo.zero corner runs",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(tail_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.zero accepts the last-byte cache-block offset without surfacing an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the tail-offset cbo.zero completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_target_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the head of the target cache block reads back as zero after tail-offset cbo.zero",
        zero_target_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_target_tail = ld(target_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("the tail of the target cache block also reads back as zero after tail-offset cbo.zero",
        zero_target_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent_head = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block remains readable and unchanged after the tail-offset cbo.zero",
        preserved_adjacent_head == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("tail-offset cbo.zero clears only the targeted cache block and preserves the neighboring block in backing memory",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_midblock_tail_offset_both_neighbors_preserved_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t previous_base_vaddr = target_base_vaddr -
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t previous_base_paddr = target_base_paddr -
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t next_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t next_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t tail_subblock_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t)) - 1U;
    const uint64_t previous_seed = 0x1021324354657687ULL;
    const uint64_t target_seed = 0x0102030405060708ULL;
    const uint64_t next_seed = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(previous_base_paddr, previous_seed);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(next_base_paddr, next_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_previous = ld(previous_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the previous neighboring block is readable before the mid-block tail-offset cbo.zero corner runs",
        warmed_previous == previous_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the middle target block is readable before the mid-block tail-offset cbo.zero corner runs",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_next = ld(next_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the next neighboring block is readable before the mid-block tail-offset cbo.zero corner runs",
        warmed_next == next_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(tail_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.zero accepts a middle-block last-byte offset without surfacing an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the mid-block tail-offset cbo.zero completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_previous = ld(previous_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the previous neighboring block remains unchanged after the mid-block tail-offset cbo.zero",
        preserved_previous == previous_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_target_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the middle target block reads back as zero after the mid-block tail-offset cbo.zero",
        zero_target_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_next = ld(next_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the next neighboring block remains unchanged after the mid-block tail-offset cbo.zero",
        preserved_next == next_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the mid-block tail-offset cbo.zero clears only the target block and preserves both neighboring blocks in backing memory",
        ai_micro_cmo_block_matches_seed(previous_base_paddr, previous_seed) &&
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(next_base_paddr, next_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbom_repeated_same_block_idempotent_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t adjacent_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t adjacent_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t first_subblock_vaddr = target_base_vaddr + 1U;
    const uintptr_t middle_subblock_vaddr = target_base_vaddr + 17U;
    const uintptr_t tail_subblock_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t)) - 1U;
    const uint64_t target_seed = 0x3141592653589793ULL;
    const uint64_t adjacent_seed = 0x2718281828459045ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE | MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the repeated same-block CBOM corner runs",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block is also readable before the repeated same-block CBOM corner runs",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(first_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first same-block cbo.clean probe retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(middle_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the subsequent same-block cbo.flush probe on a different internal offset also retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(tail_subblock_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same-block tail-offset cbo.inval probe also retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("repeating CBOM on the cache-block base itself still retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("repeating CBOM probes on the same cache block leaves the head word unchanged",
        preserved_target_head == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target_tail = ld(target_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("repeating CBOM probes on the same cache block also leaves the tail word unchanged",
        preserved_target_tail == (target_seed +
            ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * 0x1111111111111111ULL))
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent_head = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("repeating same-block CBOM probes does not disturb the adjacent cache block",
        preserved_adjacent_head == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeating CBOM probes on the same cache block preserves both the target and adjacent backing-memory blocks",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE | MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbozero_repeated_same_block_idempotent_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t adjacent_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t adjacent_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t first_zero_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t)) - 1U;
    const uintptr_t second_zero_vaddr = target_base_vaddr + 1U;
    const uint64_t target_seed = 0x0123456789abcdefULL;
    const uint64_t adjacent_seed = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the repeated cbo.zero corner runs",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent cache block is also readable before the repeated cbo.zero corner runs",
        warmed_adjacent == adjacent_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(first_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first same-block cbo.zero probe retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the first same-block cbo.zero retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_after_first = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block head reads back as zero after the first cbo.zero",
        zero_after_first == 0ULL
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(second_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("repeating cbo.zero on the same cache block through a different internal offset also retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the repeated same-block cbo.zero also retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_target_tail = ld(target_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("repeating cbo.zero on the same cache block leaves the full target block zeroed",
        zero_target_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent_head = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("repeating cbo.zero on the same cache block does not disturb the adjacent block",
        preserved_adjacent_head == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeating cbo.zero on the same cache block keeps the target block zeroed and preserves the adjacent backing-memory block",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_then_cbom_same_block_zero_persists_corner()
{
    TEST_START();

    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t previous_base_vaddr = target_base_vaddr -
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t previous_base_paddr = target_base_paddr -
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t next_base_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t next_base_paddr = target_base_paddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    const uintptr_t zero_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t)) - 1U;
    const uintptr_t cbom_mid_vaddr = target_base_vaddr + 9U;
    const uintptr_t cbom_tail_vaddr = target_base_vaddr +
        (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t)) - 1U;
    const uint64_t previous_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t target_seed = 0x89abcdef01234567ULL;
    const uint64_t next_seed = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(previous_base_paddr, previous_seed);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(next_base_paddr, next_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE | MENVCFG_CBZE | MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_previous = ld(previous_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the previous neighboring block is readable before the cbo.zero-then-CBOM corner runs",
        warmed_previous == previous_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block is readable before the cbo.zero-then-CBOM corner runs",
        warmed_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_next = ld(next_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the next neighboring block is readable before the cbo.zero-then-CBOM corner runs",
        warmed_next == next_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the leading same-block cbo.zero retires without exception before the follow-on CBOM probes",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the leading same-block cbo.zero retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_target_head = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache block head reads back as zero after the leading cbo.zero",
        zero_target_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(cbom_mid_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on same-block cbo.clean retires without exception after cbo.zero",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(cbom_tail_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on same-block cbo.flush retires without exception after cbo.zero",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on same-block cbo.inval also retires without exception after cbo.zero",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_previous = ld(previous_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the previous neighboring block remains unchanged after cbo.zero followed by same-block CBOM probes",
        preserved_previous == previous_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t zero_target_tail = ld(target_base_vaddr +
        ((AI_MICRO_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("the target block remains fully zeroed after cbo.zero followed by same-block CBOM probes",
        zero_target_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_next = ld(next_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the next neighboring block remains unchanged after cbo.zero followed by same-block CBOM probes",
        preserved_next == next_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("cbo.zero followed by same-block CBOM probes keeps the target backing-memory block zeroed and preserves both neighboring blocks",
        ai_micro_cmo_block_matches_seed(previous_base_paddr, previous_seed) &&
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(next_base_paddr, next_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE | MENVCFG_CBZE | MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbom_pmp_read_access_fault()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE);
    const uint64_t initial = 0x7766554433221100ULL;

    goto_priv(PRIV_M);
    write64(paddr + AI_MICRO_CMO_BLOCK_OFFSET, initial);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);
    ai_micro_cmo_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(vaddr);
    TEST_ASSERT("an HS CBOM-style instruction reports store access fault when PMP denies the translated read side of the maintenance access",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the failed CBOM-style probe leaves the target payload unchanged when PMP denies reads",
        read64(paddr + AI_MICRO_CMO_BLOCK_OFFSET) == initial
    );
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_pmp_read_access_fault_then_valid_probe_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t recovery_vaddr = target_base_vaddr + block_size - 1U;
    const uintptr_t deny_page_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE);
    const uint64_t target_seed = 0x4a3b2c1d0e5f6071ULL;
    const uint64_t adjacent_seed = 0x9182736455463728ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);
    ai_micro_cmo_configure_pmp_window(
        deny_page_paddr, deny_page_paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(target_base_vaddr);
    TEST_ASSERT("a denied HS CBOM probe reports store access fault before the same page is re-enabled in the adjacent-block-preservation recovery corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_base_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied CBOM probe leaves both the target block and its adjacent block unchanged before PMP permissions are restored",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a tail-offset CBOM probe on the same cache block retires normally after PMP permissions are restored in the adjacent-block-preservation recovery corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered CBOM target remains readable with its seeded payload after the earlier denied probe",
        preserved_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered CBOM target also remains readable with its original payload",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered CBOM probe preserves both the target block and its adjacent backing-memory block after the earlier PMP read denial",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_repeated_pmp_read_access_fault_stable_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t second_probe_vaddr = target_base_vaddr + 1U;
    const uintptr_t deny_page_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE);
    const uint64_t target_seed = 0x4a3b2c1d0e5f6071ULL;
    const uint64_t adjacent_seed = 0x9182736455463728ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);
    ai_micro_cmo_configure_pmp_window(
        deny_page_paddr, deny_page_paddr + PAGE_SIZE, AI_MICRO_CMO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(target_base_vaddr);
    TEST_ASSERT("the first denied HS CBOM probe reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_base_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(second_probe_vaddr);
    TEST_ASSERT("the second denied HS CBOM probe also reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_probe_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PMP-denied CBOM probes leave both the target block and its adjacent block unchanged before permissions are restored",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(second_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid subblock-offset CBOM probe still retires normally after repeated PMP read denials in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered CBOM target remains readable with its seeded payload after repeated PMP read denials",
        preserved_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered CBOM target also remains readable with its original payload after repeated PMP read denials",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PMP-denied CBOM probes stay stable and the later valid CBOM probe still preserves both backing-memory blocks",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_mmio_access_fault()
{
    TEST_START();

    const uintptr_t paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    uintptr_t vaddr = ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZFE);
    const uint64_t initial = 0x1021324354657687ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(vaddr);
    TEST_ASSERT("an HS CBOM-style instruction reports store access fault when PMA classifies the translated target as Device/MMIO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the failed PMA=Device CBOM-style probe leaves the shared backing memory unchanged",
        read64(paddr) == initial
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_mmio_access_fault_then_valid_probe_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t bad_base_paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_adjacent_paddr = bad_base_paddr + block_size;
    const uintptr_t recovery_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t recovery_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t recovery_adjacent_vaddr = recovery_base_vaddr + block_size;
    const uintptr_t recovery_adjacent_paddr = recovery_base_paddr + block_size;
    const uintptr_t recovery_probe_vaddr = recovery_base_vaddr + block_size - 1U;
    const uint64_t bad_seed = 0x1021324354657687ULL;
    const uint64_t bad_adjacent_seed = 0x89abcdef01234567ULL;
    const uint64_t recovery_seed = 0x76543210fedcba98ULL;
    const uint64_t recovery_adjacent_seed = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(bad_base_paddr, bad_seed);
    ai_micro_cmo_seed_block(bad_adjacent_paddr, bad_adjacent_seed);
    ai_micro_cmo_seed_block(recovery_base_paddr, recovery_seed);
    ai_micro_cmo_seed_block(recovery_adjacent_paddr, recovery_adjacent_seed);
    uintptr_t bad_vaddr = ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(bad_vaddr);
    TEST_ASSERT("an HS CBOM probe reports store access fault on a PMA=Device alias before the later valid recovery probe in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMA=Device CBOM probe leaves both the faulting block and its adjacent block unchanged before the later valid recovery probe",
        ai_micro_cmo_block_matches_seed(bad_base_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    TEST_ASSERT("the separate valid recovery block and its adjacent block also keep their seeded payloads after the earlier PMA=Device fault",
        ai_micro_cmo_block_matches_seed(recovery_base_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid tail-offset CBOM probe still retires normally after the earlier PMA=Device CBOM fault in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered CBOM target remains readable with its seeded payload after the earlier PMA=Device fault",
        recovered_head == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_adjacent = ld(recovery_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered CBOM target also remains readable with its original payload after the earlier PMA=Device fault",
        recovered_adjacent == recovery_adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the later valid CBOM probe preserves both the recovery block and its adjacent backing-memory block after the earlier PMA=Device fault",
        ai_micro_cmo_block_matches_seed(recovery_base_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_repeated_mmio_access_fault_stable_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t bad_base_paddr = ai_micro_cmo_mmio_backing_paddr() +
        AI_MICRO_CMO_BLOCK_OFFSET;
    const uintptr_t bad_adjacent_paddr = bad_base_paddr + block_size;
    const uintptr_t recovery_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_adjacent_vaddr = recovery_base_vaddr + block_size;
    const uintptr_t recovery_adjacent_paddr = recovery_base_paddr + block_size;
    const uintptr_t recovery_probe_vaddr = recovery_base_vaddr + 1U;
    const uint64_t bad_seed = 0x3141592653589793ULL;
    const uint64_t bad_adjacent_seed = 0x2718281828459045ULL;
    const uint64_t recovery_seed = 0x2468ace013579bdfULL;
    const uint64_t recovery_adjacent_seed = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(bad_base_paddr, bad_seed);
    ai_micro_cmo_seed_block(bad_adjacent_paddr, bad_adjacent_seed);
    ai_micro_cmo_seed_block(recovery_base_paddr, recovery_seed);
    ai_micro_cmo_seed_block(recovery_adjacent_paddr, recovery_adjacent_seed);
    uintptr_t bad_vaddr = ai_micro_cmo_prepare_mmio_alias(MENVCFG_CBZFE);
    const uintptr_t second_bad_vaddr = bad_vaddr + block_size - 1U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the first PMA=Device HS CBOM probe reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(second_bad_vaddr);
    TEST_ASSERT("the second PMA=Device HS CBOM probe also reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PMA=Device CBOM probes leave both the faulting block and its adjacent block unchanged before the later valid recovery probe",
        ai_micro_cmo_block_matches_seed(bad_base_paddr, bad_seed) &&
        ai_micro_cmo_block_matches_seed(bad_adjacent_paddr, bad_adjacent_seed)
    );
    TEST_ASSERT("the separate valid recovery block and its adjacent block also remain unchanged after repeated PMA=Device faults",
        ai_micro_cmo_block_matches_seed(recovery_base_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(recovery_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid subblock-offset CBOM probe still retires normally after repeated PMA=Device faults in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_head = ld(recovery_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered CBOM target remains readable with its seeded payload after repeated PMA=Device faults",
        recovered_head == recovery_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovered_adjacent = ld(recovery_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered CBOM target also remains readable with its original payload after repeated PMA=Device faults",
        recovered_adjacent == recovery_adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PMA=Device CBOM probes stay stable and the later valid CBOM probe still preserves both the recovery block and its adjacent backing-memory block",
        ai_micro_cmo_block_matches_seed(recovery_base_paddr, recovery_seed) &&
        ai_micro_cmo_block_matches_seed(recovery_adjacent_paddr, recovery_adjacent_seed)
    );
    ai_micro_cmo_reset_mmio_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_page_fault()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(VSI_GI) + AI_MICRO_CMO_BLOCK_OFFSET;

    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(vaddr);
    TEST_ASSERT("an HS CBOM-style instruction reports store page fault when translation resolves to a PTE.V=0 leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_page_fault_then_valid_probe_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t recovery_vaddr = target_base_vaddr + 1U;
    const uint64_t target_seed = 0x1234abcddcba4321ULL;
    const uint64_t adjacent_seed = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("an invalid-leaf HS CBOM probe reports store page fault before the later valid recovery probe in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the unrelated valid CBOM target block and its adjacent block keep their seeded payloads after the earlier page-faulting probe",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid subblock-offset CBOM probe retires normally after the earlier page-faulting probe in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered valid CBOM target remains readable with its seeded payload after the earlier page fault",
        preserved_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered valid CBOM target also remains readable with its original payload",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered valid CBOM probe preserves both the target block and its adjacent backing-memory block after the earlier page fault",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_repeated_page_fault_stable_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t recovery_vaddr = target_base_vaddr + block_size - 1U;
    const uint64_t target_seed = 0x3141592653589793ULL;
    const uint64_t adjacent_seed = 0x2718281828459045ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(bad_vaddr);
    TEST_ASSERT("the first invalid-leaf HS CBOM probe reports store page fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(bad_vaddr);
    TEST_ASSERT("the second invalid-leaf HS CBOM probe also reports store page fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated page-faulting CBOM probes leave both the valid target block and its adjacent block unchanged before the recovery probe",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid tail-offset CBOM probe still retires normally after repeated page faults in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered valid CBOM target remains readable with its seeded payload after repeated page faults",
        preserved_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered CBOM target also remains readable with its original payload after repeated page faults",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated page-faulting CBOM probes stay stable and the later valid CBOM probe still preserves both backing-memory blocks",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_tlb_access_fault()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        AI_MICRO_CMO_BLOCK_OFFSET;

    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(vaddr);
    TEST_ASSERT("an HS CBOM-style instruction reports store access fault when the upper-level PTE encoding overflows during translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(vaddr);
    TEST_ASSERT("an HS CBOM-style instruction also reports store access fault when the leaf PTE encoding overflows during translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_tlb_access_fault_then_valid_probe_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t recovery_vaddr = target_base_vaddr + 1U;
    const uint64_t target_seed = 0x6a5b4c3d2e1f8071ULL;
    const uint64_t adjacent_seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(target_base_vaddr);
    TEST_ASSERT("an upper-level-overflow HS CBOM probe reports store access fault before the later valid recovery probe in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_base_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the valid CBOM target block and its adjacent block remain unchanged after the earlier TLB access-faulting probe",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid subblock-offset CBOM probe retires normally after translation is restored in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered valid CBOM target remains readable with its seeded payload after the earlier TLB access fault",
        preserved_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered valid CBOM target also remains readable with its original payload after translation is restored",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered valid CBOM probe preserves both the target block and its adjacent backing-memory block after the earlier TLB access fault",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbom_repeated_tlb_access_fault_stable_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t second_probe_vaddr = target_base_vaddr + block_size - 1U;
    const uint64_t target_seed = 0xfedcba9876543210ULL;
    const uint64_t adjacent_seed = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(target_base_vaddr);
    TEST_ASSERT("the first leaf-overflow HS CBOM probe reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_base_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(second_probe_vaddr);
    TEST_ASSERT("the second leaf-overflow HS CBOM probe also reports store access fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_probe_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated TLB access-faulting CBOM probes leave both the target block and its adjacent block unchanged before translation is restored",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_flush(second_probe_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid tail-offset CBOM probe still retires normally after repeated TLB access faults in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered CBOM target remains readable with its seeded payload after repeated TLB access faults",
        preserved_target == target_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered CBOM target also remains readable with its original payload after repeated TLB access faults",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated TLB access-faulting CBOM probes stay stable and the later valid CBOM probe still preserves both backing-memory blocks",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbozero_page_fault()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(VSI_GI) + AI_MICRO_CMO_BLOCK_OFFSET;

    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(vaddr);
    TEST_ASSERT("an HS cbo.zero reports store page fault when translation resolves to a PTE.V=0 leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_page_fault_then_valid_zero_adjacent_block_preserved_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t recovery_vaddr = target_base_vaddr + block_size - 1U;
    const uint64_t target_seed = 0x8877665544332211ULL;
    const uint64_t adjacent_seed = 0x1029384756abcdefULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(bad_vaddr);
    TEST_ASSERT("an invalid-leaf HS cbo.zero reports store page fault before the later valid zeroing probe in the adjacent-block-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the valid cbo.zero target block and its adjacent block remain unchanged after the earlier page-faulting zero probe",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a tail-offset cbo.zero on a valid cache block retires normally after the earlier page-faulting zero probe in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the recovered cbo.zero retires without exception in the adjacent-block-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zeroed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered cbo.zero target reads back as zero after the earlier page fault",
        zeroed_target == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered cbo.zero target remains readable with its original payload",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cbo.zero clears only the target block and preserves the adjacent backing-memory block after the earlier page fault",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbozero_repeated_page_fault_stable_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 3U);
    const uintptr_t adjacent_base_vaddr = target_base_vaddr + block_size;
    const uintptr_t adjacent_base_paddr = target_base_paddr + block_size;
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t recovery_vaddr = target_base_vaddr + 1U;
    const uint64_t target_seed = 0x8877665544332211ULL;
    const uint64_t adjacent_seed = 0x1029384756abcdefULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    ai_micro_cmo_seed_block(adjacent_base_paddr, adjacent_seed);
    ai_micro_cmo_restore_pmp_full_access();
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(bad_vaddr);
    TEST_ASSERT("the first invalid-leaf HS cbo.zero reports store page fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(bad_vaddr);
    TEST_ASSERT("the second invalid-leaf HS cbo.zero also reports store page fault in the repeated-stable corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated page-faulting cbo.zero probes leave both the valid target block and its adjacent block unchanged before the recovery zeroing probe",
        ai_micro_cmo_block_matches_seed(target_base_paddr, target_seed) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a valid subblock-offset cbo.zero still retires normally after repeated page faults in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the recovered cbo.zero retires without exception in the repeated-stable corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t zeroed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the recovered cbo.zero target reads back as zero after repeated page faults",
        zeroed_target == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t preserved_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cache block adjacent to the recovered cbo.zero target remains readable with its original payload after repeated page faults",
        preserved_adjacent == adjacent_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated page-faulting cbo.zero probes stay stable and the later valid cbo.zero still clears only the target backing-memory block",
        ai_micro_cmo_block_all_zero(target_base_paddr) &&
        ai_micro_cmo_block_matches_seed(adjacent_base_paddr, adjacent_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_multiple_cbo_zero_same_window_no_merged_or_lost_flush_corner()
{
    TEST_START();

    const uintptr_t block_size = AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t target0_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target0_base_paddr = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 2U);
    const uintptr_t target1_base_vaddr = target0_base_vaddr + block_size;
    const uintptr_t target1_base_paddr = target0_base_paddr + block_size;
    const uintptr_t preserved_base_vaddr = target1_base_vaddr + block_size;
    const uintptr_t preserved_base_paddr = target1_base_paddr + block_size;
    const uintptr_t first_zero_vaddr = target0_base_vaddr + 1U;
    const uintptr_t second_zero_vaddr = target1_base_vaddr + block_size - 1U;
    const uint64_t target0_seed = 0x0123456789abcdefULL;
    const uint64_t target1_seed = 0x8877665544332211ULL;
    const uint64_t preserved_seed = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target0_base_paddr, target0_seed);
    ai_micro_cmo_seed_block(target1_base_paddr, target1_seed);
    ai_micro_cmo_seed_block(preserved_base_paddr, preserved_seed);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target0 = ld(target0_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first target cache block is readable before the multi-cbo.zero same-window probe runs",
        warmed_target0 == target0_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_target1 = ld(target1_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second target cache block is also readable before the multi-cbo.zero same-window probe runs",
        warmed_target1 == target1_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_preserved = ld(preserved_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the two cbo.zero targets is readable before the multi-cbo.zero same-window probe runs",
        warmed_preserved == preserved_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(first_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first cbo.zero in the same execution window retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(second_zero_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second cbo.zero to a different cache line in the same execution window also retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the two same-window cbo.zero operations retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target0_head = ld(target0_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the two same-window cbo.zero operations, the head of the first target cache block reads back as zero",
        observed_target0_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target0_tail = ld(target0_base_vaddr + block_size - sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the two same-window cbo.zero operations, the tail of the first target cache block also reads back as zero",
        observed_target0_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target1_head = ld(target1_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the two same-window cbo.zero operations, the head of the second target cache block reads back as zero",
        observed_target1_head == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target1_tail = ld(target1_base_vaddr + block_size - sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("after the two same-window cbo.zero operations, the tail of the second target cache block also reads back as zero",
        observed_target1_tail == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_preserved = ld(preserved_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the block adjacent to the two same-window cbo.zero targets remains readable with its original payload",
        observed_preserved == preserved_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the two same-window cbo.zero operations zero exactly their own backing-memory blocks while preserving the adjacent block",
        ai_micro_cmo_block_all_zero(target0_base_paddr) &&
        ai_micro_cmo_block_all_zero(target1_base_paddr) &&
        ai_micro_cmo_block_matches_seed(preserved_base_paddr, preserved_seed)
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    TEST_END();
}
