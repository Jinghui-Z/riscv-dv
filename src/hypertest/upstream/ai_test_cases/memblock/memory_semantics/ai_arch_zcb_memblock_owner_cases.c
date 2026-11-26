#include <rvh_test.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_ARCH_ZCB_OWNER_BYTES 96U
#define AI_ARCH_ZCB_OWNER_MAIN_OBS 14U
#define AI_ARCH_ZCB_OWNER_EDGE_OBS 8U
#define AI_ARCH_ZCB_OWNER_SCALAR_ATTR \
    __attribute__((noinline, optimize("no-tree-vectorize", \
                                      "no-tree-loop-distribute-patterns")))

static uint8_t ai_arch_zcb_owner_mem[AI_ARCH_ZCB_OWNER_BYTES]
    __attribute__((aligned(64)));

static AI_ARCH_ZCB_OWNER_SCALAR_ATTR uint8_t ai_arch_zcb_owner_seed_byte(
    unsigned index)
{
    return (uint8_t)(0x35U + (uint8_t)(index * 29U) ^
        (uint8_t)(index << 2));
}

static AI_ARCH_ZCB_OWNER_SCALAR_ATTR void ai_arch_zcb_owner_fill_pair(
    uint8_t *actual, uint8_t *expected, unsigned bytes)
{
    volatile uint8_t *actual_v = (volatile uint8_t *)actual;
    volatile uint8_t *expected_v = (volatile uint8_t *)expected;

    for (unsigned i = 0; i < bytes; ++i) {
        const uint8_t value = ai_arch_zcb_owner_seed_byte(i);
        actual_v[i] = value;
        expected_v[i] = value;
    }
}

static AI_ARCH_ZCB_OWNER_SCALAR_ATTR void ai_arch_zcb_owner_fill_u64(
    uint64_t *dst, unsigned count, uint64_t value)
{
    volatile uint64_t *dst_v = (volatile uint64_t *)dst;

    for (unsigned i = 0; i < count; ++i) {
        dst_v[i] = value ^ ((uint64_t)i << 40);
    }
}

static AI_ARCH_ZCB_OWNER_SCALAR_ATTR bool ai_arch_zcb_owner_bytes_equal(
    const uint8_t *actual, const uint8_t *expected, unsigned bytes,
    unsigned *bad_index)
{
    volatile const uint8_t *actual_v = (volatile const uint8_t *)actual;
    volatile const uint8_t *expected_v = (volatile const uint8_t *)expected;

    for (unsigned i = 0; i < bytes; ++i) {
        if (actual_v[i] != expected_v[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = 0U;
    return true;
}

static AI_ARCH_ZCB_OWNER_SCALAR_ATTR uint64_t ai_arch_zcb_owner_load_le64(
    const uint8_t *bytes, unsigned offset)
{
    volatile const uint8_t *bytes_v = (volatile const uint8_t *)bytes;
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < 8U; ++i) {
        value |= ((uint64_t)bytes_v[offset + i]) << (8U * i);
    }

    return value;
}

static inline uint64_t ai_arch_zcb_owner_sext16(uint16_t value)
{
    return (uint64_t)(int64_t)(int16_t)value;
}

static inline void ai_arch_zcb_owner_main_sequence(uintptr_t base,
    uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb\n\t"
        "mv a1, %[base]\n\t"
        "mv t1, %[obs]\n\t"
        "li a2, 0x8001\n\t"
        "c.sh a2, 0(a1)\n\t"
        "c.lh a3, 0(a1)\n\t"
        "sd a3, 0(t1)\n\t"
        "c.lhu a4, 0(a1)\n\t"
        "sd a4, 8(t1)\n\t"
        ".option push\n\t"
        ".option norvc\n\t"
        "lh t2, 0(a1)\n\t"
        "sd t2, 16(t1)\n\t"
        "lhu t2, 0(a1)\n\t"
        "sd t2, 24(t1)\n\t"
        ".option pop\n\t"
        "li a2, 0x7e\n\t"
        "c.sb a2, 1(a1)\n\t"
        "c.lhu a4, 0(a1)\n\t"
        "sd a4, 32(t1)\n\t"
        "c.lh a3, 0(a1)\n\t"
        "sd a3, 40(t1)\n\t"
        "c.lbu a5, 1(a1)\n\t"
        "sd a5, 48(t1)\n\t"
        "li a2, 0x80\n\t"
        "c.sb a2, 3(a1)\n\t"
        "c.lbu a5, 3(a1)\n\t"
        "sd a5, 56(t1)\n\t"
        ".option push\n\t"
        ".option norvc\n\t"
        "lbu t2, 3(a1)\n\t"
        "sd t2, 64(t1)\n\t"
        ".option pop\n\t"
        "li a2, -128\n\t"
        "c.sh a2, 2(a1)\n\t"
        "c.lh a3, 2(a1)\n\t"
        "sd a3, 72(t1)\n\t"
        "c.lhu a4, 2(a1)\n\t"
        "sd a4, 80(t1)\n\t"
        "c.lbu a5, 2(a1)\n\t"
        "sd a5, 88(t1)\n\t"
        "c.lbu a5, 3(a1)\n\t"
        "sd a5, 96(t1)\n\t"
        ".option push\n\t"
        ".option norvc\n\t"
        "ld t2, 0(a1)\n\t"
        "sd t2, 104(t1)\n\t"
        ".option pop\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base), [obs] "r"(obs)
        : "a1", "a2", "a3", "a4", "a5", "t1", "t2", "memory");
}

static inline void ai_arch_zcb_owner_edge_sequence(uintptr_t base,
    uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb\n\t"
        "mv a1, %[base]\n\t"
        "mv t1, %[obs]\n\t"
        "li a2, 0x8001\n\t"
        "c.sh a2, 0(a1)\n\t"
        "c.lh a3, 0(a1)\n\t"
        "sd a3, 0(t1)\n\t"
        "c.lhu a4, 0(a1)\n\t"
        "sd a4, 8(t1)\n\t"
        "li a2, -128\n\t"
        "c.sh a2, 2(a1)\n\t"
        "c.lh a3, 2(a1)\n\t"
        "sd a3, 16(t1)\n\t"
        "c.lhu a4, 2(a1)\n\t"
        "sd a4, 24(t1)\n\t"
        "li a2, 0x7f\n\t"
        "c.sb a2, 1(a1)\n\t"
        "c.lbu a5, 1(a1)\n\t"
        "sd a5, 32(t1)\n\t"
        "c.lhu a4, 0(a1)\n\t"
        "sd a4, 40(t1)\n\t"
        "c.lh a3, 0(a1)\n\t"
        "sd a3, 48(t1)\n\t"
        ".option push\n\t"
        ".option norvc\n\t"
        "lhu t2, 0(a1)\n\t"
        "sd t2, 56(t1)\n\t"
        ".option pop\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base), [obs] "r"(obs)
        : "a1", "a2", "a3", "a4", "a5", "t1", "t2", "memory");
}

