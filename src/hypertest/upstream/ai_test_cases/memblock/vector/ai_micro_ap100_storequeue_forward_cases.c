#include "../boundary/ai_micro_cross_page_fault_common.h"
#include <instructions.h>
#include <stdint.h>
#include <string.h>

#define AI_AP100_WINDOW_BYTES 24U

static inline uint64_t ai_ap100_pack_le64(const uint8_t *bytes,
    unsigned start)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= (uint64_t)bytes[start + i] << (i * 8U);
    }
    return value;
}

static inline uint32_t ai_ap100_pack_le32(const uint8_t *bytes,
    unsigned start)
{
    return ((uint32_t)bytes[start]) |
        ((uint32_t)bytes[start + 1U] << 8U) |
        ((uint32_t)bytes[start + 2U] << 16U) |
        ((uint32_t)bytes[start + 3U] << 24U);
}

static inline uint16_t ai_ap100_pack_le16(const uint8_t *bytes,
    unsigned start)
{
    return (uint16_t)bytes[start] |
        ((uint16_t)bytes[start + 1U] << 8U);
}

static inline void ai_ap100_store_le64(uint8_t *bytes, unsigned start,
    uint64_t value)
{
    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        bytes[start + i] = (uint8_t)(value >> (i * 8U));
    }
}

static inline bool ai_ap100_bytes_match(uintptr_t paddr,
    const uint8_t *expected, unsigned len, unsigned *bad_index,
    uint8_t *bad_actual, uint8_t *bad_expected)
{
    for (unsigned i = 0; i < len; ++i) {
        const uint8_t actual = read8(paddr + i);

        if (actual != expected[i]) {
            *bad_index = i;
            *bad_actual = actual;
            *bad_expected = expected[i];
            return false;
        }
    }

    *bad_index = len;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static inline void ai_ap100_seed_linear(uintptr_t paddr, uint8_t seed,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, (uint8_t)(seed + i * 3U));
    }
}

static inline uint64_t ai_ap100_sd_then_ld(uint8_t *store_addr,
    const uint8_t *load_addr, uint64_t value)
{
    uint64_t observed = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[value], 0(%[store_addr])\n\t"
        "ld %[observed], 0(%[load_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [observed] "=&r"(observed)
        : [store_addr] "r"(store_addr),
          [load_addr] "r"(load_addr),
          [value] "r"(value)
        : "memory");

    return observed;
}

static inline void ai_ap100_sd_then_lane_loads(uint8_t *store_addr,
    const uint8_t *load_base, uint64_t value, uint64_t *ld0,
    uint64_t *lbu0, uint64_t *lbu1, uint64_t *lhu7, uint64_t *lw5)
{
    uint64_t observed_ld0 = 0ULL;
    uint64_t observed_lbu0 = 0ULL;
    uint64_t observed_lbu1 = 0ULL;
    uint64_t observed_lhu7 = 0ULL;
    uint64_t observed_lw5 = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[value], 0(%[store_addr])\n\t"
        "ld %[observed_ld0], 0(%[load_base])\n\t"
        "lbu %[observed_lbu0], 0(%[load_base])\n\t"
        "lbu %[observed_lbu1], 1(%[load_base])\n\t"
        "lhu %[observed_lhu7], 7(%[load_base])\n\t"
        "lw %[observed_lw5], 5(%[load_base])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [observed_ld0] "=&r"(observed_ld0),
          [observed_lbu0] "=&r"(observed_lbu0),
          [observed_lbu1] "=&r"(observed_lbu1),
          [observed_lhu7] "=&r"(observed_lhu7),
          [observed_lw5] "=&r"(observed_lw5)
        : [store_addr] "r"(store_addr),
          [load_base] "r"(load_base),
          [value] "r"(value)
        : "memory");

    *ld0 = observed_ld0;
    *lbu0 = observed_lbu0;
    *lbu1 = observed_lbu1;
    *lhu7 = observed_lhu7;
    *lw5 = observed_lw5;
}

static inline void ai_ap100_vse32_then_ld(const uint32_t *src,
    uint32_t *dst, const uint8_t *load_addr, uint64_t *observed)
{
    uint64_t value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vse32.v v6, (%[dst])\n\t"
        "ld %[value], 0(%[load_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [value] "=&r"(value)
        : [src] "r"(src),
          [dst] "r"(dst),
          [load_addr] "r"(load_addr),
          [vl] "r"(2U)
        : "t0", "memory");

    *observed = value;
}

