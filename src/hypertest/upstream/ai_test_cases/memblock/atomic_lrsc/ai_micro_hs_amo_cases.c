#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_HS_AMO_PAGE VSRWX_GRWX
#define AI_MICRO_HS_AMO_BAD_PADDR 0x1fffffffffff8ULL

#define AI_MICRO_HS_AMO_PMP_TOR_RWX  0x0fULL
#define AI_MICRO_HS_AMO_PMP_TOR_NO_W 0x0dULL

#define AI_MICRO_HS_AMO_PBMT_NC_PAGE VSRWXPbmt1_GURWXPbmt1
#define AI_MICRO_HS_AMO_PBMT_IO_PAGE VSRWXPbmt2_GURWXPbmt1
#define AI_MICRO_HS_AMO_PBMT_BACKING_PAGE X
#define AI_MICRO_HS_AMO_PBMT_IO_MEM_BASE  0x30000000000ULL
#define AI_MICRO_HS_AMO_PBMT_IO_MEM_LIMIT 0x40000000000ULL

#define AI_MICRO_HS_AMO_MMIO_ALIAS_PAGE   VSRWX_GRWX
#define AI_MICRO_HS_AMO_MMIO_BACKING_PAGE SCRATCHPAD
#define AI_MICRO_HS_AMO_MMIO_MEM_BASE     0x30000000000ULL

static const enum test_page ai_micro_hs_amo_pressure_pages[] = {
    VSRWX_GRWX,
    VSRW_GRWX,
    VSRWX_GRW,
    VSRW_GRW,
    VSRWX_GURWX,
    VSRW_GURWX,
    VSRWX_GURW,
    VSRW_GURW,
};

static inline pte_t *ai_micro_hs_amo_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uintptr_t ai_micro_hs_amo_alias_anchor(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_micro_hs_amo_pbmt_backing_paddr(void)
{
    return phys_page_base(AI_MICRO_HS_AMO_PBMT_BACKING_PAGE);
}

static inline uintptr_t ai_micro_hs_amo_pbmt_io_backing_paddr(void)
{
    return phys_page_base_offset(AI_MICRO_HS_AMO_PBMT_BACKING_PAGE,
        AI_MICRO_HS_AMO_PBMT_IO_MEM_BASE);
}

static inline uintptr_t ai_micro_hs_amo_mmio_backing_paddr(void)
{
    return phys_page_base_offset(AI_MICRO_HS_AMO_MMIO_BACKING_PAGE,
        AI_MICRO_HS_AMO_MMIO_MEM_BASE);
}

static inline void ai_micro_hs_amo_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_MICRO_HS_AMO_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_micro_hs_amo_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_HS_AMO_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_HS_AMO_PMP_TOR_RWX << 16));
}