#pragma GCC push_options
#pragma GCC optimize ("no-tree-vectorize", "no-tree-loop-distribute-patterns")
bool ai_arch_zcb_narrow_fuop_forward_owner_corner()
{
    TEST_START();

    enum {
        MAIN_OFF = 16U,
        EDGE_OFF = 30U,
    };
    uint8_t expected[AI_ARCH_ZCB_OWNER_BYTES];
    uint64_t main_obs[AI_ARCH_ZCB_OWNER_MAIN_OBS];
    uint64_t edge_obs[AI_ARCH_ZCB_OWNER_EDGE_OBS];
    unsigned bad_index = 0U;

    goto_priv(PRIV_M);
    ai_arch_zcb_owner_fill_pair(ai_arch_zcb_owner_mem, expected,
        AI_ARCH_ZCB_OWNER_BYTES);
    ai_arch_zcb_owner_fill_u64(main_obs, AI_ARCH_ZCB_OWNER_MAIN_OBS,
        0xbad2000000000000ULL);
    ai_arch_zcb_owner_fill_u64(edge_obs, AI_ARCH_ZCB_OWNER_EDGE_OBS,
        0xbad3000000000000ULL);

    expected[MAIN_OFF + 0U] = 0x01U;
    expected[MAIN_OFF + 1U] = 0x7eU;
    expected[MAIN_OFF + 2U] = 0x80U;
    expected[MAIN_OFF + 3U] = 0xffU;
    expected[EDGE_OFF + 0U] = 0x01U;
    expected[EDGE_OFF + 1U] = 0x7fU;
    expected[EDGE_OFF + 2U] = 0x80U;
    expected[EDGE_OFF + 3U] = 0xffU;

    TEST_SETUP_EXCEPT();
    ai_arch_zcb_owner_main_sequence(
        (uintptr_t)&ai_arch_zcb_owner_mem[MAIN_OFF], main_obs);
    TEST_ASSERT("Zcb narrow owner: c.sh/c.sb immediate-forward sequence keeps c.lh/c.lhu/c.lbu signedness, byte lane, and 32-bit twin controls bound to the current uop",
        excpt.triggered == false &&
        main_obs[0] == ai_arch_zcb_owner_sext16(0x8001U) &&
        main_obs[1] == 0x8001ULL &&
        main_obs[2] == ai_arch_zcb_owner_sext16(0x8001U) &&
        main_obs[3] == 0x8001ULL &&
        main_obs[4] == 0x7e01ULL &&
        main_obs[5] == 0x7e01ULL &&
        main_obs[6] == 0x7eULL &&
        main_obs[7] == 0x80ULL &&
        main_obs[8] == 0x80ULL &&
        main_obs[9] == ai_arch_zcb_owner_sext16(0xff80U) &&
        main_obs[10] == 0xff80ULL &&
        main_obs[11] == 0x80ULL &&
        main_obs[12] == 0xffULL &&
        main_obs[13] == ai_arch_zcb_owner_load_le64(expected, MAIN_OFF)
    );

    TEST_SETUP_EXCEPT();
    ai_arch_zcb_owner_edge_sequence(
        (uintptr_t)&ai_arch_zcb_owner_mem[EDGE_OFF], edge_obs);
    TEST_ASSERT("Zcb narrow owner: c.sh on both sides of a 16B edge plus c.sb high-byte overlay preserves compressed halfword signed/unsigned select and byte mask ownership",
        excpt.triggered == false &&
        edge_obs[0] == ai_arch_zcb_owner_sext16(0x8001U) &&
        edge_obs[1] == 0x8001ULL &&
        edge_obs[2] == ai_arch_zcb_owner_sext16(0xff80U) &&
        edge_obs[3] == 0xff80ULL &&
        edge_obs[4] == 0x7fULL &&
        edge_obs[5] == 0x7f01ULL &&
        edge_obs[6] == 0x7f01ULL &&
        edge_obs[7] == 0x7f01ULL
    );

    TEST_ASSERT("Zcb narrow owner: final compressed store byte image preserves every non-target guard byte after same-block and 16B-edge narrow updates",
        ai_arch_zcb_owner_bytes_equal(ai_arch_zcb_owner_mem, expected,
            AI_ARCH_ZCB_OWNER_BYTES, &bad_index),
        "first_bad_byte=%u actual=0x%02x expected=0x%02x",
        bad_index,
        (unsigned)ai_arch_zcb_owner_mem[bad_index],
        (unsigned)expected[bad_index]
    );

    TEST_END();
}
#pragma GCC pop_options
