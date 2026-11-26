#include "ai_lr_sc_common.h"

static uint64_t ai_lr_sc_ap29_lane_matrix[8] __attribute__((aligned(64)));

static inline uint64_t ai_lr_sc_ap29_lane_seed(size_t lane)
{
    return 0x1020304050607000ULL + ((uint64_t)lane * 0x111111111111111ULL);
}

static void ai_lr_sc_ap29_reset_lanes(void)
{
    goto_priv(PRIV_M);
    for (size_t lane = 0; lane < 8U; lane++) {
        ai_lr_sc_ap29_lane_matrix[lane] = ai_lr_sc_ap29_lane_seed(lane);
    }
}

static bool ai_lr_sc_ap29_lanes_match(size_t changed_lane,
    uint64_t changed_value)
{
    for (size_t lane = 0; lane < 8U; lane++) {
        uint64_t expected =
            (lane == changed_lane) ? changed_value :
                ai_lr_sc_ap29_lane_seed(lane);
        if (ai_lr_sc_ap29_lane_matrix[lane] != expected) {
            return false;
        }
    }

    return true;
}

bool ai_micro_lr_sc_same_block_all_doubleword_lane_mismatch_matrix_corner()
{
    TEST_START();

    bool matrix_ok = true;
    const uint64_t mismatched_payload = 0x0badc0ffee0dd00dULL;
    const uint64_t refreshed_base = 0xa100000000000000ULL;

    for (size_t lr_lane = 0; lr_lane < 8U; lr_lane++) {
        for (size_t sc_lane = 0; sc_lane < 8U; sc_lane++) {
            if (lr_lane == sc_lane) {
                continue;
            }

            ai_lr_sc_ap29_reset_lanes();
            uintptr_t lr_addr =
                (uintptr_t)&ai_lr_sc_ap29_lane_matrix[lr_lane];
            uintptr_t sc_addr =
                (uintptr_t)&ai_lr_sc_ap29_lane_matrix[sc_lane];

            TEST_SETUP_EXCEPT();
            uint64_t lr_value = ai_lr_sc_lr_d64(lr_addr);
            uint32_t sc_status =
                ai_lr_sc_sc_d_status(sc_addr, mismatched_payload);
            bool mismatch_failed =
                excpt.triggered == false &&
                lr_value == ai_lr_sc_ap29_lane_seed(lr_lane) &&
                sc_status != 0U &&
                ai_lr_sc_ap29_lanes_match(8U, 0U);

            TEST_SETUP_EXCEPT();
            uint64_t refreshed_value =
                refreshed_base | ((uint64_t)lr_lane << 8) |
                (uint64_t)sc_lane;
            uint64_t refreshed_reserved = ai_lr_sc_lr_d64(sc_addr);
            uint32_t refreshed_status =
                ai_lr_sc_sc_d_status(sc_addr, refreshed_value);
            bool refresh_succeeded =
                excpt.triggered == false &&
                refreshed_reserved == ai_lr_sc_ap29_lane_seed(sc_lane) &&
                refreshed_status == 0U &&
                ai_lr_sc_ap29_lanes_match(sc_lane, refreshed_value);

            matrix_ok = matrix_ok && mismatch_failed && refresh_succeeded;
        }
    }

    TEST_ASSERT("every lr.d/sc.d pair targeting two different doubleword lanes inside one aligned 64B block fails without writing, and each lane can still be refreshed by a fresh same-address lr.d/sc.d",
        matrix_ok
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_block_sfence_refill_mismatch_preserves_full_address_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t line_offset = 0x500U;
    const uintptr_t lr_vaddr = hs_page_base(page) + line_offset;
    const uintptr_t sc_vaddr = lr_vaddr + sizeof(uint64_t);
    const uintptr_t lr_paddr = phys_page_base(page) + line_offset;
    const uintptr_t sc_paddr = lr_paddr + sizeof(uint64_t);
    const uint64_t lr_initial = 0x1122334455667788ULL;
    const uint64_t sc_initial = 0x99aabbccddeeff00ULL;
    const uint64_t mismatch_payload = 0x0123456789abcdefULL;
    const uint64_t refresh_payload = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(lr_paddr, lr_initial);
    write64(sc_paddr, sc_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t lr_value = ai_lr_sc_lr_d64(lr_vaddr);
    sfence_vma();
    uint32_t mismatch_status =
        ai_lr_sc_sc_d_status(sc_vaddr, mismatch_payload);
    TEST_ASSERT("after sfence.vma forces the store-conditional translation path to be refilled, sc.d to a different doubleword in the same line still fails by full physical address",
        excpt.triggered == false &&
        lr_value == lr_initial &&
        mismatch_status != 0U &&
        read64(lr_vaddr) == lr_initial &&
        read64(sc_vaddr) == sc_initial
    );

    TEST_SETUP_EXCEPT();
    sfence_vma();
    uint64_t refresh_reserved = ai_lr_sc_lr_d64(sc_vaddr);
    uint32_t refresh_status =
        ai_lr_sc_sc_d_status(sc_vaddr, refresh_payload);
    TEST_ASSERT("after the sfence-refill mismatch probe, a fresh same-address lr.d/sc.d pair on the second doubleword succeeds and proves the address remains writable",
        excpt.triggered == false &&
        refresh_reserved == sc_initial &&
        refresh_status == 0U &&
        read64(lr_vaddr) == lr_initial &&
        read64(sc_vaddr) == refresh_payload
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the sfence-refill mismatch probe contains only the controlled same-address refresh payload",
        read64(lr_paddr) == lr_initial &&
        read64(sc_paddr) == refresh_payload
    );

    TEST_END();
}

bool ai_micro_lr_sc_cold_lr_after_previous_reservation_no_stale_full_address_success_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t a_offset = 0x580U;
    const uintptr_t b_offset = 0x600U;
    const uintptr_t a_vaddr = hs_page_base(page) + a_offset;
    const uintptr_t b_vaddr = hs_page_base(page) + b_offset;
    const uintptr_t a_guard_vaddr = a_vaddr + sizeof(uint64_t);
    const uintptr_t b_guard_vaddr = b_vaddr + sizeof(uint64_t);
    const uintptr_t a_paddr = phys_page_base(page) + a_offset;
    const uintptr_t b_paddr = phys_page_base(page) + b_offset;
    const uintptr_t a_guard_paddr = a_paddr + sizeof(uint64_t);
    const uintptr_t b_guard_paddr = b_paddr + sizeof(uint64_t);
    const uint64_t a_initial = 0x1021324354657687ULL;
    const uint64_t b_initial = 0x8877665544332211ULL;
    const uint64_t a_guard = 0x0badf00d13579bdfULL;
    const uint64_t b_guard = 0x55aa33cc77ee11ffULL;
    const uint64_t stale_a_payload = 0xa5a5a5a55a5a5a5aULL;
    const uint64_t stale_b_payload = 0xfeedfacecafebeefULL;
    const uint64_t b_refreshed = 0xc001d00d12345678ULL;

    goto_priv(PRIV_M);
    write64(a_paddr, a_initial);
    write64(b_paddr, b_initial);
    write64(a_guard_paddr, a_guard);
    write64(b_guard_paddr, b_guard);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed_a = ld(a_vaddr);
    uint64_t warmed_b = ld(b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("both LR/SC target lines are readable before the cold-LR stale-full-address probe",
        warmed_a == a_initial && warmed_b == b_initial
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(b_vaddr);
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the B line can be invalidated before the later lr.d B refills a cold reservation path",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_a_reserved = ai_lr_sc_lr_d64(a_vaddr);
    TEST_ASSERT("lr.d A establishes an older full-address reservation before the cold lr.d B episode",
        excpt.triggered == false &&
        old_a_reserved == a_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t cold_b_reserved = ai_lr_sc_lr_d64(b_vaddr);
    uint32_t stale_a_status =
        ai_lr_sc_sc_d_status(a_vaddr, stale_a_payload);
    TEST_ASSERT("after a cold lr.d B, a naked sc.d A must fail and leave A unchanged, otherwise the old full address survived the newer LR",
        excpt.triggered == false &&
        cold_b_reserved == b_initial &&
        stale_a_status != 0U &&
        read64(a_vaddr) == a_initial &&
        read64(b_vaddr) == b_initial &&
        read64(a_guard_vaddr) == a_guard &&
        read64(b_guard_vaddr) == b_guard
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_b_status =
        ai_lr_sc_sc_d_status(b_vaddr, stale_b_payload);
    TEST_ASSERT("the failed stale-address sc.d A consumes the live LR/SC reservation, so sc.d B without a fresh lr.d also fails and does not write",
        excpt.triggered == false &&
        stale_b_status != 0U &&
        read64(a_vaddr) == a_initial &&
        read64(b_vaddr) == b_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t fresh_b_reserved = ai_lr_sc_lr_d64(b_vaddr);
    uint32_t fresh_b_status =
        ai_lr_sc_sc_d_status(b_vaddr, b_refreshed);
    TEST_ASSERT("a fresh lr.d/sc.d pair on B still succeeds after the cold-LR stale-full-address guard, proving the recovery path is writable",
        excpt.triggered == false &&
        fresh_b_reserved == b_initial &&
        fresh_b_status == 0U &&
        read64(a_vaddr) == a_initial &&
        read64(b_vaddr) == b_refreshed &&
        read64(a_guard_vaddr) == a_guard &&
        read64(b_guard_vaddr) == b_guard
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the cold-LR stale-full-address probe contains no stale A write and only the final fresh B payload",
        read64(a_paddr) == a_initial &&
        read64(b_paddr) == b_refreshed &&
        read64(a_guard_paddr) == a_guard &&
        read64(b_guard_paddr) == b_guard
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_lr_sc_same_block_different_doubleword_range_fail_then_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t lr_addr = ai_lr_sc_addr(0);
    const uintptr_t mismatched_sc_addr = ai_lr_sc_addr(2);
    const uint64_t first_initial = 0x1020304050607080ULL;
    const uint64_t second_initial = 0x5566778899aabbccULL;
    const uint64_t refreshed_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = first_initial;
    *(uint64_t *)&ai_lr_sc_words[2] = second_initial;

    TEST_SETUP_EXCEPT();
    uint64_t lr_value = ai_lr_sc_lr_d64(lr_addr);
    TEST_ASSERT("lr.d reserves the original doubleword before a different-range sc.d in the same block forces the first failure",
        excpt.triggered == false &&
        lr_value == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = ai_lr_sc_sc_d_status(mismatched_sc_addr, 0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("on Spike, sc.d first fails when it targets a different doubleword range inside the same block than the preceding lr.d",
        excpt.triggered == false &&
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == second_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(lr_addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(lr_addr, refreshed_value);
    TEST_ASSERT("after that same-block different-range failure, a fresh lr.d still captures the original target doubleword",
        excpt.triggered == false &&
        refreshed_reserved == first_initial
    );
    TEST_ASSERT("after refreshing the reservation on the original doubleword, the following sc.d succeeds and leaves the neighboring doubleword unchanged",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value &&
        *(uint64_t *)&ai_lr_sc_words[2] == second_initial
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_block_different_doubleword_range_fail_then_word_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t lr_addr = ai_lr_sc_addr(0);
    const uintptr_t mismatched_sc_addr = ai_lr_sc_addr(2);
    const uint64_t first_initial = 0x1020304050607080ULL;
    const uint64_t second_initial = 0x5566778899aabbccULL;
    const uint32_t refreshed_value = 0xaabbccddU;
    const uint64_t refreshed_expected =
        0x1020304000000000ULL | (uint64_t)refreshed_value;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = first_initial;
    *(uint64_t *)&ai_lr_sc_words[2] = second_initial;

    TEST_SETUP_EXCEPT();
    uint64_t lr_value = ai_lr_sc_lr_d64(lr_addr);
    TEST_ASSERT("lr.d reserves the original doubleword before a different-range sc.d in the same block forces the first failure",
        excpt.triggered == false &&
        lr_value == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = ai_lr_sc_sc_d_status(mismatched_sc_addr, 0x0f0ff0f05a5aa5a5ULL);
    TEST_ASSERT("on Spike, sc.d first fails when it targets a different doubleword range inside the same block than the preceding lr.d",
        excpt.triggered == false &&
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == first_initial &&
        *(uint64_t *)&ai_lr_sc_words[2] == second_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(lr_addr);
    uint32_t refreshed_status = sc_w(lr_addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that same-block different-range failure, a fresh lr.w still captures the original low word of the target doubleword",
        refreshed_reserved == (uint32_t)first_initial
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a narrower lr.w/sc.w pair on the original start address, only the low word updates while the rest of the block stays unchanged",
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_expected &&
        *(uint64_t *)&ai_lr_sc_words[2] == second_initial
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_start_different_size_fails()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2233445566778899ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the low word before an sc.d attempts to commit with the same start byte but a different size",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = ai_lr_sc_sc_d_status(addr, 0xdeadbeefcafef00dULL);
    AI_ASSERT_NO_EXCEPT_AND("sc.d fails when it reuses the same start byte as lr.w but changes the access size",
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_start_word_then_doubleword_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2233445566778899ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the low word before a wider same-start sc.d first fails",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, 0xa5a55a5af0f00f0fULL);
    AI_ASSERT_NO_EXCEPT_AND("the first sc.d fails when it reuses the same start byte as lr.w but changes the access size",
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(addr, 0xaabbccddU);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size same-start sc.d, retrying sc.w without a fresh lr.w still fails because the failed sc.d has already invalidated the reservation",
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_start_word_then_doubleword_fail_then_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2233445566778899ULL;
    const uint64_t refreshed_value = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the low word before a wider same-start sc.d first fails",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, 0xa5a55a5af0f00f0fULL);
    AI_ASSERT_NO_EXCEPT_AND("sc.d first fails when it reuses the same start byte as lr.w but changes the access size",
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after that mixed-size same-start failure, a fresh lr.d still captures the original doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d, the following sc.d succeeds and overwrites the whole doubleword",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_start_word_then_doubleword_fail_then_word_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2233445566778899ULL;
    const uint32_t refreshed_value = 0xa5a55a5aU;
    const uint64_t refreshed_expected =
        0x2233445500000000ULL | (uint64_t)refreshed_value;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(addr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w captures the low word before a wider same-start sc.d first fails",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = ai_lr_sc_sc_d_status(addr, 0xa5a55a5af0f00f0fULL);
    AI_ASSERT_NO_EXCEPT_AND("sc.d first fails when it reuses the same start byte as lr.w but changes the access size",
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that mixed-size same-start failure, a fresh lr.w still captures the original low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair, only the low word is updated while the original high word is preserved",
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_start_doubleword_then_word_different_size_fails_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2233445566778899ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t lr_value = ai_lr_sc_lr_d64(addr);
    TEST_ASSERT("lr.d captures the full doubleword before an sc.w attempts to commit with the same start byte but a different size",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = sc_w(addr, 0xa5a55a5aU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, sc.w fails when it reuses the same start byte as lr.d but changes the access size",
        sc_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_start_doubleword_then_word_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2233445566778899ULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t lr_value = ai_lr_sc_lr_d64(addr);
    TEST_ASSERT("lr.d captures the full doubleword before a narrower same-start sc.w first fails",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("the first sc.w fails when it reuses the same start byte as lr.d but changes the access size",
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(addr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after that failed mixed-size same-start sc.w, retrying sc.d without a fresh lr.d still fails because the failed sc.w has already invalidated the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_start_doubleword_then_word_fail_then_word_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2233445566778899ULL;
    const uint32_t refreshed_value = 0xa5a55a5aU;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t lr_value = ai_lr_sc_lr_d64(addr);
    TEST_ASSERT("lr.d captures the full doubleword before a narrower same-start sc.w first fails",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, sc.w first fails when it reuses the same start byte as lr.d but changes the access size",
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(addr);
    uint32_t refreshed_status = sc_w(addr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that mixed-size same-start failure, a fresh lr.w still captures the original low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w, the following sc.w succeeds and updates only the low word",
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] ==
            ((((uint64_t)(initial_value >> 32)) << 32) | (uint64_t)refreshed_value)
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_start_doubleword_then_word_fail_then_doubleword_refresh_succeeds_corner()
{
    TEST_START();

    const uintptr_t addr = ai_lr_sc_addr(0);
    const uint64_t initial_value = 0x2233445566778899ULL;
    const uint64_t refreshed_value = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    *(uint64_t *)&ai_lr_sc_words[0] = initial_value;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;

    TEST_SETUP_EXCEPT();
    uint64_t lr_value = ai_lr_sc_lr_d64(addr);
    TEST_ASSERT("lr.d captures the full doubleword before a narrower same-start sc.w first fails",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(addr, 0x0badc0deU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, sc.w first fails when it reuses the same start byte as lr.d but changes the access size",
        first_status != 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(addr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(addr, refreshed_value);
    TEST_ASSERT("after that mixed-size same-start failure, a fresh lr.d still captures the original doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair, the whole doubleword is overwritten by the refreshed payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        *(uint64_t *)&ai_lr_sc_words[0] == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_fails()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_lr_sc_hs_leaf_entry(alias_page) =
        (paddr >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared backing word before the step-wrapped non-immediate alias-sc.w sequence is exercised",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("within the current step-wrapped non-immediate same-paddr alias path, sc.w fails when it switches to the other virtual alias",
        sc_status != 0U
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(base_vaddr);
    TEST_ASSERT("the shared physical word remains unchanged after that step-wrapped non-immediate alias-sc.w failure",
        excpt.triggered == false &&
        observed == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x778899aaU;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared backing word before the different-vaddr alias mismatch is exercised",
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first sc.w still fails when it targets the same physical word through a different virtual alias in this non-immediate sequence",
        first_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(base_vaddr, 0x2468ace0U);
    AI_ASSERT_NO_EXCEPT_AND("after that failed alias-mismatched sc.w, retrying sc.w on the original alias without a fresh lr.w still fails because the failed sc.w has already consumed the reservation",
        second_status != 0U &&
        read32(alias_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias-mismatched failed-sc consumption corner, the shared backing word remains unchanged because neither sc.w commits",
        read32(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x778899aaU;
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared backing word before the alias-mismatched failure-and-original-refresh corner runs",
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the alias-mismatched sc.w first fails and leaves the shared word unchanged in this non-immediate sequence",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed alias-mismatched sc.w, a fresh lr.w on the original alias still captures the original shared word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the original alias, the following sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias-mismatched failure-and-original-refresh corner, the shared backing word contains only the refreshed original-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x778899aaU;
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared backing word before the alias-mismatched failure-and-switched-refresh corner runs",
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the alias-mismatched sc.w first fails and leaves the shared word unchanged in this non-immediate sequence",
        failed_status != 0U &&
        read32(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed alias-mismatched sc.w, a fresh lr.w on the switched alias still captures the original shared word",
        refreshed_reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing the reservation on the switched alias, the following sc.w succeeds and both aliases observe the new payload",
        refreshed_status == 0U &&
        read32(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias-mismatched failure-and-switched-refresh corner, the shared backing word contains only the refreshed switched-alias payload",
        read32(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_diff_setidx_doubleword_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t guard_value = 0x1122334455667788ULL;
    const uint64_t failed_payload = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(paddr + sizeof(uint64_t), guard_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the AP-63 minimal doubleword alias pair maps two different virtual set indices to the same physical doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t sc_status = ai_lr_sc_sc_d_status(alias_vaddr, failed_payload);
    TEST_ASSERT("lr.d through one virtual set followed by sc.d through a same-PA different-setidx alias fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        reserved == initial_value &&
        sc_status != 0U &&
        read64(base_vaddr) == initial_value &&
        read64(alias_vaddr) == initial_value &&
        read64(base_vaddr + sizeof(uint64_t)) == guard_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-63 minimal doubleword alias-fail probe leaves the shared backing doubleword unchanged",
        read64(paddr) == initial_value &&
        read64(paddr + sizeof(uint64_t)) == guard_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_diff_setidx_doubleword_failed_sc_consumes_reservation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t guard_value = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(paddr + sizeof(uint64_t), guard_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the AP-63 failed-sc-consumption doubleword alias pair uses different virtual set indices for one physical doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t first_status =
        ai_lr_sc_sc_d_status(alias_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("the first sc.d through the different-setidx same-PA alias fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        reserved == initial_value &&
        first_status != 0U &&
        read64(base_vaddr) == initial_value &&
        read64(alias_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status =
        ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that failed alias sc.d, retrying sc.d on the original alias without a fresh lr.d also fails because the failed sc.d consumed the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(base_vaddr) == initial_value &&
        read64(alias_vaddr + sizeof(uint64_t)) == guard_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-63 direct doubleword failed-sc consumption probe leaves backing target and guard unchanged",
        read64(paddr) == initial_value &&
        read64(paddr + sizeof(uint64_t)) == guard_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_diff_setidx_doubleword_fail_then_refresh_original_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t refreshed_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the AP-63 original-refresh doubleword alias pair uses different virtual set indices for one physical doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t failed_status =
        ai_lr_sc_sc_d_status(alias_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("the direct different-setidx alias sc.d fails before the original-alias refresh path is exercised",
        excpt.triggered == false &&
        reserved == initial_value &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status =
        ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after the failed alias sc.d, a fresh lr.d/sc.d on the original alias succeeds and both aliases observe the refreshed value",
        excpt.triggered == false &&
        refreshed_reserved == initial_value &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-63 direct doubleword original-refresh probe commits only the fresh original-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_diff_setidx_doubleword_fail_then_refresh_switched_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
    const uint64_t refreshed_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();
    TEST_ASSERT("the AP-63 switched-refresh doubleword alias pair uses different virtual set indices for one physical doubleword",
        ai_lr_sc_setidx(base_vaddr) != ai_lr_sc_setidx(alias_vaddr)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t failed_status =
        ai_lr_sc_sc_d_status(alias_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("the direct different-setidx alias sc.d fails before the switched-alias refresh path is exercised",
        excpt.triggered == false &&
        reserved == initial_value &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status =
        ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after the failed alias sc.d, a fresh lr.d/sc.d on the switched alias succeeds and both aliases observe the refreshed value",
        excpt.triggered == false &&
        refreshed_reserved == initial_value &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-63 direct doubleword switched-refresh probe commits only the fresh switched-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_fail_then_refresh_original_doubleword_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared low word before the alias-mismatched failure-and-original-doubleword-refresh corner runs",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the alias-mismatched sc.w first fails and leaves the shared doubleword unchanged in this non-immediate sequence",
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that failed alias-mismatched sc.w, a fresh lr.d on the original alias still captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a wider lr.d/sc.d pair on the original alias, the shared doubleword is fully overwritten and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias-mismatched failure-and-original-doubleword-refresh corner, the shared backing doubleword contains only the refreshed original-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_fail_then_refresh_switched_doubleword_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x33445566778899aaULL;
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared low word before the alias-mismatched failure-and-switched-doubleword-refresh corner runs",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the alias-mismatched sc.w first fails and leaves the shared doubleword unchanged in this non-immediate sequence",
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that failed alias-mismatched sc.w, a fresh lr.d on the switched alias still captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a wider lr.d/sc.d pair on the switched alias, the shared doubleword is fully overwritten and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the alias-mismatched failure-and-switched-doubleword-refresh corner, the shared backing doubleword contains only the refreshed switched-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fails_corner()
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared low word before the second alias attempts a wider sc.d in the non-immediate mixed-size corner",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = ai_lr_sc_sc_d_status(alias_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("on Spike, that non-immediate mixed-size alias sc.d fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        sc_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the non-immediate mixed-size alias failure corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_failed_sc_consumes_reservation_corner()
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared low word before the failed mixed-size alias-sc.d consumption corner runs",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = ai_lr_sc_sc_d_status(alias_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("on Spike, the first non-immediate mixed-size alias sc.d fails and preserves the original shared doubleword",
        excpt.triggered == false &&
        first_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(base_vaddr, 0x2468ace0U);
    AI_ASSERT_NO_EXCEPT_AND("after that failed alias sc.d, retrying sc.w on the original alias without a fresh lr.w still fails because the failed sc.d has already consumed the reservation",
        second_status != 0U &&
        read64(alias_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.d consumption corner, the shared backing doubleword remains unchanged because neither store-conditional commits",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fail_then_refresh_original_word_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared low word before the failed mixed-size alias-sc.d original-word-refresh corner runs",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = ai_lr_sc_sc_d_status(alias_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("on Spike, the first non-immediate mixed-size alias sc.d fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.d, a fresh lr.w on the original alias still captures the original shared low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the original alias, only the shared low word is updated while the high word is preserved",
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.d original-word-refresh corner, the shared backing doubleword keeps the preserved high word and the refreshed original-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fail_then_refresh_switched_word_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared low word before the failed mixed-size alias-sc.d switched-word-refresh corner runs",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = ai_lr_sc_sc_d_status(alias_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("on Spike, the first non-immediate mixed-size alias sc.d fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.d, a fresh lr.w on the switched alias still captures the original shared low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the switched alias, only the shared low word is updated while the high word is preserved",
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.d switched-word-refresh corner, the shared backing doubleword keeps the preserved high word and the refreshed switched-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fail_then_refresh_original_doubleword_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared low word before the failed mixed-size alias-sc.d original-doubleword-refresh corner runs",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = ai_lr_sc_sc_d_status(alias_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("on Spike, the first non-immediate mixed-size alias sc.d fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that failed mixed-size alias sc.d, a fresh lr.d on the original alias still captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the original alias, the shared doubleword is fully overwritten and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.d original-doubleword-refresh corner, the shared backing doubleword contains only the refreshed original-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fail_then_refresh_switched_doubleword_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint32_t lr_value = lr_w(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared low word before the failed mixed-size alias-sc.d switched-doubleword-refresh corner runs",
        lr_value == (uint32_t)initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = ai_lr_sc_sc_d_status(alias_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("on Spike, the first non-immediate mixed-size alias sc.d fails and leaves the shared doubleword unchanged",
        excpt.triggered == false &&
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that failed mixed-size alias sc.d, a fresh lr.d on the switched alias still captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the switched alias, the shared doubleword is fully overwritten and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.d switched-doubleword-refresh corner, the shared backing doubleword contains only the refreshed switched-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fails_corner()
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
    uint64_t lr_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the second alias attempts a narrower sc.w in the non-immediate mixed-size corner",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t sc_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, that non-immediate mixed-size alias sc.w fails and leaves the shared doubleword unchanged",
        sc_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the non-immediate mixed-size alias sc.w failure corner, the shared backing doubleword remains unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_failed_sc_consumes_reservation_corner()
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
    uint64_t lr_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the failed mixed-size alias-sc.w consumption corner runs",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t first_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first non-immediate mixed-size alias sc.w fails and preserves the original shared doubleword",
        first_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = ai_lr_sc_sc_d_status(base_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after that failed alias sc.w, retrying sc.d on the original alias without a fresh lr.d still fails because the failed sc.w has already consumed the reservation",
        excpt.triggered == false &&
        second_status != 0U &&
        read64(alias_vaddr) == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.w consumption corner, the shared backing doubleword remains unchanged because neither store-conditional commits",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fail_then_refresh_original_word_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t lr_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the failed mixed-size alias-sc.w original-word-refresh corner runs",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first non-immediate mixed-size alias sc.w fails and leaves the shared doubleword unchanged",
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(base_vaddr);
    uint32_t refreshed_status = sc_w(base_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.w, a fresh lr.w on the original alias still captures the original shared low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the original alias, only the shared low word is updated while the high word is preserved",
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.w original-word-refresh corner, the shared backing doubleword keeps the preserved high word and the refreshed original-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fail_then_refresh_switched_word_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t lr_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the failed mixed-size alias-sc.w switched-word-refresh corner runs",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first non-immediate mixed-size alias sc.w fails and leaves the shared doubleword unchanged",
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t refreshed_reserved = lr_w(alias_vaddr);
    uint32_t refreshed_status = sc_w(alias_vaddr, refreshed_value);
    AI_ASSERT_NO_EXCEPT_AND("after that failed mixed-size alias sc.w, a fresh lr.w on the switched alias still captures the original shared low word",
        refreshed_reserved == (uint32_t)initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("after refreshing with a same-size lr.w/sc.w pair on the switched alias, only the shared low word is updated while the high word is preserved",
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.w switched-word-refresh corner, the shared backing doubleword keeps the preserved high word and the refreshed switched-alias sc.w payload",
        read64(paddr) == refreshed_expected
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fail_then_refresh_original_doubleword_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t lr_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the failed mixed-size alias-sc.w original-doubleword-refresh corner runs",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first non-immediate mixed-size alias sc.w fails and leaves the shared doubleword unchanged",
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(base_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(base_vaddr, refreshed_value);
    TEST_ASSERT("after that failed mixed-size alias sc.w, a fresh lr.d on the original alias still captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the original alias, the shared doubleword is fully overwritten and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(alias_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.w original-doubleword-refresh corner, the shared backing doubleword contains only the refreshed original-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fail_then_refresh_switched_doubleword_succeeds_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint64_t initial_value = 0x1122334455667788ULL;
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
    uint64_t lr_value = ai_lr_sc_lr_d64(base_vaddr);
    TEST_ASSERT("lr.d through the first virtual alias captures the shared doubleword before the failed mixed-size alias-sc.w switched-doubleword-refresh corner runs",
        excpt.triggered == false &&
        lr_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t failed_status = sc_w(alias_vaddr, 0x13579bdfU);
    AI_ASSERT_NO_EXCEPT_AND("on Spike, the first non-immediate mixed-size alias sc.w fails and leaves the shared doubleword unchanged",
        failed_status != 0U &&
        read64(base_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_reserved = ai_lr_sc_lr_d64(alias_vaddr);
    uint32_t refreshed_status = ai_lr_sc_sc_d_status(alias_vaddr, refreshed_value);
    TEST_ASSERT("after that failed mixed-size alias sc.w, a fresh lr.d on the switched alias still captures the original shared doubleword",
        excpt.triggered == false &&
        refreshed_reserved == initial_value
    );
    TEST_ASSERT("after refreshing with a same-size lr.d/sc.d pair on the switched alias, the shared doubleword is fully overwritten and both aliases observe the new payload",
        excpt.triggered == false &&
        refreshed_status == 0U &&
        read64(base_vaddr) == refreshed_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the failed mixed-size alias-sc.w switched-doubleword-refresh corner, the shared backing doubleword contains only the refreshed switched-alias sc.d payload",
        read64(paddr) == refreshed_value
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_cachehit_first_succeeds_second_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x33445566U;

    goto_priv(PRIV_M);
    write32(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRS(CSR_MCOUNTEREN, HCOUNTEREN_CY);
    ai_lr_sc_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t reserved = lr_w(base_vaddr);
    const uint32_t first_committed = 0x13579bdfU;
    uint32_t alias_status = sc_w(alias_vaddr, first_committed);
    uint32_t base_retry_status = sc_w(base_vaddr, 0x2468ace0U);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared backing word before the immediate alias-switch double-sc corner runs",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-size sc.w succeeds when it immediately switches to a same-paddr virtual alias that hits in DCache",
        alias_status == 0U
    );
    AI_ASSERT_NO_EXCEPT_AND("after that immediate alias-switch success, a second sc.w back on the original alias fails without a new lr.w because the reservation has already been consumed",
        base_retry_status != 0U &&
        read32(base_vaddr) == first_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch double-sc corner, the shared backing word keeps only the first successful alias payload",
        read32(paddr) == first_committed
    );

    TEST_END();
}

bool ai_micro_lr_sc_same_paddr_alias_immediate_cachehit_success_then_alias_retry_fails_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = AI_LR_SC_ALIAS_CACHE_HIT_PAGE;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t alias_vaddr = hs_page_base(alias_page);
    const uintptr_t paddr = phys_page_base(base_page);
    const uint32_t initial_value = 0x33445566U;
    const uint32_t first_committed = 0x13579bdfU;

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
    uint32_t first_status = sc_w(alias_vaddr, first_committed);
    AI_ASSERT_NO_EXCEPT_AND("lr.w through the first virtual alias captures the shared word before the immediate alias-switch same-alias retry corner runs",
        reserved == initial_value
    );
    AI_ASSERT_NO_EXCEPT_AND("the first same-size sc.w succeeds when it immediately switches to the other same-paddr alias and the alias access hits in DCache",
        first_status == 0U &&
        read32(base_vaddr) == first_committed
    );

    TEST_SETUP_EXCEPT();
    uint32_t second_status = sc_w(alias_vaddr, 0x2468ace0U);
    AI_ASSERT_NO_EXCEPT_AND("after that successful alias sc.w, retrying sc.w again on the same switched alias without a fresh lr.w still fails because the reservation has already been consumed",
        second_status != 0U &&
        read32(base_vaddr) == first_committed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the immediate alias-switch same-alias retry corner, the shared backing word keeps only the first successful alias payload",
        read32(paddr) == first_committed
    );

    TEST_END();
}
