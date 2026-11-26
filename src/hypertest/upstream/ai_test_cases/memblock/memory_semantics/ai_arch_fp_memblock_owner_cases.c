#include <rvh_test.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_ARCH_FP_OWNER_BYTES 256U
#define AI_ARCH_FP_OWNER_SCALAR_ATTR \
    __attribute__((noinline, optimize("no-tree-vectorize", \
                                      "no-tree-loop-distribute-patterns")))

static uint8_t ai_arch_fp_owner_load_mem[AI_ARCH_FP_OWNER_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_fp_owner_store_mem[AI_ARCH_FP_OWNER_BYTES]
    __attribute__((aligned(64)));

static inline void ai_arch_fp_owner_enable_fp(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_INITIAL);
    CSRC(CSR_MSTATUS, 1ULL << 14);
}

static inline void ai_arch_fp_owner_write_fcsr(uint64_t fflags, uint64_t frm)
{
    CSRW(CSR_FCSR, ((frm & 0x7ULL) << 5) | (fflags & 0x1fULL));
}

static inline bool ai_arch_fp_owner_fcsr_matches(uint64_t fflags,
    uint64_t frm)
{
    return (CSRR(CSR_FFLAGS) & 0x1fULL) == (fflags & 0x1fULL) &&
        (CSRR(CSR_FRM) & 0x7ULL) == (frm & 0x7ULL);
}

static AI_ARCH_FP_OWNER_SCALAR_ATTR uint8_t ai_arch_fp_owner_seed_byte(
    unsigned index, uint8_t salt)
{
    return (uint8_t)(salt + (uint8_t)(index * 37U) ^
        (uint8_t)(index << 1));
}

static AI_ARCH_FP_OWNER_SCALAR_ATTR void ai_arch_fp_owner_fill_pair(
    uint8_t *actual, uint8_t *expected, unsigned bytes, uint8_t salt)
{
    volatile uint8_t *actual_v = (volatile uint8_t *)actual;
    volatile uint8_t *expected_v = (volatile uint8_t *)expected;

    for (unsigned i = 0; i < bytes; ++i) {
        const uint8_t value = ai_arch_fp_owner_seed_byte(i, salt);
        actual_v[i] = value;
        expected_v[i] = value;
    }
}

static AI_ARCH_FP_OWNER_SCALAR_ATTR void ai_arch_fp_owner_fill_u64(
    uint64_t *dst, unsigned count, uint64_t value)
{
    volatile uint64_t *dst_v = (volatile uint64_t *)dst;

    for (unsigned i = 0; i < count; ++i) {
        dst_v[i] = value ^ ((uint64_t)i << 48);
    }
}

static AI_ARCH_FP_OWNER_SCALAR_ATTR void ai_arch_fp_owner_put_le32(
    uint8_t *bytes, unsigned offset, uint32_t value)
{
    volatile uint8_t *bytes_v = (volatile uint8_t *)bytes;

    for (unsigned i = 0; i < 4U; ++i) {
        bytes_v[offset + i] = (uint8_t)(value >> (8U * i));
    }
}

static AI_ARCH_FP_OWNER_SCALAR_ATTR void ai_arch_fp_owner_put_le64(
    uint8_t *bytes, unsigned offset, uint64_t value)
{
    volatile uint8_t *bytes_v = (volatile uint8_t *)bytes;

    for (unsigned i = 0; i < 8U; ++i) {
        bytes_v[offset + i] = (uint8_t)(value >> (8U * i));
    }
}

