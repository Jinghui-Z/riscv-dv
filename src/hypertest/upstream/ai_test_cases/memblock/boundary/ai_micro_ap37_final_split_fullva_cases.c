#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <string.h>

#define AP37_SV39_MODE 39U
#define AP37_SV48_MODE 48U
#define AP37_SATP_MODE_39 (8ULL << 60)
#define AP37_SATP_MODE_48 (9ULL << 60)
#define AP37_HIGH_PPN_OVERFLOW_BIT (1ULL << 46)

#define AP37_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AP37_MCONTROL6_LOAD 0x1ULL
#define AP37_MCONTROL6_STORE 0x2ULL

static pte_t ap37_sv39_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ap37_sv39_test_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ap37_sv39_test_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ap37_sv39_high_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ap37_sv39_high_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

static pte_t ap37_sv48_high_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ap37_sv48_high_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ap37_sv48_high_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ap37_sv48_negative_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ap37_sv48_negative_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ap37_sv48_negative_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

static inline uintptr_t ap37_boundary(unsigned mode_bits)
{
    return (mode_bits == AP37_SV39_MODE) ? (1ULL << 38) : (1ULL << 47);
}

static inline uintptr_t ap37_last_page_base(unsigned mode_bits)
{
    return ap37_boundary(mode_bits) - PAGE_SIZE;
}

static inline const char *ap37_mode_name(unsigned mode_bits)
{
    return (mode_bits == AP37_SV39_MODE) ? "sv39" : "sv48";
}

static inline void ap37_zero_pt(pte_t *pt)
{
    for (unsigned i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0ULL;
    }
}

static inline pte_t ap37_next_pt_pte(pte_t *pt)
{
    return PTE_V | (((uintptr_t)pt) >> 2);
}

static inline pte_t ap37_leaf_pte(uintptr_t paddr, uint64_t flags)
{
    return (paddr >> 2) | flags;
}

static inline pte_t *ap37_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline pte_t *ap37_gstage_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hpt[5][page];
#else
    return &hpt[2][page];
#endif
}

static void ap37_install_sv39_final_page(uintptr_t paddr, uint64_t flags)
{
    goto_priv(PRIV_M);

    ap37_zero_pt(ap37_sv39_root);
    ap37_zero_pt(ap37_sv39_test_l1);
    ap37_zero_pt(ap37_sv39_test_l0);
    ap37_zero_pt(ap37_sv39_high_l1);
    ap37_zero_pt(ap37_sv39_high_l0);

    uintptr_t addr = 0ULL;
    for (unsigned i = 0; i < 4U; ++i) {
        ap37_sv39_root[i] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);
        addr += (1ULL << 30);
    }

    ap37_sv39_root[4] = ap37_next_pt_pte(ap37_sv39_test_l1);
    ap37_sv39_test_l1[0] = ap37_next_pt_pte(ap37_sv39_test_l0);
    addr = TEST_PPAGE_BASE;
    for (unsigned i = 0; i < TEST_PAGE_MAX; ++i) {
        ap37_sv39_test_l0[i] = ap37_leaf_pte(addr, PTE_V | PTE_AD | PTE_RWX);
        addr += PAGE_SIZE;
    }

    ap37_sv39_root[255] = ap37_next_pt_pte(ap37_sv39_high_l1);
    ap37_sv39_high_l1[511] = ap37_next_pt_pte(ap37_sv39_high_l0);
    ap37_sv39_high_l0[511] = ap37_leaf_pte(paddr, flags);

    CSRW(satp, (((uintptr_t)ap37_sv39_root) >> 12) | AP37_SATP_MODE_39);
    sfence_vma();
}

