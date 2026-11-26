#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_AP74_AP76_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_AP74_AP76_L0_SECTOR_PAGES 8U
#define AI_AP74_AP76_STREAM_REPS 6U

static inline void ai_ap74_ap76_seed_word(enum test_page page,
    uintptr_t off, uint64_t value)
{
    goto_priv(PRIV_M);
    write64(phys_page_base(page) + off, value);
}

static inline uint64_t ai_ap74_ap76_ld64(uintptr_t addr)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "=&r"(value) : "r"(addr) : "memory");
    return value;
}

static inline void ai_ap74_ap76_sd64(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %1, 0(%0)\n\t"
        ".option pop\n\t"
        :: "r"(addr), "r"(value) : "memory");
}

static inline void ai_ap74_ap76_prepare_hs(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_SUM);
}

static inline void ai_ap74_ap76_prepare_vs(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    hspt_init();
    hpt_init();
    vspt_init();
    sfence_vma();
    hfence();
    goto_priv(PRIV_VS);
}

static inline uint64_t ai_ap74_ap76_load_stream(const uintptr_t *vaddrs,
    const uint64_t *expected, unsigned count, unsigned reps)
{
    uint64_t mix = 0x9e3779b97f4a7c15ULL;

    for (unsigned rep = 0; rep < reps; ++rep) {
        for (unsigned i = 0; i < count; ++i) {
            uint64_t value = ai_ap74_ap76_ld64(vaddrs[i]);
            mix ^= value + ((uint64_t)(rep + 1U) << 48) +
                ((uint64_t)(i + 1U) << 40);
            mix = (mix << 7) | (mix >> 57);
            mix ^= expected[i];
        }
    }

    return mix;
}

static inline uint64_t ai_ap74_ap76_expected_stream_mix(
    const uint64_t *expected, unsigned count, unsigned reps)
{
    uint64_t mix = 0x9e3779b97f4a7c15ULL;

    for (unsigned rep = 0; rep < reps; ++rep) {
        for (unsigned i = 0; i < count; ++i) {
            uint64_t value = expected[i];
            mix ^= value + ((uint64_t)(rep + 1U) << 48) +
                ((uint64_t)(i + 1U) << 40);
            mix = (mix << 7) | (mix >> 57);
            mix ^= expected[i];
        }
    }

    return mix;
}

/*
 * AP-74A and AP-76A/B manual probe.
 *
 * Pages 0/1/2 sit in the same PTW L0 sector when PtwL0SectorSize is eight
 * PTEs, so their L2TlbPrefetch next_line values should be duplicate-filtered
 * while a pending prefetch output is held. LinkNan/RTL should observe:
 *   io.in.valid pulses for the three demand walks;
 *   only one accepted input_valid for the duplicate next_line while v is held;
 *   next_req and io.out.bits.req_info.vpn stable when input_valid is false.
 */
bool ai_micro_ap74_l2tlb_prefetch_pending_duplicate_manual_corner()
{
    TEST_START();

    const enum test_page page0 = VSRWX_GRWX;
    const enum test_page page1 = VSRWX_GRW;
    const enum test_page page2 = VSRWX_GRX;
    const enum test_page guard_page = VSRWX_GR;
    const uintptr_t off = 0x180U;
    const uintptr_t guard_off = 0x220U;
    const uintptr_t vaddrs[] = {
        hs_page_base(page0) + off,
        hs_page_base(page1) + off,
        hs_page_base(page2) + off,
    };
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + guard_off;
    const uint64_t expected[] = {
        0x7400000000000001ULL,
        0x7400000000000002ULL,
        0x7400000000000003ULL,
    };
    const uint64_t guard = 0x74aa55aa55aa0001ULL;
    const uint64_t expected_mix = ai_ap74_ap76_expected_stream_mix(expected,
        sizeof(expected) / sizeof(expected[0]), AI_AP74_AP76_STREAM_REPS);

    ai_ap74_ap76_seed_word(page0, off, expected[0]);
    ai_ap74_ap76_seed_word(page1, off, expected[1]);
    ai_ap74_ap76_seed_word(page2, off, expected[2]);
    ai_ap74_ap76_seed_word(guard_page, guard_off, guard);

    ai_ap74_ap76_prepare_hs();

    TEST_SETUP_EXCEPT();
    uint64_t mix = ai_ap74_ap76_load_stream(vaddrs, expected,
        sizeof(expected) / sizeof(expected[0]), AI_AP74_AP76_STREAM_REPS);
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-74 pending duplicate probe drives several same-L0-sector HS demand walks without changing architectural data",
        mix == expected_mix
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_value = ai_ap74_ap76_ld64(guard_vaddr);
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-74 pending duplicate probe leaves the adjacent guard translation readable after the duplicate next_line stream",
        guard_value == guard
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-74 pending duplicate probe leaves all same-sector backing words unchanged",
        read64(phys_page_base(page0) + off) == expected[0] &&
        read64(phys_page_base(page1) + off) == expected[1] &&
        read64(phys_page_base(page2) + off) == expected[2] &&
        read64(phys_page_base(guard_page) + guard_off) == guard
    );

    TEST_END();
}

