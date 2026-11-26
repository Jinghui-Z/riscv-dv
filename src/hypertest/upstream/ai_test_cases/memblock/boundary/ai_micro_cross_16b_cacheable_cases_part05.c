#include "ai_micro_cross_16b_cacheable_common.h"

bool ai_micro_hs_store_cross_16b_fault_then_5b3b_6b2b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES = 4
    };
    const bool use_access_fault[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        false,
        false,
        true,
        true,
    };
    const enum test_page fault_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        VSI_GI,
        VSI_GI,
        VSRWX_GRWX,
        VSRWX_GRWX,
    };
    const enum test_page ok_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        VSRW_GRW,
        SWITCH1,
        VSRW_GRW,
        SWITCH1,
    };
    const uintptr_t ok_offsets[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        11U,
        10U,
        11U,
        10U,
    };
    const uint64_t store_values[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        0x0123456789abcdefULL,
        0x89abcdef01234567ULL,
        0x1021324354657687ULL,
        0x0badf00dfacefeedULL,
    };
    const uint64_t fault_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
        0xddddccccbbbbaaaaULL,
    };
    const uint64_t fault_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        0x7766554433221100ULL,
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0xdeadbeefcafef00dULL,
    };
    const uint64_t fault_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        0xffeeddccbbaa9988ULL,
        0x98badcfe10325476ULL,
        0x66778899aabbccddULL,
        0x3141592653589793ULL,
    };
    const uint64_t fault_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        0x8877665544332211ULL,
        0xfedcba9876543210ULL,
        0x123456789abcdef0ULL,
        0x2718281828459045ULL,
    };
    const uint64_t ok_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        0x0123456789abcdefULL,
        0x3141592653589793ULL,
        0x1111222233334444ULL,
        0x5566778899aabbccULL,
    };
    const uint64_t ok_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        0xdeadbeefcafef00dULL,
        0xf0e1d2c3b4a59687ULL,
        0x7766554433221100ULL,
        0x1021324354657687ULL,
    };
    const uint64_t ok_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        0x0badf00dfacefeedULL,
        0x1021324354657687ULL,
        0xffeeddccbbaa9988ULL,
        0x3141592653589793ULL,
    };
    const uint64_t ok_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES] = {
        0x99aabbccddeeff00ULL,
        0x2718281828459045ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_STORE_CASES; ++i) {
        const uintptr_t fault_page_phys = phys_page_base(fault_pages[i]);
        const uintptr_t ok_page_phys = phys_page_base(ok_pages[i]);
        const uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(fault_pages[i]);
        const uintptr_t ok_vaddr =
            ai_micro_cross_16b_cacheable_addr_with_offset(ok_pages[i], ok_offsets[i]);
        const uintptr_t fault_leading_phys = fault_page_phys;
        const uintptr_t fault_low_phys = fault_page_phys + 8U;
        const uintptr_t fault_high_phys = fault_page_phys + 16U;
        const uintptr_t fault_trailing_phys = fault_page_phys + 24U;
        const uintptr_t ok_leading_phys = ok_page_phys;
        const uintptr_t ok_low_phys = ok_page_phys + 8U;
        const uintptr_t ok_high_phys = ok_page_phys + 16U;
        const uintptr_t ok_trailing_phys = ok_page_phys + 24U;
        uint64_t expected_ok_low = 0ULL;
        uint64_t expected_ok_high = 0ULL;

        goto_priv(PRIV_M);
        write64(fault_leading_phys, fault_leading_patterns[i]);
        write64(fault_low_phys, fault_low_patterns[i]);
        write64(fault_high_phys, fault_high_patterns[i]);
        write64(fault_trailing_phys, fault_trailing_patterns[i]);
        write64(ok_leading_phys, ok_leading_patterns[i]);
        write64(ok_low_phys, ok_low_patterns[i]);
        write64(ok_high_phys, ok_high_patterns[i]);
        write64(ok_trailing_phys, ok_trailing_patterns[i]);

        goto_priv(PRIV_HS);
        hspt_init();
        if (use_access_fault[i]) {
            goto_priv(PRIV_M);
            hspt_llptw_ppn_high_bit_overflow_setup();
        } else {
            goto_priv(PRIV_M);
        }
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
        TEST_ASSERT((use_access_fault[i] ?
            "a faulting cross-16B sd first reports store access fault before the later mirrored 5B+3B or 6B+2B recovery store is attempted" :
            "a faulting cross-16B sd first reports store page fault before the later mirrored 5B+3B or 6B+2B recovery store is attempted"),
            excpt.triggered == true &&
            excpt.cause == (use_access_fault[i] ? CAUSE_SAF : CAUSE_SPF) &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "the cross-16B store access-fault episode leaves both the faulting boundary and the later mirrored middle-ratio recovery boundary plus their adjacent doublewords unchanged" :
            "the cross-16B store page-fault episode leaves both the faulting boundary and the later mirrored middle-ratio recovery boundary plus their adjacent doublewords unchanged"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == ok_low_patterns[i] &&
            read64(ok_high_phys) == ok_high_patterns[i] &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sd(ok_vaddr, store_values[i]);
        TEST_ASSERT((use_access_fault[i] ?
            "after the faulting cross-16B sd access-fault episode is cleared, a later mirrored 5B+3B or 6B+2B recovery store commits without stale split-template carryover" :
            "after the faulting cross-16B sd page-fault episode is cleared, a later mirrored 5B+3B or 6B+2B recovery store commits without stale split-template carryover"),
            excpt.triggered == false
        );

        ai_micro_cross_16b_expected_words_after_store(
            ok_low_patterns[i],
            ok_high_patterns[i],
            (unsigned)(ok_offsets[i] - 8U),
            8U,
            store_values[i],
            &expected_ok_low,
            &expected_ok_high);

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "after the access-fault then mirrored middle-ratio store recovery sequence, the faulting boundary stays untouched and the recovery boundary reflects only its own exact byte image" :
            "after the page-fault then mirrored middle-ratio store recovery sequence, the faulting boundary stays untouched and the recovery boundary reflects only its own exact byte image"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == expected_ok_low &&
            read64(ok_high_phys) == expected_ok_high &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_page_fault_then_valid_cross_16b_store_recovers_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(ok_phys, leading_pattern);
    write64(ok_phys + 8U, low_pattern);
    write64(ok_phys + 16U, high_pattern);
    write64(ok_phys + 24U, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("a cross-16B HS store first reports store page fault when the misaligned access lands on an invalid leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after the cross-16B store page fault, a following cacheable cross-16B store on a valid page still commits normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-16B store updates only the two participating doublewords while the adjacent doublewords stay unchanged",
        read64(ok_phys) == leading_pattern &&
        read64(ok_phys + 8U) == 0xef66554433221100ULL &&
        read64(ok_phys + 16U) == 0xff0123456789abcdULL &&
        read64(ok_phys + 24U) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_fault_then_3b5b_2b6b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES = 4
    };
    const bool use_access_fault[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        false,
        false,
        true,
        true,
    };
    const enum test_page fault_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        VSI_GI,
        VSI_GI,
        VSRWX_GRWX,
        VSRWX_GRWX,
    };
    const enum test_page ok_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        VSRW_GRW,
        SWITCH1,
        VSRW_GRW,
        SWITCH1,
    };
    const uintptr_t ok_offsets[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        13U,
        14U,
        13U,
        14U,
    };
    const uint64_t fault_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        0x3141592653589793ULL,
        0x0123456789abcdefULL,
        0x5566778899aabbccULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t fault_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        0x8070605040302010ULL,
        0xdeadbeefcafef00dULL,
        0x89abcdef01234567ULL,
        0x0badf00dfacefeedULL,
    };
    const uint64_t fault_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        0x1122334455667799ULL,
        0x99aabbccddeeff00ULL,
        0x66778899aabbccddULL,
        0x8877665544332211ULL,
    };
    const uint64_t fault_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        0x2718281828459045ULL,
        0x123456789abcdef0ULL,
        0xddddccccbbbbaaaaULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t ok_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        0x1111222233334444ULL,
        0x4444333322221111ULL,
        0x9999aaaabbbbccccULL,
        0x7777666655554444ULL,
    };
    const uint64_t ok_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        0x7766554433221100ULL,
        0x0123456789abcdefULL,
        0xffeeddccbbaa9988ULL,
        0x1021324354657687ULL,
    };
    const uint64_t ok_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        0xffeeddccbbaa9988ULL,
        0x3141592653589793ULL,
        0x1122334455667788ULL,
        0x5a5aa5a5cc33dd44ULL,
    };
    const uint64_t ok_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES] = {
        0xaaaaaaaa55555555ULL,
        0xdddddddd88888888ULL,
        0x123456789abcdef0ULL,
        0xfaceb00c1234abcdULL,
    };

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_LOAD_CASES; ++i) {
        const uintptr_t fault_page_phys = phys_page_base(fault_pages[i]);
        const uintptr_t ok_page_phys = phys_page_base(ok_pages[i]);
        const uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(fault_pages[i]);
        const uintptr_t ok_vaddr =
            ai_micro_cross_16b_cacheable_addr_with_offset(ok_pages[i], ok_offsets[i]);
        const uintptr_t fault_leading_phys = fault_page_phys;
        const uintptr_t fault_low_phys = fault_page_phys + 8U;
        const uintptr_t fault_high_phys = fault_page_phys + 16U;
        const uintptr_t fault_trailing_phys = fault_page_phys + 24U;
        const uintptr_t ok_leading_phys = ok_page_phys;
        const uintptr_t ok_low_phys = ok_page_phys + 8U;
        const uintptr_t ok_high_phys = ok_page_phys + 16U;
        const uintptr_t ok_trailing_phys = ok_page_phys + 24U;
        const uint64_t expected_ok_value = ai_micro_cross_16b_expected_value(
            ok_low_patterns[i],
            ok_high_patterns[i],
            (unsigned)(ok_offsets[i] - 8U),
            8U,
            false);
        uint64_t observed = 0ULL;

        goto_priv(PRIV_M);
        write64(fault_leading_phys, fault_leading_patterns[i]);
        write64(fault_low_phys, fault_low_patterns[i]);
        write64(fault_high_phys, fault_high_patterns[i]);
        write64(fault_trailing_phys, fault_trailing_patterns[i]);
        write64(ok_leading_phys, ok_leading_patterns[i]);
        write64(ok_low_phys, ok_low_patterns[i]);
        write64(ok_high_phys, ok_high_patterns[i]);
        write64(ok_trailing_phys, ok_trailing_patterns[i]);

        goto_priv(PRIV_HS);
        hspt_init();
        if (use_access_fault[i]) {
            goto_priv(PRIV_M);
            hspt_llptw_ppn_high_bit_overflow_setup();
        } else {
            goto_priv(PRIV_M);
        }
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ld(fault_vaddr);
        TEST_ASSERT((use_access_fault[i] ?
            "a faulting cross-16B ld first reports load access fault before the later low-half-narrow 3B+5B or 2B+6B recovery load is attempted" :
            "a faulting cross-16B ld first reports load page fault before the later low-half-narrow 3B+5B or 2B+6B recovery load is attempted"),
            excpt.triggered == true &&
            excpt.cause == (use_access_fault[i] ? CAUSE_LAF : CAUSE_LPF) &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "the cross-16B load access-fault episode leaves both the faulting boundary and the later low-half-narrow recovery boundary plus their adjacent doublewords unchanged" :
            "the cross-16B load page-fault episode leaves both the faulting boundary and the later low-half-narrow recovery boundary plus their adjacent doublewords unchanged"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == ok_low_patterns[i] &&
            read64(ok_high_phys) == ok_high_patterns[i] &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed = ld(ok_vaddr);
        TEST_ASSERT((use_access_fault[i] ?
            "after the faulting cross-16B ld access-fault episode is cleared, a later low-half-narrow 3B+5B or 2B+6B recovery load returns only its own fresh merged payload with no stale merge-template carryover" :
            "after the faulting cross-16B ld page-fault episode is cleared, a later low-half-narrow 3B+5B or 2B+6B recovery load returns only its own fresh merged payload with no stale merge-template carryover"),
            excpt.triggered == false &&
            observed == expected_ok_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "after the access-fault then low-half-narrow load recovery sequence, the faulting boundary stays untouched and the recovery boundary keeps only its original load-source bytes" :
            "after the page-fault then low-half-narrow load recovery sequence, the faulting boundary stays untouched and the recovery boundary keeps only its original load-source bytes"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == ok_low_patterns[i] &&
            read64(ok_high_phys) == ok_high_patterns[i] &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_fault_then_3b5b_2b6b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES = 4
    };
    const bool use_access_fault[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        false,
        false,
        true,
        true,
    };
    const enum test_page fault_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        VSI_GI,
        VSI_GI,
        VSRWX_GRWX,
        VSRWX_GRWX,
    };
    const enum test_page ok_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        VSRW_GRW,
        SWITCH1,
        VSRW_GRW,
        SWITCH1,
    };
    const uintptr_t ok_offsets[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        13U,
        14U,
        13U,
        14U,
    };
    const uint64_t store_values[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        0x0123456789abcdefULL,
        0x89abcdef01234567ULL,
        0x1021324354657687ULL,
        0x0badf00dfacefeedULL,
    };
    const uint64_t fault_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        0x1111222233334444ULL,
        0x4444333322221111ULL,
        0x9999aaaabbbbccccULL,
        0x7777666655554444ULL,
    };
    const uint64_t fault_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        0x7766554433221100ULL,
        0x0123456789abcdefULL,
        0xffeeddccbbaa9988ULL,
        0x1021324354657687ULL,
    };
    const uint64_t fault_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        0xffeeddccbbaa9988ULL,
        0x3141592653589793ULL,
        0x1122334455667788ULL,
        0x5a5aa5a5cc33dd44ULL,
    };
    const uint64_t fault_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        0xaaaaaaaa55555555ULL,
        0xdddddddd88888888ULL,
        0x123456789abcdef0ULL,
        0xfaceb00c1234abcdULL,
    };
    const uint64_t ok_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        0x0123456789abcdefULL,
        0x3141592653589793ULL,
        0x1111222233334444ULL,
        0x5566778899aabbccULL,
    };
    const uint64_t ok_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        0xdeadbeefcafef00dULL,
        0xf0e1d2c3b4a59687ULL,
        0x7766554433221100ULL,
        0x89abcdef01234567ULL,
    };
    const uint64_t ok_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        0x0badf00dfacefeedULL,
        0x1021324354657687ULL,
        0xffeeddccbbaa9988ULL,
        0x3141592653589793ULL,
    };
    const uint64_t ok_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES] = {
        0x99aabbccddeeff00ULL,
        0x2718281828459045ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_FAULT_RECOVER_LOW_MID_STORE_CASES; ++i) {
        const uintptr_t fault_page_phys = phys_page_base(fault_pages[i]);
        const uintptr_t ok_page_phys = phys_page_base(ok_pages[i]);
        const uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(fault_pages[i]);
        const uintptr_t ok_vaddr =
            ai_micro_cross_16b_cacheable_addr_with_offset(ok_pages[i], ok_offsets[i]);
        const uintptr_t fault_leading_phys = fault_page_phys;
        const uintptr_t fault_low_phys = fault_page_phys + 8U;
        const uintptr_t fault_high_phys = fault_page_phys + 16U;
        const uintptr_t fault_trailing_phys = fault_page_phys + 24U;
        const uintptr_t ok_leading_phys = ok_page_phys;
        const uintptr_t ok_low_phys = ok_page_phys + 8U;
        const uintptr_t ok_high_phys = ok_page_phys + 16U;
        const uintptr_t ok_trailing_phys = ok_page_phys + 24U;
        uint64_t expected_ok_low = 0ULL;
        uint64_t expected_ok_high = 0ULL;

        goto_priv(PRIV_M);
        write64(fault_leading_phys, fault_leading_patterns[i]);
        write64(fault_low_phys, fault_low_patterns[i]);
        write64(fault_high_phys, fault_high_patterns[i]);
        write64(fault_trailing_phys, fault_trailing_patterns[i]);
        write64(ok_leading_phys, ok_leading_patterns[i]);
        write64(ok_low_phys, ok_low_patterns[i]);
        write64(ok_high_phys, ok_high_patterns[i]);
        write64(ok_trailing_phys, ok_trailing_patterns[i]);

        goto_priv(PRIV_HS);
        hspt_init();
        if (use_access_fault[i]) {
            goto_priv(PRIV_M);
            hspt_llptw_ppn_high_bit_overflow_setup();
        } else {
            goto_priv(PRIV_M);
        }
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
        TEST_ASSERT((use_access_fault[i] ?
            "a faulting cross-16B sd first reports store access fault before the later low-half-narrow 3B+5B or 2B+6B recovery store is attempted" :
            "a faulting cross-16B sd first reports store page fault before the later low-half-narrow 3B+5B or 2B+6B recovery store is attempted"),
            excpt.triggered == true &&
            excpt.cause == (use_access_fault[i] ? CAUSE_SAF : CAUSE_SPF) &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "the cross-16B store access-fault episode leaves both the faulting boundary and the later low-half-narrow recovery boundary plus their adjacent doublewords unchanged" :
            "the cross-16B store page-fault episode leaves both the faulting boundary and the later low-half-narrow recovery boundary plus their adjacent doublewords unchanged"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == ok_low_patterns[i] &&
            read64(ok_high_phys) == ok_high_patterns[i] &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sd(ok_vaddr, store_values[i]);
        TEST_ASSERT((use_access_fault[i] ?
            "after the faulting cross-16B sd access-fault episode is cleared, a later low-half-narrow 3B+5B or 2B+6B recovery store commits without stale split-template carryover" :
            "after the faulting cross-16B sd page-fault episode is cleared, a later low-half-narrow 3B+5B or 2B+6B recovery store commits without stale split-template carryover"),
            excpt.triggered == false
        );

        ai_micro_cross_16b_expected_words_after_store(
            ok_low_patterns[i],
            ok_high_patterns[i],
            (unsigned)(ok_offsets[i] - 8U),
            8U,
            store_values[i],
            &expected_ok_low,
            &expected_ok_high);

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "after the access-fault then low-half-narrow store recovery sequence, the faulting boundary stays untouched and the recovery boundary reflects only its own exact byte image" :
            "after the page-fault then low-half-narrow store recovery sequence, the faulting boundary stays untouched and the recovery boundary reflects only its own exact byte image"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == expected_ok_low &&
            read64(ok_high_phys) == expected_ok_high &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_page_fault_then_valid_cross_16b_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = ok_phys;
    uintptr_t low_phys = ok_phys + 8U;
    uintptr_t high_phys = ok_phys + 16U;
    uintptr_t trailing_phys = ok_phys + 24U;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("a cross-16B HS store first reports store page fault when the misaligned access lands on an invalid leaf in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-16B store page fault leaves both participating doublewords and their adjacent doublewords unchanged before recovery",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after the cross-16B store page fault, a following cacheable cross-16B store on a valid page still commits normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-16B store updates only the two participating doublewords while the adjacent doublewords stay unchanged in the adjacent-preservation corner",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == 0xef66554433221100ULL &&
        read64(high_phys) == 0xff0123456789abcdULL &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("an HS store that is misaligned across 16B reports store access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_access_fault_then_valid_cross_16b_store_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, leading_pattern);
    write64(paddr + 8U, low_pattern);
    write64(paddr + 16U, high_pattern);
    write64(paddr + 24U, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("a cross-16B HS store first reports store access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-16B store access fault leaves both participating doublewords and their adjacent sentinels unchanged before recovery",
        read64(paddr) == leading_pattern &&
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern &&
        read64(paddr + 24U) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after restoring the translation, the same cacheable cross-16B store succeeds and updates the split bytes normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-16B store updates only the two participating doublewords while the adjacent doublewords stay unchanged",
        read64(paddr) == leading_pattern &&
        read64(paddr + 8U) == 0xef66554433221100ULL &&
        read64(paddr + 16U) == 0xff0123456789abcdULL &&
        read64(paddr + 24U) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_access_fault_then_valid_cross_16b_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("a cross-16B HS store first reports store access fault when the leaf PPN overflows in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-16B store access fault leaves both participating doublewords and their adjacent doublewords unchanged before recovery",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after restoring the translation, the same cacheable cross-16B store succeeds and updates the split bytes normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-16B store updates only the two participating doublewords while the adjacent doublewords stay unchanged in the adjacent-preservation corner",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == 0xef66554433221100ULL &&
        read64(high_phys) == 0xff0123456789abcdULL &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_spike_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(ok_phys + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(ok_phys + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, fault_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both the original misaligned load address slot and the true second-half-start slot armed, the first cross-16B load page-fault attempt still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with the same original misaligned-load slot and second-half-start slot still armed, the second cross-16B load page-fault attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the cross-16B load first falls back to load page fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits and the fallback page fault, a following valid cacheable cross-16B load still succeeds immediately while the remaining second-half-start slot stays ignored on Spike",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_spike_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = ok_phys;
    uintptr_t low_phys = ok_phys + 8U;
    uintptr_t high_phys = ok_phys + 16U;
    uintptr_t trailing_phys = ok_phys + 24U;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, fault_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both the original misaligned load address slot and the true second-half-start slot armed, the first cross-16B load page-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped dual-address-trigger plus page-fault load probe still leaves the later recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with the same original misaligned-load slot and second-half-start slot still armed, the second cross-16B load page-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped dual-address-trigger plus page-fault load probe still leaves the later recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the cross-16B load first falls back to load page fault with the original misaligned VA in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback dual-address-trigger plus page-fault load path still leaves the later recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits and the fallback page fault, a following valid cacheable cross-16B load still succeeds immediately while the remaining second-half-start slot stays ignored on Spike in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered dual-address-trigger load path updates no backing state beyond reading the target doublewords, so the adjacent doublewords stay unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both the original misaligned load address slot and the true second-half-start slot armed, the first cross-16B load access-fault attempt still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with the same original misaligned-load slot and second-half-start slot still armed, the second cross-16B load access-fault attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the cross-16B load first falls back to load access fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits and the fallback access fault, restoring translation lets the same cacheable cross-16B load succeed while the remaining second-half-start slot stays ignored on Spike",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both the original misaligned load address slot and the true second-half-start slot armed, the first cross-16B load access-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped dual-address-trigger plus access-fault load probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with the same original misaligned-load slot and second-half-start slot still armed, the second cross-16B load access-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped dual-address-trigger plus access-fault load probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the cross-16B load first falls back to load access fault with the original misaligned VA in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback dual-address-trigger plus access-fault load path still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits and the fallback access fault, restoring translation lets the same cacheable cross-16B load succeed while the remaining second-half-start slot stays ignored on Spike in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered dual-address-trigger load path does not dirty either adjacent sentinel around the two touched doublewords",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_spike_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(ok_phys, leading_pattern);
    write64(ok_phys + 8U, low_pattern);
    write64(ok_phys + 16U, high_pattern);
    write64(ok_phys + 24U, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, fault_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both the original misaligned store address slot and the true second-half-start slot armed, the first cross-16B store page-fault attempt still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with the same original misaligned-store slot and second-half-start slot still armed, the second cross-16B store page-fault attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the cross-16B store first falls back to store page fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits and the fallback page fault, a following valid cacheable cross-16B store still commits immediately while the remaining second-half-start slot stays ignored on Spike",
        excpt.triggered == false
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered dual-address-trigger store path updates only the two participating doublewords while the adjacent doublewords stay unchanged after the trigger-plus-page-fault sequence",
        read64(ok_phys) == leading_pattern &&
        read64(ok_phys + 8U) == 0xef66554433221100ULL &&
        read64(ok_phys + 16U) == 0xff0123456789abcdULL &&
        read64(ok_phys + 24U) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_spike_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = ok_phys;
    uintptr_t low_phys = ok_phys + 8U;
    uintptr_t high_phys = ok_phys + 16U;
    uintptr_t trailing_phys = ok_phys + 24U;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, fault_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both the original misaligned store address slot and the true second-half-start slot armed, the first cross-16B store page-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped dual-address-trigger plus page-fault store probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with the same original misaligned-store slot and second-half-start slot still armed, the second cross-16B store page-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped dual-address-trigger plus page-fault store probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the cross-16B store first falls back to store page fault with the original misaligned VA in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback dual-address-trigger plus page-fault store path still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits and the fallback page fault, a following valid cacheable cross-16B store still commits immediately while the remaining second-half-start slot stays ignored on Spike in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered dual-address-trigger store path updates only the two participating doublewords while the adjacent doublewords stay unchanged in the adjacent-preservation corner",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == 0xef66554433221100ULL &&
        read64(high_phys) == 0xff0123456789abcdULL &&
        read64(trailing_phys) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(paddr, leading_pattern);
    write64(paddr + 8U, low_pattern);
    write64(paddr + 16U, high_pattern);
    write64(paddr + 24U, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both the original misaligned store address slot and the true second-half-start slot armed, the first cross-16B store access-fault attempt still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with the same original misaligned-store slot and second-half-start slot still armed, the second cross-16B store access-fault attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the cross-16B store first falls back to store access fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback dual-address-trigger plus access-fault store path still leaves both participating doublewords and their adjacent sentinels unchanged before recovery",
        read64(paddr) == leading_pattern &&
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern &&
        read64(paddr + 24U) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits and the fallback access fault, restoring translation lets the same cacheable cross-16B store succeed while the remaining second-half-start slot stays ignored on Spike",
        excpt.triggered == false
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered dual-address-trigger store path updates only the two participating doublewords while the adjacent doublewords stay unchanged after the trigger-plus-access-fault sequence",
        read64(paddr) == leading_pattern &&
        read64(paddr + 8U) == 0xef66554433221100ULL &&
        read64(paddr + 16U) == 0xff0123456789abcdULL &&
        read64(paddr + 24U) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both the original misaligned store address slot and the true second-half-start slot armed, the first cross-16B store access-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped dual-address-trigger plus access-fault store probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with the same original misaligned-store slot and second-half-start slot still armed, the second cross-16B store access-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped dual-address-trigger plus access-fault store probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the cross-16B store first falls back to store access fault with the original misaligned VA in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback dual-address-trigger plus access-fault store path still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits and the fallback access fault, restoring translation lets the same cacheable cross-16B store succeed while the remaining second-half-start slot stays ignored on Spike in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered dual-address-trigger store path updates only the two participating doublewords while the adjacent doublewords stay unchanged in the adjacent-preservation corner",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == 0xef66554433221100ULL &&
        read64(high_phys) == 0xff0123456789abcdULL &&
        read64(trailing_phys) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}
