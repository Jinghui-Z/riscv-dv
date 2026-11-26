#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_halfword_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_halfword_vaddr = alias_vaddr + sizeof(uint16_t);
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
    uint16_t alias_observed = read16(alias_upper_halfword_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before an upper-halfword load is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the upper-halfword load through the second alias still observes the high 16-bit slice of the shared payload while the reservation remains live",
        alias_observed == (uint16_t)(initial_value >> 16)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, an upper-halfword load through a different alias still does not break the reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-halfword load corner, the shared backing word finally keeps the successful sc.w payload",
        read32(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_load_before_word_sc_still_succeeds_corner()
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
    uint8_t alias_observed = read8(alias_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a same-start low-byte load is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-start low-byte load through the second alias still observes the low 8-bit slice of the shared payload while the reservation remains live",
        alias_observed == (uint8_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-start low-byte load through a different alias still does not break the reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start low-byte load corner, the shared backing word finally keeps the successful sc.w payload",
        read32(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 1U;
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a byte load at offset one is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the byte load at offset one through the second alias still observes the first interior 8-bit slice of the shared payload while the reservation remains live",
        alias_observed == (uint8_t)(initial_value >> 8)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a byte load at offset one through a different alias still does not break the reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-one byte load corner, the shared backing word finally keeps the successful sc.w payload",
        read32(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 2U;
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a byte load at offset two is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the byte load at offset two through the second alias still observes the second interior 8-bit slice of the shared payload while the reservation remains live",
        alias_observed == (uint8_t)(initial_value >> 16)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a byte load at offset two through a different alias still does not break the reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-two byte load corner, the shared backing word finally keeps the successful sc.w payload",
        read32(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_tail_byte_vaddr = alias_vaddr + sizeof(uint32_t) - 1U;
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
    uint8_t alias_observed = read8(alias_tail_byte_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original word before a tail-byte load is issued through the second alias",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the tail-byte load through the second alias still observes the high-edge 8-bit slice of the shared payload while the reservation remains live",
        alias_observed == (uint8_t)(initial_value >> 24)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a tail-byte load through a different alias still does not break the reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read32(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias tail-byte load corner, the shared backing word finally keeps the successful sc.w payload",
        read32(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t committed_value = 0x13579bdfU;
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
    uint32_t alias_observed = read32(alias_upper_word_vaddr);
    uint32_t sc_status = sc_w(base_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original low word before a same-doubleword upper-word load is issued through the second alias",
        reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-doubleword upper-word load through the second alias still observes the non-overlapping shared high 32-bit half while the low-word reservation remains live",
        alias_observed == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-doubleword but non-overlapping upper-word load through a different alias still does not break the low-word reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-word load before low-word sc.w corner, the shared backing doubleword finally keeps the untouched upper word and the successful low-word sc.w payload",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_halfword_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
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
    uint16_t alias_observed = read16(alias_upper_word_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a same-start low-halfword load is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the same-start low-halfword load through the second alias still observes the low 16-bit slice of the shared upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint16_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, a same-start low-halfword load through a different alias inside the reserved upper word still does not break the reservation and the later same-alias sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start low-halfword upper-word load corner, the shared backing doubleword finally keeps the successful sc.w payload in its high 32-bit half",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_halfword_load_before_upper_word_sc_still_succeeds_corner()
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
    uint16_t alias_observed = read16(alias_upper_halfword_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before an upper-halfword load is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the upper-halfword load through the second alias still observes the high 16-bit slice of the shared upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint16_t)(initial_value >> 48)
    );
    TEST_ASSERT("on Spike, an upper-halfword load through a different alias inside the reserved upper word still does not break the reservation and the later same-alias sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-halfword upper-word load corner, the shared backing doubleword finally keeps the successful sc.w payload in its high 32-bit half",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
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
    uint8_t alias_observed = read8(alias_upper_word_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a same-start low-byte load is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the same-start low-byte load through the second alias still observes the low 8-bit slice of the shared upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, a same-start low-byte load through a different alias inside the reserved upper word still does not break the reservation and the later same-alias sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start low-byte upper-word load corner, the shared backing doubleword finally keeps the successful sc.w payload in its high 32-bit half",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_load_before_upper_word_sc_still_succeeds_corner()
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a byte load at offset one is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the byte load at offset one through the second alias still observes the first interior 8-bit slice of the shared upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 40)
    );
    TEST_ASSERT("on Spike, a byte load at offset one through a different alias inside the reserved upper word still does not break the reservation and the later same-alias sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-one upper-word byte-load corner, the shared backing doubleword finally keeps the successful sc.w payload in its high 32-bit half",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_load_before_upper_word_sc_still_succeeds_corner()
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
    uint8_t alias_observed = read8(alias_byte_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a byte load at offset two is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the byte load at offset two through the second alias still observes the second interior 8-bit slice of the shared upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 48)
    );
    TEST_ASSERT("on Spike, a byte load at offset two through a different alias inside the reserved upper word still does not break the reservation and the later same-alias sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-two upper-word byte-load corner, the shared backing doubleword finally keeps the successful sc.w payload in its high 32-bit half",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_load_before_upper_word_sc_still_succeeds_corner()
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
    uint8_t alias_observed = read8(alias_tail_byte_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    TEST_ASSERT("lr.w through the first virtual alias captures the original upper word before a tail-byte load is issued through the second alias",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the tail-byte load through the second alias still observes the high-edge 8-bit slice of the shared upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 56)
    );
    TEST_ASSERT("on Spike, a tail-byte load through a different alias inside the reserved upper word still does not break the reservation and the later same-alias sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias tail-byte upper-word load corner, the shared backing doubleword finally keeps the successful sc.w payload in its high 32-bit half",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_word_load_before_upper_word_sc_still_succeeds_corner()
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original upper word before a same-doubleword low-word load is issued through the second alias",
        reserved == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("the same-doubleword low-word load through the second alias still observes the non-overlapping shared low 32-bit half while the upper-word reservation remains live",
        alias_observed == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-doubleword but non-overlapping low-word load through a different alias still does not break the upper-word reservation and the later same-alias sc.w succeeds",
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias low-word upper-window load corner, the shared backing doubleword finally keeps the successful upper-word sc.w payload while preserving the original low word",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t base_upper_word_vaddr = base_vaddr + sizeof(uint32_t);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
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
    uint32_t alias_observed = read32(alias_upper_word_vaddr);
    uint32_t sc_status = sc_w(base_upper_word_vaddr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the original upper word before an overlapping upper-word load is issued through the second alias",
        reserved == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("the overlapping upper-word load through the second alias still observes the reserved shared high 32-bit half while the reservation remains live",
        alias_observed == (uint32_t)(initial_value >> 32)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, an overlapping upper-word load through a different alias still does not break the reservation and the later same-alias upper-word sc.w succeeds",
        sc_status == 0U &&
        read64(base_vaddr) == expected_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-word upper-window load corner, the shared backing doubleword finally keeps the successful upper-word sc.w payload while preserving the original low word",
        read64(paddr) == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
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
    TEST_ASSERT("the success-family alias pair intentionally keeps the virtual set index identical while remapping to the same physical backing page",
        ai_lr_sc_setidx(base_vaddr) == ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word load is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word doubleword load through the second virtual alias observes the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == initial_value
    );
    TEST_ASSERT("on the same-setidx same-paddr alias path, the intervening doubleword load still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally contains the successful sc.d payload after the alias-load corner sequence",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_diff_setidx_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
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
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias load is issued",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias doubleword load still observes the shared payload before the later sc.d retires",
        excpt.triggered == false &&
        alias_observed == initial_value
    );
    TEST_ASSERT("for the different-setidx same-paddr alias path, the later same-alias sc.d is expected to fail and leave memory unchanged",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias doubleword-load sequence, the shared backing doubleword remains at the original value",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_diff_setidx_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
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
    TEST_ASSERT("the contrasting alias pair intentionally uses different virtual set indices while remapping to the same physical backing doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias load is issued in the failed-sc-consumption corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias doubleword load still observes the shared payload before the first failing sc.d retires in the failed-sc-consumption corner",
        excpt.triggered == false &&
        alias_observed == initial_value
    );
    TEST_ASSERT("the first same-alias sc.d still fails after the different-setidx alias load and leaves memory unchanged",
        excpt.triggered == false &&
        first_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that failed sc.d, retrying sc.d on the original alias without a fresh lr.d still fails because the failed sc.d has already consumed the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(alias_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-doubleword-load failed-sc-consumption corner, the shared backing doubleword remains unchanged because neither sc.d commits",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_diff_setidx_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t failed_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias load is issued in the original-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias doubleword load still observes the shared payload before the first failing sc.d retires in the original-refresh corner",
        excpt.triggered == false &&
        alias_observed == initial_value
    );
    TEST_ASSERT("the first same-alias sc.d fails and leaves the shared doubleword unchanged on the different-setidx alias-load path",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that failed sc.d, a fresh lr.d on the original alias still captures the unchanged shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing the reservation on the original alias, the following sc.d succeeds and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-doubleword-load failure-and-original-refresh corner, the shared backing doubleword contains only the refreshed original-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_diff_setidx_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t failed_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the base alias still captures the original doubleword before a different-setidx same-paddr alias load is issued in the switched-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the different-setidx alias doubleword load still observes the shared payload before the first failing sc.d retires in the switched-refresh corner",
        excpt.triggered == false &&
        alias_observed == initial_value
    );
    TEST_ASSERT("the first same-alias sc.d fails and leaves the shared doubleword unchanged on the different-setidx alias-load path",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that failed sc.d, a fresh lr.d on the switched alias still captures the unchanged shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing the reservation on the switched alias, the following sc.d succeeds and both aliases observe the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the different-setidx alias-doubleword-load failure-and-switched-refresh corner, the shared backing doubleword contains only the refreshed switched-alias payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_success_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
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
    TEST_ASSERT("the success-family alias pair intentionally keeps the virtual set index identical while remapping to the same physical backing page",
        ai_lr_sc_setidx(base_vaddr) == ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word load is issued through the second alias in the reservation-consumption corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word doubleword load through the second virtual alias observes the shared payload while the reservation remains live before the first sc.d retires",
        excpt.triggered == false &&
        alias_observed == initial_value
    );
    TEST_ASSERT("on Spike, the first same-alias sc.d still succeeds after the intervening alias doubleword load",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful sc.d, a second sc.d through the other alias without a fresh lr.d still fails because the successful shared reservation has already been consumed",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword keeps the first successful sc.d payload after the alias-doubleword-load reservation-consumption corner",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word load is issued through the second alias in the original-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word doubleword load through the second virtual alias observes the shared payload while the reservation remains live before the first sc.d retires",
        excpt.triggered == false &&
        alias_observed == initial_value
    );
    TEST_ASSERT("on Spike, the first same-alias sc.d still succeeds after the intervening alias doubleword load",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful sc.d, a stale sc.d through the other alias fails because the successful shared reservation has already been consumed",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refresh_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after the stale alias retry fails, a fresh lr.d/sc.d pair on the original alias re-establishes the shared reservation and succeeds",
        excpt.triggered == false &&
        refreshed_reserved == committed_value &&
        refresh_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally keeps the refreshed original-alias sc.d payload after the alias-doubleword-load recovery corner",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t alias_observed = read64(alias_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-physical-word load is issued through the second alias in the switched-refresh corner",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-physical-word doubleword load through the second virtual alias observes the shared payload while the reservation remains live before the first sc.d retires",
        excpt.triggered == false &&
        alias_observed == initial_value
    );
    TEST_ASSERT("on Spike, the first same-alias sc.d still succeeds after the intervening alias doubleword load",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that successful sc.d, a stale sc.d through the other alias fails because the successful shared reservation has already been consumed",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refresh_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after the stale alias retry fails, a fresh lr.d/sc.d pair on the switched alias re-establishes the shared reservation and succeeds",
        excpt.triggered == false &&
        refreshed_reserved == committed_value &&
        refresh_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the shared backing doubleword finally keeps the refreshed switched-alias sc.d payload after the alias-doubleword-load recovery corner",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_word_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
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
    uint32_t alias_observed = read32(alias_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-start low-word load is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-start low-word load through the second alias still observes the low 32-bit half of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint32_t)initial_value
    );
    TEST_ASSERT("on Spike, a same-start low-word load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start low-word load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_upper_word_vaddr = alias_vaddr + sizeof(uint32_t);
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
    uint32_t alias_observed = read32(alias_upper_word_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before an upper-word load is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the upper-word load through the second alias still observes the high 32-bit half of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, an upper-word load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-word load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte0_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
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
    uint16_t alias_observed = read16(alias_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-start halfword load at byte offset zero is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-start halfword load through the second alias still observes the low-edge 16-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint16_t)initial_value
    );
    TEST_ASSERT("on Spike, a same-start halfword load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start halfword load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte2_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_halfword_vaddr = alias_vaddr + 2U;
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
    uint16_t alias_observed = read16(alias_halfword_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a halfword load at byte offset two is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the halfword load at byte offset two through the second alias still observes the interior 16-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint16_t)(initial_value >> 16)
    );
    TEST_ASSERT("on Spike, an interior halfword load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias interior-halfword load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte4_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_halfword_vaddr = alias_vaddr + 4U;
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
    uint16_t alias_observed = read16(alias_halfword_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a halfword load at byte offset four is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the halfword load at byte offset four through the second alias still observes the high-word-edge 16-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint16_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, a high-word-edge halfword load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-four halfword load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte6_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_halfword_vaddr = alias_vaddr + 6U;
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
    uint16_t alias_observed = read16(alias_halfword_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a halfword load at byte offset six is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the halfword load at byte offset six through the second alias still observes the upper-edge 16-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint16_t)(initial_value >> 48)
    );
    TEST_ASSERT("on Spike, an upper-edge halfword load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias upper-edge halfword load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
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
    uint8_t alias_observed = read8(alias_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(base_vaddr, committed_value);
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a same-start byte load at byte offset zero is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the same-start byte load through the second alias still observes the low-edge 8-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)initial_value
    );
    TEST_ASSERT("on Spike, a same-start byte load through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias same-start byte load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_load_before_doubleword_sc_still_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_SAME_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t alias_byte_vaddr = alias_vaddr + 1U;
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
    TEST_ASSERT("lr.d through the first virtual alias captures the original doubleword before a byte load at offset one is issued through the second alias",
        excpt.triggered == false &&
        reserved == initial_value
    );
    TEST_ASSERT("the byte load at offset one through the second alias still observes the low-word interior 8-bit slice of the shared payload while the reservation remains live",
        excpt.triggered == false &&
        alias_observed == (uint8_t)(initial_value >> 8)
    );
    TEST_ASSERT("on Spike, a byte load at offset one through a different alias still does not break the reservation and the later same-alias sc.d succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        read64(base_vaddr) == committed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias offset-one byte load corner, the shared backing doubleword finally keeps the successful sc.d payload",
        read64(paddr) == committed_value
    );

    TEST_END();
}
