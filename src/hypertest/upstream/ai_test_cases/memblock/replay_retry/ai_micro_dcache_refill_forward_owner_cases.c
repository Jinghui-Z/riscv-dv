#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_DCO_LINE_BYTES 64U
#define AI_DCO_LINE_WORDS 8U
#define AI_DCO_VEC_BYTES 16U

#define AI_DCO_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline void ai_dco_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint8_t ai_dco_line_byte(uint8_t line_id, uint8_t generation,
    unsigned offset)
{
    uint8_t half = (offset >= 32U) ? 0xa5U : 0x5aU;
    uint8_t lane = (uint8_t)((offset * 17U) ^ ((offset & 7U) * 29U));

    return (uint8_t)(0x40U ^ (line_id * 0x13U) ^
        (generation * 0x2fU) ^ half ^ lane);
}

static inline void ai_dco_make_line(uint8_t *bytes, uint8_t line_id,
    uint8_t generation)
{
    for (unsigned i = 0; i < AI_DCO_LINE_BYTES; ++i) {
        bytes[i] = ai_dco_line_byte(line_id, generation, i);
    }
}

static inline uint64_t ai_dco_get_le64(const uint8_t *bytes, unsigned offset)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[offset + i]) << (i * 8U);
    }
    return value;
}

static inline void ai_dco_put_le64(uint8_t *bytes, unsigned offset,
    uint64_t value)
{
    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        bytes[offset + i] = (uint8_t)(value >> (i * 8U));
    }
}

