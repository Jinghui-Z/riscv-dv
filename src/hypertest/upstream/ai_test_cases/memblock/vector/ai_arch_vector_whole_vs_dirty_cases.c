#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <stddef.h>

#define AI_ARCH_VWHOLE_VS_OFF    (9U)
#define AI_ARCH_VWHOLE_VS_MASK   (3ULL << AI_ARCH_VWHOLE_VS_OFF)
#define AI_ARCH_VWHOLE_VS_CLEAN  (2ULL << AI_ARCH_VWHOLE_VS_OFF)
#define AI_ARCH_VWHOLE_VS_DIRTY  (3ULL << AI_ARCH_VWHOLE_VS_OFF)
#define AI_ARCH_VWHOLE_SRC_BYTES 4096U
#define AI_ARCH_VWHOLE_AP110_SENTINEL 0xddU

enum ai_arch_vwhole_ap110_kind {
    AI_ARCH_VWHOLE_AP110_VL4RE8,
    AI_ARCH_VWHOLE_AP110_VL2RE16,
    AI_ARCH_VWHOLE_AP110_VL4RE32,
    AI_ARCH_VWHOLE_AP110_VL8RE64,
};

struct ai_arch_vwhole_ap110_result {
    bool preload_ok;
    bool fault_ok;
    bool dump_ok;
    bool data_ok;
    uint64_t cause;
    uint64_t tval;
    uint64_t expected_tval;
    uint64_t vstart;
    uint64_t expected_vstart;
    size_t bad_index;
    uint8_t bad_actual;
    uint8_t bad_expected;
};

static uint8_t ai_arch_vwhole_src[AI_ARCH_VWHOLE_SRC_BYTES]
    __attribute__((aligned(64)));

static inline void ai_arch_vwhole_enable_dirty(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_MSTATUS, AI_ARCH_VWHOLE_VS_MASK);
    CSRS(CSR_MSTATUS, AI_ARCH_VWHOLE_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_arch_vwhole_force_vs_clean(void)
{
    CSRC(CSR_MSTATUS, AI_ARCH_VWHOLE_VS_MASK);
    CSRS(CSR_MSTATUS, AI_ARCH_VWHOLE_VS_CLEAN);
}

static inline void ai_arch_vwhole_force_vs_off(void)
{
    CSRC(CSR_MSTATUS, AI_ARCH_VWHOLE_VS_MASK);
}

static inline uint64_t ai_arch_vwhole_sstatus_vs(void)
{
    return CSRR(CSR_SSTATUS) & AI_ARCH_VWHOLE_VS_MASK;
}

static inline void ai_arch_vwhole_prepare_hs(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

static inline void ai_arch_vwhole_config_e8(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e8, m1, ta, ma\n\t"
        ".option pop\n\t"
        :
        :
        : "t0", "memory");
}

static inline void ai_arch_vwhole_config_e64(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e64, m1, ta, ma\n\t"
        ".option pop\n\t"
        :
        :
        : "t0", "memory");
}

static inline void ai_arch_vwhole_seed_v6_1r(const uint8_t *src)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e8, m1, ta, ma\n\t"
        "vl1re8.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src)
        : "t0", "memory");
}

static inline void ai_arch_vwhole_seed_v8_8r(const uint8_t *src)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e8, m1, ta, ma\n\t"
        "vl8re8.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src)
        : "t0", "memory");
}

static inline void ai_arch_vwhole_vl1re8_fault(const uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vl1re8.v v6, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base)
        : "memory");
}

static inline void ai_arch_vwhole_vl8re64_fault(const uint64_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vl8re64.v v8, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base)
        : "memory");
}

static inline void ai_arch_vwhole_vs1r_fault(uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vs1r.v v6, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base)
        : "memory");
}

static inline void ai_arch_vwhole_vs8r_fault(uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vs8r.v v8, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base)
        : "memory");
}

static void ai_arch_vwhole_seed_src(void)
{
    for (unsigned i = 0; i < AI_ARCH_VWHOLE_SRC_BYTES; ++i) {
        ai_arch_vwhole_src[i] = (uint8_t)(0x31U + ((i * 13U) & 0x7fU));
    }
}

static inline pte_t *ai_arch_vwhole_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_vwhole_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_arch_vwhole_hs_leaf_entry(page) = (phys_page_base(page) >> 2) |
        flags;
}