static inline void ai_ap100_dense_store_pressure(uint64_t *base,
    uint64_t seed)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[v0], 0(%[base])\n\t"
        "sd %[v1], 8(%[base])\n\t"
        "sd %[v2], 16(%[base])\n\t"
        "sd %[v3], 24(%[base])\n\t"
        "sd %[v4], 32(%[base])\n\t"
        "sd %[v5], 40(%[base])\n\t"
        "sd %[v6], 48(%[base])\n\t"
        "sd %[v7], 56(%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [v0] "r"(seed ^ 0x0102030405060708ULL),
          [v1] "r"(seed ^ 0x1112131415161718ULL),
          [v2] "r"(seed ^ 0x2122232425262728ULL),
          [v3] "r"(seed ^ 0x3132333435363738ULL),
          [v4] "r"(seed ^ 0x4142434445464748ULL),
          [v5] "r"(seed ^ 0x5152535455565758ULL),
          [v6] "r"(seed ^ 0x6162636465666768ULL),
          [v7] "r"(seed ^ 0x7172737475767778ULL)
        : "memory");
}

bool ai_micro_ap100_same16b_unaligned_sd_ld_overlap_no_direct_forward_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x200U;
    const uintptr_t base_phys = phys_page_base(page) + 0x200U;
    const uint64_t store_value = 0x1122334455667788ULL;
    uint8_t expected[AI_AP100_WINDOW_BYTES];
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;
    uint64_t observed = 0ULL;
    uint64_t expected_load = 0ULL;

    for (unsigned i = 0; i < AI_AP100_WINDOW_BYTES; ++i) {
        expected[i] = (uint8_t)(0x40U + i * 3U);
    }
    ai_ap100_store_le64(expected, 1U, store_value);
    expected_load = ai_ap100_pack_le64(expected, 0U);

    goto_priv(PRIV_M);
    ai_ap100_seed_linear(base_phys, 0x40U, AI_AP100_WINDOW_BYTES);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_ASSERT("AP-100 same-16B probe uses sd at byte offset1 so the store is non-aligned but does not cross the 16-byte forwarding block",
        ((base_vaddr + 1U) & 0x7ULL) != 0ULL &&
        ((base_vaddr + 1U) >> 4) == ((base_vaddr + 8U) >> 4)
    );

    TEST_SETUP_EXCEPT();
    observed = ai_ap100_sd_then_ld((uint8_t *)(base_vaddr + 1U),
        (const uint8_t *)base_vaddr, store_value);
    TEST_ASSERT("a younger overlapping ld must not consume unsupported direct-forward data from the same-16B non-aligned sd producer",
        excpt.triggered == false &&
        observed == expected_load,
        "observed=0x%lx expected=0x%lx",
        observed, expected_load
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-16B non-aligned sd eventually updates exactly bytes1-8 and keeps neighboring byte lanes unchanged",
        ai_ap100_bytes_match(base_phys, expected, AI_AP100_WINDOW_BYTES,
            &bad_index, &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap100_cross16b_unaligned_sd_ld_overlap_still_blocks_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x310U;
    const uintptr_t base_phys = phys_page_base(page) + 0x310U;
    const uint64_t store_value = 0xa1b2c3d4e5f60718ULL;
    uint8_t expected[AI_AP100_WINDOW_BYTES];
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;
    uint64_t observed = 0ULL;
    uint64_t expected_load = 0ULL;

    for (unsigned i = 0; i < AI_AP100_WINDOW_BYTES; ++i) {
        expected[i] = (uint8_t)(0x80U + i * 3U);
    }
    ai_ap100_store_le64(expected, 9U, store_value);
    expected_load = ai_ap100_pack_le64(expected, 8U);

    goto_priv(PRIV_M);
    ai_ap100_seed_linear(base_phys, 0x80U, AI_AP100_WINDOW_BYTES);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_ASSERT("AP-100 cross-16B control uses sd at byte offset9 so the non-aligned store spans two 16-byte forwarding blocks",
        ((base_vaddr + 9U) & 0x7ULL) != 0ULL &&
        ((base_vaddr + 9U) >> 4) != ((base_vaddr + 16U) >> 4)
    );

    TEST_SETUP_EXCEPT();
    observed = ai_ap100_sd_then_ld((uint8_t *)(base_vaddr + 9U),
        (const uint8_t *)(base_vaddr + 8U), store_value);
    TEST_ASSERT("the older cross-16B non-aligned sd remains a replay/dependency source rather than a direct-forward producer for the overlapping ld",
        excpt.triggered == false &&
        observed == expected_load,
        "observed=0x%lx expected=0x%lx",
        observed, expected_load
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-16B non-aligned sd eventually writes bytes9-16 and preserves both sides of the 24-byte observation window",
        ai_ap100_bytes_match(base_phys, expected, AI_AP100_WINDOW_BYTES,
            &bad_index, &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap100_aligned_sd_ld_forwarding_control_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x420U;
    const uintptr_t base_phys = phys_page_base(page) + 0x420U;
    const uint64_t store_value = 0x0badc0ffee123456ULL;
    uint8_t expected[AI_AP100_WINDOW_BYTES];
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;
    uint64_t observed = 0ULL;

    for (unsigned i = 0; i < AI_AP100_WINDOW_BYTES; ++i) {
        expected[i] = (uint8_t)(0x20U + i);
    }
    ai_ap100_store_le64(expected, 8U, store_value);

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_AP100_WINDOW_BYTES; ++i) {
        write8(base_phys + i, (uint8_t)(0x20U + i));
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_ASSERT("AP-100 aligned control uses an 8-byte aligned sd producer, which should remain eligible for ordinary StoreQueue forwarding",
        ((base_vaddr + 8U) & 0x7ULL) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    observed = ai_ap100_sd_then_ld((uint8_t *)(base_vaddr + 8U),
        (const uint8_t *)(base_vaddr + 8U), store_value);
    TEST_ASSERT("an aligned sd followed by an overlapping ld still observes the new value, proving AP-100 does not disable the normal aligned forwarding fast path",
        excpt.triggered == false &&
        observed == store_value,
        "observed=0x%lx expected=0x%lx",
        observed, store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned control writes exactly the aligned 8-byte store payload and leaves the rest of the window unchanged",
        ai_ap100_bytes_match(base_phys, expected, AI_AP100_WINDOW_BYTES,
            &bad_index, &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap100_vector_unitstride_same16b_unaligned_store_load_no_direct_forward_corner()
{
    TEST_START();

    const enum test_page dst_page = VSRWX_GRWX;
    const enum test_page src_page = SWITCH1;
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x500U;
    const uintptr_t dst_phys = phys_page_base(dst_page) + 0x500U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x540U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x540U;
    const uint8_t initial[AI_AP100_WINDOW_BYTES] = {
        0x50U, 0x51U, 0x52U, 0x53U, 0x54U, 0x55U, 0x56U, 0x57U,
        0x58U, 0x59U, 0x5aU, 0x5bU, 0x5cU, 0x5dU, 0x5eU, 0x5fU,
        0x60U, 0x61U, 0x62U, 0x63U, 0x64U, 0x65U, 0x66U, 0x67U
    };
    const uint8_t src_pattern[8] = {
        0xd0U, 0xd1U, 0xd2U, 0xd3U, 0xe0U, 0xe1U, 0xe2U, 0xe3U
    };
    uint8_t expected[AI_AP100_WINDOW_BYTES];
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;
    uint64_t observed = 0ULL;
    uint64_t expected_load = 0ULL;

    memcpy(expected, initial, sizeof(expected));
    memcpy(expected + 1U, src_pattern, sizeof(src_pattern));
    expected_load = ai_ap100_pack_le64(expected, 0U);

    goto_priv(PRIV_M);
    ai_micro_write_bytes(dst_phys, initial, sizeof(initial));
    ai_micro_write_bytes(src_phys, src_pattern, sizeof(src_pattern));

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_ASSERT("AP-100 vector control uses vse32.v vl=2 at byte offset1, a same-16B non-aligned UnitStride store footprint",
        ((dst_vaddr + 1U) & 0x3ULL) != 0ULL &&
        ((dst_vaddr + 1U) >> 4) == ((dst_vaddr + 8U) >> 4)
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap100_vse32_then_ld((const uint32_t *)src_vaddr,
        (uint32_t *)(dst_vaddr + 1U), (const uint8_t *)dst_vaddr,
        &observed);
    TEST_ASSERT("a younger ld overlapping a same-16B non-aligned UnitStride store must observe the architectural merged image, not stale unsupported direct-forward data",
        excpt.triggered == false &&
        observed == expected_load,
        "observed=0x%lx expected=0x%lx",
        observed, expected_load
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-16B non-aligned UnitStride store eventually updates only bytes1-8 with the two e32 source elements",
        ai_ap100_bytes_match(dst_phys, expected, AI_AP100_WINDOW_BYTES,
            &bad_index, &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap100_unaligned_store_byte_lane_overlap_matrix_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x620U;
    const uintptr_t base_phys = phys_page_base(page) + 0x620U;
    const uint64_t store_value = 0x8877665544332211ULL;
    uint8_t expected[AI_AP100_WINDOW_BYTES];
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;
    uint64_t observed_ld0 = 0ULL;
    uint64_t observed_lbu0 = 0ULL;
    uint64_t observed_lbu1 = 0ULL;
    uint64_t observed_lhu7 = 0ULL;
    uint64_t observed_lw5 = 0ULL;
    uint64_t expected_ld0 = 0ULL;
    uint64_t expected_lbu0 = 0ULL;
    uint64_t expected_lbu1 = 0ULL;
    uint64_t expected_lhu7 = 0ULL;
    uint64_t expected_lw5 = 0ULL;

    for (unsigned i = 0; i < AI_AP100_WINDOW_BYTES; ++i) {
        expected[i] = (uint8_t)(0x90U + i);
    }
    ai_ap100_store_le64(expected, 1U, store_value);
    expected_ld0 = ai_ap100_pack_le64(expected, 0U);
    expected_lbu0 = expected[0];
    expected_lbu1 = expected[1];
    expected_lhu7 = ai_ap100_pack_le16(expected, 7U);
    expected_lw5 = ai_ap100_pack_le32(expected, 5U);

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_AP100_WINDOW_BYTES; ++i) {
        write8(base_phys + i, (uint8_t)(0x90U + i));
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_ap100_sd_then_lane_loads((uint8_t *)(base_vaddr + 1U),
        (const uint8_t *)base_vaddr, store_value, &observed_ld0,
        &observed_lbu0, &observed_lbu1, &observed_lhu7, &observed_lw5);
    TEST_ASSERT("AP-100 byte-lane matrix: non-overlapping byte0 remains old while overlapping byte/half/word/doubleword loads observe the architectural merged image",
        excpt.triggered == false &&
        observed_ld0 == expected_ld0 &&
        observed_lbu0 == expected_lbu0 &&
        observed_lbu1 == expected_lbu1 &&
        observed_lhu7 == expected_lhu7 &&
        (uint32_t)observed_lw5 == (uint32_t)expected_lw5,
        "ld0=0x%lx exp=0x%lx lbu0=0x%lx exp=0x%lx lbu1=0x%lx exp=0x%lx lhu7=0x%lx exp=0x%lx lw5=0x%lx exp=0x%lx",
        observed_ld0, expected_ld0, observed_lbu0, expected_lbu0,
        observed_lbu1, expected_lbu1, observed_lhu7, expected_lhu7,
        observed_lw5, expected_lw5
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-100 byte-lane matrix leaves the complete 24-byte backing image equal to the expected single non-aligned sd overlay",
        ai_ap100_bytes_match(base_phys, expected, AI_AP100_WINDOW_BYTES,
            &bad_index, &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap100_unaligned_store_after_replay_pressure_no_stale_fast_forward_corner()
{
    TEST_START();

    const enum test_page fault_page = SWITCH1;
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page pressure_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x100U;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x740U;
    const uintptr_t target_phys = phys_page_base(target_page) + 0x740U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x780U;
    const uintptr_t pressure_phys = phys_page_base(pressure_page) + 0x780U;
    const uint64_t store_value = 0x5a6b7c8d9eaf1021ULL;
    const uint64_t pressure_seed = 0x13572468ace0bdf9ULL;
    uint8_t expected[AI_AP100_WINDOW_BYTES];
    unsigned bad_index = 0U;
    uint8_t bad_actual = 0U;
    uint8_t bad_expected = 0U;
    uint64_t observed = 0ULL;
    uint64_t expected_load = 0ULL;

    for (unsigned i = 0; i < AI_AP100_WINDOW_BYTES; ++i) {
        expected[i] = (uint8_t)(0x30U + i * 5U);
    }
    ai_ap100_store_le64(expected, 1U, store_value);
    expected_load = ai_ap100_pack_le64(expected, 0U);

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_AP100_WINDOW_BYTES; ++i) {
        write8(target_phys + i, (uint8_t)(0x30U + i * 5U));
    }
    for (unsigned i = 0; i < 8U; ++i) {
        write64(pressure_phys + i * sizeof(uint64_t), 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    (void)ld(fault_vaddr);
    TEST_ASSERT("AP-100 replay-pressure setup first creates a precise load page fault redirect before the later non-aligned store/load overlap probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr,
        "triggered=%d cause=0x%lx tval=0x%lx expected=0x%lx",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr
    );

    ai_micro_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_ap100_dense_store_pressure((uint64_t *)pressure_vaddr, pressure_seed);
    observed = ai_ap100_sd_then_ld((uint8_t *)(target_vaddr + 1U),
        (const uint8_t *)target_vaddr, store_value);
    TEST_ASSERT("after a redirect/replay episode and dense store pressure, the same-16B non-aligned sd/ld overlap still cannot use stale unsupported fast-forward data",
        excpt.triggered == false &&
        observed == expected_load,
        "observed=0x%lx expected=0x%lx",
        observed, expected_load
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-100 replay-pressure probe commits the target overlay and the dense pressure stores without corrupting either window",
        ai_ap100_bytes_match(target_phys, expected, AI_AP100_WINDOW_BYTES,
            &bad_index, &bad_actual, &bad_expected) &&
        read64(pressure_phys) == (pressure_seed ^ 0x0102030405060708ULL) &&
        read64(pressure_phys + 56U) == (pressure_seed ^ 0x7172737475767778ULL),
        "bad_index=%u actual=0x%x expected=0x%x pressure0=0x%lx pressure7=0x%lx",
        bad_index, bad_actual, bad_expected,
        read64(pressure_phys), read64(pressure_phys + 56U)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
