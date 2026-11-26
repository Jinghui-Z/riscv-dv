#include "ai_lr_sc_common.h"

bool ai_micro_lr_sc_gap_over_64_cycles_refresh_succeeds()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x1234fedcU;
    const uint32_t refreshed_value = 0x0badc0deU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w first captures the original word before the microarchitectural timeout-recovery probe begins",
        reserved == initial_value
    );

    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("the first microarchitectural sc.w fails once the reservation is held for more than sixty-four cycles",
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        stale_status != 0U &&
        ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the timed-out sc.w, a fresh lr.w still captures the original word and recreates the reservation",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after recreating the reservation, the next sc.w succeeds and updates memory",
        refreshed_status == 0U &&
        ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_refresh_adjacent_word_preserved_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t adjacent_addr = ai_lr_sc_addr(1);
    const uint32_t initial_value = 0x1234fedcU;
    const uint32_t adjacent_value = 0x55667788U;
    const uint32_t refreshed_value = 0x0badc0deU;

    ai_lr_sc_prepare_words(initial_value, adjacent_value);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w first captures the original target word before the timeout-recovery side-effect corner begins",
        reserved == initial_value
    );

    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("the stale sc.w still fails after the timeout gap and leaves both the target word and its adjacent word unchanged",
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        stale_status != 0U &&
        read32(addr) == initial_value &&
        read32(adjacent_addr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after the timed-out sc.w, a fresh lr.w still captures the original target word before recovery",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation, the succeeding sc.w updates only the target word and preserves the adjacent word",
        refreshed_status == 0U &&
        read32(addr) == refreshed_value &&
        read32(adjacent_addr) == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the timeout-recovery side-effect corner, backing memory contains the refreshed target word while the adjacent word stays untouched",
        ai_lr_sc_words[0] == refreshed_value &&
        ai_lr_sc_words[1] == adjacent_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_doubleword_timeout_fails_spike_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1234fedc5678abcdULL;
    const uint64_t committed_value = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, committed_value);
    TEST_ASSERT("when more than sixty-four cycles separate lr.d and sc.d, sc.d fails and leaves memory unchanged",
        excpt.triggered == false &&
        reserved == initial_value &&
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_doubleword_timeout_then_refresh_succeeds_spike_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1234fedc5678abcdULL;
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
    TEST_ASSERT("when more than sixty-four cycles separate lr.d and sc.d, delayed sc.d fails and leaves memory unchanged",
        excpt.triggered == false &&
        reserved == initial_value &&
        cycle_gap > AI_LR_SC_TIMEOUT_CYCLES &&
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after that timeout failure, a fresh microarchitectural lr.d still captures the original doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after recreating the reservation with a fresh lr.d, the next microarchitectural sc.d succeeds and overwrites the original payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_doubleword_timeout_clears_reservation_spike_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x1234fedc5678abcdULL;
    const uint64_t first_committed = 0xa5a55a5af0f00f0fULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(addr);
    uint64_t cycle_gap = ai_lr_sc_wait_cycle_gap_gt(AI_LR_SC_TIMEOUT_CYCLES);
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, first_committed);
    TEST_ASSERT("when more than sixty-four cycles separate lr.d and sc.d, delayed sc.d fails and leaves memory unchanged",
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

bool ai_micro_lr_sc_gap_over_64_cycles_doubleword_alias_timeout_then_alias_retry_fails_spike_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1234fedc5678abcdULL;
    const uint64_t first_committed = 0xa5a55a5af0f00f0fULL;

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
    ai_lr_sc_force_long_gap_without_counters();
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, first_committed);
    uint64_t after_first = read64(alias_vaddr);
    TEST_ASSERT("on current Spike, once a same-paddr alias lr.d is held across a long HS delay, the first sc.d on the original alias already fails",
        excpt.triggered == false &&
        reserved == initial_value &&
        first_status != 0U &&
        after_first == initial_value,
        "triggered=%d reserved=0x%016llx first_status=0x%08x after_first=0x%016llx",
        excpt.triggered, reserved, first_status, after_first
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x0f0ff0f05a5aa5a5ULL);
    uint64_t after_second = read64(base_vaddr);
    TEST_ASSERT("without a fresh lr.d, retrying sc.d through the other alias after that timeout-style failure also fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        second_status != 0U &&
        after_second == initial_value,
        "triggered=%d second_status=0x%08x after_second=0x%016llx",
        excpt.triggered, second_status, after_second
    );

    goto_priv(PRIV_M);
    uint64_t final_backing = read64(paddr);
    TEST_ASSERT("after the long-gap alias timeout corner, the shared backing doubleword still keeps the initial payload because neither sc.d commits",
        final_backing == initial_value,
        "final_backing=0x%016llx", final_backing
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_doubleword_alias_timeout_then_refresh_succeeds_spike_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1234fedc5678abcdULL;
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
    ai_lr_sc_force_long_gap_without_counters();
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, 0xa5a55a5af0f00f0fULL);
    uint64_t after_first = read64(alias_vaddr);
    TEST_ASSERT("on current Spike, once a same-paddr alias lr.d is held across a long HS delay, the first sc.d on the original alias already fails",
        excpt.triggered == false &&
        reserved == initial_value &&
        first_status != 0U &&
        after_first == initial_value,
        "triggered=%d reserved=0x%016llx first_status=0x%08x after_first=0x%016llx",
        excpt.triggered, reserved, first_status, after_first
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x0f0ff0f05a5aa5a5ULL);
    uint64_t after_second = read64(base_vaddr);
    TEST_ASSERT("without a fresh lr.d, retrying sc.d through the other alias after that timeout-style failure also fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        second_status != 0U &&
        after_second == initial_value,
        "triggered=%d second_status=0x%08x after_second=0x%016llx",
        excpt.triggered, second_status, after_second
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after both delayed alias sc.d attempts fail, a fresh lr.d through the alias still captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing the shared reservation through the alias, the following sc.d succeeds and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    uint64_t final_backing = read64(paddr);
    TEST_ASSERT("after the long-gap alias timeout recovery corner, the shared backing doubleword contains only the refreshed alias sc.d payload",
        final_backing == refreshed_value,
        "final_backing=0x%016llx", final_backing
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_doubleword_alias_timeout_then_refresh_adjacent_doubleword_preserved_spike_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t base_adjacent_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t alias_adjacent_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1234fedc5678abcdULL;
    const uint64_t adjacent_value = 0x8899aabbccddeeffULL;
    const uint64_t refreshed_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(paddr + sizeof(uint64_t), adjacent_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    ai_lr_sc_force_long_gap_without_counters();
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, 0xa5a55a5af0f00f0fULL);
    TEST_ASSERT("on current Spike, the first delayed alias sc.d still fails and leaves both the target and adjacent doublewords unchanged before recovery",
        excpt.triggered == false &&
        reserved == initial_value &&
        first_status != 0U &&
        read64(alias_vaddr) == initial_value &&
        read64(alias_adjacent_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("without a fresh lr.d, retrying sc.d through the other alias also fails and still preserves the adjacent doubleword",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == initial_value &&
        read64(base_adjacent_vaddr) == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after both delayed alias sc.d attempts fail, a fresh lr.d through the alias still captures the original shared target doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing the shared reservation through the alias, the successful sc.d overwrites only the target doubleword and leaves the adjacent doubleword intact",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value &&
        read64(base_adjacent_vaddr) == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the long-gap alias doubleword recovery side-effect corner, backing memory keeps the refreshed target doubleword and the untouched adjacent doubleword",
        read64(paddr) == refreshed_value &&
        read64(paddr + sizeof(uint64_t)) == adjacent_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_doubleword_alias_timeout_then_original_refresh_succeeds_spike_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1234fedc5678abcdULL;
    const uint64_t refreshed_value = 0x8899aabbccddeeffULL;

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
    ai_lr_sc_force_long_gap_without_counters();
    uint32_t first_status = ai_lr_sc_sc_d_status(base_vaddr, 0xa5a55a5af0f00f0fULL);
    uint64_t after_first = read64(alias_vaddr);
    TEST_ASSERT("on current Spike, once a same-paddr alias lr.d is held across a long HS delay, the first sc.d on the original alias already fails",
        excpt.triggered == false &&
        reserved == initial_value &&
        first_status != 0U &&
        after_first == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(alias_vaddr, 0x0f0ff0f05a5aa5a5ULL);
    uint64_t after_second = read64(base_vaddr);
    TEST_ASSERT("without a fresh lr.d, retrying sc.d through the other alias after that timeout-style failure also fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        second_status != 0U &&
        after_second == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after both delayed alias sc.d attempts fail, a fresh lr.d through the original alias still captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing the shared reservation through the original alias, the following sc.d succeeds and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the long-gap alias timeout original-recovery corner, the shared backing doubleword contains only the refreshed original-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_then_alias_retry_fails_spike_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x1234fedcU;
    const uint32_t first_committed = 0xa5a55a5aU;

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
    ai_lr_sc_force_long_gap_without_counters();
    uint32_t first_status = sc_w(base_vaddr, first_committed);
    uint32_t after_first = read32(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("on current Spike, once a same-paddr alias lr.w is held across a long HS delay, the first sc.w on the original alias already fails",
        reserved == initial_value &&
        first_status != 0U &&
        after_first == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x0f0ff0f0U);
    uint32_t after_second = read32(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("without a fresh lr.w, retrying sc.w through the other alias after that timeout-style failure also fails and leaves the shared word unchanged",
        second_status != 0U &&
        after_second == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the long-gap alias word-timeout corner, the shared backing word still keeps the initial payload because neither sc.w commits",
        read32(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_adjacent_word_preserved_spike_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x1234fedcU;
    const uint32_t adjacent_value = 0x55667788U;
    const uint64_t initial_doubleword =
        ((uint64_t)adjacent_value << 32) | (uint64_t)initial_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_doubleword);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    ai_lr_sc_force_long_gap_without_counters();
    uint32_t first_status = sc_w(base_vaddr, 0xa5a55a5aU);
    uint64_t after_first = read64(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("on current Spike, after a long alias timeout gap the first sc.w fails and leaves both the target word and the adjacent shared word unchanged",
        reserved == initial_value &&
        first_status != 0U &&
        after_first == initial_doubleword
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x0f0ff0f0U);
    uint64_t after_second = read64(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("without a fresh lr.w, retrying sc.w through the other alias after that timeout-style failure still leaves the whole shared doubleword unchanged",
        second_status != 0U &&
        after_second == initial_doubleword
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias timeout side-effect corner, backing memory still keeps both the original target word and the untouched adjacent shared word",
        read64(paddr) == initial_doubleword
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_then_refresh_succeeds_spike_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x1234fedcU;
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
    ai_lr_sc_force_long_gap_without_counters();
    uint32_t first_status = sc_w(base_vaddr, 0xa5a55a5aU);
    uint32_t after_first = read32(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("on current Spike, once a same-paddr alias lr.w is held across a long HS delay, the first sc.w on the original alias already fails",
        reserved == initial_value &&
        first_status != 0U &&
        after_first == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x0f0ff0f0U);
    uint32_t after_second = read32(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("without a fresh lr.w, retrying sc.w through the other alias after that timeout-style failure also fails and leaves the shared word unchanged",
        second_status != 0U &&
        after_second == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after both delayed alias sc.w attempts fail, a fresh lr.w through the alias still captures the original shared word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the shared reservation through the alias, the following sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the long-gap alias word-timeout recovery corner, the shared backing word contains only the refreshed alias sc.w payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_then_refresh_adjacent_word_preserved_spike_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x1234fedcU;
    const uint32_t adjacent_value = 0x55667788U;
    const uint64_t initial_doubleword =
        ((uint64_t)adjacent_value << 32) | (uint64_t)initial_value;
    const uint32_t refreshed_value = 0x55aa33ccU;
    const uint64_t refreshed_doubleword =
        ((uint64_t)adjacent_value << 32) | (uint64_t)refreshed_value;

    goto_priv(PRIV_M);
    write64(paddr, initial_doubleword);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    ai_lr_sc_force_long_gap_without_counters();
    uint32_t first_status = sc_w(base_vaddr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("on current Spike, the first delayed alias sc.w still fails and leaves both the target word and adjacent word unchanged before recovery",
        reserved == initial_value &&
        first_status != 0U &&
        read64(alias_vaddr) == initial_doubleword
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x0f0ff0f0U);
    AI_ASSERT_NO_EXCEPT_AND("without a fresh lr.w, retrying sc.w through the other alias also fails and still preserves the adjacent shared word",
        second_status != 0U &&
        read64(base_vaddr) == initial_doubleword
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after both delayed alias sc.w attempts fail, a fresh lr.w through the alias still captures the original shared target word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the shared reservation through the alias, the successful sc.w updates only the target word and preserves the adjacent word",
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_doubleword
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the long-gap alias word recovery side-effect corner, backing memory keeps the refreshed target word and the untouched adjacent word",
        read64(paddr) == refreshed_doubleword
    );

    TEST_END();
}

bool ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_then_original_refresh_succeeds_spike_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x1234fedcU;
    const uint32_t refreshed_value = 0x8899aabbU;

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
    ai_lr_sc_force_long_gap_without_counters();
    uint32_t first_status = sc_w(base_vaddr, 0xa5a55a5aU);
    uint32_t after_first = read32(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("on current Spike, once a same-paddr alias lr.w is held across a long HS delay, the first sc.w on the original alias already fails",
        reserved == initial_value &&
        first_status != 0U &&
        after_first == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x0f0ff0f0U);
    uint32_t after_second = read32(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("without a fresh lr.w, retrying sc.w through the other alias after that timeout-style failure also fails and leaves the shared word unchanged",
        second_status != 0U &&
        after_second == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after both delayed alias sc.w attempts fail, a fresh lr.w through the original alias still captures the original shared word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the shared reservation through the original alias, the following sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the long-gap alias word-timeout original-recovery corner, the shared backing word contains only the refreshed original-alias sc.w payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_load_still_succeeds()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t observed = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address load is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-address ordinary load still observes the original payload while the reservation remains live",
        observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("an intervening same-address load does not break the reservation and the following sc.w still succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_load_success_consumes_reservation_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t observed = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address load is inserted between lr.w and sc.w in the reservation-consumption corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-address ordinary load still observes the original payload while the reservation remains live before the first sc.w retires",
        observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w still succeeds after the intervening same-address load",
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

bool ai_micro_lr_sc_same_address_intervening_load_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;
    const uint32_t refreshed_value = 0x2468ace0U;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t observed = read32(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-address load is inserted between lr.w and sc.w in the refresh corner",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("a same-address ordinary load still observes the original payload while the reservation remains live before the first sc.w retires",
        observed == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w still succeeds after the intervening same-address load",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0xcafebabeU);
    AI_ASSERT_NO_EXCEPT_AND("after that successful sc.w, a stale retry without a fresh lr.w fails because the successful reservation has already been consumed",
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

bool ai_micro_lr_sc_same_address_intervening_low_halfword_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint16_t observed = read16(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-start low-halfword load is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening same-start low-halfword load still observes the low 16-bit slice of the original payload while the reservation remains live",
        observed == (uint16_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-start low-halfword load still does not break the reservation and the following sc.w succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_halfword_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_halfword_addr = addr + sizeof(uint16_t);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint16_t observed = read16(upper_halfword_addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before an upper-halfword load is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening upper-halfword load still observes the high 16-bit slice of the original payload while the reservation remains live",
        observed == (uint16_t)(initial_value >> 16)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, an upper-halfword load still does not break the reservation and the following sc.w succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_byte_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint8_t observed = read8(addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a same-start low-byte load is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening same-start low-byte load still observes the low 8-bit slice of the original payload while the reservation remains live",
        observed == (uint8_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a same-start low-byte load still does not break the reservation and the following sc.w succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset1_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 1U;
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a byte load at offset one is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening byte load at offset one still observes the first interior 8-bit slice of the original payload while the reservation remains live",
        observed == (uint8_t)(initial_value >> 8)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a byte load at offset one still does not break the reservation and the following sc.w succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset2_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t byte_addr = addr + 2U;
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a byte load at offset two is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening byte load at offset two still observes the second interior 8-bit slice of the original payload while the reservation remains live",
        observed == (uint8_t)(initial_value >> 16)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a byte load at offset two still does not break the reservation and the following sc.w succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_tail_byte_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t tail_byte_addr = addr + sizeof(uint32_t) - 1U;
    const uint32_t initial_value = 0x89abcdefU;
    const uint32_t committed_value = 0x13579bdfU;

    ai_lr_sc_prepare_words(initial_value, 0U);

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint8_t observed = read8(tail_byte_addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the original word before a tail-byte load is inserted between lr.w and sc.w",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the intervening tail-byte load still observes the high-edge 8-bit slice of the original payload while the reservation remains live",
        observed == (uint8_t)(initial_value >> 24)
    );
    AI_ASSERT_NO_EXCEPT_AND("on Spike, a tail-byte load still does not break the reservation and the following sc.w succeeds",
        sc_status == 0U &&
        ai_lr_sc_words[0] == committed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_word_load_before_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint32_t committed_value = 0x13579bdfU;
    const uint64_t expected_final =
        ((uint64_t)(uint32_t)(initial_value >> 32) << 32) |
        (uint64_t)committed_value;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(addr);
    uint32_t observed = read32(upper_word_addr);
    uint32_t sc_status = sc_w(addr, committed_value);
    TEST_ASSERT("lr.w captures the original low word before a same-doubleword upper-word load is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)initial_value
    );
    TEST_ASSERT("the intervening upper-word load still observes the non-overlapping high 32-bit half of the original doubleword while the low-word reservation remains live",
        excpt.triggered == false &&
        observed == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, a same-doubleword but non-overlapping upper-word load still does not break the low-word reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_halfword_load_before_upper_word_sc_still_succeeds_corner()
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
    uint16_t observed = read16(upper_word_addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a same-start low-halfword load is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening same-start low-halfword load still observes the low 16-bit slice of the original upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint16_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, a same-start low-halfword load inside the reserved upper word still does not break the reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_upper_halfword_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uintptr_t upper_halfword_addr = upper_word_addr + sizeof(uint16_t);
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
    uint16_t observed = read16(upper_halfword_addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before an upper-halfword load is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening upper-halfword load still observes the high 16-bit slice of the original upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint16_t)(initial_value >> 48)
    );
    TEST_ASSERT("on Spike, an upper-halfword load inside the reserved upper word still does not break the reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_low_byte_load_before_upper_word_sc_still_succeeds_corner()
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
    uint8_t observed = read8(upper_word_addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a same-start low-byte load is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening same-start low-byte load still observes the low 8-bit slice of the original upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 32)
    );
    TEST_ASSERT("on Spike, a same-start low-byte load inside the reserved upper word still does not break the reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset1_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uintptr_t byte_addr = upper_word_addr + 1U;
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
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a byte load at offset one is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening byte load at offset one still observes the first interior 8-bit slice of the original upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 40)
    );
    TEST_ASSERT("on Spike, a byte load at offset one inside the reserved upper word still does not break the reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_byte_at_offset2_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uintptr_t byte_addr = upper_word_addr + 2U;
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
    uint8_t observed = read8(byte_addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a byte load at offset two is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening byte load at offset two still observes the second interior 8-bit slice of the original upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 48)
    );
    TEST_ASSERT("on Spike, a byte load at offset two inside the reserved upper word still does not break the reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_address_intervening_tail_byte_load_before_upper_word_sc_still_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uintptr_t upper_word_addr = addr + sizeof(uint32_t);
    const uintptr_t tail_byte_addr = upper_word_addr + sizeof(uint32_t) - 1U;
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
    uint8_t observed = read8(tail_byte_addr);
    uint32_t sc_status = sc_w(upper_word_addr, committed_value);
    TEST_ASSERT("lr.w captures the original upper word before a tail-byte load is inserted between lr.w and sc.w",
        excpt.triggered == false &&
        reserved == (uint32_t)(initial_value >> 32)
    );
    TEST_ASSERT("the intervening tail-byte load still observes the high-edge 8-bit slice of the original upper-word payload while the reservation remains live",
        excpt.triggered == false &&
        observed == (uint8_t)(initial_value >> 56)
    );
    TEST_ASSERT("on Spike, a tail-byte load inside the reserved upper word still does not break the reservation and the following sc.w succeeds",
        excpt.triggered == false &&
        sc_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == expected_final
    );

    TEST_END();
}