static inline void ai_micro_hs_amo_reset_pbmt_env(void)
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
    ai_micro_hs_amo_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_hs_amo_prepare_pbmt_alias(enum test_page page)
{
    ai_micro_hs_amo_reset_pbmt_env();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline void ai_micro_hs_amo_prepare_pbmt_mem_3t_4t_window(void)
{
    ai_micro_hs_amo_reset_pbmt_env();

    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, AI_MICRO_HS_AMO_PBMT_IO_MEM_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_MICRO_HS_AMO_PBMT_IO_MEM_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    fence_iorw_iorw();
}

static inline uintptr_t ai_micro_hs_amo_prepare_pbmt_io_alias(enum test_page page)
{
    ai_micro_hs_amo_prepare_pbmt_mem_3t_4t_window();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x_base_paddr(page, AI_MICRO_HS_AMO_PBMT_IO_MEM_BASE);
    sfence_vma();
    return hs_page_base(page);
}

static inline void ai_micro_hs_amo_reset_mmio_env(void)
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
    ai_micro_hs_amo_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_hs_amo_prepare_mmio_alias(enum test_page alias_page,
    enum test_page backing_page)
{
    uintptr_t backing_paddr = phys_page_base_offset(backing_page,
        AI_MICRO_HS_AMO_MMIO_MEM_BASE);

    ai_micro_hs_amo_reset_mmio_env();

    CSRW(CSR_PMAADDR0, backing_paddr >> 2);
    CSRW(CSR_PMAADDR1, (backing_paddr + PAGE_SIZE) >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);

    hspt_init();
    hspt_leaf_change_base_paddr(alias_page,
        ai_micro_hs_amo_alias_anchor(alias_page, backing_paddr));
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline uintptr_t ai_micro_hs_amo_prepare_bad_paddr_alias(enum test_page page,
    uintptr_t bad_paddr)
{
    uintptr_t bad_page_start = bad_paddr & PAGE_ADDR_MSK;

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_leaf_change_base_paddr(page,
        ai_micro_hs_amo_alias_anchor(page, bad_page_start));
    sfence_vma();

    return hs_page_base(page) + (bad_paddr & (PAGE_SIZE - 1));
}

static inline uintptr_t ai_micro_hs_amo_prepare_same_paddr_alias(enum test_page alias_page,
    enum test_page base_page)
{
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_hs_amo_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline void ai_micro_hs_amo_make_leaf_access_fault(enum test_page page)
{
    goto_priv(PRIV_M);
    *ai_micro_hs_amo_hs_leaf_entry(page) |= (1ULL << 46);
    sfence_vma();
}

static inline uintptr_t ai_micro_hs_amo_cross16_addr(enum test_page page)
{
    return hs_page_base(page) + 15U;
}

static inline uint64_t ai_micro_hs_amo_cross16_expected_load(uint64_t low,
    uint64_t high)
{
    return ((high & 0x00ffffffffffffffULL) << 8) |
        ((low >> 56) & 0xffULL);
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_hs_amo_load_then_amoadd_capture(uintptr_t load_vaddr,
    uintptr_t amo_vaddr, uint64_t addend, uint64_t *load_out,
    uint64_t *amo_old_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "amoadd.d t1, a2, (a1)\n"
        "sd t0, 0(a3)\n"
        "sd t1, 0(a4)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_hs_amo_load_then_delayed_amoadd_capture(uintptr_t load_vaddr,
    uintptr_t amo_vaddr, uint64_t addend, uint64_t *load_out,
    uint64_t *amo_old_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "li t2, 0xfedcba9876543210\n"
        "li t3, 7\n"
        "divu t4, t2, t3\n"
        "addi t4, t4, 1\n"
        "divu t4, t2, t4\n"
        "andi t4, t4, 0\n"
        "amoadd.d t1, a2, (a1)\n"
        "sd t0, 0(a3)\n"
        "sd t1, 0(a4)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

static bool ai_micro_hs_atomic_entry_nearby_load_wb_window(bool check_adjacent)
{
    static const uint64_t load_before_pattern[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa55aa33cc33ccULL,
    };
    static const uint64_t load_target_pattern[] = {
        0x8877665544332211ULL,
        0x1021324354657687ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    static const uint64_t load_after_pattern[] = {
        0xa1b2c3d4e5f60718ULL,
        0x98badcfe10325476ULL,
        0x123456789abcdef0ULL,
    };
    static const uint64_t load_warm_pattern[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    static const uint64_t amo_before_pattern[] = {
        0xdeadbeefcafef00dULL,
        0x0badf00dfacefeedULL,
        0x1122334455667788ULL,
    };
    static const uint64_t amo_target_pattern[] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    static const uint64_t amo_after_pattern[] = {
        0x8877665544332211ULL,
        0xf0e1d2c3b4a59687ULL,
        0x7766554433221100ULL,
    };
    static const uint64_t amo_warm_pattern[] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1213141516171819ULL,
        0x1a1b1c1d1e1f2021ULL,
    };
    static const uint64_t immediate_addend[] = {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
        0x0303030303030303ULL,
    };
    static const uint64_t delayed_addend[] = {
        0x1111111111111111ULL,
        0x1212121212121212ULL,
        0x1313131313131313ULL,
    };
    static const uint64_t followup_addend[] = {
        0x2121212121212121ULL,
        0x2222222222222222ULL,
        0x2323232323232323ULL,
    };
    const enum test_page load_page = SWITCH1;
    const enum test_page amo_page = VSRWX_GRWX;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x18U;
    const uintptr_t load_warm_vaddr = hs_page_base(load_page) + 0x40U;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0x80U;
    const uintptr_t amo_warm_vaddr = hs_page_base(amo_page) + 0x40U;
    const uintptr_t load_base_paddr = phys_page_base(load_page);
    const uintptr_t load_before_paddr = load_base_paddr + 0x10U;
    const uintptr_t load_target_paddr = load_base_paddr + 0x18U;
    const uintptr_t load_after_paddr = load_base_paddr + 0x20U;
    const uintptr_t load_warm_paddr = load_base_paddr + 0x40U;
    const uintptr_t amo_base_paddr = phys_page_base(amo_page);
    const uintptr_t amo_warm_paddr = amo_base_paddr + 0x40U;
    const uintptr_t amo_before_paddr = amo_base_paddr + 0x78U;
    const uintptr_t amo_target_paddr = amo_base_paddr + 0x80U;
    const uintptr_t amo_after_paddr = amo_base_paddr + 0x88U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        uint64_t warm_load = 0ULL;
        uint64_t warm_amo = 0ULL;
        uint64_t load_value = 0ULL;
        uint64_t amo_old_value = 0ULL;
        uint64_t followup_old_value = 0ULL;
        const uint64_t load_expected = load_target_pattern[round];
        const uint64_t after_pair_expected = amo_target_pattern[round] +
            immediate_addend[round] + delayed_addend[round];

        goto_priv(PRIV_M);
        write64(load_before_paddr, load_before_pattern[round]);
        write64(load_target_paddr, load_target_pattern[round]);
        write64(load_after_paddr, load_after_pattern[round]);
        write64(load_warm_paddr, load_warm_pattern[round]);
        write64(amo_warm_paddr, amo_warm_pattern[round]);
        write64(amo_before_paddr, amo_before_pattern[round]);
        write64(amo_target_paddr, amo_target_pattern[round]);
        write64(amo_after_paddr, amo_after_pattern[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        warm_load = ld(load_warm_vaddr);
        warm_amo = ld(amo_warm_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the atomic-entry overlap probe, both the nearby load page and the AMO page still warm independently without exception",
            warm_load == load_warm_pattern[round] &&
            warm_amo == amo_warm_pattern[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_hs_amo_load_then_amoadd_capture(load_vaddr, amo_vaddr,
            immediate_addend[round], &load_value, &amo_old_value);
        AI_ASSERT_NO_EXCEPT_AND("when a cacheable aligned scalar load is issued immediately before an HS atomic entry, the nearby load result and the atomic old value both stay visible with no lost writeback on the shared surface",
            load_value == load_expected &&
            amo_old_value == amo_target_pattern[round]
        );

        load_value = 0ULL;
        amo_old_value = 0ULL;
        TEST_SETUP_EXCEPT();
        ai_micro_hs_amo_load_then_delayed_amoadd_capture(load_vaddr, amo_vaddr,
            delayed_addend[round], &load_value, &amo_old_value);
        AI_ASSERT_NO_EXCEPT_AND("even after repeating the nearby aligned-load-plus-atomic entry window with a tiny spacing change, the load payload and atomic old value still stay bound to the right owners",
            load_value == load_expected &&
            amo_old_value == amo_target_pattern[round] +
                immediate_addend[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after two nearby aligned-load-then-atomic entry windows, the load-side backing words remain unchanged and the atomic word contains exactly the two accumulated addends",
            read64(load_target_paddr) == load_target_pattern[round] &&
            read64(load_warm_paddr) == load_warm_pattern[round] &&
            read64(amo_warm_paddr) == amo_warm_pattern[round] &&
            read64(amo_target_paddr) == after_pair_expected &&
            (!check_adjacent ||
                (read64(load_before_paddr) == load_before_pattern[round] &&
                 read64(load_after_paddr) == load_after_pattern[round] &&
                 read64(amo_before_paddr) == amo_before_pattern[round] &&
                 read64(amo_after_paddr) == amo_after_pattern[round]))
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        followup_old_value = amoadd_d(amo_vaddr, followup_addend[round]);
        AI_ASSERT_NO_EXCEPT_AND("a follow-up HS atomic after the nearby load-overlap window sees exactly the accumulated prior result, showing there was no hidden lost or duplicate completion",
            followup_old_value == after_pair_expected
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the final atomic backing word contains only the visible accumulated sequence after the follow-up atomic",
            read64(amo_target_paddr) == after_pair_expected +
                followup_addend[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_amoadd_data_visible()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t addend = 0x0102030405060708ULL;
    const uint64_t expected = initial_value + addend;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("an HS amoadd.d on a translated cacheable page returns the old value without surfacing an exception",
        old_value == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful HS amoadd.d writes the accumulated result back to the backing memory",
        read64(paddr) == expected
    );

    TEST_END();
}

bool ai_micro_hs_amo_back_to_back_same_address_accumulates_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x88U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x88U;
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t first_addend = 0x0101010101010101ULL;
    const uint64_t second_addend = 0x1111111111111111ULL;
    const uint64_t after_first = initial_value + first_addend;
    const uint64_t final_value = after_first + second_addend;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t first_old_value = amoadd_d(vaddr, first_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first back-to-back AMO on the same HS address returns the original value and retires without exception",
        first_old_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_old_value = amoadd_d(vaddr, second_addend);
    AI_ASSERT_NO_EXCEPT_AND("the second back-to-back AMO on the same HS address observes the first AMO's committed result as its old value",
        second_old_value == after_first
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the same HS address after two back-to-back AMOs returns the fully accumulated final value",
        observed == final_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("two back-to-back AMOs on the same address commit the accumulated final value to backing memory",
        read64(paddr) == final_value
    );

    TEST_END();
}

bool ai_micro_hs_amo_back_to_back_same_address_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0xc8U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0xc8U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t adjacent = 0xf0e1d2c3b4a59687ULL;
    const uint64_t first_addend = 0x0101010101010101ULL;
    const uint64_t second_addend = 0x1111111111111111ULL;
    const uint64_t after_first = initial_value + first_addend;
    const uint64_t final_value = after_first + second_addend;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t first_old_value = amoadd_d(vaddr, first_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first back-to-back AMO on the same HS address returns the original value before the adjacent-preservation corner",
        first_old_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_old_value = amoadd_d(vaddr, second_addend);
    AI_ASSERT_NO_EXCEPT_AND("the second back-to-back AMO on the same HS address observes the first AMO's committed result before the adjacent-preservation corner",
        second_old_value == after_first
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the same HS address after two back-to-back AMOs still returns the fully accumulated final value in the adjacent-preservation corner",
        observed == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after two back-to-back AMOs on the previous word",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("two back-to-back AMOs on the same address update only the target backing word while preserving the adjacent word",
        read64(paddr) == final_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_amo_src2_operand_back_to_back_wd_chain_corner()
{
    TEST_START();

    const uintptr_t d_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x480U;
    const uintptr_t d_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x480U;
    const uintptr_t d_adjacent_paddr = d_paddr + sizeof(uint64_t);
    const uintptr_t w_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x4a0U;
    const uintptr_t w_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x4a0U;
    const uintptr_t w_adjacent_paddr = w_paddr + sizeof(uint64_t);
    const uint64_t d_initial = 0x0123456789abcdefULL;
    const uint64_t d_addend0 = 0x0000000000000011ULL;
    const uint64_t d_addend1 = 0x0000000000002200ULL;
    const uint64_t d_addend2 = 0x0000000000330000ULL;
    const uint64_t d_after0 = d_initial + d_addend0;
    const uint64_t d_after1 = d_after0 + d_addend1;
    const uint64_t d_final = d_after1 + d_addend2;
    const uint64_t d_adjacent = 0xfedcba9876543210ULL;
    const uint64_t w_initial64 = 0xa5a55a5a7fffff10ULL;
    const uint32_t w_initial = 0x7fffff10U;
    const uint32_t w_addend0 = 0x00000031U;
    const uint32_t w_addend1 = 0x00001000U;
    const uint32_t w_addend2 = 0x80000005U;
    const uint32_t w_after0 = w_initial + w_addend0;
    const uint32_t w_after1 = w_after0 + w_addend1;
    const uint32_t w_final_low = w_after1 + w_addend2;
    const uint64_t w_final64 = (w_initial64 & 0xffffffff00000000ULL) |
        (uint64_t)w_final_low;
    const uint64_t w_adjacent = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    write64(d_paddr, d_initial);
    write64(d_adjacent_paddr, d_adjacent);
    write64(w_paddr, w_initial64);
    write64(w_adjacent_paddr, w_adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t d_old0 = amoadd_d(d_vaddr, d_addend0);
    AI_ASSERT_NO_EXCEPT_AND("AMO src2 owner D-chain step0 returns the seed and consumes only step0 operand",
        d_old0 == d_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t d_old1 = amoadd_d(d_vaddr, d_addend1);
    AI_ASSERT_NO_EXCEPT_AND("AMO src2 owner D-chain step1 observes step0 result and cannot reuse or skip the previous operand",
        d_old1 == d_after0
    );

    TEST_SETUP_EXCEPT();
    uint64_t d_old2 = amoadd_d(d_vaddr, d_addend2);
    AI_ASSERT_NO_EXCEPT_AND("AMO src2 owner D-chain step2 observes the two unique earlier operands in order",
        d_old2 == d_after1
    );

    TEST_SETUP_EXCEPT();
    uint32_t w_old0 = amoadd_w(w_vaddr, w_addend0);
    AI_ASSERT_NO_EXCEPT_AND("AMO src2 owner W-chain step0 returns the old low word before width switching",
        w_old0 == w_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t w_old1 = amoadd_w(w_vaddr, w_addend1);
    AI_ASSERT_NO_EXCEPT_AND("AMO src2 owner W-chain step1 uses its own low-word operand instead of a prior D operand",
        w_old1 == w_after0
    );

    TEST_SETUP_EXCEPT();
    uint32_t w_old2 = amoadd_w(w_vaddr, w_addend2);
    AI_ASSERT_NO_EXCEPT_AND("AMO src2 owner W-chain step2 keeps the W operand and lane mask bound to the current episode",
        w_old2 == w_after1
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("back-to-back AMO D/W operand chains leave memory with the exact ordered operand sum and preserve adjacent guards",
        read64(d_paddr) == d_final &&
        read64(d_adjacent_paddr) == d_adjacent &&
        read64(w_paddr) == w_final64 &&
        read64(w_adjacent_paddr) == w_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_amo_src2_operand_fault_success_isolation_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x500U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x500U;
    const uintptr_t recovery_vaddr =
        hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x520U;
    const uintptr_t recovery_paddr =
        phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x520U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t bad_vaddr = hs_page_base(VSRW_GRWX) + 0x500U;
    const uintptr_t bad_paddr = phys_page_base(VSRW_GRWX) + 0x500U;
    const uintptr_t bad_adjacent_paddr = bad_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x1111222233334444ULL;
    const uint64_t good_addend = 0x0000000000000055ULL;
    const uint64_t recovery_initial = 0x777788889999aaaaULL;
    const uint64_t recovery_addend = 0x0000000000006600ULL;
    const uint64_t bad_initial = 0xdeadbeefcafef00dULL;
    const uint64_t bad_adjacent = 0x0123012301230123ULL;
    const uint64_t good_adjacent = 0xf0e1d2c3b4a59687ULL;
    const uint64_t fault_addend = 0x7f7f7f7f7f7f7f7fULL;
    const uint64_t final_fault_addend = 0x0101010101010101ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(bad_paddr, bad_initial);
    write64(bad_adjacent_paddr, bad_adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        bad_paddr, bad_paddr + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, fault_addend);
    TEST_ASSERT("fault-first AMO src2 owner probe reports SAF on the denied page without letting the fault operand become resident data",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the valid AMO immediately after the SAF consumes its own operand and returns the good target seed",
        good_old == good_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovery_old = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a third independent AMO after fault->success keeps a fresh src2 owner and old-value chain",
        recovery_old == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("fault->success AMO operand isolation preserves bad target bytes while committing only the two valid operands",
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent &&
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent &&
        read64(recovery_paddr) == (recovery_initial + recovery_addend)
    );

    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    write64(bad_paddr, bad_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    good_old = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("success-first AMO src2 owner probe commits the good operand before a later denied AMO",
        good_old == good_initial
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, final_fault_addend);
    TEST_ASSERT("the later denied AMO reports its own SAF and does not inherit the preceding successful AMO operand",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("success->fault AMO operand isolation keeps the successful side effect and still suppresses the denied target update",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent &&
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_atomic_entry_while_nearby_load_wb_no_lost_ldout_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_hs_atomic_entry_nearby_load_wb_window(false);
    TEST_END();
}

bool ai_micro_hs_atomic_entry_while_nearby_load_wb_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_hs_atomic_entry_nearby_load_wb_window(true);
    TEST_END();
}

bool ai_micro_hs_atomic_boundary_store_drain_no_early_start_from_stale_empty_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x320U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x320U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t older_store_value = 0x8877665544332211ULL;
    const uint64_t adjacent = 0xf0e1d2c3b4a59687ULL;
    const uint64_t addend = 0x1111111111111111ULL;
    const uint64_t final_value = older_store_value + addend;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS AMO target word is readable before the boundary store-drain stale-empty probe starts",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the boundary store-drain stale-empty probe starts",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, older_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the older HS store to the future AMO target retires without exception immediately before the boundary AMO probe",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("the boundary AMO must observe the immediately preceding store-drain result instead of starting early from a stale empty indication",
        old_value == older_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the HS target word after the boundary store-drain AMO probe returns the accumulated final value",
        observed == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the boundary store-drain AMO probe",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the boundary store-drain stale-empty AMO corner commits the preceding store before the AMO read-modify-write and preserves the adjacent backing word",
        read64(paddr) == final_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_atomic_boundary_store_drain_repeated_no_false_empty_or_extra_wait_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x3a0U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x3a0U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x420U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x420U;
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t first_store_value = 0x1122334455667788ULL;
    const uint64_t first_addend = 0x0101010101010101ULL;
    const uint64_t second_store_value = 0x99aabbccddeeff00ULL;
    const uint64_t second_addend = 0x0202020202020202ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;
    const uint64_t recovery_initial = 0x0fedcba987654321ULL;
    const uint64_t recovery_updated = 0xa1b2c3d4e5f60718ULL;
    const uint64_t after_first = first_store_value + first_addend;
    const uint64_t final_value = second_store_value + second_addend;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    write64(recovery_paddr, recovery_initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS AMO target word is readable before the repeated boundary store-drain probe starts",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is readable before the repeated boundary store-drain probe starts",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later recovery store location is readable before the repeated boundary store-drain probe starts",
        warmed_recovery == recovery_initial
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the first older HS store retires without exception before the first boundary AMO in the repeated drain probe",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_old_value = amoadd_d(vaddr, first_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first boundary AMO observes the immediately preceding store value instead of a false-empty early-start state",
        first_old_value == first_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after_first = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the first boundary AMO, the HS target word already returns the first accumulated result",
        observed_after_first == after_first
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the second older HS store also retires without exception immediately before the second boundary AMO",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_old_value = amoadd_d(vaddr, second_addend);
    AI_ASSERT_NO_EXCEPT_AND("the second boundary AMO still observes the immediately preceding second store value without false-empty misordering or extra-wait breakage",
        second_old_value == second_store_value
    );

    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_updated);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated store-drain boundary toggles, a later independent HS store still retires without getting stuck behind stale drain bookkeeping",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_final = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated boundary AMO sequence, the HS target word returns the second accumulated final value",
        observed_final == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the repeated boundary store-drain AMO sequence",
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later independent HS recovery store is visible after the repeated boundary store-drain AMO sequence",
        observed_recovery == recovery_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated boundary store-drain AMO corner neither starts early nor waits forever: both immediate pre-AMO stores are observed in order, the final AMO result reaches backing memory, and the later recovery store still commits",
        read64(paddr) == final_value &&
        read64(adjacent_paddr) == adjacent &&
        read64(recovery_paddr) == recovery_updated
    );

    TEST_END();
}

bool ai_micro_hs_amo_same_paddr_alias_back_to_back_accumulates_corner()
{
    TEST_START();

    const enum test_page base_page = AI_MICRO_HS_AMO_PAGE;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xa8U;
    const uintptr_t paddr = phys_page_base(base_page) + 0xa8U;
    const uint64_t initial_value = 0x89abcdef01234567ULL;
    const uint64_t first_addend = 0x0202020202020202ULL;
    const uint64_t second_addend = 0x1010101010101010ULL;
    const uint64_t after_first = initial_value + first_addend;
    const uint64_t final_value = after_first + second_addend;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    ai_micro_hs_amo_restore_pmp_full_access();

    uintptr_t alias_page_base = ai_micro_hs_amo_prepare_same_paddr_alias(alias_page, base_page);
    uintptr_t alias_vaddr = alias_page_base + 0xa8U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_old_value = amoadd_d(base_vaddr, first_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO through the original HS alias returns the original shared physical value",
        first_old_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_old_value = amoadd_d(alias_vaddr, second_addend);
    AI_ASSERT_NO_EXCEPT_AND("the second AMO through a different HS virtual alias observes the first alias AMO's committed result as its old value",
        second_old_value == after_first
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias reloads the fully accumulated final value after the back-to-back alias AMOs",
        observed_base == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the switched HS alias also reloads the same fully accumulated final value after the back-to-back alias AMOs",
        observed_alias == final_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("back-to-back AMOs through different virtual aliases commit the same accumulated final value to the shared backing memory",
        read64(paddr) == final_value
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_same_paddr_alias_back_to_back_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = AI_MICRO_HS_AMO_PAGE;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xe8U;
    const uintptr_t paddr = phys_page_base(base_page) + 0xe8U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x89abcdef01234567ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;
    const uint64_t first_addend = 0x0202020202020202ULL;
    const uint64_t second_addend = 0x1010101010101010ULL;
    const uint64_t after_first = initial_value + first_addend;
    const uint64_t final_value = after_first + second_addend;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    uintptr_t alias_page_base = ai_micro_hs_amo_prepare_same_paddr_alias(alias_page, base_page);
    uintptr_t alias_vaddr = alias_page_base + 0xe8U;
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_old_value = amoadd_d(base_vaddr, first_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO through the original HS alias returns the original shared physical value before the adjacent-preservation alias corner",
        first_old_value == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_old_value = amoadd_d(alias_vaddr, second_addend);
    AI_ASSERT_NO_EXCEPT_AND("the second AMO through a different HS virtual alias observes the first alias AMO's committed result before the adjacent-preservation alias corner",
        second_old_value == after_first
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias reloads the fully accumulated final value after the back-to-back alias AMOs in the adjacent-preservation corner",
        observed_base == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the switched HS alias also reloads the same fully accumulated final value after the back-to-back alias AMOs in the adjacent-preservation corner",
        observed_alias == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent shared HS word remains unchanged after back-to-back alias AMOs on the previous word",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("back-to-back AMOs through different virtual aliases update only the shared target backing word while preserving the adjacent word",
        read64(paddr) == final_value &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_dcache_miss_data_visible()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE);
    const uint64_t cached = 0x1122334455667788ULL;
    const uint64_t refreshed = 0x8877665544332211ULL;
    const uint64_t addend = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(paddr, cached);
    ai_micro_hs_amo_restore_pmp_full_access();
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a translated HS cacheable page is readable before the AMO dcache-miss probe starts",
        warmed == cached
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr);
    TEST_ASSERT("the warmed cache line can be invalidated before probing the AMO dcache-miss path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    write64(paddr, refreshed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("an HS amoadd.d still refetches the refreshed payload and retires cleanly after the line was invalidated",
        old_value == refreshed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AMO dcache-miss probe commits the expected accumulated value back to memory",
        read64(paddr) == (refreshed + addend)
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_amo_dcache_miss_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x40U;
    uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x40U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t cached = 0x1122334455667788ULL;
    const uint64_t refreshed = 0x8877665544332211ULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t addend = 0x0102030405060708ULL;
    const uint64_t final_value = refreshed + addend;

    goto_priv(PRIV_M);
    write64(paddr, cached);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a translated HS cacheable page is readable before the AMO dcache-miss adjacent-preservation probe starts",
        warmed == cached
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the AMO dcache-miss adjacent-preservation probe starts",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr);
    TEST_ASSERT("the warmed cache line can be invalidated before probing the AMO dcache-miss adjacent-preservation path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    write64(paddr, refreshed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("an HS amoadd.d still refetches the refreshed payload and retires cleanly after the line was invalidated in the adjacent-preservation corner",
        old_value == refreshed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the target HS word after the AMO dcache-miss adjacent-preservation probe returns the accumulated final value",
        observed == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the AMO dcache-miss adjacent-preservation probe",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AMO dcache-miss adjacent-preservation probe updates only the target backing word while preserving the adjacent word",
        read64(paddr) == final_value &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_amo_mshr_full_retries_visible()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_hs_amo_pressure_pages) /
        sizeof(ai_micro_hs_amo_pressure_pages[0]);
    const uint64_t seed = 0x1100220033004400ULL;
    const uint64_t refreshed_seed = 0x5500660077008800ULL;
    const uint64_t step = 0x101ULL;
    const uint64_t addend = 0x1111111111111111ULL;
    const enum test_page target_page = ai_micro_hs_amo_pressure_pages[page_count - 1];
    const uintptr_t target_vaddr = hs_page_base(target_page);
    const uintptr_t target_paddr = phys_page_base(target_page);
    const uint64_t target_before = refreshed_seed + ((page_count - 1) * step);
    const uintptr_t sample_paddr = phys_page_base(ai_micro_hs_amo_pressure_pages[0]);
    const uint64_t sample_expected = refreshed_seed;

    goto_priv(PRIV_M);
    ai_micro_hs_amo_restore_pmp_full_access();
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_hs_amo_pressure_pages[i]), seed + (i * step));
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_hs_amo_pressure_pages[i]);
        uint64_t expected = seed + (i * step);

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("each HS AMO pressure page is readable before the miss-pressure sequence begins",
            warmed == expected
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed cache line can be invalidated before the AMO miss-pressure sequence",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        write64(phys_page_base(ai_micro_hs_amo_pressure_pages[i]),
            refreshed_seed + (i * step));
        goto_priv(PRIV_HS);
    }

    // Build cold-line traffic first so the target AMO runs after a burst of
    // outstanding miss-like probes that approximate MSHR pressure.
    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_hs_amo_pressure_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch remains silent while preparing the AMO miss-pressure probe",
            excpt.triggered == false
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(target_vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("an HS amoadd.d still returns the refreshed old value and completes without an exception after miss-pressure traffic",
        old_value == target_before
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the target AMO miss-pressure probe commits the accumulated result back to memory",
        read64(target_paddr) == (target_before + addend)
    );
    TEST_ASSERT("the non-target pressure pages remain at their refreshed values after the target AMO finishes",
        read64(sample_paddr) == sample_expected
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_amo_mshr_full_retries_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_hs_amo_pressure_pages) /
        sizeof(ai_micro_hs_amo_pressure_pages[0]);
    const uintptr_t probe_offset = 0x80U;
    const uint64_t seed = 0x2200440066008800ULL;
    const uint64_t refreshed_seed = 0x9900aa00bb00cc00ULL;
    const uint64_t step = 0x101ULL;
    const uint64_t addend = 0x1111111111111111ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;
    const enum test_page target_page = ai_micro_hs_amo_pressure_pages[page_count - 1];
    const uintptr_t target_vaddr = hs_page_base(target_page) + probe_offset;
    const uintptr_t target_paddr = phys_page_base(target_page) + probe_offset;
    const uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t target_before = refreshed_seed + ((page_count - 1) * step);
    const uint64_t target_after = target_before + addend;
    const uintptr_t sample_paddr =
        phys_page_base(ai_micro_hs_amo_pressure_pages[0]) + probe_offset;
    const uint64_t sample_expected = refreshed_seed;

    goto_priv(PRIV_M);
    ai_micro_hs_amo_restore_pmp_full_access();
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_hs_amo_pressure_pages[i]) + probe_offset,
            seed + (i * step));
    }
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_hs_amo_pressure_pages[i]) + probe_offset;
        uint64_t expected = seed + (i * step);

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("each HS AMO pressure page remains readable before the adjacent-preservation miss-pressure sequence begins",
            warmed == expected
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed cache line can be invalidated before the adjacent-preservation AMO miss-pressure sequence",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        write64(phys_page_base(ai_micro_hs_amo_pressure_pages[i]) + probe_offset,
            refreshed_seed + (i * step));
        goto_priv(PRIV_HS);
    }

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target page's adjacent HS word is readable before the adjacent-preservation AMO miss-pressure probe starts",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_vaddr);
    TEST_ASSERT("the target cache line can be invalidated again after warming the adjacent word so the final AMO still re-enters the miss-pressure path",
        excpt.triggered == false
    );

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_hs_amo_pressure_pages[i]) + probe_offset);
        TEST_ASSERT("a pressure-building HS software prefetch remains silent while preparing the adjacent-preservation AMO miss-pressure probe",
            excpt.triggered == false
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(target_vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("an HS amoadd.d still returns the refreshed old value and completes without an exception after miss-pressure traffic in the adjacent-preservation corner",
        old_value == target_before
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(target_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the target HS word after the adjacent-preservation AMO miss-pressure probe returns the accumulated final value",
        observed == target_after
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the adjacent-preservation AMO miss-pressure probe",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the target AMO miss-pressure adjacent-preservation probe updates only the target backing word while preserving the adjacent word and the sampled non-target pressure page",
        read64(target_paddr) == target_after &&
        read64(adjacent_paddr) == adjacent &&
        read64(sample_paddr) == sample_expected
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_amo_redirect_then_cold_miss_replay_forward_progress_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x3c0U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x3c0U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t cached = 0x1021324354657687ULL;
    const uint64_t refreshed = 0x8877665544332211ULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;
    const uint64_t fault_addend = 0x0101010101010101ULL;
    const uint64_t recovery_addend = 0x1212121212121212ULL;
    const uint64_t followup_addend = 0x2323232323232323ULL;
    const uint64_t after_recovery = refreshed + recovery_addend;
    const uint64_t final_value = after_recovery + followup_addend;

    goto_priv(PRIV_M);
    write64(paddr, cached);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the translated HS AMO target is readable before preparing the redirect-plus-cold-miss replay probe",
        warmed == cached
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before preparing the redirect-plus-cold-miss replay probe",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr);
    TEST_ASSERT("the warmed cache line can be invalidated before the redirect-plus-cold-miss AMO replay probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    write64(paddr, refreshed);
    ai_micro_hs_amo_make_leaf_access_fault(AI_MICRO_HS_AMO_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, fault_addend);
    TEST_ASSERT("the leading HS AMO in the redirect-plus-cold-miss subset reports store access fault once the target leaf is forced into access-fault mode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the redirected faulting AMO leaves the refreshed target payload and its adjacent word unchanged before the same-line replayed AMOs run",
        read64(paddr) == refreshed &&
        read64(adjacent_paddr) == adjacent
    );
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("after the redirect, the first same-line HS AMO refetches the refreshed cold-line payload and makes forward progress immediately",
        recovery_old_value == refreshed
    );

    TEST_SETUP_EXCEPT();
    uint64_t followup_old_value = amoadd_d(vaddr, followup_addend);
    AI_ASSERT_NO_EXCEPT_AND("a second same-line HS AMO right after the redirect-plus-cold-miss recovery sees only the first successful replay result, so no duplicate completion is visible",
        followup_old_value == after_recovery
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the redirected same-line HS AMO target returns the final accumulated value after the cold-miss replay subset",
        observed == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the redirect-plus-cold-miss replay subset",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the redirect-plus-cold-miss AMO replay subset commits exactly the two successful same-line addends while preserving the adjacent word",
        read64(paddr) == final_value &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_amo_redirect_then_miss_pressure_replay_no_duplicate_completion_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_hs_amo_pressure_pages) /
        sizeof(ai_micro_hs_amo_pressure_pages[0]);
    const uintptr_t probe_offset = 0xc0U;
    const uint64_t seed = 0x2200440066008800ULL;
    const uint64_t refreshed_seed = 0x9900aa00bb00cc00ULL;
    const uint64_t step = 0x101ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;
    const uint64_t fault_addend = 0x0303030303030303ULL;
    const uint64_t recovery_addend = 0x1414141414141414ULL;
    const uint64_t followup_addend = 0x2525252525252525ULL;
    const enum test_page target_page =
        ai_micro_hs_amo_pressure_pages[page_count - 1];
    const uintptr_t target_vaddr = hs_page_base(target_page) + probe_offset;
    const uintptr_t target_paddr = phys_page_base(target_page) + probe_offset;
    const uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t target_before = refreshed_seed + ((page_count - 1) * step);
    const uint64_t after_recovery = target_before + recovery_addend;
    const uint64_t final_value = after_recovery + followup_addend;
    const uintptr_t sample_paddr =
        phys_page_base(ai_micro_hs_amo_pressure_pages[0]) + probe_offset;
    const uint64_t sample_expected = refreshed_seed;

    goto_priv(PRIV_M);
    ai_micro_hs_amo_restore_pmp_full_access();
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_hs_amo_pressure_pages[i]) + probe_offset,
            seed + (i * step));
    }
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t i = 0; i < page_count; ++i) {
        const uintptr_t vaddr =
            hs_page_base(ai_micro_hs_amo_pressure_pages[i]) + probe_offset;
        const uint64_t expected = seed + (i * step);

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(vaddr);
        AI_ASSERT_NO_EXCEPT_AND("each HS AMO pressure page is readable before the redirect-plus-miss-pressure replay sequence begins",
            warmed == expected
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed cache line can be invalidated before the redirect-plus-miss-pressure replay sequence",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        write64(phys_page_base(ai_micro_hs_amo_pressure_pages[i]) + probe_offset,
            refreshed_seed + (i * step));
        goto_priv(PRIV_HS);
    }

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target page's adjacent HS word is readable before the redirect-plus-miss-pressure replay subset starts",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(target_vaddr);
    TEST_ASSERT("the target cache line can be invalidated again after warming the adjacent word so the redirected replayed AMO still re-enters the miss-pressure path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_micro_hs_amo_make_leaf_access_fault(target_page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(target_vaddr, fault_addend);
    TEST_ASSERT("the leading HS AMO in the redirect-plus-miss-pressure subset reports store access fault when the target leaf is forced into access-fault mode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the redirected faulting AMO leaves the refreshed target line, its adjacent word, and a sampled non-target pressure page unchanged before the replayed AMOs run",
        read64(target_paddr) == target_before &&
        read64(adjacent_paddr) == adjacent &&
        read64(sample_paddr) == sample_expected
    );
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_hs_amo_pressure_pages[i]) + probe_offset);
        TEST_ASSERT("a pressure-building HS software prefetch remains silent while rebuilding miss pressure after the redirect",
            excpt.triggered == false
        );
    }

    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(target_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("after the redirect, the first same-line HS AMO still returns the refreshed old value and makes forward progress under miss pressure",
        recovery_old_value == target_before
    );

    TEST_SETUP_EXCEPT();
    uint64_t followup_old_value = amoadd_d(target_vaddr, followup_addend);
    AI_ASSERT_NO_EXCEPT_AND("a second same-line HS AMO after the redirect-plus-miss-pressure recovery sees exactly the first replay result, so no hidden rereceive or duplicate completion appears",
        followup_old_value == after_recovery
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(target_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the target HS word after the redirect-plus-miss-pressure replay subset returns the final accumulated value",
        observed == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the redirect-plus-miss-pressure replay subset",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the redirect-plus-miss-pressure AMO replay subset commits exactly the two successful same-line addends while preserving the adjacent word and the sampled non-target pressure page",
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent &&
        read64(sample_paddr) == sample_expected
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_amo_tlb_access_fault()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x1ULL);
    TEST_ASSERT("an HS AMO reports store access fault when the upper-level PTE encoding makes translation fault with AF semantics",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x2ULL);
    TEST_ASSERT("an HS AMO also reports store access fault when the leaf PTE encoding overflows during translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_amo_repeated_tlb_access_fault_stable_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x200U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x200U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122334455667788ULL;
    const uint64_t adjacent = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x0101010101010101ULL);
    TEST_ASSERT("a first HS AMO reports store access fault when the translation path repeatedly resolves to an upper-level PTE with overflowing PPN bits",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x0202020202020202ULL);
    TEST_ASSERT("repeating the same HS AMO against the same overflowing translation still reports store access fault with the same faulting virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated HS AMO translation access faults leave the original backing word and its adjacent word unchanged",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_tlb_access_fault_then_valid_access_recovers_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x220U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x220U;
    const uint64_t initial = 0x3141592653589793ULL;
    const uint64_t addend = 0x0101010101010101ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, addend);
    TEST_ASSERT("an HS AMO first reports store access fault when the selected leaf translation overflows its PPN encoding",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the translation access fault leaves the original backing word unchanged before the valid translation is restored",
        read64(paddr) == initial
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("after restoring a valid translation, retrying the same HS AMO succeeds immediately and returns the original value",
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the retried HS AMO commits its accumulated result once the valid translation is restored",
        read64(paddr) == (initial + addend)
    );

    TEST_END();
}

bool ai_micro_hs_amo_tlb_access_fault_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x230U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x230U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122334455667788ULL;
    const uint64_t adjacent = 0x8877665544332211ULL;
    const uint64_t addend = 0x0101010101010101ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, addend);
    TEST_ASSERT("an HS AMO first reports store access fault when the selected leaf translation overflows its PPN encoding in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the translation access fault leaves both the target backing word and its adjacent word unchanged before the valid translation is restored",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("after restoring a valid translation, retrying the same HS AMO succeeds immediately and still returns the original value in the adjacent-preservation corner",
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the retried HS AMO updates only the target backing word once the valid translation is restored while preserving the adjacent word",
        read64(paddr) == (initial + addend) &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_amo_page_fault()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSI_GI);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x1ULL);
    TEST_ASSERT("an HS AMO to a PTE.V=0 leaf reports store page fault with the original faulting virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_amo_repeated_page_fault_stable_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x40U;
    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x240U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x240U;
    const uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x2468ace013579bdfULL;
    const uint64_t adjacent = 0xfdb97531eca86420ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1111111111111111ULL);
    TEST_ASSERT("a first HS AMO to a leaf with PTE.V=0 reports store page fault with the original faulting virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x2222222222222222ULL);
    TEST_ASSERT("repeating the same HS AMO to the same invalid leaf still reports store page fault with the same faulting virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later valid HS load still observes its original payload after the repeated page-faulting AMO probes",
        observed == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated HS AMO page faults leave the later valid target and its adjacent backing word unchanged",
        read64(good_paddr) == good_initial &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_amo_page_fault_then_valid_access_recovers_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x80U;
    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x260U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x260U;
    const uint64_t initial = 0x0123456789abcdefULL;
    const uint64_t addend = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1111111111111111ULL);
    TEST_ASSERT("an HS AMO first reports store page fault when it targets an invalid translated page before the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(good_vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("a later HS AMO to a valid address still succeeds after the earlier page-faulting AMO probe",
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the valid-address recovery AMO commits its accumulated result after the earlier page-faulting AMO probe",
        read64(good_paddr) == (initial + addend)
    );

    TEST_END();
}

bool ai_micro_hs_amo_page_fault_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x88U;
    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x270U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x270U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t initial = 0x0123456789abcdefULL;
    const uint64_t adjacent = 0xfedcba9876543210ULL;
    const uint64_t addend = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    write64(good_adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1111111111111111ULL);
    TEST_ASSERT("an HS AMO first reports store page fault when it targets an invalid translated page before the valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(good_vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("a later HS AMO to a valid address still succeeds after the earlier page-faulting AMO probe in the adjacent-preservation corner",
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the earlier page-faulting AMO does not pollute the valid recovery target, and the successful recovery AMO updates only the target backing word while preserving the adjacent word",
        read64(good_paddr) == (initial + addend) &&
        read64(good_adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_atomic_redirect_midflight_no_stale_src2_carryover_corner()
{
    TEST_START();

    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0xc0U;
    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x278U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x278U;
    const uint64_t initial = 0x1021324354657687ULL;
    const uint64_t fault_addend = 0x0101010101010101ULL;
    const uint64_t recovery_addend = 0x1111111111111111ULL;
    const uint64_t followup_addend = 0x2222222222222222ULL;
    const uint64_t after_recovery = initial + recovery_addend;
    const uint64_t final_value = after_recovery + followup_addend;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, fault_addend);
    TEST_ASSERT("the first HS AMO in the src2-carryover diagnostic subset reports store page fault on the invalid target before the recovery AMO is issued",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the page-faulting AMO leaves the later valid recovery target unchanged before a new src2 value is provided",
        read64(good_paddr) == initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(good_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("after the fault-driven redirect, the next valid HS AMO uses only its new src2 and returns the original target value",
        recovery_old_value == initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t followup_old_value = amoadd_d(good_vaddr, followup_addend);
    AI_ASSERT_NO_EXCEPT_AND("a third HS AMO immediately after the recovery observes only the recovery result, showing the earlier faulting src2 did not leak into later atomics",
        followup_old_value == after_recovery
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fault-then-recovery AMO sequence commits exactly the recovery and follow-up addends, with no stale contribution from the faulting AMO src2",
        read64(good_paddr) == final_value
    );

    TEST_END();
}

bool ai_micro_hs_atomic_redirect_then_new_atomic_no_rereceive_or_wrong_result_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x288U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x288U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x3141592653589793ULL;
    const uint64_t adjacent = 0x8877665544332211ULL;
    const uint64_t fault_addend = 0x0303030303030303ULL;
    const uint64_t recovery_addend = 0x1212121212121212ULL;
    const uint64_t followup_addend = 0x2323232323232323ULL;
    const uint64_t after_recovery = initial + recovery_addend;
    const uint64_t final_value = after_recovery + followup_addend;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();
    hspt_init();
    ai_micro_hs_amo_make_leaf_access_fault(AI_MICRO_HS_AMO_PAGE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, fault_addend);
    TEST_ASSERT("the first HS AMO in the no-rereceive diagnostic subset reports store access fault when the leaf PTE is forced into access-fault mode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the leaf access-faulting AMO leaves both the target and adjacent backing words unchanged before the new atomic is issued",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("after restoring the valid leaf, the next HS AMO succeeds with its own src2 and returns the original value rather than a replayed or duplicated earlier result",
        recovery_old_value == initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t followup_old_value = amoadd_d(vaddr, followup_addend);
    AI_ASSERT_NO_EXCEPT_AND("a follow-up HS AMO on the same word sees exactly the first recovery result, indicating there was no hidden rereceive or duplicate completion from the earlier faulted atomic",
        followup_old_value == after_recovery
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the access-fault redirect subset commits only the two successful AMO addends while preserving the adjacent word, so no wrong result or duplicate atomic completion is visible",
        read64(paddr) == final_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_atomic_early_feedback_no_false_success_under_late_fault_corner()
{
    TEST_START();

    const uintptr_t aligned_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2c0U;
    const uintptr_t misaligned_vaddr = aligned_vaddr + 1U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2c0U;
    const uintptr_t adjacent_vaddr = aligned_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x0123456789abcdefULL;
    const uint64_t adjacent = 0xfedcba9876543210ULL;
    const uint64_t fault_addend = 0x0101010101010101ULL;
    const uint64_t store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t recovery_addend = 0x1111111111111111ULL;
    const uint64_t final_value = store_value + recovery_addend;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(misaligned_vaddr, fault_addend);
    TEST_ASSERT("the leading misaligned HS AMO in the early-feedback diagnostic subset reports store access fault with the raw misaligned address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the misaligned faulting AMO leaves both the target word and its adjacent word unchanged before younger memory operations execute",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(aligned_vaddr, store_value);
    AI_ASSERT_NO_EXCEPT_AND("a younger aligned HS store after the faulting AMO retires normally, so the later AMO sees architecturally valid store-side state rather than a false prior atomic success",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(aligned_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("the recovery HS AMO returns the younger store payload exactly, showing the earlier faulting atomic did not become a hidden success through optimistic feedback",
        recovery_old_value == store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(aligned_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the aligned HS target after the store-plus-recovery-AMO sequence returns the expected accumulated final value",
        observed == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the early-feedback false-success diagnostic subset",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the early-feedback false-success diagnostic subset commits only the younger store plus the later successful AMO while preserving the adjacent word",
        read64(paddr) == final_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_atomic_early_feedback_no_order_break_under_replay_corner()
{
    TEST_START();

    const uintptr_t aligned_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2e0U;
    const uintptr_t misaligned_vaddr = aligned_vaddr + 1U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2e0U;
    const uintptr_t adjacent_vaddr = aligned_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1021324354657687ULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;
    const uint64_t first_fault_addend = 0x0202020202020202ULL;
    const uint64_t second_fault_addend = 0x0303030303030303ULL;
    const uint64_t store_value = 0x8899aabbccddeeffULL;
    const uint64_t recovery_addend = 0x1212121212121212ULL;
    const uint64_t followup_addend = 0x2323232323232323ULL;
    const uint64_t after_recovery = store_value + recovery_addend;
    const uint64_t final_value = after_recovery + followup_addend;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(misaligned_vaddr, first_fault_addend);
    TEST_ASSERT("the first misaligned HS AMO in the early-feedback replay-order subset reports store access fault with the raw misaligned address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(misaligned_vaddr, second_fault_addend);
    TEST_ASSERT("the second misaligned HS AMO in the same subset still reports store access fault with the same raw misaligned address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated misaligned faulting AMOs leave both the target word and the adjacent word unchanged before later aligned operations execute",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(aligned_vaddr, store_value);
    AI_ASSERT_NO_EXCEPT_AND("after the repeated faulting AMOs, a younger aligned HS store still retires normally instead of being blocked by a false already-succeeded atomic state",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(aligned_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first successful aligned HS AMO after the repeated faults observes exactly the younger store payload",
        recovery_old_value == store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t followup_old_value = amoadd_d(aligned_vaddr, followup_addend);
    AI_ASSERT_NO_EXCEPT_AND("the next successful aligned HS AMO observes exactly the previous aligned recovery result, showing there was no hidden duplicate or reordered completion from the earlier faulting AMOs",
        followup_old_value == after_recovery
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(aligned_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the aligned HS target after the repeated-fault replay-order subset returns the final accumulated value from only the successful younger operations",
        observed == final_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the early-feedback replay-order subset",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the early-feedback replay-order subset preserves ordering by committing only the younger aligned store and the two later successful aligned AMOs while preserving the adjacent word",
        read64(paddr) == final_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_amo_pmp_write_access_fault()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE);
    uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE);
    const uint64_t initial_value = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        paddr, paddr + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x0101010101010101ULL);
    TEST_ASSERT("an HS AMO reports store access fault when translation succeeds but PMP denies the write side of the atomic access",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the failed HS AMO leaves the backing memory unchanged when PMP rejects the write permission",
        read64(paddr) == initial_value
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_amo_pmp_write_access_fault_repeated_stable_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x280U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x280U;
    const uint64_t initial = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x0101010101010101ULL);
    TEST_ASSERT("a first HS AMO reports store access fault when PMP repeatedly denies the write side of the translated atomic access",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x0202020202020202ULL);
    TEST_ASSERT("repeating the same HS AMO while the PMP deny window remains active still reports store access fault with the same translated address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated HS AMO PMP write faults leave the backing word unchanged",
        read64(paddr) == initial
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_amo_pmp_write_access_fault_then_restore_succeeds_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2a0U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2a0U;
    const uint64_t initial = 0x76543210fedcba98ULL;
    const uint64_t addend = 0x1111111111111111ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, addend);
    TEST_ASSERT("an HS AMO first reports store access fault when PMP denies writes to the selected translated page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied HS AMO leaves the backing word unchanged before PMP permissions are restored",
        read64(paddr) == initial
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("after restoring PMP access, retrying the same HS AMO succeeds immediately without rebuilding the translation context",
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the retried HS AMO commits its accumulated result once PMP permissions are restored",
        read64(paddr) == (initial + addend)
    );

    TEST_END();
}

bool ai_micro_hs_amo_pmp_write_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x300U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x300U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1021324354657687ULL;
    const uint64_t adjacent = 0xf0e1d2c3b4a59687ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x0101010101010101ULL);
    TEST_ASSERT("an HS AMO reports store access fault when PMP denies the write side of the atomic access in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied HS AMO leaves both the target backing word and its adjacent word unchanged when PMP rejects the write permission",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_amo_pmp_write_access_fault_then_restore_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x320U;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x320U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x89abcdef01234567ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;
    const uint64_t addend = 0x1010101010101010ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, addend);
    TEST_ASSERT("an HS AMO first reports store access fault when PMP denies writes to the selected translated page in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied HS AMO leaves both the target backing word and its adjacent word unchanged before PMP permissions are restored",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("after restoring PMP access, retrying the same HS AMO succeeds immediately and still returns the original value in the adjacent-preservation corner",
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the retried HS AMO updates only the target backing word once PMP permissions are restored while preserving the adjacent word",
        read64(paddr) == (initial + addend) &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_amo_pbmt_nonzero_access_fault()
{
    TEST_START();

    uintptr_t backing_paddr = ai_micro_hs_amo_pbmt_backing_paddr();
    uintptr_t io_backing_paddr = ai_micro_hs_amo_pbmt_io_backing_paddr();
    uintptr_t nc_vaddr = ai_micro_hs_amo_prepare_pbmt_alias(AI_MICRO_HS_AMO_PBMT_NC_PAGE);
    const uint64_t nc_initial = 0x1021324354657687ULL;
    const uint64_t io_initial = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    write64(backing_paddr, nc_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(nc_vaddr, 0x0101010101010101ULL);
    TEST_ASSERT("an HS AMO on a PBMT=NC alias reports store access fault once translation returns PBMT!=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == nc_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PBMT=NC AMO fault leaves the shared backing memory unchanged",
        read64(backing_paddr) == nc_initial
    );

    uintptr_t io_vaddr = ai_micro_hs_amo_prepare_pbmt_io_alias(AI_MICRO_HS_AMO_PBMT_IO_PAGE);

    goto_priv(PRIV_M);
    write64(io_backing_paddr, io_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(io_vaddr, 0x0202020202020202ULL);
    TEST_ASSERT("an HS AMO on a PBMT=IO alias also reports store access fault once translation returns PBMT!=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == io_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PBMT=IO AMO fault also leaves the shared backing memory unchanged",
        read64(io_backing_paddr) == io_initial
    );
    ai_micro_hs_amo_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_amo_mmio_access_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_hs_amo_mmio_backing_paddr();
    uintptr_t vaddr = ai_micro_hs_amo_prepare_mmio_alias(
        AI_MICRO_HS_AMO_MMIO_ALIAS_PAGE, AI_MICRO_HS_AMO_MMIO_BACKING_PAGE);
    const uint64_t initial_value = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x1111111111111111ULL);
    TEST_ASSERT("an HS AMO reports store access fault when translation succeeds but PMA classifies the target as Device/MMIO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMA=Device AMO fault leaves the shared backing memory unchanged",
        read64(paddr) == initial_value
    );
    ai_micro_hs_amo_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_amo_mmio_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t bad_paddr = ai_micro_hs_amo_mmio_backing_paddr() + 0x40U;
    const uintptr_t bad_adjacent_paddr = bad_paddr + sizeof(uint64_t);
    const uint64_t bad_initial = 0x13579bdf2468ace0ULL;
    const uint64_t bad_adjacent = 0x0f1e2d3c4b5a6978ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(bad_paddr, bad_initial);
    write64(bad_adjacent_paddr, bad_adjacent);

    bad_vaddr = ai_micro_hs_amo_prepare_mmio_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_MMIO_BACKING_PAGE) + 0x40U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x0101010101010101ULL);
    TEST_ASSERT("an HS AMO reports store access fault when translation succeeds but PMA classifies the target as Device/MMIO in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMA=Device AMO fault leaves both the faulting backing word and its adjacent word unchanged",
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent
    );
    ai_micro_hs_amo_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_amo_mmio_access_fault_repeated_stable_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x340U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x340U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t bad_paddr = ai_micro_hs_amo_mmio_backing_paddr() + 0x80U;
    const uintptr_t bad_adjacent_paddr = bad_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x2718281828459045ULL;
    const uint64_t good_adjacent = 0x89abcdef01234567ULL;
    const uint64_t good_addend = 0x0101010101010101ULL;
    const uint64_t bad_initial = 0x3141592653589793ULL;
    const uint64_t bad_adjacent = 0xfedcba9876543210ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(bad_paddr, bad_initial);
    write64(bad_adjacent_paddr, bad_adjacent);

    bad_vaddr = ai_micro_hs_amo_prepare_mmio_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_MMIO_BACKING_PAGE) + 0x80U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1112131415161718ULL);
    TEST_ASSERT("a first HS AMO reports store access fault when the translated target repeatedly resolves to PMA=Device/MMIO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x2122232425262728ULL);
    TEST_ASSERT("repeating the same HS AMO against the same PMA=Device alias still reports store access fault with the same translated address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated PMA=Device HS AMO faults leave both the faulting word pair and the later valid target word pair unchanged before recovery",
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent &&
        read64(good_paddr) == good_initial &&
        read64(good_adjacent_paddr) == good_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later HS AMO to a valid cacheable address still succeeds after repeated PMA=Device AMO faults",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated PMA=Device faults, the valid recovery AMO updates only its target word while preserving the adjacent word and leaving the faulting word pair unchanged",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent &&
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent
    );
    ai_micro_hs_amo_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_amo_mmio_access_fault_then_valid_access_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x360U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x360U;
    const uintptr_t bad_paddr = ai_micro_hs_amo_mmio_backing_paddr() + 0x100U;
    const uint64_t good_initial = 0x0123456789abcdefULL;
    const uint64_t good_addend = 0x0202020202020202ULL;
    const uint64_t bad_initial = 0x76543210fedcba98ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(bad_paddr, bad_initial);

    bad_vaddr = ai_micro_hs_amo_prepare_mmio_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_MMIO_BACKING_PAGE) + 0x100U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1111111111111111ULL);
    TEST_ASSERT("an HS AMO first reports store access fault when the translated target resolves to PMA=Device/MMIO before the valid-address recovery AMO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later HS AMO to a valid cacheable address still succeeds after the earlier PMA=Device AMO fault",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the valid-address recovery AMO commits its accumulated result after the earlier PMA=Device AMO fault while the faulting MMIO-backed word remains unchanged",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(bad_paddr) == bad_initial
    );
    ai_micro_hs_amo_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_amo_mmio_access_fault_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x388U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x388U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t bad_paddr = ai_micro_hs_amo_mmio_backing_paddr() + 0x140U;
    const uintptr_t bad_adjacent_paddr = bad_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x0123456789abcdefULL;
    const uint64_t good_adjacent = 0x89abcdef01234567ULL;
    const uint64_t good_addend = 0x0202020202020202ULL;
    const uint64_t bad_initial = 0x76543210fedcba98ULL;
    const uint64_t bad_adjacent = 0x0f1e2d3c4b5a6978ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(bad_paddr, bad_initial);
    write64(bad_adjacent_paddr, bad_adjacent);

    bad_vaddr = ai_micro_hs_amo_prepare_mmio_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_MMIO_BACKING_PAGE) + 0x140U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1111111111111111ULL);
    TEST_ASSERT("an HS AMO first reports store access fault when the translated target resolves to PMA=Device/MMIO before the valid-address recovery AMO in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later HS AMO to a valid cacheable address still succeeds after the earlier PMA=Device AMO fault in the adjacent-preservation corner",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMA=Device fault leaves the faulting backing word pair unchanged, and the valid recovery AMO updates only its target word while preserving the adjacent word",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent &&
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent
    );
    ai_micro_hs_amo_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_amo_bus_error_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_hs_amo_prepare_bad_paddr_alias(
        AI_MICRO_HS_AMO_PAGE, AI_MICRO_HS_AMO_BAD_PADDR);

    goto_priv(PRIV_M);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0x0102030405060708ULL);
    TEST_ASSERT("an HS AMO reports store access fault when translation succeeds but the cache request returns a bus-error-class physical access failure",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_bus_error_access_fault_repeated_stable_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2c0U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2c0U;
    const uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    bad_vaddr = ai_micro_hs_amo_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x0102030405060708ULL);
    TEST_ASSERT("a first HS AMO reports store access fault when the translated cache request repeatedly hits a bus-error-class physical failure",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1112131415161718ULL);
    TEST_ASSERT("repeating the same HS AMO against the same bad-physical alias still reports store access fault with the same translated address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later valid HS load still observes its original payload after the repeated bus-error-class AMO probes",
        observed == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated bus-error-class HS AMO faults leave the later valid target and its adjacent backing word unchanged",
        read64(good_paddr) == good_initial &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_bus_error_access_fault_then_valid_access_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2e0U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2e0U;
    const uint64_t initial = 0x2718281828459045ULL;
    const uint64_t addend = 0x0101010101010101ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    bad_vaddr = ai_micro_hs_amo_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1112131415161718ULL);
    TEST_ASSERT("an HS AMO first reports store access fault when the translated cache request hits a bus-error-class physical failure before the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(good_vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("a later HS AMO to a valid address still succeeds after the earlier bus-error-class AMO probe",
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the valid-address recovery AMO commits its accumulated result after the earlier bus-error-class AMO probe",
        read64(good_paddr) == (initial + addend)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_bus_error_access_fault_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x3a0U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x3a0U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t initial = 0x2718281828459045ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;
    const uint64_t addend = 0x0101010101010101ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, initial);
    write64(good_adjacent_paddr, adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    bad_vaddr = ai_micro_hs_amo_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1112131415161718ULL);
    TEST_ASSERT("an HS AMO first reports store access fault when the translated cache request hits a bus-error-class physical failure before the valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(good_vaddr, addend);
    AI_ASSERT_NO_EXCEPT_AND("a later HS AMO to a valid address still succeeds after the earlier bus-error-class AMO probe in the adjacent-preservation corner",
        old_value == initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the valid-address recovery AMO updates only its target backing word after the earlier bus-error-class probe while preserving the adjacent word",
        read64(good_paddr) == (initial + addend) &&
        read64(good_adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_last_instruction_page_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE);
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) +
        (2U * sizeof(uint64_t));
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) +
        (2U * sizeof(uint64_t));
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI);
    const uint64_t good_initial = 0x1122334455667788ULL;
    const uint64_t good_addend = 0x0101010101010101ULL;
    const uint64_t recovery_initial = 0x8877665544332211ULL;
    const uint64_t recovery_addend = 0x0202020202020202ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO in the consecutive pair still returns the old value and retires cleanly before the later page-faulting AMO",
        good_old_value == good_initial
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1111111111111111ULL);
    TEST_ASSERT("the last AMO in the consecutive pair reports store page fault when its translated page is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier AMO still commits its accumulated result even though the later consecutive AMO faults",
        read64(good_paddr) == (good_initial + good_addend)
    );
    TEST_ASSERT("the separate recovery target remains unchanged before the post-fault recovery AMO runs",
        read64(recovery_paddr) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later AMO still retires normally after the consecutive sequence ends with a page-faulting AMO",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-fault recovery AMO commits its accumulated result to the follow-on target",
        read64(recovery_paddr) == (recovery_initial + recovery_addend)
    );

    TEST_END();
}

bool ai_micro_hs_amo_first_instruction_page_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE);
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) +
        (2U * sizeof(uint64_t));
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) +
        (2U * sizeof(uint64_t));
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI);
    const uint64_t good_initial = 0x02468ace13579bdfULL;
    const uint64_t good_addend = 0x1111111111111111ULL;
    const uint64_t recovery_initial = 0xfdb97531eca86420ULL;
    const uint64_t recovery_addend = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x2222222222222222ULL);
    TEST_ASSERT("the first AMO in the consecutive pair reports store page fault when its translated page is invalid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the later AMO in the same consecutive sequence still returns the old value and retires successfully after the earlier page fault",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful later AMO still commits its accumulated result after the earlier AMO fault",
        read64(good_paddr) == (good_initial + good_addend)
    );
    TEST_ASSERT("the separate recovery target keeps its seeded payload before the follow-on recovery AMO runs",
        read64(recovery_paddr) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on AMO still retires normally after a consecutive sequence that begins with a page-faulting AMO",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the follow-on recovery AMO commits its accumulated result after the earlier fault-first sequence",
        read64(recovery_paddr) == (recovery_initial + recovery_addend)
    );

    TEST_END();
}

bool ai_micro_hs_amo_last_instruction_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x340U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x340U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x360U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x360U;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x100U;
    const uint64_t good_initial = 0x1122334455667788ULL;
    const uint64_t good_adjacent = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t good_addend = 0x0101010101010101ULL;
    const uint64_t recovery_initial = 0x8877665544332211ULL;
    const uint64_t recovery_adjacent = 0xfedcba9876543210ULL;
    const uint64_t recovery_addend = 0x0202020202020202ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO in the consecutive pair still returns the old value and retires cleanly before the later page-faulting AMO in the adjacent-preservation corner",
        good_old_value == good_initial
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1111111111111111ULL);
    TEST_ASSERT("the last AMO in the consecutive pair reports store page fault when its translated page is invalid in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier AMO still commits its accumulated result while preserving its adjacent word even though the later consecutive AMO page-faults",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent
    );
    TEST_ASSERT("the separate recovery target and its adjacent word remain unchanged before the post-page-fault recovery AMO runs",
        read64(recovery_paddr) == recovery_initial &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later AMO still retires normally after the consecutive sequence ends with a page-faulting AMO in the adjacent-preservation corner",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-page-fault recovery AMO commits its accumulated result while preserving its adjacent word",
        read64(recovery_paddr) == (recovery_initial + recovery_addend) &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_amo_first_instruction_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x380U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x380U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x3a0U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x3a0U;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    const uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x140U;
    const uint64_t good_initial = 0x02468ace13579bdfULL;
    const uint64_t good_adjacent = 0x55aa33cc77ee11ffULL;
    const uint64_t good_addend = 0x1111111111111111ULL;
    const uint64_t recovery_initial = 0xfdb97531eca86420ULL;
    const uint64_t recovery_adjacent = 0x1122334455667788ULL;
    const uint64_t recovery_addend = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x2222222222222222ULL);
    TEST_ASSERT("the first AMO in the consecutive pair reports store page fault when its translated page is invalid in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the later AMO in the same consecutive sequence still returns the old value and retires successfully after the earlier page fault in the adjacent-preservation corner",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful later AMO still commits its accumulated result while preserving its adjacent word after the earlier page fault",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent
    );
    TEST_ASSERT("the separate recovery target and its adjacent word keep their seeded payloads before the follow-on recovery AMO runs",
        read64(recovery_paddr) == recovery_initial &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on AMO still retires normally after a consecutive sequence that begins with a page-faulting AMO in the adjacent-preservation corner",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the follow-on recovery AMO commits its accumulated result while preserving its adjacent word",
        read64(recovery_paddr) == (recovery_initial + recovery_addend) &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_amo_last_instruction_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE);
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) +
        (2U * sizeof(uint64_t));
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) +
        (2U * sizeof(uint64_t));
    const uintptr_t bad_vaddr = hs_page_base(VSRW_GRWX);
    const uintptr_t bad_paddr = phys_page_base(VSRW_GRWX);
    const uint64_t good_initial = 0x5566778899aabbccULL;
    const uint64_t good_addend = 0x0101010101010101ULL;
    const uint64_t recovery_initial = 0x2468ace013579bdfULL;
    const uint64_t recovery_addend = 0x020406080a0c0e10ULL;
    const uint64_t bad_initial = 0x0badf00d12345678ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    write64(bad_paddr, bad_initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        bad_paddr, bad_paddr + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO in the consecutive pair still returns the old value and retires cleanly before the later SAFing AMO",
        good_old_value == good_initial
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x3333333333333333ULL);
    TEST_ASSERT("the last AMO in the consecutive pair reports store access fault when PMP denies the translated write side",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier AMO still commits its accumulated result even though the later consecutive AMO raises SAF",
        read64(good_paddr) == (good_initial + good_addend)
    );
    TEST_ASSERT("the faulting AMO leaves the denied target unchanged when the later consecutive AMO raises SAF",
        read64(bad_paddr) == bad_initial
    );
    TEST_ASSERT("the separate recovery target remains unchanged before the post-SAF recovery AMO runs",
        read64(recovery_paddr) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later AMO still retires normally after the consecutive sequence ends with a SAFing AMO",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-SAF recovery AMO commits its accumulated result to the follow-on target",
        read64(recovery_paddr) == (recovery_initial + recovery_addend)
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_amo_first_instruction_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE);
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) +
        (2U * sizeof(uint64_t));
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) +
        (2U * sizeof(uint64_t));
    const uintptr_t bad_vaddr = hs_page_base(VSRW_GRWX);
    const uintptr_t bad_paddr = phys_page_base(VSRW_GRWX);
    const uint64_t good_initial = 0x89abcdef01234567ULL;
    const uint64_t good_addend = 0x1111111111111111ULL;
    const uint64_t recovery_initial = 0x76543210fedcba98ULL;
    const uint64_t recovery_addend = 0x0102030405060708ULL;
    const uint64_t bad_initial = 0xfeedfacecafebeefULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    write64(bad_paddr, bad_initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        bad_paddr, bad_paddr + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x4444444444444444ULL);
    TEST_ASSERT("the first AMO in the consecutive pair reports store access fault when PMP denies the translated write side",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the later AMO in the same consecutive sequence still returns the old value and retires successfully after the earlier SAFing AMO",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting first AMO leaves the denied target unchanged when the earlier consecutive AMO raises SAF",
        read64(bad_paddr) == bad_initial
    );
    TEST_ASSERT("the successful later AMO still commits its accumulated result after the earlier AMO raises SAF",
        read64(good_paddr) == (good_initial + good_addend)
    );
    TEST_ASSERT("the separate recovery target keeps its seeded payload before the follow-on post-SAF recovery AMO runs",
        read64(recovery_paddr) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on AMO still retires normally after a consecutive sequence that begins with a SAFing AMO",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-SAF follow-on AMO commits its accumulated result to the recovery target",
        read64(recovery_paddr) == (recovery_initial + recovery_addend)
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_amo_last_instruction_mmio_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x200U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x200U;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x210U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x210U;
    const uintptr_t bad_paddr = ai_micro_hs_amo_mmio_backing_paddr() + 0x180U;
    const uint64_t good_initial = 0x5566778899aabbccULL;
    const uint64_t good_addend = 0x0101010101010101ULL;
    const uint64_t recovery_initial = 0x2468ace013579bdfULL;
    const uint64_t recovery_addend = 0x020406080a0c0e10ULL;
    const uint64_t bad_initial = 0x0badf00d12345678ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    write64(bad_paddr, bad_initial);

    bad_vaddr = ai_micro_hs_amo_prepare_mmio_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_MMIO_BACKING_PAGE) + 0x180U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO in the consecutive pair still returns the old value and retires cleanly before the later PMA=Device SAFing AMO",
        good_old_value == good_initial
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x3333333333333333ULL);
    TEST_ASSERT("the last AMO in the consecutive pair reports store access fault when the translated target resolves to PMA=Device/MMIO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier AMO still commits its accumulated result even though the later consecutive AMO hits a PMA=Device SAF",
        read64(good_paddr) == (good_initial + good_addend)
    );
    TEST_ASSERT("the faulting PMA=Device AMO leaves the MMIO-backed target unchanged when the later consecutive AMO raises SAF",
        read64(bad_paddr) == bad_initial
    );
    TEST_ASSERT("the separate recovery target remains unchanged before the post-PMA-fault recovery AMO runs",
        read64(recovery_paddr) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later AMO still retires normally after the consecutive sequence ends with a PMA=Device SAFing AMO",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-PMA-fault recovery AMO commits its accumulated result to the follow-on target",
        read64(recovery_paddr) == (recovery_initial + recovery_addend)
    );
    ai_micro_hs_amo_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_amo_first_instruction_mmio_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x220U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x220U;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x230U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x230U;
    const uintptr_t bad_paddr = ai_micro_hs_amo_mmio_backing_paddr() + 0x1a0U;
    const uint64_t good_initial = 0x89abcdef01234567ULL;
    const uint64_t good_addend = 0x1111111111111111ULL;
    const uint64_t recovery_initial = 0x76543210fedcba98ULL;
    const uint64_t recovery_addend = 0x0102030405060708ULL;
    const uint64_t bad_initial = 0xfeedfacecafebeefULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    write64(bad_paddr, bad_initial);

    bad_vaddr = ai_micro_hs_amo_prepare_mmio_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_MMIO_BACKING_PAGE) + 0x1a0U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x4444444444444444ULL);
    TEST_ASSERT("the first AMO in the consecutive pair reports store access fault when the translated target resolves to PMA=Device/MMIO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the later AMO in the same consecutive sequence still returns the old value and retires successfully after the earlier PMA=Device SAFing AMO",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting first PMA=Device AMO leaves the MMIO-backed target unchanged when the earlier consecutive AMO raises SAF",
        read64(bad_paddr) == bad_initial
    );
    TEST_ASSERT("the successful later AMO still commits its accumulated result after the earlier PMA=Device SAF",
        read64(good_paddr) == (good_initial + good_addend)
    );
    TEST_ASSERT("the separate recovery target keeps its seeded payload before the follow-on post-PMA-fault AMO runs",
        read64(recovery_paddr) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on AMO still retires normally after a consecutive sequence that begins with a PMA=Device SAFing AMO",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-PMA-fault follow-on AMO commits its accumulated result to the recovery target",
        read64(recovery_paddr) == (recovery_initial + recovery_addend)
    );
    ai_micro_hs_amo_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_amo_last_instruction_mmio_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x240U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x240U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x260U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x260U;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    const uintptr_t bad_paddr = ai_micro_hs_amo_mmio_backing_paddr() + 0x1c0U;
    const uintptr_t bad_adjacent_paddr = bad_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x3141592653589793ULL;
    const uint64_t good_adjacent = 0x2718281828459045ULL;
    const uint64_t good_addend = 0x0101010101010101ULL;
    const uint64_t recovery_initial = 0x0123456789abcdefULL;
    const uint64_t recovery_adjacent = 0xfedcba9876543210ULL;
    const uint64_t recovery_addend = 0x0202020202020202ULL;
    const uint64_t bad_initial = 0x13579bdf2468ace0ULL;
    const uint64_t bad_adjacent = 0x0f1e2d3c4b5a6978ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);
    write64(bad_paddr, bad_initial);
    write64(bad_adjacent_paddr, bad_adjacent);

    bad_vaddr = ai_micro_hs_amo_prepare_mmio_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_MMIO_BACKING_PAGE) + 0x1c0U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO in the consecutive pair still returns the old value and retires cleanly before the later PMA=Device SAFing AMO in the adjacent-preservation corner",
        good_old_value == good_initial
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x0102030405060708ULL);
    TEST_ASSERT("the last AMO in the consecutive pair reports store access fault when the translated target resolves to PMA=Device/MMIO in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier AMO still commits its accumulated result while preserving its adjacent word even though the later consecutive AMO raises a PMA=Device SAF",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent
    );
    TEST_ASSERT("the faulting MMIO-backed target and its adjacent word both remain unchanged when the later consecutive AMO raises a PMA=Device SAF",
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent
    );
    TEST_ASSERT("the separate recovery target and its adjacent word remain unchanged before the post-PMA-fault recovery AMO runs",
        read64(recovery_paddr) == recovery_initial &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later AMO still retires normally after the consecutive sequence ends with a PMA=Device SAFing AMO in the adjacent-preservation corner",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-PMA-fault recovery AMO commits its accumulated result while preserving its adjacent word",
        read64(recovery_paddr) == (recovery_initial + recovery_addend) &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );
    ai_micro_hs_amo_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_amo_first_instruction_mmio_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x280U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x280U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2a0U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x2a0U;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    const uintptr_t bad_paddr = ai_micro_hs_amo_mmio_backing_paddr() + 0x1e0U;
    const uintptr_t bad_adjacent_paddr = bad_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x89abcdef01234567ULL;
    const uint64_t good_adjacent = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t good_addend = 0x1111111111111111ULL;
    const uint64_t recovery_initial = 0x76543210fedcba98ULL;
    const uint64_t recovery_adjacent = 0x1029384756abcdefULL;
    const uint64_t recovery_addend = 0x0102030405060708ULL;
    const uint64_t bad_initial = 0xfeedfacecafebeefULL;
    const uint64_t bad_adjacent = 0x8877665544332211ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);
    write64(bad_paddr, bad_initial);
    write64(bad_adjacent_paddr, bad_adjacent);

    bad_vaddr = ai_micro_hs_amo_prepare_mmio_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_MMIO_BACKING_PAGE) + 0x1e0U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1112131415161718ULL);
    TEST_ASSERT("the first AMO in the consecutive pair reports store access fault when the translated target resolves to PMA=Device/MMIO in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the later AMO in the same consecutive sequence still returns the old value and retires successfully after the earlier PMA=Device SAFing AMO in the adjacent-preservation corner",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting first MMIO-backed AMO leaves both the target and its adjacent word unchanged when the earlier consecutive AMO raises a PMA=Device SAF",
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent
    );
    TEST_ASSERT("the successful later AMO still commits its accumulated result while preserving its adjacent word after the earlier PMA=Device SAF",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent
    );
    TEST_ASSERT("the separate recovery target and its adjacent word keep their seeded payloads before the follow-on post-PMA-fault recovery AMO runs",
        read64(recovery_paddr) == recovery_initial &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on AMO still retires normally after a consecutive sequence that begins with a PMA=Device SAFing AMO in the adjacent-preservation corner",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-PMA-fault follow-on AMO commits its accumulated result while preserving its adjacent word",
        read64(recovery_paddr) == (recovery_initial + recovery_addend) &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );
    ai_micro_hs_amo_reset_mmio_env();

    TEST_END();
}

bool ai_micro_hs_amo_last_instruction_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x100U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x100U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x120U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x120U;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    const uintptr_t bad_vaddr = hs_page_base(VSRW_GRWX) + 0x100U;
    const uintptr_t bad_paddr = phys_page_base(VSRW_GRWX) + 0x100U;
    const uintptr_t bad_adjacent_paddr = bad_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x5566778899aabbccULL;
    const uint64_t good_adjacent = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t good_addend = 0x0101010101010101ULL;
    const uint64_t recovery_initial = 0x2468ace013579bdfULL;
    const uint64_t recovery_adjacent = 0xfedcba9876543210ULL;
    const uint64_t recovery_addend = 0x020406080a0c0e10ULL;
    const uint64_t bad_initial = 0x0badf00d12345678ULL;
    const uint64_t bad_adjacent = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);
    write64(bad_paddr, bad_initial);
    write64(bad_adjacent_paddr, bad_adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        bad_paddr, bad_paddr + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO in the consecutive pair still returns the old value and retires cleanly before the later SAFing AMO in the adjacent-preservation corner",
        good_old_value == good_initial
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x3333333333333333ULL);
    TEST_ASSERT("the last AMO in the consecutive pair reports store access fault when PMP denies the translated write side in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier AMO still commits its accumulated result while preserving its adjacent word even though the later consecutive AMO raises SAF",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent
    );
    TEST_ASSERT("the denied target and its adjacent word both remain unchanged when the later consecutive AMO raises SAF",
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent
    );
    TEST_ASSERT("the separate recovery target and its adjacent word remain unchanged before the post-SAF recovery AMO runs",
        read64(recovery_paddr) == recovery_initial &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later AMO still retires normally after the consecutive sequence ends with a SAFing AMO in the adjacent-preservation corner",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-SAF recovery AMO commits its accumulated result while preserving its adjacent word",
        read64(recovery_paddr) == (recovery_initial + recovery_addend) &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_amo_first_instruction_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x140U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x140U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x160U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x160U;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    const uintptr_t bad_vaddr = hs_page_base(VSRW_GRWX) + 0x140U;
    const uintptr_t bad_paddr = phys_page_base(VSRW_GRWX) + 0x140U;
    const uintptr_t bad_adjacent_paddr = bad_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x89abcdef01234567ULL;
    const uint64_t good_adjacent = 0x55aa33cc77ee11ffULL;
    const uint64_t good_addend = 0x1111111111111111ULL;
    const uint64_t recovery_initial = 0x76543210fedcba98ULL;
    const uint64_t recovery_adjacent = 0x1122334455667788ULL;
    const uint64_t recovery_addend = 0x0102030405060708ULL;
    const uint64_t bad_initial = 0xfeedfacecafebeefULL;
    const uint64_t bad_adjacent = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);
    write64(bad_paddr, bad_initial);
    write64(bad_adjacent_paddr, bad_adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_hs_amo_configure_pmp_window(
        bad_paddr, bad_paddr + PAGE_SIZE, AI_MICRO_HS_AMO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x4444444444444444ULL);
    TEST_ASSERT("the first AMO in the consecutive pair reports store access fault when PMP denies the translated write side in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the later AMO in the same consecutive sequence still returns the old value and retires successfully after the earlier SAFing AMO in the adjacent-preservation corner",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied target and its adjacent word both remain unchanged when the earlier consecutive AMO raises SAF",
        read64(bad_paddr) == bad_initial &&
        read64(bad_adjacent_paddr) == bad_adjacent
    );
    TEST_ASSERT("the successful later AMO still commits its accumulated result while preserving its adjacent word after the earlier AMO raises SAF",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent
    );
    TEST_ASSERT("the separate recovery target and its adjacent word keep their seeded payloads before the follow-on post-SAF recovery AMO runs",
        read64(recovery_paddr) == recovery_initial &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on AMO still retires normally after a consecutive sequence that begins with a SAFing AMO in the adjacent-preservation corner",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-SAF follow-on AMO commits its accumulated result while preserving its adjacent word",
        read64(recovery_paddr) == (recovery_initial + recovery_addend) &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );
    ai_micro_hs_amo_restore_pmp_full_access();

    TEST_END();
}

bool ai_micro_hs_amo_last_instruction_bus_error_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0xc0U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0xc0U;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0xd0U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0xd0U;
    const uint64_t good_initial = 0x3141592653589793ULL;
    const uint64_t good_addend = 0x0101010101010101ULL;
    const uint64_t recovery_initial = 0x2718281828459045ULL;
    const uint64_t recovery_addend = 0x0202020202020202ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    uintptr_t bad_vaddr = ai_micro_hs_amo_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO in the consecutive pair still returns the old value and retires cleanly before the later bus-error AMO",
        good_old_value == good_initial
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x0102030405060708ULL);
    TEST_ASSERT("the last AMO in the consecutive pair reports store access fault when the translated cache request hits a bus-error-class physical failure",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier AMO still commits its accumulated result even though the later consecutive AMO raises a bus-error-class SAF",
        read64(good_paddr) == (good_initial + good_addend)
    );
    TEST_ASSERT("the separate recovery target remains unchanged before the post-bus-error recovery AMO runs",
        read64(recovery_paddr) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later AMO still retires normally after the consecutive sequence ends with a bus-error-class SAFing AMO",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-bus-error recovery AMO commits its accumulated result to the follow-on target",
        read64(recovery_paddr) == (recovery_initial + recovery_addend)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_last_instruction_bus_error_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x180U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x180U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x1a0U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x1a0U;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x3141592653589793ULL;
    const uint64_t good_adjacent = 0x2718281828459045ULL;
    const uint64_t good_addend = 0x0101010101010101ULL;
    const uint64_t recovery_initial = 0x0123456789abcdefULL;
    const uint64_t recovery_adjacent = 0xfedcba9876543210ULL;
    const uint64_t recovery_addend = 0x0202020202020202ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    bad_vaddr = ai_micro_hs_amo_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_BAD_PADDR - sizeof(uint64_t));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the first AMO in the consecutive pair still returns the old value and retires cleanly before the later bus-error AMO in the adjacent-preservation corner",
        good_old_value == good_initial
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x0102030405060708ULL);
    TEST_ASSERT("the last AMO in the consecutive pair reports store access fault when the translated cache request hits a bus-error-class physical failure in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful earlier AMO still commits its accumulated result while preserving its adjacent word even though the later consecutive AMO raises a bus-error-class SAF",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent
    );
    TEST_ASSERT("the separate recovery target and its adjacent word remain unchanged before the post-bus-error recovery AMO runs",
        read64(recovery_paddr) == recovery_initial &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a later AMO still retires normally after the consecutive sequence ends with a bus-error-class SAFing AMO in the adjacent-preservation corner",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-bus-error recovery AMO commits its accumulated result while preserving its adjacent word",
        read64(recovery_paddr) == (recovery_initial + recovery_addend) &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_first_instruction_bus_error_access_fault_recovers_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0xe0U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0xe0U;
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0xf0U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0xf0U;
    const uint64_t good_initial = 0x0123456789abcdefULL;
    const uint64_t good_addend = 0x1111111111111111ULL;
    const uint64_t recovery_initial = 0xfedcba9876543210ULL;
    const uint64_t recovery_addend = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(recovery_paddr, recovery_initial);
    ai_micro_hs_amo_restore_pmp_full_access();

    uintptr_t bad_vaddr = ai_micro_hs_amo_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_BAD_PADDR);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1112131415161718ULL);
    TEST_ASSERT("the first AMO in the consecutive pair reports store access fault when the translated cache request hits a bus-error-class physical failure",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the later AMO in the same consecutive sequence still returns the old value and retires successfully after the earlier bus-error-class SAF",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful later AMO still commits its accumulated result after the earlier bus-error-class SAF",
        read64(good_paddr) == (good_initial + good_addend)
    );
    TEST_ASSERT("the separate recovery target keeps its seeded payload before the follow-on post-bus-error AMO runs",
        read64(recovery_paddr) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on AMO still retires normally after a consecutive sequence that begins with a bus-error-class SAFing AMO",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-bus-error follow-on AMO commits its accumulated result to the recovery target",
        read64(recovery_paddr) == (recovery_initial + recovery_addend)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_first_instruction_bus_error_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t good_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x1c0U;
    const uintptr_t good_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x1c0U;
    const uintptr_t good_adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(AI_MICRO_HS_AMO_PAGE) + 0x1e0U;
    const uintptr_t recovery_paddr = phys_page_base(AI_MICRO_HS_AMO_PAGE) + 0x1e0U;
    const uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    const uint64_t good_initial = 0x89abcdef01234567ULL;
    const uint64_t good_adjacent = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t good_addend = 0x1111111111111111ULL;
    const uint64_t recovery_initial = 0x76543210fedcba98ULL;
    const uint64_t recovery_adjacent = 0x1029384756abcdefULL;
    const uint64_t recovery_addend = 0x0102030405060708ULL;
    uintptr_t bad_vaddr;

    goto_priv(PRIV_M);
    write64(good_paddr, good_initial);
    write64(good_adjacent_paddr, good_adjacent);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);
    ai_micro_hs_amo_restore_pmp_full_access();

    bad_vaddr = ai_micro_hs_amo_prepare_bad_paddr_alias(
        VSRW_GRWX, AI_MICRO_HS_AMO_BAD_PADDR - sizeof(uint64_t));

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(bad_vaddr, 0x1112131415161718ULL);
    TEST_ASSERT("the first AMO in the consecutive pair reports store access fault when the translated cache request hits a bus-error-class physical failure in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t good_old_value = amoadd_d(good_vaddr, good_addend);
    AI_ASSERT_NO_EXCEPT_AND("the later AMO in the same consecutive sequence still returns the old value and retires successfully after the earlier bus-error-class SAF in the adjacent-preservation corner",
        good_old_value == good_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful later AMO still commits its accumulated result while preserving its adjacent word after the earlier bus-error-class SAF",
        read64(good_paddr) == (good_initial + good_addend) &&
        read64(good_adjacent_paddr) == good_adjacent
    );
    TEST_ASSERT("the separate recovery target and its adjacent word keep their seeded payloads before the follow-on post-bus-error AMO runs",
        read64(recovery_paddr) == recovery_initial &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_old_value = amoadd_d(recovery_vaddr, recovery_addend);
    AI_ASSERT_NO_EXCEPT_AND("a follow-on AMO still retires normally after a consecutive sequence that begins with a bus-error-class SAFing AMO in the adjacent-preservation corner",
        recovery_old_value == recovery_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-bus-error follow-on AMO commits its accumulated result while preserving its adjacent word",
        read64(recovery_paddr) == (recovery_initial + recovery_addend) &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