static void ap37_install_sv48_final_page(uintptr_t paddr, uint64_t flags)
{
    goto_priv(PRIV_M);

    hspt_init();
    ap37_zero_pt(ap37_sv48_high_l2);
    ap37_zero_pt(ap37_sv48_high_l1);
    ap37_zero_pt(ap37_sv48_high_l0);
    ap37_zero_pt(ap37_sv48_negative_l2);
    ap37_zero_pt(ap37_sv48_negative_l1);
    ap37_zero_pt(ap37_sv48_negative_l0);

    hspt[0][255] = ap37_next_pt_pte(ap37_sv48_high_l2);
    ap37_sv48_high_l2[511] = ap37_next_pt_pte(ap37_sv48_high_l1);
    ap37_sv48_high_l1[511] = ap37_next_pt_pte(ap37_sv48_high_l0);
    ap37_sv48_high_l0[511] = ap37_leaf_pte(paddr, flags);

    CSRW(satp, (((uintptr_t)hspt) >> 12) | AP37_SATP_MODE_48);
    sfence_vma();
}

static void ap37_map_sv48_negative_base_page(uintptr_t paddr, uint64_t flags)
{
    goto_priv(PRIV_M);

    ap37_zero_pt(ap37_sv48_negative_l2);
    ap37_zero_pt(ap37_sv48_negative_l1);
    ap37_zero_pt(ap37_sv48_negative_l0);

    hspt[0][256] = ap37_next_pt_pte(ap37_sv48_negative_l2);
    ap37_sv48_negative_l2[0] = ap37_next_pt_pte(ap37_sv48_negative_l1);
    ap37_sv48_negative_l1[0] = ap37_next_pt_pte(ap37_sv48_negative_l0);
    ap37_sv48_negative_l0[0] = ap37_leaf_pte(paddr, flags);
    sfence_vma();
}

static void ap37_install_final_page(unsigned mode_bits, uintptr_t paddr,
    uint64_t flags)
{
    if (mode_bits == AP37_SV39_MODE) {
        ap37_install_sv39_final_page(paddr, flags);
    } else {
        ap37_install_sv48_final_page(paddr, flags);
    }
}

static void ap37_reset_translation(void)
{
    goto_priv(PRIV_M);
    CSRW(satp, 0ULL);
    hspt_init();
    sfence_vma();
}

static inline void ap37_write_bytes(uintptr_t addr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ap37_bytes_match(uintptr_t addr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline uint64_t ap37_ld_probe(uintptr_t addr, uint64_t sentinel)
{
    register uint64_t value asm("a4") = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value) : "r"(addr) : "memory");
    return value;
}

static inline uint64_t ap37_lh_probe(uintptr_t addr, uint64_t sentinel)
{
    register uint64_t value asm("a4") = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lh %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value) : "r"(addr) : "memory");
    return value;
}

static inline uint64_t ap37_lw_probe(uintptr_t addr, uint64_t sentinel)
{
    register uint64_t value asm("a4") = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lw %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value) : "r"(addr) : "memory");
    return value;
}

static inline void ap37_sd(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %1, 0(%0)\n\t"
        ".option pop\n\t"
        :: "r"(addr), "r"(value) : "memory");
}

static inline void ap37_sh(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sh %1, 0(%0)\n\t"
        ".option pop\n\t"
        :: "r"(addr), "r"(value) : "memory");
}

static inline void ap37_sw(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sw %1, 0(%0)\n\t"
        ".option pop\n\t"
        :: "r"(addr), "r"(value) : "memory");
}

static inline void ap37_vle32_masked_zero_to_v6(const uint32_t *src,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %1, e32, m1, tu, mu\n\t"
        "vmv.v.i v0, 0\n\t"
        "vle32.v v6, (%0), v0.t\n\t"
        ".option pop\n\t"
        :: "r"(src), "r"(vl) : "t0", "memory");
}

static void ap37_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0" ::: "memory");
    CSRW(CSR_TDATA2, 0ULL);
}

static void ap37_arm_mcontrol6_slot(size_t slot, uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0" ::: "memory");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, AP37_MCONTROL6_MSU_BASE | access_bits);
}

