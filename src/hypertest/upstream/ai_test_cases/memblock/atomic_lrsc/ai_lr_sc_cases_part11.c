#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_same_address_intervening_doubleword_amo_failed_sc_clears_reservation_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t amo_old_value = amoadd_d(addr, amo_delta);
    uint64_t after_amo = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-address amoadd.d is inserted between lr.d and sc.d in the reservation-consumption corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-address amoadd.d itself observes and updates the reserved doubleword normally before the first sc.d retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("the first sc.d fails after the intervening same-address amoadd.d and preserves the intervening AMO result",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x8877665544332211ULL);
    TEST_ASSERT("after that failed sc.d, a second sc.d without a fresh lr.d still fails because the prior failed SC has already cleared the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_amo_sc_fails_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;
    const uint64_t refreshed_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t amo_old_value = amoadd_d(addr, amo_delta);
    uint64_t after_amo = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-address amoadd.d is inserted between lr.d and sc.d in the refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-address amoadd.d itself observes and updates the reserved doubleword normally before the first sc.d retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("the first sc.d fails after the intervening same-address amoadd.d and preserves the intervening AMO result",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x8877665544332211ULL);
    TEST_ASSERT("after that failed sc.d, a stale retry without a fresh lr.d fails because the prior failed SC has already cleared the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refresh_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after the stale retry fails, a fresh lr.d/sc.d pair re-establishes the reservation and succeeds",
        excpt.triggered == false &&
        refreshed_reserved == expected_after_amo &&
        refresh_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_word_amo_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint32_t amo_delta = 0x01010101U;
    const uint64_t expected_after_amo =
        ((uint64_t)(uint32_t)(initial_value >> 32) << 32) |
        (uint64_t)((uint32_t)initial_value + amo_delta);
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint32_t amo_old_value = amoadd_w(addr, amo_delta);
    uint64_t after_amo = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-start amoadd.w is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-start amoadd.w itself observes the low word and updates only the low 32-bit half before sc.d retires",
        excpt.triggered == false &&
        amo_old_value == (uint32_t)initial_value &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("on Spike, a same-start amoadd.w does not break the reservation and the following sc.d still succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_word_amo_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint32_t amo_delta = 0x01010101U;
    const uint64_t expected_after_amo =
        ((uint64_t)((uint32_t)(initial_value >> 32) + amo_delta) << 32) |
        (uint64_t)(uint32_t)initial_value;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint32_t amo_old_value = amoadd_w(upper_word_addr, amo_delta);
    uint64_t after_amo = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before an upper-word amoadd.w is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening upper-word amoadd.w itself observes the high word and updates only the upper 32-bit half before sc.d retires",
        excpt.triggered == false &&
        amo_old_value == (uint32_t)(initial_value >> 32) &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("an upper-word amoadd.w invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_store_sc_fails_preserves_store()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t alias_store_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the success-family alias pair intentionally keeps the virtual set index identical while remapping to the same physical backing page",
        ai_lr_sc_setidx(base_vaddr) == ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_vaddr, alias_store_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word store is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word store through the second virtual alias becomes visible through the first alias before sc.w retires",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening store invalidates the reservation, so the later same-alias sc.w successful",
        sc_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally contains the intervening sc.w payload after the alias-store corner sequence",
        read32(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_store_diff_setidx_fails()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t alias_store_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing word",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_vaddr, alias_store_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias store is issued",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias store becomes visible through the base alias before the later sc.w retires",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("for the different-setidx same-paddr alias path, the later same-alias sc.w is expected to fail and preserve the alias-store result",
        sc_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-store sequence, the shared backing word keeps the alias-store payload",
        read32(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_store_diff_setidx_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t alias_store_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing word",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_vaddr, alias_store_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t first_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias store is issued in the failed-sc-consumption corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias store still becomes visible before the first failing sc.w retires in the failed-sc-consumption corner",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w still fails after the different-setidx alias store and preserves the alias-store result",
        first_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(base_vaddr, 0xcafebabeU);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, retrying sc.w on the original alias without a fresh lr.w still fails because the failed sc.w has already consumed the reservation",
        second_status != 0U &&
        read32(alias_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-store failed-sc-consumption corner, the shared backing word keeps only the alias-store payload",
        read32(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_store_diff_setidx_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t alias_store_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x89abcdefU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing word",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_vaddr, alias_store_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t failed_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias store is issued in the original-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias store still becomes visible before the first failing sc.w retires in the original-refresh corner",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w fails and preserves the alias-store result on the different-setidx path",
        failed_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, a fresh lr.w on the original alias captures the alias-store value that remained in memory",
        refreshed_reserved == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original alias, the following sc.w succeeds and both aliases observe the refreshed payload",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-store failure-and-original-refresh corner, the shared backing word contains only the refreshed original-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_store_diff_setidx_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t alias_store_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x89abcdefU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing word",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_vaddr, alias_store_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t failed_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias store is issued in the switched-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias store still becomes visible before the first failing sc.w retires in the switched-refresh corner",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w fails and preserves the alias-store result on the different-setidx path",
        failed_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, a fresh lr.w on the switched alias captures the alias-store value that remained in memory",
        refreshed_reserved == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the switched alias, the following sc.w succeeds and both aliases observe the refreshed payload",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-store failure-and-switched-refresh corner, the shared backing word contains only the refreshed switched-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_store_sc_fails_preserves_store_and_adjacent_word_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t alias_store_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;
    const uint32_t adjacent_value = 0x89abcdefU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);
    write32(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the success-family alias pair intentionally keeps the virtual set index identical while remapping to the same physical backing page",
        ai_lr_sc_setidx(base_vaddr) == ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_vaddr, alias_store_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word store is issued through the second alias in the adjacent-word-preservation corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word store through the second virtual alias becomes visible through the first alias before sc.w retires in the adjacent-word-preservation corner",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-physical-word store through a different virtual alias invalidates the reservation, so the later same-alias sc.w successful in the adjacent-word-preservation corner",
        sc_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t observed_adjacent_base = read32(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word through the base alias remains unchanged across the alias-store-plus-sc.w sequence",
        observed_adjacent_base == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t observed_adjacent_alias = read32(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word through the remapped alias also remains unchanged across the alias-store-plus-sc.w sequence",
        observed_adjacent_alias == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing target word finally contains the intervening sc.w payload while the adjacent backing word remains unchanged after the alias-store corner sequence",
        read32(paddr) == alias_store_value &&
        read32(adjacent_paddr) == adjacent_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_store_failed_sc_clears_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t alias_store_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_vaddr, alias_store_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word store is issued through the second alias in the reservation-consumption corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word store through the second virtual alias becomes visible through the first alias before the first sc.w retires",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w fails after the intervening alias store and preserves the intervening store data",
        sc_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0xcafebabeU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a second sc.w through the other alias without a fresh lr.w still fails because the prior failed SC has already cleared the reservation",
        second_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word keeps the first successful sc.w payload after the alias-store reservation-consumption corner",
        read32(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_store_sc_fails_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t alias_store_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x89abcdefU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_vaddr, alias_store_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word store is issued through the second alias in the refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word store through the second virtual alias becomes visible through the first alias before the first sc.w retires",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w fails after the intervening alias store and preserves the intervening store data",
        sc_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0xcafebabeU);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, a stale sc.w through the other alias fails because the prior failed SC has already cleared the reservation",
        second_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refresh_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the stale alias retry fails, a fresh lr.w/sc.w pair on the original alias re-establishes the shared reservation and succeeds",
        refreshed_reserved == alias_store_value &&
        refresh_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally keeps the refreshed original-alias sc.w payload after the alias-store recovery corner",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_store_sc_fails_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t alias_store_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x89abcdefU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_vaddr, alias_store_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word store is issued through the second alias in the switched-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word store through the second virtual alias becomes visible through the first alias before the first sc.w retires",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w fails after the intervening alias store and preserves the intervening store data",
        sc_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0xcafebabeU);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, a stale sc.w through the other alias fails because the prior failed SC has already cleared the reservation",
        second_status != 0U &&
        read32(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refresh_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the stale alias retry fails, a fresh lr.w/sc.w pair on the switched alias re-establishes the shared reservation and succeeds",
        refreshed_reserved == alias_store_value &&
        refresh_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally keeps the refreshed switched-alias sc.w payload after the alias-store recovery corner",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_halfword_store_before_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint16_t low_halfword_value = 0x33ccU;
    const uint32_t expected_after_store =
        ((uint32_t)(initial_value & 0xffff0000U)) | (uint32_t)low_halfword_value;
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write16(alias_vaddr, low_halfword_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-start low-halfword store is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-start low-halfword store through the second alias becomes visible as a partial update of the shared word before sc.w retires",
        after_alias_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-start low-halfword store through a different alias invalidates the reservation, so the later same-alias sc.w successful",
        sc_status != 0U &&
        read32(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias low-halfword store corner, the shared backing word finally contains the intervening sc.w payload",
        read32(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_halfword_store_before_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_halfword_vaddr = alias_vaddr + sizeof(uint16_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint16_t upper_halfword_value = 0x55aaU;
    const uint32_t expected_after_store =
        ((uint32_t)upper_halfword_value << 16) | (uint32_t)(initial_value & 0x0000ffffU);
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write16(alias_upper_halfword_vaddr, upper_halfword_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before an upper-halfword store is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the upper-halfword store through the second alias becomes visible as a partial update of the shared word before sc.w retires",
        after_alias_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("an upper-halfword store through a different alias invalidates the reservation, so the later same-alias sc.w successful",
        sc_status != 0U &&
        read32(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-halfword store corner, the shared backing word finally contains the intervening sc.w payload",
        read32(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_store_before_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint8_t low_byte_value = 0x33U;
    const uint32_t expected_after_store =
        ((uint32_t)(initial_value & 0xffffff00U)) | (uint32_t)low_byte_value;
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write8(alias_vaddr, low_byte_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-start low-byte store is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-start low-byte store through the second alias becomes visible as a one-byte update of the shared word before sc.w retires",
        after_alias_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-start low-byte store through a different alias invalidates the reservation, so the later same-alias sc.w successful",
        sc_status != 0U &&
        read32(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias low-byte store corner, the shared backing word finally contains the intervening sc.w payload",
        read32(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_before_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 1U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint8_t byte_value = 0x33U;
    const uint32_t expected_after_store =
        (initial_value & ~(0xffU << 8)) |
        ((uint32_t)byte_value << 8);
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write8(alias_byte_vaddr, byte_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a byte store at offset one is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the byte store at offset one through the second alias becomes visible as an interior one-byte update of the shared word before sc.w retires",
        after_alias_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("an interior byte store at offset one through a different alias invalidates the reservation, so the later same-alias sc.w successful",
        sc_status != 0U &&
        read32(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias byte-at-offset-one store corner, the shared backing word finally contains the intervening sc.w payload",
        read32(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_before_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 2U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint8_t byte_value = 0x55U;
    const uint32_t expected_after_store =
        (initial_value & ~(0xffU << 16)) |
        ((uint32_t)byte_value << 16);
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write8(alias_byte_vaddr, byte_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a byte store at offset two is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the byte store at offset two through the second alias becomes visible as an interior one-byte update of the shared word before sc.w retires",
        after_alias_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("an interior byte store at offset two through a different alias invalidates the reservation, so the later same-alias sc.w successful",
        sc_status != 0U &&
        read32(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias byte-at-offset-two store corner, the shared backing word finally contains the intervening sc.w payload",
        read32(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_store_before_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_tail_byte_vaddr = alias_vaddr + sizeof(uint32_t) - 1U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint8_t tail_byte_value = 0x55U;
    const uint32_t expected_after_store =
        ((uint32_t)tail_byte_value << 24) | (uint32_t)(initial_value & 0x00ffffffU);
    const uint32_t committed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write8(alias_tail_byte_vaddr, tail_byte_value);
    uint32_t after_alias_store = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a tail-byte store is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the tail-byte store through the second alias becomes visible as a one-byte update of the shared word before sc.w retires",
        after_alias_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("a tail-byte store through a different alias invalidates the reservation, so the later same-alias sc.w successful",
        sc_status != 0U &&
        read32(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias tail-byte store corner, the shared backing word finally contains the intervening sc.w payload",
        read32(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_store_before_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint32_t upper_word_value = 0x55aa33ccU;
    const uint64_t expected_after_store =
        ((uint64_t)upper_word_value << 32) |
        (uint64_t)(uint32_t)initial_value;
    const uint32_t committed_value = 0x13579bdfU;
    const uint64_t expected_final =
        ((uint64_t)upper_word_value << 32) |
        (uint64_t)committed_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    write32(alias_upper_word_vaddr, upper_word_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original low word before a same-doubleword upper-word store is issued through the second alias",
        reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-doubleword upper-word store through the second alias becomes visible as a non-overlapping shared high-half update before the low-word sc.w retires",
        after_alias_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-doubleword but non-overlapping upper-word store through a different alias invalidates the low-word reservation, so the later same-alias sc.w fails and preserves the intervening update",
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-word store before low-word sc.w corner, the shared backing doubleword finally keeps the store-updated upper word and the successful low-word sc.w payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_halfword_store_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x33ccU;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffffULL << 32)) |
        ((uint64_t)halfword_value << 32);
    const uint32_t committed_value = 0x13579bdfU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)initial_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_upper_word_vaddr);
    write16(alias_upper_word_vaddr, halfword_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a same-start low-halfword store is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the same-start low-halfword store through the second alias becomes visible as a low-edge partial update of the shared upper word before sc.w retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a same-start low-halfword store through a different alias inside the reserved upper word invalidates the reservation, so the later same-alias sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start low-halfword upper-word store corner, the shared backing doubleword finally keeps the intervening sc.w payload in its high 32-bit half",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_halfword_store_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t alias_upper_halfword_vaddr = alias_upper_word_vaddr + sizeof(uint16_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x55aaU;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffffULL << 48)) |
        ((uint64_t)halfword_value << 48);
    const uint32_t committed_value = 0x13579bdfU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)initial_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_upper_word_vaddr);
    write16(alias_upper_halfword_vaddr, halfword_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before an upper-halfword store is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the upper-halfword store through the second alias becomes visible as a high-edge partial update of the shared upper word before sc.w retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("an upper-halfword store through a different alias inside the reserved upper word invalidates the reservation, so the later same-alias sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-halfword upper-word store corner, the shared backing doubleword finally keeps the intervening sc.w payload in its high 32-bit half",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_store_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x33U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 32)) |
        ((uint64_t)byte_value << 32);
    const uint32_t committed_value = 0x13579bdfU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)initial_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_upper_word_vaddr);
    write8(alias_upper_word_vaddr, byte_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a same-start low-byte store is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the same-start low-byte store through the second alias becomes visible as a low-edge one-byte update of the shared upper word before sc.w retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a same-start low-byte store through a different alias inside the reserved upper word invalidates the reservation, so the later same-alias sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start low-byte upper-word store corner, the shared backing doubleword finally keeps the intervening sc.w payload in its high 32-bit half",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t alias_byte_vaddr = alias_upper_word_vaddr + 1U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x44U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 40)) |
        ((uint64_t)byte_value << 40);
    const uint32_t committed_value = 0x13579bdfU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)initial_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_upper_word_vaddr);
    write8(alias_byte_vaddr, byte_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a byte store at offset one is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the byte store at offset one through the second alias becomes visible as a first interior one-byte update of the shared upper word before sc.w retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset one through a different alias inside the reserved upper word invalidates the reservation, so the later same-alias sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-one upper-word byte-store corner, the shared backing doubleword finally keeps the intervening sc.w payload in its high 32-bit half",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t alias_byte_vaddr = alias_upper_word_vaddr + 2U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x66U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 48)) |
        ((uint64_t)byte_value << 48);
    const uint32_t committed_value = 0x13579bdfU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)initial_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_upper_word_vaddr);
    write8(alias_byte_vaddr, byte_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a byte store at offset two is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the byte store at offset two through the second alias becomes visible as a second interior one-byte update of the shared upper word before sc.w retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset two through a different alias inside the reserved upper word invalidates the reservation, so the later same-alias sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-two upper-word byte-store corner, the shared backing doubleword finally keeps the intervening sc.w payload in its high 32-bit half",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_store_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t alias_tail_byte_vaddr = alias_upper_word_vaddr + sizeof(uint32_t) - 1U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x77U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 56)) |
        ((uint64_t)byte_value << 56);
    const uint32_t committed_value = 0x13579bdfU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)initial_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_upper_word_vaddr);
    write8(alias_tail_byte_vaddr, byte_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a tail-byte store is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the tail-byte store through the second alias becomes visible as a high-edge one-byte update of the shared upper word before sc.w retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a tail-byte store through a different alias inside the reserved upper word invalidates the reservation, so the later same-alias sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias tail-byte upper-word store corner, the shared backing doubleword finally keeps the intervening sc.w payload in its high 32-bit half",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_word_store_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint32_t low_word_value = 0x55aa33ccU;
    const uint64_t expected_after_store =
        ((uint64_t)(uint32_t)(initial_value >> 32) << 32) |
        (uint64_t)low_word_value;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)low_word_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_upper_word_vaddr);
    write32(alias_vaddr, low_word_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original upper word before a same-doubleword low-word store is issued through the second alias",
        reserved == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-doubleword low-word store through the second alias becomes visible as a non-overlapping shared low-half update before the upper-word sc.w retires",
        after_alias_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-doubleword but non-overlapping low-word store through a different alias invalidates the upper-word reservation, so the later same-alias sc.w fails and preserves the intervening update",
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias low-word upper-window store corner, the shared backing doubleword finally keeps the store-updated low word and the successful upper-word sc.w payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_store_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint32_t upper_word_value = 0x55aa33ccU;
    const uint64_t expected_after_store =
        ((uint64_t)upper_word_value << 32) |
        (uint64_t)(uint32_t)initial_value;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)initial_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_upper_word_vaddr);
    write32(alias_upper_word_vaddr, upper_word_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original upper word before an overlapping upper-word store is issued through the second alias",
        reserved == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("the overlapping upper-word store through the second alias becomes visible as a shared high-half update before the later upper-word sc.w retires",
        after_alias_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("an overlapping upper-word store through a different alias invalidates the reservation, so the later same-alias upper-word sc.w fails and preserves the intervening update",
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-word upper-window store corner, the shared backing doubleword finally keeps the intervening upper-word sc.w payload while preserving the original low word",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_amo_still_succeeds()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x10203040U;
    const uint32_t amo_delta = 0x01010101U;
    const uint32_t expected_after_amo = initial_value + amo_delta;
    const uint32_t committed_value = 0xa5a55a5aU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the success-family alias pair intentionally keeps the virtual set index identical while remapping to the same physical backing page",
        ai_lr_sc_setidx(base_vaddr) == ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    uint32_t amo_old_value = amoadd_w(alias_vaddr, amo_delta);
    uint32_t after_alias_amo = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word AMO is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word amoadd.w through the second virtual alias observes the old value and updates the shared word before sc.w retires",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("on the same-setidx same-paddr alias path, the intervening AMO still leaves the later same-alias sc.w successful",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally contains the successful sc.w payload after the alias-AMO corner sequence",
        read32(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_amo_diff_setidx_fails()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x10203040U;
    const uint32_t amo_delta = 0x01010101U;
    const uint32_t expected_after_amo = initial_value + amo_delta;
    const uint32_t committed_value = 0xa5a55a5aU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing word",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    uint32_t amo_old_value = amoadd_w(alias_vaddr, amo_delta);
    uint32_t after_alias_amo = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias AMO is issued",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias amoadd.w still observes the old value and updates the shared word before the later sc.w retires",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("for the different-setidx same-paddr alias path, the later same-alias sc.w is expected to fail and preserve the alias-AMO result",
        sc_status != 0U &&
        read32(base_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-AMO sequence, the shared backing word keeps the alias-AMO updated payload",
        read32(paddr) == expected_after_amo
    );

    TEST_END();
}
