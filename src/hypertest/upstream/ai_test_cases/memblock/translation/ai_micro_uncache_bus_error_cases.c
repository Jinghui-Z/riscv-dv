#include <rvh_test.h>
#include <page_tables.h>
#include <stddef.h>
#include <stdint.h>

#define AI_MICRO_UNCACHE_MMIO_MEM_BASE         0x30000000000ULL
#define AI_MICRO_UNCACHE_MMIO_MEM_LIMIT        0x40000000000ULL
#define AI_MICRO_UNCACHE_RESP_MMIO64_BASE      0x40070000ULL
#define AI_MICRO_UNCACHE_RESP_MMIO64_LIMIT     0x40080000ULL
#define AI_MICRO_UNCACHE_BAD_PADDR \
    (phys_page_base_offset(VSRWXPbmt2_GURWXPbmt3, \
        AI_MICRO_UNCACHE_MMIO_MEM_BASE) + 0x7e0ULL)
#define AI_MICRO_UNCACHE_BAD_PADDR_ALT         (AI_MICRO_UNCACHE_BAD_PADDR - 0x10ULL)
#define AI_MICRO_UNCACHE_PBMT_NC_GOOD_PAGE     VSRWXPbmt1_GURWXPbmt0
#define AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE       VSRWX_GRWX
#define AI_MICRO_UNCACHE_MMIO_GOOD_PAGE        SCRATCHPAD
#define AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE    VSRWXPbmt1_GURWXPbmt1
#define AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE    VSRWXPbmt2_GURWXPbmt1
#define AI_MICRO_UNCACHE_ORDER_AF_PAGE         VSI_GI
#define AI_MICRO_UNCACHE_ORDER_MARKER_PAGE     SWITCH1

#if PT_LVLS == 4
#define AI_MICRO_UNCACHE_ORDER_LEAF_LEVEL      4
#else
#define AI_MICRO_UNCACHE_ORDER_LEAF_LEVEL      2
#endif

struct ai_micro_uncache_exception_snapshot {
    bool triggered;
    uint64_t cause;
    uint64_t tval;
};

static inline uint64_t ai_micro_uncache_extract_unsigned(uint64_t value,
    unsigned offset_bytes, unsigned size_bytes)
{
    const uint64_t mask =
        (size_bytes == sizeof(uint8_t)) ? 0xffULL :
        (size_bytes == sizeof(uint16_t)) ? 0xffffULL :
        0xffffffffULL;

    return (value >> (offset_bytes * 8U)) & mask;
}

static inline uint64_t ai_micro_uncache_sign_extend(uint64_t value,
    unsigned size_bytes)
{
    if (size_bytes == sizeof(uint8_t)) {
        return (uint64_t)(int64_t)(int8_t)value;
    }
    if (size_bytes == sizeof(uint16_t)) {
        return (uint64_t)(int64_t)(int16_t)value;
    }
    return (uint64_t)(int64_t)(int32_t)value;
}

static inline uint64_t ai_micro_uncache_extract_scalar(uint64_t value,
    unsigned offset_bytes, unsigned size_bytes, bool signed_load)
{
    uint64_t extracted =
        ai_micro_uncache_extract_unsigned(value, offset_bytes, size_bytes);

    if (signed_load) {
        return ai_micro_uncache_sign_extend(extracted, size_bytes);
    }
    return extracted;
}

static inline uint64_t ai_micro_uncache_merge_unsigned(uint64_t base,
    uint64_t value, unsigned offset_bytes, unsigned size_bytes)
{
    const uint64_t mask =
        (size_bytes == sizeof(uint8_t)) ? 0xffULL :
        (size_bytes == sizeof(uint16_t)) ? 0xffffULL :
        0xffffffffULL;
    const unsigned shift = offset_bytes * 8U;
    const uint64_t shifted_mask = mask << shift;

    return (base & ~shifted_mask) | ((value & mask) << shift);
}

static inline uint64_t ai_micro_uncache_narrow_load(unsigned size_bytes,
    uintptr_t vaddr)
{
    if (size_bytes == sizeof(uint8_t)) {
        return (uint64_t)lbu(vaddr);
    }
    if (size_bytes == sizeof(uint16_t)) {
        return (uint64_t)lhu(vaddr);
    }
    return (uint64_t)lwu(vaddr);
}

static inline uint64_t ai_micro_uncache_scalar_load(bool signed_load,
    unsigned size_bytes, uintptr_t vaddr)
{
    if (size_bytes == sizeof(uint8_t)) {
        uint64_t value = (uint64_t)lbu(vaddr);
        return signed_load ? ai_micro_uncache_sign_extend(value, size_bytes) : value;
    }
    if (size_bytes == sizeof(uint16_t)) {
        uint64_t value = (uint64_t)lhu(vaddr);
        return signed_load ? ai_micro_uncache_sign_extend(value, size_bytes) : value;
    }
    {
        uint64_t value = (uint64_t)lwu(vaddr);
        return signed_load ? ai_micro_uncache_sign_extend(value, size_bytes) : value;
    }
}

static inline void ai_micro_uncache_narrow_store(unsigned size_bytes,
    uintptr_t vaddr, uint64_t value)
{
    if (size_bytes == sizeof(uint8_t)) {
        sb(vaddr, (uint8_t)value);
        return;
    }
    if (size_bytes == sizeof(uint16_t)) {
        sh(vaddr, (uint16_t)value);
        return;
    }
    sw(vaddr, (uint32_t)value);
}

static inline uintptr_t ai_micro_uncache_alias_anchor(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_micro_uncache_alias_vaddr(enum test_page page,
    uintptr_t aliased_paddr)
{
    return hs_page_base(page) + (aliased_paddr & (PAGE_SIZE - 1));
}

static inline bool ai_micro_uncache_is_pbmt_io_alias(enum test_page page)
{
    return page == AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE;
}

static inline bool ai_micro_uncache_is_mmio_mem_paddr(uintptr_t paddr)
{
    return paddr >= AI_MICRO_UNCACHE_MMIO_MEM_BASE &&
        paddr < AI_MICRO_UNCACHE_MMIO_MEM_LIMIT;
}

static inline uintptr_t ai_micro_uncache_good_mmio64_paddr(uintptr_t offset)
{
    /*
     * 0x4007_0000 is LinkNan SimMMIO IntrGen.  It is a real MMIO responder
     * for 8-byte scalar accesses, so PBMT=IO 64-bit good/recovery probes can
     * stay device-typed without falling back to normal DRAM.
     */
    return AI_MICRO_UNCACHE_RESP_MMIO64_BASE +
        (offset & (AI_MICRO_UNCACHE_RESP_MMIO64_LIMIT -
            AI_MICRO_UNCACHE_RESP_MMIO64_BASE - sizeof(uint64_t)));
}

static inline void ai_micro_uncache_prepare_pbmt_io_mem_window(void)
{
    goto_priv(PRIV_M);
    /*
     * PBMT=IO recovery/good accesses must stay on a responsive device-style
     * physical window.  The 3T LinkNan window is backed by extra uncache memory;
     * ordinary DRAM backing would route a device request to a non-existent MMIO
     * responder.
     */
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFFULL);
    CSRW(CSR_PMACFG0, 0x6FULL);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000ULL);
    CSRW(CSR_PMAADDR12, 0x20000000ULL);
    CSRW(CSR_PMAADDR13, 0x120000000ULL);
    CSRW(CSR_PMAADDR14, 0xC000000000ULL);
    CSRW(CSR_PMAADDR15, 0x10000000000ULL);

    CSRW(CSR_PMACFG0, 0x0ULL);
    CSRW(CSR_PMAADDR0, AI_MICRO_UNCACHE_MMIO_MEM_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_MICRO_UNCACHE_MMIO_MEM_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    fence_iorw_iorw();
}

static inline uintptr_t ai_micro_uncache_good_paddr_for_pbmt_alias(
    enum test_page page, uintptr_t offset)
{
    if (ai_micro_uncache_is_pbmt_io_alias(page)) {
        return phys_page_base_offset(AI_MICRO_UNCACHE_MMIO_GOOD_PAGE,
            AI_MICRO_UNCACHE_MMIO_MEM_BASE) + offset;
    }

    return phys_page_base(SWITCH2) + offset;
}

static inline void ai_micro_uncache_prepare_direct_pbmt_io_paddr(uintptr_t paddr)
{
    if (ai_micro_uncache_is_mmio_mem_paddr(paddr)) {
        ai_micro_uncache_prepare_pbmt_io_mem_window();
    }
}

static inline void ai_micro_uncache_write_pbmt_good_paddr(uintptr_t paddr,
    uint64_t value)
{
    ai_micro_uncache_prepare_direct_pbmt_io_paddr(paddr);
    write64(paddr, value);
    fence_iorw_iorw();
}

static inline uint64_t ai_micro_uncache_read_pbmt_good_paddr(uintptr_t paddr)
{
    ai_micro_uncache_prepare_direct_pbmt_io_paddr(paddr);
    return read64(paddr);
}

static inline void ai_micro_uncache_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0x0fULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_micro_uncache_configure_pmp_bad_page(uintptr_t paddr)
{
    const uintptr_t page_start = paddr & PAGE_ADDR_MSK;

    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, page_start >> 2);
    CSRW(CSR_PMPADDR1, (page_start + PAGE_SIZE) >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, 0x0fULL | (0x08ULL << 8) | (0x0fULL << 16));
}

static inline void ai_micro_uncache_reset_pbmt_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    ai_micro_uncache_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_uncache_reset_mmio_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
    ai_micro_uncache_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_uncache_prepare_pbmt_bad_alias(enum test_page page,
    uintptr_t bad_paddr)
{
    uintptr_t bad_page_start = bad_paddr & PAGE_ADDR_MSK;

    goto_priv(PRIV_M);
    if (ai_micro_uncache_is_mmio_mem_paddr(bad_paddr)) {
        ai_micro_uncache_prepare_pbmt_io_mem_window();
    }
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_uncache_configure_pmp_bad_page(bad_paddr);
    pbmt_hspt_to_x(page);
    hspt_leaf_change_base_paddr(page,
        ai_micro_uncache_alias_anchor(page, bad_page_start));
    sfence_vma();

    return hs_page_base(page) + (bad_paddr & (PAGE_SIZE - 1));
}

static inline uintptr_t ai_micro_uncache_prepare_pbmt_alias_to_paddr(
    enum test_page page, uintptr_t paddr)
{
    uintptr_t page_start = paddr & PAGE_ADDR_MSK;