static AI_ARCH_FP_OWNER_SCALAR_ATTR bool ai_arch_fp_owner_bytes_equal(
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

static inline uint64_t ai_arch_fp_owner_flw_boxed(uint32_t value)
{
    return 0xffffffff00000000ULL | (uint64_t)value;
}

static inline uint64_t ai_arch_fp_owner_lw_signed(uint32_t value)
{
    return (uint64_t)(int64_t)(int32_t)value;
}

static inline void ai_arch_fp_owner_load_selector_sequence(
    uintptr_t flw0_addr, uintptr_t lwu_addr, uintptr_t lw_addr,
    uintptr_t flw1_addr, uintptr_t fld0_addr, uintptr_t ld_addr,
    uintptr_t fld1_addr, uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "flw ft0, 0(%[flw0])\n\t"
        "fmv.x.d t0, ft0\n\t"
        "sd t0, 0(%[obs])\n\t"
        "lwu t0, 0(%[lwu])\n\t"
        "sd t0, 8(%[obs])\n\t"
        "lw t0, 0(%[lw])\n\t"
        "sd t0, 16(%[obs])\n\t"
        "flw ft1, 0(%[flw1])\n\t"
        "fmv.x.d t0, ft1\n\t"
        "sd t0, 24(%[obs])\n\t"
        "fld ft2, 0(%[fld0])\n\t"
        "fmv.x.d t0, ft2\n\t"
        "sd t0, 32(%[obs])\n\t"
        "ld t0, 0(%[ld])\n\t"
        "sd t0, 40(%[obs])\n\t"
        "fld ft3, 0(%[fld1])\n\t"
        "fmv.x.d t0, ft3\n\t"
        "sd t0, 48(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [flw0] "r"(flw0_addr),
          [lwu] "r"(lwu_addr),
          [lw] "r"(lw_addr),
          [flw1] "r"(flw1_addr),
          [fld0] "r"(fld0_addr),
          [ld] "r"(ld_addr),
          [fld1] "r"(fld1_addr),
          [obs] "r"(obs)
        : "t0", "ft0", "ft1", "ft2", "ft3", "memory");
}

static inline void ai_arch_fp_owner_store_producer_sequence(
    uintptr_t base_addr, uintptr_t fsw_addr, uintptr_t fsw2_addr,
    uint64_t fsd_payload, uint32_t fsw_payload, uint32_t sw_payload,
    uint64_t sd_payload, uint32_t fsw2_payload, uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft0, %[fsd_payload]\n\t"
        "fsd ft0, 0(%[base])\n\t"
        "ld t0, 0(%[base])\n\t"
        "sd t0, 0(%[obs])\n\t"
        "fld ft1, 0(%[base])\n\t"
        "fmv.x.d t0, ft1\n\t"
        "sd t0, 8(%[obs])\n\t"
        "fmv.w.x ft2, %[fsw_payload]\n\t"
        "fsw ft2, 0(%[fsw])\n\t"
        "ld t0, 0(%[base])\n\t"
        "sd t0, 16(%[obs])\n\t"
        "flw ft3, 0(%[fsw])\n\t"
        "fmv.x.d t0, ft3\n\t"
        "sd t0, 24(%[obs])\n\t"
        "lwu t0, 0(%[fsw])\n\t"
        "sd t0, 32(%[obs])\n\t"
        "lw t0, 0(%[fsw])\n\t"
        "sd t0, 40(%[obs])\n\t"
        "sw %[sw_payload], 0(%[base])\n\t"
        "ld t0, 0(%[base])\n\t"
        "sd t0, 48(%[obs])\n\t"
        "fld ft4, 0(%[base])\n\t"
        "fmv.x.d t0, ft4\n\t"
        "sd t0, 56(%[obs])\n\t"
        "sd %[sd_payload], 0(%[base])\n\t"
        "ld t0, 0(%[base])\n\t"
        "sd t0, 64(%[obs])\n\t"
        "fld ft5, 0(%[base])\n\t"
        "fmv.x.d t0, ft5\n\t"
        "sd t0, 72(%[obs])\n\t"
        "fmv.w.x ft6, %[fsw2_payload]\n\t"
        "fsw ft6, 0(%[fsw2])\n\t"
        "ld t0, 0(%[base])\n\t"
        "sd t0, 80(%[obs])\n\t"
        "flw ft7, 0(%[fsw2])\n\t"
        "fmv.x.d t0, ft7\n\t"
        "sd t0, 88(%[obs])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base_addr),
          [fsw] "r"(fsw_addr),
          [fsw2] "r"(fsw2_addr),
          [fsd_payload] "r"(fsd_payload),
          [fsw_payload] "r"((uint64_t)fsw_payload),
          [sw_payload] "r"((uint64_t)sw_payload),
          [sd_payload] "r"(sd_payload),
          [fsw2_payload] "r"((uint64_t)fsw2_payload),
          [obs] "r"(obs)
        : "t0", "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6",
          "ft7", "memory");
}