/*
 * AP-74B/C and AP-76C/D manual probe.
 *
 * The first two accesses target a duplicate next_line pair. The third access
 * jumps by one eight-page sector, so same-cycle fire plus different input is
 * allowed to hand off cleanly. RTL evidence should show X fires once, a
 * duplicate X input is rejected, and the non-duplicate Y input is accepted
 * without mixing the fired X payload with the next transaction.
 */
bool ai_micro_ap74_l2tlb_prefetch_fire_handoff_manual_corner()
{
    TEST_START();

    const enum test_page dup0_page = VSRWX_GX;
    const enum test_page dup1_page = VSRW_GRWX;
    const enum test_page diff_page = VSRX_GURWX;
    const enum test_page store_page = VSRW_GURWX;
    const uintptr_t off = 0x2c0U;
    const uintptr_t store_off = 0x300U;
    const uintptr_t dup0_vaddr = hs_page_base(dup0_page) + off;
    const uintptr_t dup1_vaddr = hs_page_base(dup1_page) + off;
    const uintptr_t diff_vaddr = hs_page_base(diff_page) + off;
    const uintptr_t store_vaddr = hs_page_base(store_page) + store_off;
    const uint64_t dup0 = 0x74b0000000000000ULL;
    const uint64_t dup1 = 0x74b0000000000001ULL;
    const uint64_t diff = 0x74b0000000000008ULL;
    const uint64_t store_initial = 0x74b05eed00000000ULL;
    const uint64_t store_update = 0x74b0feed00000001ULL;

    ai_ap74_ap76_seed_word(dup0_page, off, dup0);
    ai_ap74_ap76_seed_word(dup1_page, off, dup1);
    ai_ap74_ap76_seed_word(diff_page, off, diff);
    ai_ap74_ap76_seed_word(store_page, store_off, store_initial);

    ai_ap74_ap76_prepare_hs();

    TEST_SETUP_EXCEPT();
    uint64_t observed_dup0 = ai_ap74_ap76_ld64(dup0_vaddr);
    uint64_t observed_dup1 = ai_ap74_ap76_ld64(dup1_vaddr);
    uint64_t observed_diff = ai_ap74_ap76_ld64(diff_vaddr);
    ai_ap74_ap76_sd64(store_vaddr, store_update);
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-74 fire handoff probe issues duplicate-sector then different-sector HS walks and reaches a later store",
        observed_dup0 == dup0 &&
        observed_dup1 == dup1 &&
        observed_diff == diff
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-74 fire handoff probe drains the explicit load/store sequence before memory image checking",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-74 fire handoff probe updates only the explicit store target and preserves both duplicate and different-sector load data",
        read64(phys_page_base(dup0_page) + off) == dup0 &&
        read64(phys_page_base(dup1_page) + off) == dup1 &&
        read64(phys_page_base(diff_page) + off) == diff &&
        read64(phys_page_base(store_page) + store_off) == store_update
    );

    TEST_END();
}

/*
 * AP-74D/E and AP-74G manual probe.
 *
 * The first loop fills the four-entry old_reqs history with fired L2TLB
 * prefetch outputs. A later duplicate in the first sector should be filtered by
 * old_reqs while the record is live. The sfence/satp sequence then clears both
 * pending and historical duplicate state; following accesses must make forward
 * progress in the refreshed context and leave data unchanged.
 */
