#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_MPR_LINE_BYTES 64U
#define AI_MPR_VEC_BYTES 16U
#define AI_MPR_ROUNDS 3U

#define AI_MPR_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline void ai_mpr_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint8_t ai_mpr_line_byte(uint8_t role, uint8_t generation,
    unsigned offset)
{
    return (uint8_t)(0x6dU ^ (uint8_t)(role * 0x29U) ^
        (uint8_t)(generation * 0x31U) ^ (uint8_t)(offset * 0x0dU) ^
        (uint8_t)((offset & 0x0fU) * 0x11U) ^
        (offset >= 32U ? 0xc6U : 0x39U));
}

static inline void ai_mpr_make_line(uint8_t *bytes, uint8_t role,
    uint8_t generation)
{
    for (unsigned i = 0; i < AI_MPR_LINE_BYTES; ++i) {
        bytes[i] = ai_mpr_line_byte(role, generation, i);
    }
}

static inline uint64_t ai_mpr_get_le64(const uint8_t *bytes, unsigned offset)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[offset + i]) << (8U * i);
    }

    return value;
}

static inline void ai_mpr_put_le64(uint8_t *bytes, unsigned offset,
    uint64_t value)
{
    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        bytes[offset + i] = (uint8_t)(value >> (8U * i));
    }
}

static inline void ai_mpr_write_line(uintptr_t paddr, const uint8_t *bytes)
{
    for (unsigned i = 0; i < AI_MPR_LINE_BYTES; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_mpr_match_bytes(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_mpr_fill_bytes(uintptr_t paddr, uint8_t value,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, value);
    }
}

static inline void ai_mpr_dirty_pressure_window(uintptr_t first_vaddr,
    uintptr_t second_vaddr, uintptr_t third_vaddr, uint64_t first_value,
    uint64_t second_value, uint64_t third_value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[first_value], 0(%[first_addr])\n\t"
        "sd %[second_value], 16(%[second_addr])\n\t"
        "sd %[third_value], 32(%[third_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [first_addr] "r"(first_vaddr),
          [second_addr] "r"(second_vaddr),
          [third_addr] "r"(third_vaddr),
          [first_value] "r"(first_value),
          [second_value] "r"(second_value),
          [third_value] "r"(third_value)
        : "memory");
}

static inline void ai_mpr_amo_observer_window(uintptr_t target_vaddr,
    uintptr_t observer_vaddr, uintptr_t vector_vaddr, uintptr_t dump_vaddr,
    uintptr_t marker_vaddr, uint64_t first_addend, uint64_t second_addend,
    uint64_t marker_value, uint64_t *first_old_out,
    uint64_t *second_old_out, uint64_t *observer_low_out,
    uint64_t *observer_high_out)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;
    uint64_t observer_low = 0ULL;
    uint64_t observer_high = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[target_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[target_addr])\n\t"
        "ld %[observer_low], 0(%[observer_addr])\n\t"
        "ld %[observer_high], 32(%[observer_addr])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[vector_addr])\n\t"
        "vse8.v v8, (%[dump_addr])\n\t"
        "sd %[first_old], 0(%[marker_addr])\n\t"
        "sd %[second_old], 8(%[marker_addr])\n\t"
        "sd %[observer_low], 16(%[marker_addr])\n\t"
        "sd %[observer_high], 24(%[marker_addr])\n\t"
        "sd %[marker_value], 32(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [observer_low] "=&r"(observer_low),
          [observer_high] "=&r"(observer_high)
        : [target_addr] "r"(target_vaddr),
          [observer_addr] "r"(observer_vaddr),
          [vector_addr] "r"(vector_vaddr),
          [dump_addr] "r"(dump_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [marker_value] "r"(marker_value),
          [vl] "r"((int)AI_MPR_VEC_BYTES)
        : "t0", "memory");

    *first_old_out = first_old;
    *second_old_out = second_old;
    *observer_low_out = observer_low;
    *observer_high_out = observer_high;
}

