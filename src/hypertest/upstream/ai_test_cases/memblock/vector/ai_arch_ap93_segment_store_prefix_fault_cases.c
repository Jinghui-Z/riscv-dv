#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline pte_t *ai_ap93_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ap93_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ai_ap93_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_ap93_write_bytes(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline void ai_ap93_write_words(uintptr_t paddr, const uint32_t *words,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write32(paddr + i * sizeof(uint32_t), words[i]);
    }
}

static inline void ai_ap93_vlseg2e8_to_v8(const uint8_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vlseg2e8.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap93_vsseg2e8_from_v8(uint8_t *dst, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vsseg2e8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap93_vlseg2e32_to_v8(const uint32_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlseg2e32.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap93_vsseg2e32_from_v8(uint32_t *dst, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vsseg2e32.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap93_vssseg2e32_from_v8(uint32_t *dst,
    intptr_t stride_bytes, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vssseg2e32.v v8, (%[dst]), %[stride]\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst),
          [stride] "r"(stride_bytes),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap93_vsuxseg2ei32_from_v8(uint8_t *base,
    const uint32_t *offsets, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v10, (%[offsets])\n\t"
        "vsuxseg2ei32.v v8, (%[base]), v10\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap93_prepare_three_pages(enum test_page src_page,
    enum test_page low_page, enum test_page high_page)
{
    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap93_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap93_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap93_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    sfence_vma();
}

static inline void ai_ap93_cleanup_hs(void)
{
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();
}

bool ai_arch_ap93_vsseg2e8_field1_fault_keeps_field0_store_corner()
{
    TEST_START();

    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x180U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x180U;
    const uintptr_t dst_vaddr = hs_page_base(low_page) + PAGE_SIZE - 1U;
    const uintptr_t dst_paddr = phys_page_base(low_page) + PAGE_SIZE - 1U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint8_t src_bytes[2] = { 0x5aU, 0xc3U };
    const uint8_t old0 = 0x11U;
    const uint8_t old1 = 0x22U;
    const uint8_t low_guard = 0x33U;
    const uint8_t high_guard = 0x44U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap93_write_bytes(src_paddr, src_bytes, 2U);
    write8(dst_paddr, old0);
    write8(fault_paddr, old1);
    write8(dst_paddr - 1U, low_guard);
    write8(fault_paddr + 1U, high_guard);

    ai_ap93_prepare_three_pages(src_page, low_page, high_page);

    TEST_SETUP_EXCEPT();
    ai_ap93_vlseg2e8_to_v8((const uint8_t *)src_vaddr, 1U);
    AI_ASSERT_NO_EXCEPT_AND("AP-93 setup loads one e8 segment source without taking an exception",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap93_vsseg2e8_from_v8((uint8_t *)dst_vaddr, 1U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-93 vsseg2e8 field1 no-write page reports a store page fault on the second field while vstart stays at segment index 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-93 vsseg2e8 commits field0 before the field1 fault and preserves the faulting field plus adjacent guards",
        read8(dst_paddr) == src_bytes[0] &&
        read8(fault_paddr) == old1 &&
        read8(dst_paddr - 1U) == low_guard &&
        read8(fault_paddr + 1U) == high_guard,
        "field0=0x%x expected=0x%x field1=0x%x old=0x%x",
        read8(dst_paddr), src_bytes[0], read8(fault_paddr), old1
    );

    ai_ap93_cleanup_hs();

    TEST_END();
}

bool ai_arch_ap93_vsseg2e32_later_segment_fault_keeps_prefix_fields_corner()
{
    TEST_START();

    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x200U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x200U;
    const uintptr_t dst_vaddr = hs_page_base(low_page) + PAGE_SIZE - 12U;
    const uintptr_t dst_paddr = phys_page_base(low_page) + PAGE_SIZE - 12U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint32_t src_words[4] = {
        0x100000a1U, 0x100000b2U, 0x100000c3U, 0x100000d4U,
    };
    const uint32_t old_words[4] = {
        0xaaaa0001U, 0xbbbb0002U, 0xcccc0003U, 0xdddd0004U,
    };
    const uint32_t low_guard = 0xeeee0005U;
    const uint32_t high_guard = 0xffff0006U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap93_write_words(src_paddr, src_words, 4U);
    write32(dst_paddr, old_words[0]);
    write32(dst_paddr + 4U, old_words[1]);
    write32(dst_paddr + 8U, old_words[2]);
    write32(fault_paddr, old_words[3]);
    write32(dst_paddr - 4U, low_guard);
    write32(fault_paddr + 4U, high_guard);

    ai_ap93_prepare_three_pages(src_page, low_page, high_page);

    TEST_SETUP_EXCEPT();
    ai_ap93_vlseg2e32_to_v8((const uint32_t *)src_vaddr, 2U);
    AI_ASSERT_NO_EXCEPT_AND("AP-93 setup loads two e32 segment-store source elements without exception",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap93_vsseg2e32_from_v8((uint32_t *)dst_vaddr, 2U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-93 vsseg2e32 reports the field1 fault in element1 with vstart equal to the element index rather than the field index",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-93 vsseg2e32 keeps the completed segment and faulting-segment field0 visible while field1 and guards remain old",
        read32(dst_paddr) == src_words[0] &&
        read32(dst_paddr + 4U) == src_words[1] &&
        read32(dst_paddr + 8U) == src_words[2] &&
        read32(fault_paddr) == old_words[3] &&
        read32(dst_paddr - 4U) == low_guard &&
        read32(fault_paddr + 4U) == high_guard,
        "seg0=%x,%x field0=%x field1=%x",
        read32(dst_paddr), read32(dst_paddr + 4U),
        read32(dst_paddr + 8U), read32(fault_paddr)
    );

    ai_ap93_cleanup_hs();

    TEST_END();
}

bool ai_arch_ap93_vssseg2e32_field1_fault_keeps_field0_store_corner()
{
    TEST_START();

    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const intptr_t stride_bytes = 16;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x280U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x280U;
    const uintptr_t dst_vaddr = hs_page_base(low_page) + PAGE_SIZE - 20U;
    const uintptr_t dst_paddr = phys_page_base(low_page) + PAGE_SIZE - 20U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint32_t src_words[4] = {
        0x210000a1U, 0x210000b2U, 0x210000c3U, 0x210000d4U,
    };
    const uint32_t gap0 = 0x45450011U;
    const uint32_t gap1 = 0x56560022U;
    const uint32_t old_fault = 0x67670033U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap93_write_words(src_paddr, src_words, 4U);
    write32(dst_paddr, 0x81810001U);
    write32(dst_paddr + 4U, 0x82820002U);
    write32(dst_paddr + 8U, gap0);
    write32(dst_paddr + 12U, gap1);
    write32(dst_paddr + 16U, 0x83830003U);
    write32(fault_paddr, old_fault);

    ai_ap93_prepare_three_pages(src_page, low_page, high_page);

    TEST_SETUP_EXCEPT();
    ai_ap93_vlseg2e32_to_v8((const uint32_t *)src_vaddr, 2U);
    AI_ASSERT_NO_EXCEPT_AND("AP-93 setup loads the vssseg2e32 data registers without exception",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap93_vssseg2e32_from_v8((uint32_t *)dst_vaddr, stride_bytes, 2U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-93 vssseg2e32 reports the second segment field1 store fault with vstart equal to element1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-93 vssseg2e32 preserves stride gaps, commits completed segments and field0 of the faulting segment, and leaves field1 old",
        read32(dst_paddr) == src_words[0] &&
        read32(dst_paddr + 4U) == src_words[1] &&
        read32(dst_paddr + 8U) == gap0 &&
        read32(dst_paddr + 12U) == gap1 &&
        read32(dst_paddr + 16U) == src_words[2] &&
        read32(fault_paddr) == old_fault,
        "seg0=%x,%x gaps=%x,%x field0=%x field1=%x",
        read32(dst_paddr), read32(dst_paddr + 4U),
        read32(dst_paddr + 8U), read32(dst_paddr + 12U),
        read32(dst_paddr + 16U), read32(fault_paddr)
    );

    ai_ap93_cleanup_hs();

    TEST_END();
}

bool ai_arch_ap93_vsuxseg2ei32_field1_fault_keeps_field0_store_corner()
{
    TEST_START();

    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x300U;
    const uintptr_t offsets_vaddr = hs_page_base(src_page) + 0x340U;
    const uintptr_t offsets_paddr = phys_page_base(src_page) + 0x340U;
    const uintptr_t base_vaddr = hs_page_base(low_page);
    const uintptr_t field0_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uint32_t src_words[2] = { 0x310000a1U, 0x310000b2U };
    const uint32_t offset0 = PAGE_SIZE - 4U;
    const uint32_t old0 = 0x91910001U;
    const uint32_t old1 = 0x92920002U;
    const uint32_t guard = 0x93930003U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap93_write_words(src_paddr, src_words, 2U);
    write32(offsets_paddr, offset0);
    write32(field0_paddr, old0);
    write32(fault_paddr, old1);
    write32(field0_paddr - 4U, guard);

    ai_ap93_prepare_three_pages(src_page, low_page, high_page);

    TEST_SETUP_EXCEPT();
    ai_ap93_vlseg2e32_to_v8((const uint32_t *)src_vaddr, 1U);
    AI_ASSERT_NO_EXCEPT_AND("AP-93 setup loads the indexed segment-store payload registers without exception",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap93_vsuxseg2ei32_from_v8((uint8_t *)base_vaddr,
        (const uint32_t *)offsets_vaddr, 1U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-93 vsuxseg2ei32 reports field1 no-write page fault while keeping vstart at indexed element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-93 indexed segment store keeps the field0 side effect visible before field1 faults and leaves unrelated words old",
        read32(field0_paddr) == src_words[0] &&
        read32(fault_paddr) == old1 &&
        read32(field0_paddr - 4U) == guard,
        "field0=%x expected=%x field1=%x old=%x",
        read32(field0_paddr), src_words[0], read32(fault_paddr), old1
    );

    ai_ap93_cleanup_hs();

    TEST_END();
}

bool ai_arch_ap93_segment_load_fault_keeps_fault_segment_unwritten_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x380U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x380U;
    const uintptr_t fault_src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t fault_src_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x400U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x400U;
    const uint32_t old_words[2] = { 0x410000a1U, 0x410000b2U };
    const uint32_t fault_src_words[2] = { 0x420000c3U, 0x420000d4U };
    const uint32_t dump_seed0 = 0x51510001U;
    const uint32_t dump_seed1 = 0x52520002U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap93_write_words(old_paddr, old_words, 2U);
    write32(fault_src_paddr, fault_src_words[0]);
    write32(fault_paddr, fault_src_words[1]);
    write32(dump_paddr, dump_seed0);
    write32(dump_paddr + 4U, dump_seed1);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap93_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap93_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap93_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_ap93_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_ap93_vlseg2e32_to_v8((const uint32_t *)old_vaddr, 1U);
    AI_ASSERT_NO_EXCEPT_AND("AP-93/AP-85 setup preloads old segment-load destination registers without exception",
        true
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap93_vlseg2e32_to_v8((const uint32_t *)fault_src_vaddr, 1U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-93 keeps the AP-85 load-side rule: a field1 load fault reports segment index 0 without committing the partial segment",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap93_vsseg2e32_from_v8((uint32_t *)dump_vaddr, 1U);
    AI_ASSERT_NO_EXCEPT_AND("AP-93/AP-85 dump of destination registers after the segment-load fault completes without a second fault",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-93 store-side prefix policy does not regress AP-85 load-side whole-faulting-segment destination suppression",
        read32(dump_paddr) == old_words[0] &&
        read32(dump_paddr + 4U) == old_words[1] &&
        read32(fault_src_paddr) == fault_src_words[0] &&
        read32(fault_paddr) == fault_src_words[1],
        "dump=%x,%x old=%x,%x",
        read32(dump_paddr), read32(dump_paddr + 4U),
        old_words[0], old_words[1]
    );

    ai_ap93_cleanup_hs();

    TEST_END();
}
