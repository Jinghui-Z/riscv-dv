#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_store_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_tail_byte_vaddr = alias_vaddr + sizeof(uint64_t) - 1U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t tail_byte_value = 0x55U;
    const uint64_t expected_after_store =
        (initial_value & 0x00ffffffffffffffULL) |
        ((uint64_t)tail_byte_value << 56);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

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
    write8(alias_tail_byte_vaddr, tail_byte_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a tail-byte store is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the tail-byte store through the second alias becomes visible as a one-byte update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a one-byte store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias tail-byte store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset5_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 5U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x55U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 40)) |
        ((uint64_t)byte_value << 40);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

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
    write8(alias_byte_vaddr, byte_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte store at offset five is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte store at offset five through the second alias becomes visible as an interior one-byte update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("an interior byte store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias interior-byte store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_sc_fails_preserves_amo_corner()
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
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

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
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word AMO is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word amoadd.d through the second virtual alias observes the old value and updates the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("the intervening amoadd.d invalidates the reservation, so the later same-alias sc.d successful",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally contains the intervening sc.d payload after the alias-AMO corner sequence",
        read64(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_diff_setidx_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias AMO is issued",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias amoadd.d still observes the old value and updates the shared doubleword before the later sc.d retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("for the different-setidx same-paddr alias path, the later same-alias sc.d is expected to fail and preserve the alias-AMO result",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-AMO sequence, the shared backing doubleword keeps the alias-AMO updated payload",
        read64(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_diff_setidx_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias AMO is issued in the failed-sc-consumption corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias amoadd.d still updates the shared doubleword before the first failing sc.d retires in the failed-sc-consumption corner",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("the first same-alias sc.d still fails after the different-setidx alias amoadd.d and preserves the AMO result",
        excpt.triggered == false &&
        first_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that failed sc.d, retrying sc.d on the original alias without a fresh lr.d still fails because the failed sc.d has already consumed the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(alias_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-amoadd.d failed-sc-consumption corner, the shared backing doubleword keeps only the alias-AMO payload",
        read64(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_diff_setidx_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;
    const uint64_t refreshed_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t failed_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias AMO is issued in the original-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias amoadd.d still updates the shared doubleword before the first failing sc.d retires in the original-refresh corner",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("the first same-alias sc.d fails and preserves the alias-AMO result on the different-setidx path",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that failed sc.d, a fresh lr.d on the original alias captures the alias-AMO value that remained in memory",
        excpt.triggered == false &&
        refreshed_reserved == expected_after_amo
    );
    TEST_ASSERT("after refreshing the reservation on the original alias, the following sc.d succeeds and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-amoadd.d failure-and-original-refresh corner, the shared backing doubleword contains only the refreshed original-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_diff_setidx_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;
    const uint64_t refreshed_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t failed_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias AMO is issued in the switched-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias amoadd.d still updates the shared doubleword before the first failing sc.d retires in the switched-refresh corner",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("the first same-alias sc.d fails and preserves the alias-AMO result on the different-setidx path",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that failed sc.d, a fresh lr.d on the switched alias captures the alias-AMO value that remained in memory",
        excpt.triggered == false &&
        refreshed_reserved == expected_after_amo
    );
    TEST_ASSERT("after refreshing the reservation on the switched alias, the following sc.d succeeds and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-amoadd.d failure-and-switched-refresh corner, the shared backing doubleword contains only the refreshed switched-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_failed_sc_clears_reservation_corner()
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
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

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
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word AMO is issued through the second alias in the reservation-consumption corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word amoadd.d through the second virtual alias observes the old value and updates the shared doubleword before the first sc.d retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("the first sc.d fails after the intervening alias amoadd.d and preserves the intervening AMO result",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that failed sc.d, a second sc.d through the other alias without a fresh lr.d still fails because the prior failed SC has already cleared the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword keeps the first successful sc.d payload after the alias-AMO reservation-consumption corner",
        read64(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_sc_fails_then_refresh_original_succeeds_corner()
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
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;
    const uint64_t refreshed_value = 0x0f1e2d3c4b5a6978ULL;

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
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word AMO is issued through the second alias in the original-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word amoadd.d through the second virtual alias observes the old value and updates the shared doubleword before the first sc.d retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("the first sc.d fails after the intervening alias amoadd.d and preserves the intervening AMO result",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful sc.d, a stale sc.d through the other alias fails because the prior failed SC has already cleared the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refresh_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after the stale alias retry fails, a fresh lr.d/sc.d pair on the original alias re-establishes the shared reservation and succeeds",
        excpt.triggered == false &&
        refreshed_reserved == expected_after_amo &&
        refresh_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally keeps the refreshed original-alias sc.d payload after the alias-amoadd.d recovery corner",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_sc_fails_then_refresh_switched_succeeds_corner()
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
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;
    const uint64_t refreshed_value = 0x0f1e2d3c4b5a6978ULL;

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
    uint64_t amo_old_value = amoadd_d(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word AMO is issued through the second alias in the switched-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word amoadd.d through the second virtual alias observes the old value and updates the shared doubleword before the first sc.d retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("the first sc.d fails after the intervening alias amoadd.d and preserves the intervening AMO result",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful sc.d, a stale sc.d through the other alias fails because the prior failed SC has already cleared the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refresh_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after the stale alias retry fails, a fresh lr.d/sc.d pair on the switched alias re-establishes the shared reservation and succeeds",
        excpt.triggered == false &&
        refreshed_reserved == expected_after_amo &&
        refresh_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally keeps the refreshed switched-alias sc.d payload after the alias-amoadd.d recovery corner",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_word_amo_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint32_t amo_delta = 0x01010101U;
    const uint64_t expected_after_amo =
        ((uint64_t)(uint32_t)(initial_value >> 32) << 32) |
        (uint64_t)((uint32_t)initial_value + amo_delta);
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

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
    uint32_t amo_old_value = amoadd_w(alias_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-start amoadd.w is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-start amoadd.w through the second alias observes the low word and updates only the low 32-bit half before sc.d retires",
        excpt.triggered == false &&
        amo_old_value == (uint32_t)initial_value &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("a same-start amoadd.w through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start amoadd.w corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_amo_before_doubleword_sc_fails_preserves_intervening_corner()
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
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

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
    uint32_t amo_old_value = amoadd_w(alias_upper_word_vaddr, amo_delta);
    uint64_t after_alias_amo = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before an upper-word amoadd.w is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the upper-word amoadd.w through the second alias observes the high word and updates only the upper 32-bit half before sc.d retires",
        excpt.triggered == false &&
        amo_old_value == (uint32_t)(initial_value >> 32) &&
        after_alias_amo == expected_after_amo
    );
    TEST_ASSERT("an upper-word amoadd.w through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_amo
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-word amoadd.w corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_store_still_succeeds()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t intervening_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write32(addr, intervening_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address ordinary store is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening same-address ordinary store becomes architecturally visible before the later sc.w retires",
        after_store == intervening_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-address ordinary store does not break the reservation and the following sc.w still succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_store_adjacent_word_preserved_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t adjacent_addr = ai_lr_sc_addr(1);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t intervening_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;
    const uint32_t adjacent_value = 0x2468ace0U;

    ai_lr_sc_prepare_words(initial_value, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write32(addr, intervening_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address ordinary store is inserted between lr.w and sc.w in the adjacent-word-preservation corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening same-address ordinary store becomes architecturally visible before the later sc.w retires in the adjacent-word-preservation corner",
        after_store == intervening_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-address ordinary store does not break the reservation and the following sc.w still succeeds in the adjacent-word-preservation corner",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t observed_adjacent = read32(adjacent_addr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word remains unchanged across the same-address store-plus-sc.w sequence",
        observed_adjacent == adjacent_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_store_success_consumes_reservation_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t intervening_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write32(addr, intervening_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address ordinary store is inserted between lr.w and sc.w in the reservation-consumption corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening same-address ordinary store becomes architecturally visible before the first sc.w retires",
        after_store == intervening_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w still succeeds after the intervening same-address ordinary store and overwrites the store data",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0xcafebabeU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a second sc.w without a fresh lr.w still fails because the successful reservation has already been consumed",
        second_status != 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_store_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t intervening_value = 0x55aa33ccU;
    const uint32_t committed_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x2468ace0U;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write32(addr, intervening_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address ordinary store is inserted between lr.w and sc.w in the refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening same-address ordinary store becomes architecturally visible before the first sc.w retires",
        after_store == intervening_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w still succeeds after the intervening same-address ordinary store and overwrites the store data",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0xcafebabeU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a stale retry without a fresh lr.w fails because the old reservation has already been consumed",
        second_status != 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refresh_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the stale retry fails, a fresh lr.w/sc.w pair re-establishes the reservation and succeeds",
        refreshed_reserved == committed_value &&
        refresh_status == 0U &&
        ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_halfword_store_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint16_t low_halfword_value = 0x33ccU;
    const uint32_t expected_after_store =
        ((uint32_t)(initial_value & 0xffff0000U)) | (uint32_t)low_halfword_value;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write16(addr, low_halfword_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-start low-halfword store is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening same-start low-halfword store becomes visible as a partial update of the reserved word before sc.w retires",
        after_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-start low-halfword store does not break the reservation and the following sc.w still succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_halfword_store_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_halfword_addr = addr + sizeof(uint16_t);
    const uint32_t initial_value = 0x89abcdefU;
    const uint16_t upper_halfword_value = 0x55aaU;
    const uint32_t expected_after_store =
        ((uint32_t)upper_halfword_value << 16) | (uint32_t)(initial_value & 0x0000ffffU);
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write16(upper_halfword_addr, upper_halfword_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before an upper-halfword store is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening upper-halfword store becomes visible as a partial update of the reserved word before sc.w retires",
        after_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, an upper-halfword store does not break the reservation and the following sc.w still succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_byte_store_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint8_t low_byte_value = 0x33U;
    const uint32_t expected_after_store =
        ((uint32_t)(initial_value & 0xffffff00U)) | (uint32_t)low_byte_value;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write8(addr, low_byte_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-start low-byte store is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening same-start low-byte store becomes visible as a one-byte update of the reserved word before sc.w retires",
        after_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-start low-byte store does not break the reservation and the following sc.w still succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset1_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 1U;
    const uint32_t initial_value = 0x89abcdefU;
    const uint8_t byte_value = 0x33U;
    const uint32_t expected_after_store =
        (initial_value & ~(0xffU << 8)) |
        ((uint32_t)byte_value << 8);
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write8(byte_addr, byte_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a byte store at offset one touches the interior of the reserved word",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening byte store at offset one becomes visible as an interior one-byte update of the reserved word before sc.w retires",
        after_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, an interior byte store at offset one does not break the reservation and the following sc.w still succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset2_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 2U;
    const uint32_t initial_value = 0x89abcdefU;
    const uint8_t byte_value = 0x55U;
    const uint32_t expected_after_store =
        (initial_value & ~(0xffU << 16)) |
        ((uint32_t)byte_value << 16);
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write8(byte_addr, byte_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a byte store at offset two touches the interior of the reserved word",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening byte store at offset two becomes visible as an interior one-byte update of the reserved word before sc.w retires",
        after_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, an interior byte store at offset two does not break the reservation and the following sc.w still succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_tail_byte_store_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t tail_byte_addr = addr + sizeof(uint32_t) - 1U;
    const uint32_t initial_value = 0x89abcdefU;
    const uint8_t tail_byte_value = 0x55U;
    const uint32_t expected_after_store =
        ((uint32_t)tail_byte_value << 24) | (uint32_t)(initial_value & 0x00ffffffU);
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write8(tail_byte_addr, tail_byte_value);
    uint32_t after_store = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a tail-byte store is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening tail-byte store becomes visible as a one-byte update of the reserved word before sc.w retires",
        after_store == expected_after_store
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a tail-byte store does not break the reservation and the following sc.w still succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_word_store_before_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write32(upper_word_addr, upper_word_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    TEST_ASSERT("lr.w captures the original low word before a same-doubleword upper-word store is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the intervening upper-word store becomes visible as a non-overlapping high-half update of the shared doubleword before the low-word sc.w retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a same-doubleword but non-overlapping upper-word store invalidates the low-word reservation, so the following sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_halfword_store_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    write16(upper_word_addr, halfword_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a same-start low-halfword store is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening same-start low-halfword store becomes visible as a low-edge partial update of the reserved upper word before sc.w retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("on Spike, a same-start low-halfword store inside the reserved upper word still does not break the reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_halfword_store_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uintptr_t upper_halfword_addr = upper_word_addr + sizeof(uint16_t);
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    write16(upper_halfword_addr, halfword_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before an upper-halfword store is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening upper-halfword store becomes visible as a high-edge partial update of the reserved upper word before sc.w retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("an upper-halfword store inside the reserved upper word invalidates the reservation, so the following sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_byte_store_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    write8(upper_word_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a same-start low-byte store is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening same-start low-byte store becomes visible as a low-edge one-byte update of the reserved upper word before sc.w retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("on Spike, a same-start low-byte store inside the reserved upper word still does not break the reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset1_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uintptr_t byte_addr = upper_word_addr + 1U;
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    write8(byte_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a byte store at offset one is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening byte store at offset one becomes visible as a first interior one-byte update of the reserved upper word before sc.w retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset one inside the reserved upper word invalidates the reservation, so the following sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset2_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uintptr_t byte_addr = upper_word_addr + 2U;
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    write8(byte_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a byte store at offset two is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening byte store at offset two becomes visible as a second interior one-byte update of the reserved upper word before sc.w retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset two inside the reserved upper word invalidates the reservation, so the following sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_tail_byte_store_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uintptr_t tail_byte_addr = upper_word_addr + sizeof(uint32_t) - 1U;
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    write8(tail_byte_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a tail-byte store is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening tail-byte store becomes visible as a high-edge one-byte update of the reserved upper word before sc.w retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a tail-byte store inside the reserved upper word invalidates the reservation, so the following sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_word_store_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    write32(addr, low_word_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a same-doubleword low-word store is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening low-word store becomes visible as a non-overlapping low-half update of the shared doubleword before the upper-word sc.w retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("on Spike, a same-doubleword but non-overlapping low-word store still does not break the upper-word reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}