#pragma GCC push_options
#pragma GCC optimize ("no-tree-vectorize", "no-tree-loop-distribute-patterns")
bool ai_arch_fp_load_selector_cross16_owner_corner()
{
    TEST_START();

    enum {
        FLW0_OFF = 13U,
        LWU_OFF = 32U + 13U,
        LW_OFF = 64U + 14U,
        FLW1_OFF = 96U + 15U,
        FLD0_OFF = 128U + 9U,
        LD_OFF = 160U + 13U,
        FLD1_OFF = 192U + 15U,
        OBS_COUNT = 7U,
    };
    const uint64_t fflags_sentinel = 0x15ULL;
    const uint64_t frm_sentinel = 0x2ULL;
    const uint32_t flw0_payload = 0x80000001U;
    const uint32_t lwu_payload = 0x89abcdefU;
    const uint32_t lw_payload = 0x8000f00dU;
    const uint32_t flw1_payload = 0x00000080U;
    const uint64_t fld0_payload = 0x7ff8123456789abcULL;
    const uint64_t ld_payload = 0x8000000080000001ULL;
    const uint64_t fld1_payload = 0x0000000000000001ULL;
    uint8_t expected[AI_ARCH_FP_OWNER_BYTES];
    uint64_t obs[OBS_COUNT];
    unsigned bad_index = 0U;

    goto_priv(PRIV_M);
    ai_arch_fp_owner_enable_fp();
    ai_arch_fp_owner_write_fcsr(fflags_sentinel, frm_sentinel);

    ai_arch_fp_owner_fill_pair(ai_arch_fp_owner_load_mem, expected,
        AI_ARCH_FP_OWNER_BYTES, 0x41U);
    ai_arch_fp_owner_fill_u64(obs, OBS_COUNT, 0xbad0000000000000ULL);

    ai_arch_fp_owner_put_le32(ai_arch_fp_owner_load_mem, FLW0_OFF,
        flw0_payload);
    ai_arch_fp_owner_put_le32(expected, FLW0_OFF, flw0_payload);
    ai_arch_fp_owner_put_le32(ai_arch_fp_owner_load_mem, LWU_OFF,
        lwu_payload);
    ai_arch_fp_owner_put_le32(expected, LWU_OFF, lwu_payload);
    ai_arch_fp_owner_put_le32(ai_arch_fp_owner_load_mem, LW_OFF,
        lw_payload);
    ai_arch_fp_owner_put_le32(expected, LW_OFF, lw_payload);
    ai_arch_fp_owner_put_le32(ai_arch_fp_owner_load_mem, FLW1_OFF,
        flw1_payload);
    ai_arch_fp_owner_put_le32(expected, FLW1_OFF, flw1_payload);
    ai_arch_fp_owner_put_le64(ai_arch_fp_owner_load_mem, FLD0_OFF,
        fld0_payload);
    ai_arch_fp_owner_put_le64(expected, FLD0_OFF, fld0_payload);
    ai_arch_fp_owner_put_le64(ai_arch_fp_owner_load_mem, LD_OFF,
        ld_payload);
    ai_arch_fp_owner_put_le64(expected, LD_OFF, ld_payload);
    ai_arch_fp_owner_put_le64(ai_arch_fp_owner_load_mem, FLD1_OFF,
        fld1_payload);
    ai_arch_fp_owner_put_le64(expected, FLD1_OFF, fld1_payload);

    TEST_SETUP_EXCEPT();
    ai_arch_fp_owner_load_selector_sequence(
        (uintptr_t)&ai_arch_fp_owner_load_mem[FLW0_OFF],
        (uintptr_t)&ai_arch_fp_owner_load_mem[LWU_OFF],
        (uintptr_t)&ai_arch_fp_owner_load_mem[LW_OFF],
        (uintptr_t)&ai_arch_fp_owner_load_mem[FLW1_OFF],
        (uintptr_t)&ai_arch_fp_owner_load_mem[FLD0_OFF],
        (uintptr_t)&ai_arch_fp_owner_load_mem[LD_OFF],
        (uintptr_t)&ai_arch_fp_owner_load_mem[FLD1_OFF],
        obs);

    TEST_ASSERT("FP load owner: back-to-back cross16 flw/lwu/lw/fld/ld sequence keeps current fpWen and data-select for every destination class",
        excpt.triggered == false &&
        obs[0] == ai_arch_fp_owner_flw_boxed(flw0_payload) &&
        obs[1] == (uint64_t)lwu_payload &&
        obs[2] == ai_arch_fp_owner_lw_signed(lw_payload) &&
        obs[3] == ai_arch_fp_owner_flw_boxed(flw1_payload) &&
        obs[4] == fld0_payload &&
        obs[5] == ld_payload &&
        obs[6] == fld1_payload &&
        ai_arch_fp_owner_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    TEST_ASSERT("FP load owner: cross16/misaligned FP and integer load consumers leave the backing byte image and guards unchanged",
        ai_arch_fp_owner_bytes_equal(ai_arch_fp_owner_load_mem, expected,
            AI_ARCH_FP_OWNER_BYTES, &bad_index),
        "first_bad_byte=%u actual=0x%02x expected=0x%02x",
        bad_index,
        (unsigned)ai_arch_fp_owner_load_mem[bad_index],
        (unsigned)expected[bad_index]
    );

    TEST_END();
}

bool ai_arch_fp_store_producer_forward_cross16_owner_corner()
{
    TEST_START();

    enum {
        STORE_BASE_OFF = 32U + 9U,
        FSW_OFF = STORE_BASE_OFF + 4U,
        FSW2_OFF = STORE_BASE_OFF + 6U,
        OBS_COUNT = 12U,
    };
    const uint64_t fflags_sentinel = 0x12ULL;
    const uint64_t frm_sentinel = 0x1ULL;
    const uint64_t fsd_payload = 0x7ff8123456789abcULL;
    const uint32_t fsw_payload = 0x80000001U;
    const uint32_t sw_payload = 0x7fc12345U;
    const uint64_t sd_payload = 0x0123456789abcdefULL;
    const uint32_t fsw2_payload = 0xdeadc0deU;
    const uint64_t after_fsd = fsd_payload;
    const uint64_t after_fsw = 0x8000000156789abcULL;
    const uint64_t after_sw = 0x800000017fc12345ULL;
    const uint64_t after_sd = sd_payload;
    const uint64_t after_fsw2 = 0xc0de456789abcdefULL;
    uint8_t expected[AI_ARCH_FP_OWNER_BYTES];
    uint64_t obs[OBS_COUNT];
    unsigned bad_index = 0U;

    goto_priv(PRIV_M);
    ai_arch_fp_owner_enable_fp();
    ai_arch_fp_owner_write_fcsr(fflags_sentinel, frm_sentinel);

    ai_arch_fp_owner_fill_pair(ai_arch_fp_owner_store_mem, expected,
        AI_ARCH_FP_OWNER_BYTES, 0x93U);
    ai_arch_fp_owner_fill_u64(obs, OBS_COUNT, 0xbad1000000000000ULL);

    ai_arch_fp_owner_put_le64(expected, STORE_BASE_OFF, fsd_payload);
    ai_arch_fp_owner_put_le32(expected, FSW_OFF, fsw_payload);
    ai_arch_fp_owner_put_le32(expected, STORE_BASE_OFF, sw_payload);
    ai_arch_fp_owner_put_le64(expected, STORE_BASE_OFF, sd_payload);
    ai_arch_fp_owner_put_le32(expected, FSW2_OFF, fsw2_payload);

    TEST_SETUP_EXCEPT();
    ai_arch_fp_owner_store_producer_sequence(
        (uintptr_t)&ai_arch_fp_owner_store_mem[STORE_BASE_OFF],
        (uintptr_t)&ai_arch_fp_owner_store_mem[FSW_OFF],
        (uintptr_t)&ai_arch_fp_owner_store_mem[FSW2_OFF],
        fsd_payload, fsw_payload, sw_payload, sd_payload, fsw2_payload,
        obs);

    TEST_ASSERT("FP store owner: fsd/fsw/sw/sd/fsw producer switch forwards the current raw bytes and width template to immediate FP and integer observers",
        excpt.triggered == false &&
        obs[0] == after_fsd &&
        obs[1] == after_fsd &&
        obs[2] == after_fsw &&
        obs[3] == ai_arch_fp_owner_flw_boxed(fsw_payload) &&
        obs[4] == (uint64_t)fsw_payload &&
        obs[5] == ai_arch_fp_owner_lw_signed(fsw_payload) &&
        obs[6] == after_sw &&
        obs[7] == after_sw &&
        obs[8] == after_sd &&
        obs[9] == after_sd &&
        obs[10] == after_fsw2 &&
        obs[11] == ai_arch_fp_owner_flw_boxed(fsw2_payload) &&
        ai_arch_fp_owner_fcsr_matches(fflags_sentinel, frm_sentinel)
    );

    TEST_ASSERT("FP store owner: cross16 fsw overlays only its four bytes after sd and preserves the exact final byte image plus surrounding guards",
        ai_arch_fp_owner_bytes_equal(ai_arch_fp_owner_store_mem, expected,
            AI_ARCH_FP_OWNER_BYTES, &bad_index),
        "first_bad_byte=%u actual=0x%02x expected=0x%02x",
        bad_index,
        (unsigned)ai_arch_fp_owner_store_mem[bad_index],
        (unsigned)expected[bad_index]
    );

    TEST_END();
}
#pragma GCC pop_options
