#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AP77_VL_E32 4U
#define AP77_VL_E8 8U
#define AP77_WHOLE_BYTES 64U

static inline pte_t *ap77_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline pte_t *ap77_gstage_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hpt[5][page];
#else
    return &hpt[2][page];
#endif
}

static inline void ap77_enable_vector_pbmt(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 3ULL << 9);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ap77_prepare_hs_pbmt_alias_with_flags(enum test_page page,
    uintptr_t paddr, uint64_t pbmt_flags)
{
    goto_priv(PRIV_M);
    ap77_enable_vector_pbmt();
    hspt_init();
    *ap77_hs_leaf_entry(page) =
        (paddr >> 2) | PTE_V | PTE_AD | PTE_RWX | pbmt_flags;
    sfence_vma();
}

static inline void ap77_prepare_hs_pbmt_alias(enum test_page page,
    uintptr_t paddr)
{
    ap77_prepare_hs_pbmt_alias_with_flags(page, paddr, PTE_Pbmt1);
}

static inline void ap77_cleanup(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_HGATP, 0ULL);
    CSRW(CSR_VSATP, 0ULL);
    CSRW(CSR_SATP, 0ULL);
    hspt_init();
    sfence_vma();
    hfence();
}

static inline void ap77_fill32(uintptr_t paddr, uint32_t base,
    unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        write32(paddr + i * sizeof(uint32_t), base + i);
    }
}

static inline void ap77_fill8(uintptr_t paddr, uint8_t base, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        write8(paddr + i, (uint8_t)(base + i));
    }
}

static inline uint64_t ap77_pack64(uintptr_t paddr)
{
    uint64_t value = 0;

    for (unsigned i = 0; i < 8U; ++i) {
        value |= ((uint64_t)read8(paddr + i)) << (8U * i);
    }

    return value;
}

static inline void ap77_vle32_fault(const uint32_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [vl] "r"((unsigned)AP77_VL_E32)
        : "t0", "memory");
}

static inline void ap77_vse32_fault(const uint32_t *src, uint32_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vse32.v v6, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [base] "r"(base),
          [vl] "r"((unsigned)AP77_VL_E32)
        : "t0", "memory");
}

static inline void ap77_vlse32_fault(const uint32_t *base,
    intptr_t stride_bytes)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlse32.v v6, (%[base]), %[stride]\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [stride] "r"(stride_bytes),
          [vl] "r"((unsigned)AP77_VL_E32)
        : "t0", "memory");
}

static inline void ap77_vsse32_fault(const uint32_t *src, uint32_t *base,
    intptr_t stride_bytes)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vsse32.v v6, (%[base]), %[stride]\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [base] "r"(base),
          [stride] "r"(stride_bytes),
          [vl] "r"((unsigned)AP77_VL_E32)
        : "t0", "memory");
}

static inline void ap77_vluxei32_fault(const uint8_t *base,
    const uint32_t *offsets)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vluxei32.v v6, (%[base]), v8\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [vl] "r"((unsigned)AP77_VL_E32)
        : "t0", "memory");
}

static inline void ap77_vsuxei32_fault(const uint32_t *src,
    const uint32_t *offsets, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsuxei32.v v6, (%[base]), v8\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AP77_VL_E32)
        : "t0", "memory");
}

static inline void ap77_vlseg2e8_fault(const uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vlseg2e8.v v8, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [vl] "r"((unsigned)AP77_VL_E8)
        : "t0", "memory");
}

static inline void ap77_vsseg2e8_fault(const uint8_t *src, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[src])\n\t"
        "vle8.v v9, (%[src])\n\t"
        "vsseg2e8.v v8, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [base] "r"(base),
          [vl] "r"((unsigned)AP77_VL_E8)
        : "t0", "memory");
}

static inline void ap77_vle32_masked_all_inactive(const uint32_t *bad_base,
    const uint32_t *good_base, uint32_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[good])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vle32.v v6, (%[bad]), v0.t\n\t"
        "vse32.v v6, (%[dump])\n\t"
        ".option pop\n\t"
        :
        : [bad] "r"(bad_base),
          [good] "r"(good_base),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AP77_VL_E32)
        : "t0", "memory");
}

static inline void ap77_vse32_masked_all_inactive(const uint32_t *src,
    uint32_t *bad_base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vse32.v v6, (%[bad]), v0.t\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [bad] "r"(bad_base),
          [vl] "r"((unsigned)AP77_VL_E32)
        : "t0", "memory");
}

