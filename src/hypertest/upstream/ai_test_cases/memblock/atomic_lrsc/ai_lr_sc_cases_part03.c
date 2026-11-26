#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_cachehit_first_succeeds_second_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRS(CSR_SCOUNTEREN, HCOUNTEREN_CY);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t alias_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    uint32_t base_retry_status = ai_lr_sc_sc_d_status(base_vaddr,
        0x8877665544332211ULL);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the immediate alias-switch doubleword corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first same-size sc.d succeeds when it immediately switches to a same-paddr virtual alias that hits in DCache",
        excpt.triggered == false &&
        alias_status == 0U
    );
    TEST_ASSERT("after that immediate alias-switch sc.d success, a second sc.d back on the original alias fails without a new lr.d because the reservation has already been consumed",
        excpt.triggered == false &&
        base_retry_status != 0U &&
        read64(base_vaddr) == first_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch doubleword corner, the shared backing doubleword keeps only the first successful alias payload",
        read64(paddr) == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_cachehit_success_then_alias_retry_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the immediate alias-switch doubleword same-alias retry corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first same-size sc.d succeeds when it immediately switches to the other same-paddr alias and the alias access hits in DCache",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr,
        0x8877665544332211ULL);
    TEST_ASSERT("after that successful alias sc.d, retrying sc.d again on the same switched alias without a fresh lr.d still fails because the reservation has already been consumed",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == first_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch doubleword same-alias retry corner, the shared backing doubleword keeps only the first successful alias payload",
        read64(paddr) == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_cachehit_success_retry_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x33445566U;
    const uint32_t first_committed = 0x13579bdfU;
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
    uint32_t alias_status = sc_w(alias_vaddr, first_committed);
    uint32_t base_retry_status = sc_w(base_vaddr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the original alias captures the shared word before the immediate alias-switch refresh corner runs",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w can still succeed when it immediately switches to the other alias for the same physical word",
        alias_status == 0U &&
        read32(base_vaddr) == first_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("without a fresh lr.w, the immediate retry back on the original alias still fails because the earlier alias success has already consumed the reservation",
        base_retry_status != 0U &&
        read32(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that immediate alias retry failure, a fresh lr.w on the original alias still captures the first committed shared word",
        refreshed_reserved == first_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original alias, the following sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch recovery corner, the shared backing word contains only the refreshed original-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_cachehit_success_retry_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x33445566U;
    const uint32_t first_committed = 0x13579bdfU;
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
    uint32_t alias_status = sc_w(alias_vaddr, first_committed);
    uint32_t base_retry_status = sc_w(base_vaddr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the original alias captures the shared word before the immediate alias-switch switched-refresh corner runs",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w can still succeed when it immediately switches to the other alias for the same physical word",
        alias_status == 0U &&
        read32(base_vaddr) == first_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("without a fresh lr.w, the immediate retry back on the original alias still fails because the earlier alias success has already consumed the reservation",
        base_retry_status != 0U &&
        read32(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that immediate alias retry failure, a fresh lr.w on the switched alias still captures the first committed shared word",
        refreshed_reserved == first_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the switched alias, the following sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch switched-refresh corner, the shared backing word contains only the refreshed switched-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_cachehit_success_retry_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refreshed_value = 0x8877665544332211ULL;

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
    uint32_t alias_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    uint32_t base_retry_status = ai_lr_sc_sc_d_status(base_vaddr,
        0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("lr.d through the original alias captures the shared doubleword before the immediate alias-switch doubleword refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("on Spike, the first sc.d can still succeed when it immediately switches to the other alias for the same physical doubleword",
        excpt.triggered == false &&
        alias_status == 0U &&
        read64(base_vaddr) == first_committed
    );
    TEST_ASSERT("without a fresh lr.d, the immediate retry back on the original alias still fails because the earlier alias sc.d success has already consumed the reservation",
        excpt.triggered == false &&
        base_retry_status != 0U &&
        read64(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that immediate alias retry failure, a fresh lr.d on the original alias still captures the first committed shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_committed
    );
    TEST_ASSERT("after refreshing the reservation on the original alias, the following sc.d succeeds and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch doubleword recovery corner, the shared backing doubleword contains only the refreshed original-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_cachehit_success_retry_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refreshed_value = 0x8877665544332211ULL;

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
    uint32_t alias_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    uint32_t base_retry_status = ai_lr_sc_sc_d_status(base_vaddr,
        0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("lr.d through the original alias captures the shared doubleword before the immediate alias-switch doubleword switched-refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("on Spike, the first sc.d can still succeed when it immediately switches to the other alias for the same physical doubleword",
        excpt.triggered == false &&
        alias_status == 0U &&
        read64(base_vaddr) == first_committed
    );
    TEST_ASSERT("without a fresh lr.d, the immediate retry back on the original alias still fails because the earlier alias sc.d success has already consumed the reservation",
        excpt.triggered == false &&
        base_retry_status != 0U &&
        read64(alias_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that immediate alias retry failure, a fresh lr.d on the switched alias still captures the first committed shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_committed
    );
    TEST_ASSERT("after refreshing the reservation on the switched alias, the following sc.d succeeds and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch doubleword switched-refresh corner, the shared backing doubleword contains only the refreshed switched-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t alias_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first alias captures the low word before the immediate alias-switch mixed-size corner runs",
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("when lr.w immediately switches to a same-paddr alias and widens to sc.d, the mixed-size SC fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch mixed-size failure corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_clears_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t alias_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first alias captures the low word before a wider sc.d first commits through the second alias",
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the first same-paddr mixed-size sc.d through the other alias fails because the LR.W reservation does not match SC.D width",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t base_retry_status = sc_w(base_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.d, retrying sc.w on the original alias without a fresh lr.w still fails because the SC attempt cleared the reservation",
        base_retry_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed-size alias failure-consumption corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_alias_doubleword_retry_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first alias captures the low word before the immediate alias-switch widening same-alias retry corner runs",
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the first mixed-size sc.d through the same-paddr alias fails because LR.W and SC.D widths differ",
        excpt.triggered == false &&
        first_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr,
        0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("after that failed mixed-size alias sc.d, retrying sc.d again on the same switched alias without a fresh reservation still fails and leaves memory unchanged",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch widening retry corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_alias_word_retry_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first alias captures the low word before the immediate alias-switch widening cross-size retry corner runs",
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the first mixed-size sc.d through the same-paddr alias fails because LR.W and SC.D widths differ",
        excpt.triggered == false &&
        first_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.d, retrying sc.w on the same switched alias without a fresh reservation still fails and leaves memory unchanged",
        second_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch widening word-retry corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_alias_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0x0f0ff0f05a5aa5a5ULL;
    const uint64_t refreshed_value = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t alias_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first alias captures the low word before the immediate alias-switch mixed-size refresh corner runs",
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the first mixed-size sc.d through the same-paddr alias fails because LR.W and SC.D widths differ, leaving the refresh to start from the original doubleword",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that failed mixed-size alias sc.d, a fresh lr.d on the switched alias captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the switched alias, the shared doubleword is updated and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed-size alias refresh corner, the shared backing doubleword contains only the refreshed alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_original_word_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0x0f0ff0f05a5aa5a5ULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        (initial_value & 0xffffffff00000000ULL) | (uint64_t)refreshed_value;

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
    uint32_t alias_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first alias captures the low word before the immediate alias-switch mixed-size original-refresh corner runs",
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the first mixed-size sc.d through the same-paddr alias fails because LR.W and SC.D widths differ, leaving the original-alias refresh to start from the original doubleword",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.d, a fresh lr.w on the original alias captures the original low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the original alias, only the shared low word is updated while the original high word is preserved",
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed-size original-alias refresh corner, the shared backing doubleword keeps the original high word and the refreshed original-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_original_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0x0f0ff0f05a5aa5a5ULL;
    const uint64_t refreshed_value = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t alias_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first alias captures the low word before the immediate alias-switch mixed-size original-doubleword-refresh corner runs",
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the first mixed-size sc.d through the same-paddr alias fails because LR.W and SC.D widths differ, leaving the original-alias doubleword refresh to start from the original value",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that failed mixed-size alias sc.d, a fresh lr.d on the original alias captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a wider lr.d/sc.d pair on the original alias, the shared doubleword is updated again and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed-size original-alias doubleword refresh corner, the shared backing doubleword contains only the refreshed original-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_alias_word_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0x0f0ff0f05a5aa5a5ULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        (initial_value & 0xffffffff00000000ULL) | (uint64_t)refreshed_value;

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
    uint32_t alias_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first alias captures the low word before the immediate alias-switch mixed-size alias-word-refresh corner runs",
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the first mixed-size sc.d through the same-paddr alias fails because LR.W and SC.D widths differ, leaving the switched-alias word refresh to start from the original value",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.d, a fresh lr.w on the switched alias captures the original low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the switched alias, only the shared low word is updated while the original high word is preserved",
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed-size switched-alias word refresh corner, the shared backing doubleword keeps the original high word and the refreshed switched-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t first_committed = 0x13579bdfU;

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
    uint32_t alias_status = sc_w(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first alias captures the shared doubleword before the immediate alias-switch narrowing corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("when lr.d immediately switches to a same-paddr alias and narrows to sc.w, the mixed-size SC fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch narrowing failure corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_clears_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t first_committed = 0x13579bdfU;

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
    uint32_t alias_status = sc_w(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first alias captures the shared doubleword before a narrower sc.w first commits through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first same-paddr mixed-size sc.w through the other alias fails because the LR.D reservation does not match SC.W width",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t base_retry_status = ai_lr_sc_sc_d_status(base_vaddr,
        0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("after that failed mixed-size alias sc.w, retrying sc.d on the original alias without a fresh lr.d still fails because the SC attempt cleared the reservation",
        excpt.triggered == false &&
        base_retry_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed-size alias narrowing failure-consumption corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_alias_word_retry_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t first_committed = 0x13579bdfU;

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
    uint32_t first_status = sc_w(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first alias captures the shared doubleword before the immediate alias-switch narrowing same-alias retry corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first mixed-size sc.w through the same-paddr alias fails because LR.D and SC.W widths differ",
        excpt.triggered == false &&
        first_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x2468ace0U);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.w, retrying sc.w again on the same switched alias without a fresh reservation still fails and leaves memory unchanged",
        second_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch narrowing same-alias retry corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_alias_doubleword_retry_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t first_committed = 0x13579bdfU;

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
    uint32_t first_status = sc_w(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first alias captures the shared doubleword before the immediate alias-switch narrowing cross-size retry corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first mixed-size sc.w through the same-paddr alias fails because LR.D and SC.W widths differ",
        excpt.triggered == false &&
        first_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr,
        0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("after that failed mixed-size alias sc.w, retrying sc.d on the same switched alias without a fresh reservation still fails and leaves memory unchanged",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch narrowing doubleword-retry corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_alias_word_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t first_committed = 0x0badc0deU;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        (initial_value & 0xffffffff00000000ULL) | (uint64_t)refreshed_value;

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
    uint32_t alias_status = sc_w(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first alias captures the shared doubleword before the immediate alias-switch narrowing refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first mixed-size sc.w through the same-paddr alias fails because LR.D and SC.W widths differ, leaving the switched-alias word refresh to start from the original value",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.w, a fresh lr.w on the switched alias captures the original low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the switched alias, the shared low word is updated while the original high word is preserved",
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed-size alias narrowing refresh corner, the shared backing doubleword keeps the original high word and the refreshed alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_original_word_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t first_committed = 0x0badc0deU;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        (initial_value & 0xffffffff00000000ULL) | (uint64_t)refreshed_value;

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
    uint32_t alias_status = sc_w(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first alias captures the shared doubleword before the immediate alias-switch narrowing original-word-refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first mixed-size sc.w through the same-paddr alias fails because LR.D and SC.W widths differ, leaving the original-alias word refresh to start from the original value",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.w, a fresh lr.w on the original alias captures the original low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the original alias, only the shared low word is updated while the original high word is preserved",
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed-size original-alias word refresh corner, the shared backing doubleword keeps the original high word and the refreshed original-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;

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
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;
    uint32_t sc_status = ai_lr_sc_sc_d_status(alias_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the second alias attempts sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("sc.d through a same-paddr virtual alias succeeds when the alias access hits in DCache",
        excpt.triggered == false &&
        sc_status == 0U
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(base_vaddr);
    TEST_ASSERT("the shared physical doubleword now contains the successful alias-sc.d payload",
        excpt.triggered == false &&
        observed == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the different-vaddr alias success-consumption corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first sc.d through the same-paddr virtual alias succeeds when the alias access hits in DCache and updates the shared doubleword",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful alias sc.d, retrying sc.d on the original alias without a fresh lr.d still fails because the reservation was consumed",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(alias_vaddr) == first_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-vaddr alias doubleword success-consumption corner, the shared backing doubleword keeps only the first successful alias sc.d payload",
        read64(paddr) == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_alias_retry_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the different-vaddr alias same-alias retry corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first alias sc.d succeeds in the same-paddr different-vaddr sequence when the alias access hits in DCache and updates the shared doubleword",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr,
        0x8877665544332211ULL);
    TEST_ASSERT("after that successful alias sc.d, retrying sc.d again on the same switched alias without a fresh lr.d still fails because the reservation was consumed",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == first_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-vaddr alias same-alias retry corner, the shared backing doubleword keeps only the first successful alias sc.d payload",
        read64(paddr) == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_alias_word_retry_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the different-vaddr alias cross-size retry corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first alias sc.d succeeds in the same-paddr different-vaddr sequence when the alias access hits in DCache and updates the shared doubleword",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful alias sc.d, retrying a narrower sc.w on the same switched alias without a fresh lr.d still fails because the reservation was consumed",
        second_status != 0U &&
        read64(base_vaddr) == first_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-vaddr alias cross-size retry corner, the shared backing doubleword keeps only the first successful alias sc.d payload",
        read64(paddr) == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refreshed_value = 0x8877665544332211ULL;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the different-vaddr alias original-refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first alias sc.d succeeds in the same-paddr different-vaddr sequence when the alias access hits in DCache and updates the shared doubleword",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that successful alias sc.d, a fresh lr.d on the original alias still captures the first committed shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_committed
    );
    TEST_ASSERT("after refreshing the reservation on the original alias, the following sc.d succeeds and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-vaddr alias doubleword original-refresh corner, the shared backing doubleword contains only the refreshed original-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refreshed_value = 0x8877665544332211ULL;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the different-vaddr alias switched-refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first alias sc.d succeeds in the same-paddr different-vaddr sequence when the alias access hits in DCache and updates the shared doubleword",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that successful alias sc.d, a fresh lr.d on the switched alias still captures the first committed shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_committed
    );
    TEST_ASSERT("after refreshing the reservation on the switched alias, the following sc.d succeeds and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-vaddr alias doubleword switched-refresh corner, the shared backing doubleword contains only the refreshed switched-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_refresh_original_word_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        (first_committed & 0xffffffff00000000ULL) | (uint64_t)refreshed_value;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the different-vaddr alias original-word-refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first alias sc.d succeeds in the same-paddr different-vaddr sequence when the alias access hits in DCache and updates the shared doubleword",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that successful alias sc.d, a fresh lr.w on the original alias still captures the first committed low word",
        refreshed_reserved == (uint32_t)first_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a narrower lr.w/sc.w pair on the original alias, only the shared low word is updated while the high word from the earlier sc.d is preserved",
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-vaddr alias doubleword original-word-refresh corner, the shared backing doubleword keeps the earlier high word and the refreshed original-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_refresh_switched_word_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t first_committed = 0xa1b2c3d4e5f60718ULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        (first_committed & 0xffffffff00000000ULL) | (uint64_t)refreshed_value;

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
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the different-vaddr alias switched-word-refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first alias sc.d succeeds in the same-paddr different-vaddr sequence when the alias access hits in DCache and updates the shared doubleword",
        excpt.triggered == false &&
        first_status == 0U &&
        read64(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that successful alias sc.d, a fresh lr.w on the switched alias still captures the first committed low word",
        refreshed_reserved == (uint32_t)first_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a narrower lr.w/sc.w pair on the switched alias, only the shared low word is updated while the high word from the earlier sc.d is preserved",
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-vaddr alias doubleword switched-word-refresh corner, the shared backing doubleword keeps the earlier high word and the refreshed switched-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_inner_only_succeeds()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x33445566U;
    const uint32_t inner_committed = 0xa1b2c3d4U;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w through the first virtual alias captures the original shared word before a nested alias reservation is created",
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(alias_vaddr);
    uint32_t inner_status = sc_w(alias_vaddr, inner_committed);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w through the second virtual alias refreshes the reservation on the same shared backing word",
        inner_value == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("with same-physical-word nested aliases, the innermost sc.w through the latest alias succeeds",
        inner_status == 0U &&
        read32(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(base_vaddr, 0xdeadbeefU);
    AI_ASSERT_NO_EXCEPT_AND("after the inner alias sc.w commits, the older reservation through the first alias is gone and the outer sc.w fails",
        outer_status != 0U &&
        read32(base_vaddr) == inner_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally contains the successful inner alias sc.w payload",
        read32(paddr) == inner_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_inner_success_then_original_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x33445566U;
    const uint32_t inner_committed = 0xa1b2c3d4U;
    const uint32_t refreshed_value = 0x13579bdfU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w through the first virtual alias captures the original shared word before a nested same-size alias original-refresh corner begins",
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(alias_vaddr);
    uint32_t inner_status = sc_w(alias_vaddr, inner_committed);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w through the second virtual alias refreshes the reservation on the same shared word before the inner sc.w commits",
        inner_value == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the inner alias sc.w succeeds first and updates the shared word visible through both aliases",
        inner_status == 0U &&
        read32(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(base_vaddr, 0xdeadbeefU);
    AI_ASSERT_NO_EXCEPT_AND("after the inner alias sc.w commits, the older outer sc.w through the original alias fails because its reservation has been consumed",
        outer_status != 0U &&
        read32(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that outer failure, a fresh lr.w on the original alias still captures the inner committed shared word",
        refreshed_reserved == inner_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the original alias, the shared word is updated again and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally contains the refreshed original-alias sc.w payload after the nested same-size alias corner",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}