bool ai_micro_ap74_l2tlb_prefetch_history_flush_manual_corner()
{
    TEST_START();

    const enum test_page sector_pages[] = {
        VSRWX_GRWX,
        VSRW_GR,
        VSR_GRW,
        VSURWX_GRWX,
    };
    const enum test_page duplicate_page = VSRWX_GRW;
    const enum test_page post_flush_page = VSURW_GRWX;
    const uintptr_t off = 0x3c0U;
    uintptr_t vaddrs[sizeof(sector_pages) / sizeof(sector_pages[0])] = {0};
    uint64_t expected[sizeof(sector_pages) / sizeof(sector_pages[0])] = {0};
    const uint64_t duplicate_value = 0x74d0000000000001ULL;
    const uint64_t post_flush_value = 0x74d0000000000040ULL;

    for (unsigned i = 0; i < sizeof(sector_pages) / sizeof(sector_pages[0]);
        ++i) {
        vaddrs[i] = hs_page_base(sector_pages[i]) + off;
        expected[i] = 0x74d0000000000010ULL + i;
        ai_ap74_ap76_seed_word(sector_pages[i], off, expected[i]);
    }
    ai_ap74_ap76_seed_word(duplicate_page, off, duplicate_value);
    ai_ap74_ap76_seed_word(post_flush_page, off, post_flush_value);

    ai_ap74_ap76_prepare_hs();

    TEST_SETUP_EXCEPT();
    uint64_t mix = ai_ap74_ap76_load_stream(vaddrs, expected,
        sizeof(sector_pages) / sizeof(sector_pages[0]), 2U);
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-74 history probe first fires four different L0-sector demand walks to populate old_reqs",
        mix == ai_ap74_ap76_expected_stream_mix(expected,
            sizeof(sector_pages) / sizeof(sector_pages[0]), 2U)
    );

    TEST_SETUP_EXCEPT();
    uint64_t duplicate = ai_ap74_ap76_ld64(hs_page_base(duplicate_page) + off);
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-74 history probe then revisits a duplicate next_line while old_reqs should still suppress repeated prefetch output",
        duplicate == duplicate_value
    );

    goto_priv(PRIV_M);
    uint64_t satp_before = CSRR(CSR_SATP);
    CSRW(CSR_SATP, satp_before ^ (1ULL << 44));
    sfence_vma();
    uint64_t satp_after = CSRR(CSR_SATP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t after_flush =
        ai_ap74_ap76_ld64(hs_page_base(post_flush_page) + off);
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-74 flush probe confirms the post-sfence translation stream still accepts a fresh prefetch candidate after history is cleared",
        after_flush == post_flush_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-74 history/flush probe changes only satp ASID state and leaves all backing payloads intact",
        ((satp_before ^ satp_after) & (0xffffULL << 44)) != 0 &&
        read64(phys_page_base(duplicate_page) + off) == duplicate_value &&
        read64(phys_page_base(post_flush_page) + off) == post_flush_value
    );

    TEST_END();
}

/*
 * AP-76A/B/G manual probe.
 *
 * This stream keeps producing raw prefetch candidates through repeated demand
 * loads. In LinkNan, correlate the raw candidate stream against
 * l2tlb_prefetch_input_count,
 * l2tlb_prefetch_valid_count, and L2TlbPrefetch_hart*. A broken AP-76 design
 * can show next_req changing on raw io.in.valid while output valid is held.
 */
bool ai_micro_ap76_l2tlb_prefetch_valid_hold_payload_manual_corner()
{
    TEST_START();

    const enum test_page page_a = VSR_GRWX;
    const enum test_page page_b = VSR_GURWX;
    const enum test_page page_c = VSRX_GRWX;
    const enum test_page marker_page = VSRW_GRWX;
    const uintptr_t off_a = 0x480U;
    const uintptr_t off_b = 0x500U;
    const uintptr_t off_c = 0x580U;
    const uintptr_t marker_off = 0x600U;
    const uintptr_t vaddrs[] = {
        hs_page_base(page_a) + off_a,
        hs_page_base(page_b) + off_b,
        hs_page_base(page_c) + off_c,
        hs_page_base(page_a) + off_a,
        hs_page_base(page_b) + off_b,
        hs_page_base(page_c) + off_c,
    };
    const uint64_t expected[] = {
        0x76a000000000000aULL,
        0x76a000000000000bULL,
        0x76a000000000000cULL,
        0x76a000000000000aULL,
        0x76a000000000000bULL,
        0x76a000000000000cULL,
    };
    const uint64_t marker_initial = 0x76a05eed00000000ULL;
    const uint64_t marker_update = 0x76a0feed00000001ULL;

    ai_ap74_ap76_seed_word(page_a, off_a, expected[0]);
    ai_ap74_ap76_seed_word(page_b, off_b, expected[1]);
    ai_ap74_ap76_seed_word(page_c, off_c, expected[2]);
    ai_ap74_ap76_seed_word(marker_page, marker_off, marker_initial);

    ai_ap74_ap76_prepare_hs();

    TEST_SETUP_EXCEPT();
    uint64_t mix = ai_ap74_ap76_load_stream(vaddrs, expected,
        sizeof(vaddrs) / sizeof(vaddrs[0]), AI_AP74_AP76_STREAM_REPS);
    ai_ap74_ap76_sd64(hs_page_base(marker_page) + marker_off, marker_update);
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-76 valid-hold probe drives raw prefetch candidates while architectural loads keep returning the accepted payloads",
        mix == ai_ap74_ap76_expected_stream_mix(expected,
            sizeof(expected) / sizeof(expected[0]), AI_AP74_AP76_STREAM_REPS)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-76 valid-hold probe commits only the explicit marker store and preserves all candidate backing words",
        read64(phys_page_base(page_a) + off_a) == expected[0] &&
        read64(phys_page_base(page_b) + off_b) == expected[1] &&
        read64(phys_page_base(page_c) + off_c) == expected[2] &&
        read64(phys_page_base(marker_page) + marker_off) == marker_update
    );

    TEST_END();
}