bool ai_micro_hs_mainpipe_amo_after_dirty_pressure_no_ghost_replace_release_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page pressure0_page = VSRW_GRWX;
    const enum test_page pressure1_page = SWITCH1;
    const enum test_page pressure2_page = SWITCH2;
    const enum test_page observer_page = VSRW_GURWX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0xb00U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0xb00U;
    const uintptr_t pressure0_vaddr = hs_page_base(pressure0_page) + 0xb00U;
    const uintptr_t pressure0_paddr = phys_page_base(pressure0_page) + 0xb00U;
    const uintptr_t pressure1_vaddr = hs_page_base(pressure1_page) + 0xb00U;
    const uintptr_t pressure1_paddr = phys_page_base(pressure1_page) + 0xb00U;
    const uintptr_t pressure2_vaddr = hs_page_base(pressure2_page) + 0xb00U;
    const uintptr_t pressure2_paddr = phys_page_base(pressure2_page) + 0xb00U;
    const uintptr_t observer_vaddr = hs_page_base(observer_page) + 0xb00U;
    const uintptr_t observer_paddr = phys_page_base(observer_page) + 0xb00U;
    const uintptr_t vector_vaddr = hs_page_base(pressure1_page) + 0xb30U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0xb00U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0xb00U;
    const uintptr_t marker_vaddr = hs_page_base(scratch_page) + 0xb40U;
    const uintptr_t marker_paddr = phys_page_base(scratch_page) + 0xb40U;
    const uintptr_t marker_guard_paddr = marker_paddr + 5U * sizeof(uint64_t);
    const uint64_t first_addends[AI_MPR_ROUNDS] = {
        0x0001000200030004ULL,
        0x0010002000300040ULL,
        0x0100010001000100ULL,
    };
    const uint64_t second_addends[AI_MPR_ROUNDS] = {
        0x0004000300020001ULL,
        0x0040003000200010ULL,
        0x0001000100010001ULL,
    };
    const uint64_t marker_guard = 0xfedcba9876543210ULL;
    uint64_t original_menvcfg = 0ULL;
    uint64_t original_mstatus = 0ULL;

    goto_priv(PRIV_M);
    original_menvcfg = CSRR(CSR_MENVCFG);
    original_mstatus = CSRR(CSR_MSTATUS);
    CSRW(CSR_MENVCFG, (original_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    ai_mpr_enable_vector_state();

    for (unsigned round = 0; round < AI_MPR_ROUNDS; ++round) {
        uint8_t target_bytes[AI_MPR_LINE_BYTES];
        uint8_t pressure0_bytes[AI_MPR_LINE_BYTES];
        uint8_t pressure1_bytes[AI_MPR_LINE_BYTES];
        uint8_t pressure2_bytes[AI_MPR_LINE_BYTES];
        uint8_t observer_bytes[AI_MPR_LINE_BYTES];
        uint8_t expected_target[AI_MPR_LINE_BYTES];
        uint8_t expected_pressure0[AI_MPR_LINE_BYTES];
        uint8_t expected_pressure1[AI_MPR_LINE_BYTES];
        uint8_t expected_pressure2[AI_MPR_LINE_BYTES];
        uint64_t first_old = 0ULL;
        uint64_t second_old = 0ULL;
        uint64_t observer_low = 0ULL;
        uint64_t observer_high = 0ULL;

        ai_mpr_make_line(target_bytes, 0x10U, (uint8_t)round);
        ai_mpr_make_line(pressure0_bytes, 0x20U, (uint8_t)round);
        ai_mpr_make_line(pressure1_bytes, 0x30U, (uint8_t)round);
        ai_mpr_make_line(pressure2_bytes, 0x40U, (uint8_t)round);
        ai_mpr_make_line(observer_bytes, 0x50U, (uint8_t)round);
        for (unsigned i = 0; i < AI_MPR_LINE_BYTES; ++i) {
            expected_target[i] = target_bytes[i];
            expected_pressure0[i] = pressure0_bytes[i];
            expected_pressure1[i] = pressure1_bytes[i];
            expected_pressure2[i] = pressure2_bytes[i];
        }

        const uint64_t target_initial = ai_mpr_get_le64(target_bytes, 0U);
        const uint64_t first_expected = target_initial + first_addends[round];
        const uint64_t second_expected =
            first_expected + second_addends[round];
        const uint64_t pressure0_update =
            0x8a00000000000000ULL | (uint64_t)round;
        const uint64_t pressure1_update =
            0x8b00000000000010ULL | (uint64_t)round;
        const uint64_t pressure2_update =
            0x8c00000000000020ULL | (uint64_t)round;
        const uint64_t marker_value =
            0x9d00000000000030ULL | (uint64_t)round;

        ai_mpr_put_le64(expected_target, 0U, second_expected);
        ai_mpr_put_le64(expected_pressure0, 0U, pressure0_update);
        ai_mpr_put_le64(expected_pressure1, 16U, pressure1_update);
        ai_mpr_put_le64(expected_pressure2, 32U, pressure2_update);

        goto_priv(PRIV_M);
        ai_mpr_write_line(target_paddr, target_bytes);
        ai_mpr_write_line(pressure0_paddr, pressure0_bytes);
        ai_mpr_write_line(pressure1_paddr, pressure1_bytes);
        ai_mpr_write_line(pressure2_paddr, pressure2_bytes);
        ai_mpr_write_line(observer_paddr, observer_bytes);
        ai_mpr_fill_bytes(dump_paddr, 0xa5U, AI_MPR_VEC_BYTES);
        for (unsigned i = 0; i < 5U; ++i) {
            write64(marker_paddr + i * sizeof(uint64_t), 0ULL);
        }
        write64(marker_guard_paddr, marker_guard ^ round);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        bool warm_ok = ld(target_vaddr) == target_initial &&
            ld(pressure0_vaddr) == ai_mpr_get_le64(pressure0_bytes, 0U) &&
            ld(pressure1_vaddr + 16U) ==
                ai_mpr_get_le64(pressure1_bytes, 16U) &&
            ld(pressure2_vaddr + 32U) ==
                ai_mpr_get_le64(pressure2_bytes, 32U) &&
            ld(observer_vaddr) == ai_mpr_get_le64(observer_bytes, 0U) &&
            ld(observer_vaddr + 32U) ==
                ai_mpr_get_le64(observer_bytes, 32U) &&
            ld(vector_vaddr) == ai_mpr_get_le64(pressure1_bytes, 48U);
        AI_MPR_ASSERT_NO_EXCEPT_AND("mainpipe replace quarantine setup warms target, dirty-pressure, scalar observer, and vector observer lines",
            warm_ok
        );

        TEST_SETUP_EXCEPT();
        ai_mpr_dirty_pressure_window(pressure0_vaddr, pressure1_vaddr,
            pressure2_vaddr, pressure0_update, pressure1_update,
            pressure2_update);
        AI_MPR_ASSERT_NO_EXCEPT_AND("dirty-pressure stores retire before the cold AMO miss/refill window that must clear non-replacement MainPipeReq.replace",
            true
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(target_vaddr);
        AI_MPR_ASSERT_NO_EXCEPT_AND("only the target AMO line is cooled so the following AMO request enters miss/refill pressure after a real dirty-pressure episode",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_mpr_amo_observer_window(target_vaddr, observer_vaddr,
            vector_vaddr, dump_vaddr, marker_vaddr, first_addends[round],
            second_addends[round], marker_value, &first_old, &second_old,
            &observer_low, &observer_high);
        AI_MPR_ASSERT_NO_EXCEPT_AND("cold AMO refill after dirty pressure performs exactly two RMW updates and keeps scalar/vector observers on their own lines",
            first_old == target_initial &&
            second_old == first_expected &&
            ld(target_vaddr) == second_expected &&
            observer_low == ai_mpr_get_le64(observer_bytes, 0U) &&
            observer_high == ai_mpr_get_le64(observer_bytes, 32U)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("MainPipeReq ghost-replace quarantine keeps AMO final image, dirty pressure lines, vector dump, marker, and guards bound to their own producers",
            ai_mpr_match_bytes(target_paddr, expected_target,
                AI_MPR_LINE_BYTES) &&
            ai_mpr_match_bytes(pressure0_paddr, expected_pressure0,
                AI_MPR_LINE_BYTES) &&
            ai_mpr_match_bytes(pressure1_paddr, expected_pressure1,
                AI_MPR_LINE_BYTES) &&
            ai_mpr_match_bytes(pressure2_paddr, expected_pressure2,
                AI_MPR_LINE_BYTES) &&
            ai_mpr_match_bytes(observer_paddr, observer_bytes,
                AI_MPR_LINE_BYTES) &&
            ai_mpr_match_bytes(dump_paddr, &pressure1_bytes[48U],
                AI_MPR_VEC_BYTES) &&
            read64(marker_paddr) == target_initial &&
            read64(marker_paddr + 8U) == first_expected &&
            read64(marker_paddr + 16U) ==
                ai_mpr_get_le64(observer_bytes, 0U) &&
            read64(marker_paddr + 24U) ==
                ai_mpr_get_le64(observer_bytes, 32U) &&
            read64(marker_paddr + 32U) == marker_value &&
            read64(marker_guard_paddr) == (marker_guard ^ round)
        );
    }

    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, original_menvcfg);
    CSRW(CSR_MSTATUS, original_mstatus);

    TEST_END();
}
