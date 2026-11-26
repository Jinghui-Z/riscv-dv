#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_same_address_intervening_low_word_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint32_t observed = read32(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a same-doubleword low-word load is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening low-word load still observes the non-overlapping low 32-bit half of the original doubleword while the upper-word reservation remains live",
        excpt.triggered == false &&
        observed == (uint32_t)initial_value
    );
    TEST_ASSERT("on Spike, a same-doubleword but non-overlapping low-word load still does not break the upper-word reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_word_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint32_t observed = read32(upper_word_addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before an overlapping upper-word load is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening upper-word load still observes the reserved high 32-bit half of the original doubleword while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, an overlapping upper-word load still does not break the reservation and the following upper-word sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_load_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t observed = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-address doubleword load is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("a same-address doubleword load still observes the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == initial_value
    );
    TEST_ASSERT("on Spike, an intervening same-address doubleword load does not break the reservation and the following sc.d still succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_load_success_consumes_reservation_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t observed = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-address doubleword load is inserted between lr.d and sc.d in the reservation-consumption corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("a same-address doubleword load still observes the original payload while the reservation remains live before the first sc.d retires",
        excpt.triggered == false &&
        observed == initial_value
    );
    TEST_ASSERT("on Spike, the first sc.d still succeeds after the intervening same-address doubleword load",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful sc.d, a second sc.d without a fresh lr.d still fails because the successful reservation has already been consumed",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_load_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refreshed_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t observed = read64(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-address doubleword load is inserted between lr.d and sc.d in the refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("a same-address doubleword load still observes the original payload while the reservation remains live before the first sc.d retires",
        excpt.triggered == false &&
        observed == initial_value
    );
    TEST_ASSERT("on Spike, the first sc.d still succeeds after the intervening same-address doubleword load",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful sc.d, a stale retry without a fresh lr.d fails because the successful reservation has already been consumed",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refresh_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after the stale retry fails, a fresh lr.d/sc.d pair re-establishes the reservation and succeeds",
        excpt.triggered == false &&
        refreshed_reserved == committed_value &&
        refresh_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_word_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint32_t observed = read32(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-start low-word load is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-start low-word load still observes the low 32-bit half of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint32_t)initial_value
    );
    TEST_ASSERT("on Spike, a same-start low-word load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_word_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint32_t observed = read32(upper_word_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before an upper-word load is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening upper-word load still observes the high 32-bit half of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, an upper-word load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_halfword_at_byte0_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint16_t observed = read16(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-start halfword load at byte offset zero is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-start halfword load still observes the low-edge 16-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint16_t)initial_value
    );
    TEST_ASSERT("on Spike, a same-start halfword load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_halfword_at_byte2_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t halfword_addr = addr + 2U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint16_t observed = read16(halfword_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a halfword load at byte offset two is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening halfword load at byte offset two still observes the interior 16-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint16_t)(initial_value >> 16)
    );
    TEST_ASSERT("on Spike, an interior halfword load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_halfword_at_byte4_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t halfword_addr = addr + 4U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint16_t observed = read16(halfword_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a halfword load at byte offset four is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening halfword load at byte offset four still observes the high-word-edge 16-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint16_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, a high-word-edge halfword load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_halfword_at_byte6_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t halfword_addr = addr + 6U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint16_t observed = read16(halfword_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a halfword load at byte offset six is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening halfword load at byte offset six still observes the upper-edge 16-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint16_t)(initial_value >> 48)
    );
    TEST_ASSERT("on Spike, an upper-edge halfword load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_byte_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint8_t observed = read8(addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a same-start byte load at byte offset zero is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening same-start byte load still observes the low-edge 8-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)initial_value
    );
    TEST_ASSERT("on Spike, a same-start byte load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset1_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 1U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte load at offset one is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte load at offset one still observes the low-word interior 8-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 8)
    );
    TEST_ASSERT("on Spike, a byte load at offset one still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset2_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 2U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte load at offset two is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte load at offset two still observes another low-word interior 8-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 16)
    );
    TEST_ASSERT("on Spike, a byte load at offset two still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset3_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 3U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte load at offset three is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte load at offset three still observes the low-word-edge 8-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 24)
    );
    TEST_ASSERT("on Spike, a low-word-edge byte load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset4_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 4U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte load at offset four is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte load at offset four still observes the high-word-edge 8-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, a high-word-edge byte load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset5_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 5U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte load at offset five is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte load at offset five still observes the high-word interior 8-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 40)
    );
    TEST_ASSERT("on Spike, a byte load at offset five still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset6_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 6U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a byte load at offset six is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening byte load at offset six still observes the upper-edge interior 8-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 48)
    );
    TEST_ASSERT("on Spike, a byte load at offset six still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_tail_byte_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + sizeof(uint64_t) - 1U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t committed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("lr.d captures the original doubleword before a tail-byte load at byte offset seven is inserted between lr.d and sc.d",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the intervening tail-byte load still observes the top-edge 8-bit slice of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 56)
    );
    TEST_ASSERT("on Spike, a tail-byte load still does not break the reservation and the following sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_load_before_low_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t committed_value = 0xa5a55a5aU;
    const uint64_t expected_final =
        ((uint64_t)(uint32_t)(initial_value >> 32) << 32) |
        (uint64_t)committed_value;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint64_t observed = read64(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    TEST_ASSERT("lr.w captures the original low word before a same-start doubleword load is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the intervening same-start doubleword load still observes both 32-bit halves of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == initial_value
    );
    TEST_ASSERT("on Spike, a same-start doubleword load still does not break the reservation and the following low-word sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_doubleword_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t observed = read64(addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before an overlapping doubleword load is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening overlapping doubleword load still observes both 32-bit halves of the original payload while the reservation remains live",
        excpt.triggered == false &&
        observed == initial_value
    );
    TEST_ASSERT("on Spike, an overlapping doubleword load still does not break the reservation and the following upper-word sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_load_still_succeeds()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word load is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word load through the second virtual alias observes the shared payload while the reservation remains live",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on the same-setidx same-paddr alias path, the intervening load still does not break the reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word finally contains the successful sc.w payload after the alias-load corner sequence",
        read32(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_load_diff_setidx_fails()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias load is issued",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias load still observes the shared payload before the later sc.w retires",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("for the different-setidx same-paddr alias path, the later same-alias sc.w is expected to fail and leave memory unchanged",
        sc_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-load sequence, the shared backing word remains at the original value",
        read32(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_load_diff_setidx_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t first_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias load is issued in the failed-sc-consumption corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias load still observes the shared payload before the first failing sc.w retires in the failed-sc-consumption corner",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w still fails after the different-setidx alias load and leaves memory unchanged",
        first_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(base_vaddr, 0xcafebabeU);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, retrying sc.w on the original alias without a fresh lr.w still fails because the failed sc.w has already consumed the reservation",
        second_status != 0U &&
        read32(alias_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-load failed-sc-consumption corner, the shared backing word remains unchanged because neither sc.w commits",
        read32(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_load_diff_setidx_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t failed_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias load is issued in the original-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias load still observes the shared payload before the first failing sc.w retires in the original-refresh corner",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w fails and leaves the shared word unchanged on the different-setidx alias-load path",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, a fresh lr.w on the original alias still captures the unchanged shared word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original alias, the following sc.w succeeds and both aliases observe the refreshed payload",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-load failure-and-original-refresh corner, the shared backing word contains only the refreshed original-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_load_diff_setidx_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t failed_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the base alias still captures the original word before a different-setidx same-paddr alias load is issued in the switched-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the different-setidx alias load still observes the shared payload before the first failing sc.w retires in the switched-refresh corner",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-alias sc.w fails and leaves the shared word unchanged on the different-setidx alias-load path",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed sc.w, a fresh lr.w on the switched alias still captures the unchanged shared word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the switched alias, the following sc.w succeeds and both aliases observe the refreshed payload",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-load failure-and-switched-refresh corner, the shared backing word contains only the refreshed switched-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_load_success_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word load is issued through the second alias in the reservation-consumption corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word load through the second virtual alias observes the shared payload while the reservation remains live before the first sc.w retires",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first same-alias sc.w still succeeds after the intervening alias load",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0xcafebabeU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a second sc.w through the other alias without a fresh lr.w still fails because the successful shared reservation has already been consumed",
        second_status != 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing word keeps the first successful sc.w payload after the alias-load reservation-consumption corner",
        read32(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_load_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word load is issued through the second alias in the original-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word load through the second virtual alias observes the shared payload while the reservation remains live before the first sc.w retires",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first same-alias sc.w still succeeds after the intervening alias load",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0xcafebabeU);
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
    TEST_ASSERT("the shared backing word finally keeps the refreshed original-alias sc.w payload after the alias-load recovery corner",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_load_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-physical-word load is issued through the second alias in the switched-refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-physical-word load through the second virtual alias observes the shared payload while the reservation remains live before the first sc.w retires",
        alias_observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first same-alias sc.w still succeeds after the intervening alias load",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0xcafebabeU);
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
    TEST_ASSERT("the shared backing word finally keeps the refreshed switched-alias sc.w payload after the alias-load recovery corner",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_halfword_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x89abcdefU;
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
    uint16_t alias_observed = read16(alias_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-start low-halfword load is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-start low-halfword load through the second alias still observes the low 16-bit slice of the shared payload while the reservation remains live",
        alias_observed == (uint16_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-start low-halfword load through a different alias still does not break the reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start low-halfword load corner, the shared backing word finally keeps the successful sc.w payload",
        read32(paddr) == committed_value
    );

    TEST_END();
}