    goto_priv(PRIV_M);
    if (ai_micro_uncache_is_mmio_mem_paddr(paddr)) {
        ai_micro_uncache_prepare_pbmt_io_mem_window();
    }
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    hspt_leaf_change_base_paddr(page,
        ai_micro_uncache_alias_anchor(page, page_start));
    sfence_vma();

    return hs_page_base(page) + (paddr & (PAGE_SIZE - 1));
}

static inline uintptr_t ai_micro_uncache_prepare_mmio_bad_alias(enum test_page alias_page,
    uintptr_t bad_paddr)
{
    uintptr_t bad_page_start = bad_paddr & PAGE_ADDR_MSK;

    ai_micro_uncache_reset_mmio_env();

    CSRW(CSR_PMAADDR0, bad_page_start >> 2);
    CSRW(CSR_PMAADDR1, (bad_page_start + PAGE_SIZE) >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);

    hspt_init();
    hspt_leaf_change_base_paddr(alias_page,
        ai_micro_uncache_alias_anchor(alias_page, bad_page_start));
    sfence_vma();

    return hs_page_base(alias_page) + (bad_paddr & (PAGE_SIZE - 1));
}

static inline uintptr_t ai_micro_uncache_prepare_mmio_alias_to_paddr(
    enum test_page alias_page, uintptr_t paddr)
{
    uintptr_t page_start = paddr & PAGE_ADDR_MSK;

    ai_micro_uncache_reset_mmio_env();

    CSRW(CSR_PMAADDR0, page_start >> 2);
    CSRW(CSR_PMAADDR1, (page_start + PAGE_SIZE) >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);

    hspt_init();
    hspt_leaf_change_base_paddr(alias_page,
        ai_micro_uncache_alias_anchor(alias_page, page_start));
    sfence_vma();

    return hs_page_base(alias_page) + (paddr & (PAGE_SIZE - 1));
}

static inline void ai_micro_uncache_make_leaf_access_fault(enum test_page page)
{
    goto_priv(PRIV_M);
    hspt[AI_MICRO_UNCACHE_ORDER_LEAF_LEVEL][page] |= (1ULL << 46);
    sfence_vma();
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_uncache_load_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_vaddr,
    struct ai_micro_uncache_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "la t1, excpt\n"
        "lbu t2, %c[exc_triggered](t1)\n"
        "sb t2, %c[snap_triggered](a3)\n"
        "ld t2, %c[exc_cause](t1)\n"
        "sd t2, %c[snap_cause](a3)\n"
        "ld t2, %c[exc_tval](t1)\n"
        "sd t2, %c[snap_tval](a3)\n"
        "sb zero, %c[exc_triggered](t1)\n"
        "li t2, 1\n"
        "sb t2, %c[exc_testing](t1)\n"
        "ld t0, 0(a1)\n"
        "li t2, 1\n"
        "sd t2, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_uncache_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_uncache_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_uncache_exception_snapshot, tval))
        : "t0", "t1", "t2", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_uncache_store_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_vaddr,
    uint64_t first_value, uint64_t second_value,
    struct ai_micro_uncache_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a3, 0(a0)\n"
        "la t0, excpt\n"
        "lbu t1, %c[exc_triggered](t0)\n"
        "sb t1, %c[snap_triggered](a5)\n"
        "ld t1, %c[exc_cause](t0)\n"
        "sd t1, %c[snap_cause](a5)\n"
        "ld t1, %c[exc_tval](t0)\n"
        "sd t1, %c[snap_tval](a5)\n"
        "sb zero, %c[exc_triggered](t0)\n"
        "li t1, 1\n"
        "sb t1, %c[exc_testing](t0)\n"
        "sd a4, 0(a1)\n"
        "li t1, 1\n"
        "sd t1, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_uncache_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_uncache_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_uncache_exception_snapshot, tval))
        : "t0", "t1", "memory");
}