static inline void ai_dco_write_line(uintptr_t paddr, const uint8_t *bytes)
{
    for (unsigned i = 0; i < AI_DCO_LINE_BYTES; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_dco_match_bytes(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static __attribute__((noinline)) uint64_t ai_dco_same_pc_load(uintptr_t addr)
{
    uint64_t value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[value], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline void ai_dco_refill_tag_window(uintptr_t refill_addr,
    uintptr_t first_addr, uintptr_t second_addr, uintptr_t marker_addr,
    uint64_t *refill_out, uint64_t *first_out, uint64_t *second_out)
{
    uint64_t refill_value = 0ULL;
    uint64_t first_value = 0ULL;
    uint64_t second_value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[refill_value], 0(%[refill_addr])\n\t"
        "ld %[first_value], 0(%[first_addr])\n\t"
        "sd %[first_value], 0(%[marker_addr])\n\t"
        "ld %[second_value], 0(%[second_addr])\n\t"
        "sd %[second_value], 8(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [refill_value] "=&r"(refill_value),
          [first_value] "=&r"(first_value),
          [second_value] "=&r"(second_value)
        : [refill_addr] "r"(refill_addr),
          [first_addr] "r"(first_addr),
          [second_addr] "r"(second_addr),
          [marker_addr] "r"(marker_addr)
        : "memory");

    *refill_out = refill_value;
    *first_out = first_value;
    *second_out = second_value;
}

bool ai_micro_hs_missqueue_multisource_refill_vector_window_no_halfline_cross_source_corner()
{
    TEST_START();

    const uintptr_t vec0_vaddr = hs_page_base(VSRWX_GRWX) + 0x500U;
    const uintptr_t vec0_paddr = phys_page_base(VSRWX_GRWX) + 0x500U;
    const uintptr_t vec1_vaddr = hs_page_base(VSRWX_GRWX) + 0x600U;
    const uintptr_t vec1_paddr = phys_page_base(VSRWX_GRWX) + 0x600U;
    const uintptr_t load_vaddr = hs_page_base(VSRW_GRWX) + 0x500U;
    const uintptr_t load_paddr = phys_page_base(VSRW_GRWX) + 0x500U;
    const uintptr_t store_vaddr = hs_page_base(SWITCH1) + 0x500U;
    const uintptr_t store_paddr = phys_page_base(SWITCH1) + 0x500U;
    const uintptr_t amo_vaddr = hs_page_base(SWITCH2) + 0x500U;
    const uintptr_t amo_paddr = phys_page_base(SWITCH2) + 0x500U;
    const uintptr_t dump_vaddr = hs_page_base(SCRATCHPAD) + 0x500U;
    const uintptr_t dump_paddr = phys_page_base(SCRATCHPAD) + 0x500U;
    const uintptr_t pressure_vaddrs[] = {
        hs_page_base(VSRWX_GRWX) + 0x780U,
        hs_page_base(VSRW_GRWX) + 0x780U,
        hs_page_base(SWITCH1) + 0x780U,
        hs_page_base(SWITCH2) + 0x780U,
    };
    const uintptr_t pressure_paddrs[] = {
        phys_page_base(VSRWX_GRWX) + 0x780U,
        phys_page_base(VSRW_GRWX) + 0x780U,
        phys_page_base(SWITCH1) + 0x780U,
        phys_page_base(SWITCH2) + 0x780U,
    };
    const uint64_t pressure_values[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0xf0e1d2c3b4a59687ULL,
        0x7766554433221100ULL,
    };
    const size_t pressure_count =
        sizeof(pressure_vaddrs) / sizeof(pressure_vaddrs[0]);
    const uint64_t store_full = 0x0badf00d13579bdfULL;
    const uint64_t amo_addend = 0x0001000200030004ULL;
    uint8_t vec0_bytes[AI_DCO_LINE_BYTES];
    uint8_t vec1_bytes[AI_DCO_LINE_BYTES];
    uint8_t load_bytes[AI_DCO_LINE_BYTES];
    uint8_t store_bytes[AI_DCO_LINE_BYTES];
    uint8_t amo_bytes[AI_DCO_LINE_BYTES];
    uint8_t expected_store[AI_DCO_LINE_BYTES];
    uint8_t expected_amo[AI_DCO_LINE_BYTES];
    uint64_t original_menvcfg = 0ULL;
    uint64_t original_mstatus = 0ULL;

    ai_dco_make_line(vec0_bytes, 0x11U, 0x02U);
    ai_dco_make_line(vec1_bytes, 0x12U, 0x03U);
    ai_dco_make_line(load_bytes, 0x21U, 0x04U);
    ai_dco_make_line(store_bytes, 0x31U, 0x05U);
    ai_dco_make_line(amo_bytes, 0x41U, 0x06U);
    for (unsigned i = 0; i < AI_DCO_LINE_BYTES; ++i) {
        expected_store[i] = store_bytes[i];
        expected_amo[i] = amo_bytes[i];
    }
    expected_store[18] = 0xfeU;
    expected_store[19] = 0xcaU;
    ai_dco_put_le64(expected_store, 40U, store_full);
    expected_store[63] = 0x5dU;
    const uint64_t amo_old_expected = ai_dco_get_le64(amo_bytes, 24U);
    ai_dco_put_le64(expected_amo, 24U, amo_old_expected + amo_addend);

    goto_priv(PRIV_M);
    original_menvcfg = CSRR(CSR_MENVCFG);
    original_mstatus = CSRR(CSR_MSTATUS);
    CSRW(CSR_MENVCFG, (original_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    ai_dco_enable_vector_state();
    ai_dco_write_line(vec0_paddr, vec0_bytes);
    ai_dco_write_line(vec1_paddr, vec1_bytes);
    ai_dco_write_line(load_paddr, load_bytes);
    ai_dco_write_line(store_paddr, store_bytes);
    ai_dco_write_line(amo_paddr, amo_bytes);
    for (unsigned i = 0; i < AI_DCO_LINE_BYTES; ++i) {
        write8(dump_paddr + i, 0x5aU);
    }
    for (size_t i = 0; i < pressure_count; ++i) {
        write64(pressure_paddrs[i], pressure_values[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    bool warm_ok = ld(vec0_vaddr) == ai_dco_get_le64(vec0_bytes, 0U) &&
        ld(vec1_vaddr) == ai_dco_get_le64(vec1_bytes, 0U) &&
        ld(load_vaddr) == ai_dco_get_le64(load_bytes, 0U) &&
        ld(store_vaddr) == ai_dco_get_le64(store_bytes, 0U) &&
        ld(amo_vaddr) == ai_dco_get_le64(amo_bytes, 0U);
    for (size_t i = 0; i < pressure_count; ++i) {
        warm_ok = warm_ok && (ld(pressure_vaddrs[i]) == pressure_values[i]);
    }
    AI_DCO_ASSERT_NO_EXCEPT_AND("all ordinary DRAM refill-forward owner targets warm with readable payloads before the multi-source miss window",
        warm_ok
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(vec0_vaddr);
    cbo_inval(vec1_vaddr);
    cbo_inval(load_vaddr);
    cbo_inval(store_vaddr);
    cbo_inval(amo_vaddr);
    for (size_t i = 0; i < pressure_count; ++i) {
        cbo_inval(pressure_vaddrs[i]);
    }
    AI_DCO_ASSERT_NO_EXCEPT_AND("all refill-forward owner target lines can be invalidated before concurrent load vector store and AMO pressure",
        true
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(load_vaddr);
    prefetch_r(vec0_vaddr);
    uint64_t load_low = ld(load_vaddr);
    uint64_t load_high = ld(load_vaddr + 32U);
    vle8_to_v6((const uint8_t *)vec0_vaddr, AI_DCO_VEC_BYTES);
    vse8_from_v6((uint8_t *)dump_vaddr, AI_DCO_VEC_BYTES);
    vle8_to_v6((const uint8_t *)(vec1_vaddr + 48U), AI_DCO_VEC_BYTES);
    vse8_from_v6((uint8_t *)(dump_vaddr + 32U), AI_DCO_VEC_BYTES);
    sh(store_vaddr + 18U, 0xcafeU);
    sd(store_vaddr + 40U, store_full);
    sb(store_vaddr + 63U, 0x5dU);
    uint64_t amo_old = amoadd_d(amo_vaddr + 24U, amo_addend);
    uint64_t pressure_mix = 0ULL;
    for (size_t i = 0; i < pressure_count; ++i) {
        prefetch_r(pressure_vaddrs[i]);
        pressure_mix ^= ld(pressure_vaddrs[i]);
    }
    fence_iorw_iorw();
    AI_DCO_ASSERT_NO_EXCEPT_AND("multi-source cold load vector store and AMO refill pressure retires without an unexpected trap",
        load_low == ai_dco_get_le64(load_bytes, 0U) &&
        load_high == ai_dco_get_le64(load_bytes, 32U) &&
        amo_old == amo_old_expected &&
        pressure_mix ==
            (pressure_values[0] ^ pressure_values[1] ^
             pressure_values[2] ^ pressure_values[3])
    );

    TEST_SETUP_EXCEPT();
    bool visible_store_ok = true;
    bool visible_amo_ok = true;
    for (unsigned i = 0; i < AI_DCO_LINE_WORDS; ++i) {
        visible_store_ok = visible_store_ok &&
            (ld(store_vaddr + (i * sizeof(uint64_t))) ==
             ai_dco_get_le64(expected_store, i * sizeof(uint64_t)));
        visible_amo_ok = visible_amo_ok &&
            (ld(amo_vaddr + (i * sizeof(uint64_t))) ==
             ai_dco_get_le64(expected_amo, i * sizeof(uint64_t)));
    }
    AI_DCO_ASSERT_NO_EXCEPT_AND("store-miss refill complement and AMO update remain bound to their own cachelines after the multi-source grant-pressure window",
        visible_store_ok && visible_amo_ok
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("vector refill windows dump the exact low and tail 16B images instead of duplicating a half-window or borrowing another source line",
        ai_dco_match_bytes(dump_paddr, vec0_bytes, AI_DCO_VEC_BYTES) &&
        ai_dco_match_bytes(dump_paddr + 32U, vec1_bytes + 48U,
            AI_DCO_VEC_BYTES)
    );
    TEST_ASSERT("backing memory after the multi-source refill-forward owner case preserves load payloads, store byte masks, AMO exactly-once update, and pressure lines",
        ai_dco_match_bytes(load_paddr, load_bytes, AI_DCO_LINE_BYTES) &&
        ai_dco_match_bytes(store_paddr, expected_store, AI_DCO_LINE_BYTES) &&
        ai_dco_match_bytes(amo_paddr, expected_amo, AI_DCO_LINE_BYTES) &&
        read64(pressure_paddrs[0]) == pressure_values[0] &&
        read64(pressure_paddrs[1]) == pressure_values[1] &&
        read64(pressure_paddrs[2]) == pressure_values[2] &&
        read64(pressure_paddrs[3]) == pressure_values[3]
    );
    CSRW(CSR_MENVCFG, original_menvcfg);
    CSRW(CSR_MSTATUS, original_mstatus);

    TEST_END();
}

bool ai_micro_hs_same_set_way_training_dependent_store_no_wrong_way_payload_corner()
{
    TEST_START();

    const enum test_page pages[] = {
        VSRWX_GRWX, VSRW_GRWX, SWITCH1, SWITCH2,
    };
    const uintptr_t set_offset = 0x900U;
    const uintptr_t marker_vaddr = hs_page_base(SCRATCHPAD) + 0x900U;
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD) + 0x900U;
    const uintptr_t dump_vaddr = hs_page_base(SCRATCHPAD) + 0x940U;
    const uintptr_t dump_paddr = phys_page_base(SCRATCHPAD) + 0x940U;
    uint8_t line_bytes[4][AI_DCO_LINE_BYTES];
    uint64_t original_mstatus = 0ULL;

    for (unsigned i = 0; i < 4U; ++i) {
        ai_dco_make_line(line_bytes[i], (uint8_t)(0x51U + i),
            (uint8_t)(0x07U + i));
    }

    goto_priv(PRIV_M);
    original_mstatus = CSRR(CSR_MSTATUS);
    ai_dco_enable_vector_state();
    for (unsigned i = 0; i < 4U; ++i) {
        ai_dco_write_line(phys_page_base(pages[i]) + set_offset,
            line_bytes[i]);
    }
    write64(marker_paddr, 0x1111222233334444ULL);
    write64(marker_paddr + sizeof(uint64_t), 0x5555666677778888ULL);
    for (unsigned i = 0; i < AI_DCO_VEC_BYTES; ++i) {
        write8(dump_paddr + i, 0x3cU);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    bool warm_ok = true;
    for (unsigned i = 0; i < 4U; ++i) {
        warm_ok = warm_ok &&
            (ai_dco_same_pc_load(hs_page_base(pages[i]) + set_offset) ==
             ai_dco_get_le64(line_bytes[i], 0U));
    }
    AI_DCO_ASSERT_NO_EXCEPT_AND("same-set way-training probe first makes all candidate cachelines resident with distinct payloads",
        warm_ok
    );

    TEST_SETUP_EXCEPT();
    bool train_ok = true;
    for (unsigned round = 0; round < 16U; ++round) {
        for (unsigned i = 1U; i < 4U; ++i) {
            train_ok = train_ok &&
                (ai_dco_same_pc_load(hs_page_base(pages[i]) + set_offset) ==
                 ai_dco_get_le64(line_bytes[i], 0U));
        }
    }
    uint64_t observed_a =
        ai_dco_same_pc_load(hs_page_base(pages[0]) + set_offset);
    sd(marker_vaddr, observed_a);
    vle8_to_v6((const uint8_t *)(hs_page_base(pages[0]) + set_offset),
        AI_DCO_VEC_BYTES);
    vse8_from_v6((uint8_t *)dump_vaddr, AI_DCO_VEC_BYTES);
    fence_iorw_iorw();
    AI_DCO_ASSERT_NO_EXCEPT_AND("after training the same load PC toward other same-index ways, scalar and vector observers still consume only the target line",
        observed_a == ai_dco_get_le64(line_bytes[0], 0U) &&
        train_ok
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the dependent marker store captures the target-way payload rather than any trained wrong-way payload",
        read64(marker_paddr) == ai_dco_get_le64(line_bytes[0], 0U) &&
        read64(marker_paddr + sizeof(uint64_t)) == 0x5555666677778888ULL
    );
    TEST_ASSERT("the same-set way-training vector dump and all candidate cachelines preserve their own high-entropy images",
        ai_dco_match_bytes(dump_paddr, line_bytes[0], AI_DCO_VEC_BYTES) &&
        ai_dco_match_bytes(phys_page_base(pages[0]) + set_offset,
            line_bytes[0], AI_DCO_LINE_BYTES) &&
        ai_dco_match_bytes(phys_page_base(pages[1]) + set_offset,
            line_bytes[1], AI_DCO_LINE_BYTES) &&
        ai_dco_match_bytes(phys_page_base(pages[2]) + set_offset,
            line_bytes[2], AI_DCO_LINE_BYTES) &&
        ai_dco_match_bytes(phys_page_base(pages[3]) + set_offset,
            line_bytes[3], AI_DCO_LINE_BYTES)
    );
    CSRW(CSR_MSTATUS, original_mstatus);

    TEST_END();
}

bool ai_micro_hs_dcache_refill_tagwrite_backpressure_held_load_owner_no_wrong_line_corner()
{
    TEST_START();

    const uintptr_t base_offset = 0xb00U;
    const uintptr_t marker_vaddr = hs_page_base(SCRATCHPAD) + 0xb00U;
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD) + 0xb00U;
    const uint64_t marker_guard = 0x1122aabb3344ccddULL;
    uint64_t original_menvcfg = 0ULL;

    goto_priv(PRIV_M);
    original_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(CSR_MENVCFG, (original_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t off = base_offset + (round * AI_DCO_LINE_BYTES);
        const uintptr_t refill_paddr = phys_page_base(VSRWX_GRWX) + off;
        const uintptr_t first_paddr = phys_page_base(VSRW_GRWX) + off;
        const uintptr_t second_paddr = phys_page_base(SWITCH1) + off;
        write64(refill_paddr, 0x1000000000000000ULL + round);
        write64(first_paddr, 0x2100000000000000ULL + round);
        write64(second_paddr, 0x3200000000000000ULL + round);
    }
    write64(marker_paddr, 0ULL);
    write64(marker_paddr + sizeof(uint64_t), 0ULL);
    write64(marker_paddr + 2U * sizeof(uint64_t), marker_guard);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t off = base_offset + (round * AI_DCO_LINE_BYTES);
        const uintptr_t refill_vaddr = hs_page_base(VSRWX_GRWX) + off;
        const uintptr_t refill_paddr = phys_page_base(VSRWX_GRWX) + off;
        const uintptr_t first_vaddr = hs_page_base(VSRW_GRWX) + off;
        const uintptr_t second_vaddr = hs_page_base(SWITCH1) + off;
        const uint64_t refill_new = 0x9000000000000000ULL + round;
        const uint64_t first_expected = 0x2100000000000000ULL + round;
        const uint64_t second_expected = 0x3200000000000000ULL + round;
        uint64_t refill_observed = 0ULL;
        uint64_t first_observed = 0ULL;
        uint64_t second_observed = 0ULL;

        TEST_SETUP_EXCEPT();
        bool warm_ok = ld(refill_vaddr) ==
            (0x1000000000000000ULL + round);
        warm_ok = warm_ok &&
            (ld(first_vaddr) == (0x2100000000000000ULL + round));
        warm_ok = warm_ok &&
            (ld(second_vaddr) == (0x3200000000000000ULL + round));
        AI_DCO_ASSERT_NO_EXCEPT_AND("tag-write backpressure probe warms the refill and held-load candidate lines before cooling the refill owner",
            warm_ok
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(refill_vaddr);
        AI_DCO_ASSERT_NO_EXCEPT_AND("tag-write backpressure probe invalidates only the refill owner line before issuing nearby same-index loads",
            true
        );

        goto_priv(PRIV_M);
        write64(refill_paddr, refill_new);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_dco_refill_tag_window(refill_vaddr, first_vaddr, second_vaddr,
            marker_vaddr, &refill_observed, &first_observed,
            &second_observed);
        AI_DCO_ASSERT_NO_EXCEPT_AND("cold refill tag update and two nearby held load candidates complete with their own owner payloads",
            refill_observed == refill_new &&
            first_observed == first_expected &&
            second_observed == second_expected
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("marker stores after the tag-write backpressure window capture each held-load owner exactly once",
            read64(marker_paddr) == first_expected &&
            read64(marker_paddr + sizeof(uint64_t)) == second_expected &&
            read64(marker_paddr + 2U * sizeof(uint64_t)) == marker_guard &&
            read64(refill_paddr) == refill_new
        );
        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, original_menvcfg);

    TEST_END();
}
