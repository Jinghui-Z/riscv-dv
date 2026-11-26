#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS 3U

static inline void ai_memblock_scalar_store_then_lrsc_window(
    uintptr_t target_vaddr,
    uintptr_t pressure_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t pressure_value,
    uint64_t sc_value,
    uint64_t marker_value,
    uint64_t *lr_old,
    uint64_t *sc_status,
    uint64_t *pressure_observed,
    uint64_t *marker_observed)
{
    uint64_t old_value;
    uint64_t status_value;
    uint64_t pressure_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_value], 0(%[target_addr])\n\t"
        "sd %[pressure_value], 0(%[pressure_addr])\n\t"
        "lr.d %[old_value], (%[target_addr])\n\t"
        "sc.d %[status_value], %[sc_value], (%[target_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[pressure_readback], 0(%[pressure_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [status_value] "=&r"(status_value),
          [pressure_readback] "=&r"(pressure_readback),
          [marker_readback] "=&r"(marker_readback)
        : [target_addr] "r"(target_vaddr),
          [pressure_addr] "r"(pressure_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [store_value] "r"(store_value),
          [pressure_value] "r"(pressure_value),
          [sc_value] "r"(sc_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_old = old_value;
    *sc_status = status_value;
    *pressure_observed = pressure_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_scalar_store_then_lrsc_no_stale_sbuffer_or_reservation_owner_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page pressure_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x480U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x480U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x480U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x480U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x480U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x480U;
    const uintptr_t target_guard_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t pressure_guard_vaddr = pressure_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t target_guard_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t pressure_guard_paddr = pressure_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0101010102020202ULL,
        0x1111111122222222ULL,
        0x2121212132323232ULL,
    };
    const uint64_t store_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xa001a002a003a004ULL,
        0xb001b002b003b004ULL,
        0xc001c002c003c004ULL,
    };
    const uint64_t sc_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x4444555566667777ULL,
        0x5555666677778888ULL,
        0x6666777788889999ULL,
    };
    const uint64_t pressure_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0123456701234567ULL,
        0x1234567812345678ULL,
        0x2345678923456789ULL,
    };
    const uint64_t pressure_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xd0d1d2d3d4d5d6d7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xf0f1f2f3f4f5f6f7ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x89abcdef01234567ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t target_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xff00aa0055003300ULL,
        0xee11bb1166114411ULL,
        0xdd22cc2277225522ULL,
    };
    const uint64_t pressure_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0badc0de55aa33ccULL,
        0x1234fedc5678ba90ULL,
        0x55aa33cc7799bbddULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x8877aabbccddeeffULL,
        0x77668899aabbccddULL,
        0x6655778899aabbccULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS; ++round) {
        uint64_t lr_old = 0ULL;
        uint64_t sc_status = 1ULL;
        uint64_t pressure_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        write64(target_paddr, target_seeds[round]);
        write64(pressure_paddr, pressure_seeds[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(target_guard_paddr, target_guards[round]);
        write64(pressure_guard_paddr, pressure_guards[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the scalar-store then LR/SC owner-window starts, target/pressure/marker are readable with seeded values",
            ld(target_vaddr) == target_seeds[round] &&
            ld(pressure_vaddr) == pressure_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_scalar_store_then_lrsc_window(target_vaddr,
            pressure_vaddr, marker_vaddr, store_values[round],
            pressure_values[round], sc_values[round], marker_values[round],
            &lr_old, &sc_status, &pressure_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("ordinary scalar store immediately followed by LR/SC drains or forwards the store before LR and preserves reservation success",
            lr_old == store_values[round] &&
            sc_status == 0ULL &&
            pressure_observed == pressure_values[round] &&
            marker_observed == marker_values[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the scalar-store then LR/SC owner-window leaves the SC result, pressure store, marker, and all guards visible",
            ld(target_vaddr) == sc_values[round] &&
            ld(pressure_vaddr) == pressure_values[round] &&
            ld(marker_vaddr) == marker_values[round] &&
            ld(target_guard_vaddr) == target_guards[round] &&
            ld(pressure_guard_vaddr) == pressure_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar-store then LR/SC owner-window contains only the SC result, pressure store, marker update, and unchanged guards",
            read64(target_paddr) == sc_values[round] &&
            read64(pressure_paddr) == pressure_values[round] &&
            read64(marker_paddr) == marker_values[round] &&
            read64(target_guard_paddr) == target_guards[round] &&
            read64(pressure_guard_paddr) == pressure_guards[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_lrsc_then_scalar_store_window(
    uintptr_t lrsc_vaddr,
    uintptr_t store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t sc_value,
    uint64_t store_value,
    uint64_t marker_value,
    uint64_t *lr_old,
    uint64_t *sc_status,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t old_value;
    uint64_t status_value;
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[old_value], (%[lrsc_addr])\n\t"
        "sc.d %[status_value], %[sc_value], (%[lrsc_addr])\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [status_value] "=&r"(status_value),
          [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [lrsc_addr] "r"(lrsc_vaddr),
          [store_addr] "r"(store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [sc_value] "r"(sc_value),
          [store_value] "r"(store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_old = old_value;
    *sc_status = status_value;
    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_lrsc_then_scalar_store_no_stale_reservation_or_storequeue_progress_corner()
{
    TEST_START();

    const enum test_page lrsc_page = SWITCH1;
    const enum test_page store_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t lrsc_vaddr = hs_page_base(lrsc_page) + 0x580U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x580U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x580U;
    const uintptr_t lrsc_paddr = phys_page_base(lrsc_page) + 0x580U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x580U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x580U;
    const uintptr_t lrsc_guard_vaddr = lrsc_vaddr + sizeof(uint64_t);
    const uintptr_t store_guard_vaddr = store_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t lrsc_guard_paddr = lrsc_paddr + sizeof(uint64_t);
    const uintptr_t store_guard_paddr = store_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t lrsc_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x1020304050607080ULL,
        0x2030405060708090ULL,
        0x30405060708090a0ULL,
    };
    const uint64_t sc_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x7777666655554444ULL,
        0x8888777766665555ULL,
        0x9999888877776666ULL,
    };
    const uint64_t store_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1a1b1c1d1e1f2021ULL,
        0x2a2b2c2d2e2f3031ULL,
    };
    const uint64_t store_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xaabbccddeeff0011ULL,
        0xbbccddeeff001122ULL,
        0xccddeeff00112233ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x1357246813572468ULL,
        0x2468135724681357ULL,
        0x369cf258369cf258ULL,
    };
    const uint64_t marker_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xfeedfacecafebeefULL,
        0xdecafbad12345678ULL,
        0x0badf00d55aa33ccULL,
    };
    const uint64_t lrsc_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xfdb97531eca86420ULL,
        0x02468ace13579bdfULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t store_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0102030405060708ULL,
        0x1020304050607080ULL,
        0x8877665544332211ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS; ++round) {
        uint64_t lr_old = 0ULL;
        uint64_t sc_status = 1ULL;
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        write64(lrsc_paddr, lrsc_seeds[round]);
        write64(store_paddr, store_seeds[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(lrsc_guard_paddr, lrsc_guards[round]);
        write64(store_guard_paddr, store_guards[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the LR/SC then scalar-store owner-window starts, LRSC/store/marker targets are readable with seeded values",
            ld(lrsc_vaddr) == lrsc_seeds[round] &&
            ld(store_vaddr) == store_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_lrsc_then_scalar_store_window(lrsc_vaddr,
            store_vaddr, marker_vaddr, sc_values[round],
            store_values[round], marker_values[round], &lr_old,
            &sc_status, &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("LR/SC immediately followed by ordinary scalar store releases reservation ownership before the following store and marker progress",
            lr_old == lrsc_seeds[round] &&
            sc_status == 0ULL &&
            store_observed == store_values[round] &&
            marker_observed == marker_values[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the LR/SC then scalar-store owner-window leaves the SC result, independent store, marker, and all guards visible",
            ld(lrsc_vaddr) == sc_values[round] &&
            ld(store_vaddr) == store_values[round] &&
            ld(marker_vaddr) == marker_values[round] &&
            ld(lrsc_guard_vaddr) == lrsc_guards[round] &&
            ld(store_guard_vaddr) == store_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the LR/SC then scalar-store owner-window contains only the SC result, independent store, marker update, and unchanged guards",
            read64(lrsc_paddr) == sc_values[round] &&
            read64(store_paddr) == store_values[round] &&
            read64(marker_paddr) == marker_values[round] &&
            read64(lrsc_guard_paddr) == lrsc_guards[round] &&
            read64(store_guard_paddr) == store_guards[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_amo_then_scalar_store_window(
    uintptr_t amo_vaddr,
    uintptr_t store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t amo_addend,
    uint64_t store_value,
    uint64_t marker_value,
    uint64_t *amo_old,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t old_value;
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[old_value], %[amo_addend], (%[amo_addr])\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [amo_addr] "r"(amo_vaddr),
          [store_addr] "r"(store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [amo_addend] "r"(amo_addend),
          [store_value] "r"(store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *amo_old = old_value;
    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_amo_then_scalar_store_no_stale_atomic_flush_owner_or_storequeue_progress_corner()
{
    TEST_START();

    const enum test_page amo_page = SWITCH1;
    const enum test_page store_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0x680U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x680U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x680U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0x680U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x680U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x680U;
    const uintptr_t amo_guard_vaddr = amo_vaddr + sizeof(uint64_t);
    const uintptr_t store_guard_vaddr = store_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t amo_guard_paddr = amo_paddr + sizeof(uint64_t);
    const uintptr_t store_guard_paddr = store_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t amo_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x1223344556677880ULL,
        0x2334455667788990ULL,
        0x3445566778899aa0ULL,
    };
    const uint64_t amo_addends[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0000000000000105ULL,
        0x0000000000000206ULL,
        0x0000000000000307ULL,
    };
    const uint64_t store_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0011223344556677ULL,
        0x1122334455667788ULL,
        0x2233445566778899ULL,
    };
    const uint64_t store_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xdad1dad2dad3dad4ULL,
        0xeae1eae2eae3eae4ULL,
        0xfaf1faf2faf3faf4ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x445566778899aabbULL,
        0x5566778899aabbccULL,
        0x66778899aabbccddULL,
    };
    const uint64_t marker_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t amo_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x8081828384858687ULL,
        0x9091929394959697ULL,
        0xa0a1a2a3a4a5a6a7ULL,
    };
    const uint64_t store_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0706050403020100ULL,
        0x1716151413121110ULL,
        0x2726252423222120ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x8877aabbccddeeffULL,
        0x9988bbccddee0011ULL,
        0xaa99ccddeeff1122ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS; ++round) {
        uint64_t amo_old = 0ULL;
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t amo_final = amo_seeds[round] + amo_addends[round];

        goto_priv(PRIV_M);
        write64(amo_paddr, amo_seeds[round]);
        write64(store_paddr, store_seeds[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(amo_guard_paddr, amo_guards[round]);
        write64(store_guard_paddr, store_guards[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the AMO then scalar-store owner-window starts, AMO/store/marker targets are readable with seeded values",
            ld(amo_vaddr) == amo_seeds[round] &&
            ld(store_vaddr) == store_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_amo_then_scalar_store_window(amo_vaddr,
            store_vaddr, marker_vaddr, amo_addends[round],
            store_values[round], marker_values[round],
            &amo_old, &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("AMO immediately followed by ordinary scalar store releases atomic flush ownership before the following store and marker progress",
            amo_old == amo_seeds[round] &&
            store_observed == store_values[round] &&
            marker_observed == marker_values[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the AMO then scalar-store owner-window leaves the AMO result, independent store, marker, and all guards visible",
            ld(amo_vaddr) == amo_final &&
            ld(store_vaddr) == store_values[round] &&
            ld(marker_vaddr) == marker_values[round] &&
            ld(amo_guard_vaddr) == amo_guards[round] &&
            ld(store_guard_vaddr) == store_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the AMO then scalar-store owner-window contains only the AMO result, independent store, marker update, and unchanged guards",
            read64(amo_paddr) == amo_final &&
            read64(store_paddr) == store_values[round] &&
            read64(marker_paddr) == marker_values[round] &&
            read64(amo_guard_paddr) == amo_guards[round] &&
            read64(store_guard_paddr) == store_guards[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_memblock_scalar_store_then_amo_window(
    uintptr_t target_vaddr,
    uintptr_t pressure_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t pressure_value,
    uint64_t amo_addend,
    uint64_t marker_value,
    uint64_t *amo_old,
    uint64_t *pressure_observed,
    uint64_t *marker_observed)
{
    uint64_t old_value;
    uint64_t pressure_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_value], 0(%[target_addr])\n\t"
        "sd %[pressure_value], 0(%[pressure_addr])\n\t"
        "amoadd.d %[old_value], %[amo_addend], (%[target_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[pressure_readback], 0(%[pressure_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [pressure_readback] "=&r"(pressure_readback),
          [marker_readback] "=&r"(marker_readback)
        : [target_addr] "r"(target_vaddr),
          [pressure_addr] "r"(pressure_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [store_value] "r"(store_value),
          [pressure_value] "r"(pressure_value),
          [amo_addend] "r"(amo_addend),
          [marker_value] "r"(marker_value)
        : "memory");

    *amo_old = old_value;
    *pressure_observed = pressure_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_scalar_store_then_amo_no_stale_sbuffer_or_atomic_flush_owner_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page pressure_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x780U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x780U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x780U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x780U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x780U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x780U;
    const uintptr_t target_guard_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t pressure_guard_vaddr = pressure_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t target_guard_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t pressure_guard_paddr = pressure_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t store_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x7001700270037004ULL,
        0x8001800280038004ULL,
        0x9001900290039004ULL,
    };
    const uint64_t amo_addends[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0000000000000041ULL,
        0x0000000000000052ULL,
        0x0000000000000063ULL,
    };
    const uint64_t pressure_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t pressure_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xaa55aa55cc33cc33ULL,
        0xbb66bb66dd44dd44ULL,
        0xcc77cc77ee55ee55ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x0f0e0d0c0b0a0908ULL,
        0x1f1e1d1c1b1a1918ULL,
        0x2f2e2d2c2b2a2928ULL,
    };
    const uint64_t marker_values[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
        0x3456789abcdef012ULL,
    };
    const uint64_t target_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
    };
    const uint64_t pressure_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_SCALAR_ATOMIC_ROUNDS; ++round) {
        uint64_t amo_old = 0ULL;
        uint64_t pressure_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t amo_final = store_values[round] + amo_addends[round];

        goto_priv(PRIV_M);
        write64(target_paddr, target_seeds[round]);
        write64(pressure_paddr, pressure_seeds[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(target_guard_paddr, target_guards[round]);
        write64(pressure_guard_paddr, pressure_guards[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the scalar-store then AMO owner-window starts, target/pressure/marker are readable with seeded values",
            ld(target_vaddr) == target_seeds[round] &&
            ld(pressure_vaddr) == pressure_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_scalar_store_then_amo_window(target_vaddr,
            pressure_vaddr, marker_vaddr, store_values[round],
            pressure_values[round], amo_addends[round], marker_values[round],
            &amo_old, &pressure_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("ordinary scalar store immediately followed by AMO drains or forwards the store before the atomic old value is captured",
            amo_old == store_values[round] &&
            pressure_observed == pressure_values[round] &&
            marker_observed == marker_values[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the scalar-store then AMO owner-window leaves the AMO result, pressure store, marker, and all guards visible",
            ld(target_vaddr) == amo_final &&
            ld(pressure_vaddr) == pressure_values[round] &&
            ld(marker_vaddr) == marker_values[round] &&
            ld(target_guard_vaddr) == target_guards[round] &&
            ld(pressure_guard_vaddr) == pressure_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar-store then AMO owner-window contains only the AMO result, pressure store, marker update, and unchanged guards",
            read64(target_paddr) == amo_final &&
            read64(pressure_paddr) == pressure_values[round] &&
            read64(marker_paddr) == marker_values[round] &&
            read64(target_guard_paddr) == target_guards[round] &&
            read64(pressure_guard_paddr) == pressure_guards[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
