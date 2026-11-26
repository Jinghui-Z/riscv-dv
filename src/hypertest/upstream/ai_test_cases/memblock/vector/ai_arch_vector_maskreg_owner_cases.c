#include "../boundary/ai_micro_cross_page_fault_common.h"
#include <rvh_test.h>
#include <stdint.h>

#define AI_ARCH_MASKREG_E8_VL 17U
#define AI_ARCH_MASKREG_E8_BYTES 24U
#define AI_ARCH_MASKREG_E16_VL 13U
#define AI_ARCH_MASKREG_E16_ELEMS 16U
#define AI_ARCH_MASKREG_DUMP_BYTES 8U
#define AI_ARCH_VSM_FAULT_VL 9U
#define AI_ARCH_VSM_FAULT_BYTES 2U
#define AI_ARCH_VSM_FOLLOWUP_BYTES 8U
#define AI_ARCH_VLM_FAULT_VL 9U
#define AI_ARCH_VLM_MASK_BYTES 2U
#define AI_ARCH_VLM_DUMP_BYTES 4U
#define AI_ARCH_VLM_CONSUME_BYTES 12U
#define AI_ARCH_MASKREG_SCALAR_ATTR \
    __attribute__((noinline, optimize("no-tree-vectorize", \
                                      "no-tree-loop-distribute-patterns")))

static uint8_t ai_arch_maskreg_e8_mask[4] __attribute__((aligned(8)));
static uint8_t ai_arch_maskreg_e8_raw_dump[AI_ARCH_MASKREG_DUMP_BYTES]
    __attribute__((aligned(8)));