static uint64_t ap37_enable_machine_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static void ap37_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static uint64_t ap37_expected_low_after_sd(uint64_t original, uint64_t value)
{
    return (original & 0xffffffffULL) | ((value & 0xffffffffULL) << 32);
}

static uint64_t ap37_expected_unaligned_ld_plus1(uint64_t low,
    uint64_t high)
{
    return (low >> 8) | ((high & 0xffULL) << 56);
}

static uint64_t ap37_expected_guest_tval2(uintptr_t vaddr)
{
    return ((uint64_t)vaddr) >> 2;
}

bool ai_micro_ap37_scalar_ld_final_split_sv39_sv48_fullva_manual_corner()
{
    TEST_START();

    const enum test_page backing_page = SWITCH1;
    const uintptr_t backing_paddr = phys_page_base(backing_page);
    const uint64_t low_seed = 0x0706050403020100ULL;
    const uint64_t load_sentinel = 0xfeedface37000001ULL;
    const unsigned modes[] = { AP37_SV39_MODE, AP37_SV48_MODE };

    for (unsigned i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        const unsigned mode = modes[i];
        const uintptr_t boundary = ap37_boundary(mode);
        const uintptr_t fault_vaddr = boundary - 4U;

        goto_priv(PRIV_M);
        write64(backing_paddr + PAGE_SIZE - 8U, low_seed);
        ap37_install_final_page(mode, backing_paddr, PTE_V | PTE_AD | PTE_RWX);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t observed = ap37_ld_probe(fault_vaddr, load_sentinel);
        TEST_ASSERT("AP-37 scalar ld final split must run fullVA check in both Sv39 and Sv48 and report the first non-canonical final-split VA",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == boundary &&
            observed == load_sentinel,
            "mode=%s observed=0x%llx sentinel=0x%llx tval=0x%llx expected=0x%llx",
            ap37_mode_name(mode),
            (unsigned long long)observed,
            (unsigned long long)load_sentinel,
            (unsigned long long)excpt.tval,
            (unsigned long long)boundary
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-37 scalar ld final split fullVA fault leaves the mapped low-half backing bytes unchanged",
            read64(backing_paddr + PAGE_SIZE - 8U) == low_seed,
            "mode=%s", ap37_mode_name(mode)
        );
        ap37_reset_translation();
    }

    TEST_END();
}

bool ai_micro_ap37_scalar_sd_final_split_sv39_sv48_fullva_manual_corner()
{
    TEST_START();

    const enum test_page backing_page = SWITCH1;
    const uintptr_t backing_paddr = phys_page_base(backing_page);
    const uint64_t low_seed = 0x8877665544332211ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t low_fragment = ap37_expected_low_after_sd(low_seed, store_value);
    const unsigned modes[] = { AP37_SV39_MODE, AP37_SV48_MODE };

    for (unsigned i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        const unsigned mode = modes[i];
        const uintptr_t boundary = ap37_boundary(mode);
        const uintptr_t fault_vaddr = boundary - 4U;

        goto_priv(PRIV_M);
        write64(backing_paddr + PAGE_SIZE - 8U, low_seed);
        ap37_install_final_page(mode, backing_paddr, PTE_V | PTE_AD | PTE_RWX);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ap37_sd(fault_vaddr, store_value);
        TEST_ASSERT("AP-37 scalar sd final split must run fullVA check in both Sv39 and Sv48 and report the first non-canonical final-split VA",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SPF &&
            excpt.tval == boundary,
            "mode=%s tval=0x%llx expected=0x%llx",
            ap37_mode_name(mode),
            (unsigned long long)excpt.tval,
            (unsigned long long)boundary
        );

        goto_priv(PRIV_M);
        uint64_t low_after = read64(backing_paddr + PAGE_SIZE - 8U);
        TEST_ASSERT("AP-37 scalar sd final split fullVA fault must not retire as a completed store; low fragment follows platform precise-side-effect policy but no silent success is allowed",
            low_after == low_seed || low_after == low_fragment,
            "mode=%s low_after=0x%llx seed=0x%llx low_fragment=0x%llx",
            ap37_mode_name(mode),
            (unsigned long long)low_after,
            (unsigned long long)low_seed,
            (unsigned long long)low_fragment
        );
        ap37_reset_translation();
    }

    TEST_END();
}

