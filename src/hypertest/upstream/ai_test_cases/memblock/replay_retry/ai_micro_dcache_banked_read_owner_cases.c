#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_BRO_LINE_BYTES 64U
#define AI_BRO_VEC_BYTES 16U
#define AI_BRO_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define AI_BRO_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline void ai_bro_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint8_t ai_bro_line_byte(uint8_t role, uint8_t generation,
    unsigned offset)
{
    uint8_t bank_hint = (uint8_t)((offset & 0x0fU) * 0x11U);
    uint8_t half_hint = (offset >= 32U) ? 0xc3U : 0x3cU;

    return (uint8_t)(0x5aU ^ (role * 0x25U) ^
        (generation * 0x37U) ^ bank_hint ^ half_hint ^
        (uint8_t)(offset * 7U));
}

static inline void ai_bro_make_line(uint8_t *bytes, uint8_t role,
    uint8_t generation)
{
    for (unsigned i = 0; i < AI_BRO_LINE_BYTES; ++i) {
        bytes[i] = ai_bro_line_byte(role, generation, i);
    }
}

static inline uint64_t ai_bro_get_le64(const uint8_t *bytes, unsigned offset)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[offset + i]) << (i * 8U);
    }

    return value;
}

static inline void ai_bro_put_le64(uint8_t *bytes, unsigned offset,
    uint64_t value)
{
    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        bytes[offset + i] = (uint8_t)(value >> (i * 8U));
    }
}