static inline uint64_t ai_arch_vwhole_read_vlenb(void)
{
    return CSRR(CSR_VLENB);
}

static inline uint8_t ai_arch_vwhole_ap110_pattern(uint8_t seed, size_t idx)
{
    return (uint8_t)(seed + ((idx * 29U) & 0xffU) + (idx >> 3));
}

static void ai_arch_vwhole_ap110_write_pattern(uintptr_t paddr, size_t bytes,
    uint8_t seed)
{
    for (size_t i = 0; i < bytes; ++i) {
        write8(paddr + i, ai_arch_vwhole_ap110_pattern(seed, i));
    }
}

static void ai_arch_vwhole_ap110_fill(uintptr_t paddr, size_t bytes,
    uint8_t value)
{
    for (size_t i = 0; i < bytes; ++i) {
        write8(paddr + i, value);
    }
}

static bool ai_arch_vwhole_ap110_check_pattern(uintptr_t paddr, size_t bytes,
    uint8_t seed, size_t *bad_index, uint8_t *bad_actual,
    uint8_t *bad_expected)
{
    for (size_t i = 0; i < bytes; ++i) {
        const uint8_t expected = ai_arch_vwhole_ap110_pattern(seed, i);
        const uint8_t actual = read8(paddr + i);

        if (actual != expected) {
            *bad_index = i;
            *bad_actual = actual;
            *bad_expected = expected;
            return false;
        }
    }
    *bad_index = bytes;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static bool ai_arch_vwhole_ap110_check_old_new(uintptr_t paddr, size_t vlenb,
    unsigned regs, unsigned new_regs, uint8_t old_seed, uint8_t new_seed,
    size_t *bad_index, uint8_t *bad_actual, uint8_t *bad_expected)
{
    const size_t new_bytes = (size_t)new_regs * vlenb;
    const size_t total_bytes = (size_t)regs * vlenb;

    for (size_t i = 0; i < total_bytes; ++i) {
        const uint8_t seed = i < new_bytes ? new_seed : old_seed;
        const uint8_t expected = ai_arch_vwhole_ap110_pattern(seed, i);
        const uint8_t actual = read8(paddr + i);

        if (actual != expected) {
            *bad_index = i;
            *bad_actual = actual;
            *bad_expected = expected;
            return false;
        }
    }
    *bad_index = total_bytes;
    *bad_actual = 0U;
    *bad_expected = 0U;
    return true;
}

static inline unsigned ai_arch_vwhole_ap110_regs(
    enum ai_arch_vwhole_ap110_kind kind)
{
    switch (kind) {
    case AI_ARCH_VWHOLE_AP110_VL2RE16:
        return 2U;
    case AI_ARCH_VWHOLE_AP110_VL4RE8:
    case AI_ARCH_VWHOLE_AP110_VL4RE32:
        return 4U;
    case AI_ARCH_VWHOLE_AP110_VL8RE64:
    default:
        return 8U;
    }
}

static inline unsigned ai_arch_vwhole_ap110_eew_bytes(
    enum ai_arch_vwhole_ap110_kind kind)
{
    switch (kind) {
    case AI_ARCH_VWHOLE_AP110_VL2RE16:
        return 2U;
    case AI_ARCH_VWHOLE_AP110_VL4RE32:
        return 4U;
    case AI_ARCH_VWHOLE_AP110_VL8RE64:
        return 8U;
    case AI_ARCH_VWHOLE_AP110_VL4RE8:
    default:
        return 1U;
    }
}

static inline void ai_arch_vwhole_ap110_load(
    enum ai_arch_vwhole_ap110_kind kind, const void *base)
{
    switch (kind) {
    case AI_ARCH_VWHOLE_AP110_VL2RE16:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, zero, e16, m1, ta, ma\n\t"
            "vl2re16.v v8, (%[base])\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base)
            : "t0", "memory");
        break;
    case AI_ARCH_VWHOLE_AP110_VL4RE32:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, zero, e32, m1, ta, ma\n\t"
            "vl4re32.v v8, (%[base])\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base)
            : "t0", "memory");
        break;
    case AI_ARCH_VWHOLE_AP110_VL8RE64:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, zero, e64, m1, ta, ma\n\t"
            "vl8re64.v v8, (%[base])\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base)
            : "t0", "memory");
        break;
    case AI_ARCH_VWHOLE_AP110_VL4RE8:
    default:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, zero, e8, m1, ta, ma\n\t"
            "vl4re8.v v8, (%[base])\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base)
            : "t0", "memory");
        break;
    }
}