static uint8_t ai_arch_maskreg_e8_src[AI_ARCH_MASKREG_E8_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_arch_maskreg_e8_dst[AI_ARCH_MASKREG_E8_BYTES]
    __attribute__((aligned(64)));
static uint16_t ai_arch_maskreg_e16_mask[2] __attribute__((aligned(8)));
static uint8_t ai_arch_maskreg_e16_raw_dump[AI_ARCH_MASKREG_DUMP_BYTES]
    __attribute__((aligned(8)));
static uint16_t ai_arch_maskreg_e16_src[AI_ARCH_MASKREG_E16_ELEMS]
    __attribute__((aligned(64)));
static uint16_t ai_arch_maskreg_e16_dst[AI_ARCH_MASKREG_E16_ELEMS]
    __attribute__((aligned(64)));

static inline void ai_arch_maskreg_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static AI_ARCH_MASKREG_SCALAR_ATTR void ai_arch_maskreg_store_byte(
    uint8_t *base, unsigned index, uint8_t value)
{
    volatile uint8_t *ptr = (volatile uint8_t *)base;

    ptr[index] = value;
}

static AI_ARCH_MASKREG_SCALAR_ATTR void ai_arch_maskreg_store_hword(
    uint16_t *base, unsigned index, uint16_t value)
{
    volatile uint16_t *ptr = (volatile uint16_t *)base;

    ptr[index] = value;
}

static AI_ARCH_MASKREG_SCALAR_ATTR bool ai_arch_maskreg_byte_equal(
    const uint8_t *actual,
    const uint8_t *expected, unsigned bytes, unsigned *bad_index)
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

static AI_ARCH_MASKREG_SCALAR_ATTR bool ai_arch_maskreg_hword_equal(
    const uint16_t *actual,
    const uint16_t *expected, unsigned count, unsigned *bad_index)
{
    volatile const uint16_t *actual_v = (volatile const uint16_t *)actual;
    volatile const uint16_t *expected_v = (volatile const uint16_t *)expected;

    for (unsigned i = 0; i < count; ++i) {
        if (actual_v[i] != expected_v[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = 0U;
    return true;
}

static AI_ARCH_MASKREG_SCALAR_ATTR unsigned ai_arch_maskreg_bit_is_set(
    const uint8_t *mask, unsigned bit)
{
    volatile const uint8_t *mask_v = (volatile const uint8_t *)mask;

    return (unsigned)((mask_v[bit >> 3] >> (bit & 7U)) & 1U);
}

static inline void ai_arch_maskreg_e8_round(const uint8_t *mask,
    const uint8_t *src, uint8_t *raw_dump, uint8_t *dst,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m2, tu, mu\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vsm.v v0, (%[raw_dump])\n\t"
        "vle8.v v8, (%[src])\n\t"
        "vse8.v v8, (%[dst]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [mask] "r"(mask),
          [src] "r"(src),
          [raw_dump] "r"(raw_dump),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_MASKREG_E8_VL)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_arch_maskreg_e16_round(const void *mask,
    const uint16_t *src, uint8_t *raw_dump, uint16_t *dst,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m2, tu, mu\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vsm.v v0, (%[raw_dump])\n\t"
        "vle16.v v8, (%[src])\n\t"
        "vse16.v v8, (%[dst]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [mask] "r"(mask),
          [src] "r"(src),
          [raw_dump] "r"(raw_dump),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_MASKREG_E16_VL)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_arch_vsm_fault_round_start(const uint8_t *mask,
    uint8_t *dst, unsigned vl, unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, %[start]\n\t"
        "vsm.v v0, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [mask] "r"(mask),
          [dst] "r"(dst),
          [vl] "r"(vl),
          [start] "r"(start)
        : "t0", "memory");
}

static inline void ai_arch_vsm_followup_vse8(const uint8_t *src,
    uint8_t *dst)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v8, (%[src])\n\t"
        "vse8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_ARCH_VSM_FOLLOWUP_BYTES)
        : "t0", "memory");
}

static inline void ai_arch_vlm_seed_then_fault(const uint8_t *old_mask,
    const uint8_t *fault_mask, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlm.v v0, (%[old_mask])\n\t"
        "csrw vstart, zero\n\t"
        "vlm.v v0, (%[fault_mask])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [old_mask] "r"(old_mask),
          [fault_mask] "r"(fault_mask),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_arch_vlm_retry_from_vstart(const uint8_t *mask,
    unsigned vl, unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [mask] "r"(mask),
          [vl] "r"(vl),
          [start] "r"(start)
        : "t0", "memory");
}

static inline void ai_arch_vlm_dump_and_consume_v0(const uint8_t *src,
    uint8_t *raw_dump, uint8_t *dst, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vsm.v v0, (%[raw_dump])\n\t"
        "vle8.v v8, (%[src])\n\t"
        "vse8.v v8, (%[dst]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [raw_dump] "r"(raw_dump),
          [dst] "r"(dst),
          [vl] "r"(vl)
        : "t0", "memory");
}

static AI_ARCH_MASKREG_SCALAR_ATTR void ai_arch_vlm_build_consumer_expected(
    uint8_t *expected, const uint8_t *seed, const uint8_t *src,
    const uint8_t *mask, unsigned vl)
{
    for (unsigned i = 0; i < AI_ARCH_VLM_CONSUME_BYTES; ++i) {
        uint8_t value = seed[i];
        if (i < vl && ai_arch_maskreg_bit_is_set(mask, i) != 0U) {
            value = src[i];
        }
        expected[i] = value;
    }
}

