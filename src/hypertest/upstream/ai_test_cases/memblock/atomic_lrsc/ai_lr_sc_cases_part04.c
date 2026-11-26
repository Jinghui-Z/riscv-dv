#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_same_paddr_alias_nested_inner_success_then_switched_refresh_succeeds_corner()
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
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w through the first virtual alias captures the original shared word before a nested same-size alias switched-refresh corner begins",
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
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that outer failure, a fresh lr.w on the switched alias still captures the inner committed shared word",
        refreshed_reserved == inner_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the switched alias, the shared word is updated again and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally contains the refreshed switched-alias sc.w payload after the nested same-size alias corner",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_doubleword_inner_only_succeeds()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("an outer lr.d through the first virtual alias captures the original shared doubleword before a nested alias reservation is created",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(alias_vaddr, inner_committed);
    TEST_ASSERT("a nested lr.d through the second virtual alias refreshes the reservation on the same shared doubleword",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("with same-physical-word nested doubleword aliases, the innermost sc.d through the latest alias succeeds",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after the inner alias sc.d commits, the older reservation through the first alias is gone and the outer sc.d fails",
        excpt.triggered == false &&
        outer_status != 0U &&
        read64(base_vaddr) == inner_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally contains the successful inner alias sc.d payload",
        read64(paddr) == inner_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_doubleword_inner_success_then_original_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;
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
    uint64_t outer_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("an outer lr.d through the first virtual alias captures the original shared doubleword before a nested same-size alias original-refresh corner begins",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(alias_vaddr, inner_committed);
    TEST_ASSERT("a nested lr.d through the second virtual alias refreshes the reservation on the same shared doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("on Spike, the inner alias sc.d succeeds first and updates the shared doubleword visible through both aliases",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after the inner alias sc.d commits, the older outer sc.d through the original alias fails because its reservation has been consumed",
        excpt.triggered == false &&
        outer_status != 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that outer doubleword failure, a fresh lr.d on the original alias still captures the inner committed shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == inner_committed
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the original alias, the shared doubleword is fully updated again and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally contains the refreshed original-alias sc.d payload after the nested same-size alias corner",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_doubleword_inner_success_then_switched_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;
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
    uint64_t outer_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("an outer lr.d through the first virtual alias captures the original shared doubleword before a nested same-size alias switched-refresh corner begins",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(alias_vaddr, inner_committed);
    TEST_ASSERT("a nested lr.d through the second virtual alias refreshes the reservation on the same shared doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("on Spike, the inner alias sc.d succeeds first and updates the shared doubleword visible through both aliases",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after the inner alias sc.d commits, the older outer sc.d through the original alias fails because its reservation has been consumed",
        excpt.triggered == false &&
        outer_status != 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that outer doubleword failure, a fresh lr.d on the switched alias still captures the inner committed shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == inner_committed
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the switched alias, the shared doubleword is fully updated again and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally contains the refreshed switched-alias sc.d payload after the nested same-size alias corner",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_only_succeeds()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("an outer lr.d through the first virtual alias captures the original shared doubleword before a mixed-size nested alias reservation is created",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(alias_vaddr);
    uint32_t inner_status = sc_w(alias_vaddr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w through the second virtual alias captures the low word of the same shared doubleword",
        inner_value == (uint32_t)initial_value
    );
    TEST_ASSERT("with same-physical-word nested aliases but different sizes, the newer narrower sc.w through the latest alias succeeds and updates only the low word",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == 0x11223344a5a55a5aULL
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after the mixed-size nested alias sequence, the older wider sc.d through the first alias fails and preserves the inner low-word update",
        excpt.triggered == false &&
        outer_status != 0U &&
        read64(base_vaddr) == 0x11223344a5a55a5aULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally keeps the successful inner low-word update after the older wider alias sc.d fails",
        read64(paddr) == 0x11223344a5a55a5aULL
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_success_then_original_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t refreshed_value = 0x8877665544332211ULL;
    const uint64_t first_expected = 0x11223344a5a55a5aULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("an outer lr.d through the first virtual alias captures the original shared doubleword before a mixed-size nested alias original-refresh corner begins",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(alias_vaddr);
    uint32_t inner_status = sc_w(alias_vaddr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w through the second virtual alias captures the low word of the shared doubleword before the inner sc.w commits",
        inner_value == (uint32_t)initial_value
    );
    TEST_ASSERT("on Spike, the newer narrower sc.w through the second alias succeeds and leaves a partial low-word update visible to both aliases",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == first_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(base_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("after the inner alias sc.w commits, the older wider sc.d through the first alias fails because its reservation has been consumed",
        excpt.triggered == false &&
        outer_status != 0U &&
        read64(base_vaddr) == first_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that outer failure, a fresh lr.d on the original alias captures the partially updated shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_expected
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the original alias, the shared doubleword is fully overwritten and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the nested mixed-size alias original-doubleword refresh corner, the shared backing doubleword contains only the refreshed original-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_success_then_switched_word_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t inner_committed = 0xa5a55a5aU;
    const uint64_t first_expected = 0x11223344a5a55a5aULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        0x1122334400000000ULL | (uint64_t)refreshed_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("an outer lr.d through the first virtual alias captures the original shared doubleword before a mixed-size nested alias switched-refresh corner begins",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(alias_vaddr);
    uint32_t inner_status = sc_w(alias_vaddr, inner_committed);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w through the second virtual alias captures the low word of the shared doubleword before the inner sc.w commits",
        inner_value == (uint32_t)initial_value
    );
    TEST_ASSERT("on Spike, the newer narrower sc.w through the switched alias succeeds first and leaves a partial low-word update visible to both aliases",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == first_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(base_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("after that inner alias sc.w commits, the older wider sc.d through the original alias fails because its reservation has been consumed",
        excpt.triggered == false &&
        outer_status != 0U &&
        read64(base_vaddr) == first_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the outer failure, a fresh lr.w on the switched alias captures the inner committed low word",
        refreshed_reserved == inner_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the switched alias, the shared low word is updated again while the original high word is preserved",
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the nested mixed-size alias switched-word refresh corner, the shared backing doubleword keeps the original high word and the refreshed switched-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_success_then_original_word_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t inner_committed = 0xa5a55a5aU;
    const uint64_t first_expected = 0x11223344a5a55a5aULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        0x1122334400000000ULL | (uint64_t)refreshed_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t outer_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("an outer lr.d through the first virtual alias captures the original shared doubleword before a mixed-size nested alias original-word refresh corner begins",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(alias_vaddr);
    uint32_t inner_status = sc_w(alias_vaddr, inner_committed);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w through the second virtual alias captures the low word of the shared doubleword before the inner sc.w commits",
        inner_value == (uint32_t)initial_value
    );
    TEST_ASSERT("on Spike, the newer narrower sc.w through the second alias succeeds first and leaves a partial low-word update visible to both aliases",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == first_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(base_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("after that inner alias sc.w commits, the older wider sc.d through the original alias fails because its reservation has been consumed",
        excpt.triggered == false &&
        outer_status != 0U &&
        read64(base_vaddr) == first_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the outer failure, a fresh lr.w on the original alias captures the inner committed low word",
        refreshed_reserved == inner_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a narrower lr.w/sc.w pair on the original alias, the shared low word is updated again while the original high word is preserved",
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the nested mixed-size alias original-word refresh corner, the shared backing doubleword keeps the original high word and the refreshed original-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_success_then_switched_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t inner_committed = 0xa5a55a5aU;
    const uint64_t first_expected = 0x11223344a5a55a5aULL;
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
    uint64_t outer_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("an outer lr.d through the first virtual alias captures the original shared doubleword before a mixed-size nested alias switched-doubleword refresh corner begins",
        excpt.triggered == false &&
        outer_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t inner_value = lr_w(alias_vaddr);
    uint32_t inner_status = sc_w(alias_vaddr, inner_committed);
    AI_ASSERT_NO_EXCEPT_AND("a nested lr.w through the second virtual alias captures the low word of the shared doubleword before the inner sc.w commits",
        inner_value == (uint32_t)initial_value
    );
    TEST_ASSERT("on Spike, the newer narrower sc.w through the second alias succeeds first and leaves a partial low-word update visible to both aliases",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == first_expected
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = ai_lr_sc_sc_d_status(base_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("after that inner alias sc.w commits, the older wider sc.d through the original alias fails because its reservation has been consumed",
        excpt.triggered == false &&
        outer_status != 0U &&
        read64(base_vaddr) == first_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after the outer failure, a fresh lr.d on the switched alias captures the partially updated shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_expected
    );
    TEST_ASSERT("after refreshing with a wider lr.d/sc.d pair on the switched alias, the shared doubleword is fully overwritten again and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the nested mixed-size alias switched-doubleword refresh corner, the shared backing doubleword contains only the refreshed switched-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_only_succeeds()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w through the first virtual alias captures the low word before a wider nested alias reservation is created",
        outer_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(alias_vaddr, inner_committed);
    TEST_ASSERT("a nested lr.d through the second virtual alias captures the full shared doubleword before the wider sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("with same-physical-word nested aliases and a wider inner reservation, the latest sc.d succeeds and updates the whole shared doubleword",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(base_vaddr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the inner alias sc.d commits, the older narrower sc.w through the first alias fails",
        outer_status != 0U &&
        read64(base_vaddr) == inner_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally contains the successful inner alias sc.d payload after the older narrower sc.w fails",
        read64(paddr) == inner_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_success_then_original_word_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        0xa1b2c3d400000000ULL | (uint64_t)refreshed_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w through the first virtual alias captures the low word before a wider nested alias original-refresh corner begins",
        outer_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(alias_vaddr, inner_committed);
    TEST_ASSERT("a nested lr.d through the second virtual alias captures the full shared doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("on Spike, the newer wider sc.d through the second alias succeeds and updates the whole shared doubleword",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(base_vaddr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the inner alias sc.d commits, the older narrower sc.w through the first alias fails because its reservation has been consumed",
        outer_status != 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that outer failure, a fresh lr.w on the original alias captures the low word of the committed shared doubleword",
        refreshed_reserved == (uint32_t)inner_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the original alias, only the shared low word is updated while the high word from the earlier sc.d is preserved",
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the nested mixed-size alias original-word refresh corner, the shared backing doubleword keeps the earlier high word and the refreshed original-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_success_then_switched_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;
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
    uint32_t outer_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w through the first virtual alias captures the low word before a wider nested alias switched-doubleword refresh corner begins",
        outer_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(alias_vaddr, inner_committed);
    TEST_ASSERT("a nested lr.d through the second virtual alias captures the full shared doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("on Spike, the newer wider sc.d through the switched alias succeeds first and updates the whole shared doubleword",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(base_vaddr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after that inner alias sc.d commits, the older narrower sc.w through the original alias fails because its reservation has been consumed",
        outer_status != 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after the outer failure, a fresh lr.d on the switched alias captures the inner committed shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == inner_committed
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the switched alias, the shared doubleword is fully overwritten again",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the nested mixed-size alias switched-doubleword refresh corner, the shared backing doubleword contains only the refreshed switched-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_success_then_original_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;
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
    uint32_t outer_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w through the first virtual alias captures the low word before a wider nested alias original-doubleword refresh corner begins",
        outer_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(alias_vaddr, inner_committed);
    TEST_ASSERT("a nested lr.d through the second virtual alias captures the full shared doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("on Spike, the newer wider sc.d through the second alias succeeds and updates the whole shared doubleword",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(base_vaddr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the inner alias sc.d commits, the older narrower sc.w through the first alias fails because its reservation has been consumed",
        outer_status != 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after the outer failure, a fresh lr.d on the original alias captures the inner committed shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == inner_committed
    );
    TEST_ASSERT("after refreshing with a wider lr.d/sc.d pair on the original alias, the shared doubleword is fully overwritten again and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the nested mixed-size alias original-doubleword refresh corner, the shared backing doubleword contains only the refreshed original-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_success_then_switched_word_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t inner_committed = 0xa1b2c3d4e5f60718ULL;
    const uint32_t refreshed_value = 0x13579bdfU;
    const uint64_t refreshed_expected =
        0xa1b2c3d400000000ULL | (uint64_t)refreshed_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t outer_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an outer lr.w through the first virtual alias captures the low word before a wider nested alias switched-word refresh corner begins",
        outer_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t inner_value = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t inner_status = ai_lr_sc_sc_d_status(alias_vaddr, inner_committed);
    TEST_ASSERT("a nested lr.d through the second virtual alias captures the full shared doubleword before the inner sc.d commits",
        excpt.triggered == false &&
        inner_value == initial_value
    );
    TEST_ASSERT("on Spike, the newer wider sc.d through the second alias succeeds and updates the whole shared doubleword",
        excpt.triggered == false &&
        inner_status == 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t outer_status = sc_w(base_vaddr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the inner alias sc.d commits, the older narrower sc.w through the first alias fails because its reservation has been consumed",
        outer_status != 0U &&
        read64(base_vaddr) == inner_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the outer failure, a fresh lr.w on the switched alias captures the low word of the committed shared doubleword",
        refreshed_reserved == (uint32_t)inner_committed
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a narrower lr.w/sc.w pair on the switched alias, only the shared low word is updated while the high word from the earlier sc.d is preserved",
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the nested mixed-size alias switched-word refresh corner, the shared backing doubleword keeps the earlier high word and the refreshed switched-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_arch_lr_sc_timeout_over_64_cycles_fails()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x55667788U;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w first captures the original word before the reservation is held across a long cycle gap",
        reserved == initial_value
    );

    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the lr/sc gap exceeds sixty-four cycles, sc.w fails instead of committing",
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        sc_status != 0U &&
        ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_timeout_over_64_cycles_adjacent_word_preserved_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t adjacent_addr = ai_lr_sc_addr(1);
    const uint32_t initial_value = 0x55667788U;
    const uint32_t adjacent_value = 0x11223344U;

    ai_lr_sc_prepare_words(initial_value, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w first captures the original target word before the architectural timeout side-effect corner begins",
        reserved == initial_value
    );

    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("after the architectural lr/sc gap exceeds sixty-four cycles, the failed sc.w leaves both the target word and its adjacent word unchanged",
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        sc_status != 0U &&
        read32(addr) == initial_value &&
        read32(adjacent_addr) == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the architectural timeout adjacent-word corner, backing memory still keeps both the original target word and the untouched adjacent word",
        ai_lr_sc_words[0] == initial_value &&
        ai_lr_sc_words[1] == adjacent_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_timeout_then_refresh_succeeds()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x55667788U;
    const uint32_t refreshed_value = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w first captures the original word before the timeout-and-refresh recovery sequence begins",
        reserved == initial_value
    );

    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w fails once the reservation has timed out across more than sixty-four cycles",
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        stale_status != 0U &&
        ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the timeout failure, a fresh lr.w can still establish a new reservation on the same word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation with a new lr.w, the following sc.w succeeds and commits its payload",
        refreshed_status == 0U &&
        ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_timeout_then_refresh_adjacent_word_preserved_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t adjacent_addr = ai_lr_sc_addr(1);
    const uint32_t initial_value = 0x55667788U;
    const uint32_t adjacent_value = 0x11223344U;
    const uint32_t refreshed_value = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(initial_value, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w first captures the original target word before the architectural timeout-and-refresh side-effect corner begins",
        reserved == initial_value
    );

    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("the first timed-out sc.w fails and leaves both the target word and the adjacent word unchanged",
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        stale_status != 0U &&
        read32(addr) == initial_value &&
        read32(adjacent_addr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the timeout failure, a fresh lr.w still captures the original word before the recovery commit",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation with a new lr.w, the following sc.w succeeds and updates only the target word while preserving the adjacent word",
        refreshed_status == 0U &&
        read32(addr) == refreshed_value &&
        read32(adjacent_addr) == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the architectural timeout refresh adjacent-word corner, backing memory keeps the refreshed target word and the untouched adjacent word",
        ai_lr_sc_words[0] == refreshed_value &&
        ai_lr_sc_words[1] == adjacent_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_timeout_repeated_fails_stable_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x55667788U;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t first_reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("the first lr.w captures the original word before the repeated architectural timeout probe begins",
        first_reserved == initial_value
    );

    uint64_t first_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w fails once the lr/sc gap exceeds sixty-four cycles and leaves the word unchanged",
        first_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        first_status != 0U &&
        ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("after one timeout failure, a fresh lr.w still captures the same original word for a second timeout attempt",
        second_reserved == initial_value
    );

    uint64_t second_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("the second timed-out sc.w also fails and still leaves the word unchanged",
        second_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        second_status != 0U &&
        ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_timeout_repeated_fails_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x55667788U;
    const uint32_t refreshed_value = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t first_reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("the first lr.w captures the original word before the repeated-timeout recovery corner begins",
        first_reserved == initial_value
    );

    uint64_t first_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w fails after the reservation times out across more than sixty-four cycles",
        first_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        first_status != 0U &&
        ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("after the first timeout failure, a second lr.w still captures the original word before another timeout attempt",
        second_reserved == initial_value
    );

    uint64_t second_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0x12345678U);
    AI_ASSERT_NO_EXCEPT_AND("the second timed-out sc.w also fails and still leaves the original word unchanged",
        second_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        second_status != 0U &&
        ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after two timeout failures, a fresh lr.w still captures the original word for the recovery commit",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after two timeout failures, a fresh lr.w/sc.w pair still succeeds and updates the word",
        refreshed_status == 0U &&
        ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_fails_spike_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);
    uint32_t stale_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("after the doubleword reservation is held for more than sixty-four cycles, sc.d fails and leaves memory unchanged",
        excpt.triggered == false &&
        reserved == initial_value &&
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        stale_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_adjacent_doubleword_preserved_spike_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t adjacent_addr = ai_lr_sc_addr(2);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t adjacent_value = 0x8899aabbccddeeffULL;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    *(uint64_t *)&ai_lr_sc_words[2] = adjacent_value;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);
    uint32_t stale_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("after more than sixty-four cycles, delayed sc.d fails and preserves both target and adjacent doublewords",
        excpt.triggered == false &&
        reserved == initial_value &&
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        stale_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value &&
        *(uint64_t *)&ai_lr_sc_words[2] == adjacent_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_then_refresh_succeeds_spike_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa5a55a5af0f00f0fULL;
    const uint64_t refreshed_value = 0x0f0ff0f05a5aa5a5ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, first_committed);
    TEST_ASSERT("after the doubleword reservation is held for more than sixty-four cycles, delayed sc.d fails and leaves memory unchanged",
        excpt.triggered == false &&
        reserved == initial_value &&
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after that timeout failure, a fresh lr.d still captures the original doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after recreating the reservation with a fresh lr.d, the next sc.d succeeds and overwrites the original payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_then_refresh_adjacent_doubleword_preserved_spike_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t adjacent_addr = ai_lr_sc_addr(2);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t adjacent_value = 0x8899aabbccddeeffULL;
    const uint64_t first_committed = 0xa5a55a5af0f00f0fULL;
    const uint64_t refreshed_value = 0x0f0ff0f05a5aa5a5ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    *(uint64_t *)&ai_lr_sc_words[2] = adjacent_value;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, first_committed);
    TEST_ASSERT("after more than sixty-four cycles, delayed sc.d fails and leaves both target and adjacent doublewords unchanged",
        excpt.triggered == false &&
        reserved == initial_value &&
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value &&
        *(uint64_t *)&ai_lr_sc_words[2] == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after that timeout failure, a fresh lr.d still captures the original target doubleword before the refresh commit",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after recreating the reservation with a fresh lr.d, the next sc.d succeeds, overwrites only the target doubleword, and still preserves the adjacent doubleword",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value &&
        *(uint64_t *)&ai_lr_sc_words[2] == adjacent_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_clears_reservation_spike_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_committed = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, first_committed);
    TEST_ASSERT("after the doubleword reservation is held for more than sixty-four cycles, delayed sc.d fails and leaves memory unchanged",
        excpt.triggered == false &&
        reserved == initial_value &&
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("after the timeout failure, a second sc.d without a fresh lr.d still fails and leaves memory unchanged",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_second_sc_fail_then_refresh_succeeds_spike_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t adjacent_addr = ai_lr_sc_addr(2);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t adjacent_value = 0x8899aabbccddeeffULL;
    const uint64_t first_committed = 0xa5a55a5af0f00f0fULL;
    const uint64_t refreshed_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    *(uint64_t *)&ai_lr_sc_words[2] = adjacent_value;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, first_committed);
    TEST_ASSERT("after more than sixty-four cycles, delayed sc.d fails before the refresh corner begins",
        excpt.triggered == false &&
        reserved == initial_value &&
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value &&
        *(uint64_t *)&ai_lr_sc_words[2] == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("without a fresh lr.d, the second sc.d still fails because the reservation already timed out",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value &&
        *(uint64_t *)&ai_lr_sc_words[2] == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after that timeout failure, a fresh lr.d still captures the original doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing the reservation with a new lr.d, the following sc.d succeeds, overwrites only the target doubleword, and preserves the adjacent doubleword",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value &&
        *(uint64_t *)&ai_lr_sc_words[2] == adjacent_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_original_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t first_committed = 0x0badc0deU;
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
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t alias_status = sc_w(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first alias captures the shared doubleword before the immediate alias-switch narrowing original-refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first mixed-size sc.w through the same-paddr alias fails because LR.D and SC.W widths differ, leaving the original-alias doubleword refresh to start from the original value",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that failed mixed-size alias sc.w, a fresh lr.d on the original alias captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the original alias, the shared doubleword is fully overwritten and both aliases observe the new payload",
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

bool ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_alias_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t first_committed = 0x0badc0deU;
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
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t alias_status = sc_w(alias_vaddr, first_committed);
    TEST_ASSERT("lr.d through the first alias captures the shared doubleword before the immediate alias-switch narrowing switched-doubleword-refresh corner runs",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the first mixed-size sc.w through the same-paddr alias fails because LR.D and SC.W widths differ, leaving the switched-alias doubleword refresh to start from the original value",
        excpt.triggered == false &&
        alias_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that failed mixed-size alias sc.w, a fresh lr.d on the switched alias captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the switched alias, the shared doubleword is fully overwritten and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed-size switched-alias doubleword refresh corner, the shared backing doubleword contains only the refreshed switched-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_fails()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x1234fedcU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w first captures the original word before the microarchitectural timeout gap begins",
        reserved == initial_value
    );

    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("when more than sixty-four cycles separate lr.w and sc.w, the sc.w fails",
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        sc_status != 0U &&
        ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_adjacent_word_preserved_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t adjacent_addr = ai_lr_sc_addr(1);
    const uint32_t initial_value = 0x1234fedcU;
    const uint32_t adjacent_value = 0x55667788U;

    ai_lr_sc_prepare_words(initial_value, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w first captures the original target word before the timeout side-effect corner begins",
        reserved == initial_value
    );

    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("when a word-sc times out after more than sixty-four cycles, neither the target word nor the adjacent word is modified",
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        sc_status != 0U &&
        read32(addr) == initial_value &&
        read32(adjacent_addr) == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the timeout side-effect corner, backing memory still keeps both the original target word and the untouched adjacent word",
        ai_lr_sc_words[0] == initial_value &&
        ai_lr_sc_words[1] == adjacent_value
    );

    TEST_END();
}