static inline void ai_arch_vwhole_ap110_dump(
    enum ai_arch_vwhole_ap110_kind kind, void *base)
{
    switch (kind) {
    case AI_ARCH_VWHOLE_AP110_VL2RE16:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vs2r.v v8, (%[base])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base)
            : "memory");
        break;
    case AI_ARCH_VWHOLE_AP110_VL4RE8:
    case AI_ARCH_VWHOLE_AP110_VL4RE32:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vs4r.v v8, (%[base])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base)
            : "memory");
        break;
    case AI_ARCH_VWHOLE_AP110_VL8RE64:
    default:
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vs8r.v v8, (%[base])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base)
            : "memory");
        break;
    }
}

static void ai_arch_vwhole_ap110_prepare_pages(enum test_page old_page,
    enum test_page low_page, enum test_page high_page, enum test_page dump_page)
{
    goto_priv(PRIV_HS);
    hspt_init();
    ai_arch_vwhole_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_arch_vwhole_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_arch_vwhole_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_arch_vwhole_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
}

static void ai_arch_vwhole_ap110_cleanup(void)
{
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();
}

static struct ai_arch_vwhole_ap110_result
ai_arch_vwhole_ap110_run_fault_window(enum ai_arch_vwhole_ap110_kind kind,
    uint64_t vlenb, unsigned fault_reg, uint8_t old_seed, uint8_t new_seed)
{
    struct ai_arch_vwhole_ap110_result result = { 0 };
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page dump_page = SCRATCHPAD;
    const unsigned regs = ai_arch_vwhole_ap110_regs(kind);
    const unsigned eew_bytes = ai_arch_vwhole_ap110_eew_bytes(kind);
    const size_t total_bytes = (size_t)regs * (size_t)vlenb;
    const size_t bytes_before_fault = (size_t)fault_reg * (size_t)vlenb;
    const uintptr_t old_vaddr = hs_page_base(old_page);
    const uintptr_t old_paddr = phys_page_base(old_page);
    const uintptr_t low_vaddr = hs_page_base(low_page);
    const uintptr_t low_paddr = phys_page_base(low_page);
    const uintptr_t high_vaddr = hs_page_base(high_page);
    const uintptr_t dump_vaddr = hs_page_base(dump_page);
    const uintptr_t dump_paddr = phys_page_base(dump_page);
    const uintptr_t src_offset = PAGE_SIZE - bytes_before_fault;
    const uintptr_t fault_base_vaddr = low_vaddr + src_offset;

    result.expected_tval = high_vaddr;
    result.expected_vstart = bytes_before_fault / eew_bytes;

    goto_priv(PRIV_M);
    ai_arch_vwhole_ap110_write_pattern(old_paddr, total_bytes, old_seed);
    ai_arch_vwhole_ap110_write_pattern(low_paddr + src_offset,
        bytes_before_fault, new_seed);
    ai_arch_vwhole_ap110_fill(dump_paddr, total_bytes,
        AI_ARCH_VWHOLE_AP110_SENTINEL);

    ai_arch_vwhole_ap110_prepare_pages(old_page, low_page, high_page,
        dump_page);

    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_ap110_load(kind, (const void *)old_vaddr);
    result.preload_ok = excpt.triggered == false;

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_ap110_load(kind, (const void *)fault_base_vaddr);
    result.cause = excpt.cause;
    result.tval = excpt.tval;
    result.vstart = CSRR(CSR_VSTART);
    result.fault_ok = excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == result.expected_tval &&
        result.vstart == result.expected_vstart;

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_ap110_dump(kind, (void *)dump_vaddr);
    result.dump_ok = excpt.triggered == false;

    goto_priv(PRIV_M);
    result.data_ok = ai_arch_vwhole_ap110_check_old_new(dump_paddr,
        (size_t)vlenb, regs, fault_reg, old_seed, new_seed,
        &result.bad_index, &result.bad_actual, &result.bad_expected);

    return result;
}