#pragma GCC push_options
#pragma GCC optimize ("no-tree-vectorize", "no-tree-loop-distribute-patterns")
bool ai_arch_maskreg_bytepack_consumer_owner_corner()
{
    TEST_START();

    uint8_t expected_e8_raw[AI_ARCH_MASKREG_DUMP_BYTES];
    uint8_t expected_e8_dst[AI_ARCH_MASKREG_E8_BYTES];
    uint8_t expected_e16_raw[AI_ARCH_MASKREG_DUMP_BYTES];
    uint16_t expected_e16_dst[AI_ARCH_MASKREG_E16_ELEMS];
    unsigned observed_vl = 0U;
    unsigned bad_index = 0U;

    ai_arch_maskreg_enable_vector_state();

    ai_arch_maskreg_e8_mask[0] = 0xb5U;
    ai_arch_maskreg_e8_mask[1] = 0x4aU;
    ai_arch_maskreg_e8_mask[2] = 0xe3U;
    ai_arch_maskreg_e8_mask[3] = 0x7cU;

    for (unsigned i = 0; i < AI_ARCH_MASKREG_DUMP_BYTES; ++i) {
        ai_arch_maskreg_store_byte(ai_arch_maskreg_e8_raw_dump, i, 0x6dU);
        ai_arch_maskreg_store_byte(expected_e8_raw, i, 0x6dU);
        ai_arch_maskreg_store_byte(ai_arch_maskreg_e16_raw_dump, i, 0x8eU);
        ai_arch_maskreg_store_byte(expected_e16_raw, i, 0x8eU);
    }
    expected_e8_raw[0] = ai_arch_maskreg_e8_mask[0];
    expected_e8_raw[1] = ai_arch_maskreg_e8_mask[1];
    expected_e8_raw[2] = ai_arch_maskreg_e8_mask[2];

    for (unsigned i = 0; i < AI_ARCH_MASKREG_E8_BYTES; ++i) {
        uint8_t src_value = (uint8_t)(0x30U + i * 3U);
        uint8_t dst_value = 0xa6U;

        ai_arch_maskreg_store_byte(ai_arch_maskreg_e8_src, i, src_value);
        ai_arch_maskreg_store_byte(ai_arch_maskreg_e8_dst, i, dst_value);
        if (i < AI_ARCH_MASKREG_E8_VL &&
            ai_arch_maskreg_bit_is_set(ai_arch_maskreg_e8_mask, i)) {
            dst_value = src_value;
        }
        ai_arch_maskreg_store_byte(expected_e8_dst, i, dst_value);
    }

    TEST_SETUP_EXCEPT();
    ai_arch_maskreg_e8_round(ai_arch_maskreg_e8_mask,
        ai_arch_maskreg_e8_src, ai_arch_maskreg_e8_raw_dump,
        ai_arch_maskreg_e8_dst, &observed_vl);
    TEST_ASSERT("vlm/vsm e8,m2 vl=17 stores exactly ceil(vl/8) raw mask bytes and the immediate v0.t consumer ignores last-byte tail bits",
        excpt.triggered == false &&
        observed_vl == AI_ARCH_MASKREG_E8_VL &&
        ai_arch_maskreg_byte_equal(ai_arch_maskreg_e8_raw_dump,
            expected_e8_raw, AI_ARCH_MASKREG_DUMP_BYTES, &bad_index) &&
        ai_arch_maskreg_byte_equal(ai_arch_maskreg_e8_dst,
            expected_e8_dst, AI_ARCH_MASKREG_E8_BYTES, &bad_index),
        "vl=%u bad=%u raw=%02x,%02x,%02x,%02x dst_bad=0x%x exp=0x%x",
        observed_vl, bad_index, ai_arch_maskreg_e8_raw_dump[0],
        ai_arch_maskreg_e8_raw_dump[1], ai_arch_maskreg_e8_raw_dump[2],
        ai_arch_maskreg_e8_raw_dump[3], ai_arch_maskreg_e8_dst[bad_index],
        expected_e8_dst[bad_index]
    );

    ((uint8_t *)ai_arch_maskreg_e16_mask)[0] = 0x39U;
    ((uint8_t *)ai_arch_maskreg_e16_mask)[1] = 0xf4U;
    ((uint8_t *)ai_arch_maskreg_e16_mask)[2] = 0x2bU;
    ((uint8_t *)ai_arch_maskreg_e16_mask)[3] = 0xd0U;
    expected_e16_raw[0] = ((uint8_t *)ai_arch_maskreg_e16_mask)[0];
    expected_e16_raw[1] = ((uint8_t *)ai_arch_maskreg_e16_mask)[1];

    for (unsigned i = 0; i < AI_ARCH_MASKREG_E16_ELEMS; ++i) {
        uint16_t src_value = (uint16_t)(0x6100U + 0x21U * i);
        uint16_t dst_value = (uint16_t)(0xd000U | i);

        ai_arch_maskreg_store_hword(ai_arch_maskreg_e16_src, i, src_value);
        ai_arch_maskreg_store_hword(ai_arch_maskreg_e16_dst, i, dst_value);
        if (i < AI_ARCH_MASKREG_E16_VL &&
            ai_arch_maskreg_bit_is_set(
                (const uint8_t *)ai_arch_maskreg_e16_mask, i)) {
            dst_value = src_value;
        }
        ai_arch_maskreg_store_hword(expected_e16_dst, i, dst_value);
    }

    TEST_SETUP_EXCEPT();
    ai_arch_maskreg_e16_round(ai_arch_maskreg_e16_mask,
        ai_arch_maskreg_e16_src, ai_arch_maskreg_e16_raw_dump,
        ai_arch_maskreg_e16_dst, &observed_vl);
    TEST_ASSERT("back-to-back vlm/vsm after e8 round switches to e16,m2 vl=13 without inheriting old mask-register byte count or v0 consumer bits",
        excpt.triggered == false &&
        observed_vl == AI_ARCH_MASKREG_E16_VL &&
        ai_arch_maskreg_byte_equal(ai_arch_maskreg_e16_raw_dump,
            expected_e16_raw, AI_ARCH_MASKREG_DUMP_BYTES, &bad_index) &&
        ai_arch_maskreg_hword_equal(ai_arch_maskreg_e16_dst,
            expected_e16_dst, AI_ARCH_MASKREG_E16_ELEMS, &bad_index),
        "vl=%u bad=%u raw=%02x,%02x,%02x dst_bad=0x%x exp=0x%x",
        observed_vl, bad_index, ai_arch_maskreg_e16_raw_dump[0],
        ai_arch_maskreg_e16_raw_dump[1], ai_arch_maskreg_e16_raw_dump[2],
        ai_arch_maskreg_e16_dst[bad_index], expected_e16_dst[bad_index]
    );

    TEST_END();
}

