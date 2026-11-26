#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 2U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte load at offset two is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte load at offset two through the second alias still observes another low-word interior 8-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 16)
    );
    TEST_ASSERT("on Spike, a byte load at offset two through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-two byte load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset3_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 3U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte load at offset three is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte load at offset three through the second alias still observes the low-word-edge 8-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 24)
    );
    TEST_ASSERT("on Spike, a low-word-edge byte load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-three byte load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset4_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 4U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte load at offset four is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte load at offset four through the second alias still observes the high-word-edge 8-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, a high-word-edge byte load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-four byte load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset5_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 5U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte load at offset five is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte load at offset five through the second alias still observes the high-word interior 8-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 40)
    );
    TEST_ASSERT("on Spike, a byte load at offset five through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-five byte load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset6_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 6U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte load at offset six is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte load at offset six through the second alias still observes the upper-edge interior 8-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 48)
    );
    TEST_ASSERT("on Spike, a byte load at offset six through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-six byte load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + sizeof(uint64_t) - 1U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a tail-byte load at byte offset seven is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the tail-byte load through the second alias still observes the top-edge 8-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 56)
    );
    TEST_ASSERT("on Spike, a tail-byte load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias tail-byte load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_before_low_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)(uint32_t)(initial_value >> 32) << 32) |
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
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original low word before a same-start doubleword load is issued through the second alias",
        reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-start doubleword load through the second alias still observes both 32-bit halves of the shared payload while the reservation remains live",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-start doubleword load through a different alias still does not break the reservation and the later same-alias low-word sc.w succeeds",
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start doubleword load corner, the shared backing doubleword finally keeps the successful low-word sc.w payload and the untouched upper word",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original upper word before an overlapping doubleword load is issued through the second alias",
        reserved == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("the overlapping doubleword load through the second alias still observes both 32-bit halves of the shared payload while the reservation remains live",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, an overlapping doubleword load through a different alias still does not break the reservation and the later same-alias upper-word sc.w succeeds",
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias overlapping doubleword load corner, the shared backing doubleword finally keeps the untouched low word and the successful upper-word sc.w payload",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_sc_fails_preserves_store_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word store is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word doubleword store through the second virtual alias becomes visible through the first alias before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == alias_store_value
    );
    TEST_ASSERT("the intervening doubleword store invalidates the reservation, so the later same-alias sc.d successful",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally contains the intervening sc.d payload after the alias-store corner sequence",
        read64(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_diff_setidx_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias store is issued",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias doubleword store becomes visible through the base alias before the later sc.d retires",
        excpt.triggered == false &&
        after_alias_store == alias_store_value
    );
    TEST_ASSERT("for the different-setidx same-paddr alias path, the later same-alias sc.d is expected to fail and preserve the alias-store result",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias doubleword-store sequence, the shared backing doubleword keeps the alias-store payload",
        read64(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_diff_setidx_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias store is issued in the failed-sc-consumption corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias doubleword store still becomes visible before the first failing sc.d retires in the failed-sc-consumption corner",
        excpt.triggered == false &&
        after_alias_store == alias_store_value
    );
    TEST_ASSERT("the first same-alias sc.d still fails after the different-setidx alias store and preserves the alias-store result",
        excpt.triggered == false &&
        first_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that failed sc.d, retrying sc.d on the original alias without a fresh lr.d still fails because the failed sc.d has already consumed the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(alias_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-doubleword-store failed-sc-consumption corner, the shared backing doubleword keeps only the alias-store payload",
        read64(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_diff_setidx_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;
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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t failed_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias store is issued in the original-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias doubleword store still becomes visible before the first failing sc.d retires in the original-refresh corner",
        excpt.triggered == false &&
        after_alias_store == alias_store_value
    );
    TEST_ASSERT("the first same-alias sc.d fails and preserves the alias-store result on the different-setidx path",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that failed sc.d, a fresh lr.d on the original alias captures the alias-store value that remained in memory",
        excpt.triggered == false &&
        refreshed_reserved == alias_store_value
    );
    TEST_ASSERT("after refreshing the reservation on the original alias, the following sc.d succeeds and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-doubleword-store failure-and-original-refresh corner, the shared backing doubleword contains only the refreshed original-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_diff_setidx_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;
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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t failed_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias store is issued in the switched-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias doubleword store still becomes visible before the first failing sc.d retires in the switched-refresh corner",
        excpt.triggered == false &&
        after_alias_store == alias_store_value
    );
    TEST_ASSERT("the first same-alias sc.d fails and preserves the alias-store result on the different-setidx path",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that failed sc.d, a fresh lr.d on the switched alias captures the alias-store value that remained in memory",
        excpt.triggered == false &&
        refreshed_reserved == alias_store_value
    );
    TEST_ASSERT("after refreshing the reservation on the switched alias, the following sc.d succeeds and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-doubleword-store failure-and-switched-refresh corner, the shared backing doubleword contains only the refreshed switched-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_failed_sc_clears_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word store is issued through the second alias in the reservation-consumption corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word doubleword store through the second virtual alias becomes visible through the first alias before the first sc.d retires",
        excpt.triggered == false &&
        after_alias_store == alias_store_value
    );
    TEST_ASSERT("the first same-alias sc.d fails after the intervening alias doubleword store and preserves the intervening store data",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that failed sc.d, a second sc.d through the other alias without a fresh lr.d still fails because the prior failed SC has already cleared the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword keeps the first successful sc.d payload after the alias-doubleword-store reservation-consumption corner",
        read64(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_sc_fails_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;
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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word store is issued through the second alias in the refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word doubleword store through the second alias becomes visible through the first alias before the first sc.d retires",
        excpt.triggered == false &&
        after_alias_store == alias_store_value
    );
    TEST_ASSERT("the first same-alias sc.d fails after the intervening alias doubleword store and preserves the intervening store data",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful sc.d, a stale sc.d through the other alias fails because the prior failed SC has already cleared the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refresh_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after the stale alias retry fails, a fresh lr.d/sc.d pair on the original alias re-establishes the shared reservation and succeeds",
        excpt.triggered == false &&
        refreshed_reserved == alias_store_value &&
        refresh_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally keeps the refreshed original-alias sc.d payload after the alias-doubleword-store recovery corner",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_sc_fails_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;
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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word store is issued through the second alias in the switched-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word doubleword store through the second alias becomes visible through the first alias before the first sc.d retires",
        excpt.triggered == false &&
        after_alias_store == alias_store_value
    );
    TEST_ASSERT("the first same-alias sc.d fails after the intervening alias doubleword store and preserves the intervening store data",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful sc.d, a stale sc.d through the other alias fails because the prior failed SC has already cleared the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refresh_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after the stale alias retry fails, a fresh lr.d/sc.d pair on the switched alias re-establishes the shared reservation and succeeds",
        excpt.triggered == false &&
        refreshed_reserved == alias_store_value &&
        refresh_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally keeps the refreshed switched-alias sc.d payload after the alias-doubleword-store recovery corner",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_before_low_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)(uint32_t)(alias_store_value >> 32) << 32) |
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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original low word before a same-start doubleword store is issued through the second alias",
        reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-start doubleword store through the second alias becomes visible across both 32-bit halves before sc.w retires",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-start doubleword store through a different alias invalidates the reservation, so the later same-alias low-word sc.w fails and preserves the intervening update",
        sc_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start doubleword store corner, the shared backing doubleword finally keeps the intervening low-word sc.w payload and the store-updated upper word",
        read64(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_before_upper_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t alias_store_value = 0x55aa33cc77ee11ffULL;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)alias_store_value;

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
    write64(alias_vaddr, alias_store_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original upper word before an overlapping doubleword store is issued through the second alias",
        reserved == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("the overlapping doubleword store through the second alias becomes visible across both 32-bit halves before the upper-word sc.w retires",
        after_alias_store == alias_store_value
    );
    AI_ASSERT_NO_EXCEPT_AND("an overlapping doubleword store through a different alias invalidates the reservation, so the later same-alias upper-word sc.w fails and preserves the intervening update",
        sc_status != 0U &&
        read64(base_vaddr) == alias_store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias overlapping doubleword store corner, the shared backing doubleword finally keeps the store-updated low word and the successful upper-word sc.w payload",
        read64(paddr) == alias_store_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_store_before_doubleword_sc_fails_preserves_intervening_corner()
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
        ((uint64_t)upper_word_value << 32) | (uint64_t)(uint32_t)initial_value;
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
    write32(alias_upper_word_vaddr, upper_word_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before an upper-word store is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the upper-word store through the second alias becomes visible as a partial update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("an upper-word store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-word store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_word_store_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint32_t low_word_value = 0x55aa33ccU;
    const uint64_t expected_after_store =
        ((uint64_t)(uint32_t)(initial_value >> 32) << 32) |
        (uint64_t)low_word_value;
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
    write32(alias_vaddr, low_word_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a low-word store is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the low-word store through the second alias becomes visible as a same-start partial update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a same-start low-word store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias low-word store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte0_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x33ccU;
    const uint64_t expected_after_store =
        (initial_value & ~0xffffULL) |
        (uint64_t)halfword_value;
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
    write16(alias_vaddr, halfword_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-start halfword store is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-start halfword store through the second alias becomes visible as a low-edge partial update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a same-start halfword store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start halfword store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte2_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_halfword_vaddr = alias_vaddr + 2U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x55aaU;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffffULL << 16)) |
        ((uint64_t)halfword_value << 16);
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
    write16(alias_halfword_vaddr, halfword_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a halfword store at byte offset two is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the halfword store at byte offset two through the second alias becomes visible as an interior partial update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("an interior halfword store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias interior-halfword store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte4_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_halfword_vaddr = alias_vaddr + 4U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x55aaU;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffffULL << 32)) |
        ((uint64_t)halfword_value << 32);
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
    write16(alias_halfword_vaddr, halfword_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a halfword store at byte offset four is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the halfword store at byte offset four through the second alias becomes visible as a high-word-edge partial update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a high-word-edge halfword store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-four halfword store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte6_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_halfword_vaddr = alias_vaddr + 6U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x55aaU;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffffULL << 48)) |
        ((uint64_t)halfword_value << 48);
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
    write16(alias_halfword_vaddr, halfword_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a halfword store at byte offset six is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the halfword store at byte offset six through the second alias becomes visible as an upper-edge partial update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("an upper-edge halfword store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-edge halfword store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_store_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x22U;
    const uint64_t expected_after_store =
        (initial_value & ~0xffULL) |
        (uint64_t)byte_value;
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
    write8(alias_vaddr, byte_value);
    uint64_t after_alias_store = read64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-start low-byte store is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-start low-byte store through the second alias becomes visible as a low-edge one-byte update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a same-start low-byte store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start low-byte store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 1U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x33U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 8)) |
        ((uint64_t)byte_value << 8);
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
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte store at offset one is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte store at offset one through the second alias becomes visible as a low-word interior one-byte update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset one through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-one byte store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 2U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x55U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 16)) |
        ((uint64_t)byte_value << 16);
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
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte store at offset two is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte store at offset two through the second alias becomes visible as a low-word interior one-byte update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset two through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-two byte store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset3_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 3U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x44U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 24)) |
        ((uint64_t)byte_value << 24);
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
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte store at offset three is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte store at offset three through the second alias becomes visible as a low-word-edge one-byte update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a low-word-edge byte store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-three byte store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset4_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 4U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x66U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 32)) |
        ((uint64_t)byte_value << 32);
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
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte store at offset four is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte store at offset four through the second alias becomes visible as a high-word-edge one-byte update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a high-word-edge byte store through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-four byte store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset6_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 6U;
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x77U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 48)) |
        ((uint64_t)byte_value << 48);
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
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte store at offset six is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte store at offset six through the second alias becomes visible as an upper-edge interior one-byte update of the shared doubleword before sc.d retires",
        excpt.triggered == false &&
        after_alias_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset six through a different alias invalidates the reservation, so the later same-alias sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == expected_after_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-six byte store corner, the shared backing doubleword finally keeps only the successful sc.d payload",
        read64(paddr) == expected_after_store
    );

    TEST_END();
}