static struct ai_arch_vwhole_ap110_result
ai_arch_vwhole_ap110_run_full_copy(enum ai_arch_vwhole_ap110_kind kind,
    uint64_t vlenb, uint8_t seed)
{
    struct ai_arch_vwhole_ap110_result result = { 0 };
    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page dump_page = SCRATCHPAD;
    const unsigned regs = ai_arch_vwhole_ap110_regs(kind);
    const size_t total_bytes = (size_t)regs * (size_t)vlenb;
    const uintptr_t src_vaddr = hs_page_base(src_page);
    const uintptr_t src_paddr = phys_page_base(src_page);
    const uintptr_t dump_vaddr = hs_page_base(dump_page);
    const uintptr_t dump_paddr = phys_page_base(dump_page);

    goto_priv(PRIV_M);
    ai_arch_vwhole_ap110_write_pattern(src_paddr, total_bytes, seed);
    ai_arch_vwhole_ap110_fill(dump_paddr, total_bytes + 1U,
        AI_ARCH_VWHOLE_AP110_SENTINEL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_arch_vwhole_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_arch_vwhole_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_ap110_load(kind, (const void *)src_vaddr);
    result.preload_ok = excpt.triggered == false;

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_ap110_dump(kind, (void *)dump_vaddr);
    result.dump_ok = excpt.triggered == false;

    goto_priv(PRIV_M);
    result.data_ok = ai_arch_vwhole_ap110_check_pattern(dump_paddr,
        total_bytes, seed, &result.bad_index, &result.bad_actual,
        &result.bad_expected) &&
        read8(dump_paddr + total_bytes) == AI_ARCH_VWHOLE_AP110_SENTINEL;

    return result;
}

static inline bool ai_arch_vwhole_ap110_fault_window_supported(
    enum ai_arch_vwhole_ap110_kind kind, uint64_t vlenb, unsigned fault_reg)
{
    const unsigned regs = ai_arch_vwhole_ap110_regs(kind);
    const size_t total_bytes = (size_t)regs * (size_t)vlenb;
    const size_t fault_bytes = (size_t)fault_reg * (size_t)vlenb;

    return vlenb > 0ULL &&
        fault_reg > 0U &&
        fault_reg < regs &&
        total_bytes <= PAGE_SIZE &&
        fault_bytes <= PAGE_SIZE;
}

static inline bool ai_arch_vwhole_ap110_full_copy_supported(
    enum ai_arch_vwhole_ap110_kind kind, uint64_t vlenb)
{
    const unsigned regs = ai_arch_vwhole_ap110_regs(kind);
    const size_t total_bytes = (size_t)regs * (size_t)vlenb;

    return vlenb > 0ULL && total_bytes < PAGE_SIZE;
}

static inline bool ai_arch_vwhole_ap110_fault_result_ok(
    const struct ai_arch_vwhole_ap110_result *result)
{
    return result->preload_ok &&
        result->fault_ok &&
        result->dump_ok &&
        result->data_ok;
}

static inline bool ai_arch_vwhole_ap110_copy_result_ok(
    const struct ai_arch_vwhole_ap110_result *result)
{
    return result->preload_ok &&
        result->dump_ok &&
        result->data_ok;
}

bool ai_arch_ap110_vl4re8_fault_second_register_preserves_old_tail_manual_corner()
{
    TEST_START();

    struct ai_arch_vwhole_ap110_result result = { 0 };
    bool supported = false;
    uint64_t vlenb = 0ULL;

    ai_arch_vwhole_enable_dirty();
    vlenb = ai_arch_vwhole_read_vlenb();
    supported = ai_arch_vwhole_ap110_fault_window_supported(
        AI_ARCH_VWHOLE_AP110_VL4RE8, vlenb, 1U);
    TEST_ASSERT("AP-110 vl4re8.v second-register fault setup fits in the single-page old/new/dump windows for this VLEN",
        supported,
        "vlenb=%lu page_size=%lu",
        vlenb, (uint64_t)PAGE_SIZE
    );

    if (supported) {
        result = ai_arch_vwhole_ap110_run_fault_window(
            AI_ARCH_VWHOLE_AP110_VL4RE8, vlenb, 1U, 0x20U, 0x80U);
        TEST_ASSERT("AP-110 vl4re8.v fault at v9 keeps v8 new and preserves old v9..v11 tail after dumping destination whole registers",
            ai_arch_vwhole_ap110_fault_result_ok(&result),
            "preload=%d fault=%d dump=%d data=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%lu bad_index=%lu actual=0x%x expected=0x%x",
            result.preload_ok, result.fault_ok, result.dump_ok,
            result.data_ok, result.cause, result.tval,
            result.expected_tval, result.vstart, result.expected_vstart,
            (uint64_t)result.bad_index, result.bad_actual,
            result.bad_expected
        );
    }

    ai_arch_vwhole_ap110_cleanup();

    TEST_END();
}

bool ai_arch_ap110_vl4re8_fault_third_fourth_register_boundaries_manual_corner()
{
    TEST_START();

    struct ai_arch_vwhole_ap110_result result2 = { 0 };
    struct ai_arch_vwhole_ap110_result result3 = { 0 };
    bool supported2 = false;
    bool supported3 = false;
    uint64_t vlenb = 0ULL;

    ai_arch_vwhole_enable_dirty();
    vlenb = ai_arch_vwhole_read_vlenb();
    supported2 = ai_arch_vwhole_ap110_fault_window_supported(
        AI_ARCH_VWHOLE_AP110_VL4RE8, vlenb, 2U);
    supported3 = ai_arch_vwhole_ap110_fault_window_supported(
        AI_ARCH_VWHOLE_AP110_VL4RE8, vlenb, 3U);
    TEST_ASSERT("AP-110 vl4re8.v third/fourth-register boundary setups fit in the page windows for this VLEN",
        supported2 && supported3,
        "vlenb=%lu page_size=%lu supported2=%d supported3=%d",
        vlenb, (uint64_t)PAGE_SIZE, supported2, supported3
    );

    if (supported2) {
        result2 = ai_arch_vwhole_ap110_run_fault_window(
            AI_ARCH_VWHOLE_AP110_VL4RE8, vlenb, 2U, 0x31U, 0x91U);
        TEST_ASSERT("AP-110 vl4re8.v fault at v10 uses whole-register boundary mapping: v8..v9 new, v10..v11 old",
            ai_arch_vwhole_ap110_fault_result_ok(&result2),
            "preload=%d fault=%d dump=%d data=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%lu bad_index=%lu actual=0x%x expected=0x%x",
            result2.preload_ok, result2.fault_ok, result2.dump_ok,
            result2.data_ok, result2.cause, result2.tval,
            result2.expected_tval, result2.vstart,
            result2.expected_vstart, (uint64_t)result2.bad_index,
            result2.bad_actual, result2.bad_expected
        );
        ai_arch_vwhole_ap110_cleanup();
        ai_arch_vwhole_enable_dirty();
    }

    if (supported3) {
        result3 = ai_arch_vwhole_ap110_run_fault_window(
            AI_ARCH_VWHOLE_AP110_VL4RE8, vlenb, 3U, 0x42U, 0xa2U);
        TEST_ASSERT("AP-110 vl4re8.v fault at v11 uses whole-register boundary mapping: v8..v10 new, v11 old",
            ai_arch_vwhole_ap110_fault_result_ok(&result3),
            "preload=%d fault=%d dump=%d data=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%lu bad_index=%lu actual=0x%x expected=0x%x",
            result3.preload_ok, result3.fault_ok, result3.dump_ok,
            result3.data_ok, result3.cause, result3.tval,
            result3.expected_tval, result3.vstart,
            result3.expected_vstart, (uint64_t)result3.bad_index,
            result3.bad_actual, result3.bad_expected
        );
    }

    ai_arch_vwhole_ap110_cleanup();

    TEST_END();
}

bool ai_arch_ap110_whole_load_width_matrix_tail_preservation_manual_corner()
{
    TEST_START();

    struct ai_arch_vwhole_ap110_result r16 = { 0 };
    struct ai_arch_vwhole_ap110_result r32 = { 0 };
    struct ai_arch_vwhole_ap110_result r64 = { 0 };
    bool supported16 = false;
    bool supported32 = false;
    bool supported64 = false;
    uint64_t vlenb = 0ULL;

    ai_arch_vwhole_enable_dirty();
    vlenb = ai_arch_vwhole_read_vlenb();
    supported16 = ai_arch_vwhole_ap110_fault_window_supported(
        AI_ARCH_VWHOLE_AP110_VL2RE16, vlenb, 1U);
    supported32 = ai_arch_vwhole_ap110_fault_window_supported(
        AI_ARCH_VWHOLE_AP110_VL4RE32, vlenb, 2U);
    supported64 = ai_arch_vwhole_ap110_fault_window_supported(
        AI_ARCH_VWHOLE_AP110_VL8RE64, vlenb, 4U);
    TEST_ASSERT("AP-110 whole-register EEW=16/32/64 matrix fits in the page windows for this VLEN",
        supported16 && supported32 && supported64,
        "vlenb=%lu page_size=%lu supported16=%d supported32=%d supported64=%d",
        vlenb, (uint64_t)PAGE_SIZE, supported16, supported32,
        supported64
    );

    if (supported16) {
        r16 = ai_arch_vwhole_ap110_run_fault_window(
            AI_ARCH_VWHOLE_AP110_VL2RE16, vlenb, 1U, 0x24U, 0x84U);
        TEST_ASSERT("AP-110 vl2re16.v fault at the second whole register preserves the old tail using EEW-scaled vstart",
            ai_arch_vwhole_ap110_fault_result_ok(&r16),
            "preload=%d fault=%d dump=%d data=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%lu bad_index=%lu actual=0x%x expected=0x%x",
            r16.preload_ok, r16.fault_ok, r16.dump_ok, r16.data_ok,
            r16.cause, r16.tval, r16.expected_tval, r16.vstart,
            r16.expected_vstart, (uint64_t)r16.bad_index,
            r16.bad_actual, r16.bad_expected
        );
        ai_arch_vwhole_ap110_cleanup();
        ai_arch_vwhole_enable_dirty();
    }

    if (supported32) {
        r32 = ai_arch_vwhole_ap110_run_fault_window(
            AI_ARCH_VWHOLE_AP110_VL4RE32, vlenb, 2U, 0x35U, 0x95U);
        TEST_ASSERT("AP-110 vl4re32.v fault at a later whole register preserves old destination registers after the exception",
            ai_arch_vwhole_ap110_fault_result_ok(&r32),
            "preload=%d fault=%d dump=%d data=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%lu bad_index=%lu actual=0x%x expected=0x%x",
            r32.preload_ok, r32.fault_ok, r32.dump_ok, r32.data_ok,
            r32.cause, r32.tval, r32.expected_tval, r32.vstart,
            r32.expected_vstart, (uint64_t)r32.bad_index,
            r32.bad_actual, r32.bad_expected
        );
        ai_arch_vwhole_ap110_cleanup();
        ai_arch_vwhole_enable_dirty();
    }

    if (supported64) {
        r64 = ai_arch_vwhole_ap110_run_fault_window(
            AI_ARCH_VWHOLE_AP110_VL8RE64, vlenb, 4U, 0x46U, 0xa6U);
        TEST_ASSERT("AP-110 vl8re64.v fault in a later whole-register group keeps the merge boundary in whole-register order",
            ai_arch_vwhole_ap110_fault_result_ok(&r64),
            "preload=%d fault=%d dump=%d data=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu expected_vstart=%lu bad_index=%lu actual=0x%x expected=0x%x",
            r64.preload_ok, r64.fault_ok, r64.dump_ok, r64.data_ok,
            r64.cause, r64.tval, r64.expected_tval, r64.vstart,
            r64.expected_vstart, (uint64_t)r64.bad_index,
            r64.bad_actual, r64.bad_expected
        );
    }

    ai_arch_vwhole_ap110_cleanup();

    TEST_END();
}

bool ai_arch_ap110_whole_register_nf_handle_count_full_copy_manual_corner()
{
    TEST_START();

    struct ai_arch_vwhole_ap110_result r2 = { 0 };
    struct ai_arch_vwhole_ap110_result r4 = { 0 };
    struct ai_arch_vwhole_ap110_result r8 = { 0 };
    bool supported2 = false;
    bool supported4 = false;
    bool supported8 = false;
    uint64_t vlenb = 0ULL;

    ai_arch_vwhole_enable_dirty();
    vlenb = ai_arch_vwhole_read_vlenb();
    supported2 = ai_arch_vwhole_ap110_full_copy_supported(
        AI_ARCH_VWHOLE_AP110_VL2RE16, vlenb);
    supported4 = ai_arch_vwhole_ap110_full_copy_supported(
        AI_ARCH_VWHOLE_AP110_VL4RE8, vlenb);
    supported8 = ai_arch_vwhole_ap110_full_copy_supported(
        AI_ARCH_VWHOLE_AP110_VL8RE64, vlenb);
    TEST_ASSERT("AP-110 whole-register full-copy nf+1 register-count probes fit in the scratch page with a guard byte",
        supported2 && supported4 && supported8,
        "vlenb=%lu page_size=%lu supported2=%d supported4=%d supported8=%d",
        vlenb, (uint64_t)PAGE_SIZE, supported2, supported4, supported8
    );

    if (supported2) {
        r2 = ai_arch_vwhole_ap110_run_full_copy(
            AI_ARCH_VWHOLE_AP110_VL2RE16, vlenb, 0x52U);
        TEST_ASSERT("AP-110 vl2re16.v still copies and dumps both whole destination registers after the range-mapping fix",
            ai_arch_vwhole_ap110_copy_result_ok(&r2),
            "preload=%d dump=%d data=%d bad_index=%lu actual=0x%x expected=0x%x",
            r2.preload_ok, r2.dump_ok, r2.data_ok,
            (uint64_t)r2.bad_index, r2.bad_actual, r2.bad_expected
        );
        ai_arch_vwhole_ap110_cleanup();
        ai_arch_vwhole_enable_dirty();
    }

    if (supported4) {
        r4 = ai_arch_vwhole_ap110_run_full_copy(
            AI_ARCH_VWHOLE_AP110_VL4RE8, vlenb, 0x64U);
        TEST_ASSERT("AP-110 vl4re8.v still copies and dumps all four whole destination registers after the range-mapping fix",
            ai_arch_vwhole_ap110_copy_result_ok(&r4),
            "preload=%d dump=%d data=%d bad_index=%lu actual=0x%x expected=0x%x",
            r4.preload_ok, r4.dump_ok, r4.data_ok,
            (uint64_t)r4.bad_index, r4.bad_actual, r4.bad_expected
        );
        ai_arch_vwhole_ap110_cleanup();
        ai_arch_vwhole_enable_dirty();
    }

    if (supported8) {
        r8 = ai_arch_vwhole_ap110_run_full_copy(
            AI_ARCH_VWHOLE_AP110_VL8RE64, vlenb, 0x76U);
        TEST_ASSERT("AP-110 vl8re64.v still copies and dumps all eight whole destination registers after the range-mapping fix",
            ai_arch_vwhole_ap110_copy_result_ok(&r8),
            "preload=%d dump=%d data=%d bad_index=%lu actual=0x%x expected=0x%x",
            r8.preload_ok, r8.dump_ok, r8.data_ok,
            (uint64_t)r8.bad_index, r8.bad_actual, r8.bad_expected
        );
    }

    ai_arch_vwhole_ap110_cleanup();

    TEST_END();
}

bool ai_arch_vl1re8_element0_page_fault_dirty_vs_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI);
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    ai_arch_vwhole_enable_dirty();
    ai_arch_vwhole_config_e8();
    ai_arch_vwhole_prepare_hs();

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_arch_vwhole_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_vl1re8_fault((const uint8_t *)bad_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_arch_vwhole_sstatus_vs();
    TEST_ASSERT("vl1re8.v element-0 load page fault reports the base address, leaves vstart at zero, and dirties the mstatus/sstatus VS alias before the first whole-register memory access",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr &&
        observed_vstart == 0ULL &&
        observed_vs == AI_ARCH_VWHOLE_VS_DIRTY,
        "cause=0x%lx tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, observed_vstart, observed_vs
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_vl1re8_element0_access_fault_dirty_vs_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSRWX_GRWX);
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    ai_arch_vwhole_enable_dirty();
    ai_arch_vwhole_config_e8();
    ai_arch_vwhole_prepare_hs();

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_arch_vwhole_force_vs_clean();
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_vl1re8_fault((const uint8_t *)bad_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_arch_vwhole_sstatus_vs();
    TEST_ASSERT("vl1re8.v element-0 load access fault from a high-PPN PTW entry still dirties the mstatus/sstatus VS alias before the first whole-register memory access",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr &&
        observed_vstart == 0ULL &&
        observed_vs == AI_ARCH_VWHOLE_VS_DIRTY,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, bad_vaddr, observed_vstart, observed_vs
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_vl8re64_element0_page_fault_dirty_vs_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI);
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    ai_arch_vwhole_enable_dirty();
    ai_arch_vwhole_config_e64();
    ai_arch_vwhole_prepare_hs();

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_arch_vwhole_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_vl8re64_fault((const uint64_t *)bad_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_arch_vwhole_sstatus_vs();
    TEST_ASSERT("vl8re64.v element-0 load page fault uses the same VS dirty-before-memory path for wide multi-register whole loads",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr &&
        observed_vstart == 0ULL &&
        observed_vs == AI_ARCH_VWHOLE_VS_DIRTY,
        "cause=0x%lx tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, observed_vstart, observed_vs
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_vs1r_element0_page_fault_dirty_vs_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI);
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    ai_arch_vwhole_enable_dirty();
    ai_arch_vwhole_seed_src();
    ai_arch_vwhole_seed_v6_1r(ai_arch_vwhole_src);
    ai_arch_vwhole_prepare_hs();

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_arch_vwhole_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_vs1r_fault((uint8_t *)bad_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_arch_vwhole_sstatus_vs();
    TEST_ASSERT("vs1r.v element-0 store page fault reports the base address and dirties the mstatus/sstatus VS alias even though store-whole has no destination vector register writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr &&
        observed_vstart == 0ULL &&
        observed_vs == AI_ARCH_VWHOLE_VS_DIRTY,
        "cause=0x%lx tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, observed_vstart, observed_vs
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_vs8r_element0_page_fault_dirty_vs_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI);
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    ai_arch_vwhole_enable_dirty();
    ai_arch_vwhole_seed_src();
    ai_arch_vwhole_seed_v8_8r(ai_arch_vwhole_src);
    ai_arch_vwhole_prepare_hs();

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    ai_arch_vwhole_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_vs8r_fault((uint8_t *)bad_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_arch_vwhole_sstatus_vs();
    TEST_ASSERT("vs8r.v element-0 store page fault uses the same dirty-before-memory path for the multi-register store-whole family",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr &&
        observed_vstart == 0ULL &&
        observed_vs == AI_ARCH_VWHOLE_VS_DIRTY,
        "cause=0x%lx tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, observed_vstart, observed_vs
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_vl1re8_vs_off_illegal_keeps_vs_off_corner()
{
    TEST_START();

    const uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    const uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX) + 0x180U;
    const uint8_t before = 0x4bU;
    uint64_t observed_vs = AI_ARCH_VWHOLE_VS_DIRTY;

    goto_priv(PRIV_M);
    write8(valid_paddr, before);
    ai_arch_vwhole_enable_dirty();
    ai_arch_vwhole_config_e8();
    ai_arch_vwhole_prepare_hs();

    goto_priv(PRIV_M);
    ai_arch_vwhole_force_vs_off();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_vl1re8_fault((const uint8_t *)valid_vaddr);
    observed_vs = ai_arch_vwhole_sstatus_vs();
    TEST_ASSERT("with mstatus.VS=Off, vl1re8.v raises illegal instruction before memory translation and AP-101 dirty-before-memory must not run",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        observed_vs == 0ULL,
        "cause=0x%lx tval=0x%lx vs=0x%lx",
        excpt.cause, excpt.tval, observed_vs
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the VS-off illegal whole-register probe leaves the valid backing byte unchanged, showing that the memory path was not reached",
        read8(valid_paddr) == before,
        "paddr=0x%lx actual=0x%x expected=0x%x",
        valid_paddr, read8(valid_paddr), before
    );

    ai_arch_vwhole_enable_dirty();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_vl1re8_nonzero_vstart_page_fault_dirty_vs_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI);
    uint64_t observed_vstart = 0ULL;
    uint64_t observed_vs = 0ULL;

    ai_arch_vwhole_enable_dirty();
    ai_arch_vwhole_config_e8();
    ai_arch_vwhole_prepare_hs();

    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 1ULL);
    ai_arch_vwhole_force_vs_clean();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_vwhole_vl1re8_fault((const uint8_t *)bad_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    observed_vs = ai_arch_vwhole_sstatus_vs();
    TEST_ASSERT("vl1re8.v with non-zero vstart faults on the first resumed whole-register memory access and still dirties the mstatus/sstatus VS alias before issuing that resumed access",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr + 1ULL &&
        observed_vstart == 1ULL &&
        observed_vs == AI_ARCH_VWHOLE_VS_DIRTY,
        "cause=0x%lx tval=0x%lx vstart=%lu vs=0x%lx",
        excpt.cause, excpt.tval, observed_vstart, observed_vs
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}