bool ai_arch_vsm_mask_store_tail_fault_repair_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page follow_page = SWITCH1;
    const uintptr_t dst_vaddr = hs_page_base(low_page) + PAGE_SIZE - 1U;
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 1U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t follow_vaddr = hs_page_base(follow_page) + 0x540U;
    const uintptr_t follow_src_vaddr = hs_page_base(follow_page) + 0x560U;
    const uintptr_t mask_vaddr = hs_page_base(follow_page) + 0x580U;
    const uintptr_t follow_phys = phys_page_base(follow_page) + 0x540U;
    const uintptr_t follow_src_phys = phys_page_base(follow_page) + 0x560U;
    const uintptr_t mask_phys = phys_page_base(follow_page) + 0x580U;
    const uintptr_t before_phys = low_phys - 1U;
    const uintptr_t after_phys = high_phys + 2U;
    const uintptr_t follow_before_phys = follow_phys - 1U;
    const uintptr_t follow_after_phys =
        follow_phys + AI_ARCH_VSM_FOLLOWUP_BYTES;
    const uint8_t expected_mask[AI_ARCH_VSM_FAULT_BYTES] = {
        0xb5U, 0x4aU
    };
    const uint8_t old_high[2] = {
        0x8dU, 0x8eU
    };
    const uint8_t follow_seed[AI_ARCH_VSM_FOLLOWUP_BYTES] = {
        0xc0U, 0xc1U, 0xc2U, 0xc3U,
        0xc4U, 0xc5U, 0xc6U, 0xc7U
    };
    const uint8_t follow_new[AI_ARCH_VSM_FOLLOWUP_BYTES] = {
        0x31U, 0x42U, 0x53U, 0x64U,
        0x75U, 0x86U, 0x97U, 0xa8U
    };
    const uint8_t before = 0x5aU;
    const uint8_t after = 0xa5U;
    const uint8_t follow_before = 0x66U;
    const uint8_t follow_after = 0x99U;
    unsigned fault_vstart = 0U;
    unsigned retry_vstart = 0U;

    ai_arch_maskreg_enable_vector_state();

    goto_priv(PRIV_M);
    write8(low_phys, 0x6dU);
    ai_micro_write_bytes(high_phys, old_high, sizeof(old_high));
    ai_micro_write_bytes(follow_phys, follow_seed, sizeof(follow_seed));
    ai_micro_write_bytes(follow_src_phys, follow_new, sizeof(follow_new));
    write8(mask_phys, expected_mask[0]);
    write8(mask_phys + 1U, expected_mask[1]);
    write8(mask_phys + 2U, 0xe3U);
    write8(mask_phys + 3U, 0x7cU);
    write8(before_phys, before);
    write8(after_phys, after);
    write8(follow_before_phys, follow_before);
    write8(follow_after_phys, follow_after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(follow_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the vsm.v tail-fault probe places ceil(vl=9/8)=2 mask bytes across an HS page boundary so byte0 is prefix and byte1 is the no-write fault byte",
        (dst_vaddr & 0xfffULL) == PAGE_SIZE - 1U &&
        high_fault_vaddr == dst_vaddr + 1U
    );

    TEST_SETUP_EXCEPT();
    ai_arch_vsm_fault_round_start((const uint8_t *)mask_vaddr,
        (uint8_t *)dst_vaddr, AI_ARCH_VSM_FAULT_VL, 0U);
    fault_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("vsm.v with vl=9 writes only the first raw mask byte before the second mask byte faults on the high page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_fault_vaddr &&
        fault_vstart == 1U,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=%u",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)high_fault_vaddr,
        fault_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repair, the vsm.v prefix byte is visible, the fault byte and adjacent guards remain old, and the follow-up target has no stale side effect",
        read8(before_phys) == before &&
        read8(low_phys) == expected_mask[0] &&
        ai_micro_bytes_match(high_phys, old_high, sizeof(old_high)) &&
        read8(after_phys) == after &&
        ai_micro_bytes_match(follow_phys, follow_seed,
            sizeof(follow_seed)) &&
        read8(follow_before_phys) == follow_before &&
        read8(follow_after_phys) == follow_after
    );

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_arch_vsm_fault_round_start((const uint8_t *)mask_vaddr,
        (uint8_t *)dst_vaddr, AI_ARCH_VSM_FAULT_VL, fault_vstart);
    retry_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("after repairing the high leaf, retrying vsm.v from mask-byte vstart=1 completes without rewriting stale exception state",
        excpt.triggered == false &&
        retry_vstart == 0U,
        "retry_vstart=%u",
        retry_vstart
    );

    TEST_SETUP_EXCEPT();
    ai_arch_vsm_followup_vse8((const uint8_t *)follow_src_vaddr,
        (uint8_t *)follow_vaddr);
    TEST_ASSERT("a normal vector store immediately after the repaired vsm.v fault episode is not poisoned by old StoreQueue exception state",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repair and the follow-up vector store, the raw mask byte image and all guard regions match the architectural oracle exactly",
        read8(before_phys) == before &&
        read8(low_phys) == expected_mask[0] &&
        read8(high_phys) == expected_mask[1] &&
        read8(high_phys + 1U) == old_high[1] &&
        read8(after_phys) == after &&
        ai_micro_bytes_match(follow_phys, follow_new,
            sizeof(follow_new)) &&
        read8(follow_before_phys) == follow_before &&
        read8(follow_after_phys) == follow_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_vlm_mask_load_fault_old_new_v0_merge_repair_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page scratch_page = SWITCH1;
    const uintptr_t fault_mask_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - 1U;
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_mask_low_phys =
        phys_page_base(low_page) + PAGE_SIZE - 1U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page);
    const uintptr_t scratch_phys = phys_page_base(scratch_page);
    const uintptr_t old_mask_vaddr = scratch_vaddr + 0x600U;
    const uintptr_t raw_before_vaddr = scratch_vaddr + 0x610U;
    const uintptr_t raw_after_vaddr = scratch_vaddr + 0x620U;
    const uintptr_t consume_src_vaddr = scratch_vaddr + 0x640U;
    const uintptr_t consume_before_vaddr = scratch_vaddr + 0x660U;
    const uintptr_t consume_after_vaddr = scratch_vaddr + 0x680U;
    const uintptr_t old_mask_phys = scratch_phys + 0x600U;
    const uintptr_t raw_before_phys = scratch_phys + 0x610U;
    const uintptr_t raw_after_phys = scratch_phys + 0x620U;
    const uintptr_t consume_src_phys = scratch_phys + 0x640U;
    const uintptr_t consume_before_phys = scratch_phys + 0x660U;
    const uintptr_t consume_after_phys = scratch_phys + 0x680U;
    const uint8_t old_mask[AI_ARCH_VLM_MASK_BYTES] = { 0xa6U, 0x01U };
    const uint8_t new_mask[AI_ARCH_VLM_MASK_BYTES] = { 0x35U, 0x7eU };
    const uint8_t expected_before_raw[AI_ARCH_VLM_DUMP_BYTES] = {
        0x35U, 0x01U, 0xd3U, 0xd4U,
    };
    const uint8_t expected_after_raw[AI_ARCH_VLM_DUMP_BYTES] = {
        0x35U, 0x7eU, 0xe3U, 0xe4U,
    };
    uint8_t consume_src[AI_ARCH_VLM_CONSUME_BYTES];
    uint8_t consume_seed_before[AI_ARCH_VLM_CONSUME_BYTES];
    uint8_t consume_seed_after[AI_ARCH_VLM_CONSUME_BYTES];
    uint8_t expected_consume_before[AI_ARCH_VLM_CONSUME_BYTES];
    uint8_t expected_consume_after[AI_ARCH_VLM_CONSUME_BYTES];
    unsigned fault_vstart = 0U;
    unsigned retry_vstart = 0U;

    ai_arch_maskreg_enable_vector_state();

    for (unsigned i = 0; i < AI_ARCH_VLM_CONSUME_BYTES; ++i) {
        consume_src[i] = (uint8_t)(0x20U + 0x11U * i);
        consume_seed_before[i] = (uint8_t)(0x90U + i);
        consume_seed_after[i] = (uint8_t)(0xc0U + i);
    }
    ai_arch_vlm_build_consumer_expected(expected_consume_before,
        consume_seed_before, consume_src, expected_before_raw,
        AI_ARCH_VLM_FAULT_VL);
    ai_arch_vlm_build_consumer_expected(expected_consume_after,
        consume_seed_after, consume_src, expected_after_raw,
        AI_ARCH_VLM_FAULT_VL);

    goto_priv(PRIV_M);
    write8(fault_mask_low_phys, new_mask[0]);
    write8(high_phys, new_mask[1]);
    write8(high_phys + 1U, 0x6eU);
    ai_micro_write_bytes(old_mask_phys, old_mask, sizeof(old_mask));
    ai_micro_write_bytes(consume_src_phys, consume_src, sizeof(consume_src));
    ai_micro_write_bytes(consume_before_phys, consume_seed_before,
        sizeof(consume_seed_before));
    ai_micro_write_bytes(consume_after_phys, consume_seed_after,
        sizeof(consume_seed_after));
    write8(raw_before_phys, 0xd3U);
    write8(raw_before_phys + 1U, 0xd4U);
    write8(raw_before_phys + 2U, 0xd3U);
    write8(raw_before_phys + 3U, 0xd4U);
    write8(raw_after_phys, 0xe3U);
    write8(raw_after_phys + 1U, 0xe4U);
    write8(raw_after_phys + 2U, 0xe3U);
    write8(raw_after_phys + 3U, 0xe4U);
    write8(consume_before_phys - 1U, 0x5aU);
    write8(consume_before_phys + AI_ARCH_VLM_CONSUME_BYTES, 0xa5U);
    write8(consume_after_phys - 1U, 0x66U);
    write8(consume_after_phys + AI_ARCH_VLM_CONSUME_BYTES, 0x99U);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the vlm.v fault probe places ceil(vl=9/8)=2 mask bytes across an HS page boundary so byte0 can update v0 and byte1 raises LPF",
        (fault_mask_vaddr & 0xfffULL) == PAGE_SIZE - 1U &&
        high_fault_vaddr == fault_mask_vaddr + 1U
    );

    TEST_SETUP_EXCEPT();
    ai_arch_vlm_seed_then_fault((const uint8_t *)old_mask_vaddr,
        (const uint8_t *)fault_mask_vaddr, AI_ARCH_VLM_FAULT_VL);
    fault_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("vlm.v with vl=9 reports a precise load page fault on the second raw mask byte and leaves vstart at mask-byte index 1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == high_fault_vaddr &&
        fault_vstart == 1U,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=%u",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)high_fault_vaddr,
        fault_vstart
    );

    TEST_SETUP_EXCEPT();
    ai_arch_vlm_dump_and_consume_v0((const uint8_t *)consume_src_vaddr,
        (uint8_t *)raw_before_vaddr, (uint8_t *)consume_before_vaddr,
        AI_ARCH_VLM_FAULT_VL);
    TEST_ASSERT("after the faulting vlm.v, v0 contains the new prefix mask byte and the old fault/suffix byte, and an immediate v0.t consumer observes that mixed image",
        excpt.triggered == false &&
        ai_micro_bytes_match(raw_before_phys, expected_before_raw,
            sizeof(expected_before_raw)) &&
        ai_micro_bytes_match(consume_before_phys, expected_consume_before,
            sizeof(expected_consume_before)) &&
        read8(consume_before_phys - 1U) == 0x5aU &&
        read8(consume_before_phys + AI_ARCH_VLM_CONSUME_BYTES) == 0xa5U,
        "raw=%02x,%02x,%02x,%02x consume8=0x%x expected8=0x%x",
        read8(raw_before_phys), read8(raw_before_phys + 1U),
        read8(raw_before_phys + 2U), read8(raw_before_phys + 3U),
        read8(consume_before_phys + 8U), expected_consume_before[8]
    );

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_arch_vlm_retry_from_vstart((const uint8_t *)fault_mask_vaddr,
        AI_ARCH_VLM_FAULT_VL, fault_vstart);
    retry_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("after repairing the high leaf, retrying vlm.v from mask-byte vstart=1 completes and refreshes the old second mask byte",
        excpt.triggered == false &&
        retry_vstart == 0U,
        "retry_vstart=%u",
        retry_vstart
    );

    TEST_SETUP_EXCEPT();
    ai_arch_vlm_dump_and_consume_v0((const uint8_t *)consume_src_vaddr,
        (uint8_t *)raw_after_vaddr, (uint8_t *)consume_after_vaddr,
        AI_ARCH_VLM_FAULT_VL);
    TEST_ASSERT("after retry, v0 contains the complete new mask image and the follow-on masked store no longer consumes the old lane8 bit",
        excpt.triggered == false &&
        ai_micro_bytes_match(raw_after_phys, expected_after_raw,
            sizeof(expected_after_raw)) &&
        ai_micro_bytes_match(consume_after_phys, expected_consume_after,
            sizeof(expected_consume_after)) &&
        read8(consume_after_phys - 1U) == 0x66U &&
        read8(consume_after_phys + AI_ARCH_VLM_CONSUME_BYTES) == 0x99U,
        "raw=%02x,%02x,%02x,%02x consume8=0x%x expected8=0x%x",
        read8(raw_after_phys), read8(raw_after_phys + 1U),
        read8(raw_after_phys + 2U), read8(raw_after_phys + 3U),
        read8(consume_after_phys + 8U), expected_consume_after[8]
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}
#pragma GCC pop_options
