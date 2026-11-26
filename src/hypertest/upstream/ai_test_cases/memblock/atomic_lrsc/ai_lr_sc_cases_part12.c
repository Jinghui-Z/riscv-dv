#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_same_paddr_alias_intervening_amo_diff_setidx_failed_sc_consumes_reservation_corner()
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
    uint32_t first_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias AMO is issued in the failed-sc-consumption corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias amoadd.w still updates the shared word before the first failing sc.w retires in the failed-sc-consumption corner",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w still fails after the different-setidx alias AMO and preserves the AMO result",
        first_status != 0U &&
        read32(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(base_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, retrying sc.w on the original alias without a fresh lr.w still fails because the failed sc.w has already consumed the reservation",
        second_status != 0U &&
        read32(alias_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-amoadd.w failed-sc-consumption corner, the shared backing word keeps only the alias-AMO payload",
        read32(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_amo_diff_setidx_fail_then_refresh_original_succeeds_corner()
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
    const uint32_t refreshed_value = 0x2468ace0U;

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
    uint32_t failed_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias AMO is issued in the original-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias amoadd.w still updates the shared word before the first failing sc.w retires in the original-refresh corner",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w fails and preserves the alias-AMO result on the different-setidx path",
        failed_status != 0U &&
        read32(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, a fresh lr.w on the original alias captures the alias-AMO value that remained in memory",
        refreshed_reserved == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original alias, the following sc.w succeeds and both aliases observe the refreshed payload",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-amoadd.w failure-and-original-refresh corner, the shared backing word contains only the refreshed original-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_amo_diff_setidx_fail_then_refresh_switched_succeeds_corner()
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
    const uint32_t refreshed_value = 0x2468ace0U;

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
    uint32_t failed_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias AMO is issued in the switched-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias amoadd.w still updates the shared word before the first failing sc.w retires in the switched-refresh corner",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w fails and preserves the alias-AMO result on the different-setidx path",
        failed_status != 0U &&
        read32(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, a fresh lr.w on the switched alias captures the alias-AMO value that remained in memory",
        refreshed_reserved == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the switched alias, the following sc.w succeeds and both aliases observe the refreshed payload",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-amoadd.w failure-and-switched-refresh corner, the shared backing word contains only the refreshed switched-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_amo_success_consumes_reservation_corner()
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

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    uint32_t amo_old_value = amoadd_w(alias_vaddr, amo_delta);
    uint32_t after_alias_amo = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word AMO is issued through the second alias in the reservation-consumption corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word amoadd.w through the second virtual alias observes the old value and updates the shared word before the first sc.w retires",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w still succeeds after the intervening alias AMO and overwrites the AMO result",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a second sc.w through the other alias without a fresh lr.w still fails because the successful shared reservation has already been consumed",
        second_status != 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word keeps the first successful sc.w payload after the alias-AMO reservation-consumption corner",
        read32(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_amo_fail_then_refresh_original_succeeds_corner()
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
    const uint32_t refreshed_value = 0x2468ace0U;

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
    uint32_t amo_old_value = amoadd_w(alias_vaddr, amo_delta);
    uint32_t after_alias_amo = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word AMO is issued through the second alias in the original-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word amoadd.w through the second virtual alias observes the old value and updates the shared word before the first sc.w retires",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w still succeeds after the intervening alias AMO and overwrites the AMO result",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a stale sc.w through the other alias fails because the successful shared reservation has already been consumed",
        second_status != 0U &&
        read32(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refresh_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the stale alias retry fails, a fresh lr.w/sc.w pair on the original alias re-establishes the shared reservation and succeeds",
        refreshed_reserved == committed_value &&
        refresh_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally keeps the refreshed original-alias sc.w payload after the alias-amoadd.w recovery corner",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_amo_fail_then_refresh_switched_succeeds_corner()
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
    const uint32_t refreshed_value = 0x2468ace0U;

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
    uint32_t amo_old_value = amoadd_w(alias_vaddr, amo_delta);
    uint32_t after_alias_amo = read32(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word AMO is issued through the second alias in the switched-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word amoadd.w through the second virtual alias observes the old value and updates the shared word before the first sc.w retires",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w still succeeds after the intervening alias AMO and overwrites the AMO result",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a stale sc.w through the other alias fails because the successful shared reservation has already been consumed",
        second_status != 0U &&
        read32(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refresh_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the stale alias retry fails, a fresh lr.w/sc.w pair on the switched alias re-establishes the shared reservation and succeeds",
        refreshed_reserved == committed_value &&
        refresh_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally keeps the refreshed switched-alias sc.w payload after the alias-amoadd.w recovery corner",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_amo_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint32_t amo_delta = 0x01010101U;
    const uint64_t expected_after_amo =
        ((uint64_t)((uint32_t)(initial_value >> 32) + amo_delta) << 32) |
        (uint64_t)(uint32_t)initial_value;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)(uint32_t)(expected_after_amo >> 32) << 32) |
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
    uint32_t amo_old_value = amoadd_w(alias_upper_word_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original low word before a same-doubleword upper-word amoadd.w is issued through the second alias",
        reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-doubleword upper-word amoadd.w through the second alias observes the old shared high word and updates only the non-overlapping upper 32-bit half before the low-word sc.w retires",
        amo_old_value == (uint32_t)(initial_value >> 32) &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-doubleword but non-overlapping upper-word amoadd.w through a different alias still does not break the low-word reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-word amoadd.w before low-word sc.w corner, the shared backing doubleword finally keeps the AMO-updated upper word and the successful low-word sc.w payload",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_word_amo_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint32_t amo_delta = 0x01010101U;
    const uint64_t expected_after_amo =
        ((uint64_t)(uint32_t)(initial_value >> 32) << 32) |
        (uint64_t)((uint32_t)initial_value + amo_delta);
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)expected_after_amo;

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
    uint32_t amo_old_value = amoadd_w(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original upper word before a same-doubleword low-word amoadd.w is issued through the second alias",
        reserved == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-doubleword low-word amoadd.w through the second alias observes the old low word and updates only the non-overlapping shared low 32-bit half before the upper-word sc.w retires",
        amo_old_value == (uint32_t)initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-doubleword but non-overlapping low-word amoadd.w through a different alias invalidates the upper-word reservation, so the later same-alias sc.w fails and preserves the intervening update",
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias low-word amoadd.w before upper-word sc.w corner, the shared backing doubleword finally keeps the AMO-updated low word and the successful upper-word sc.w payload",
        read64(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_amo_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint32_t amo_delta = 0x01010101U;
    const uint64_t expected_after_amo =
        ((uint64_t)((uint32_t)(initial_value >> 32) + amo_delta) << 32) |
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
    uint32_t amo_old_value = amoadd_w(alias_upper_word_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original upper word before an overlapping upper-word amoadd.w is issued through the second alias",
        reserved == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("the overlapping upper-word amoadd.w through the second alias observes the old shared upper word and updates only that reserved high 32-bit half before sc.w retires",
        amo_old_value == (uint32_t)(initial_value >> 32) &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("an overlapping upper-word amoadd.w through a different alias invalidates the reservation, so the later same-alias upper-word sc.w fails and preserves the intervening update",
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias overlapping upper-word amoadd.w corner, the shared backing doubleword finally keeps the intervening upper-word sc.w payload while preserving the original low word",
        read64(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_before_low_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)(uint32_t)(expected_after_amo >> 32) << 32) |
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
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original low word before a same-start amoadd.d is issued through the second alias",
        reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-start amoadd.d through the second alias observes the old doubleword and updates both 32-bit halves before sc.w retires",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-start amoadd.d through a different alias still does not break the reservation and the later same-alias low-word sc.w succeeds",
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start amoadd.d corner, the shared backing doubleword finally keeps the successful low-word sc.w payload and the AMO-updated upper word",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)expected_after_amo;

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
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original upper word before an overlapping amoadd.d is issued through the second alias",
        reserved == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("the overlapping amoadd.d through the second alias observes the old doubleword and updates both 32-bit halves before the upper-word sc.w retires",
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("an overlapping amoadd.d through a different alias invalidates the reservation, so the later same-alias upper-word sc.w fails and preserves the intervening update",
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias overlapping amoadd.d corner, the shared backing doubleword finally keeps the AMO-updated low word and the successful upper-word sc.w payload",
        read64(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_second_sc_without_new_lr_fails()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t first_committed = 0x1122aabbU;

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
    uint32_t first_status = sc_w(base_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before the reservation-consumption alias corner is exercised",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w through the original alias succeeds and consumes the live reservation on the shared physical word",
        first_status == 0U &&
        read32(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x55aa33ccU);
    AI_ASSERT_NO_EXCEPT_AND("without a new lr.w, a second sc.w through the other alias also fails because the earlier success has already consumed the shared reservation",
        second_status != 0U &&
        read32(base_vaddr) == first_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word keeps the first successful sc.w payload after the second alias sc.w fails",
        read32(paddr) == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_second_doubleword_sc_without_new_lr_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t first_committed = 0x1122aabbccddeeffULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before the reservation-consumption doubleword alias corner is exercised",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first sc.d through the original alias succeeds and consumes the live reservation on the shared physical doubleword",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x55aa33cc77ee11ffULL);
    TEST_ASSERT("without a new lr.d, a second sc.d through the other alias also fails because the earlier success has already consumed the shared reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == first_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword keeps the first successful sc.d payload after the second alias sc.d fails",
        read64(paddr) == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_second_sc_without_new_lr_fails()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t first_committed = 0x1122aabbU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t first_status = sc_w(addr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before the reservation-consumption corner is exercised",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w succeeds and consumes the live reservation",
        first_status == 0U &&
        ai_lr_sc_words[0] == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0x55aa33ccU);
    AI_ASSERT_NO_EXCEPT_AND("without a new lr.w, a second sc.w fails because the earlier success has already consumed the reservation",
        second_status != 0U &&
        ai_lr_sc_words[0] == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_second_doubleword_sc_without_new_lr_fails_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t first_committed = 0x1122aabbccddeeffULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, first_committed);
    TEST_ASSERT("lr.d captures the original doubleword before the reservation-consumption doubleword corner is exercised",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first sc.d succeeds and consumes the live reservation",
        excpt.triggered == false &&
        first_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x55aa33cc77ee11ffULL);
    TEST_ASSERT("without a new lr.d, a second sc.d fails because the earlier success has already consumed the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_second_sc_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t first_committed = 0x1122aabbU;
    const uint32_t refreshed_value = 0x55aa33ccU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t first_status = sc_w(addr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before the reservation-consumption-and-refresh corner is exercised",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w succeeds and consumes the live reservation before the refresh sequence starts",
        first_status == 0U &&
        ai_lr_sc_words[0] == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("without a new lr.w, the second sc.w still fails because the earlier success has already consumed the reservation",
        second_status != 0U &&
        ai_lr_sc_words[0] == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that consumed-reservation failure, a fresh lr.w still captures the first committed word",
        refreshed_reserved == first_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation with a new lr.w, the following sc.w succeeds and overwrites the earlier payload",
        refreshed_status == 0U &&
        ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_second_doubleword_sc_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t first_committed = 0x1122aabbccddeeffULL;
    const uint64_t refreshed_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, first_committed);
    TEST_ASSERT("lr.d captures the original doubleword before the reservation-consumption-and-refresh doubleword corner is exercised",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first sc.d succeeds and consumes the live reservation before the doubleword refresh sequence starts",
        excpt.triggered == false &&
        first_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("without a new lr.d, the second sc.d still fails because the earlier success has already consumed the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after that consumed-reservation doubleword failure, a fresh lr.d still captures the first committed doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_committed
    );
    TEST_ASSERT("after refreshing the reservation with a new lr.d, the following sc.d succeeds and overwrites the earlier payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_second_sc_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t first_committed = 0x1122aabbU;
    const uint32_t refreshed_value = 0x55aa33ccU;

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
    uint32_t first_status = sc_w(base_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before the alias reservation-consumption-and-refresh corner is exercised",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w through the original alias succeeds and consumes the shared reservation before the alias refresh sequence starts",
        first_status == 0U &&
        read32(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("without a new lr.w, the second sc.w through the other alias still fails because the earlier success has already consumed the shared reservation",
        second_status != 0U &&
        read32(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that consumed-reservation alias failure, a fresh lr.w through the other alias still captures the first committed word",
        refreshed_reserved == first_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the shared reservation through the other alias, the following alias sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally contains the refreshed alias sc.w payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_second_sc_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x2468ace0U;
    const uint32_t first_committed = 0x1122aabbU;
    const uint32_t refreshed_value = 0x55aa33ccU;

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
    uint32_t first_status = sc_w(base_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before the original-alias refresh-after-consumption corner is exercised",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w through the original alias succeeds and consumes the shared reservation before the original-alias refresh sequence starts",
        first_status == 0U &&
        read32(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("without a new lr.w, the second sc.w through the other alias still fails because the earlier success has already consumed the shared reservation",
        second_status != 0U &&
        read32(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that consumed-reservation alias failure, a fresh lr.w through the original alias still captures the first committed word",
        refreshed_reserved == first_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the shared reservation through the original alias, the following original-alias sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally contains the refreshed original-alias sc.w payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_second_doubleword_sc_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t first_committed = 0x1122aabbccddeeffULL;
    const uint64_t refreshed_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before the alias reservation-consumption-and-refresh doubleword corner is exercised",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first sc.d through the original alias succeeds and consumes the shared reservation before the alias doubleword refresh sequence starts",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("without a new lr.d, the second sc.d through the other alias still fails because the earlier success has already consumed the shared reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that consumed-reservation alias doubleword failure, a fresh lr.d through the other alias still captures the first committed doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_committed
    );
    TEST_ASSERT("after refreshing the shared doubleword reservation through the other alias, the following alias sc.d succeeds and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally contains the refreshed alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_second_doubleword_sc_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t first_committed = 0x1122aabbccddeeffULL;
    const uint64_t refreshed_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before the original-alias doubleword refresh-after-consumption corner is exercised",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first sc.d through the original alias succeeds and consumes the shared reservation before the original-alias doubleword refresh sequence starts",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("without a new lr.d, the second sc.d through the other alias still fails because the earlier success has already consumed the shared reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that consumed-reservation alias doubleword failure, a fresh lr.d through the original alias still captures the first committed doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_committed
    );
    TEST_ASSERT("after refreshing the shared doubleword reservation through the original alias, the following original-alias sc.d succeeds and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally contains the refreshed original-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_original_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x778899aaU;
    const uint32_t blocked_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x2468ace0U;

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
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias captures the shared backing word before the repeated alias-sc.w store-trigger corner runs",
        reserved == initial_value
    );

    ai_lr_sc_arm_mcontrol6_0(alias_vaddr, AI_LR_SC_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sc_w(alias_vaddr, blocked_value);
    TEST_ASSERT("with the store trigger armed, the first alias-mismatched sc.w reports breakpoint and leaves the shared word unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(paddr) == initial_value
    );
    ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sc_w(alias_vaddr, blocked_value ^ 0x01010101U);
    TEST_ASSERT("with the store trigger still armed, the repeated alias-mismatched sc.w reports breakpoint and still leaves the shared word unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(paddr) == initial_value
    );

    ai_lr_sc_restore_machine_status(saved_mstatus);
    ai_lr_sc_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, blocked_value);
    AI_ASSERT_NO_EXCEPT_AND("after clearing the store trigger, the same alias-mismatched sc.w falls back to its normal failure behavior and still leaves the shared word unchanged",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated trigger hits and the post-clear mismatch failure, a fresh lr.w on the original alias still captures the untouched shared word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original alias, the following sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias-sc.w store-trigger corner finally updates only the shared backing word with the refreshed original-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_switched_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x55667788U;
    const uint32_t blocked_value = 0x89abcdefU;
    const uint32_t refreshed_value = 0x2468ace0U;

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
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias captures the shared backing word before the repeated alias-sc.w store-trigger switched-refresh corner runs",
        reserved == initial_value
    );

    ai_lr_sc_arm_mcontrol6_0(alias_vaddr, AI_LR_SC_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sc_w(alias_vaddr, blocked_value);
    TEST_ASSERT("with the store trigger armed in the switched-refresh corner, the first alias-mismatched sc.w reports breakpoint and leaves the shared word unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(paddr) == initial_value
    );
    ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sc_w(alias_vaddr, blocked_value ^ 0x01010101U);
    TEST_ASSERT("with the store trigger still armed in the switched-refresh corner, the repeated alias-mismatched sc.w reports breakpoint and leaves the shared word unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(paddr) == initial_value
    );

    ai_lr_sc_restore_machine_status(saved_mstatus);
    ai_lr_sc_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, blocked_value);
    AI_ASSERT_NO_EXCEPT_AND("after clearing the store trigger, the same alias-mismatched sc.w still falls back to its normal failure behavior in the switched-refresh corner",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated trigger hits and the post-clear mismatch failure, a fresh lr.w on the switched alias still captures the untouched shared word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the switched alias, the following sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias-sc.w store-trigger switched-refresh corner finally updates only the shared backing word with the refreshed switched-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_execute_and_store_trigger_repeated_breakpoint_then_original_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uintptr_t trigger_pc = (uintptr_t)ai_lr_sc_sc_w_helper;
    const uint32_t initial_value = 0x99aabbccU;
    const uint32_t blocked_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x2468ace0U;

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
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias captures the shared backing word before the dual-trigger alias-sc.w corner runs",
        reserved == initial_value
    );

    ai_lr_sc_clear_mcontrol6_slot(0);
    ai_lr_sc_clear_mcontrol6_slot(1);
    ai_lr_sc_arm_mcontrol6_slot(0, trigger_pc, AI_LR_SC_MCONTROL6_EXECUTE);
    ai_lr_sc_arm_mcontrol6_slot(1, alias_vaddr, AI_LR_SC_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_w_helper(alias_vaddr, blocked_value);
    TEST_ASSERT("with both execute-PC and store triggers armed, the first alias-mismatched sc.w helper call reports breakpoint before the shared word changes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_w_helper(alias_vaddr, blocked_value ^ 0x01010101U);
    TEST_ASSERT("with both trigger slots still armed, the second alias-mismatched sc.w helper call also reports breakpoint before the shared word changes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(base_vaddr) == initial_value
    );
    ai_lr_sc_restore_machine_status(saved_mstatus);

    ai_lr_sc_clear_mcontrol6_slot(0);
    saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_w_helper(alias_vaddr, blocked_value);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining store trigger still reports breakpoint on the same alias-mismatched sc.w helper call",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(base_vaddr) == initial_value
    );
    ai_lr_sc_restore_machine_status(saved_mstatus);

    ai_lr_sc_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, blocked_value);
    AI_ASSERT_NO_EXCEPT_AND("after clearing both trigger slots, the same alias-mismatched sc.w falls back to its normal failure behavior in the dual-trigger corner",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated dual-trigger hits and the post-clear mismatch failure, a fresh lr.w on the original alias still captures the untouched shared word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original alias, the following sc.w succeeds and both aliases observe the new payload in the dual-trigger corner",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the dual-trigger alias-sc.w corner finally updates only the shared backing word with the refreshed original-alias payload",
        read32(paddr) == refreshed_value
    );

    ai_lr_sc_clear_mcontrol6_slot(0);
    ai_lr_sc_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_original_refresh_adjacent_word_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uint32_t initial_value = 0x778899aaU;
    const uint32_t blocked_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x2468ace0U;
    const uint32_t adjacent_value = 0x55aa33ccU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);
    write32(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias captures the shared backing word before the adjacent-word-preservation store-trigger corner runs",
        reserved == initial_value
    );

    ai_lr_sc_arm_mcontrol6_0(alias_vaddr, AI_LR_SC_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sc_w(alias_vaddr, blocked_value);
    TEST_ASSERT("with the store trigger armed, the first alias-mismatched sc.w reports breakpoint and leaves the target and adjacent words unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(paddr) == initial_value &&
        read32(adjacent_paddr) == adjacent_value
    );
    ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sc_w(alias_vaddr, blocked_value ^ 0x01010101U);
    TEST_ASSERT("with the store trigger still armed, the repeated alias-mismatched sc.w reports breakpoint and leaves the target and adjacent words unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(paddr) == initial_value &&
        read32(adjacent_paddr) == adjacent_value
    );

    ai_lr_sc_restore_machine_status(saved_mstatus);
    ai_lr_sc_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, blocked_value);
    AI_ASSERT_NO_EXCEPT_AND("after clearing the store trigger, the same alias-mismatched sc.w falls back to its normal failure behavior while still leaving the target word and the adjacent word unchanged",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated trigger hits and the post-clear mismatch failure, a fresh lr.w on the original alias still captures the untouched shared word in the adjacent-word-preservation corner",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original alias, the following sc.w succeeds while the adjacent word still remains unchanged",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t observed_adjacent_alias = read32(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word through the switched alias also remains unchanged across the repeated-trigger-and-refresh sequence",
        observed_adjacent_alias == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias-sc.w store-trigger adjacent-word-preservation corner finally updates only the shared target word while keeping the adjacent backing word unchanged",
        read32(paddr) == refreshed_value &&
        read32(adjacent_paddr) == adjacent_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_switched_refresh_adjacent_word_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uint32_t initial_value = 0x55667788U;
    const uint32_t blocked_value = 0x89abcdefU;
    const uint32_t refreshed_value = 0x2468ace0U;
    const uint32_t adjacent_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);
    write32(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias captures the shared backing word before the switched-refresh adjacent-word-preservation store-trigger corner runs",
        reserved == initial_value
    );

    ai_lr_sc_arm_mcontrol6_0(alias_vaddr, AI_LR_SC_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sc_w(alias_vaddr, blocked_value);
    TEST_ASSERT("with the store trigger armed in the switched-refresh adjacent-word corner, the first alias-mismatched sc.w reports breakpoint and leaves both words unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(paddr) == initial_value &&
        read32(adjacent_paddr) == adjacent_value
    );
    ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sc_w(alias_vaddr, blocked_value ^ 0x01010101U);
    TEST_ASSERT("with the store trigger still armed in the switched-refresh adjacent-word corner, the repeated alias-mismatched sc.w reports breakpoint and leaves both words unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(paddr) == initial_value &&
        read32(adjacent_paddr) == adjacent_value
    );

    ai_lr_sc_restore_machine_status(saved_mstatus);
    ai_lr_sc_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, blocked_value);
    AI_ASSERT_NO_EXCEPT_AND("after clearing the store trigger, the same alias-mismatched sc.w still falls back to its normal failure behavior in the switched-refresh adjacent-word-preservation corner while leaving both words unchanged",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated trigger hits and the post-clear mismatch failure, a fresh lr.w on the switched alias still captures the untouched shared word in the switched-refresh adjacent-word-preservation corner",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the switched alias, the following sc.w succeeds while the adjacent word still remains unchanged",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value &&
        read32(adjacent_alias_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t observed_adjacent_base = read32(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word through the original alias also remains unchanged across the switched-refresh sequence",
        observed_adjacent_base == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias-sc.w store-trigger switched-refresh adjacent-word-preservation corner finally updates only the shared target word while keeping the adjacent backing word unchanged",
        read32(paddr) == refreshed_value &&
        read32(adjacent_paddr) == adjacent_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_execute_and_store_trigger_repeated_breakpoint_then_original_refresh_adjacent_word_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uintptr_t trigger_pc = (uintptr_t)ai_lr_sc_sc_w_helper;
    const uint32_t initial_value = 0x99aabbccU;
    const uint32_t blocked_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x2468ace0U;
    const uint32_t adjacent_value = 0x55aa33ccU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);
    write32(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias captures the shared backing word before the adjacent-word-preservation dual-trigger alias-sc.w corner runs",
        reserved == initial_value
    );

    ai_lr_sc_clear_mcontrol6_slot(0);
    ai_lr_sc_clear_mcontrol6_slot(1);
    ai_lr_sc_arm_mcontrol6_slot(0, trigger_pc, AI_LR_SC_MCONTROL6_EXECUTE);
    ai_lr_sc_arm_mcontrol6_slot(1, alias_vaddr, AI_LR_SC_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_w_helper(alias_vaddr, blocked_value);
    TEST_ASSERT("with both execute-PC and store triggers armed, the first alias-mismatched sc.w helper call reports breakpoint before either the target word or the adjacent word changes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_w_helper(alias_vaddr, blocked_value ^ 0x01010101U);
    TEST_ASSERT("with both trigger slots still armed, the second alias-mismatched sc.w helper call also reports breakpoint before either the target word or the adjacent word changes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );
    ai_lr_sc_restore_machine_status(saved_mstatus);

    ai_lr_sc_clear_mcontrol6_slot(0);
    saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_w_helper(alias_vaddr, blocked_value);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining store trigger still reports breakpoint on the same alias-mismatched helper call while leaving both words unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );
    ai_lr_sc_restore_machine_status(saved_mstatus);

    ai_lr_sc_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, blocked_value);
    AI_ASSERT_NO_EXCEPT_AND("after clearing both trigger slots, the same alias-mismatched sc.w falls back to its normal failure behavior in the adjacent-word-preservation dual-trigger corner while leaving both words unchanged",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated dual-trigger hits and the post-clear mismatch failure, a fresh lr.w on the original alias still captures the untouched shared word in the adjacent-word-preservation corner",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original alias, the following sc.w succeeds while the adjacent word still remains unchanged in the dual-trigger corner",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t observed_adjacent_alias = read32(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word through the switched alias also remains unchanged across the dual-trigger repeated-breakpoint-and-refresh sequence",
        observed_adjacent_alias == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the dual-trigger alias-sc.w adjacent-word-preservation corner finally updates only the shared target word while keeping the adjacent backing word unchanged",
        read32(paddr) == refreshed_value &&
        read32(adjacent_paddr) == adjacent_value
    );

    ai_lr_sc_clear_mcontrol6_slot(0);
    ai_lr_sc_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_execute_and_store_trigger_repeated_breakpoint_then_switched_refresh_adjacent_word_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uintptr_t trigger_pc = (uintptr_t)ai_lr_sc_sc_w_helper;
    const uint32_t initial_value = 0x8899aabbU;
    const uint32_t blocked_value = 0x2468ace0U;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint32_t adjacent_value = 0x55aa33ccU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);
    write32(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias captures the shared backing word before the switched-refresh adjacent-word-preservation dual-trigger alias-sc.w corner runs",
        reserved == initial_value
    );

    ai_lr_sc_clear_mcontrol6_slot(0);
    ai_lr_sc_clear_mcontrol6_slot(1);
    ai_lr_sc_arm_mcontrol6_slot(0, trigger_pc, AI_LR_SC_MCONTROL6_EXECUTE);
    ai_lr_sc_arm_mcontrol6_slot(1, alias_vaddr, AI_LR_SC_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_w_helper(alias_vaddr, blocked_value);
    TEST_ASSERT("with both execute-PC and store triggers armed, the first alias-mismatched sc.w helper call reports breakpoint before either the target word or the adjacent word changes in the switched-refresh corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_w_helper(alias_vaddr, blocked_value ^ 0x01010101U);
    TEST_ASSERT("with both trigger slots still armed, the second alias-mismatched sc.w helper call also reports breakpoint before either the target word or the adjacent word changes in the switched-refresh corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );
    ai_lr_sc_restore_machine_status(saved_mstatus);

    ai_lr_sc_clear_mcontrol6_slot(0);
    saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_w_helper(alias_vaddr, blocked_value);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining store trigger still reports breakpoint on the same alias-mismatched helper call in the switched-refresh corner while leaving both words unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );
    ai_lr_sc_restore_machine_status(saved_mstatus);

    ai_lr_sc_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, blocked_value);
    AI_ASSERT_NO_EXCEPT_AND("after clearing both trigger slots, the same alias-mismatched sc.w falls back to its normal failure behavior in the switched-refresh adjacent-word-preservation dual-trigger corner while leaving both words unchanged",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value &&
        read32(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated dual-trigger hits and the post-clear mismatch failure, a fresh lr.w on the switched alias still captures the untouched shared word in the switched-refresh adjacent-word-preservation corner",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the switched alias, the following sc.w succeeds while the adjacent word still remains unchanged in the switched-refresh dual-trigger corner",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value &&
        read32(adjacent_alias_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t observed_adjacent_base = read32(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word through the original alias also remains unchanged across the switched-refresh dual-trigger sequence",
        observed_adjacent_base == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the switched-refresh dual-trigger alias-sc.w adjacent-word-preservation corner finally updates only the shared target word while keeping the adjacent backing word unchanged",
        read32(paddr) == refreshed_value &&
        read32(adjacent_paddr) == adjacent_value
    );

    ai_lr_sc_clear_mcontrol6_slot(0);
    ai_lr_sc_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_original_doubleword_refresh_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x778899aa13579bdfULL;
    const uint64_t blocked_value = 0x2468ace055aa33ccULL;
    const uint64_t refreshed_value = 0x1122aabbccddeeffULL;
    const uint64_t adjacent_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("lr.d through the base alias captures the shared backing doubleword before the adjacent-doubleword-preservation store-trigger corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );

    ai_lr_sc_arm_mcontrol6_0(alias_vaddr, AI_LR_SC_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_d_status(alias_vaddr, blocked_value);
    TEST_ASSERT("with the store trigger armed, the first alias-mismatched sc.d reports breakpoint and leaves the target and adjacent doublewords unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_value
    );
    ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_d_status(alias_vaddr, blocked_value ^ 0x0101010101010101ULL);
    TEST_ASSERT("with the store trigger still armed, the repeated alias-mismatched sc.d reports breakpoint and leaves the target and adjacent doublewords unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_value
    );

    ai_lr_sc_restore_machine_status(saved_mstatus);
    ai_lr_sc_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t failed_status = ai_lr_sc_sc_d_status(alias_vaddr, blocked_value);
    TEST_ASSERT("after clearing the store trigger, the same alias-mismatched sc.d falls back to its normal failure behavior while still leaving the target doubleword and the adjacent doubleword unchanged",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value &&
        read64(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after the repeated trigger hits and the post-clear mismatch failure, a fresh lr.d on the original alias still captures the untouched shared doubleword in the adjacent-doubleword-preservation corner",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing the reservation on the original alias, the following sc.d succeeds while the adjacent doubleword still remains unchanged",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value &&
        read64(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_alias = read64(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent doubleword through the switched alias also remains unchanged across the repeated-trigger-and-refresh sequence",
        excpt.triggered == false &&
        observed_adjacent_alias == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias-sc.d store-trigger adjacent-doubleword-preservation corner finally updates only the shared target doubleword while keeping the adjacent backing doubleword unchanged",
        read64(paddr) == refreshed_value &&
        read64(adjacent_paddr) == adjacent_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_switched_doubleword_refresh_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x5566778889abcdefULL;
    const uint64_t blocked_value = 0x2468ace013579bdfULL;
    const uint64_t refreshed_value = 0x55aa33cc77ee11ffULL;
    const uint64_t adjacent_value = 0x1021324354657687ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("lr.d through the base alias captures the shared backing doubleword before the switched-refresh adjacent-doubleword-preservation store-trigger corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );

    ai_lr_sc_arm_mcontrol6_0(alias_vaddr, AI_LR_SC_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_d_status(alias_vaddr, blocked_value);
    TEST_ASSERT("with the store trigger armed in the switched-refresh adjacent-doubleword corner, the first alias-mismatched sc.d reports breakpoint and leaves both doublewords unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_value
    );
    ai_lr_sc_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_lr_sc_sc_d_status(alias_vaddr, blocked_value ^ 0x0101010101010101ULL);
    TEST_ASSERT("with the store trigger still armed in the switched-refresh adjacent-doubleword corner, the repeated alias-mismatched sc.d reports breakpoint and leaves both doublewords unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_value
    );

    ai_lr_sc_restore_machine_status(saved_mstatus);
    ai_lr_sc_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t failed_status = ai_lr_sc_sc_d_status(alias_vaddr, blocked_value);
    TEST_ASSERT("after clearing the store trigger, the same alias-mismatched sc.d still falls back to its normal failure behavior in the switched-refresh adjacent-doubleword-preservation corner while leaving both doublewords unchanged",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value &&
        read64(adjacent_base_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after the repeated trigger hits and the post-clear mismatch failure, a fresh lr.d on the switched alias still captures the untouched shared doubleword in the switched-refresh adjacent-doubleword-preservation corner",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing the reservation on the switched alias, the following sc.d succeeds while the adjacent doubleword still remains unchanged",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value &&
        read64(adjacent_alias_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_base = read64(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent doubleword through the original alias also remains unchanged across the switched-refresh sequence",
        excpt.triggered == false &&
        observed_adjacent_base == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias-sc.d store-trigger switched-refresh adjacent-doubleword-preservation corner finally updates only the shared target doubleword while keeping the adjacent backing doubleword unchanged",
        read64(paddr) == refreshed_value &&
        read64(adjacent_paddr) == adjacent_value
    );

    TEST_END();
}
