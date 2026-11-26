#include "../boundary/ai_micro_cross_page_fault_common.h"
#include <instructions.h>
#include <stdint.h>
#include <string.h>

static inline uint32_t ai_ap_store_split_le32(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0]) |
        ((uint32_t)bytes[1] << 8U) |
        ((uint32_t)bytes[2] << 16U) |
        ((uint32_t)bytes[3] << 24U);
}

static inline bool ai_ap_store_split_byte_old_or_current(uintptr_t addr,
    uint8_t old_value, uint8_t current_value)
{
    const uint8_t actual = read8(addr);

    return actual == old_value || actual == current_value;
}

static inline void ai_ap_store_split_masked_vse16_from_src(const uint16_t *src,
    const uint8_t *mask, uint16_t *dst, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "vle16.v v6, (%[src])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vse16.v v6, (%[dst]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [mask] "r"(mask),
          [dst] "r"(dst),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap_store_split_masked_vse32_from_src(const uint32_t *src,
    const uint8_t *mask, uint32_t *dst, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vse32.v v6, (%[dst]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [mask] "r"(mask),
          [dst] "r"(dst),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap_store_split_masked_vse32_from_src_start(
    const uint32_t *src, const uint8_t *mask, uint32_t *dst, unsigned vl,
    unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[src])\n\t"
        "csrw vstart, zero\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, %[start]\n\t"
        "vse32.v v6, (%[dst]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [mask] "r"(mask),
          [dst] "r"(dst),
          [vl] "r"(vl),
          [start] "r"(start)
        : "t0", "memory");
}

static inline void ai_ap_store_split_vse32_then_lw(const uint32_t *src,
    uint32_t *dst, const uint32_t *load_addr, uint64_t *observed)
{
    uint64_t value = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vse32.v v6, (%[dst])\n\t"
        "lw %[value], 0(%[load_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [value] "=&r"(value)
        : [src] "r"(src),
          [dst] "r"(dst),
          [load_addr] "r"(load_addr),
          [vl] "r"(4U)
        : "t0", "memory");

    *observed = value;
}

static inline void ai_ap_store_split_sw_then_lw(uint32_t *addr, uint32_t value,
    uint64_t *observed)
{
    uint64_t load_value = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sw %[store_value], 0(%[addr])\n\t"
        "lw %[load_value], 0(%[addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [load_value] "=&r"(load_value)
        : [addr] "r"(addr),
          [store_value] "r"(value)
        : "memory");

    *observed = load_value;
}

static inline void ai_ap_store_split_pressure_then_vse8(const uint8_t *src,
    uint8_t *dst, uint64_t *pressure)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t1, 0x1011121314151617\n\t"
        "sd t1, 0(%[pressure])\n\t"
        "li t1, 0x2021222324252627\n\t"
        "sd t1, 8(%[pressure])\n\t"
        "li t1, 0x3031323334353637\n\t"
        "sd t1, 16(%[pressure])\n\t"
        "li t1, 0x4041424344454647\n\t"
        "sd t1, 24(%[pressure])\n\t"
        "li t1, 0x5051525354555657\n\t"
        "sd t1, 32(%[pressure])\n\t"
        "li t1, 0x6061626364656667\n\t"
        "sd t1, 40(%[pressure])\n\t"
        "li t1, 0x7071727374757677\n\t"
        "sd t1, 48(%[pressure])\n\t"
        "li t1, 0x08090a0b0c0d0e0f\n\t"
        "sd t1, 56(%[pressure])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v6, (%[src])\n\t"
        "vse8.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [dst] "r"(dst),
          [pressure] "r"(pressure),
          [vl] "r"(16U)
        : "t0", "t1", "memory");
}