bool ai_micro_ap37_scalar_offset_matrix_final_split_fullva_manual_corner()
{
    TEST_START();

    const enum test_page backing_page = SWITCH1;
    const uintptr_t backing_paddr = phys_page_base(backing_page);
    const uint64_t seed = 0x1716151413121110ULL;
    const uint64_t sentinel = 0xfeedface37000003ULL;
    const unsigned mode = AP37_SV48_MODE;
    const uintptr_t boundary = ap37_boundary(mode);

    struct ap37_load_probe {
        const char *name;
        uintptr_t low_bytes;
        uint64_t (*probe)(uintptr_t, uint64_t);
    };

    const struct ap37_load_probe probes[] = {
        { "lh+1", 1U, ap37_lh_probe },
        { "lw+1", 1U, ap37_lw_probe },
        { "lw+2", 2U, ap37_lw_probe },
        { "lw+3", 3U, ap37_lw_probe },
        { "ld+1", 1U, ap37_ld_probe },
        { "ld+4", 4U, ap37_ld_probe },
        { "ld+7", 7U, ap37_ld_probe },
    };

    for (unsigned i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i) {
        const uintptr_t fault_vaddr = boundary - probes[i].low_bytes;

        goto_priv(PRIV_M);
        write64(backing_paddr + PAGE_SIZE - 8U, seed);
        ap37_install_final_page(mode, backing_paddr, PTE_V | PTE_AD | PTE_RWX);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t observed = probes[i].probe(fault_vaddr, sentinel);
        TEST_ASSERT("AP-37 final-split fullVA offset matrix covers LH/LW/LD split offsets and always reports the final split boundary VA",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == boundary &&
            observed == sentinel,
            "probe=%s observed=0x%llx tval=0x%llx expected=0x%llx",
            probes[i].name,
            (unsigned long long)observed,
            (unsigned long long)excpt.tval,
            (unsigned long long)boundary
        );
        ap37_reset_translation();
    }

    TEST_END();
}

