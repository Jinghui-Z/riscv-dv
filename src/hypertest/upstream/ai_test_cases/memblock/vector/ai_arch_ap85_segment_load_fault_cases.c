#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_AP85_MAX_WORDS 16U

#define AI_AP85_ASSERT_NO_EXCEPT(message) \
    TEST_ASSERT(message, excpt.triggered == false)

static inline pte_t *ai_ap85_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ap85_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ai_ap85_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_ap85_write_words(uintptr_t paddr, const uint32_t *words,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write32(paddr + i * sizeof(uint32_t), words[i]);
    }
}

static inline void ai_ap85_fill_words(uintptr_t paddr, uint32_t seed,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write32(paddr + i * sizeof(uint32_t), seed + i);
    }
}

static inline bool ai_ap85_check_words(uintptr_t paddr,
    const uint32_t *expected, unsigned len, unsigned *bad_index,
    uint32_t *bad_actual, uint32_t *bad_expected)
{
    for (unsigned i = 0; i < len; ++i) {
        const uint32_t actual = read32(paddr + i * sizeof(uint32_t));

        if (actual != expected[i]) {
            *bad_index = i;
            *bad_actual = actual;
            *bad_expected = expected[i];
            return false;
        }
    }
    return true;
}

static inline void ai_ap85_prepare_pages(enum test_page a, enum test_page b,
    enum test_page c, enum test_page d)
{
    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap85_set_hs_leaf(a, PTE_V | PTE_AD | PTE_RWX);
    ai_ap85_set_hs_leaf(b, PTE_V | PTE_AD | PTE_RWX);
    ai_ap85_set_hs_leaf(c, PTE_V | PTE_AD | PTE_X);
    ai_ap85_set_hs_leaf(d, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
}

static inline void ai_ap85_cleanup_hs(void)
{
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();
}

static inline void ai_ap85_vlseg2e32_to_v8(const uint32_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vlseg2e32.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap85_vsseg2e32_from_v8(uint32_t *dst, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vsseg2e32.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap85_vlseg4e32_to_v8(const uint32_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vlseg4e32.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap85_vsseg4e32_from_v8(uint32_t *dst, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vsseg4e32.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap85_vlseg8e32_to_v8(const uint32_t *src, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vlseg8e32.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap85_vsseg8e32_from_v8(uint32_t *dst, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vsseg8e32.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap85_vlsseg2e32_to_v8(const uint32_t *src,
    intptr_t stride, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vlsseg2e32.v v8, (%[src]), %[stride]\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [stride] "r"(stride),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap85_vluxseg2ei32_to_v8(const uint8_t *base,
    const uint32_t *offsets, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v10, (%[offsets])\n\t"
        "vluxseg2ei32.v v8, (%[base]), v10\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap85_masked_vlseg2e32_mu(const uint32_t *src,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vmclr.m v0\n\t"
        "vlseg2e32.v v8, (%[src]), v0.t\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap85_masked_vlseg2e32_ma(const uint32_t *src,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, ma\n\t"
        "vmclr.m v0\n\t"
        "vlseg2e32.v v8, (%[src]), v0.t\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(vl)
        : "t0", "memory");
}

bool ai_arch_ap85_vlseg2e32_later_segment_field1_fault_no_partial_dest_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x100U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x100U;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 12U;
    const uintptr_t src_paddr = phys_page_base(low_page) + PAGE_SIZE - 12U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x100U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x100U;
    const uint32_t old_words[4] = {
        0x85010010U, 0x85010020U, 0x85010030U, 0x85010040U,
    };
    const uint32_t src_words[4] = {
        0x8501a001U, 0x8501b002U, 0x8501c003U, 0x8501d004U,
    };
    const uint32_t expected_dump[4] = {
        src_words[0], src_words[1], old_words[2], old_words[3],
    };
    unsigned bad_index = 0U;
    uint32_t bad_actual = 0U;
    uint32_t bad_expected = 0U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap85_write_words(old_paddr, old_words, 4U);
    ai_ap85_write_words(src_paddr, src_words, 3U);
    write32(fault_paddr, src_words[3]);
    ai_ap85_fill_words(dump_paddr, 0xdead8501U, 4U);

    ai_ap85_prepare_pages(old_page, low_page, high_page, dump_page);

    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg2e32_to_v8((const uint32_t *)old_vaddr, 2U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 setup preloads two old vlseg2e32 destination segments without exception");

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg2e32_to_v8((const uint32_t *)src_vaddr, 2U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-85 vlseg2e32 field1 page fault in segment1 reports vstart at the segment index and the precise faulting field address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vsseg2e32_from_v8((uint32_t *)dump_vaddr, 2U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 dump after vlseg2e32 fault completes without a second exception");

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-85 keeps completed segment0 visible but suppresses every destination field of the faulting segment1",
        ai_ap85_check_words(dump_paddr, expected_dump, 4U, &bad_index,
            &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    ai_ap85_cleanup_hs();

    TEST_END();
}

bool ai_arch_ap85_vlseg4_vlseg8_middle_field_fault_no_partial_dest_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x180U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x180U;
    const uintptr_t src4_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t src4_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t src8_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t src8_paddr = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x180U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x180U;
    const uint32_t old4[4] = {
        0x85020010U, 0x85020020U, 0x85020030U, 0x85020040U,
    };
    const uint32_t old8[8] = {
        0x85028010U, 0x85028020U, 0x85028030U, 0x85028040U,
        0x85028050U, 0x85028060U, 0x85028070U, 0x85028080U,
    };
    const uint32_t src4[4] = {
        0x8502a001U, 0x8502b002U, 0x8502c003U, 0x8502d004U,
    };
    const uint32_t src8[8] = {
        0x8502e001U, 0x8502e002U, 0x8502e003U, 0x8502e004U,
        0x8502e005U, 0x8502e006U, 0x8502e007U, 0x8502e008U,
    };
    unsigned bad_index = 0U;
    uint32_t bad_actual = 0U;
    uint32_t bad_expected = 0U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap85_write_words(old_paddr, old4, 4U);
    ai_ap85_write_words(src4_paddr, src4, 2U);
    write32(fault_paddr, src4[2]);
    write32(fault_paddr + 4U, src4[3]);
    ai_ap85_fill_words(dump_paddr, 0xdead8502U, 8U);

    ai_ap85_prepare_pages(old_page, low_page, high_page, dump_page);

    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg4e32_to_v8((const uint32_t *)old_vaddr, 1U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 setup preloads old vlseg4e32 destination fields without exception");

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg4e32_to_v8((const uint32_t *)src4_vaddr, 1U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-85 vlseg4e32 middle field2 load page fault reports element index 0 and the high-page field address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vsseg4e32_from_v8((uint32_t *)dump_vaddr, 1U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 dump after vlseg4e32 fault completes without exception");

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-85 vlseg4e32 suppresses fields0..3 of the faulting segment even though fields0 and 1 were individually readable",
        ai_ap85_check_words(dump_paddr, old4, 4U, &bad_index,
            &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    goto_priv(PRIV_M);
    ai_ap85_write_words(old_paddr, old8, 8U);
    ai_ap85_write_words(src8_paddr, src8, 4U);
    for (unsigned i = 4U; i < 8U; ++i) {
        write32(fault_paddr + (i - 4U) * sizeof(uint32_t), src8[i]);
    }
    ai_ap85_fill_words(dump_paddr, 0xbeef8502U, 8U);

    ai_ap85_prepare_pages(old_page, low_page, high_page, dump_page);

    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg8e32_to_v8((const uint32_t *)old_vaddr, 1U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 setup preloads old vlseg8e32 destination fields without exception");

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg8e32_to_v8((const uint32_t *)src8_vaddr, 1U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-85 vlseg8e32 middle field4 load page fault also uses the buffered no-partial-destination path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vsseg8e32_from_v8((uint32_t *)dump_vaddr, 1U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 dump after vlseg8e32 fault completes without exception");

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-85 vlseg8e32 suppresses every field of the faulting segment after a middle-field fault",
        ai_ap85_check_words(dump_paddr, old8, 8U, &bad_index,
            &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    ai_ap85_cleanup_hs();

    TEST_END();
}

bool ai_arch_ap85_vluxseg2ei32_field1_fault_no_partial_dest_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x280U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x280U;
    const uintptr_t offsets_vaddr = hs_page_base(old_page) + 0x300U;
    const uintptr_t offsets_paddr = phys_page_base(old_page) + 0x300U;
    const uintptr_t base_vaddr = hs_page_base(low_page);
    const uintptr_t field0_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x280U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x280U;
    const uint32_t old_words[2] = { 0x85030010U, 0x85030020U };
    const uint32_t src_words[2] = { 0x8503a001U, 0x8503b002U };
    const uint32_t offset0 = PAGE_SIZE - 4U;
    unsigned bad_index = 0U;
    uint32_t bad_actual = 0U;
    uint32_t bad_expected = 0U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap85_write_words(old_paddr, old_words, 2U);
    write32(offsets_paddr, offset0);
    write32(field0_paddr, src_words[0]);
    write32(fault_paddr, src_words[1]);
    ai_ap85_fill_words(dump_paddr, 0xdead8503U, 2U);

    ai_ap85_prepare_pages(old_page, low_page, high_page, dump_page);

    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg2e32_to_v8((const uint32_t *)old_vaddr, 1U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 setup preloads indexed segment-load old destination fields without exception");

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vluxseg2ei32_to_v8((const uint8_t *)base_vaddr,
        (const uint32_t *)offsets_vaddr, 1U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-85 vluxseg2ei32 reports the field1 indexed load page fault address while keeping vstart at the segment index",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vsseg2e32_from_v8((uint32_t *)dump_vaddr, 1U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 dump after indexed segment-load fault completes without exception");

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-85 indexed segment load suppresses the valid field0 destination when field1 of the same segment faults",
        ai_ap85_check_words(dump_paddr, old_words, 2U, &bad_index,
            &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    ai_ap85_cleanup_hs();

    TEST_END();
}

bool ai_arch_ap85_masked_vlseg2e32_maskedoff_fault_no_trap_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x380U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x380U;
    const uintptr_t bad_vaddr = hs_page_base(high_page);
    const uintptr_t bad_paddr = phys_page_base(high_page);
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x380U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x380U;
    const uint32_t old_words[2] = { 0x85040010U, 0x85040020U };
    unsigned bad_index = 0U;
    uint32_t bad_actual = 0U;
    uint32_t bad_expected = 0U;

    goto_priv(PRIV_M);
    ai_ap85_write_words(old_paddr, old_words, 2U);
    write32(bad_paddr, 0x8504a001U);
    write32(bad_paddr + 4U, 0x8504b002U);
    ai_ap85_fill_words(dump_paddr, 0xdead8504U, 2U);

    ai_ap85_prepare_pages(old_page, low_page, high_page, dump_page);

    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg2e32_to_v8((const uint32_t *)old_vaddr, 1U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 setup preloads masked segment-load destination guards without exception");

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_masked_vlseg2e32_mu((const uint32_t *)bad_vaddr, 1U);
    TEST_ASSERT("AP-85 masked-off vlseg2e32 mu variant does not probe the no-read page and does not raise a load fault",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL,
        "cause=0x%lx tval=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, CSRR(CSR_VSTART)
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vsseg2e32_from_v8((uint32_t *)dump_vaddr, 1U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 dump after masked-off mu segment load completes without exception");

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-85 masked-off vlseg2e32 with vma=mu preserves inactive destination fields instead of taking AP-85 fault suppression path",
        ai_ap85_check_words(dump_paddr, old_words, 2U, &bad_index,
            &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap85_masked_vlseg2e32_ma((const uint32_t *)bad_vaddr, 1U);
    TEST_ASSERT("AP-85 masked-off vlseg2e32 with vma=ma also suppresses the faulting memory probe; destination value is intentionally not constrained",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL,
        "cause=0x%lx tval=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, CSRR(CSR_VSTART)
    );

    ai_ap85_cleanup_hs();

    TEST_END();
}

bool ai_arch_ap85_vlsseg2e32_field1_cross_page_no_partial_dest_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const intptr_t stride = 8;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x480U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x480U;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 12U;
    const uintptr_t src_paddr = phys_page_base(low_page) + PAGE_SIZE - 12U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x480U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x480U;
    const uint32_t old_words[4] = {
        0x85050010U, 0x85050020U, 0x85050030U, 0x85050040U,
    };
    const uint32_t src_words[4] = {
        0x8505a001U, 0x8505b002U, 0x8505c003U, 0x8505d004U,
    };
    const uint32_t expected_dump[4] = {
        src_words[0], src_words[1], old_words[2], old_words[3],
    };
    unsigned bad_index = 0U;
    uint32_t bad_actual = 0U;
    uint32_t bad_expected = 0U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap85_write_words(old_paddr, old_words, 4U);
    ai_ap85_write_words(src_paddr, src_words, 3U);
    write32(fault_paddr, src_words[3]);
    ai_ap85_fill_words(dump_paddr, 0xdead8505U, 4U);

    ai_ap85_prepare_pages(old_page, low_page, high_page, dump_page);

    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg2e32_to_v8((const uint32_t *)old_vaddr, 2U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 setup preloads old destination fields before strided segment load fault test");

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vlsseg2e32_to_v8((const uint32_t *)src_vaddr, stride, 2U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-85 vlsseg2e32 cross-page field1 load fault reports segment index 1 and the field1 high-page tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vsseg2e32_from_v8((uint32_t *)dump_vaddr, 2U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 dump after vlsseg2e32 fault completes without exception");

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-85 strided segment load keeps the faulting segment transactionally old while preserving completed prefix segment data",
        ai_ap85_check_words(dump_paddr, expected_dump, 4U, &bad_index,
            &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    ai_ap85_cleanup_hs();

    TEST_END();
}

bool ai_arch_ap85_vlseg2e32_tail_policy_after_fault_no_partial_dest_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x580U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x580U;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 12U;
    const uintptr_t src_paddr = phys_page_base(low_page) + PAGE_SIZE - 12U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t fault_paddr = phys_page_base(high_page);
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x580U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x580U;
    const uint32_t old_words[8] = {
        0x85060010U, 0x85060020U, 0x85060030U, 0x85060040U,
        0x85060050U, 0x85060060U, 0x85060070U, 0x85060080U,
    };
    const uint32_t src_words[4] = {
        0x8506a001U, 0x8506b002U, 0x8506c003U, 0x8506d004U,
    };
    const uint32_t expected_dump[8] = {
        src_words[0], src_words[1], old_words[2], old_words[3],
        old_words[4], old_words[5], old_words[6], old_words[7],
    };
    unsigned bad_index = 0U;
    uint32_t bad_actual = 0U;
    uint32_t bad_expected = 0U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap85_write_words(old_paddr, old_words, 8U);
    ai_ap85_write_words(src_paddr, src_words, 3U);
    write32(fault_paddr, src_words[3]);
    ai_ap85_fill_words(dump_paddr, 0xdead8506U, 8U);

    ai_ap85_prepare_pages(old_page, low_page, high_page, dump_page);

    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg2e32_to_v8((const uint32_t *)old_vaddr, 4U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 setup preloads four old destination segments for active-suffix and tail observation");

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vlseg2e32_to_v8((const uint32_t *)src_vaddr, 3U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-85 vlseg2e32 middle fault reports segment1 while leaving active suffix and tail segments for later observation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap85_vsseg2e32_from_v8((uint32_t *)dump_vaddr, 4U);
    AI_AP85_ASSERT_NO_EXCEPT("AP-85 dump after tail-policy segment fault completes without exception");

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-85 faulting segment suppression does not corrupt active suffix elements or tail-observed guard data after the trap",
        ai_ap85_check_words(dump_paddr, expected_dump, 8U, &bad_index,
            &bad_actual, &bad_expected),
        "bad_index=%u actual=0x%x expected=0x%x",
        bad_index, bad_actual, bad_expected
    );

    ai_ap85_cleanup_hs();

    TEST_END();
}
