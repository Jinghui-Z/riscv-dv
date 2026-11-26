#include "ai_micro_memblock_flush_atomic_common.h"

bool ai_micro_hs_fence_drain_then_amo_no_flush_owner_collision_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x240U;
    const uintptr_t pressure_vaddr = target_vaddr + 64U;
    const uintptr_t target_guard_vaddr = target_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x340U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x240U;
    const uintptr_t pressure_paddr = target_paddr + 64U;
    const uintptr_t target_guard_paddr = target_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x340U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x1011223344556677ULL,
        0x2021334455667788ULL,
        0x3031445566778899ULL,
    };
    const uint64_t drained_store_values[] = {
        0x1111111122222222ULL,
        0x3333333344444444ULL,
        0x5555555566666666ULL,
    };
    const uint64_t amo_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t pressure_values[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t target_guard_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadbeeff00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t marker_guard_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0;
         round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(target_seeds);
         ++round) {
        const uint64_t expected_target =
            drained_store_values[round] + amo_addends[round];

        goto_priv(PRIV_M);
        write64(target_paddr, target_seeds[round]);
        write64(pressure_paddr, 0ULL);
        write64(target_guard_paddr, target_guard_values[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guard_values[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the fence-drain plus AMO flush-owner probe starts, target and marker words are readable with seeded values",
            warmed_target == target_seeds[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(target_vaddr, drained_store_values[round]);
        sd(pressure_vaddr, pressure_values[round]);
        fence_iorw_iorw();
        const uint64_t amo_old_value = amoadd_d(target_vaddr,
            amo_addends[round]);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_vaddr);
        const uint64_t observed_pressure = ld(pressure_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("a cacheable fence-drain immediately followed by AMO and follow-up store keeps the preceding store, AMO old value, and later marker store in order",
            amo_old_value == drained_store_values[round] &&
            observed_target == expected_target &&
            observed_pressure == pressure_values[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        const uint64_t observed_target_guard = ld(target_guard_vaddr);
        const uint64_t observed_marker_guard = ld(marker_guard_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the fence-drain plus AMO flush-owner window does not corrupt adjacent guard words or block later scalar loads",
            observed_target_guard == target_guard_values[round] &&
            observed_marker_guard == marker_guard_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the fence-drain plus AMO window contains only the current round AMO result, pressure store, marker store, and unchanged guards",
            read64(target_paddr) == expected_target &&
            read64(pressure_paddr) == pressure_values[round] &&
            read64(target_guard_paddr) == target_guard_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guard_values[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_fence_drain_then_lrsc_no_flush_owner_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x2a0U;
    const uintptr_t pressure_vaddr = target_vaddr + 64U;
    const uintptr_t target_guard_vaddr = target_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x360U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x2a0U;
    const uintptr_t pressure_paddr = target_paddr + 64U;
    const uintptr_t target_guard_paddr = target_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x360U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };
    const uint64_t drained_store_values[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t sc_payloads[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t pressure_values[] = {
        0xa1a2a3a4a5a6a7a8ULL,
        0xb1b2b3b4b5b6b7b8ULL,
        0xc1c2c3c4c5c6c7c8ULL,
    };
    const uint64_t marker_seeds[] = {
        0x1020304050607080ULL,
        0x21436587a9cbed0fULL,
        0x3141592653589793ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t target_guard_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadbeeff00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t marker_guard_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0;
         round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(target_seeds);
         ++round) {
        uint64_t lr_observed = 0ULL;
        uint64_t sc_status = 1ULL;

        goto_priv(PRIV_M);
        write64(target_paddr, target_seeds[round]);
        write64(pressure_paddr, 0ULL);
        write64(target_guard_paddr, target_guard_values[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guard_values[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the fence-drain plus LR/SC flush-owner probe starts, target and marker words are readable with seeded values",
            warmed_target == target_seeds[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "sd %[drained_store], 0(%[target_addr])\n\t"
            "sd %[pressure_store], 0(%[pressure_addr])\n\t"
            "fence rw, rw\n\t"
            "lr.d %[lr_out], 0(%[target_addr])\n\t"
            "sc.d %[sc_status], %[sc_payload], 0(%[target_addr])\n\t"
            "sd %[marker_update], 0(%[marker_addr])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [lr_out] "=&r"(lr_observed),
              [sc_status] "=&r"(sc_status)
            : [target_addr] "r"(target_vaddr),
              [drained_store] "r"(drained_store_values[round]),
              [pressure_addr] "r"(pressure_vaddr),
              [pressure_store] "r"(pressure_values[round]),
              [sc_payload] "r"(sc_payloads[round]),
              [marker_addr] "r"(marker_vaddr),
              [marker_update] "r"(marker_updates[round])
            : "memory");
        const uint64_t observed_target = ld(target_vaddr);
        const uint64_t observed_pressure = ld(pressure_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("a cacheable fence-drain immediately followed by LR/SC keeps the drained store visible, preserves reservation, and lets the later marker store progress",
            lr_observed == drained_store_values[round] &&
            sc_status == 0ULL &&
            observed_target == sc_payloads[round] &&
            observed_pressure == pressure_values[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        const uint64_t observed_target_guard = ld(target_guard_vaddr);
        const uint64_t observed_marker_guard = ld(marker_guard_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the fence-drain plus LR/SC flush-owner window does not corrupt adjacent guard words or block later scalar loads",
            observed_target_guard == target_guard_values[round] &&
            observed_marker_guard == marker_guard_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the fence-drain plus LR/SC window contains only the current SC payload, pressure store, marker store, and unchanged guards",
            read64(target_paddr) == sc_payloads[round] &&
            read64(pressure_paddr) == pressure_values[round] &&
            read64(target_guard_paddr) == target_guard_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guard_values[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_amo_then_fence_no_stale_atomic_flush_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x2e0U;
    const uintptr_t pressure_vaddr = target_vaddr + 64U;
    const uintptr_t target_guard_vaddr = target_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x380U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x2e0U;
    const uintptr_t pressure_paddr = target_paddr + 64U;
    const uintptr_t target_guard_paddr = target_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x380U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t amo_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t pressure_values[] = {
        0xa8a7a6a5a4a3a2a1ULL,
        0xb8b7b6b5b4b3b2b1ULL,
        0xc8c7c6c5c4c3c2c1ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t target_guard_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadbeeff00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t marker_guard_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0;
         round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(target_seeds);
         ++round) {
        const uint64_t expected_target =
            target_seeds[round] + amo_addends[round];
        uint64_t amo_old_value = 0ULL;

        goto_priv(PRIV_M);
        write64(target_paddr, target_seeds[round]);
        write64(pressure_paddr, 0ULL);
        write64(target_guard_paddr, target_guard_values[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guard_values[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the AMO plus fence owner-window probe starts, target and marker words are readable with seeded values",
            warmed_target == target_seeds[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "sd %[pressure_store], 0(%[pressure_addr])\n\t"
            "amoadd.d %[amo_old], %[amo_addend], (%[target_addr])\n\t"
            "fence rw, rw\n\t"
            "sd %[marker_update], 0(%[marker_addr])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [amo_old] "=&r"(amo_old_value)
            : [target_addr] "r"(target_vaddr),
              [amo_addend] "r"(amo_addends[round]),
              [pressure_addr] "r"(pressure_vaddr),
              [pressure_store] "r"(pressure_values[round]),
              [marker_addr] "r"(marker_vaddr),
              [marker_update] "r"(marker_updates[round])
            : "memory");
        const uint64_t observed_target = ld(target_vaddr);
        const uint64_t observed_pressure = ld(pressure_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block AMO immediately followed by fence keeps atomic flush ownership and sbIsEmpty ordered so the AMO result, pressure store, and later marker all remain current",
            amo_old_value == target_seeds[round] &&
            observed_target == expected_target &&
            observed_pressure == pressure_values[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        const uint64_t observed_target_guard = ld(target_guard_vaddr);
        const uint64_t observed_marker_guard = ld(marker_guard_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the AMO plus fence owner-window does not corrupt adjacent guard words or block later scalar loads",
            observed_target_guard == target_guard_values[round] &&
            observed_marker_guard == marker_guard_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the AMO plus fence owner-window contains only the current AMO result, pressure store, marker store, and unchanged guards",
            read64(target_paddr) == expected_target &&
            read64(pressure_paddr) == pressure_values[round] &&
            read64(target_guard_paddr) == target_guard_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guard_values[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_lrsc_then_fence_no_stale_reservation_flush_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x420U;
    const uintptr_t pressure_vaddr = target_vaddr + 64U;
    const uintptr_t target_guard_vaddr = target_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x420U;
    const uintptr_t pressure_paddr = target_paddr + 64U;
    const uintptr_t target_guard_paddr = target_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3c0U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t sc_payloads[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t pressure_values[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_seeds[] = {
        0x1020304050607080ULL,
        0x21436587a9cbed0fULL,
        0x3141592653589793ULL,
    };
    const uint64_t marker_updates[] = {
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x7766554433221100ULL,
    };
    const uint64_t target_guard_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadbeeff00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t marker_guard_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0;
         round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(target_seeds);
         ++round) {
        uint64_t lr_observed = 0ULL;
        uint64_t sc_status = 1ULL;

        goto_priv(PRIV_M);
        write64(target_paddr, target_seeds[round]);
        write64(pressure_paddr, 0ULL);
        write64(target_guard_paddr, target_guard_values[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guard_values[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the LR/SC plus fence owner-window probe starts, target and marker words are readable with seeded values",
            warmed_target == target_seeds[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "sd %[pressure_store], 0(%[pressure_addr])\n\t"
            "lr.d %[lr_out], 0(%[target_addr])\n\t"
            "sc.d %[sc_status], %[sc_payload], 0(%[target_addr])\n\t"
            "fence rw, rw\n\t"
            "sd %[marker_update], 0(%[marker_addr])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [lr_out] "=&r"(lr_observed),
              [sc_status] "=&r"(sc_status)
            : [target_addr] "r"(target_vaddr),
              [pressure_addr] "r"(pressure_vaddr),
              [pressure_store] "r"(pressure_values[round]),
              [sc_payload] "r"(sc_payloads[round]),
              [marker_addr] "r"(marker_vaddr),
              [marker_update] "r"(marker_updates[round])
            : "memory");
        const uint64_t observed_target = ld(target_vaddr);
        const uint64_t observed_pressure = ld(pressure_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block LR/SC immediately followed by fence keeps reservation ownership and sbIsEmpty ordered so SC result, pressure store, and later marker all remain current",
            lr_observed == target_seeds[round] &&
            sc_status == 0ULL &&
            observed_target == sc_payloads[round] &&
            observed_pressure == pressure_values[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        const uint64_t observed_target_guard = ld(target_guard_vaddr);
        const uint64_t observed_marker_guard = ld(marker_guard_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the LR/SC plus fence owner-window does not corrupt adjacent guard words or block later scalar loads",
            observed_target_guard == target_guard_values[round] &&
            observed_marker_guard == marker_guard_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the LR/SC plus fence owner-window contains only the current SC payload, pressure store, marker store, and unchanged guards",
            read64(target_paddr) == sc_payloads[round] &&
            read64(pressure_paddr) == pressure_values[round] &&
            read64(target_guard_paddr) == target_guard_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guard_values[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_load_fault_return_then_amo_borrowed_tlb_no_stale_response_corner()
{
    TEST_START();

    const enum test_page fault_page = SWITCH1;
    const enum test_page amo_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x1c0U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x1c0U;
    const uintptr_t fault_guard_paddr = fault_paddr + sizeof(uint64_t);
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0x2c0U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0x2c0U;
    const uintptr_t amo_guard_paddr = amo_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x340U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x340U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t fault_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0x55aa33cc77ee11ddULL,
    };
    const uint64_t fault_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t amo_seeds[] = {
        0x1111222233334444ULL,
        0x2222333344445555ULL,
        0x3333444455556666ULL,
    };
    const uint64_t amo_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t amo_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(amo_seeds);
         ++round) {
        const uint64_t expected_amo =
            amo_seeds[round] + amo_addends[round];

        goto_priv(PRIV_M);
        write64(fault_paddr, fault_seeds[round]);
        write64(fault_guard_paddr, fault_guards[round]);
        write64(amo_paddr, amo_seeds[round]);
        write64(amo_guard_paddr, amo_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_atomic_tlb_set_hs_leaf(fault_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_atomic_tlb_set_hs_leaf(amo_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_atomic_tlb_set_hs_leaf(marker_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        (void)ld(fault_vaddr);
        TEST_ASSERT("the leading HS scalar load reports its own load page fault before the AMO borrowed-TLB probe begins",
            excpt.triggered &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == fault_vaddr
        );

        TEST_SETUP_EXCEPT();
        const uint64_t amo_old = amoadd_d(amo_vaddr, amo_addends[round]);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_amo = ld(amo_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after returning from the load page fault, the immediate AMO that borrows load0 TLB state still observes and updates only its own address",
            amo_old == amo_seeds[round] &&
            observed_amo == expected_amo &&
            observed_marker == marker_updates[round]
        );

        ai_memblock_atomic_tlb_set_hs_leaf(fault_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        const uint64_t recovered_fault_load = ld(fault_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("repairing the earlier faulting load page after the AMO returns the original load payload with no stale atomic TLB or exception metadata",
            recovered_fault_load == fault_seeds[round] &&
            ld(amo_guard_paddr) == amo_guards[round] &&
            ld(marker_guard_paddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the load-fault-return plus AMO borrowed-TLB window contains only the AMO update, marker update, repaired load seed, and unchanged guards",
            read64(fault_paddr) == fault_seeds[round] &&
            read64(fault_guard_paddr) == fault_guards[round] &&
            read64(amo_paddr) == expected_amo &&
            read64(amo_guard_paddr) == amo_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_cbo_flush_then_amo_same_block_no_cmo_atomic_flush_owner_collision_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x300U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x300U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t target_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t target_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x380U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x380U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0101010101010101ULL,
        0x2121212121212121ULL,
        0x4141414141414141ULL,
    };
    const uint64_t pre_amo_stores[] = {
        0x1011222233334444ULL,
        0x2021333344445555ULL,
        0x3031444455556666ULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000101ULL,
        0x0000000000000202ULL,
        0x0000000000000303ULL,
    };
    const uint64_t second_addends[] = {
        0x0000000000000011ULL,
        0x0000000000000022ULL,
        0x0000000000000033ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_seeds[] = {
        0x55aa55aa33cc33ccULL,
        0xa55aa55ac33cc33cULL,
        0x5aa55aa53cc33cc3ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t target_guard_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t marker_guard_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t after_first_amo =
            pre_amo_stores[round] + first_addends[round];
        const uint64_t final_target =
            after_first_amo + second_addends[round];

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(target_guard_paddr, target_guard_values[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guard_values[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO-flush plus AMO owner-window probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(target_word_vaddr, pre_amo_stores[round]);
        sd(neighbor_word_vaddr, neighbor_updates[round]);
        cbo_clean(target_word_vaddr);
        cbo_flush(target_tail_vaddr);
        const uint64_t first_amo_old =
            amoadd_d(target_word_vaddr, first_addends[round]);
        cbo_clean(neighbor_word_vaddr);
        const uint64_t second_amo_old =
            amoadd_d(target_word_vaddr, second_addends[round]);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO clean/flush adjacent to two AMOs keeps CMO and atomic flush ownership separate with ordered old values and follow-up store progress",
            first_amo_old == pre_amo_stores[round] &&
            second_amo_old == after_first_amo &&
            observed_target == final_target &&
            observed_neighbor == neighbor_updates[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO/AMO flush-owner window does not corrupt adjacent guard words or block later scalar loads",
            ld(target_guard_vaddr) == target_guard_values[round] &&
            ld(marker_guard_vaddr) == marker_guard_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO/AMO owner-window probe contains only the current AMO result, same-block neighbor store, marker store, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                final_target,
                3U,
                neighbor_updates[round]) &&
            read64(target_guard_paddr) == target_guard_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guard_values[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_dual_amo_storedata_mux_no_cross_target_addend_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t base_vaddr = hs_page_base(target_page) + 0x580U;
    const uintptr_t base_paddr = phys_page_base(target_page) + 0x580U;
    const uintptr_t guard0_vaddr = base_vaddr;
    const uintptr_t target0_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t target1_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t guard1_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard0_paddr = base_paddr;
    const uintptr_t target0_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t target1_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t guard1_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x580U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x580U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t target0_seeds[] = {
        0x1010101020202020ULL,
        0x3030303040404040ULL,
        0x5050505060606060ULL,
    };
    const uint64_t target1_seeds[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t target0_first_addends[] = {
        0x0000000000000101ULL,
        0x0000000000000202ULL,
        0x0000000000000303ULL,
    };
    const uint64_t target1_first_addends[] = {
        0x0000000000001001ULL,
        0x0000000000002002ULL,
        0x0000000000003003ULL,
    };
    const uint64_t target0_second_addends[] = {
        0x0000000000010001ULL,
        0x0000000000020002ULL,
        0x0000000000030003ULL,
    };
    const uint64_t target1_second_addends[] = {
        0x0000000000100001ULL,
        0x0000000000200002ULL,
        0x0000000000300003ULL,
    };
    const uint64_t guard0_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t guard1_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guard_values[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0;
         round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(target0_seeds);
         ++round) {
        const uint64_t target0_after_first =
            target0_seeds[round] + target0_first_addends[round];
        const uint64_t target1_after_first =
            target1_seeds[round] + target1_first_addends[round];
        const uint64_t target0_final =
            target0_after_first + target0_second_addends[round];
        const uint64_t target1_final =
            target1_after_first + target1_second_addends[round];

        goto_priv(PRIV_M);
        write64(guard0_paddr, guard0_values[round]);
        write64(target0_paddr, target0_seeds[round]);
        write64(target1_paddr, target1_seeds[round]);
        write64(guard1_paddr, guard1_values[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guard_values[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target0 = ld(target0_vaddr);
        const uint64_t warmed_target1 = ld(target1_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the dual-AMO address/data mux probe starts, both adjacent targets and marker are readable with seeded values",
            warmed_target0 == target0_seeds[round] &&
            warmed_target1 == target1_seeds[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        const uint64_t target0_old_first =
            amoadd_d(target0_vaddr, target0_first_addends[round]);
        const uint64_t target1_old_first =
            amoadd_d(target1_vaddr, target1_first_addends[round]);
        const uint64_t target0_old_second =
            amoadd_d(target0_vaddr, target0_second_addends[round]);
        const uint64_t target1_old_second =
            amoadd_d(target1_vaddr, target1_second_addends[round]);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target0 = ld(target0_vaddr);
        const uint64_t observed_target1 = ld(target1_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("back-to-back AMOs to two adjacent targets keep each address request paired with its own storeDataIn addend and old value",
            target0_old_first == target0_seeds[round] &&
            target1_old_first == target1_seeds[round] &&
            target0_old_second == target0_after_first &&
            target1_old_second == target1_after_first &&
            observed_target0 == target0_final &&
            observed_target1 == target1_final &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the dual-AMO address/data mux window does not corrupt adjacent guard words or block later scalar loads",
            ld(guard0_vaddr) == guard0_values[round] &&
            ld(guard1_vaddr) == guard1_values[round] &&
            ld(marker_guard_vaddr) == marker_guard_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the dual-AMO mux probe contains only per-target addends, the marker update, and unchanged guards",
            read64(guard0_paddr) == guard0_values[round] &&
            read64(target0_paddr) == target0_final &&
            read64(target1_paddr) == target1_final &&
            read64(guard1_paddr) == guard1_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guard_values[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_cboinval_then_amo_no_stale_inval_owner_or_old_value_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xe40U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xe40U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t inval_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0x55aa33cc77ee11ddULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000101ULL,
        0x0000000000000202ULL,
        0x0000000000000303ULL,
    };
    const uint64_t second_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_inval_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t after_first_amo =
            block_seeds[round] + first_addends[round];
        const uint64_t final_target =
            after_first_amo + second_addends[round];
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        uint64_t first_amo_old = 0ULL;
        uint64_t second_amo_old = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL plus AMO owner-window probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_cboinval_amo_then_marker(inval_vaddr,
            target_word_vaddr, first_addends[round], second_addends[round],
            neighbor_word_vaddr, neighbor_updates[round], marker_vaddr,
            marker_updates[round], &first_amo_old, &second_amo_old);
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by same-block AMOs releases inval ownership so the first AMO old value comes from the refilled seed and marker progresses",
            first_amo_old == block_seeds[round] &&
            second_amo_old == after_first_amo &&
            observed_target == final_target &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL plus AMO owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL plus AMO owner-window contains the AMO result, neighbor store, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                final_target,
                5U,
                neighbor_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_inval_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_amo_then_cboinval_no_stale_atomic_flush_owner_or_inval_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xf40U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xf40U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t inval_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xf40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xf40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x123456789abcdef0ULL,
        0x0fedcba987654321ULL,
        0x456789abcdef0123ULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000111ULL,
        0x0000000000000222ULL,
        0x0000000000000333ULL,
    };
    const uint64_t second_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t neighbor_updates[] = {
        0xa1b2c3d4e5f60718ULL,
        0xb2c3d4e5f6071829ULL,
        0xc3d4e5f60718293aULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x2468ace013579bdfULL,
        0x13579bdf02468aceULL,
        0xeca86420fdb97531ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_inval_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t after_first_amo =
            block_seeds[round] + first_addends[round];
        const uint64_t final_target =
            after_first_amo + second_addends[round];
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        uint64_t first_amo_old = 0ULL;
        uint64_t second_amo_old = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the AMO plus CBO.INVAL owner-window probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_amo_cboinval_then_marker(target_word_vaddr,
            first_addends[round], second_addends[round], inval_vaddr,
            neighbor_word_vaddr, neighbor_updates[round], marker_vaddr,
            marker_updates[round], &first_amo_old, &second_amo_old);
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block AMOs immediately followed by CBO.INVAL keep atomic updates ordered before inval ownership releases to the later store and marker",
            first_amo_old == block_seeds[round] &&
            second_amo_old == after_first_amo &&
            observed_target == final_target &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the AMO plus CBO.INVAL owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the AMO plus CBO.INVAL owner-window contains the AMO result, post-inval neighbor store, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                final_target,
                5U,
                neighbor_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_inval_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cboinval_then_lrsc_no_stale_inval_owner_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x940U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x940U;
    const uintptr_t lrsc_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t inval_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x940U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x940U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1029384756abcdefULL,
        0x89abcdef76543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t sc_payloads[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_inval_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(lrsc_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL plus LR/SC owner-window probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_cboinval_lrsc_then_marker(inval_vaddr, lrsc_vaddr,
            sc_payloads[round], neighbor_word_vaddr,
            neighbor_updates[round], marker_vaddr, marker_updates[round],
            &lr_observed, &sc_status);
        const uint64_t observed_target = ld(lrsc_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by LR/SC releases inval ownership so LR sees the refilled seed, SC succeeds, and later stores progress",
            lr_observed == block_seeds[round] &&
            sc_status == 0ULL &&
            observed_target == sc_payloads[round] &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL plus LR/SC owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL plus LR/SC owner-window contains the successful SC payload, neighbor store, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                sc_payloads[round],
                5U,
                neighbor_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_inval_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_lrsc_then_cboinval_no_stale_reservation_or_inval_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x840U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x840U;
    const uintptr_t lrsc_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t inval_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x840U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x840U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x2468ace013579bdfULL,
        0x13579bdf02468aceULL,
        0xeca86420fdb97531ULL,
    };
    const uint64_t sc_payloads[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t neighbor_updates[] = {
        0xa1a2a3a4a5a6a7a8ULL,
        0xb1b2b3b4b5b6b7b8ULL,
        0xc1c2c3c4c5c6c7c8ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x123456789abcdef0ULL,
        0x0fedcba987654321ULL,
        0x456789abcdef0123ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_inval_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(lrsc_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the LR/SC plus CBO.INVAL owner-window probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_lrsc_cboinval_then_marker(lrsc_vaddr,
            sc_payloads[round], inval_vaddr, neighbor_word_vaddr,
            neighbor_updates[round], marker_vaddr, marker_updates[round],
            &lr_observed, &sc_status);
        const uint64_t observed_target = ld(lrsc_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("LR/SC immediately followed by CBO.INVAL keeps the successful SC visible before inval ownership releases to the later store and marker",
            lr_observed == block_seeds[round] &&
            sc_status == 0ULL &&
            observed_target == sc_payloads[round] &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the LR/SC plus CBO.INVAL owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the LR/SC plus CBO.INVAL owner-window contains the successful SC payload, post-inval neighbor store, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                sc_payloads[round],
                5U,
                neighbor_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_inval_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_fence_then_cboinval_no_stale_fence_flush_owner_or_inval_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x5c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x5c0U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x5c0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x5c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x324b5c6d7e8f9011ULL,
        0x435c6d7e8f901122ULL,
        0x546d7e8f90112233ULL,
    };
    const uint64_t drained_updates[] = {
        0x0ddc0ffeec0ffee0ULL,
        0x1ee1deadc0de0001ULL,
        0x2ee2facefeed0002ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0x1029384756a5b4c3ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t after_guards[] = {
        0x8877aabbccddeeffULL,
        0x77668899aabbccddULL,
        0x6655778899aabbccULL,
    };
    const uint64_t marker_seeds[] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };

    ai_memblock_cmo_inval_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the fence plus CBO.INVAL owner-window probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_neighbor == block_seeds[round] +
                3U * 0x1111111111111111ULL &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(target_word_vaddr, drained_updates[round]);
        sd(neighbor_word_vaddr, neighbor_updates[round]);
        fence_iorw_iorw();
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
            ".option pop\n\t"
            :
            : [inval_addr] "r"(inval_vaddr)
            : "memory");
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("fence drain immediately followed by CBO.INVAL keeps drained stores visible and releases inval ownership before the post-inval marker",
            observed_target == drained_updates[round] &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the fence plus CBO.INVAL owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the fence plus CBO.INVAL owner-window contains the drained stores, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                drained_updates[round],
                3U,
                neighbor_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_inval_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_amo_no_stale_zero_flush_owner_or_old_value_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x700U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x700U;
    const uintptr_t zero_subblock_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr +
        AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x700U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x700U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x2122232425262728ULL,
        0x4142434445464748ULL,
        0x6162636465666768ULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000001111ULL,
        0x0000000000002222ULL,
        0x0000000000003333ULL,
    };
    const uint64_t second_addends[] = {
        0x0000000000010001ULL,
        0x0000000000020002ULL,
        0x0000000000030003ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_seeds[] = {
        0x55aa55aa33cc33ccULL,
        0xa55aa55ac33cc33cULL,
        0x5aa55aa53cc33cc3ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t after_first_amo = first_addends[round];
        const uint64_t final_target =
            first_addends[round] + second_addends[round];

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus AMO flush-owner probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        cbo_zero(zero_subblock_vaddr);
        const uint64_t first_amo_old =
            amoadd_d(target_word_vaddr, first_addends[round]);
        const uint64_t second_amo_old =
            amoadd_d(target_word_vaddr, second_addends[round]);
        sd(neighbor_word_vaddr, neighbor_updates[round]);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by AMOs keeps zero flush ownership separate so the first AMO old value is zero and later stores still progress",
            first_amo_old == 0ULL &&
            second_amo_old == after_first_amo &&
            observed_target == final_target &&
            observed_neighbor == neighbor_updates[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus AMO owner-window does not corrupt adjacent guard words or block later scalar loads",
            ld(target_base_vaddr + 2U * sizeof(uint64_t)) == 0ULL &&
            ld(target_tail_vaddr) == 0ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus AMO owner-window contains only the current AMO result, same-block neighbor store, marker store, and unchanged guards",
            ai_memblock_cmo_amo_zero_block_matches_two_updates(
                target_base_paddr,
                0U,
                final_target,
                5U,
                neighbor_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_vsegment_no_stale_zero_flush_owner_or_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xa00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xa00U;
    const uintptr_t zero_subblock_vaddr =
        src_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0xb00U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0xb00U;
    const uintptr_t dst_before_vaddr = dst_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr = dst_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t dst_before_paddr = dst_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr = dst_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x123456789abcdef0ULL,
        0x21436587a9cbed0fULL,
        0x3141592653589793ULL,
    };
    const uint64_t dst_fills[] = {
        0x5a5a5a5a5a5a5a5aULL,
        0xa5a5a5a5a5a5a5a5ULL,
        0x3c3c3c3c3c3c3c3cULL,
    };
    const uint64_t dst_before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(dst_paddr, dst_fills[round]);
        write64(dst_paddr + sizeof(uint64_t), ~dst_fills[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus VSegment flush-owner probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        cbo_zero(zero_subblock_vaddr);
        ai_memblock_cmo_vsegment_copy_then_marker(src_base_vaddr, dst_vaddr,
            marker_vaddr, marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_dst0 = ld(dst_vaddr);
        const uint64_t observed_dst1 = ld(dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by VSegment load/store keeps zero flush ownership separate so segment payload is zero and marker still progresses",
            observed_src == 0ULL &&
            observed_tail == 0ULL &&
            observed_dst0 == 0ULL &&
            observed_dst1 == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus VSegment owner-window does not corrupt destination or marker guards",
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus VSegment owner-window contains a fully zeroed source block, zero segment destination, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_all_zero(src_base_paddr) &&
            read64(dst_paddr) == 0ULL &&
            read64(dst_paddr + sizeof(uint64_t)) == 0ULL &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_normal_vector_no_stale_zero_flush_owner_or_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xd00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xd00U;
    const uintptr_t zero_subblock_vaddr =
        src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0xe00U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0xe00U;
    const uintptr_t dst_before_vaddr = dst_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr = dst_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_before_paddr = dst_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr = dst_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t dst_fills[] = {
        0xc3c3c3c3c3c3c3c3ULL,
        0x3c3c3c3c3c3c3c3cULL,
        0xa55aa55ac33cc33cULL,
    };
    const uint64_t dst_before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(dst_paddr, dst_fills[round]);
        write64(dst_paddr + sizeof(uint64_t), ~dst_fills[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus normal-vector flush-owner probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        cbo_zero(zero_subblock_vaddr);
        ai_memblock_cmo_normal_vector_copy_then_marker(src_base_vaddr,
            dst_vaddr, marker_vaddr, marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_dst0 = ld(dst_vaddr);
        const uint64_t observed_dst1 = ld(dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by normal vector load/store keeps zero flush ownership separate so vector payload is zero and marker still progresses",
            observed_src == 0ULL &&
            observed_tail == 0ULL &&
            observed_dst0 == 0ULL &&
            observed_dst1 == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus normal-vector owner-window does not corrupt destination or marker guards",
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus normal-vector owner-window contains a fully zeroed source block, zero vector destination, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_all_zero(src_base_paddr) &&
            read64(dst_paddr) == 0ULL &&
            read64(dst_paddr + sizeof(uint64_t)) == 0ULL &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_cross16_load_no_stale_zero_flush_owner_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xf00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xf00U;
    const uintptr_t zero_subblock_vaddr =
        src_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xf00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xf00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1112131415161718ULL,
        0x2122232425262728ULL,
        0x3132333435363738ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus cross-16B load probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        cbo_zero(zero_subblock_vaddr);
        const uint64_t cross16_observed =
            ai_memblock_cmo_cross16_load_then_marker(cross16_load_vaddr,
                marker_vaddr, marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by a same-block cross-16B load keeps zero flush ownership separate so the split-load merge payload is zero and marker still progresses",
            cross16_observed == 0ULL &&
            observed_src == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus cross-16B load owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus cross-16B load owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_all_zero(src_base_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cbom_then_cross16_load_no_stale_cmo_flush_owner_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xd00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xd00U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x31425364758697a8ULL,
    };
    const uint64_t word1_updates[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8877665544332211ULL,
    };
    const uint64_t word2_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0x1123581321345589ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(word1_updates[round],
                word2_updates[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM plus cross-16B load probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == block_seeds[round] + 0x1111111111111111ULL &&
            warmed_word2 == block_seeds[round] + 2U *
                0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_word1_vaddr, word1_updates[round]);
        sd(src_word2_vaddr, word2_updates[round]);
        cbo_clean(src_base_vaddr);
        cbo_flush(src_tail_vaddr);
        const uint64_t cross16_observed =
            ai_memblock_cmo_cross16_load_then_marker(cross16_load_vaddr,
                marker_vaddr, marker_updates[round]);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by a same-block cross-16B load keeps CMO flush ownership separate so the split-load merge payload sees the drained stores",
            cross16_observed == expected_cross16 &&
            observed_word1 == word1_updates[round] &&
            observed_word2 == word2_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM plus cross-16B load owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM plus cross-16B load owner-window contains the two drained source stores, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                word1_updates[round],
                2U,
                word2_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cboinval_then_cross16_load_no_stale_inval_owner_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xc40U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xc40U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = src_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x445566778899aabbULL,
        0x5566778899aabbccULL,
        0x66778899aabbccddULL,
    };
    const uint64_t before_guards[] = {
        0x1011223344556677ULL,
        0x2021334455667788ULL,
        0x3031445566778899ULL,
    };
    const uint64_t after_guards[] = {
        0x9988776655443322ULL,
        0x8877665544332211ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0a0b0c0d0e0f0102ULL,
        0x1020304050607080ULL,
        0x8070605040302010ULL,
    };
    const uint64_t marker_updates[] = {
        0xabcdef0123456789ULL,
        0x89abcdef76543210ULL,
        0x13572468ace0bdf1ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_inval_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(expected_word1,
                expected_word2);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL plus cross-16B load probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == expected_word1 &&
            warmed_word2 == expected_word2 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        const uint64_t cross16_observed =
            ai_memblock_cmo_cboinval_cross16_load_then_marker(inval_vaddr,
                cross16_load_vaddr, marker_vaddr, marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by a same-block cross-16B load releases inval ownership so the split-load merge refills current payload and marker progresses",
            cross16_observed == expected_cross16 &&
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL plus cross-16B load owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL plus cross-16B load owner-window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_seed(src_base_paddr,
                block_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_inval_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cbom_then_cross16_store_no_stale_cmo_flush_owner_or_partial_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xe00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xe00U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x31425364758697a8ULL,
    };
    const uint64_t head_updates[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8877665544332211ULL,
    };
    const uint64_t store_payloads[] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff10ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(seed_word1,
                store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM plus cross-16B store probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_base_vaddr, head_updates[round]);
        cbo_clean(src_base_vaddr);
        cbo_flush(src_tail_vaddr);
        ai_memblock_cmo_cross16_store_then_marker(cross16_store_vaddr,
            store_payloads[round], marker_vaddr, marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by a same-block cross-16B store keeps CMO flush ownership separate so only the split-store byte lanes overlay the seeded block",
            observed_src == head_updates[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM plus cross-16B store owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM plus cross-16B store owner-window contains the drained head store, precise split-store byte overlay, marker update, and unchanged guards",
            ai_memblock_cmo_block_matches_head_and_cross16_store(
                src_base_paddr,
                block_seeds[round],
                head_updates[round],
                store_payloads[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}