static inline void ai_bro_write_line(uintptr_t paddr, const uint8_t *bytes)
{
    for (unsigned i = 0; i < AI_BRO_LINE_BYTES; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_bro_match_bytes(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_bro_fill_bytes(uintptr_t paddr, uint8_t value,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, value);
    }
}

static inline void ai_bro_three_read_vector_window(
    uintptr_t replay_vaddr,
    uintptr_t first_vaddr,
    uintptr_t second_vaddr,
    uintptr_t vector_vaddr,
    uintptr_t dump_vaddr,
    uintptr_t marker_vaddr,
    uint64_t *replay_out,
    uint64_t *first_out,
    uint64_t *second_out)
{
    uint64_t replay_value = 0ULL;
    uint64_t first_value = 0ULL;
    uint64_t second_value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[replay_value], 0(%[replay_addr])\n\t"
        "ld %[first_value], 0(%[first_addr])\n\t"
        "ld %[second_value], 0(%[second_addr])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[vector_addr])\n\t"
        "vse8.v v8, (%[dump_addr])\n\t"
        "sd %[replay_value], 0(%[marker_addr])\n\t"
        "sd %[first_value], 8(%[marker_addr])\n\t"
        "sd %[second_value], 16(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [replay_value] "=&r"(replay_value),
          [first_value] "=&r"(first_value),
          [second_value] "=&r"(second_value)
        : [replay_addr] "r"(replay_vaddr),
          [first_addr] "r"(first_vaddr),
          [second_addr] "r"(second_vaddr),
          [vector_addr] "r"(vector_vaddr),
          [dump_addr] "r"(dump_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [vl] "r"((int)AI_BRO_VEC_BYTES)
        : "t0", "memory");

    *replay_out = replay_value;
    *first_out = first_value;
    *second_out = second_value;
}

static inline void ai_bro_amo_readline_load_vector_window(
    uintptr_t amo_vaddr,
    uintptr_t load_vaddr,
    uintptr_t vector_vaddr,
    uintptr_t dump_vaddr,
    uintptr_t marker_vaddr,
    uint64_t addend,
    uint64_t *amo_old_out,
    uint64_t *load_low_out,
    uint64_t *load_high_out)
{
    uint64_t amo_old = 0ULL;
    uint64_t load_low = 0ULL;
    uint64_t load_high = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[amo_old], %[addend], (%[amo_addr])\n\t"
        "ld %[load_low], 0(%[load_addr])\n\t"
        "ld %[load_high], 32(%[load_addr])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[vector_addr])\n\t"
        "vse8.v v8, (%[dump_addr])\n\t"
        "sd %[amo_old], 0(%[marker_addr])\n\t"
        "sd %[load_low], 8(%[marker_addr])\n\t"
        "sd %[load_high], 16(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [amo_old] "=&r"(amo_old),
          [load_low] "=&r"(load_low),
          [load_high] "=&r"(load_high)
        : [amo_addr] "r"(amo_vaddr),
          [load_addr] "r"(load_vaddr),
          [vector_addr] "r"(vector_vaddr),
          [dump_addr] "r"(dump_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [addend] "r"(addend),
          [vl] "r"((int)AI_BRO_VEC_BYTES)
        : "t0", "memory");

    *amo_old_out = amo_old;
    *load_low_out = load_low;
    *load_high_out = load_high;
}

bool ai_micro_hs_bankeddata_three_read_replay_pressure_no_starvation_or_cross_payload_corner()
{
    TEST_START();

    const uintptr_t replay_vaddr = hs_page_base(VSRWX_GRWX) + 0x900U;
    const uintptr_t replay_paddr = phys_page_base(VSRWX_GRWX) + 0x900U;
    const uintptr_t first_vaddr = hs_page_base(VSRW_GRWX) + 0x900U;
    const uintptr_t first_paddr = phys_page_base(VSRW_GRWX) + 0x900U;
    const uintptr_t second_vaddr = hs_page_base(SWITCH1) + 0x900U;
    const uintptr_t second_paddr = phys_page_base(SWITCH1) + 0x900U;
    const uintptr_t vector_vaddr = hs_page_base(SWITCH2) + 0x930U;
    const uintptr_t vector_paddr = phys_page_base(SWITCH2) + 0x930U;
    const uintptr_t dump_vaddr = hs_page_base(SCRATCHPAD) + 0x900U;
    const uintptr_t dump_paddr = phys_page_base(SCRATCHPAD) + 0x900U;
    const uintptr_t marker_vaddr = hs_page_base(SCRATCHPAD) + 0x940U;
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD) + 0x940U;
    const uintptr_t marker_guard_paddr = marker_paddr + 3U * sizeof(uint64_t);
    const uint64_t marker_guard = 0xfeedface13579bdfULL;
    uint64_t original_menvcfg = 0ULL;
    uint64_t original_mstatus = 0ULL;

    goto_priv(PRIV_M);
    original_menvcfg = CSRR(CSR_MENVCFG);
    original_mstatus = CSRR(CSR_MSTATUS);
    CSRW(CSR_MENVCFG, (original_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    ai_bro_enable_vector_state();

    for (unsigned round = 0; round < 4U; ++round) {
        uint8_t replay_bytes[AI_BRO_LINE_BYTES];
        uint8_t first_bytes[AI_BRO_LINE_BYTES];
        uint8_t second_bytes[AI_BRO_LINE_BYTES];
        uint8_t vector_bytes[AI_BRO_LINE_BYTES];
        uint64_t replay_observed = 0ULL;
        uint64_t first_observed = 0ULL;
        uint64_t second_observed = 0ULL;

        ai_bro_make_line(replay_bytes, 0x11U, (uint8_t)round);
        ai_bro_make_line(first_bytes, 0x22U, (uint8_t)round);
        ai_bro_make_line(second_bytes, 0x33U, (uint8_t)round);
        ai_bro_make_line(vector_bytes, 0x44U, (uint8_t)round);

        goto_priv(PRIV_M);
        ai_bro_write_line(replay_paddr, replay_bytes);
        ai_bro_write_line(first_paddr, first_bytes);
        ai_bro_write_line(second_paddr, second_bytes);
        ai_bro_write_line(vector_paddr - 0x30U, vector_bytes);
        ai_bro_fill_bytes(dump_paddr, 0xa5U, AI_BRO_VEC_BYTES);
        write64(marker_paddr, 0ULL);
        write64(marker_paddr + 8U, 0ULL);
        write64(marker_paddr + 16U, 0ULL);
        write64(marker_guard_paddr, marker_guard ^ round);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        bool warm_ok = ld(replay_vaddr) == ai_bro_get_le64(replay_bytes, 0U) &&
            ld(first_vaddr) == ai_bro_get_le64(first_bytes, 0U) &&
            ld(second_vaddr) == ai_bro_get_le64(second_bytes, 0U) &&
            ld(vector_vaddr) == ai_bro_get_le64(vector_bytes, 48U);
        AI_BRO_ASSERT_NO_EXCEPT_AND("banked-data three-read setup warms same-offset ordinary DRAM lines before the cold replay-like pressure window",
            warm_ok
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(replay_vaddr);
        AI_BRO_ASSERT_NO_EXCEPT_AND("banked-data three-read pressure cools only the replay-like line before issuing same-offset scalar and vector observers",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_bro_three_read_vector_window(replay_vaddr, first_vaddr,
            second_vaddr, vector_vaddr, dump_vaddr, marker_vaddr,
            &replay_observed, &first_observed, &second_observed);
        AI_BRO_ASSERT_NO_EXCEPT_AND("banked-data three-read pressure keeps cold replay-like load, two same-offset loads, vector dump, and marker progress from starving or crossing payloads",
            replay_observed == ai_bro_get_le64(replay_bytes, 0U) &&
            first_observed == ai_bro_get_le64(first_bytes, 0U) &&
            second_observed == ai_bro_get_le64(second_bytes, 0U)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("banked-data three-read pressure leaves all scalar/vector payloads and marker words bound to their own line across the replay-like window",
            read64(marker_paddr) == ai_bro_get_le64(replay_bytes, 0U) &&
            read64(marker_paddr + 8U) == ai_bro_get_le64(first_bytes, 0U) &&
            read64(marker_paddr + 16U) == ai_bro_get_le64(second_bytes, 0U) &&
            read64(marker_guard_paddr) == (marker_guard ^ round) &&
            ai_bro_match_bytes(dump_paddr, &vector_bytes[48U],
                AI_BRO_VEC_BYTES) &&
            ai_bro_match_bytes(first_paddr, first_bytes,
                AI_BRO_LINE_BYTES) &&
            ai_bro_match_bytes(second_paddr, second_bytes,
                AI_BRO_LINE_BYTES)
        );
    }

    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, original_menvcfg);
    CSRW(CSR_MSTATUS, original_mstatus);

    TEST_END();
}

bool ai_micro_hs_bankeddata_amo_readline_vs_load_vector_no_wrong_line_or_halfwindow_corner()
{
    TEST_START();

    const uintptr_t amo_vaddr = hs_page_base(VSRWX_GRWX) + 0xa00U;
    const uintptr_t amo_paddr = phys_page_base(VSRWX_GRWX) + 0xa00U;
    const uintptr_t load_vaddr = hs_page_base(VSRW_GRWX) + 0xa00U;
    const uintptr_t load_paddr = phys_page_base(VSRW_GRWX) + 0xa00U;
    const uintptr_t vector_vaddr = hs_page_base(SWITCH1) + 0xa30U;
    const uintptr_t vector_paddr = phys_page_base(SWITCH1) + 0xa30U;
    const uintptr_t dump_vaddr = hs_page_base(SCRATCHPAD) + 0xa00U;
    const uintptr_t dump_paddr = phys_page_base(SCRATCHPAD) + 0xa00U;
    const uintptr_t marker_vaddr = hs_page_base(SCRATCHPAD) + 0xa40U;
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD) + 0xa40U;
    const uintptr_t marker_guard_paddr = marker_paddr + 3U * sizeof(uint64_t);
    const uint64_t addends[] = {
        0x0001000200030004ULL,
        0x0004000300020001ULL,
        0x0100010001000100ULL,
    };
    const uint64_t marker_guard = 0x123456789abcdef0ULL;
    uint64_t original_mstatus = 0ULL;

    goto_priv(PRIV_M);
    original_mstatus = CSRR(CSR_MSTATUS);
    ai_bro_enable_vector_state();

    for (unsigned round = 0; round < AI_BRO_ARRAY_SIZE(addends); ++round) {
        uint8_t amo_bytes[AI_BRO_LINE_BYTES];
        uint8_t load_bytes[AI_BRO_LINE_BYTES];
        uint8_t vector_bytes[AI_BRO_LINE_BYTES];
        uint8_t expected_amo[AI_BRO_LINE_BYTES];
        uint64_t amo_old_observed = 0ULL;
        uint64_t load_low_observed = 0ULL;
        uint64_t load_high_observed = 0ULL;

        ai_bro_make_line(amo_bytes, 0x55U, (uint8_t)round);
        ai_bro_make_line(load_bytes, 0x66U, (uint8_t)round);
        ai_bro_make_line(vector_bytes, 0x77U, (uint8_t)round);
        for (unsigned i = 0; i < AI_BRO_LINE_BYTES; ++i) {
            expected_amo[i] = amo_bytes[i];
        }
        const uint64_t amo_old_expected = ai_bro_get_le64(amo_bytes, 24U);
        ai_bro_put_le64(expected_amo, 24U,
            amo_old_expected + addends[round]);

        goto_priv(PRIV_M);
        ai_bro_write_line(amo_paddr, amo_bytes);
        ai_bro_write_line(load_paddr, load_bytes);
        ai_bro_write_line(vector_paddr - 0x30U, vector_bytes);
        ai_bro_fill_bytes(dump_paddr, 0x5aU, AI_BRO_VEC_BYTES);
        write64(marker_paddr, 0ULL);
        write64(marker_paddr + 8U, 0ULL);
        write64(marker_paddr + 16U, 0ULL);
        write64(marker_guard_paddr, marker_guard ^ round);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        bool warm_ok = ld(amo_vaddr + 24U) == amo_old_expected &&
            ld(load_vaddr) == ai_bro_get_le64(load_bytes, 0U) &&
            ld(load_vaddr + 32U) == ai_bro_get_le64(load_bytes, 32U) &&
            ld(vector_vaddr) == ai_bro_get_le64(vector_bytes, 48U);
        AI_BRO_ASSERT_NO_EXCEPT_AND("banked-data readline setup warms AMO, scalar-load, and vector-load ordinary DRAM lines before the same-offset owner window",
            warm_ok
        );

        TEST_SETUP_EXCEPT();
        ai_bro_amo_readline_load_vector_window(amo_vaddr + 24U,
            load_vaddr, vector_vaddr, dump_vaddr, marker_vaddr,
            addends[round], &amo_old_observed, &load_low_observed,
            &load_high_observed);
        AI_BRO_ASSERT_NO_EXCEPT_AND("AMO readline plus same-offset scalar/vector observers retire without wrong-line data, half-window mix, or replay storm symptoms",
            amo_old_observed == amo_old_expected &&
            load_low_observed == ai_bro_get_le64(load_bytes, 0U) &&
            load_high_observed == ai_bro_get_le64(load_bytes, 32U)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("banked-data readline pressure updates only the AMO word and leaves scalar/vector observers bound to their own lines",
            ai_bro_match_bytes(amo_paddr, expected_amo,
                AI_BRO_LINE_BYTES) &&
            ai_bro_match_bytes(load_paddr, load_bytes,
                AI_BRO_LINE_BYTES) &&
            ai_bro_match_bytes(dump_paddr, &vector_bytes[48U],
                AI_BRO_VEC_BYTES) &&
            read64(marker_paddr) == amo_old_expected &&
            read64(marker_paddr + 8U) == ai_bro_get_le64(load_bytes, 0U) &&
            read64(marker_paddr + 16U) == ai_bro_get_le64(load_bytes, 32U) &&
            read64(marker_guard_paddr) == (marker_guard ^ round)
        );
    }

    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, original_mstatus);

    TEST_END();
}