/*
 * AP-76E/F and AP-74G manual probe.
 *
 * A HS pending-prefetch window is followed by an explicit sfence/satp.changed
 * flush, then a two-stage VS window. RTL should not emit a stale HS VPN with
 * a new s2xlate value; the pending HS request must be flushed and later VS
 * outputs should be accepted only under the new context.
 */
bool ai_micro_ap76_l2tlb_prefetch_context_flush_manual_corner()
{
    TEST_START();

    const enum test_page hs_page = VSR_GRWX;
    const enum test_page vs_page0 = VSRWX_GRWX;
    const enum test_page vs_page1 = VSRWX_GRW;
    const uintptr_t hs_off = 0x680U;
    const uintptr_t vs_off0 = 0x700U;
    const uintptr_t vs_off1 = 0x780U;
    const uintptr_t hs_vaddr = hs_page_base(hs_page) + hs_off;
    const uintptr_t vs_vaddr0 = vs_page_base(vs_page0) + vs_off0;
    const uintptr_t vs_vaddr1 = vs_page_base(vs_page1) + vs_off1;
    const uint64_t hs_value = 0x76e0000000000001ULL;
    const uint64_t vs_value0 = 0x76e0000000000002ULL;
    const uint64_t vs_value1 = 0x76e0000000000003ULL;

    ai_ap74_ap76_seed_word(hs_page, hs_off, hs_value);
    ai_ap74_ap76_seed_word(vs_page0, vs_off0, vs_value0);
    ai_ap74_ap76_seed_word(vs_page1, vs_off1, vs_value1);

    ai_ap74_ap76_prepare_hs();

    TEST_SETUP_EXCEPT();
    uint64_t hs_observed = ai_ap74_ap76_ld64(hs_vaddr);
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-76 context-flush probe first creates a HS translation/prefetch candidate before satp.changed flush",
        hs_observed == hs_value
    );

    goto_priv(PRIV_M);
    uint64_t satp_before = CSRR(CSR_SATP);
    CSRW(CSR_SATP, satp_before ^ (2ULL << 44));
    sfence_vma();

    ai_ap74_ap76_prepare_vs();

    TEST_SETUP_EXCEPT();
    uint64_t vs_observed0 = ai_ap74_ap76_ld64(vs_vaddr0);
    uint64_t vs_observed1 = ai_ap74_ap76_ld64(vs_vaddr1);
    AI_AP74_AP76_ASSERT_NO_EXCEPT_AND("AP-76 context-flush probe then issues VS two-stage demand walks under the refreshed s2xlate context",
        vs_observed0 == vs_value0 &&
        vs_observed1 == vs_value1
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-76 context-flush probe preserves HS and VS backing data across satp/hgatp/vsatp flush boundaries",
        read64(phys_page_base(hs_page) + hs_off) == hs_value &&
        read64(phys_page_base(vs_page0) + vs_off0) == vs_value0 &&
        read64(phys_page_base(vs_page1) + vs_off1) == vs_value1
    );
    CSRW(CSR_HGATP, 0ULL);
    CSRW(CSR_VSATP, 0ULL);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();
    hfence();

    TEST_END();
}