bool ai_micro_hs_load_uncache_bus_error_access_fault()
{
    TEST_START();

    uintptr_t nc_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(nc_vaddr);
    TEST_ASSERT("an HS load on a PBMT=NC alias reports load access fault with the replayed virtual address when the uncache response carries a bus error",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == nc_vaddr
    );

    uintptr_t io_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(io_vaddr);
    TEST_ASSERT("an HS load on a PBMT=IO alias reports load access fault with the replayed virtual address when the uncache response carries a bus error",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == io_vaddr
    );

    uintptr_t mmio_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(mmio_vaddr);
    TEST_ASSERT("an HS load on a PMA=Device alias reports load access fault with the replayed virtual address when the uncache response carries a bus error",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == mmio_vaddr
    );

    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_bus_error_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("an HS store on a PMA=Device alias reports store access fault with the replayed virtual address when the uncache response carries a bus error",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_uncache_bus_error_access_fault()
{
    TEST_START();

    uintptr_t nc_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS store on a PBMT=NC alias reports store access fault with the replayed virtual address when the uncache response carries a bus error",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == nc_vaddr
    );

    uintptr_t io_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(io_vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("an HS store on a PBMT=IO alias reports store access fault with the replayed virtual address when the uncache response carries a bus error",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == io_vaddr
    );

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_nc_repeated_bus_error_then_normal_scalar_recovers_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base(SWITCH1) + 0x260U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH1) + 0x260U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x1122334455667788ULL;
    const uint64_t updated_good = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent_good = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first HS load on a PBMT=NC bad alias reports a precise load access fault with the original replayed virtual address before the later recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_good = ld(good_vaddr);
    TEST_ASSERT("after the first PBMT=NC load bus error, an unrelated normal scalar load still returns the seeded payload without stale exception state",
        excpt.triggered == false &&
        first_good == initial_good
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("repeating the same HS load on the PBMT=NC bad alias still reports a precise load access fault with the same replayed virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated_good);
    TEST_ASSERT("after the repeated PBMT=NC load bus errors, an unrelated normal scalar store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_good = ld(good_vaddr);
    TEST_ASSERT("after the repeated PBMT=NC load bus errors and the later normal scalar store, the updated payload is immediately readable",
        excpt.triggered == false &&
        observed_good == updated_good
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=NC load bus errors and the later normal scalar recovery path leave the adjacent doubleword unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PBMT=NC load bus errors do not poison subsequent scalar accesses, so only the intended normal-memory doubleword is updated",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_io_repeated_bus_error_then_normal_scalar_recovers_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base(SWITCH2) + 0x2a0U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH2) + 0x2a0U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x2233445566778899ULL;
    const uint64_t updated_good = 0x76543210fedcba98ULL;
    const uint64_t adjacent_good = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first HS load on a PBMT=IO bad alias reports a precise load access fault with the original replayed virtual address before the later recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_good = ld(good_vaddr);
    TEST_ASSERT("after the first PBMT=IO load bus error, an unrelated normal scalar load still returns the seeded payload without stale exception state",
        excpt.triggered == false &&
        first_good == initial_good
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("repeating the same HS load on the PBMT=IO bad alias still reports a precise load access fault with the same replayed virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated_good);
    TEST_ASSERT("after the repeated PBMT=IO load bus errors, an unrelated normal scalar store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_good = ld(good_vaddr);
    TEST_ASSERT("after the repeated PBMT=IO load bus errors and the later normal scalar store, the updated payload is immediately readable",
        excpt.triggered == false &&
        observed_good == updated_good
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=IO load bus errors and the later normal scalar recovery path leave the adjacent doubleword unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PBMT=IO load bus errors do not poison subsequent scalar accesses, so only the intended normal-memory doubleword is updated",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_nc_repeated_bus_error_then_normal_scalar_recovers_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base(SWITCH1) + 0x2e0U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH1) + 0x2e0U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x33445566778899aaULL;
    const uint64_t updated_good = 0x89abcdef01234567ULL;
    const uint64_t adjacent_good = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("the first HS store on a PBMT=NC bad alias reports a precise store access fault with the original replayed virtual address before the later recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_good = ld(good_vaddr);
    TEST_ASSERT("after the first PBMT=NC store bus error, an unrelated normal scalar load still returns the seeded payload without stale exception state",
        excpt.triggered == false &&
        first_good == initial_good
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xf0e1d2c3b4a59687ULL);
    TEST_ASSERT("repeating the same HS store on the PBMT=NC bad alias still reports a precise store access fault with the same replayed virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated_good);
    TEST_ASSERT("after the repeated PBMT=NC store bus errors, an unrelated normal scalar store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_good = ld(good_vaddr);
    TEST_ASSERT("after the repeated PBMT=NC store bus errors and the later normal scalar store, the updated payload is immediately readable",
        excpt.triggered == false &&
        observed_good == updated_good
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=NC store bus errors and the later normal scalar recovery path leave the adjacent doubleword unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PBMT=NC store bus errors do not poison subsequent scalar accesses, so only the intended normal-memory doubleword is updated",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_io_repeated_bus_error_then_normal_scalar_recovers_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base(SWITCH2) + 0x320U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH2) + 0x320U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x445566778899aabbULL;
    const uint64_t updated_good = 0x0123fedc4567ba98ULL;
    const uint64_t adjacent_good = 0x02468ace13579bdfULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("the first HS store on a PBMT=IO bad alias reports a precise store access fault with the original replayed virtual address before the later recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_good = ld(good_vaddr);
    TEST_ASSERT("after the first PBMT=IO store bus error, an unrelated normal scalar load still returns the seeded payload without stale exception state",
        excpt.triggered == false &&
        first_good == initial_good
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x99aabbccddeeff00ULL);
    TEST_ASSERT("repeating the same HS store on the PBMT=IO bad alias still reports a precise store access fault with the same replayed virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated_good);
    TEST_ASSERT("after the repeated PBMT=IO store bus errors, an unrelated normal scalar store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_good = ld(good_vaddr);
    TEST_ASSERT("after the repeated PBMT=IO store bus errors and the later normal scalar store, the updated payload is immediately readable",
        excpt.triggered == false &&
        observed_good == updated_good
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=IO store bus errors and the later normal scalar recovery path leave the adjacent doubleword unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PBMT=IO store bus errors do not poison subsequent scalar accesses, so only the intended normal-memory doubleword is updated",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_nc_bus_error_reports_older_va_under_younger_store_pressure_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base(SWITCH1) + 0x360U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH1) + 0x360U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x5566778899aabbccULL;
    const uint64_t updated_good = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t adjacent_good = 0xfaceb00c12345678ULL;
    uint64_t observed_good = 0ULL;
    uint64_t observed_adjacent = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x8877665544332211ULL);
    sd(good_vaddr, updated_good);
    fence_iorw_iorw();
    observed_good = ld(good_vaddr);
    TEST_ASSERT("when an older HS store on a PBMT=NC bad alias later reports a bus-error access fault, its precise tval must stay bound to that older bad alias even if a younger normal store has already entered the same execution window and later recovers",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr &&
        observed_good == updated_good
    );

    TEST_SETUP_EXCEPT();
    observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("after the older PBMT=NC store bus error and the younger normal-store recovery path, the adjacent normal-memory doubleword remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older PBMT=NC store bus error does not let a younger normal store corrupt neighboring memory while recovering after trap return",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_nc_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner()
{
    TEST_START();

    const uintptr_t older_bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t younger_bad_vaddr = ai_micro_uncache_alias_vaddr(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR_ALT);
    const uintptr_t good_paddr = phys_page_base(SWITCH2) + 0x3a0U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH2) + 0x3a0U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x66778899aabbccddULL;
    const uint64_t updated_good = 0x3141592653589793ULL;
    const uint64_t adjacent_good = 0x1020304050607080ULL;
    uint64_t observed_good = 0ULL;
    uint64_t observed_adjacent = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(older_bad_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("the first HS store bus error on the PBMT=NC bad alias reports the first bad virtual address instead of any later alias offset",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == older_bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(younger_bad_vaddr, 0xf0e1d2c3b4a59687ULL);
    TEST_ASSERT("after the first PBMT=NC store bus-error trap returns, reusing the same aliased bad page at a different offset refreshes precise tval to that next bad virtual address instead of keeping the older one stale",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == younger_bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated_good);
    fence_iorw_iorw();
    TEST_ASSERT("after two distinct PBMT=NC store bus-error offsets, an unrelated normal scalar store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_good = ld(good_vaddr);
    TEST_ASSERT("after two distinct PBMT=NC store bus-error offsets, the later normal-memory recovery load sees only its intended payload",
        excpt.triggered == false &&
        observed_good == updated_good
    );

    TEST_SETUP_EXCEPT();
    observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("after two distinct PBMT=NC store bus-error offsets, the adjacent normal-memory doubleword remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("distinct PBMT=NC store bus-error offsets do not poison later normal-memory recovery, so only the intended doubleword is updated",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_nc_bus_error_then_good_pbmt_nc_store_clears_stale_exception_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_UNCACHE_PBMT_NC_GOOD_PAGE) + 0x3c0U;
    const uintptr_t good_nc_vaddr = hs_page_base(AI_MICRO_UNCACHE_PBMT_NC_GOOD_PAGE) + 0x3c0U;
    const uint64_t initial_good = 0x778899aabbccddeeULL;
    const uint64_t updated_good = 0xdeadbeef10293847ULL;
    uint64_t observed_good = 0ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x55aa55aa55aa55aaULL);
    TEST_ASSERT("the first HS store on the PBMT=NC bad alias reports the expected precise store access fault before the stale-exception clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_uncache_restore_pmp_full_access();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(AI_MICRO_UNCACHE_PBMT_NC_GOOD_PAGE);
    sfence_vma();
    write64(good_paddr, initial_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_nc_vaddr, updated_good);
    fence_iorw_iorw();
    TEST_ASSERT("after a PBMT=NC store bus error, the next valid PBMT=NC store episode on another good NC page must not inherit stale uncache exception state",
        excpt.triggered == false,
        "triggered=%d cause=0x%llx tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval
    );

    TEST_SETUP_EXCEPT();
    observed_good = ld(good_nc_vaddr);
    TEST_ASSERT("after the bad-then-good PBMT=NC store sequence, the same NC alias immediately reads back the new payload instead of a stale fault-era state",
        excpt.triggered == false &&
        observed_good == updated_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed_good, updated_good
    );

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_nc_bus_error_then_repeated_good_pbmt_nc_store_no_false_fault_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_nc_vaddr = hs_page_base(AI_MICRO_UNCACHE_PBMT_NC_GOOD_PAGE) + 0x3e0U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_UNCACHE_PBMT_NC_GOOD_PAGE) + 0x3e0U;
    const uint64_t initial_good = 0x8899aabbccddeeffULL;
    const uint64_t first_good = 0x0102030405060708ULL;
    const uint64_t second_good = 0xf1e2d3c4b5a69788ULL;
    uint64_t observed_good = 0ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xa5a55a5aa5a55a5aULL);
    TEST_ASSERT("the first HS store on the PBMT=NC bad alias faults precisely before the repeated-good-store stale-exception probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_uncache_restore_pmp_full_access();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(AI_MICRO_UNCACHE_PBMT_NC_GOOD_PAGE);
    sfence_vma();
    write64(good_paddr, initial_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_nc_vaddr, first_good);
    TEST_ASSERT("after the bad PBMT=NC store fault, the first later good PBMT=NC store episode must not inherit the stale exception bit",
        excpt.triggered == false,
        "triggered=%d cause=0x%llx tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval
    );

    TEST_SETUP_EXCEPT();
    sd(good_nc_vaddr, second_good);
    fence_iorw_iorw();
    TEST_ASSERT("after the first later good PBMT=NC store already succeeded, a second good PBMT=NC store in the same alias window must also complete without a false repeated fault",
        excpt.triggered == false,
        "triggered=%d cause=0x%llx tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval
    );

    TEST_SETUP_EXCEPT();
    observed_good = ld(good_nc_vaddr);
    TEST_ASSERT("after the bad-then-good-then-good PBMT=NC store sequence, the target doubleword keeps only the youngest good payload",
        excpt.triggered == false &&
        observed_good == second_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed_good, second_good
    );

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_nc_bus_error_then_same_alias_good_pbmt_nc_load_clears_stale_fault_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    static const uint64_t good_lines[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t initial_good = good_lines[0];
    const uint64_t updated_good = good_lines[1];
    uintptr_t good_vaddr = 0;
    uint64_t observed = 0ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first HS load on the PBMT=NC bad alias reports the expected precise load access fault before the same-alias stale-fault clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    good_vaddr = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, (uintptr_t)&good_lines[0]);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(good_vaddr);
    TEST_ASSERT("after a PBMT=NC load bus error, retargeting the same PBMT=NC alias to a good physical page must let the next good uncache load complete without inheriting stale fault state",
        excpt.triggered == false &&
        observed == initial_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, initial_good
    );

    good_vaddr = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, (uintptr_t)&good_lines[1]);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(good_vaddr);
    TEST_ASSERT("after the first recovered good PBMT=NC load already succeeds, retargeting the same alias to a second static good line must still complete without a false repeated fault",
        excpt.triggered == false &&
        observed == updated_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, updated_good
    );

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_io_bus_error_then_same_alias_good_pbmt_io_load_clears_stale_fault_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = ai_micro_uncache_good_mmio64_paddr(0x380U);
    const uint64_t initial_good = 0x89abcdef01234567ULL;
    const uint64_t updated_good = 0x76543210fedcba98ULL;
    uintptr_t good_vaddr = 0;
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_uncache_write_pbmt_good_paddr(good_paddr, initial_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first HS load on the PBMT=IO bad alias reports the expected precise load access fault before the same-alias stale-fault clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    good_vaddr = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, good_paddr);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(good_vaddr);
    TEST_ASSERT("after a PBMT=IO load bus error, retargeting the same PBMT=IO alias to a good physical page must let the next good IO-style load complete without inheriting stale fault state",
        excpt.triggered == false &&
        observed == initial_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, initial_good
    );

    goto_priv(PRIV_M);
    ai_micro_uncache_write_pbmt_good_paddr(good_paddr, updated_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(good_vaddr);
    TEST_ASSERT("after the first recovered good PBMT=IO load already succeeds on the retargeted alias, a second good load on the same alias must still return the newest payload without a false repeated fault",
        excpt.triggered == false &&
        observed == updated_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, updated_good
    );

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_mmio_bus_error_then_same_alias_good_mmio_load_clears_stale_fault_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base_offset(
        AI_MICRO_UNCACHE_MMIO_GOOD_PAGE, AI_MICRO_UNCACHE_MMIO_MEM_BASE) + 0x1c0U;
    const uint64_t initial_good = 0x1123581321345589ULL;
    const uint64_t updated_good = 0x99aabbccddeeff01ULL;
    uintptr_t good_vaddr = 0;
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the first HS load on the PMA=Device bad alias reports the expected precise load access fault before the same-alias MMIO stale-fault clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    good_vaddr = ai_micro_uncache_prepare_mmio_alias_to_paddr(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, good_paddr);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(good_vaddr);
    TEST_ASSERT("after a PMA=Device load bus error, retargeting the same MMIO alias to a good physical page must let the next good MMIO-style load complete without inheriting stale fault state",
        excpt.triggered == false &&
        observed == initial_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, initial_good
    );

    goto_priv(PRIV_M);
    write64(good_paddr, updated_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(good_vaddr);
    TEST_ASSERT("after the first recovered good MMIO load already succeeds on the retargeted alias, a second good MMIO load on the same alias must still return the newest payload without a false repeated fault",
        excpt.triggered == false &&
        observed == updated_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, updated_good
    );

    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_bus_error_then_same_alias_good_mmio_store_clears_stale_exception_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base_offset(
        AI_MICRO_UNCACHE_MMIO_GOOD_PAGE, AI_MICRO_UNCACHE_MMIO_MEM_BASE) + 0x240U;
    const uint64_t initial_good = 0x1020304050607080ULL;
    const uint64_t first_good = 0x8877665544332211ULL;
    const uint64_t second_good = 0x0f1e2d3c4b5a6978ULL;
    uintptr_t good_vaddr = 0;
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xa55aa55aa55aa55aULL);
    TEST_ASSERT("the first HS store on the PMA=Device bad alias reports the expected precise store access fault before the same-alias MMIO stale-exception clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    good_vaddr = ai_micro_uncache_prepare_mmio_alias_to_paddr(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, good_paddr);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_vaddr, first_good);
    fence_iorw_iorw();
    TEST_ASSERT("after a PMA=Device store bus error, retargeting the same MMIO alias to a good physical page must let the next good MMIO-style store complete without inheriting stale exception state",
        excpt.triggered == false,
        "triggered=%d cause=0x%llx tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_vaddr, second_good);
    fence_iorw_iorw();
    TEST_ASSERT("after the first recovered good MMIO store already succeeds on the retargeted alias, a second good MMIO store on the same alias must also complete without a false repeated fault",
        excpt.triggered == false,
        "triggered=%d cause=0x%llx tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(good_vaddr);
    TEST_ASSERT("after the bad-then-good-then-good MMIO store sequence on the retargeted alias, the target doubleword keeps only the youngest good payload",
        excpt.triggered == false &&
        observed == second_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, second_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the stale-exception clear probe on the retargeted MMIO store alias, the backing doubleword matches only the final good payload",
        read64(good_paddr) == second_good
    );

    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_io_bus_error_then_same_alias_good_pbmt_io_store_clears_stale_exception_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = ai_micro_uncache_good_mmio64_paddr(0x240U);
    const uint64_t initial_good = 0x2030405060708090ULL;
    const uint64_t first_good = 0x7766554433221100ULL;
    const uint64_t second_good = 0x1f2e3d4c5b6a7988ULL;
    uintptr_t good_vaddr = 0;
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_uncache_write_pbmt_good_paddr(good_paddr, initial_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xa55aa55aa55aa55aULL);
    TEST_ASSERT("the first HS store on the PBMT=IO bad alias reports the expected precise store access fault before the same-alias IO stale-exception clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    good_vaddr = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, good_paddr);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_vaddr, first_good);
    fence_iorw_iorw();
    TEST_ASSERT("after a PBMT=IO store bus error, retargeting the same IO alias to a good physical page must let the next good IO-style store complete without inheriting stale exception state",
        excpt.triggered == false,
        "triggered=%d cause=0x%llx tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_vaddr, second_good);
    fence_iorw_iorw();
    TEST_ASSERT("after the first recovered good PBMT=IO store already succeeds on the retargeted alias, a second good IO-style store on the same alias must also complete without a false repeated fault",
        excpt.triggered == false,
        "triggered=%d cause=0x%llx tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = ld(good_vaddr);
    TEST_ASSERT("after the bad-then-good-then-good PBMT=IO store sequence on the retargeted alias, the target doubleword keeps only the youngest good payload",
        excpt.triggered == false &&
        observed == second_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, second_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the stale-exception clear probe on the retargeted PBMT=IO store alias, the backing doubleword matches only the final good payload",
        ai_micro_uncache_read_pbmt_good_paddr(good_paddr) == second_good
    );

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_bus_error_reports_older_va_under_younger_store_pressure_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base(SWITCH1) + 0x2a0U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH1) + 0x2a0U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x123456789abcdef0ULL;
    const uint64_t updated_good = 0x0badf00d55aa33ccULL;
    const uint64_t adjacent_good = 0xcafebabefeedfaceULL;
    uint64_t observed_good = 0ULL;
    uint64_t observed_adjacent = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x8877665544332211ULL);
    sd(good_vaddr, updated_good);
    fence_iorw_iorw();
    observed_good = ld(good_vaddr);
    TEST_ASSERT("when an older HS store on a PMA=Device bad alias later reports a bus-error access fault, its precise tval must stay bound to that older bad alias even if a younger normal store has already entered the same execution window and later recovers",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr &&
        observed_good == updated_good
    );

    TEST_SETUP_EXCEPT();
    observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("after the older PMA=Device store bus error and the younger normal-store recovery path, the adjacent normal-memory doubleword remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older PMA=Device store bus error does not let a younger normal store corrupt neighboring memory while recovering after trap return",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner()
{
    TEST_START();

    const uintptr_t older_bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t younger_bad_vaddr = ai_micro_uncache_alias_vaddr(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR_ALT);
    const uintptr_t good_paddr = phys_page_base(SWITCH2) + 0x2e0U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH2) + 0x2e0U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x5566778899aabbccULL;
    const uint64_t updated_good = 0x2718281828459045ULL;
    const uint64_t adjacent_good = 0x0102030405060708ULL;
    uint64_t observed_good = 0ULL;
    uint64_t observed_adjacent = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(older_bad_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("the first HS store bus error on the PMA=Device bad alias reports the first bad virtual address instead of any later alias offset",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == older_bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(younger_bad_vaddr, 0xf0e1d2c3b4a59687ULL);
    TEST_ASSERT("after the first PMA=Device store bus-error trap returns, reusing the same aliased bad page at a different offset refreshes precise tval to that next bad virtual address instead of keeping the older one stale",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == younger_bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated_good);
    fence_iorw_iorw();
    TEST_ASSERT("after two distinct PMA=Device store bus-error offsets, an unrelated normal scalar store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_good = ld(good_vaddr);
    TEST_ASSERT("after two distinct PMA=Device store bus-error offsets, the later normal-memory recovery load sees only its intended payload",
        excpt.triggered == false &&
        observed_good == updated_good
    );

    TEST_SETUP_EXCEPT();
    observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("after two distinct PMA=Device store bus-error offsets, the adjacent normal-memory doubleword remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("distinct PMA=Device store bus-error offsets do not poison later normal-memory recovery, so only the intended doubleword is updated",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_load_mmio_bus_error_reports_older_va_under_younger_load_pressure_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base(SWITCH1) + 0x320U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH1) + 0x320U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x3141592653589793ULL;
    const uint64_t adjacent_good = 0x2718281828459045ULL;
    uint64_t observed_good = 0ULL;
    uint64_t observed_adjacent = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    observed_good = ld(good_vaddr);
    TEST_ASSERT("when an older HS load on a PMA=Device bad alias later reports a bus-error access fault, its precise tval must stay bound to that older bad alias even if a younger normal load has already entered the same execution window",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr &&
        observed_good == initial_good
    );

    TEST_SETUP_EXCEPT();
    observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("after the older PMA=Device load bus error and the younger normal-load probe, the adjacent normal-memory doubleword remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older PMA=Device load bus error does not poison later normal-load recovery, so the observed normal-memory words keep their original payloads",
        read64(good_paddr) == initial_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_load_mmio_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner()
{
    TEST_START();

    const uintptr_t older_bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t younger_bad_vaddr = ai_micro_uncache_alias_vaddr(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR_ALT);
    const uintptr_t good_paddr = phys_page_base(SWITCH2) + 0x340U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH2) + 0x340U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x89abcdef01234567ULL;
    const uint64_t updated_good = 0xfedcba9876543210ULL;
    const uint64_t adjacent_good = 0x1122334455667788ULL;
    uint64_t observed_good = 0ULL;
    uint64_t observed_adjacent = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(older_bad_vaddr);
    TEST_ASSERT("the first HS load bus error on the PMA=Device bad alias reports the first bad virtual address instead of any later alias offset",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == older_bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(younger_bad_vaddr);
    TEST_ASSERT("after the first PMA=Device load bus-error trap returns, reusing the same aliased bad page at a different offset refreshes precise tval to that next bad virtual address instead of keeping the older one stale",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == younger_bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated_good);
    TEST_ASSERT("after two distinct PMA=Device load bus-error offsets, an unrelated normal scalar store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed_good = ld(good_vaddr);
    TEST_ASSERT("after two distinct PMA=Device load bus-error offsets, the later normal-memory recovery load sees only its intended payload",
        excpt.triggered == false &&
        observed_good == updated_good
    );

    TEST_SETUP_EXCEPT();
    observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("after two distinct PMA=Device load bus-error offsets, the adjacent normal-memory doubleword remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("distinct PMA=Device load bus-error offsets do not poison later normal-memory recovery, so only the intended doubleword is updated",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good
    );
    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_load_mmio_bus_error_then_load_access_fault_oldest_exception_first_corner()
{
    TEST_START();

    const uintptr_t mmio_bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_UNCACHE_ORDER_AF_PAGE) + 0x180U;
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_UNCACHE_ORDER_AF_PAGE) + 0x180U;
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_UNCACHE_ORDER_MARKER_PAGE) + 0x80U;
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_UNCACHE_ORDER_MARKER_PAGE) + 0x80U;
    const uint64_t af_seed = 0x0f1e2d3c4b5a6978ULL;
    struct ai_micro_uncache_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);
    ai_micro_uncache_make_leaf_access_fault(AI_MICRO_UNCACHE_ORDER_AF_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_uncache_load_fault_pair_capture(mmio_bad_vaddr, af_vaddr,
        marker_vaddr, &first);

    TEST_ASSERT("in the MMIO-load bus-error versus normal-load AF diagnostic pair, the first observed precise trap stays the older PMA=Device load bus error",
        first.triggered == true &&
        first.cause == CAUSE_LAF &&
        first.tval == mmio_bad_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger normal HS load in that pair still reports its own access-fault address instead of inheriting the older MMIO bus-error entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger load-AF backing word remains unchanged and the trailing marker store executes only after both precise traps in the MMIO-load-first diagnostic pair",
        read64(af_paddr) == af_seed &&
        read64(marker_paddr) == 1ULL
    );

    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_load_access_fault_then_load_mmio_bus_error_redirect_replacement_corner()
{
    TEST_START();

    const uintptr_t mmio_bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_UNCACHE_ORDER_AF_PAGE) + 0x188U;
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_UNCACHE_ORDER_AF_PAGE) + 0x188U;
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_UNCACHE_ORDER_MARKER_PAGE) + 0x88U;
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_UNCACHE_ORDER_MARKER_PAGE) + 0x88U;
    const uint64_t af_seed = 0x1122334455667788ULL;
    struct ai_micro_uncache_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);
    ai_micro_uncache_make_leaf_access_fault(AI_MICRO_UNCACHE_ORDER_AF_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_uncache_load_fault_pair_capture(af_vaddr, mmio_bad_vaddr,
        marker_vaddr, &first);

    TEST_ASSERT("in the redirect-replacement diagnostic pair with older normal-load AF and younger PMA=Device load bus error, the first precise trap remains the older access fault",
        first.triggered == true &&
        first.cause == CAUSE_LAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after that redirect, the younger PMA=Device load bus error must replace the resident exception entry and report its own bad alias instead of the stale older AF address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == mmio_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older load-AF backing word remains unchanged and the trailing marker store still executes only after both precise traps in the AF-first MMIO-load diagnostic pair",
        read64(af_paddr) == af_seed &&
        read64(marker_paddr) == 1ULL
    );

    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_mmio_bus_error_then_store_access_fault_oldest_exception_first_corner()
{
    TEST_START();

    const uintptr_t mmio_bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_UNCACHE_ORDER_AF_PAGE) + 0x1a0U;
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_UNCACHE_ORDER_AF_PAGE) + 0x1a0U;
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_UNCACHE_ORDER_MARKER_PAGE) + 0xa0U;
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_UNCACHE_ORDER_MARKER_PAGE) + 0xa0U;
    const uint64_t af_seed = 0x5566778899aabbccULL;
    struct ai_micro_uncache_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);
    ai_micro_uncache_make_leaf_access_fault(AI_MICRO_UNCACHE_ORDER_AF_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_uncache_store_fault_pair_capture(mmio_bad_vaddr, af_vaddr,
        marker_vaddr, 0xa1b2c3d4e5f60718ULL, 0x1021324354657687ULL, &first);

    TEST_ASSERT("in the MMIO-store bus-error versus normal-store AF diagnostic pair, the first observed precise trap stays the older PMA=Device store bus error",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == mmio_bad_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger normal HS store in that pair still reports its own access-fault address instead of inheriting the older MMIO bus-error entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger store-AF target remains unchanged and the trailing marker store executes only after both precise traps in the MMIO-store-first diagnostic pair",
        read64(af_paddr) == af_seed &&
        read64(marker_paddr) == 1ULL
    );

    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_store_access_fault_then_store_mmio_bus_error_redirect_replacement_corner()
{
    TEST_START();

    const uintptr_t mmio_bad_vaddr = ai_micro_uncache_prepare_mmio_bad_alias(
        AI_MICRO_UNCACHE_MMIO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_UNCACHE_ORDER_AF_PAGE) + 0x1a8U;
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_UNCACHE_ORDER_AF_PAGE) + 0x1a8U;
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_UNCACHE_ORDER_MARKER_PAGE) + 0xa8U;
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_UNCACHE_ORDER_MARKER_PAGE) + 0xa8U;
    const uint64_t af_seed = 0x89abcdef01234567ULL;
    struct ai_micro_uncache_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);
    ai_micro_uncache_make_leaf_access_fault(AI_MICRO_UNCACHE_ORDER_AF_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_uncache_store_fault_pair_capture(af_vaddr, mmio_bad_vaddr,
        marker_vaddr, 0x2233445566778899ULL, 0xfedcba9876543210ULL, &first);

    TEST_ASSERT("in the redirect-replacement diagnostic pair with older normal-store AF and younger PMA=Device store bus error, the first precise trap remains the older access fault",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after that redirect, the younger PMA=Device store bus error must replace the resident exception entry and report its own bad alias instead of the stale older AF address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == mmio_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older store-AF target remains unchanged and the trailing marker store still executes only after both precise traps in the AF-first MMIO-store diagnostic pair",
        read64(af_paddr) == af_seed &&
        read64(marker_paddr) == 1ULL
    );

    ai_micro_uncache_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_narrow_bus_error_then_different_shape_same_alias_lane_refresh_corner()
{
    TEST_START();

    const uintptr_t bad_page_start = AI_MICRO_UNCACHE_BAD_PADDR & PAGE_ADDR_MSK;
    static const uintptr_t target_offsets[] = {
        0x180U,
        0x1c0U,
    };
    static const unsigned bad_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned bad_offsets[] = {
        7U,
        2U,
    };
    static const unsigned good_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned good_offsets[] = {
        2U,
        4U,
    };
    static const uint64_t initial_goods[] = {
        0x1122334455667788ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t updated_goods[] = {
        0x8877665544332211ULL,
        0x76543210fedcba98ULL,
    };
    static const uint64_t initial_good_images[] = {
        0x1122334455667788ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t updated_good_images[] = {
        0x8877665544332211ULL,
        0x76543210fedcba98ULL,
    };

    for (unsigned round = 0; round < (sizeof(target_offsets) / sizeof(target_offsets[0])); ++round) {
        const uintptr_t target_offset = target_offsets[round];
        const uintptr_t bad_paddr = bad_page_start + target_offset + bad_offsets[round];
        const uintptr_t initial_good_paddr =
            (uintptr_t)&initial_good_images[round];
        const uintptr_t updated_good_paddr =
            (uintptr_t)&updated_good_images[round];
        const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
            AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, bad_paddr);
        const uint64_t initial_good = initial_goods[round];
        const uint64_t updated_good = updated_goods[round];
        const uint64_t expected_initial = ai_micro_uncache_extract_unsigned(
            initial_good, good_offsets[round], good_sizes[round]);
        const uint64_t expected_updated = ai_micro_uncache_extract_unsigned(
            updated_good, good_offsets[round], good_sizes[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        (void)ai_micro_uncache_narrow_load(bad_sizes[round], bad_vaddr);
        TEST_ASSERT("on the PBMT=NC replay-head narrow-load recovery path, the faulting bad-alias byte/halfword probe still reports its own precise narrow virtual address before the different-shape same-alias recovery load",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == bad_vaddr,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx bad_vaddr=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, bad_vaddr
        );

        uintptr_t good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
            AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, initial_good_paddr);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t observed_initial = ai_micro_uncache_narrow_load(
            good_sizes[round], good_vaddr_base + good_offsets[round]);
        TEST_ASSERT("after retargeting the same PBMT=NC alias from the faulting narrow bus-error page to a good physical page, the next different-shape narrow load must refresh lane selection and return only its intended bytes",
            excpt.triggered == false &&
            observed_initial == expected_initial,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_initial, expected_initial
        );

        good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
            AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, updated_good_paddr);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t observed_updated = ai_micro_uncache_narrow_load(
            good_sizes[round], good_vaddr_base + good_offsets[round]);
        TEST_ASSERT("after the first recovered PBMT=NC narrow load already succeeds on the retargeted alias, repeating the same different-shape narrow load must return the refreshed payload without a false repeated fault",
            excpt.triggered == false &&
            observed_updated == expected_updated,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_updated, expected_updated
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_line = ld(good_vaddr_base);
        TEST_ASSERT("reading back the whole retargeted PBMT=NC line after the narrow fault-recovery sequence still observes the intact backing doubleword with no stray lane corruption",
            excpt.triggered == false &&
            observed_line == updated_good,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_line, updated_good
        );
    }

    ai_micro_uncache_reset_pbmt_env();

    for (unsigned round = 0; round < (sizeof(target_offsets) / sizeof(target_offsets[0])); ++round) {
        const uintptr_t target_offset = target_offsets[round];
        const uintptr_t bad_paddr = bad_page_start + target_offset + bad_offsets[round];
        const uintptr_t initial_good_paddr =
            ai_micro_uncache_good_paddr_for_pbmt_alias(
                AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, target_offset);
        const uintptr_t updated_good_paddr =
            ai_micro_uncache_good_paddr_for_pbmt_alias(
                AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, target_offset + 0x20U);
        const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
            AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, bad_paddr);
        const uint64_t initial_good = initial_goods[round];
        const uint64_t updated_good = updated_goods[round];
        const uint64_t expected_initial = ai_micro_uncache_extract_unsigned(
            initial_good, good_offsets[round], good_sizes[round]);
        const uint64_t expected_updated = ai_micro_uncache_extract_unsigned(
            updated_good, good_offsets[round], good_sizes[round]);

        goto_priv(PRIV_M);
        ai_micro_uncache_write_pbmt_good_paddr(initial_good_paddr, initial_good);
        ai_micro_uncache_write_pbmt_good_paddr(updated_good_paddr, updated_good);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        (void)ai_micro_uncache_narrow_load(bad_sizes[round], bad_vaddr);
        TEST_ASSERT("on the PBMT=IO replay-head narrow-load recovery path, the faulting bad-alias byte/halfword probe still reports its own precise narrow virtual address before the different-shape same-alias recovery load",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == bad_vaddr,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx bad_vaddr=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, bad_vaddr
        );

        uintptr_t good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
            AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, initial_good_paddr);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t observed_initial = ai_micro_uncache_narrow_load(
            good_sizes[round], good_vaddr_base + good_offsets[round]);
        TEST_ASSERT("after retargeting the same PBMT=IO alias from the faulting narrow bus-error page to a good physical page, the next different-shape narrow load must refresh lane selection and return only its intended bytes",
            excpt.triggered == false &&
            observed_initial == expected_initial,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_initial, expected_initial
        );

        good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
            AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, updated_good_paddr);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t observed_updated = ai_micro_uncache_narrow_load(
            good_sizes[round], good_vaddr_base + good_offsets[round]);
        TEST_ASSERT("after the first recovered PBMT=IO narrow load already succeeds on the retargeted alias, repeating the same different-shape narrow load must return the refreshed payload without a false repeated fault",
            excpt.triggered == false &&
            observed_updated == expected_updated,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_updated, expected_updated
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_line = ld(good_vaddr_base);
        TEST_ASSERT("reading back the whole retargeted PBMT=IO line after the narrow fault-recovery sequence still observes the intact backing doubleword with no stray lane corruption",
            excpt.triggered == false &&
            observed_line == updated_good,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_line, updated_good
        );
    }

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_narrow_bus_error_then_different_shape_same_alias_mask_refresh_corner()
{
    TEST_START();

    const uintptr_t bad_page_start = AI_MICRO_UNCACHE_BAD_PADDR & PAGE_ADDR_MSK;
    static const uintptr_t target_offsets[] = {
        0x200U,
        0x240U,
    };
    static const unsigned bad_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned bad_offsets[] = {
        7U,
        2U,
    };
    static const uint64_t bad_values[] = {
        0x00000000000000a5ULL,
        0x000000000000b4c3ULL,
    };
    static const unsigned good_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned good_offsets[] = {
        2U,
        4U,
    };
    static const uint64_t initial_goods[] = {
        0x1817161514131211ULL,
        0x2726252423222120ULL,
    };
    static uint64_t initial_good_images[2][2] = {
        { 0x1817161514131211ULL, 0x2726252423222120ULL },
        { 0x1817161514131211ULL, 0x2726252423222120ULL },
    };
    static const uint64_t first_good_values[] = {
        0x000000000000d5e6ULL,
        0x00000000c7d8e9faULL,
    };
    static const uint64_t second_good_values[] = {
        0x00000000000093a4ULL,
        0x000000003456789aULL,
    };

    for (unsigned round = 0; round < (sizeof(target_offsets) / sizeof(target_offsets[0])); ++round) {
        const uintptr_t target_offset = target_offsets[round];
        const uintptr_t bad_paddr = bad_page_start + target_offset + bad_offsets[round];
        const uintptr_t good_paddr_base = (uintptr_t)&initial_good_images[0][round];
        const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
            AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, bad_paddr);
        const uint64_t initial_good = initial_goods[round];
        const uint64_t expected_first = ai_micro_uncache_merge_unsigned(
            initial_good, first_good_values[round], good_offsets[round], good_sizes[round]);
        const uint64_t expected_second = ai_micro_uncache_merge_unsigned(
            expected_first, second_good_values[round], good_offsets[round], good_sizes[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_uncache_narrow_store(bad_sizes[round], bad_vaddr, bad_values[round]);
        TEST_ASSERT("on the PBMT=NC storequeue-head narrow-store recovery path, the faulting bad-alias byte/halfword store still reports its own precise virtual address before the different-shape same-alias recovery store",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == bad_vaddr,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx bad_vaddr=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, bad_vaddr
        );

        const uintptr_t good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
            AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE, good_paddr_base);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_uncache_narrow_store(
            good_sizes[round], good_vaddr_base + good_offsets[round], first_good_values[round]);
        fence_iorw_iorw();
        TEST_ASSERT("after retargeting the same PBMT=NC alias from the faulting narrow bus-error page to a good physical page, the next different-shape narrow store must refresh mask selection and clear stale exception state",
            excpt.triggered == false,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_first = ld(good_vaddr_base);
        TEST_ASSERT("reading back the retargeted PBMT=NC line after the first recovered narrow store shows only the intended byte-lane merge with no stale mask carryover",
            excpt.triggered == false &&
            observed_first == expected_first,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_first, expected_first
        );

        TEST_SETUP_EXCEPT();
        ai_micro_uncache_narrow_store(
            good_sizes[round], good_vaddr_base + good_offsets[round], second_good_values[round]);
        fence_iorw_iorw();
        TEST_ASSERT("after the first recovered PBMT=NC narrow store already succeeds on the retargeted alias, repeating the same different-shape narrow store must still complete without a false repeated fault",
            excpt.triggered == false,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_second = ld(good_vaddr_base);
        TEST_ASSERT("reading back the retargeted PBMT=NC line after the second recovered narrow store still shows only the newest intended byte-lane merge",
            excpt.triggered == false &&
            observed_second == expected_second,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_second, expected_second
        );

    }

    ai_micro_uncache_reset_pbmt_env();

    for (unsigned round = 0; round < (sizeof(target_offsets) / sizeof(target_offsets[0])); ++round) {
        const uintptr_t target_offset = target_offsets[round];
        const uintptr_t bad_paddr = bad_page_start + target_offset + bad_offsets[round];
        const uintptr_t good_paddr_base =
            ai_micro_uncache_good_paddr_for_pbmt_alias(
                AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, target_offset);
        const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
            AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, bad_paddr);
        const uint64_t initial_good = initial_goods[round];
        const uint64_t expected_first = ai_micro_uncache_merge_unsigned(
            initial_good, first_good_values[round], good_offsets[round], good_sizes[round]);
        const uint64_t expected_second = ai_micro_uncache_merge_unsigned(
            expected_first, second_good_values[round], good_offsets[round], good_sizes[round]);

        goto_priv(PRIV_M);
        ai_micro_uncache_write_pbmt_good_paddr(good_paddr_base, initial_good);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_uncache_narrow_store(bad_sizes[round], bad_vaddr, bad_values[round]);
        TEST_ASSERT("on the PBMT=IO storequeue-head narrow-store recovery path, the faulting bad-alias byte/halfword store still reports its own precise virtual address before the different-shape same-alias recovery store",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == bad_vaddr,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx bad_vaddr=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, bad_vaddr
        );

        const uintptr_t good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
            AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, good_paddr_base);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_uncache_narrow_store(
            good_sizes[round], good_vaddr_base + good_offsets[round], first_good_values[round]);
        fence_iorw_iorw();
        TEST_ASSERT("after retargeting the same PBMT=IO alias from the faulting narrow bus-error page to a good physical page, the next different-shape narrow store must refresh mask selection and clear stale exception state",
            excpt.triggered == false,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_first = ld(good_vaddr_base);
        TEST_ASSERT("reading back the retargeted PBMT=IO line after the first recovered narrow store shows only the intended byte-lane merge with no stale mask carryover",
            excpt.triggered == false &&
            observed_first == expected_first,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_first, expected_first
        );

        TEST_SETUP_EXCEPT();
        ai_micro_uncache_narrow_store(
            good_sizes[round], good_vaddr_base + good_offsets[round], second_good_values[round]);
        fence_iorw_iorw();
        TEST_ASSERT("after the first recovered PBMT=IO narrow store already succeeds on the retargeted alias, repeating the same different-shape narrow store must still complete without a false repeated fault",
            excpt.triggered == false,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_second = ld(good_vaddr_base);
        TEST_ASSERT("reading back the retargeted PBMT=IO line after the second recovered narrow store still shows only the newest intended byte-lane merge",
            excpt.triggered == false &&
            observed_second == expected_second,
            "round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            round, excpt.triggered, excpt.cause, excpt.tval, observed_second, expected_second
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the PBMT=IO backing memory keeps exactly the final expected byte image after the bad-then-good narrow-store recovery round",
            ai_micro_uncache_read_pbmt_good_paddr(good_paddr_base) == expected_second,
            "round=%u observed=0x%llx expected=0x%llx",
            round, ai_micro_uncache_read_pbmt_good_paddr(good_paddr_base),
            expected_second
        );
    }

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_narrow_bus_error_tval_refreshes_across_different_shape_same_alias_faults_corner()
{
    TEST_START();

    const uintptr_t bad_page_start = AI_MICRO_UNCACHE_BAD_PADDR & PAGE_ADDR_MSK;
    static const uintptr_t target_offsets[] = {
        0x280U,
        0x2c0U,
    };
    static const unsigned older_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned older_offsets[] = {
        7U,
        2U,
    };
    static const unsigned younger_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned younger_offsets[] = {
        2U,
        4U,
    };
    static const uint64_t good_lines[] = {
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
    };
    static const uint64_t good_line_images[] = {
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
    };

    for (unsigned kind = 0; kind < 2U; ++kind) {
        const enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE;
        const bool use_static_good = !ai_micro_uncache_is_pbmt_io_alias(alias_page);

        for (unsigned round = 0; round < (sizeof(target_offsets) / sizeof(target_offsets[0])); ++round) {
            const uintptr_t target_offset = target_offsets[round];
            const uintptr_t older_bad_paddr =
                bad_page_start + target_offset + older_offsets[round];
            const uintptr_t younger_bad_paddr =
                bad_page_start + target_offset + younger_offsets[round];
            const uintptr_t older_bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
                alias_page, older_bad_paddr);
            const uintptr_t younger_bad_vaddr = ai_micro_uncache_alias_vaddr(
                alias_page, younger_bad_paddr);
            const uintptr_t good_paddr_base = use_static_good ?
                (uintptr_t)&good_line_images[round] :
                ai_micro_uncache_good_paddr_for_pbmt_alias(
                    alias_page, target_offset);
            const uint64_t good_line = good_lines[round];
            const uint64_t expected_recovery = ai_micro_uncache_extract_unsigned(
                good_line, younger_offsets[round], younger_sizes[round]);

            if (!use_static_good) {
                goto_priv(PRIV_M);
                ai_micro_uncache_write_pbmt_good_paddr(
                    good_paddr_base, good_line);
            }

            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            (void)ai_micro_uncache_narrow_load(older_sizes[round], older_bad_vaddr);
            TEST_ASSERT("for repeated PBMT narrow load bus errors on the same aliased bad page, the first fault still reports its own exact older narrow virtual address before any later different-shape fault arrives",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == older_bad_vaddr,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx older_bad_vaddr=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval, older_bad_vaddr
            );

            TEST_SETUP_EXCEPT();
            (void)ai_micro_uncache_narrow_load(younger_sizes[round], younger_bad_vaddr);
            TEST_ASSERT("after the first PBMT narrow load bus-error trap returns, reusing the same bad alias at a different offset and width must refresh precise tval to the newer narrow virtual address instead of keeping the older one stale",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == younger_bad_vaddr,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx younger_bad_vaddr=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval, younger_bad_vaddr
            );

            const uintptr_t good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
                alias_page, good_paddr_base);

            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            uint64_t observed_recovery = ai_micro_uncache_narrow_load(
                younger_sizes[round], good_vaddr_base + younger_offsets[round]);
            TEST_ASSERT("after two different-shape PBMT narrow load bus errors on the same alias, retargeting that alias to a good page must let the next recovery load observe only the intended narrow slice with no stale fault carryover",
                excpt.triggered == false &&
                observed_recovery == expected_recovery,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval, observed_recovery, expected_recovery
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_line = ld(good_vaddr_base);
            TEST_ASSERT("after the repeated-different-shape PBMT narrow load fault sequence, whole-line readback through the retargeted alias still shows the intact backing doubleword",
                excpt.triggered == false &&
                observed_line == good_line,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval, observed_line, good_line
            );
        }
    }

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_narrow_bus_error_tval_refreshes_across_different_shape_same_alias_faults_corner()
{
    TEST_START();

    const uintptr_t bad_page_start = AI_MICRO_UNCACHE_BAD_PADDR & PAGE_ADDR_MSK;
    static const uintptr_t target_offsets[] = {
        0x300U,
        0x340U,
    };
    static const unsigned older_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned older_offsets[] = {
        7U,
        2U,
    };
    static const uint64_t older_values[] = {
        0x00000000000000a5ULL,
        0x000000000000c3d4ULL,
    };
    static const unsigned younger_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned younger_offsets[] = {
        2U,
        4U,
    };
    static const uint64_t younger_values[] = {
        0x00000000000091b2ULL,
        0x000000003456789aULL,
    };
    static const uint64_t initial_lines[] = {
        0x2233445566778899ULL,
        0x3141592653589793ULL,
    };
    static uint64_t initial_line_images[2][2] = {
        { 0x2233445566778899ULL, 0x3141592653589793ULL },
        { 0x2233445566778899ULL, 0x3141592653589793ULL },
    };

    for (unsigned kind = 0; kind < 2U; ++kind) {
        const enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE;
        const bool use_static_good =
            !ai_micro_uncache_is_pbmt_io_alias(alias_page);

        for (unsigned round = 0; round < (sizeof(target_offsets) / sizeof(target_offsets[0])); ++round) {
            const uintptr_t target_offset = target_offsets[round];
            const uintptr_t older_bad_paddr =
                bad_page_start + target_offset + older_offsets[round];
            const uintptr_t younger_bad_paddr =
                bad_page_start + target_offset + younger_offsets[round];
            const uintptr_t older_bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
                alias_page, older_bad_paddr);
            const uintptr_t younger_bad_vaddr = ai_micro_uncache_alias_vaddr(
                alias_page, younger_bad_paddr);
            const uintptr_t good_paddr_base = use_static_good ?
                (uintptr_t)&initial_line_images[kind][round] :
                ai_micro_uncache_good_paddr_for_pbmt_alias(
                    alias_page, target_offset);
            const uint64_t initial_line = initial_lines[round];
            const uint64_t expected_line = ai_micro_uncache_merge_unsigned(
                initial_line, younger_values[round], younger_offsets[round], younger_sizes[round]);

            if (!use_static_good) {
                goto_priv(PRIV_M);
                ai_micro_uncache_write_pbmt_good_paddr(
                    good_paddr_base, initial_line);
            }

            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            ai_micro_uncache_narrow_store(older_sizes[round], older_bad_vaddr, older_values[round]);
            TEST_ASSERT("for repeated PBMT narrow store bus errors on the same aliased bad page, the first fault still reports its own exact older narrow virtual address before any later different-shape fault arrives",
                excpt.triggered == true &&
                excpt.cause == CAUSE_SAF &&
                excpt.tval == older_bad_vaddr,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx older_bad_vaddr=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval, older_bad_vaddr
            );

            TEST_SETUP_EXCEPT();
            ai_micro_uncache_narrow_store(younger_sizes[round], younger_bad_vaddr, younger_values[round]);
            TEST_ASSERT("after the first PBMT narrow store bus-error trap returns, reusing the same bad alias at a different offset and width must refresh precise tval to the newer narrow virtual address instead of keeping the older one stale",
                excpt.triggered == true &&
                excpt.cause == CAUSE_SAF &&
                excpt.tval == younger_bad_vaddr,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx younger_bad_vaddr=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval, younger_bad_vaddr
            );

            const uintptr_t good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
                alias_page, good_paddr_base);

            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            ai_micro_uncache_narrow_store(
                younger_sizes[round], good_vaddr_base + younger_offsets[round], younger_values[round]);
            fence_iorw_iorw();
            TEST_ASSERT("after two different-shape PBMT narrow store bus errors on the same alias, retargeting that alias to a good page must let the next recovery store retire without stale exception carryover",
                excpt.triggered == false,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_line = ld(good_vaddr_base);
            TEST_ASSERT("after the repeated-different-shape PBMT narrow store fault sequence, line readback through the retargeted alias shows only the intended younger byte-lane merge",
                excpt.triggered == false &&
                observed_line == expected_line,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval, observed_line, expected_line
            );

            if (!use_static_good) {
                goto_priv(PRIV_M);
                TEST_ASSERT("after the repeated-different-shape PBMT=IO narrow store fault sequence, backing memory keeps exactly the expected final byte image",
                    ai_micro_uncache_read_pbmt_good_paddr(good_paddr_base) == expected_line,
                    "kind=%u round=%u observed=0x%llx expected=0x%llx",
                    kind, round,
                    ai_micro_uncache_read_pbmt_good_paddr(good_paddr_base),
                    expected_line
                );
            }

        }
    }

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_narrow_bus_error_then_same_width_signedness_switch_recovery_corner()
{
    TEST_START();

    const uintptr_t bad_page_start = AI_MICRO_UNCACHE_BAD_PADDR & PAGE_ADDR_MSK;
    static const uintptr_t target_offsets[] = {
        0x180U,
        0x1c0U,
        0x200U,
    };
    static const unsigned sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned offsets[] = {
        7U,
        6U,
        4U,
    };
    static const uint64_t initial_goods[] = {
        0x8155aa1122334455ULL,
        0x92345566778899aaULL,
        0x89abcdef10203040ULL,
    };
    static const uint64_t updated_goods[] = {
        0xfe66778899aabbccULL,
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
    };
    static const uint64_t initial_good_images[] = {
        0x8155aa1122334455ULL,
        0x92345566778899aaULL,
        0x89abcdef10203040ULL,
    };
    static const uint64_t updated_good_images[] = {
        0xfe66778899aabbccULL,
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
    };

    for (unsigned kind = 0; kind < 2U; ++kind) {
        enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE;
        const bool use_static_good = !ai_micro_uncache_is_pbmt_io_alias(alias_page);

        for (unsigned round = 0; round < (sizeof(target_offsets) / sizeof(target_offsets[0])); ++round) {
            const uintptr_t target_offset = target_offsets[round];
            const unsigned size_bytes = sizes[round];
            const unsigned offset_bytes = offsets[round];
            const uintptr_t bad_paddr = bad_page_start + target_offset + offset_bytes;
            const uintptr_t initial_good_paddr = use_static_good ?
                (uintptr_t)&initial_good_images[round] :
                ai_micro_uncache_good_paddr_for_pbmt_alias(
                    alias_page, target_offset);
            const uintptr_t updated_good_paddr = use_static_good ?
                (uintptr_t)&updated_good_images[round] :
                ai_micro_uncache_good_paddr_for_pbmt_alias(
                    alias_page, target_offset + 0x20U);
            const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
                alias_page, bad_paddr);
            const uint64_t initial_good = initial_goods[round];
            const uint64_t updated_good = updated_goods[round];
            const uint64_t expected_initial_unsigned = ai_micro_uncache_extract_scalar(
                initial_good, offset_bytes, size_bytes, false);
            const uint64_t expected_initial_signed = ai_micro_uncache_extract_scalar(
                initial_good, offset_bytes, size_bytes, true);
            const uint64_t expected_updated_unsigned = ai_micro_uncache_extract_scalar(
                updated_good, offset_bytes, size_bytes, false);
            const uint64_t expected_updated_signed = ai_micro_uncache_extract_scalar(
                updated_good, offset_bytes, size_bytes, true);

            if (!use_static_good) {
                goto_priv(PRIV_M);
                ai_micro_uncache_write_pbmt_good_paddr(
                    initial_good_paddr, initial_good);
                ai_micro_uncache_write_pbmt_good_paddr(
                    updated_good_paddr, updated_good);
            }

            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            (void)ai_micro_uncache_scalar_load(true, size_bytes, bad_vaddr);
            TEST_ASSERT("after a signed narrow PBMT load bus error, the precise trap still reports the original same-width virtual address before signedness-switch recovery",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == bad_vaddr,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx bad_vaddr=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval, bad_vaddr
            );

            uintptr_t good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
                alias_page, initial_good_paddr);

            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            uint64_t observed_initial_unsigned = ai_micro_uncache_scalar_load(
                false, size_bytes, good_vaddr_base + offset_bytes);
            TEST_ASSERT("retargeting the same PBMT alias from a faulting signed narrow load to a good page must refresh same-width unsigned decode instead of inheriting stale sign extension",
                excpt.triggered == false &&
                observed_initial_unsigned == expected_initial_unsigned,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval,
                observed_initial_unsigned, expected_initial_unsigned
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_initial_signed = ai_micro_uncache_scalar_load(
                true, size_bytes, good_vaddr_base + offset_bytes);
            TEST_ASSERT("after the first same-width unsigned recovery load already succeeds, switching back to the signed narrow form on the same PBMT alias must still use the current request decode",
                excpt.triggered == false &&
                observed_initial_signed == expected_initial_signed,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval,
                observed_initial_signed, expected_initial_signed
            );

            good_vaddr_base = ai_micro_uncache_prepare_pbmt_alias_to_paddr(
                alias_page, updated_good_paddr);

            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            uint64_t observed_updated_unsigned = ai_micro_uncache_scalar_load(
                false, size_bytes, good_vaddr_base + offset_bytes);
            TEST_ASSERT("after a later backing-memory update, the same-width unsigned PBMT recovery load must observe only the refreshed narrow slice with no stale signedness carryover",
                excpt.triggered == false &&
                observed_updated_unsigned == expected_updated_unsigned,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval,
                observed_updated_unsigned, expected_updated_unsigned
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_updated_signed = ai_micro_uncache_scalar_load(
                true, size_bytes, good_vaddr_base + offset_bytes);
            TEST_ASSERT("repeating the same-width signed PBMT recovery load after the unsigned retry must still sign-extend only the current narrow slice without a false repeated fault",
                excpt.triggered == false &&
                observed_updated_signed == expected_updated_signed,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval,
                observed_updated_signed, expected_updated_signed
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_line = ld(good_vaddr_base);
            TEST_ASSERT("whole-line readback after the PBMT signedness-switch recovery sequence still observes the intact backing doubleword with no stray lane corruption",
                excpt.triggered == false &&
                observed_line == updated_good,
                "kind=%u round=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
                kind, round, excpt.triggered, excpt.cause, excpt.tval,
                observed_line, updated_good
            );
        }

        ai_micro_uncache_reset_pbmt_env();
    }

    TEST_END();
}

bool ai_micro_hs_load_pbmt_bus_error_reports_older_va_under_younger_load_pressure_corner()
{
    TEST_START();

    static const enum test_page alias_pages[] = {
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE,
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE,
    };
    static const enum test_page good_pages[] = {
        SWITCH1,
        SWITCH2,
    };
    static const uintptr_t target_offsets[] = {
        0x360U,
        0x3a0U,
    };
    static const uint64_t initial_goods[] = {
        0x3141592653589793ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t adjacent_goods[] = {
        0x2718281828459045ULL,
        0x1122334455667788ULL,
    };

    for (unsigned kind = 0; kind < (sizeof(alias_pages) / sizeof(alias_pages[0])); ++kind) {
        const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
            alias_pages[kind], AI_MICRO_UNCACHE_BAD_PADDR);
        const uintptr_t good_paddr = phys_page_base(good_pages[kind]) + target_offsets[kind];
        const uintptr_t good_vaddr = hs_page_base(good_pages[kind]) + target_offsets[kind];
        const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
        const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
        const uint64_t initial_good = initial_goods[kind];
        const uint64_t adjacent_good = adjacent_goods[kind];
        uint64_t observed_good = 0ULL;
        uint64_t observed_adjacent = 0ULL;

        goto_priv(PRIV_M);
        write64(good_paddr, initial_good);
        write64(good_adjacent_paddr, adjacent_good);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ld(bad_vaddr);
        observed_good = ld(good_vaddr);
        TEST_ASSERT("when an older HS load on a PBMT bad alias later reports a bus-error access fault, its precise tval must stay bound to that older bad alias even if a younger normal load has already entered the same execution window",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == bad_vaddr &&
            observed_good == initial_good,
            "kind=%u triggered=%d cause=0x%llx tval=0x%llx bad_vaddr=0x%llx observed=0x%llx expected=0x%llx",
            kind, excpt.triggered, excpt.cause, excpt.tval, bad_vaddr, observed_good, initial_good
        );

        TEST_SETUP_EXCEPT();
        observed_adjacent = ld(good_adjacent_vaddr);
        TEST_ASSERT("after the older PBMT load bus error and the younger normal-load probe, the adjacent normal-memory doubleword remains unchanged",
            excpt.triggered == false &&
            observed_adjacent == adjacent_good,
            "kind=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            kind, excpt.triggered, excpt.cause, excpt.tval, observed_adjacent, adjacent_good
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the older PBMT load bus error does not poison later normal-load recovery, so the observed normal-memory words keep their original payloads",
            read64(good_paddr) == initial_good &&
            read64(good_adjacent_paddr) == adjacent_good,
            "kind=%u observed_good=0x%llx expected_good=0x%llx observed_adj=0x%llx expected_adj=0x%llx",
            kind, read64(good_paddr), initial_good, read64(good_adjacent_paddr), adjacent_good
        );
    }

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner()
{
    TEST_START();

    static const enum test_page alias_pages[] = {
        AI_MICRO_UNCACHE_PBMT_NC_ALIAS_PAGE,
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE,
    };
    static const enum test_page good_pages[] = {
        SWITCH2,
        SWITCH1,
    };
    static const uintptr_t target_offsets[] = {
        0x3c0U,
        0x380U,
    };
    static const uint64_t initial_goods[] = {
        0x89abcdef01234567ULL,
        0x66778899aabbccddULL,
    };
    static const uint64_t updated_goods[] = {
        0xfedcba9876543210ULL,
        0x3141592653589793ULL,
    };
    static const uint64_t adjacent_goods[] = {
        0x1122334455667788ULL,
        0x1020304050607080ULL,
    };

    for (unsigned kind = 0; kind < (sizeof(alias_pages) / sizeof(alias_pages[0])); ++kind) {
        const uintptr_t older_bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
            alias_pages[kind], AI_MICRO_UNCACHE_BAD_PADDR);
        const uintptr_t younger_bad_vaddr = ai_micro_uncache_alias_vaddr(
            alias_pages[kind], AI_MICRO_UNCACHE_BAD_PADDR_ALT);
        const uintptr_t good_paddr = phys_page_base(good_pages[kind]) + target_offsets[kind];
        const uintptr_t good_vaddr = hs_page_base(good_pages[kind]) + target_offsets[kind];
        const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
        const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
        const uint64_t initial_good = initial_goods[kind];
        const uint64_t updated_good = updated_goods[kind];
        const uint64_t adjacent_good = adjacent_goods[kind];
        uint64_t observed_good = 0ULL;
        uint64_t observed_adjacent = 0ULL;

        goto_priv(PRIV_M);
        write64(good_paddr, initial_good);
        write64(good_adjacent_paddr, adjacent_good);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ld(older_bad_vaddr);
        TEST_ASSERT("the first HS load bus error on the PBMT bad alias reports the first bad virtual address instead of any later alias offset",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == older_bad_vaddr,
            "kind=%u triggered=%d cause=0x%llx tval=0x%llx older_bad_vaddr=0x%llx",
            kind, excpt.triggered, excpt.cause, excpt.tval, older_bad_vaddr
        );

        TEST_SETUP_EXCEPT();
        ld(younger_bad_vaddr);
        TEST_ASSERT("after the first PBMT load bus-error trap returns, reusing the same aliased bad page at a different offset refreshes precise tval to that next bad virtual address instead of keeping the older one stale",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == younger_bad_vaddr,
            "kind=%u triggered=%d cause=0x%llx tval=0x%llx younger_bad_vaddr=0x%llx",
            kind, excpt.triggered, excpt.cause, excpt.tval, younger_bad_vaddr
        );

        TEST_SETUP_EXCEPT();
        sd(good_vaddr, updated_good);
        TEST_ASSERT("after two distinct PBMT load bus-error offsets, an unrelated normal scalar store still completes successfully",
            excpt.triggered == false,
            "kind=%u triggered=%d cause=0x%llx tval=0x%llx",
            kind, excpt.triggered, excpt.cause, excpt.tval
        );

        TEST_SETUP_EXCEPT();
        observed_good = ld(good_vaddr);
        TEST_ASSERT("after two distinct PBMT load bus-error offsets, the later normal-memory recovery load sees only its intended payload",
            excpt.triggered == false &&
            observed_good == updated_good,
            "kind=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            kind, excpt.triggered, excpt.cause, excpt.tval, observed_good, updated_good
        );

        TEST_SETUP_EXCEPT();
        observed_adjacent = ld(good_adjacent_vaddr);
        TEST_ASSERT("after two distinct PBMT load bus-error offsets, the adjacent normal-memory doubleword remains unchanged",
            excpt.triggered == false &&
            observed_adjacent == adjacent_good,
            "kind=%u triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
            kind, excpt.triggered, excpt.cause, excpt.tval, observed_adjacent, adjacent_good
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("distinct PBMT load bus-error offsets do not poison later normal-memory recovery, so only the intended doubleword is updated",
            read64(good_paddr) == updated_good &&
            read64(good_adjacent_paddr) == adjacent_good,
            "kind=%u observed_good=0x%llx expected_good=0x%llx observed_adj=0x%llx expected_adj=0x%llx",
            kind, read64(good_paddr), updated_good, read64(good_adjacent_paddr), adjacent_good
        );
    }

    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_io_bus_error_reports_older_va_under_younger_store_pressure_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t good_paddr = phys_page_base(SWITCH1) + 0x3e0U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH1) + 0x3e0U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x123456789abcdef0ULL;
    const uint64_t updated_good = 0x0badf00d55aa33ccULL;
    const uint64_t adjacent_good = 0xcafebabefeedfaceULL;
    uint64_t observed_good = 0ULL;
    uint64_t observed_adjacent = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0x8877665544332211ULL);
    sd(good_vaddr, updated_good);
    fence_iorw_iorw();
    observed_good = ld(good_vaddr);
    TEST_ASSERT("when an older HS store on a PBMT=IO bad alias later reports a bus-error access fault, its precise tval must stay bound to that older bad alias even if a younger normal store has already entered the same execution window and later recovers",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr &&
        observed_good == updated_good,
        "triggered=%d cause=0x%llx tval=0x%llx bad_vaddr=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, bad_vaddr, observed_good, updated_good
    );

    TEST_SETUP_EXCEPT();
    observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("after the older PBMT=IO store bus error and the younger normal-store recovery path, the adjacent normal-memory doubleword remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed_adjacent, adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older PBMT=IO store bus error does not let a younger normal store corrupt neighboring memory while recovering after trap return",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good,
        "observed_good=0x%llx expected_good=0x%llx observed_adj=0x%llx expected_adj=0x%llx",
        read64(good_paddr), updated_good, read64(good_adjacent_paddr), adjacent_good
    );
    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_io_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner()
{
    TEST_START();

    const uintptr_t older_bad_vaddr = ai_micro_uncache_prepare_pbmt_bad_alias(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR);
    const uintptr_t younger_bad_vaddr = ai_micro_uncache_alias_vaddr(
        AI_MICRO_UNCACHE_PBMT_IO_ALIAS_PAGE, AI_MICRO_UNCACHE_BAD_PADDR_ALT);
    const uintptr_t good_paddr = phys_page_base(SWITCH2) + 0x400U;
    const uintptr_t good_vaddr = hs_page_base(SWITCH2) + 0x400U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t good_adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    const uint64_t initial_good = 0x5566778899aabbccULL;
    const uint64_t updated_good = 0x2718281828459045ULL;
    const uint64_t adjacent_good = 0x0102030405060708ULL;
    uint64_t observed_good = 0ULL;
    uint64_t observed_adjacent = 0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial_good);
    write64(good_adjacent_paddr, adjacent_good);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(older_bad_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("the first HS store bus error on the PBMT=IO bad alias reports the first bad virtual address instead of any later alias offset",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == older_bad_vaddr,
        "triggered=%d cause=0x%llx tval=0x%llx older_bad_vaddr=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, older_bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(younger_bad_vaddr, 0xf0e1d2c3b4a59687ULL);
    TEST_ASSERT("after the first PBMT=IO store bus-error trap returns, reusing the same aliased bad page at a different offset refreshes precise tval to that next bad virtual address instead of keeping the older one stale",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == younger_bad_vaddr,
        "triggered=%d cause=0x%llx tval=0x%llx younger_bad_vaddr=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, younger_bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(good_vaddr, updated_good);
    fence_iorw_iorw();
    TEST_ASSERT("after two distinct PBMT=IO store bus-error offsets, an unrelated normal scalar store still completes successfully",
        excpt.triggered == false,
        "triggered=%d cause=0x%llx tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval
    );

    TEST_SETUP_EXCEPT();
    observed_good = ld(good_vaddr);
    TEST_ASSERT("after two distinct PBMT=IO store bus-error offsets, the later normal-memory recovery load sees only its intended payload",
        excpt.triggered == false &&
        observed_good == updated_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed_good, updated_good
    );

    TEST_SETUP_EXCEPT();
    observed_adjacent = ld(good_adjacent_vaddr);
    TEST_ASSERT("after two distinct PBMT=IO store bus-error offsets, the adjacent normal-memory doubleword remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_good,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed_adjacent, adjacent_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("distinct PBMT=IO store bus-error offsets do not poison later normal-memory recovery, so only the intended doubleword is updated",
        read64(good_paddr) == updated_good &&
        read64(good_adjacent_paddr) == adjacent_good,
        "observed_good=0x%llx expected_good=0x%llx observed_adj=0x%llx expected_adj=0x%llx",
        read64(good_paddr), updated_good, read64(good_adjacent_paddr), adjacent_good
    );
    ai_micro_uncache_reset_pbmt_env();

    TEST_END();
}