bool ai_micro_ap84_vse32_samepage_misaligned_success_vstart_zero_corner()
{
    TEST_START();

    const enum test_page dst_page = VSRWX_GRWX;
    const enum test_page src_page = SWITCH1;
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x20dU;
    const uintptr_t dst_phys = phys_page_base(dst_page) + 0x20dU;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x300U;
    const uintptr_t before_phys = dst_phys - 1U;
    const uintptr_t after_phys = dst_phys + 16U;
    const uint8_t before = 0xa4U;
    const uint8_t after = 0x4aU;
    const uint8_t initial[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t src_pattern[16] = {
        0xc0, 0xc1, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb,
        0xcc, 0xcd, 0xce, 0xcf
    };
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(dst_phys, initial, sizeof(initial));
    ai_micro_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    write8(before_phys, before);
    write8(after_phys, after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_ASSERT("the AP-84 same-page success probe intentionally starts vse32.v at low bits 13 so StoreMisalignBuffer still sees a non-aligned UnitStride store",
        (dst_vaddr & 0xfULL) == 13ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the AP-84 same-page success probe stages a four-element e32 vector payload before the non-aligned store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)dst_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a non-faulting same-page non-aligned vse32.v completes without inheriting stale non-zero vstart state",
        excpt.triggered == false &&
        observed_vstart == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-84 same-page non-aligned vse32.v writes the exact four e32 elements and preserves the adjacent guard bytes",
        ai_micro_bytes_match(dst_phys, src_pattern, sizeof(src_pattern)) &&
        read8(before_phys) == before &&
        read8(after_phys) == after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap90_masked_vse16_inactive_first_cross_active_suffix_exact_once_corner()
{
    TEST_START();

    const enum test_page dst_page = VSRWX_GRWX;
    const enum test_page src_page = SWITCH1;
    const enum test_page mask_page = SWITCH2;
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x20fU;
    const uintptr_t dst_phys = phys_page_base(dst_page) + 0x20fU;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x340U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x340U;
    const uintptr_t mask_vaddr = hs_page_base(mask_page) + 0x380U;
    const uintptr_t mask_phys = phys_page_base(mask_page) + 0x380U;
    const uintptr_t window_phys = dst_phys - 1U;
    const uint8_t initial_window[10] = {
        0x70, 0x71, 0x72, 0x73, 0x74,
        0x75, 0x76, 0x77, 0x78, 0x79
    };
    const uint8_t src_pattern[8] = {
        0xa0, 0xa1, 0xb0, 0xb1,
        0xc0, 0xc1, 0xd0, 0xd1
    };
    uint8_t expected_window[10];

    memcpy(expected_window, initial_window, sizeof(expected_window));
    memcpy(expected_window + 3U, src_pattern + 2U, 4U);

    goto_priv(PRIV_M);
    ai_micro_write_bytes(window_phys, initial_window, sizeof(initial_window));
    ai_micro_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    write8(mask_phys, 0x06U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_ASSERT("the AP-90 e16 mask probe starts at low bits 15 so element0 is the first 16B-crossing element and is masked off",
        (dst_vaddr & 0xfULL) == 15ULL
    );

    TEST_SETUP_EXCEPT();
    ai_ap_store_split_masked_vse16_from_src((const uint16_t *)src_vaddr,
        (const uint8_t *)mask_vaddr, (uint16_t *)dst_vaddr, 4U);
    TEST_ASSERT("masked vse16.v with inactive first-crossing element and active suffix elements must retire without a spurious split exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-90 masked e16 UnitStride split writes only active suffix element1-2 bytes exactly once and leaves inactive first-cross/tail bytes old",
        ai_micro_bytes_match(window_phys, expected_window, sizeof(expected_window))
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap90_masked_vse32_inactive_first_cross_replay_suffix_refresh_corner()
{
    TEST_START();

    const enum test_page dst_page = VSRWX_GRWX;
    const enum test_page src0_page = SWITCH1;
    const enum test_page src1_page = SWITCH2;
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x30dU;
    const uintptr_t dst_phys = phys_page_base(dst_page) + 0x30dU;
    const uintptr_t src0_vaddr = hs_page_base(src0_page) + 0x3c0U;
    const uintptr_t src0_phys = phys_page_base(src0_page) + 0x3c0U;
    const uintptr_t src1_vaddr = hs_page_base(src1_page) + 0x400U;
    const uintptr_t src1_phys = phys_page_base(src1_page) + 0x400U;
    const uintptr_t mask0_vaddr = hs_page_base(src0_page) + 0x3b0U;
    const uintptr_t mask0_phys = phys_page_base(src0_page) + 0x3b0U;
    const uintptr_t mask1_vaddr = hs_page_base(src1_page) + 0x3f0U;
    const uintptr_t mask1_phys = phys_page_base(src1_page) + 0x3f0U;
    const uintptr_t before_phys = dst_phys - 1U;
    const uintptr_t after_phys = dst_phys + 16U;
    const uint8_t before = 0x90U;
    const uint8_t after = 0x09U;
    const uint8_t initial[16] = {
        0x10, 0x11, 0x12, 0x13,
        0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b,
        0x1c, 0x1d, 0x1e, 0x1f
    };
    const uint8_t src0[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t src1[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    uint8_t expected[16];

    memcpy(expected, initial, sizeof(expected));
    memcpy(expected + 4U, src0 + 4U, 4U);
    memcpy(expected + 8U, src1 + 8U, 8U);

    goto_priv(PRIV_M);
    ai_micro_write_bytes(dst_phys, initial, sizeof(initial));
    ai_micro_write_bytes(src0_phys, src0, sizeof(src0));
    ai_micro_write_bytes(src1_phys, src1, sizeof(src1));
    write8(mask0_phys, 0x02U);
    write8(mask1_phys, 0x0cU);
    write8(before_phys, before);
    write8(after_phys, after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_ASSERT("the AP-90 e32 replay-refresh probe starts at low bits 13, where elemSize not elemMask decides that the masked-off element0 crosses the 16B boundary",
        (dst_vaddr & 0xfULL) == 13ULL
    );

    TEST_SETUP_EXCEPT();
    ai_ap_store_split_masked_vse32_from_src((const uint32_t *)src0_vaddr,
        (const uint8_t *)mask0_vaddr, (uint32_t *)dst_vaddr, 4U);
    TEST_ASSERT("the first AP-90 masked e32 split updates only element1 without dropping it through a false inactive-cross shift",
        excpt.triggered == false
    );

    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_ap_store_split_masked_vse32_from_src((const uint32_t *)src1_vaddr,
        (const uint8_t *)mask1_vaddr, (uint32_t *)dst_vaddr, 4U);
    TEST_ASSERT("the second AP-90 masked e32 split after translation refresh updates unfinished suffix elements without reusing a stale finish-flow mask",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-90 repeated masked e32 UnitStride split leaves inactive element0 old, keeps element1 from the first store, writes element2-3 from the second store, and preserves guards",
        ai_micro_bytes_match(dst_phys, expected, sizeof(expected)) &&
        read8(before_phys) == before &&
        read8(after_phys) == after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_unitstride_vstore_inactive_first_cross_fault_repair_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH1;
    const enum test_page mask_page = SWITCH2;
    const uintptr_t dst_vaddr = hs_page_base(low_page) + PAGE_SIZE - 19U;
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t dst_low_phys = phys_page_base(low_page) + PAGE_SIZE - 19U;
    const uintptr_t dst_high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x480U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x480U;
    const uintptr_t mask_vaddr = hs_page_base(mask_page) + 0x4c0U;
    const uintptr_t mask_phys = phys_page_base(mask_page) + 0x4c0U;
    const uintptr_t before_phys = dst_low_phys - 1U;
    const uintptr_t after_phys = dst_high_phys + 5U;
    const uint8_t before = 0x57U;
    const uint8_t after = 0x75U;
    const uint8_t initial[24] = {
        0x60, 0x61, 0x62, 0x63,
        0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b,
        0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73,
        0x74, 0x75, 0x76, 0x77
    };
    const uint8_t src_pattern[20] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xb0, 0xb1, 0xb2, 0xb3,
        0xc0, 0xc1, 0xc2, 0xc3,
        0xd0, 0xd1, 0xd2, 0xd3,
        0xe0, 0xe1, 0xe2, 0xe3
    };
    uint8_t expected_low_after_fault[19];
    uint8_t expected_high_after_fault[5];
    uint8_t expected_low_after_repair[19];
    uint8_t expected_high_after_repair[5];
    unsigned fault_vstart = 0U;
    unsigned retry_vstart = 0U;

    memcpy(expected_low_after_fault, initial,
        sizeof(expected_low_after_fault));
    memcpy(expected_low_after_fault + 4U, src_pattern + 4U, 12U);
    memcpy(expected_high_after_fault, initial + 19U,
        sizeof(expected_high_after_fault));

    memcpy(expected_low_after_repair, initial,
        sizeof(expected_low_after_repair));
    memcpy(expected_low_after_repair + 4U, src_pattern + 4U, 15U);
    memcpy(expected_high_after_repair, initial + 19U,
        sizeof(expected_high_after_repair));
    expected_high_after_repair[0] = src_pattern[19U];

    goto_priv(PRIV_M);
    ai_micro_write_bytes(dst_low_phys, initial, 19U);
    ai_micro_write_bytes(dst_high_phys, initial + 19U, 5U);
    ai_micro_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    write8(mask_phys, 0x1eU);
    write8(mask_phys + 1U, 0x00U);
    write8(before_phys, before);
    write8(after_phys, after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(mask_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the AP-90 fault delta starts vse32.v at page_end-19, so element0 is inactive but crosses a 16B boundary while element4 faults on the high page",
        (dst_vaddr & 0xfULL) == 13ULL &&
        ((dst_vaddr + 16U) & 0xfffULL) == PAGE_SIZE - 3U
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap_store_split_masked_vse32_from_src_start(
        (const uint32_t *)src_vaddr, (const uint8_t *)mask_vaddr,
        (uint32_t *)dst_vaddr, 5U, 0U);
    fault_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("masked vse32.v with inactive first-cross element commits active suffix elements before the no-write high-page fault and reports element4",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_fault_vaddr &&
        fault_vstart == 4U,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=%u",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)high_fault_vaddr,
        fault_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repair, inactive element0 stays old, active element1-3 bytes are visible, and the high-page side of the faulting element remains untouched",
        ai_micro_bytes_match(dst_low_phys, expected_low_after_fault,
            16U) &&
        ai_ap_store_split_byte_old_or_current(dst_low_phys + 16U,
            initial[16U], src_pattern[16U]) &&
        ai_ap_store_split_byte_old_or_current(dst_low_phys + 17U,
            initial[17U], src_pattern[17U]) &&
        ai_ap_store_split_byte_old_or_current(dst_low_phys + 18U,
            initial[18U], src_pattern[18U]) &&
        ai_micro_bytes_match(dst_high_phys, expected_high_after_fault,
            sizeof(expected_high_after_fault)) &&
        ai_micro_bytes_match(src_phys, src_pattern, sizeof(src_pattern)) &&
        read8(before_phys) == before &&
        read8(after_phys) == after
    );

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_ap_store_split_masked_vse32_from_src_start(
        (const uint32_t *)src_vaddr, (const uint8_t *)mask_vaddr,
        (uint32_t *)dst_vaddr, 5U, fault_vstart);
    retry_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("repairing the high leaf and retrying from vstart=4 writes only the faulted element without clobbering inactive element0 or prefix bytes",
        excpt.triggered == false &&
        retry_vstart == 0U,
        "retry_vstart=%u",
        retry_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repair, the final byte image keeps inactive element0 old and contains active element1-4 payload across the page boundary with guards intact",
        ai_micro_bytes_match(dst_low_phys, expected_low_after_repair,
            sizeof(expected_low_after_repair)) &&
        ai_micro_bytes_match(dst_high_phys, expected_high_after_repair,
            sizeof(expected_high_after_repair)) &&
        ai_micro_bytes_match(src_phys, src_pattern, sizeof(src_pattern)) &&
        read8(before_phys) == before &&
        read8(after_phys) == after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap96_vse32_high_split_suffix_lw_gets_new_data_corner()
{
    TEST_START();

    const enum test_page dst_page = VSRWX_GRWX;
    const enum test_page src_page = SWITCH1;
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x40fU;
    const uintptr_t dst_phys = phys_page_base(dst_page) + 0x40fU;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x440U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x440U;
    const uintptr_t before_phys = dst_phys - 1U;
    const uintptr_t after_phys = dst_phys + 16U;
    const uint8_t before = 0x96U;
    const uint8_t after = 0x69U;
    const uint8_t initial[16] = {
        0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b,
        0x8c, 0x8d, 0x8e, 0x8f
    };
    const uint8_t src_pattern[16] = {
        0x11, 0x22, 0x33, 0x44,
        0x21, 0x32, 0x43, 0x54,
        0x31, 0x42, 0x53, 0x64,
        0x41, 0x52, 0x63, 0x74
    };
    const uint32_t expected_lw = ai_ap_store_split_le32(src_pattern + 8U);
    uint64_t observed_lw = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(dst_phys, initial, sizeof(initial));
    ai_micro_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    write8(before_phys, before);
    write8(after_phys, after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_ASSERT("the AP-96 forwarding probe starts vse32.v at low bits 15 so high split suffix lanes beyond the first e32 chunk are active",
        (dst_vaddr & 0xfULL) == 15ULL
    );

    TEST_SETUP_EXCEPT();
    ai_ap_store_split_vse32_then_lw((const uint32_t *)src_vaddr,
        (uint32_t *)dst_vaddr, (const uint32_t *)(dst_vaddr + 8U),
        &observed_lw);
    TEST_ASSERT("a younger lw overlapping the UnitStride high split suffix must observe forwarded/replayed new data rather than stale DCache bytes",
        excpt.triggered == false &&
        (uint32_t)observed_lw == expected_lw,
        "observed=0x%llx expected=0x%x",
        (unsigned long long)observed_lw, expected_lw
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-96 UnitStride high split suffix store eventually commits the full 4x32-bit payload and preserves adjacent guards",
        ai_micro_bytes_match(dst_phys, src_pattern, sizeof(src_pattern)) &&
        read8(before_phys) == before &&
        read8(after_phys) == after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap96_scalar_unaligned_sw_forwarding_control_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x50fU;
    const uintptr_t base_phys = phys_page_base(page) + 0x50fU;
    const uintptr_t before_phys = base_phys - 1U;
    const uintptr_t after_phys = base_phys + 4U;
    const uint8_t before = 0x6cU;
    const uint8_t after = 0xc6U;
    const uint8_t initial[4] = {0x01, 0x02, 0x03, 0x04};
    const uint32_t store_value = 0x55667711U;
    const uint8_t expected[4] = {0x11, 0x77, 0x66, 0x55};
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(base_phys, initial, sizeof(initial));
    write8(before_phys, before);
    write8(after_phys, after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_ASSERT("the AP-96 scalar control starts sw at low bits 15 so the ordinary scalar StoreMisalignBuffer split path is exercised separately from UnitStride",
        (base_vaddr & 0xfULL) == 15ULL
    );

    TEST_SETUP_EXCEPT();
    ai_ap_store_split_sw_then_lw((uint32_t *)base_vaddr, store_value, &observed);
    TEST_ASSERT("a scalar unaligned sw followed by overlapping lw still forwards or replays according to the scalar-width split mask",
        excpt.triggered == false &&
        (uint32_t)observed == store_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-96 scalar control writes exactly the scalar word bytes and keeps adjacent guards intact",
        ai_micro_bytes_match(base_phys, expected, sizeof(expected)) &&
        read8(before_phys) == before &&
        read8(after_phys) == after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap97_cross_page_vector_store_nonadjacent_high_paddr_after_sq_pressure_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const enum test_page high_phys_page = SWITCH2;
    const enum test_page src_page = SWITCH1;
    const enum test_page pressure_page = SCRATCHPAD;
    const uintptr_t dst_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t default_neighbor_high_phys = phys_page_base(high_page);
    const uintptr_t remapped_high_phys = phys_page_base(high_phys_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x600U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x600U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x680U;
    const uintptr_t pressure_phys = phys_page_base(pressure_page) + 0x680U;
    const uintptr_t before_phys = low_phys - 1U;
    const uintptr_t after_phys = remapped_high_phys + 8U;
    const uint8_t before = 0x97U;
    const uint8_t after = 0x79U;
    const uint8_t low_initial[8] = {
        0x10, 0x11, 0x12, 0x13,
        0x14, 0x15, 0x16, 0x17
    };
    const uint8_t high_initial[8] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27
    };
    const uint8_t wrong_neighbor_initial[8] = {
        0x60, 0x61, 0x62, 0x63,
        0x64, 0x65, 0x66, 0x67
    };
    const uint8_t src_pattern[16] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xa4, 0xa5, 0xa6, 0xa7,
        0xb0, 0xb1, 0xb2, 0xb3,
        0xb4, 0xb5, 0xb6, 0xb7
    };

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys, low_initial, sizeof(low_initial));
    ai_micro_write_bytes(remapped_high_phys, high_initial, sizeof(high_initial));
    ai_micro_write_bytes(default_neighbor_high_phys, wrong_neighbor_initial,
        sizeof(wrong_neighbor_initial));
    ai_micro_write_bytes(src_phys, src_pattern, sizeof(src_pattern));
    memset((void *)pressure_phys, 0, 64U);
    write8(before_phys, before);
    write8(after_phys, after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf_paddr(high_page, remapped_high_phys,
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pressure_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_ASSERT("the AP-97 cross-page probe remaps the high virtual page away from the physical page selected by paddrLow+8",
        default_neighbor_high_phys != remapped_high_phys &&
        (dst_vaddr & (PAGE_SIZE - 1ULL)) == PAGE_SIZE - 8ULL
    );

    TEST_SETUP_EXCEPT();
    ai_ap_store_split_pressure_then_vse8((const uint8_t *)src_vaddr,
        (uint8_t *)dst_vaddr, (uint64_t *)pressure_vaddr);
    TEST_ASSERT("after StoreQueue pressure, a 16-byte vector store crossing into a non-adjacent high physical page must retire without cross-page sideband loss",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-97 cross-page vector store writes its low half to the low page and its high half to the translated high page, not to paddrLow+8",
        ai_micro_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_bytes_match(remapped_high_phys, src_pattern + 8U, 8U) &&
        ai_micro_bytes_match(default_neighbor_high_phys, wrong_neighbor_initial,
            sizeof(wrong_neighbor_initial)) &&
        read8(before_phys) == before &&
        read8(after_phys) == after
    );
    TEST_ASSERT("the AP-97 pressure stores also retire, keeping the vector-store wrap-pressure window architecturally visible",
        read64(pressure_phys) == 0x1011121314151617ULL &&
        read64(pressure_phys + 56U) == 0x08090a0b0c0d0e0fULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
