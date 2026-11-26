#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_same_address_intervening_upper_word_store_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    write32(upper_word_addr, upper_word_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before an overlapping upper-word store is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening upper-word store becomes visible as an overlapping high-half update of the shared doubleword before the later sc.w retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("on Spike, an overlapping upper-word store still does not break the reservation and the following upper-word sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_store_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t intervening_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write64(addr, intervening_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-address ordinary doubleword store is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-address ordinary doubleword store becomes architecturally visible before the later sc.d retires",
        excpt.triggered == false &&
        after_store == intervening_value
    );
    TEST_ASSERT("after the same-address ordinary doubleword store, sc.d either succeeds with the committed value or fails while preserving the intervening store",
        excpt.triggered == false &&
        (((sc_status == 0U) &&
          (*(uint64_t *)&ai_lr_sc_words[0] == committed_value)) ||
         ((sc_status != 0U) &&
          (*(uint64_t *)&ai_lr_sc_words[0] == intervening_value)))
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_store_adjacent_doubleword_preserved_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t adjacent_addr = ai_lr_sc_addr(2);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t intervening_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    *(uint64_t *)&ai_lr_sc_words[2] = adjacent_value;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write64(addr, intervening_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-address ordinary doubleword store is inserted between lr.d and sc.d in the adjacent-doubleword-preservation corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-address ordinary doubleword store becomes architecturally visible before the later sc.d retires in the adjacent-doubleword-preservation corner",
        excpt.triggered == false &&
        after_store == intervening_value
    );
    TEST_ASSERT("after the same-address ordinary doubleword store, sc.d either succeeds with the committed value or fails while preserving the intervening store",
        excpt.triggered == false &&
        (((sc_status == 0U) &&
          (*(uint64_t *)&ai_lr_sc_words[0] == committed_value)) ||
         ((sc_status != 0U) &&
          (*(uint64_t *)&ai_lr_sc_words[0] == intervening_value)))
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = read64(adjacent_addr);
    TEST_ASSERT("the adjacent doubleword remains unchanged across the same-address store.d-plus-sc.d sequence",
        excpt.triggered == false &&
        observed_adjacent == adjacent_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_store_success_consumes_reservation_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t intervening_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write64(addr, intervening_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-address ordinary doubleword store is inserted between lr.d and sc.d in the reservation-consumption corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-address ordinary doubleword store becomes architecturally visible before the first sc.d retires",
        excpt.triggered == false &&
        after_store == intervening_value
    );
    const uint64_t first_final_value = *(uint64_t *)&ai_lr_sc_words[0];
    TEST_ASSERT("after the intervening same-address ordinary doubleword store, the first sc.d either succeeds and consumes the reservation or fails while preserving the store",
        excpt.triggered == false &&
        (((sc_status == 0U) &&
          (first_final_value == committed_value)) ||
         ((sc_status != 0U) &&
          (first_final_value == intervening_value)))
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x8877665544332211ULL);
    TEST_ASSERT("a second sc.d without a fresh lr.d cannot create a new successful update",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_final_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_store_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t intervening_value = 0x55aa33cc77ee11ffULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refreshed_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write64(addr, intervening_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-address ordinary doubleword store is inserted between lr.d and sc.d in the refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-address ordinary doubleword store becomes architecturally visible before the first sc.d retires",
        excpt.triggered == false &&
        after_store == intervening_value
    );
    const uint64_t first_final_value = *(uint64_t *)&ai_lr_sc_words[0];
    TEST_ASSERT("after the intervening same-address ordinary doubleword store, the first sc.d either succeeds and consumes the reservation or fails while preserving the store",
        excpt.triggered == false &&
        (((sc_status == 0U) &&
          (first_final_value == committed_value)) ||
         ((sc_status != 0U) &&
          (first_final_value == intervening_value)))
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x8877665544332211ULL);
    TEST_ASSERT("a stale retry without a fresh lr.d cannot create a new successful update",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refresh_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after the stale retry fails, a fresh lr.d/sc.d pair re-establishes the reservation and succeeds",
        excpt.triggered == false &&
        refreshed_reserved == first_final_value &&
        refresh_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_store_before_low_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t intervening_value = 0x55aa33cc77ee11ffULL;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)(uint32_t)(intervening_value >> 32) << 32) |
        (uint64_t)committed_value;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    write64(addr, intervening_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    TEST_ASSERT("lr.w captures the original low word before a same-start doubleword store is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the intervening same-start doubleword store becomes architecturally visible across both 32-bit halves before sc.w retires",
        excpt.triggered == false &&
        after_store == intervening_value
    );
    TEST_ASSERT("after the same-start doubleword store, low-word sc.w either succeeds with the committed low word or fails while preserving the intervening store",
        excpt.triggered == false &&
        (((sc_status == 0U) &&
          (*(uint64_t *)&ai_lr_sc_words[0] == expected_final)) ||
         ((sc_status != 0U) &&
          (*(uint64_t *)&ai_lr_sc_words[0] == intervening_value)))
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_store_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint64_t intervening_value = 0x55aa33cc77ee11ffULL;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)intervening_value;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    write64(addr, intervening_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before an overlapping doubleword store is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening overlapping doubleword store becomes architecturally visible across both 32-bit halves before the upper-word sc.w retires",
        excpt.triggered == false &&
        after_store == intervening_value
    );
    TEST_ASSERT("after the overlapping doubleword store, upper-word sc.w either succeeds with the committed high word or fails while preserving the intervening store",
        excpt.triggered == false &&
        (((sc_status == 0U) &&
          (*(uint64_t *)&ai_lr_sc_words[0] == expected_final)) ||
         ((sc_status != 0U) &&
          (*(uint64_t *)&ai_lr_sc_words[0] == intervening_value)))
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_word_store_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint32_t upper_word_value = 0x55aa33ccU;
    const uint64_t expected_after_store =
        ((uint64_t)upper_word_value << 32) | (uint64_t)(uint32_t)initial_value;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write32(upper_word_addr, upper_word_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before an upper-word store touches only the high half of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening upper-word store becomes visible as a partial update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a same-address upper-word store invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_word_store_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint32_t low_word_value = 0x55aa33ccU;
    const uint64_t expected_after_store =
        ((uint64_t)(uint32_t)(initial_value >> 32) << 32) |
        (uint64_t)low_word_value;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write32(addr, low_word_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a low-word store touches the same start byte of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening low-word store becomes visible as a same-start partial update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("on Spike, a same-start low-word store still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_halfword_at_byte0_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x33ccU;
    const uint64_t expected_after_store =
        (initial_value & ~0xffffULL) |
        (uint64_t)halfword_value;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write16(addr, halfword_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-start halfword store touches the low edge of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-start halfword store becomes visible as a low-edge partial update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("on Spike, a same-start halfword store still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_halfword_at_byte2_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t halfword_addr = addr + 2U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x55aaU;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffffULL << 16)) |
        ((uint64_t)halfword_value << 16);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write16(halfword_addr, halfword_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a halfword store at byte offset two touches the interior of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening halfword store at byte offset two becomes visible as an interior partial update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("an interior halfword store invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_halfword_at_byte4_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t halfword_addr = addr + 4U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x55aaU;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffffULL << 32)) |
        ((uint64_t)halfword_value << 32);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write16(halfword_addr, halfword_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a halfword store at byte offset four touches the first two bytes of the high 32-bit word inside the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening halfword store at byte offset four becomes visible as a high-word-edge partial update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a high-word-edge halfword store invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_halfword_at_byte6_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t halfword_addr = addr + 6U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint16_t halfword_value = 0x55aaU;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffffULL << 48)) |
        ((uint64_t)halfword_value << 48);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write16(halfword_addr, halfword_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a halfword store at byte offset six touches the upper edge of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening halfword store at byte offset six becomes visible as an upper-edge partial update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("an upper-edge halfword store invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_byte_store_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x22U;
    const uint64_t expected_after_store =
        (initial_value & ~0xffULL) |
        (uint64_t)byte_value;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write8(addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-start low-byte store touches the low edge of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-start low-byte store becomes visible as a low-edge one-byte update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("on Spike, a same-start low-byte store still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset1_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 1U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x33U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 8)) |
        ((uint64_t)byte_value << 8);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write8(byte_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte store at offset one touches the low-word interior of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte store at offset one becomes visible as a low-word interior one-byte update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset one invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset2_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 2U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x55U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 16)) |
        ((uint64_t)byte_value << 16);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write8(byte_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte store at offset two touches the low-word interior of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte store at offset two becomes visible as a low-word interior one-byte update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset two invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset3_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 3U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x44U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 24)) |
        ((uint64_t)byte_value << 24);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write8(byte_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte store at offset three touches the last byte of the low 32-bit word inside the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte store at offset three becomes visible as a low-word-edge one-byte update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a low-word-edge byte store invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset4_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 4U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x66U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 32)) |
        ((uint64_t)byte_value << 32);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write8(byte_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte store at offset four touches the first byte of the high 32-bit word inside the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte store at offset four becomes visible as a high-word-edge one-byte update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a high-word-edge byte store invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset6_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 6U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x77U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 48)) |
        ((uint64_t)byte_value << 48);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write8(byte_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte store at offset six touches the upper-edge interior of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte store at offset six becomes visible as an upper-edge interior one-byte update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a byte store at offset six invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_tail_byte_store_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t tail_byte_addr = addr + sizeof(uint64_t) - 1U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t tail_byte_value = 0x55U;
    const uint64_t expected_after_store =
        (initial_value & 0x00ffffffffffffffULL) |
        ((uint64_t)tail_byte_value << 56);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write8(tail_byte_addr, tail_byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a tail-byte store touches only the most-significant byte of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening tail-byte store becomes visible as a one-byte update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("a same-address tail-byte store invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset5_before_doubleword_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 5U;
    const uint64_t initial_value = 0x2468ace013579bdfULL;
    const uint8_t byte_value = 0x55U;
    const uint64_t expected_after_store =
        (initial_value & ~(0xffULL << 40)) |
        ((uint64_t)byte_value << 40);
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    write8(byte_addr, byte_value);
    uint64_t after_store = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte store at offset five touches the interior of the reserved doubleword",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte store at offset five becomes visible as an interior one-byte update of the reserved doubleword before sc.d retires",
        excpt.triggered == false &&
        after_store == expected_after_store
    );
    TEST_ASSERT("an interior byte store invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_store
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_amo_sc_fails_preserves_amo()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x10203040U;
    const uint32_t amo_delta = 0x01010101U;
    const uint32_t expected_after_amo = initial_value + amo_delta;
    const uint32_t committed_value = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t amo_old_value = amoadd_w(addr, amo_delta);
    uint32_t after_amo = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address AMO is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening amoadd.w itself observes and updates the reserved word normally before sc.w retires",
        amo_old_value == initial_value &&
        after_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-address AMO invalidates the reservation, so the following sc.w fails and preserves the intervening update",
        sc_status != 0U &&
        ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_amo_failed_sc_clears_reservation_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x10203040U;
    const uint32_t amo_delta = 0x01010101U;
    const uint32_t expected_after_amo = initial_value + amo_delta;
    const uint32_t committed_value = 0xa5a55a5aU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t amo_old_value = amoadd_w(addr, amo_delta);
    uint32_t after_amo = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address AMO is inserted between lr.w and sc.w in the reservation-consumption corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening amoadd.w itself observes and updates the reserved word normally before the first sc.w retires",
        amo_old_value == initial_value &&
        after_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w fails after the intervening same-address AMO and preserves the intervening AMO result",
        sc_status != 0U &&
        ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a second sc.w without a fresh lr.w still fails because the prior failed SC has already cleared the reservation",
        second_status != 0U &&
        ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_amo_sc_fails_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x10203040U;
    const uint32_t amo_delta = 0x01010101U;
    const uint32_t expected_after_amo = initial_value + amo_delta;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint32_t refreshed_value = 0x2468ace0U;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t amo_old_value = amoadd_w(addr, amo_delta);
    uint32_t after_amo = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address AMO is inserted between lr.w and sc.w in the refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening amoadd.w itself observes and updates the reserved word normally before the first sc.w retires",
        amo_old_value == initial_value &&
        after_amo == expected_after_amo
    );
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w fails after the intervening same-address AMO and preserves the intervening AMO result",
        sc_status != 0U &&
        ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a stale retry without a fresh lr.w fails because the prior failed SC has already cleared the reservation",
        second_status != 0U &&
        ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refresh_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the stale retry fails, a fresh lr.w/sc.w pair re-establishes the reservation and succeeds",
        refreshed_reserved == expected_after_amo &&
        refresh_status == 0U &&
        ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_word_amo_before_word_sc_fails_preserves_intervening_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t amo_old_value = amoadd_w(upper_word_addr, amo_delta);
    uint64_t after_amo = read64(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    TEST_ASSERT("lr.w captures the original low word before a same-doubleword upper-word amoadd.w is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the intervening upper-word amoadd.w itself observes the old high word and updates only the non-overlapping upper 32-bit half before the low-word sc.w retires",
        excpt.triggered == false &&
        amo_old_value == (uint32_t)(initial_value >> 32) &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("a same-doubleword but non-overlapping upper-word amoadd.w invalidates the low-word reservation, so the following sc.w fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_word_amo_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    uint32_t amo_old_value = amoadd_w(addr, amo_delta);
    uint64_t after_amo = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a same-doubleword low-word amoadd.w is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening low-word amoadd.w itself observes the old low word and updates only the non-overlapping low 32-bit half before the upper-word sc.w retires",
        excpt.triggered == false &&
        amo_old_value == (uint32_t)initial_value &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("on Spike, a same-doubleword but non-overlapping low-word amoadd.w still does not break the upper-word reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_word_amo_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
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
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(upper_word_addr);
    uint32_t amo_old_value = amoadd_w(upper_word_addr, amo_delta);
    uint64_t after_amo = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before an overlapping upper-word amoadd.w is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening upper-word amoadd.w itself observes the old upper word and updates only the reserved high 32-bit half before sc.w retires",
        excpt.triggered == false &&
        amo_old_value == (uint32_t)(initial_value >> 32) &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("on Spike, an overlapping upper-word amoadd.w still does not break the reservation and the following upper-word sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_amo_before_low_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t amo_addr = ai_lr_sc_addr(0);
    const uintptr_t sc_addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)(uint32_t)(expected_after_amo >> 32) << 32) |
        (uint64_t)committed_value;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(sc_addr);
    uint64_t amo_old_value = amoadd_d(amo_addr, amo_delta);
    uint64_t after_amo = read64(amo_addr);
    uint32_t sc_status = sc_w(sc_addr, committed_value);
    TEST_ASSERT("lr.w captures the original low word before a same-start amoadd.d is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the intervening same-start amoadd.d itself observes the old doubleword and updates both 32-bit halves before sc.w retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("on Spike, a same-start amoadd.d still does not break the reservation and the following low-word sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_amo_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t amo_addr = ai_lr_sc_addr(0);
    const uintptr_t sc_addr = ai_lr_sc_addr(1);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t amo_delta = 0x0101010101010101ULL;
    const uint64_t expected_after_amo = initial_value + amo_delta;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)committed_value << 32) |
        (uint64_t)(uint32_t)expected_after_amo;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(sc_addr);
    uint64_t amo_old_value = amoadd_d(amo_addr, amo_delta);
    uint64_t after_amo = read64(amo_addr);
    uint32_t sc_status = sc_w(sc_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before an overlapping amoadd.d is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening overlapping amoadd.d itself observes the old doubleword and updates both 32-bit halves before the upper-word sc.w retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("on Spike, an overlapping amoadd.d still does not break the reservation and the following upper-word sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_amo_sc_fails_preserves_amo_corner()
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
    TEST_ASSERT("lr.d captures the original doubleword before a same-address amoadd.d is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-address amoadd.d itself observes and updates the reserved doubleword normally before sc.d retires",
        excpt.triggered == false &&
        amo_old_value == initial_value &&
        after_amo == expected_after_amo
    );
    TEST_ASSERT("a same-address amoadd.d invalidates the reservation, so the following sc.d fails and preserves the intervening update",
        excpt.triggered == false &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_after_amo
    );

    TEST_END();
}