bool ai_micro_ap37_low_split_adjusted_fullva_priority_manual_corner()
{
    TEST_START();

    const enum test_page negative_page = SWITCH2;
    const enum test_page final_page = SWITCH1;
    const uintptr_t negative_paddr = phys_page_base(negative_page);
    const uintptr_t final_paddr = phys_page_base(final_page);
    const uintptr_t sv48_negative_base = 0xffff800000000000ULL;
    const uintptr_t sv48_positive_boundary = ap37_boundary(AP37_SV48_MODE);
    const uint64_t low_seed = 0x7766554433221100ULL;
    const uint64_t high_seed = 0xffeeddccbbaa9988ULL;
    const uint64_t final_seed = 0x1122334455667788ULL;
    const uint64_t sentinel = 0xfeedface3700000aULL;

    goto_priv(PRIV_M);
    write64(negative_paddr, low_seed);
    write64(negative_paddr + 8U, high_seed);
    write64(final_paddr + PAGE_SIZE - 8U, final_seed);
    ap37_install_final_page(AP37_SV48_MODE, final_paddr,
        PTE_V | PTE_AD | PTE_RWX);
    ap37_map_sv48_negative_base_page(negative_paddr,
        PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ap37_ld_probe(sv48_negative_base + 1U, sentinel);
    TEST_ASSERT("AP-37 low split adjusted-down fullVA at the Sv48 negative canonical base remains legal and merges the unaligned load correctly",
        excpt.triggered == false &&
        observed == ap37_expected_unaligned_ld_plus1(low_seed, high_seed),
        "observed=0x%llx expected=0x%llx",
        (unsigned long long)observed,
        (unsigned long long)ap37_expected_unaligned_ld_plus1(low_seed,
            high_seed)
    );

    TEST_SETUP_EXCEPT();
    observed = ap37_ld_probe(sv48_positive_boundary - 7U, sentinel);
    TEST_ASSERT("AP-37 after a legal adjusted-low split, the positive-boundary final split still reports the final non-canonical fullVA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == sv48_positive_boundary &&
        observed == sentinel,
        "observed=0x%llx tval=0x%llx expected=0x%llx",
        (unsigned long long)observed,
        (unsigned long long)excpt.tval,
        (unsigned long long)sv48_positive_boundary
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-37 low-split adjusted fullVA priority sequence leaves both backing pages unchanged",
        read64(negative_paddr) == low_seed &&
        read64(negative_paddr + 8U) == high_seed &&
        read64(final_paddr + PAGE_SIZE - 8U) == final_seed
    );
    ap37_reset_translation();

    TEST_END();
}

bool ai_micro_ap37_leaf_overflow_final_split_access_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const uintptr_t vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uint64_t low_seed = 0x7060504030201000ULL;
    const uint64_t high_seed = 0x8f8e8d8c8b8a8988ULL;
    const uint64_t load_sentinel = 0xfeedface37000004ULL;
    const uint64_t store_value = 0x0123456789abcdefULL;
    const uint64_t low_fragment = ap37_expected_low_after_sd(low_seed, store_value);

    goto_priv(PRIV_M);
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    hspt_init();
    *ap37_hs_leaf_entry(high_page) |= AP37_HIGH_PPN_OVERFLOW_BIT;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ap37_ld_probe(vaddr, load_sentinel);
    TEST_ASSERT("AP-37 final split leaf-PPN overflow on a split load reports load access fault at the high/final split VA and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == high_vaddr &&
        observed == load_sentinel,
        "observed=0x%llx tval=0x%llx expected=0x%llx",
        (unsigned long long)observed,
        (unsigned long long)excpt.tval,
        (unsigned long long)high_vaddr
    );

    goto_priv(PRIV_M);
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ap37_sd(vaddr, store_value);
    TEST_ASSERT("AP-37 final split leaf-PPN overflow on a split store reports store access fault at the high/final split VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == high_vaddr,
        "tval=0x%llx expected=0x%llx",
        (unsigned long long)excpt.tval,
        (unsigned long long)high_vaddr
    );

    goto_priv(PRIV_M);
    uint64_t low_after = read64(low_paddr);
    TEST_ASSERT("AP-37 split store with final leaf-PPN overflow does not write the high-half backing word; low fragment follows platform precise-side-effect policy",
        (low_after == low_seed || low_after == low_fragment) &&
        read64(high_paddr) == high_seed,
        "low_after=0x%llx high_after=0x%llx",
        (unsigned long long)low_after,
        (unsigned long long)read64(high_paddr)
    );

    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap37_vector_load_final_split_fullva_vstart_manual_corner()
{
    TEST_START();

    const enum test_page backing_page = SWITCH1;
    const enum test_page old_page = SWITCH2;
    const enum test_page scratch_page = SCRATCHPAD;
    const unsigned mode = AP37_SV48_MODE;
    const uintptr_t boundary = ap37_boundary(mode);
    const uintptr_t fault_vaddr = boundary - 7U;
    const uintptr_t backing_paddr = phys_page_base(backing_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x240U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x240U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x280U;
    const uintptr_t scratch_paddr = phys_page_base(scratch_page) + 0x280U;
    const uint8_t src_bytes[16] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab,
        0xac, 0xad, 0xae, 0xaf
    };
    const uint8_t old_bytes[16] = {
        0xd0, 0xd1, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb,
        0xdc, 0xdd, 0xde, 0xdf
    };
    uint8_t expected[16];
    uint64_t observed_vstart = 0ULL;

    memcpy(expected, old_bytes, sizeof(expected));
    memcpy(expected, src_bytes, 4U);

    goto_priv(PRIV_M);
    ap37_write_bytes(backing_paddr + PAGE_SIZE - 7U, src_bytes, 7U);
    ap37_write_bytes(old_paddr, old_bytes, sizeof(old_bytes));
    ap37_write_bytes(scratch_paddr, old_bytes, sizeof(old_bytes));
    ap37_install_final_page(mode, backing_paddr, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("AP-37 vector load final-split fullVA test preloads v6 with a known old payload",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-37 vector unit-stride load whose element1 final split crosses the fullVA boundary traps with vstart=1 and final-split tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == boundary &&
        observed_vstart == 1ULL,
        "vstart=0x%llx tval=0x%llx expected=0x%llx",
        (unsigned long long)observed_vstart,
        (unsigned long long)excpt.tval,
        (unsigned long long)boundary
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 4);
    TEST_ASSERT("AP-37 vector load final-split fullVA test can dump v6 after the fault for destination footprint checking",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-37 vector load final-split fullVA fault updates only prefix element0 and leaves the faulting/suffix destination bytes old",
        ap37_bytes_match(scratch_paddr, expected, sizeof(expected))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    ap37_reset_translation();

    TEST_END();
}

bool ai_micro_ap37_vector_store_final_split_fullva_vstart_manual_corner()
{
    TEST_START();

    const enum test_page backing_page = SWITCH1;
    const enum test_page src_page = SWITCH2;
    const unsigned mode = AP37_SV48_MODE;
    const uintptr_t boundary = ap37_boundary(mode);
    const uintptr_t fault_vaddr = boundary - 7U;
    const uintptr_t backing_paddr = phys_page_base(backing_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x300U;
    const uint8_t dest_initial[16] = {
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
    uint8_t expected[7];
    uint64_t observed_vstart = 0ULL;

    memcpy(expected, dest_initial, sizeof(expected));
    memcpy(expected, src_pattern, 4U);

    goto_priv(PRIV_M);
    ap37_write_bytes(backing_paddr + PAGE_SIZE - 7U, dest_initial, 7U);
    ap37_write_bytes(src_paddr, src_pattern, sizeof(src_pattern));
    ap37_install_final_page(mode, backing_paddr, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("AP-37 vector store final-split fullVA test preloads v6 with the source payload",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-37 vector unit-stride store whose element1 final split crosses the fullVA boundary traps with vstart=1 and final-split tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == boundary &&
        observed_vstart == 1ULL,
        "vstart=0x%llx tval=0x%llx expected=0x%llx",
        (unsigned long long)observed_vstart,
        (unsigned long long)excpt.tval,
        (unsigned long long)boundary
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-37 vector store final-split fullVA fault commits at most the prefix element and must not expose faulting element low bytes",
        ap37_bytes_match(backing_paddr + PAGE_SIZE - 7U, expected, sizeof(expected))
    );
    TEST_ASSERT("AP-37 vector store final-split fullVA fault leaves the source vector backing payload unchanged",
        ap37_bytes_match(src_paddr, src_pattern, sizeof(src_pattern))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    ap37_reset_translation();

    TEST_END();
}

bool ai_micro_ap37_masked_vector_final_split_fullva_ignored_manual_corner()
{
    TEST_START();

    const enum test_page backing_page = SWITCH1;
    const enum test_page old_page = SWITCH2;
    const enum test_page scratch_page = SCRATCHPAD;
    const unsigned mode = AP37_SV48_MODE;
    const uintptr_t boundary = ap37_boundary(mode);
    const uintptr_t masked_bad_vaddr = boundary - 3U;
    const uintptr_t backing_paddr = phys_page_base(backing_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x340U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x340U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x380U;
    const uintptr_t scratch_paddr = phys_page_base(scratch_page) + 0x380U;
    const uint8_t old_bytes[16] = {
        0x51, 0x52, 0x53, 0x54,
        0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x5b, 0x5c,
        0x5d, 0x5e, 0x5f, 0x60
    };
    const uint8_t boundary_seed[7] = {
        0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7
    };

    goto_priv(PRIV_M);
    ap37_write_bytes(backing_paddr + PAGE_SIZE - 7U, boundary_seed,
        sizeof(boundary_seed));
    ap37_write_bytes(old_paddr, old_bytes, sizeof(old_bytes));
    ap37_write_bytes(scratch_paddr, old_bytes, sizeof(old_bytes));
    ap37_install_final_page(mode, backing_paddr, PTE_V | PTE_AD | PTE_RWX);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("AP-37 masked vector final-split fullVA test preloads v6 before the all-inactive masked access",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ap37_vle32_masked_zero_to_v6((const uint32_t *)masked_bad_vaddr, 1);
    TEST_ASSERT("AP-37 masked-off vector element whose address would cross the final-split fullVA boundary must not raise a false page fault",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 4);
    TEST_ASSERT("AP-37 masked-off vector final-split fullVA test can dump v6 after the masked access",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-37 masked-off vector final-split fullVA access leaves the destination register image unchanged",
        ap37_bytes_match(scratch_paddr, old_bytes, sizeof(old_bytes))
    );
    TEST_ASSERT("AP-37 masked-off vector final-split fullVA access leaves the high-boundary backing bytes unchanged",
        ap37_bytes_match(backing_paddr + PAGE_SIZE - 7U, boundary_seed,
            sizeof(boundary_seed))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    ap37_reset_translation();

    TEST_END();
}

bool ai_micro_ap37_tlb_warm_trigger_final_split_fullva_manual_corner()
{
    TEST_START();

    const enum test_page backing_page = SWITCH1;
    const unsigned mode = AP37_SV48_MODE;
    const uintptr_t boundary = ap37_boundary(mode);
    const uintptr_t warm_vaddr = boundary - 8U;
    const uintptr_t fault_vaddr = boundary - 4U;
    const uintptr_t backing_paddr = phys_page_base(backing_page);
    const uint64_t seed = 0x78695a4b3c2d1e0fULL;
    const uint64_t load_sentinel = 0xfeedface37000008ULL;
    const uintptr_t trigger_addr = fault_vaddr;
    const uintptr_t second_split_trigger_addr = boundary;

    goto_priv(PRIV_M);
    write64(backing_paddr + PAGE_SIZE - 8U, seed);
    ap37_install_final_page(mode, backing_paddr, PTE_V | PTE_AD | PTE_RWX);
    ap37_clear_mcontrol6_slot(0);
    ap37_arm_mcontrol6_slot(0, trigger_addr, AP37_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ap37_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ap37_ld_probe(fault_vaddr, load_sentinel);
    TEST_ASSERT("AP-37 original-address load trigger remains visible before the final-split fullVA page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        observed == load_sentinel
    );

    ap37_restore_machine_status(saved_mstatus);
    ap37_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    ap37_arm_mcontrol6_slot(0, second_split_trigger_addr,
        AP37_MCONTROL6_LOAD);
    saved_mstatus = ap37_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ap37_ld_probe(fault_vaddr, load_sentinel);
    TEST_ASSERT("AP-37 a trigger armed on the final-split address must not make the internal second split bypass the fullVA page-fault priority",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == boundary &&
        observed == load_sentinel,
        "cause=0x%llx observed=0x%llx tval=0x%llx expected=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)observed,
        (unsigned long long)excpt.tval,
        (unsigned long long)boundary
    );

    ap37_restore_machine_status(saved_mstatus);
    ap37_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm_value = ap37_ld_probe(warm_vaddr, 0ULL);
    TEST_ASSERT("AP-37 TLB-warm probe first performs a legal aligned access on the final canonical page",
        excpt.triggered == false &&
        warm_value == seed
    );

    TEST_SETUP_EXCEPT();
    observed = ap37_ld_probe(fault_vaddr, load_sentinel);
    TEST_ASSERT("AP-37 final-split fullVA check must still fire after the last canonical page has a warm TLB entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == boundary &&
        observed == load_sentinel,
        "observed=0x%llx tval=0x%llx expected=0x%llx",
        (unsigned long long)observed,
        (unsigned long long)excpt.tval,
        (unsigned long long)boundary
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-37 trigger/TLB-warm final-split fullVA sequence leaves the backing payload unchanged",
        read64(backing_paddr + PAGE_SIZE - 8U) == seed
    );
    ap37_reset_translation();

    TEST_END();
}

bool ai_micro_ap37_vs_gstage_final_split_high_ppn_manual_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const uintptr_t vaddr = vs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_vaddr = vs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uint64_t low_seed = 0x4f4e4d4c4b4a4948ULL;
    const uint64_t high_seed = 0x5f5e5d5c5b5a5958ULL;
    const uint64_t load_sentinel = 0xfeedface37000009ULL;
    const uint64_t overflow_leaf =
        (phys_page_base(high_page) >> 2) |
        PTE_V | PTE_U | PTE_AD | PTE_RWX |
        AP37_HIGH_PPN_OVERFLOW_BIT;

    goto_priv(PRIV_M);
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    hspt_init();
    hpt_init();
    vspt_init();
    *ap37_gstage_leaf_entry(high_page) = 0ULL;
    sfence_vma();
    hfence();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ap37_ld_probe(vaddr, load_sentinel);
    TEST_ASSERT("AP-37 VS/G-stage split load whose final split lands on a G-stage-invalid leaf reports load guest-page fault with final-split GVA and GPA metadata",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_LGPF &&
        excpt.tval == high_vaddr &&
        excpt.tval2 == ap37_expected_guest_tval2(high_vaddr) &&
        observed == load_sentinel,
        "observed=0x%llx tval=0x%llx expected_tval=0x%llx tval2=0x%llx expected_tval2=0x%llx cause=0x%llx",
        (unsigned long long)observed,
        (unsigned long long)excpt.tval,
        (unsigned long long)high_vaddr,
        (unsigned long long)excpt.tval2,
        (unsigned long long)ap37_expected_guest_tval2(high_vaddr),
        (unsigned long long)excpt.cause
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-37 VS/G-stage final-split guest-page fault leaves both backing pages unchanged",
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed
    );

    *ap37_gstage_leaf_entry(high_page) = overflow_leaf;
    sfence_vma();
    hfence();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    observed = ap37_ld_probe(vaddr, load_sentinel);
    TEST_ASSERT("AP-37 VS/G-stage split load whose final split hits a high-PPN G-stage leaf reports access fault rather than guest-page fault",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_LAF &&
        excpt.cause != CAUSE_LGPF &&
        excpt.tval == high_vaddr &&
        observed == load_sentinel,
        "observed=0x%llx tval=0x%llx expected=0x%llx cause=0x%llx",
        (unsigned long long)observed,
        (unsigned long long)excpt.tval,
        (unsigned long long)high_vaddr,
        (unsigned long long)excpt.cause
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-37 VS/G-stage final-split high-PPN fault leaves the G-stage leaf and backing payload unchanged",
        *ap37_gstage_leaf_entry(high_page) == overflow_leaf &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed
    );

    *ap37_gstage_leaf_entry(high_page) =
        (phys_page_base(high_page) >> 2) | PTE_V | PTE_U | PTE_AD | PTE_RWX;
    sfence_vma();
    hfence();

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    observed = ap37_ld_probe(vaddr, load_sentinel);
    TEST_ASSERT("after repairing the G-stage high-PPN leaf, the same AP-37 split load recovers and returns the merged payload",
        excpt.triggered == false &&
        observed == ((high_seed & 0xffffffffULL) << 32) | (low_seed >> 32)
    );

    goto_priv(PRIV_M);
    CSRW(CSR_HGATP, 0ULL);
    CSRW(CSR_VSATP, 0ULL);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();
    hfence();

    TEST_END();
}