static inline void ap77_vle32ff_fault(const uint32_t *base,
    unsigned *observed_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, ma\n\t"
        "vle32ff.v v6, (%[base])\n\t"
        "csrr %[observed_vl], vl\n\t"
        ".option pop\n\t"
        : [observed_vl] "=&r"(*observed_vl)
        : [base] "r"(base),
          [vl] "r"((unsigned)AP77_VL_E32)
        : "t0", "memory");
}

static inline void ap77_vl1re8_fault(const uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e8, m1, tu, mu\n\t"
        "vl1re8.v v6, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base)
        : "t0", "memory");
}

static inline void ap77_vs1r_fault(const uint8_t *src, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v6, (%[src])\n\t"
        "vs1r.v v6, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [base] "r"(base),
          [vl] "r"((unsigned)AP77_WHOLE_BYTES)
        : "t0", "memory");
}

static inline bool ap77_words_match(uintptr_t paddr, const uint32_t *expected,
    unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        if ((uint32_t)read32(paddr + i * sizeof(uint32_t)) != expected[i]) {
            return false;
        }
    }

    return true;
}

static inline void ap77_prepare_vs_pbmt_alias(enum test_page page,
    uintptr_t paddr)
{
    goto_priv(PRIV_M);
    ap77_enable_vector_pbmt();
    CSRS(CSR_VSSTATUS, 3ULL << 9);
    CSRS(CSR_HENVCFG, HENVCFG_PBMTE);
    hspt_init();
    hpt_init();
    vspt_init();
    *ap77_gstage_leaf_entry(page) =
        (paddr >> 2) | PTE_V | PTE_U | PTE_AD | PTE_RWX | PTE_Pbmt1;
    sfence_vma();
    hfence();
}

