#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_LDAOUT_ROUNDS 3U

static inline void ai_memblock_ldaout_amo_then_scalar_load_window(
    uintptr_t amo_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t amo_addend,
    uint64_t marker_value,
    uint64_t *amo_old,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t old_value;
    uint64_t loaded_value;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[old_value], %[amo_addend], (%[amo_addr])\n\t"
        "ld %[loaded_value], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [loaded_value] "=&r"(loaded_value),
          [marker_readback] "=&r"(marker_readback)
        : [amo_addr] "r"(amo_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [amo_addend] "r"(amo_addend),
          [marker_value] "r"(marker_value)
        : "memory");

    *amo_old = old_value;
    *load_observed = loaded_value;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_amo_then_scalar_load_no_stale_atomic_ldaout_or_load_payload_corner()
{
    TEST_START();

    const enum test_page amo_page = SWITCH1;
    const enum test_page load_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0x180U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x180U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x180U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0x180U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x180U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x180U;
    const uintptr_t amo_guard_vaddr = amo_vaddr + sizeof(uint64_t);
    const uintptr_t load_guard_vaddr = load_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t amo_guard_paddr = amo_paddr + sizeof(uint64_t);
    const uintptr_t load_guard_paddr = load_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t amo_seeds[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x1122334455667700ULL,
        0x2233445566778810ULL,
        0x3344556677889920ULL,
    };
    const uint64_t amo_addends[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x0000000000000123ULL,
        0x0000000000000234ULL,
        0x0000000000000345ULL,
    };
    const uint64_t load_values[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0xa5a55a5adeadbeefULL,
        0xb6b66b6bcafef00dULL,
        0xc7c77c7c13579bdfULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x0102030405060708ULL,
        0x1020304050607080ULL,
        0x8877665544332211ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0xfeedfacecafebeefULL,
        0xdecafbad12345678ULL,
        0x0badf00d55aa33ccULL,
    };
    const uint64_t amo_guards[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t load_guards[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0xaabbccddeeff0011ULL,
        0xbbccddeeff001122ULL,
        0xccddeeff00112233ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_LDAOUT_ROUNDS; ++round) {
        uint64_t amo_old = 0ULL;
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t amo_final = amo_seeds[round] + amo_addends[round];

        goto_priv(PRIV_M);
        write64(amo_paddr, amo_seeds[round]);
        write64(load_paddr, load_values[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(amo_guard_paddr, amo_guards[round]);
        write64(load_guard_paddr, load_guards[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the AMO then scalar-load ldaOut handoff starts, AMO/load/marker targets are readable with seeded values",
            ld(amo_vaddr) == amo_seeds[round] &&
            ld(load_vaddr) == load_values[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_ldaout_amo_then_scalar_load_window(amo_vaddr,
            load_vaddr, marker_vaddr, amo_addends[round],
            marker_updates[round], &amo_old, &load_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("AMO immediately followed by scalar load returns the AMO old value and the independent load payload without ldaOut cross-binding",
            amo_old == amo_seeds[round] &&
            load_observed == load_values[round] &&
            load_observed != amo_seeds[round] &&
            load_observed != amo_final &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the AMO then scalar-load ldaOut handoff leaves both targets and all guards visible with fresh payloads",
            ld(amo_vaddr) == amo_final &&
            ld(load_vaddr) == load_values[round] &&
            ld(marker_vaddr) == marker_updates[round] &&
            ld(amo_guard_vaddr) == amo_guards[round] &&
            ld(load_guard_vaddr) == load_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the AMO then scalar-load ldaOut handoff contains only the AMO update, independent load seed, marker update, and unchanged guards",
            read64(amo_paddr) == amo_final &&
            read64(load_paddr) == load_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(amo_guard_paddr) == amo_guards[round] &&
            read64(load_guard_paddr) == load_guards[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_ldaout_lrsc_then_scalar_load_window(
    uintptr_t lrsc_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t sc_value,
    uint64_t marker_value,
    uint64_t *lr_old,
    uint64_t *sc_status,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t old_value;
    uint64_t status_value;
    uint64_t loaded_value;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[old_value], (%[lrsc_addr])\n\t"
        "sc.d %[status_value], %[sc_value], (%[lrsc_addr])\n\t"
        "ld %[loaded_value], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [status_value] "=&r"(status_value),
          [loaded_value] "=&r"(loaded_value),
          [marker_readback] "=&r"(marker_readback)
        : [lrsc_addr] "r"(lrsc_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [sc_value] "r"(sc_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_old = old_value;
    *sc_status = status_value;
    *load_observed = loaded_value;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_lrsc_then_scalar_load_no_stale_reservation_ldaout_or_load_payload_corner()
{
    TEST_START();

    const enum test_page lrsc_page = SWITCH1;
    const enum test_page load_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t lrsc_vaddr = hs_page_base(lrsc_page) + 0x280U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x280U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x280U;
    const uintptr_t lrsc_paddr = phys_page_base(lrsc_page) + 0x280U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x280U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x280U;
    const uintptr_t lrsc_guard_vaddr = lrsc_vaddr + sizeof(uint64_t);
    const uintptr_t load_guard_vaddr = load_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t lrsc_guard_paddr = lrsc_paddr + sizeof(uint64_t);
    const uintptr_t load_guard_paddr = load_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t lrsc_seeds[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x456789abcdef0011ULL,
        0x56789abcdef00122ULL,
        0x6789abcdef001233ULL,
    };
    const uint64_t sc_values[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x1111222233334444ULL,
        0x2222333344445555ULL,
        0x3333444455556666ULL,
    };
    const uint64_t load_values[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0xd0d1d2d3d4d5d6d7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xf0f1f2f3f4f5f6f7ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x89abcdef01234567ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x0123456789abcdefULL,
        0x89abcdef76543210ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t lrsc_guards[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x0f0e0d0c0b0a0908ULL,
        0x1f1e1d1c1b1a1918ULL,
        0x2f2e2d2c2b2a2928ULL,
    };
    const uint64_t load_guards[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x8081828384858687ULL,
        0x9091929394959697ULL,
        0xa0a1a2a3a4a5a6a7ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x8877aabbccddeeffULL,
        0x77668899aabbccddULL,
        0x6655778899aabbccULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_LDAOUT_ROUNDS; ++round) {
        uint64_t lr_old = 0ULL;
        uint64_t sc_status = 1ULL;
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        write64(lrsc_paddr, lrsc_seeds[round]);
        write64(load_paddr, load_values[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(lrsc_guard_paddr, lrsc_guards[round]);
        write64(load_guard_paddr, load_guards[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the LR/SC then scalar-load ldaOut handoff starts, LRSC/load/marker targets are readable with seeded values",
            ld(lrsc_vaddr) == lrsc_seeds[round] &&
            ld(load_vaddr) == load_values[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_ldaout_lrsc_then_scalar_load_window(lrsc_vaddr,
            load_vaddr, marker_vaddr, sc_values[round],
            marker_updates[round], &lr_old, &sc_status,
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("LR/SC immediately followed by scalar load returns the LR old value, successful SC status, and independent load payload without ldaOut cross-binding",
            lr_old == lrsc_seeds[round] &&
            sc_status == 0ULL &&
            load_observed == load_values[round] &&
            load_observed != lrsc_seeds[round] &&
            load_observed != sc_values[round] &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the LR/SC then scalar-load ldaOut handoff leaves the SC result, independent load target, marker, and all guards visible",
            ld(lrsc_vaddr) == sc_values[round] &&
            ld(load_vaddr) == load_values[round] &&
            ld(marker_vaddr) == marker_updates[round] &&
            ld(lrsc_guard_vaddr) == lrsc_guards[round] &&
            ld(load_guard_vaddr) == load_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the LR/SC then scalar-load ldaOut handoff contains only the SC update, independent load seed, marker update, and unchanged guards",
            read64(lrsc_paddr) == sc_values[round] &&
            read64(load_paddr) == load_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(lrsc_guard_paddr) == lrsc_guards[round] &&
            read64(load_guard_paddr) == load_guards[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_ldaout_scalar_load_then_lrsc_window(
    uintptr_t load_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t marker_vaddr,
    uint64_t sc_value,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *lr_old,
    uint64_t *sc_status,
    uint64_t *marker_observed)
{
    uint64_t loaded_value;
    uint64_t old_value;
    uint64_t status_value;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[loaded_value], 0(%[load_addr])\n\t"
        "lr.d %[old_value], (%[lrsc_addr])\n\t"
        "sc.d %[status_value], %[sc_value], (%[lrsc_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [loaded_value] "=&r"(loaded_value),
          [old_value] "=&r"(old_value),
          [status_value] "=&r"(status_value),
          [marker_readback] "=&r"(marker_readback)
        : [load_addr] "r"(load_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [sc_value] "r"(sc_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = loaded_value;
    *lr_old = old_value;
    *sc_status = status_value;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_scalar_load_then_lrsc_no_lost_load0_or_stale_reservation_ldaout_corner()
{
    TEST_START();

    const enum test_page load_page = SWITCH1;
    const enum test_page lrsc_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x380U;
    const uintptr_t lrsc_vaddr = hs_page_base(lrsc_page) + 0x380U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x380U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x380U;
    const uintptr_t lrsc_paddr = phys_page_base(lrsc_page) + 0x380U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x380U;
    const uintptr_t load_guard_vaddr = load_vaddr + sizeof(uint64_t);
    const uintptr_t lrsc_guard_vaddr = lrsc_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t load_guard_paddr = load_paddr + sizeof(uint64_t);
    const uintptr_t lrsc_guard_paddr = lrsc_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t load_values[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1a1b1c1d1e1f2021ULL,
        0x2a2b2c2d2e2f3031ULL,
    };
    const uint64_t lrsc_seeds[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0xbaadf00d01234567ULL,
        0xc001d00d89abcdefULL,
        0xd15ea5ed10293847ULL,
    };
    const uint64_t sc_values[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x4444333322221111ULL,
        0x5555444433332222ULL,
        0x6666555544443333ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x1111111122222222ULL,
        0x3333333344444444ULL,
        0x5555555566666666ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0xabcdef9876543210ULL,
        0x1234567890abcdefULL,
        0xfedcba0987654321ULL,
    };
    const uint64_t load_guards[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x1357246813572468ULL,
        0x2468135724681357ULL,
        0x369cf258369cf258ULL,
    };
    const uint64_t lrsc_guards[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_LDAOUT_ROUNDS] = {
        0xffeeddccbbaa9988ULL,
        0xeeddccbbaa998877ULL,
        0xddccbbaa99887766ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_LDAOUT_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t lr_old = 0ULL;
        uint64_t sc_status = 1ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        write64(load_paddr, load_values[round]);
        write64(lrsc_paddr, lrsc_seeds[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(load_guard_paddr, load_guards[round]);
        write64(lrsc_guard_paddr, lrsc_guards[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the scalar-load then LR/SC ldaOut handoff starts, load/LRSC/marker targets are readable with seeded values",
            ld(load_vaddr) == load_values[round] &&
            ld(lrsc_vaddr) == lrsc_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_ldaout_scalar_load_then_lrsc_window(load_vaddr,
            lrsc_vaddr, marker_vaddr, sc_values[round],
            marker_updates[round], &load_observed, &lr_old,
            &sc_status, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("scalar load immediately followed by LR/SC preserves the load0 payload and uses fresh reservation payload on the shared ldaOut surface",
            load_observed == load_values[round] &&
            load_observed != lrsc_seeds[round] &&
            load_observed != sc_values[round] &&
            lr_old == lrsc_seeds[round] &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the scalar-load then LR/SC ldaOut handoff leaves the independent load target, SC result, marker, and all guards visible",
            ld(load_vaddr) == load_values[round] &&
            ld(lrsc_vaddr) == sc_values[round] &&
            ld(marker_vaddr) == marker_updates[round] &&
            ld(load_guard_vaddr) == load_guards[round] &&
            ld(lrsc_guard_vaddr) == lrsc_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar-load then LR/SC ldaOut handoff contains only the independent load seed, SC update, marker update, and unchanged guards",
            read64(load_paddr) == load_values[round] &&
            read64(lrsc_paddr) == sc_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(load_guard_paddr) == load_guards[round] &&
            read64(lrsc_guard_paddr) == lrsc_guards[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
