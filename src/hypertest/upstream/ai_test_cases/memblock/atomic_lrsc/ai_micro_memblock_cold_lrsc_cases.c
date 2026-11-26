#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <stddef.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline void ai_micro_cold_lrsc_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_cold_lrsc_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_cold_lrsc_previous_address_guard_window(
    uintptr_t old_vaddr,
    uintptr_t cold_vaddr,
    uint64_t stale_old_payload,
    uint64_t fresh_cold_payload,
    uint64_t *old_lr_observed,
    uint64_t *cold_lr_observed,
    uint64_t *stale_old_sc_status,
    uint64_t *fresh_cold_lr_observed,
    uint64_t *fresh_cold_sc_status,
    uint64_t *old_after,
    uint64_t *cold_after)
{
    uint64_t old_lr = 0ULL;
    uint64_t cold_lr = 0ULL;
    uint64_t stale_status = 0ULL;
    uint64_t fresh_lr = 0ULL;
    uint64_t fresh_status = 0ULL;
    uint64_t old_readback = 0ULL;
    uint64_t cold_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[cold_addr], 0\n\t"
        "fence rw, rw\n\t"
        "lr.d %[old_lr], 0(%[old_addr])\n\t"
        "lr.d %[cold_lr], 0(%[cold_addr])\n\t"
        "sc.d %[stale_status], %[stale_payload], 0(%[old_addr])\n\t"
        "lr.d %[fresh_lr], 0(%[cold_addr])\n\t"
        "sc.d %[fresh_status], %[fresh_payload], 0(%[cold_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[old_readback], 0(%[old_addr])\n\t"
        "ld %[cold_readback], 0(%[cold_addr])\n\t"
        ".option pop\n\t"
        : [old_lr] "=&r"(old_lr),
          [cold_lr] "=&r"(cold_lr),
          [stale_status] "=&r"(stale_status),
          [fresh_lr] "=&r"(fresh_lr),
          [fresh_status] "=&r"(fresh_status),
          [old_readback] "=&r"(old_readback),
          [cold_readback] "=&r"(cold_readback)
        : [old_addr] "r"(old_vaddr),
          [cold_addr] "r"(cold_vaddr),
          [stale_payload] "r"(stale_old_payload),
          [fresh_payload] "r"(fresh_cold_payload)
        : "memory");

    *old_lr_observed = old_lr;
    *cold_lr_observed = cold_lr;
    *stale_old_sc_status = stale_status;
    *fresh_cold_lr_observed = fresh_lr;
    *fresh_cold_sc_status = fresh_status;
    *old_after = old_readback;
    *cold_after = cold_readback;
}

bool ai_micro_hs_cold_lr_refill_replaces_previous_full_address_reservation_corner()
{
    TEST_START();

    const enum test_page old_page = SWITCH1;
    const enum test_page cold_page = SWITCH2;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x540U;
    const uintptr_t cold_vaddr = hs_page_base(cold_page) + 0x540U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x540U;
    const uintptr_t cold_paddr = phys_page_base(cold_page) + 0x540U;
    const uintptr_t old_guard_vaddr = old_vaddr + sizeof(uint64_t);
    const uintptr_t cold_guard_vaddr = cold_vaddr + sizeof(uint64_t);
    const uintptr_t old_guard_paddr = old_paddr + sizeof(uint64_t);
    const uintptr_t cold_guard_paddr = cold_paddr + sizeof(uint64_t);
    const uint64_t old_seeds[] = {
        0x1029384756abcdefULL,
        0x21436587a9cbed0fULL,
        0x89abcdef76543210ULL,
    };
    const uint64_t cold_seeds[] = {
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t stale_old_payloads[] = {
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t fresh_cold_payloads[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t old_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t cold_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };

    ai_micro_cold_lrsc_prepare_hs_env();

    for (size_t round = 0; round < sizeof(old_seeds) / sizeof(old_seeds[0]);
         ++round) {
        uint64_t old_lr_observed = 0ULL;
        uint64_t cold_lr_observed = 0ULL;
        uint64_t stale_old_status = 0ULL;
        uint64_t fresh_cold_lr_observed = 0ULL;
        uint64_t fresh_cold_status = 0ULL;
        uint64_t old_after = 0ULL;
        uint64_t cold_after = 0ULL;

        goto_priv(PRIV_M);
        write64(old_paddr, old_seeds[round]);
        write64(cold_paddr, cold_seeds[round]);
        write64(old_guard_paddr, old_guards[round]);
        write64(cold_guard_paddr, cold_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the cold-LR full-address replacement window, both old and cold LR/SC lines are readable with distinct guard payloads",
            ld(old_vaddr) == old_seeds[round] &&
            ld(cold_vaddr) == cold_seeds[round] &&
            ld(old_guard_vaddr) == old_guards[round] &&
            ld(cold_guard_vaddr) == cold_guards[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_cold_lrsc_previous_address_guard_window(old_vaddr,
            cold_vaddr, stale_old_payloads[round],
            fresh_cold_payloads[round], &old_lr_observed,
            &cold_lr_observed, &stale_old_status,
            &fresh_cold_lr_observed, &fresh_cold_status,
            &old_after, &cold_after);
        AI_ASSERT_NO_EXCEPT_AND("after cbo.inval prepares B cold, LR.A followed by cold LR.B makes a naked SC.A fail and leave A unchanged, proving the cold LR replaced the previous full-address reservation",
            old_lr_observed == old_seeds[round] &&
            cold_lr_observed == cold_seeds[round] &&
            stale_old_status != 0ULL &&
            old_after == old_seeds[round]
        );
        AI_ASSERT_NO_EXCEPT_AND("after the stale-address SC fails, a fresh LR.B/SC.B still succeeds so the guard distinguishes stale lrsc_addr from a globally broken LR/SC path",
            fresh_cold_lr_observed == cold_seeds[round] &&
            fresh_cold_status == 0ULL &&
            cold_after == fresh_cold_payloads[round] &&
            ld(cold_guard_vaddr) == cold_guards[round] &&
            ld(old_guard_vaddr) == old_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cold-LR previous-address guard contains no stale SC.A write and exactly one fresh SC.B payload",
            read64(old_paddr) == old_seeds[round] &&
            read64(cold_paddr) == fresh_cold_payloads[round] &&
            read64(old_guard_paddr) == old_guards[round] &&
            read64(cold_guard_paddr) == cold_guards[round]
        );
    }

    ai_micro_cold_lrsc_reset_hs_env();

    TEST_END();
}