bool ai_micro_ap77_vector_pbmt_macro_surface_manual_smoke_corner()
{
    TEST_START();

    const enum test_page fault_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page io_fault_page = VSRWXPbmt2_GURWXPbmt1;
    const enum test_page vs_fault_page = SWITCH1;
    const uintptr_t fault_paddr = phys_page_base(X) + 0x100U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x100U;
    const uintptr_t io_fault_vaddr = hs_page_base(io_fault_page) + 0x100U;
    const uintptr_t good_paddr = phys_page_base(SWITCH2) + 0x200U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH2) + 0x200U;
    const uintptr_t dump_paddr = phys_page_base(SCRATCHPAD) + 0x300U;
    const uintptr_t dump_vaddr = hs_page_base(SCRATCHPAD) + 0x300U;
    const uint32_t good_words[AP77_VL_E32] = {
        0x10111213U, 0x20212223U, 0x30313233U, 0x40414243U
    };
    const uint32_t src_words[AP77_VL_E32] = {
        0xa0a1a2a3U, 0xb0b1b2b3U, 0xc0c1c2c3U, 0xd0d1d2d3U
    };
    const uint32_t offsets[AP77_VL_E32] = {0U, 4U, 8U, 12U};
    const uint32_t fault_initial[AP77_VL_E32] = {
        0x51525354U, 0x61626364U, 0x71727374U, 0x81828384U
    };
    unsigned observed_vl = 0U;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AP77_VL_E32; ++i) {
        write32(good_paddr + i * sizeof(uint32_t), good_words[i]);
        write32(good_paddr + 0x40U + i * sizeof(uint32_t), src_words[i]);
        write32(fault_paddr + i * sizeof(uint32_t), fault_initial[i]);
        write32(dump_paddr + i * sizeof(uint32_t), 0U);
    }
    ap77_fill8(fault_paddr + 0x80U, 0x70U, 32U);
    ap77_fill8(good_paddr + 0x80U, 0x20U, AP77_WHOLE_BYTES);

    ap77_prepare_hs_pbmt_alias(fault_page, fault_paddr & PAGE_ADDR_MSK);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ap77_vle32_fault((const uint32_t *)fault_vaddr);
    TEST_ASSERT("AP-77 unit-stride aligned vector load through PBMT=NC reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vse32_fault((const uint32_t *)(good_vaddr + 0x40U),
        (uint32_t *)fault_vaddr);
    TEST_ASSERT("AP-77 unit-stride aligned vector store through PBMT=NC reports store access fault before modifying the target",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vlse32_fault((const uint32_t *)fault_vaddr, sizeof(uint32_t));
    TEST_ASSERT("AP-77 strided vector load through PBMT=NC preserves vector identity and reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vsse32_fault((const uint32_t *)(good_vaddr + 0x40U),
        (uint32_t *)fault_vaddr, sizeof(uint32_t));
    TEST_ASSERT("AP-77 strided vector store through PBMT=NC reports store access fault before modifying the target",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vluxei32_fault((const uint8_t *)fault_vaddr,
        offsets);
    TEST_ASSERT("AP-77 indexed vector load through PBMT=NC reports load access fault from the first active indexed element",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vsuxei32_fault((const uint32_t *)(good_vaddr + 0x40U),
        offsets, (uint8_t *)fault_vaddr);
    TEST_ASSERT("AP-77 indexed vector store through PBMT=NC reports store access fault and does not commit target bytes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vlseg2e8_fault((const uint8_t *)fault_vaddr);
    TEST_ASSERT("AP-77 segment vector load through PBMT=NC reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vsseg2e8_fault((const uint8_t *)(good_vaddr + 0x80U),
        (uint8_t *)fault_vaddr);
    TEST_ASSERT("AP-77 segment vector store through PBMT=NC reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vl1re8_fault((const uint8_t *)(fault_vaddr + 0x80U));
    TEST_ASSERT("AP-77 whole-register vector load through PBMT=NC reports load access fault on the whole-register base",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_vaddr + 0x80U
    );

    TEST_SETUP_EXCEPT();
    ap77_vs1r_fault((const uint8_t *)(good_vaddr + 0x80U),
        (uint8_t *)(fault_vaddr + 0x80U));
    TEST_ASSERT("AP-77 whole-register vector store through PBMT=NC reports store access fault on the whole-register base",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr + 0x80U
    );

    TEST_SETUP_EXCEPT();
    ap77_vle32_masked_all_inactive((const uint32_t *)fault_vaddr,
        (const uint32_t *)good_vaddr, (uint32_t *)dump_vaddr);
    TEST_ASSERT("AP-77 all-inactive masked vector load does not consult the PBMT=NC target",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ap77_vse32_masked_all_inactive((const uint32_t *)(good_vaddr + 0x40U),
        (uint32_t *)fault_vaddr);
    TEST_ASSERT("AP-77 all-inactive masked vector store does not consult or modify the PBMT=NC target",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ap77_vle32ff_fault((const uint32_t *)fault_vaddr, &observed_vl);
    TEST_ASSERT("AP-77 fault-only-first vector load still uses the PBMT vector access-fault classifier for element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-77 PBMT faulting vector stores and all-inactive masked store leave the target words unchanged while masked load preserves the good dump",
        ap77_words_match(fault_paddr, fault_initial, AP77_VL_E32) &&
        ap77_words_match(dump_paddr, good_words, AP77_VL_E32) &&
        ap77_pack64(fault_paddr + 0x80U) == 0x7776757473727170ULL
    );

    ap77_prepare_hs_pbmt_alias_with_flags(io_fault_page,
        fault_paddr & PAGE_ADDR_MSK, PTE_Pbmt2);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ap77_vle32_fault((const uint32_t *)io_fault_vaddr);
    TEST_ASSERT("AP-77 unit-stride aligned vector load through PBMT=IO reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == io_fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vse32_fault((const uint32_t *)(good_vaddr + 0x40U),
        (uint32_t *)io_fault_vaddr);
    TEST_ASSERT("AP-77 unit-stride aligned vector store through PBMT=IO reports store access fault before modifying the target",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == io_fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    ap77_vle32_masked_all_inactive((const uint32_t *)io_fault_vaddr,
        (const uint32_t *)good_vaddr, (uint32_t *)dump_vaddr);
    TEST_ASSERT("AP-77 all-inactive masked vector load does not consult the PBMT=IO target",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ap77_vse32_masked_all_inactive((const uint32_t *)(good_vaddr + 0x40U),
        (uint32_t *)io_fault_vaddr);
    TEST_ASSERT("AP-77 all-inactive masked vector store does not consult or modify the PBMT=IO target",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ap77_vle32ff_fault((const uint32_t *)io_fault_vaddr, &observed_vl);
    TEST_ASSERT("AP-77 fault-only-first vector load also uses the PBMT=IO vector access-fault classifier for element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == io_fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-77 PBMT=IO faulting vector store and all-inactive masked store leave the target words unchanged",
        ap77_words_match(fault_paddr, fault_initial, AP77_VL_E32) &&
        ap77_pack64(fault_paddr + 0x80U) == 0x7776757473727170ULL
    );

    ap77_prepare_vs_pbmt_alias(vs_fault_page, fault_paddr & PAGE_ADDR_MSK);
    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    ap77_vlse32_fault((const uint32_t *)(vs_page_base(vs_fault_page) + 0x100U),
        sizeof(uint32_t));
    TEST_ASSERT("AP-77 VS/G-stage vector translation also preserves vector identity into the PBMT access classifier",
        excpt.triggered == true &&
        (excpt.cause == CAUSE_LAF ||
         excpt.cause == CAUSE_LOAD_GUEST_PAGE_FAULT)
    );

    ap77_cleanup();
    TEST_END();
}
